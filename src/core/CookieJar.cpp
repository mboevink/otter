#include "CookieJar.h"
#include "SettingsManager.h"

#include <QtCore/QFile>
#include <QtCore/QTimerEvent>

namespace Otter
{

CookieJar::CookieJar(QObject *parent) : QNetworkCookieJar(parent),
	m_autoSaveTimer(0)
{
	QFile file(SettingsManager::getPath() + "/cookies.dat");

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QList<QNetworkCookie> allCookies;
	QDataStream stream(&file);
	quint32 amount;

	stream >> amount;

	for (quint32 i = 0; i < amount; ++i)
	{
		QByteArray value;

		stream >> value;

		const QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(value);

		for (int j = 0; j < cookies.count(); ++j)
		{
			allCookies.append(cookies.at(j));
		}

		if (stream.atEnd())
		{
			break;
		}
	}

	setAllCookies(allCookies);
}

void CookieJar::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_autoSaveTimer)
	{
		killTimer(m_autoSaveTimer);

		m_autoSaveTimer = 0;

		save();
	}
}

bool CookieJar::insertCookie(const QNetworkCookie &cookie)
{
	const bool result = QNetworkCookieJar::insertCookie(cookie);

	if (result)
	{
		if (m_autoSaveTimer == 0)
		{
			m_autoSaveTimer = startTimer(1000);
		}

		emit cookieInserted(cookie);
	}

	return result;
}

bool CookieJar::deleteCookie(const QNetworkCookie &cookie)
{
	const bool result = QNetworkCookieJar::deleteCookie(cookie);

	if (result)
	{
		if (m_autoSaveTimer == 0)
		{
			m_autoSaveTimer = startTimer(1000);
		}

		emit cookieDeleted(cookie);
	}

	return result;
}

bool CookieJar::updateCookie(const QNetworkCookie &cookie)
{
	const bool result = QNetworkCookieJar::updateCookie(cookie);

	if (result && m_autoSaveTimer == 0)
	{
		m_autoSaveTimer = startTimer(1000);
	}

	return result;
}

void CookieJar::save()
{
	QFile file(SettingsManager::getPath() + "/cookies.dat");

	if (!file.open(QIODevice::WriteOnly))
	{
		return;
	}

	QList<QNetworkCookie> cookies = allCookies();
	QDataStream stream(&file);
	stream << quint32(cookies.size());

	for (int i = 0; i < cookies.size(); ++i)
	{
		stream << cookies.at(i).toRawForm();
	}
}

QList<QNetworkCookie> CookieJar::getCookies() const
{
	return allCookies();
}

}
