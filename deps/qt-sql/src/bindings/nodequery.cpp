#include "nodequery.h"

#include <QDateTime>
#include <QVariant>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include "db/connection.h"
#include "logger/easylogging++.h"

namespace bindings {

using v8::Array;
using v8::BigInt;
using v8::Context;
using v8::Date;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Int32;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

bool validateDate(Isolate* isolate, int day, int month, int year) {
  if (day < 1 || day > 31) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateDate day must be in range [0, 31]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  if (month < 1 || month > 12) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateDate month must be in range [0, 12]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  if (year < 1900 || year > 3000) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateDate year must be in range [1900, 3000]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  return true;
}

bool validateTime(Isolate* isolate, int hours, int minutes, int seconds) {
  if (hours < 0 || hours > 24) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateTime hours must be in range [0, 24]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  if (minutes < 0 || minutes > 60) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateTime minutes must be in range [0, 60]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  if (seconds < 0 || seconds > 60) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::validateTime year must be in range [0, 60]");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return false;
  }

  return true;
}

NodeQuery::NodeQuery()
    : innerQuery(std::make_unique<QSqlQuery>(
          QSqlDatabase::database(db::makeThreadLocalConnectionName()))) {}

NodeQuery::~NodeQuery() {}

void NodeQuery::init(Local<Object> exports) {
  auto isolate = exports->GetIsolate();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, newObject);
  tpl->SetClassName(
      String::NewFromUtf8(isolate, "Query", NewStringType::kNormal)
          .ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "prepare", &prepare);
  NODE_SET_PROTOTYPE_METHOD(tpl, "execute", &execute);

  NODE_SET_PROTOTYPE_METHOD(tpl, "next", &next);
  NODE_SET_PROTOTYPE_METHOD(tpl, "previous", &previous);
  NODE_SET_PROTOTYPE_METHOD(tpl, "first", &first);
  NODE_SET_PROTOTYPE_METHOD(tpl, "last", &last);

  NODE_SET_PROTOTYPE_METHOD(tpl, "recordCount", &recordCount);
  NODE_SET_PROTOTYPE_METHOD(tpl, "fieldsCount", &fieldsCount);
  NODE_SET_PROTOTYPE_METHOD(tpl, "numRowsAffected", &numRowsAffected);
  NODE_SET_PROTOTYPE_METHOD(tpl, "lastInsertId", &lastInsertId);

  NODE_SET_PROTOTYPE_METHOD(tpl, "addParameter", &addParameter);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addDateParameter", &addDateParameter);
  NODE_SET_PROTOTYPE_METHOD(tpl, "addTimeParameter", &addTimeParameter);
  NODE_SET_PROTOTYPE_METHOD(tpl, "fieldValue", &fieldValue);

  NODE_SET_PROTOTYPE_METHOD(tpl, "lastError", &lastError);

  Local<Context> context = isolate->GetCurrentContext();
  exports
      ->Set(context,
            String::NewFromUtf8(isolate, "Query", NewStringType::kNormal)
                .ToLocalChecked(),
            tpl->GetFunction(context).ToLocalChecked())
      .FromJust();
}

QSqlQuery* NodeQuery::query() const {
  return innerQuery.get();
}

