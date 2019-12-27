#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql/QSqlDatabase>

#include "oledbconnectionparser.h"


namespace db {

QString makeThreadLocalConnectionName();

QSqlDatabase byOleDbConnectionParser(const OleDbConnectionParser &oleParser,
                                     const QString &connName = makeThreadLocalConnectionName());
QSqlDatabase toAccess(const QString &path,
                      const QString &connName = makeThreadLocalConnectionName());
QSqlDatabase toMssql(const QString &server, const QString &login, const QString &pass,
                     const QString &connName = makeThreadLocalConnectionName());
QSqlDatabase toOracleByTns(const QString &server, const QString &login, const QString &pass,
                           const QString &connName = makeThreadLocalConnectionName());

} // namespace db

#endif // CONNECTION_H
