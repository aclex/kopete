
/*
    kopetecommandhandler.h - Command Handler

    Copyright (c) 2003      by Jason Keirstead       <jason@keirstead.org>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef _KOPETECOMMANDHANDLER_H_
#define _KOPETECOMMANDHANDLER_H_

#include <qdict.h>
#include "kopetemessage.h"

class KopetePlugin;
class KopeteMessageManager;
class KopeteCommand;
class KProcess;
struct CommandHandlerPrivate;

typedef QDict<KopeteCommand> CommandList;

class KopeteCommandHandler : public QObject
{
	Q_OBJECT

	public:
		KopeteCommandHandler();
		~KopeteCommandHandler();

		/**
		 * Returns a pointer to the command handler
		 */
		static KopeteCommandHandler *commandHandler();

		/**
		 * Registers a command with the command handler. Command matching is
		 * case insensitive. All commands are registered, regardless of whether
		 * or not they are already handled by another handler. This is so that
		 * if the first plugin is unloaded, the next handler in the sequence
		 * will handle the command. However, there are certain commands which
		 * are reserved (internally handled by the KopeteCommandHandler). These
		 * commands can also be overridden by registering a new suplicate command.
		 * Reserved commands can be discovered using @ref reserved() if needed.
		 *
		 * @param parent The plugin who owns this command
		 * @param command The command we want to handle, not including the '/'
		 * @param handlerSlot The slot used to handle the command. This slot must
		 *   accept two paramaters, a QString of arguments, and a KopeteMessageManager
		 *   pointer to the manager under which the command was sent.
		 * @param help An optional help string to be shown when the user uses
		 *   /help <command>
		 */
		void registerCommand( KopetePlugin *parent, const QString &command, const char* handlerSlot,
			const QString &help = QString::null );

		/**
		 * Unregisters a command. When a plugin unloads, all commands are
		 * automaticlly unregistered and deleted. This function should only
		 * be called in the case of a plugin which loads and unloads commands
		 * dynamicly.
		 *
		 * @param parent The plugin who owns this command
		 * @param command The command to unload
		 */
		void unregisterCommand( KopetePlugin *parent, const QString &command );

		/**
		 * Processes a message to see if any commands should be handled
		 *
		 * @param msg The message to process
		 * @param manager The manager who owns this message
		 * @return True if the command was handled, false if not
		 */
		bool processMessage( KopeteMessage &msg, KopeteMessageManager *manager );

		/**
		 * Parses a string of command arguments into a QStringList. Quoted
		 * blocks within the arguments string are treated as one argument.
		 */
		static QStringList parseArguments( const QString &args );

		/**
		 * Used to discover if a command is already handled by any handler
		 *
		 * @param command The command to check
		 * @return True if the command is already being handled, False if not
		 */
		bool commandHandled( const QString &command );

		/**
		 * Returns the list of reserved (internal) commands.
		 *
		 * @return A list of commands reserved for internal Kopete use
		 */
		const QStringList reserved() const;

	private slots:
		void slotPluginLoaded( KopetePlugin * );
		void slotPluginDestroyed( QObject * );
		void slotExecReturnedData(KProcess *proc, char *buff, int bufflen );
		void slotExecFinished(KProcess *proc);

	private:
		/**
		 * Function to run a reserved (internal) command
		 */
		void reservedCommand( const QString &command, const QString &args, KopeteMessageManager *manager );

		/**
		 * Helper function. Returns all the commands that can be used by a KMM of this protocol
		 * (all non-protocol commands, plus this protocols commands)
		 */
		CommandList commands( KopeteProtocol * );

		static CommandHandlerPrivate *p;
};

#endif
