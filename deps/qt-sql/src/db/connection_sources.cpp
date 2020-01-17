#include "connection_sources.h"

#include <QTextCodec>
#include <QSettings>
#include <QProcessEnvironment>

#include <logger/easylogging++.h>


constexpr const char* kEnvConnectionStringKey = "CONNECTION_STRING";

namespace db {

QString environmentConnectionString()
{
  auto env = QProcessEnvironment::systemEnvironment();
  if (env.contains(kEnvConnectionStringKey)) {
    return env.value(kEnvConnectionStringKey);
  }

  return QString();
}

QString registryConnectionString()
{
  auto defaultTextCodec = QTextCodec::codecForName("Windows-1251");

  QSettings userConnection(
      "HKEY_CURRENT_USER\\Software\\НИИ ВН\\АРМ «Химик-аналитик»",
      QSettings::NativeFormat);

  auto userConnectionString = userConnection.value("Connect");
  if (!userConnectionString.isNull() &&
      !userConnectionString.toString().isEmpty()) {
    return userConnectionString.toString();
  }

  QSettings machineConnection(
      "HKEY_LOCAL_MACHINE\\Software\\НИИ ВН\\АРМ «Химик-аналитик»",
      QSettings::NativeFormat);

  auto machineConnectionString = machineConnection.value("Connect");
  if (!machineConnectionString.isNull() &&
      !machineConnectionString.toString().isEmpty()) {
    return machineConnectionString.toString();
  }

  LOG(WARNING) << "Connection string is empty";
  return QString();
}

} // namespace db
