#include <exception>

#include <QString>


class AppException final : public std::exception
{
public:
	explicit AppException(const QString &message);

	const QString &message() const;
	const char *what() const noexcept override;

private:
	QString m_message;
};
