#include "SessionsManager.h"
#include "Application.h"
#include "SettingsManager.h"
#include "WindowsManager.h"
#include "../ui/MainWindow.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

namespace Otter
{

SessionsManager* SessionsManager::m_instance = NULL;
QString SessionsManager::m_session;
QList<WindowsManager*> SessionsManager::m_windows;
QList<SessionEntry> SessionsManager::m_closedWindows;
bool SessionsManager::m_dirty = false;

SessionsManager::SessionsManager(QObject *parent) : QObject(parent),
	m_autoSaveTimer(0)
{
}

void SessionsManager::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_autoSaveTimer)
	{
		m_dirty = false;

		killTimer(m_autoSaveTimer);

		m_autoSaveTimer = 0;

		saveSession();
	}
}

void SessionsManager::scheduleAutoSave()
{
	if (m_autoSaveTimer == 0)
	{
		m_autoSaveTimer = startTimer(1000);
	}
}

void SessionsManager::createInstance(QObject *parent)
{
	m_instance = new SessionsManager(parent);
}

void SessionsManager::registerWindow(WindowsManager *manager)
{
	if (manager)
	{
		m_windows.append(manager);
	}
}

void SessionsManager::storeClosedWindow(WindowsManager *manager)
{
	if (!manager)
	{
		return;
	}

	SessionEntry session = manager->getSession();

	if (!session.windows.isEmpty())
	{
		m_closedWindows.prepend(session);

		emit m_instance->closedWindowsChanged();
	}
}

void SessionsManager::markSessionModified()
{
	if (m_session == "default" && !m_dirty)
	{
		m_dirty = true;

		m_instance->scheduleAutoSave();
	}
}

SessionsManager *SessionsManager::getInstance()
{
	return m_instance;
}

QString SessionsManager::getCurrentSession()
{
	return m_session;
}

QString SessionsManager::getSessionPath(const QString &path, bool bound)
{
	QString cleanPath = path;

	if (cleanPath.isEmpty())
	{
		cleanPath = "default.ini";
	}
	else
	{
		if (!cleanPath.endsWith(".ini"))
		{
			cleanPath += ".ini";
		}

		if (bound)
		{
			cleanPath = cleanPath.replace('/', QString()).replace('\\', QString());
		}
		else if (QFileInfo(cleanPath).isAbsolute())
		{
			return cleanPath;
		}
	}

	return SettingsManager::getPath() + "/sessions/" + cleanPath;
}

QStringList SessionsManager::getClosedWindows()
{
	QStringList closedWindows;

	for (int i = 0; i < m_closedWindows.count(); ++i)
	{
		const SessionEntry window = m_closedWindows.at(i);
		const QString title = window.windows.value(window.index, SessionWindow()).title();

		closedWindows.append(title.isEmpty() ? tr("(Untitled)") : title);
	}

	return closedWindows;
}

SessionInformation SessionsManager::getSession(const QString &path)
{
	const QString sessionPath = getSessionPath(path);
	const QSettings sessionData(sessionPath, QSettings::IniFormat);
	const int windows = sessionData.value("Session/windows", 0).toInt();
	SessionInformation session;
	session.path = path;
	session.title = sessionData.value("Session/title", ((path == "default") ? tr("Default") : tr("(Untitled)"))).toString();
	session.index = (sessionData.value("Session/index", 1).toInt() - 1);

	for (int i = 1; i <= windows; ++i)
	{
		const int tabs = sessionData.value(QString("%1/Properties/windows").arg(i), 0).toInt();
		SessionEntry sessionEntry;
		sessionEntry.index = (sessionData.value(QString("%1/Properties/index").arg(i), 1).toInt() - 1);

		for (int j = 1; j <= tabs; ++j)
		{
			const int history = sessionData.value(QString("%1/%2/Properties/history").arg(i).arg(j), 0).toInt();
			SessionWindow sessionWindow;
			sessionWindow.group = sessionData.value(QString("%1/%2/Properties/group").arg(i).arg(j), 0).toInt();
			sessionWindow.index = (sessionData.value(QString("%1/%2/Properties/index").arg(i).arg(j), 1).toInt() - 1);
			sessionWindow.pinned = sessionData.value(QString("%1/%2/Properties/pinned").arg(i).arg(j), false).toBool();

			for (int k = 1; k <= history; ++k)
			{
				HistoryEntry historyEntry;
				historyEntry.url = sessionData.value(QString("%1/%2/History/%3/url").arg(i).arg(j).arg(k), 0).toString();
				historyEntry.title = sessionData.value(QString("%1/%2/History/%3/title").arg(i).arg(j).arg(k), 1).toString();
				historyEntry.position = sessionData.value(QString("%1/%2/History/%3/position").arg(i).arg(j).arg(k), 1).toPoint();

				sessionWindow.history.append(historyEntry);
			}

			sessionEntry.windows.append(sessionWindow);
		}

		session.windows.append(sessionEntry);
	}

	return session;
}

