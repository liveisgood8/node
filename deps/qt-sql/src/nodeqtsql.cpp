#include "nodeqtsql.h"

#include <thread>

#include <QCoreApplication>
#include <QtPlugin>

#include <logger/configuration.h>
#include <node.h>

#include "bindings/nodequery.h"
#include "db/connection.h"
#include "db/connection_sources.h"
#include "db/oledbconnectionparser.h"
#include "exception.h"

Q_IMPORT_PLUGIN(QODBCDriverPlugin);
Q_IMPORT_PLUGIN(QOCIDriverPlugin);

namespace nodeqtsql {

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

static bool isApplicationInit = false;

void qtMessageHandler(QtMsgType type,
                      const QMessageLogContext& context,
                      const QString& msg) {
  const QString message = QString("QT --- %1 (%2:%3, %4)")
                              .arg(msg)
                              .arg(context.file)
                              .arg(context.line)
                              .arg(context.function);

  switch (type) {
    case QtDebugMsg:
      qDebug() << msg;
      break;
    case QtInfoMsg:
      qInfo() << msg;
      LOG(INFO) << message;
      break;
    case QtWarningMsg:
      qWarning() << msg;
      LOG(WARNING) << message;
      break;
    case QtCriticalMsg:
      qCritical() << msg;
      LOG(ERROR) << "CRITICAL " << message;
      break;
    case QtFatalMsg:
      LOG(ERROR) << "FATAL " << message;
      qFatal(msg.toUtf8().constData());
      abort();
  }
}

db::OleDbConnectionParser makeOleDbParser() {
  const auto connectionString = db::registryConnectionString();

  return db::OleDbConnectionParser(connectionString);
}

void createCoreApplication() {
  if (!isApplicationInit)
  {
    int argc = 0;
    static QCoreApplication application(argc, nullptr);

    qInstallMessageHandler(qtMessageHandler);

    isApplicationInit = true;
  }
}

bool openConnection() {
  if (QSqlDatabase::database(db::makeThreadLocalConnectionName()).isOpen()) {
    return true;
  }

  createCoreApplication();

  const auto parser = makeOleDbParser();
  db::byOleDbConnectionParser(parser);

  return true;
}

void jsOpenConnection(const FunctionCallbackInfo<Value>& args) {
  try {
    bool result = openConnection();
    args.GetReturnValue().Set(result);
  } catch (const AppException& err) {
    auto isolate = args.GetIsolate();
    auto jsErrorString =
        String::NewFromUtf8(isolate, err.message().toUtf8().constData());

    isolate->ThrowException(Exception::Error(jsErrorString.ToLocalChecked()));
  }
}

void jsCloseConnection(const FunctionCallbackInfo<Value>& args) {
  Q_UNUSED(args)

  QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

void initAll(Local<Object> exports) {
  NODE_SET_METHOD(exports, "openConnection", &jsOpenConnection);
  NODE_SET_METHOD(exports, "closeConnection", &jsCloseConnection);
  bindings::NodeQuery::init(exports);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, initAll)

}  // namespace nodeqtsql
