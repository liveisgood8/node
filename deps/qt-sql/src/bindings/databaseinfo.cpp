#include "databaseinfo.h"

#include "../db/connection.h"


namespace bindings {

using v8::Local;
using v8::Object;
using v8::Value;
using v8::Null;
using v8::Isolate;
using v8::String;
using v8::PropertyAttribute;
using v8::EscapableHandleScope;

void NodeDatabaseInfo::init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "type", &databaseType);
}

void NodeDatabaseInfo::databaseType(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();

  const auto type = db::type();
  switch (type) {
    case db::Type::Access:
      args.GetReturnValue().Set(
          String::NewFromUtf8(isolate, "access").ToLocalChecked());
      break;
    case db::Type::Mssql:
      args.GetReturnValue().Set(
          String::NewFromUtf8(isolate, "mssql").ToLocalChecked());
      break;
    case db::Type::Oracle:
      args.GetReturnValue().Set(
          String::NewFromUtf8(isolate, "oracle").ToLocalChecked());
      break;
    default:
      args.GetReturnValue().SetNull();
      break;
  }
}

}  // namespace bindings
