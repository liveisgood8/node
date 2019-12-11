#include "oledbconnectionparser.h"

#include <QRegularExpression>


namespace db {

OleDbConnectionParser::OleDbConnectionParser(const QString &connectionString)
    : m_connectionString(connectionString)
{
    Q_ASSERT(!m_connectionString.isEmpty());

    defineType();
}

bool OleDbConnectionParser::isEmpty() const
{
    return m_connectionString.isEmpty();
}

QString OleDbConnectionParser::connectionString() const
{
    return m_connectionString;
}

OleDbConnectionParser::Types OleDbConnectionParser::type() const
{
    return m_type;
}

QString OleDbConnectionParser::value(const QString &key) const
{
    int keyStartIdx = m_connectionString.indexOf(key);
    if (keyStartIdx == -1)
    {
        return QString();
    }

    keyStartIdx++; // Пропускаем знак '='

    int endValueIdx = m_connectionString.indexOf(";", keyStartIdx);
    if (endValueIdx == -1)
    {
        return m_connectionString.mid(keyStartIdx + key.length());
    }

    return m_connectionString.mid(keyStartIdx + key.length(),
                                endValueIdx - keyStartIdx - key.length());
}

QString OleDbConnectionParser::login() const
{
    return value("User ID");
}

QString OleDbConnectionParser::password() const
{
    return value("Password");
}

QString OleDbConnectionParser::host() const
{
    return value("Data Source");
}

void OleDbConnectionParser::removeKey(const QString &key)
{
    int keyStartIdx = m_connectionString.indexOf(key);
    if (keyStartIdx == -1)
    {
        return;
    }

    int endValueIdx = m_connectionString.indexOf(";", keyStartIdx);
    if (endValueIdx == -1)
    {
        endValueIdx = m_connectionString.length();
    }
    else
    {
        endValueIdx++; //Берем в учет знак ';'
    }

    m_connectionString.remove(keyStartIdx, endValueIdx - keyStartIdx);
}

void OleDbConnectionParser::defineType()
{
    if (m_connectionString.contains("SQLOLEDB") ||
            m_connectionString.contains("SQLNCLI") ||
            m_connectionString.contains("MSOLEDBSQL"))
    {
        m_type = Mssql;
    }
    else if (m_connectionString.contains("OraOLEDB"))
    {
        QRegularExpression ipRegEx("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
        if (ipRegEx.match(host()).hasMatch())
        {
            m_type = OracleIp;
        }
        else
        {
            m_type = OracleTns;
        }
    }
    else
    {
        m_type = Access;
    }
}

} // namespace db
