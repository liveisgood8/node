#include "connection.h"

#include <QtSql/QSqlError>

#include "exception.h"


#define ACCESS_DRIVER "Microsoft Access Driver (*.mdb)"
#define MSSQL_DRIVER "SQL Server Native Client 11.0"


namespace db {

Type databaseType = Type::None;

void openEx(QSqlDatabase &db)
{
    if (!db.open())
    {
        throw AppException(db.lastError().text());
    }
}

QSqlDatabase toAccess(const QString &path,
    const QString &connName)
{
    Q_ASSERT(!path.isEmpty());

    QString odbcString = "Driver={" ACCESS_DRIVER "};"
        "DBQ=" + path + ";";

    auto db = QSqlDatabase::addDatabase("QODBC", connName);
    db.setDatabaseName(odbcString);

    openEx(db);

    databaseType = Type::Access;
    return db;
}

QSqlDatabase toMssql(const QString &server, const QString &login, const QString &pass,
    const QString &connName)
{
    Q_ASSERT(!server.isEmpty());
    Q_ASSERT(!login.isEmpty());
    Q_ASSERT(!pass.isEmpty());

    QString odbcString = "Driver={" MSSQL_DRIVER "};"
        "Server=" + server + ";"
        "Database=" + login + ";"
        "Uid=" + login + ";"
        "Pwd=" + pass + ";"
        "MARS_Connection=Yes;";

    auto db = QSqlDatabase::addDatabase("QODBC", connName);
    db.setDatabaseName(odbcString);

    openEx(db);

    databaseType = Type::Mssql;
    return db;
}

QSqlDatabase toOracleByTns(const QString &server, const QString &login, const QString &pass,
    const QString &connName)
{
    Q_ASSERT(!server.isEmpty());
    Q_ASSERT(!login.isEmpty());
    Q_ASSERT(!pass.isEmpty());

    auto db = QSqlDatabase::addDatabase("QOCI", connName);
    db.setDatabaseName(server);
    db.setUserName(login);
    db.setPassword(pass);

    openEx(db);

    databaseType = Type::Oracle;
    return db;
}

Type type() {
  return databaseType;
}

QSqlDatabase byOleDbConnectionParser(const OleDbConnectionParser &oleParser,
    const QString &connName)
{
    if (oleParser.type() == OleDbConnectionParser::Access)
    {
        return toAccess(oleParser.host(), connName);
    }
    else if (oleParser.type() == OleDbConnectionParser::Mssql)
    {
        return toMssql(oleParser.host(), oleParser.login(), oleParser.password(), connName);
    }
    else if (oleParser.type() == OleDbConnectionParser::OracleTns)
    {
        return toOracleByTns(oleParser.host(), oleParser.login(), oleParser.password(), connName);
    }
    else // OracleIp
    {
        throw std::runtime_error("Подключение к Oracle по IP не реализовано!");
    }
}

} // namespace db