QStringList SessionsManager::getSessions()
{
	QStringList entries = QDir(SettingsManager::getPath() + "/sessions/").entryList(QStringList("*.ini"), QDir::Files);

	for (int i = 0; i < entries.count(); ++i)
	{
		entries[i] = QFileInfo(entries.at(i)).completeBaseName();
	}

	if (!m_session.isEmpty() && !entries.contains(m_session))
	{
		entries.append(m_session);
	}

	if (!entries.contains("default"))
	{
		entries.append("default");
	}

	entries.sort();

	return entries;
}

bool SessionsManager::restoreClosedWindow(int index)
{
	Application *application = qobject_cast<Application*>(QCoreApplication::instance());

	if (!application)
	{
		return false;
	}

	if (index < 0)
	{
		index = 0;
	}

	application->createWindow(false, m_closedWindows.value(index, SessionEntry()));

	m_closedWindows.removeAt(index);

	emit m_instance->closedWindowsChanged();

	return true;
}

bool SessionsManager::restoreSession(const QString &path, MainWindow *window)
{
	const SessionInformation session = getSession(path);

	if (session.windows.isEmpty())
	{
		return false;
	}

	const QList<SessionEntry> closedWindows = m_closedWindows;

	if (m_session.isEmpty())
	{
		m_session = path;
	}

	m_closedWindows = session.windows;

	if (window && m_closedWindows.count() > 0)
	{
		window->getWindowsManager()->restore(m_closedWindows.first().windows);

		m_closedWindows.removeAt(0);
	}

	const int windows = m_closedWindows.count();

	for (int i = 0; i < windows; ++i)
	{
		restoreClosedWindow();
	}

	m_closedWindows = closedWindows;

	return true;
}

bool SessionsManager::saveSession(const QString &path, const QString &title, MainWindow *window)
{
	QList<MainWindow*> windows;

	if (window)
	{
		windows.append(window);
	}
	else
	{
		Application *application = qobject_cast<Application*>(QCoreApplication::instance());

		if (!application)
		{
			return false;
		}

		windows = application->getWindows();
	}

	QDir().mkpath(SettingsManager::getPath() + "/sessions/");

	const QString sessionPath = getSessionPath(path);
	QString sessionTitle = QSettings(sessionPath, QSettings::IniFormat).value("Session/title").toString();

	if (!title.isEmpty())
	{
		sessionTitle = title;
	}

	QFile file(sessionPath);

	if (!file.open(QIODevice::WriteOnly))
	{
		return false;
	}

	QTextStream stream(&file);
	stream << "[Session]\n";
	stream << "title=" << sessionTitle.replace('\n', "\\n") << '\n';
	stream << "windows=" << windows.count() << '\n';
	stream << "index=1\n\n";

	for (int i = 0; i < windows.count(); ++i)
	{
		const SessionEntry sessionEntry = windows.at(i)->getWindowsManager()->getSession();

		stream << QString("[%1/Properties]\n").arg(i + 1);
		stream << "groups=0\n";
		stream << "windows=" << sessionEntry.windows.count() << '\n';
		stream << "index=" << (sessionEntry.index + 1) << "\n\n";

		for (int j = 0; j < sessionEntry.windows.count(); ++j)
		{
			stream << QString("[%1/%2/Properties]\n").arg(i + 1).arg(j + 1);
			stream << "pinned=" << sessionEntry.windows.at(j).pinned << '\n';
			stream << "group=0\n";
			stream << "history=" << sessionEntry.windows.at(j).history.count() << '\n';
			stream << "index=" << (sessionEntry.windows.at(j).index +  1) << "\n\n";

			for (int k = 0; k < sessionEntry.windows.at(j).history.count(); ++k)
			{
				stream << QString("[%1/%2/History/%3]\n").arg(i + 1).arg(j + 1).arg(k + 1);
				stream << "url=" << sessionEntry.windows.at(j).history.at(k).url << '\n';
				stream << "title=" << QString(sessionEntry.windows.at(j).history.at(k).title).replace('\n', "\\n") << '\n';
				stream << "position=" << sessionEntry.windows.at(j).history.at(k).position.x() << ',' << sessionEntry.windows.at(j).history.at(k).position.y() << '\n';
				stream << "zoom=" << sessionEntry.windows.at(j).history.at(k).zoom << "\n\n";
			}
		}
	}

	file.close();

	return true;
}

bool SessionsManager::deleteSession(const QString &path)
{
	const QString cleanPath = getSessionPath(path, true);

	if (QFile::exists(cleanPath))
	{
		return QFile::remove(cleanPath);
	}

	return false;
}

bool SessionsManager::moveSession(const QString &from, const QString &to)
{
	return QFile::rename(getSessionPath(from), getSessionPath(to));
}

}