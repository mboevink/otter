/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "Console.h"

namespace Otter
{

Console* Console::m_instance = NULL;
QList<ConsoleMessage*> Console::m_messages;

Console::Console(QObject *parent) : QObject(parent)
{
}

Console::~Console()
{
	for (int i = 0; i < m_messages.count(); ++i)
	{
		delete m_messages.at(i);
	}
}

void Console::createInstance(QObject *parent)
{
	m_instance = new Console(parent);
}

void Console::addMessage(const QString &note, MessageCategory category, MessageLevel level, const QString &source, int line, qint64 window)
{
	ConsoleMessage *message = new ConsoleMessage();
	message->time = QDateTime::currentDateTime();
	message->note = note;
	message->source = source;
	message->category = category;
	message->level = level;
	message->line = line;
	message->window = window;

	m_messages.append(message);

	emit m_instance->messageAdded(message);
}

Console *Console::getInstance()
{
	return m_instance;
}

QList<ConsoleMessage*> Console::getMessages()
{
	return m_messages;
}

}
