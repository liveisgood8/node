#ifndef OLEDBCONNECTIONPARSER_H
#define OLEDBCONNECTIONPARSER_H

#include <QString>


namespace db {

class OleDbConnectionParser
{
public:
    enum Types
    {
        Access,
        Mssql,
        OracleTns,
        OracleIp
    };

public:
    OleDbConnectionParser(const QString &connectionString);

    bool isEmpty() const;

    QString connectionString() const;
    Types type() const;
    QString value(const QString &key) const;

    QString login() const;
    QString password() const;
    QString host() const;

    void removeKey(const QString &key);

private:
    void defineType();

private:
    QString m_connectionString;
    Types m_type = Access;
};

} // namespace db

#endif // OLEDBCONNECTIONPARSER_H
