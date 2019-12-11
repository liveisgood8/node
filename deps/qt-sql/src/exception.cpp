#include "exception.h"


AppException::AppException(const QString &message)
	: m_message(message)
{

}

const QString & AppException::message() const
{
	return m_message;
}

const char *AppException::what() const noexcept
{
	return m_message.toUtf8().constData();
}