void NodeQuery::newObject(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new MyObject(...)`
    NodeQuery* obj = new NodeQuery();
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Query must call as constructor")
            .ToLocalChecked()));
  }
}

void NodeQuery::prepare(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();

  if (args.Length() < 1 || !args[0]->IsString()) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::prepare must receive sql query string");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  String::Utf8Value sqlRawString(isolate, args[0]);
  const QString sql(*sqlRawString);

  Q_ASSERT(!sql.isEmpty());

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->prepare(sql);

  LOG(INFO) << "NodeQuery::prepare: " << sql;

  args.GetReturnValue().Set(result);
}

void NodeQuery::execute(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->exec();

  args.GetReturnValue().Set(result);
}

void NodeQuery::next(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->next();

  args.GetReturnValue().Set(result);
}

void NodeQuery::previous(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->previous();

  args.GetReturnValue().Set(result);
}

void NodeQuery::first(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->first();

  args.GetReturnValue().Set(result);
}

void NodeQuery::last(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  bool result = obj->query()->last();

  args.GetReturnValue().Set(result);
}

void NodeQuery::recordCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  int recordCount = obj->query()->size();

  args.GetReturnValue().Set(recordCount);
}

void NodeQuery::fieldsCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  int fieldsCount = obj->query()->record().count();

  args.GetReturnValue().Set(fieldsCount);
}

void NodeQuery::numRowsAffected(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  int affectedRows = obj->query()->numRowsAffected();

  args.GetReturnValue().Set(affectedRows);
}

void NodeQuery::addParameter(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  if (args.Length() < 1) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::addParameter must receive value of parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  if (args[0]->IsUndefined() || args[0]->IsNull()) {
    obj->query()->addBindValue(QVariant());

    LOG(INFO) << "NodeQuery::addParameter: null";
  } else if (args[0]->IsInt32()) {
    auto value = args[0]->Int32Value(context).FromJust();
    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (int32): " << value;
  } else if (args[0]->IsBigInt()) {
    auto value = args[0]->ToBigInt(context).ToLocalChecked()->Int64Value();
    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (bigint): " << value;
  } else if (args[0]->IsUint32()) {
    auto value = args[0]->Uint32Value(context).FromJust();
    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (uint32): " << value;
  } else if (args[0]->IsNumber()) {
    auto value = args[0]->NumberValue(context).FromJust();
    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (double): " << value;
  } else if (args[0]->IsBoolean()) {
    auto value = args[0]->BooleanValue(isolate);
    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (boolean): " << value;
  } else if (args[0]->IsString()) {
    String::Utf8Value sqlRawString(isolate, args[0]);
    const QString value(*sqlRawString);

    obj->query()->addBindValue(value);

    LOG(INFO) << "NodeQuery::addParameter (string): " << value;
  } else if (args[0]->IsDate()) {
    const double msSinceEpoch =
        args[0].As<Date>()->NumberValue(context).FromJust();

    QDateTime dt;
    dt.setMSecsSinceEpoch(static_cast<qint64>(msSinceEpoch));

    obj->query()->addBindValue(dt);

    LOG(INFO) << "NodeQuery::addParameter (datetime ISO): "
              << dt.toString(Qt::ISODate);
  } else {
    auto errorJsString = String::NewFromUtf8(
        isolate, "NodeQuery::addParameter unknown parameter type");
    isolate->ThrowException(
        Exception::TypeError(errorJsString.ToLocalChecked()));
  }
}

void NodeQuery::addDateParameter(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  if (args.Length() < 3) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::addDateParamter must receive year, month, day");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[0]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive day "
                            "(number) as first parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[1]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive month "
                            "(number) as second parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[2]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive year "
                            "(number) as third parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  const int day = args[0]->Int32Value(context).FromJust();
  const int month = args[1]->Int32Value(context).FromJust();
  const int year = args[2]->Int32Value(context).FromJust();

  LOG(INFO) << "NodeQuery::addDateParamter: " << day << "." << month << "."
            << year;

  if (!validateDate(isolate, day, month, year)) {
    return;
  }

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());

  QDate date(year, month, day);
  obj->innerQuery->addBindValue(std::move(date));
}

void NodeQuery::addTimeParameter(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  if (args.Length() < 3) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::addDateParamter must receive year, month, day");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[0]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive hours "
                            "(number) as first parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[1]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive minutes "
                            "(number) as second parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  if (!args[2]->IsInt32()) {
    auto errorString =
        String::NewFromUtf8(isolate,
                            "NodeQuery::addDateParamter must receive seconds "
                            "(number) as third parameter");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  const int hours = args[0]->Int32Value(context).FromJust();
  const int minutes = args[1]->Int32Value(context).FromJust();
  const int seconds = args[2]->Int32Value(context).FromJust();

  if (!validateTime(isolate, hours, minutes, seconds)) {
    return;
  }

  LOG(INFO) << "NodeQuery::addTimeParameter: " << hours << ":" << minutes << ":"
            << seconds;

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());

  QTime time(hours, minutes, seconds);
  obj->innerQuery->addBindValue(std::move(time));
}

void NodeQuery::fieldValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto context = isolate->GetCurrentContext();

  if (args.Length() < 1 || !args[0]->IsInt32()) {
    auto errorString = String::NewFromUtf8(
        isolate, "NodeQuery::fieldValue must receive field index");
    isolate->ThrowException(Exception::TypeError(errorString.ToLocalChecked()));
    return;
  }

  auto fieldIndex = args[0]->Int32Value(context).FromJust();

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  auto value = obj->query()->value(fieldIndex);
  auto valueType = value.type();

  if (obj->query()->isNull(fieldIndex) || valueType == QVariant::Invalid) {
    args.GetReturnValue().SetNull();
  } else if (valueType == QVariant::Int) {
    args.GetReturnValue().Set(value.toInt());
  } else if (valueType == QVariant::LongLong) {
    args.GetReturnValue().Set(BigInt::New(isolate, value.toLongLong()));
  } else if (valueType == QVariant::UInt) {
    args.GetReturnValue().Set(value.toUInt());
  } else if (valueType == QVariant::Double) {
    args.GetReturnValue().Set(value.toDouble());
  } else if (valueType == QVariant::Bool) {
    args.GetReturnValue().Set(value.toBool());
  } else if (valueType == QVariant::String) {
    auto string =
        String::NewFromUtf8(isolate, value.toString().toUtf8().constData());
    args.GetReturnValue().Set(string.ToLocalChecked());
  } else if (valueType == QVariant::DateTime) {
    auto dt = value.toDateTime();
    auto msSinceEpoch = dt.toMSecsSinceEpoch();

    auto date = Date::New(context, static_cast<double>(msSinceEpoch));

    args.GetReturnValue().Set(date.ToLocalChecked());
  } else if (valueType == QVariant::ByteArray) {
    auto byteArray = value.toByteArray();

    Local<Array> jsByteArray = Array::New(isolate, byteArray.length());
    for (int i = 0; i < byteArray.length(); i++) {
      jsByteArray->Set(context, i, Int32::New(isolate, byteArray[i]));
    }

    args.GetReturnValue().Set(jsByteArray);
  } else {
    auto errorString = "NodeQuery::fieldValue unknown field type - " +
                       QString::number(valueType);

    auto errorJsString =
        String::NewFromUtf8(isolate, errorString.toUtf8().constData());
    isolate->ThrowException(
        Exception::TypeError(errorJsString.ToLocalChecked()));
  }
}

void NodeQuery::lastError(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();

  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());
  auto errorText = obj->query()->lastError().text();

  auto jsErrorString =
      String::NewFromUtf8(isolate, errorText.toUtf8().constData());

  args.GetReturnValue().Set(jsErrorString.ToLocalChecked());
}

void NodeQuery::lastInsertId(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  auto obj = ObjectWrap::Unwrap<NodeQuery>(args.Holder());

  if (obj->query()->driver()->hasFeature(
          QSqlDriver::DriverFeature::LastInsertId)) {
    const auto lastInsertId = obj->query()->lastInsertId();
    args.GetReturnValue().Set(lastInsertId.isValid() ? lastInsertId.toInt()
                                                     : -1);

    return;
  }

  args.GetReturnValue().Set(-1);
}

}  // namespace bindings
