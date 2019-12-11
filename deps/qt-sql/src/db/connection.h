#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql/QSqlDatabase>

#include "oledbconnectionparser.h"


namespace db {

QSqlDatabase byOleDbConnectionParser(const OleDbConnectionParser &oleParser,
                                     const QString &connName = QLatin1String(QSqlDatabase::defaultConnection));
QSqlDatabase toAccess(const QString &path,
                      const QString &connName = QLatin1String(QSqlDatabase::defaultConnection));
QSqlDatabase toMssql(const QString &server, const QString &login, const QString &pass,
                     const QString &connName = QLatin1String(QSqlDatabase::defaultConnection));
QSqlDatabase toOracleByTns(const QString &server, const QString &login, const QString &pass,
                           const QString &connName = QLatin1String(QSqlDatabase::defaultConnection));

} // namespace db

#endif // CONNECTION_H
