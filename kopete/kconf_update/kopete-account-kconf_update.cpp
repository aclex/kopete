/*
    kconf_update app for migrating kopete 0.6.x accounts to 0.7. Code is
    not up to my normal standards, but it does the job, and since it's
    supposed to run exactly once on each system that's good enough for me :)

    Copyright (c) 2003      by Martijn Klingens <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <qmap.h>
#include <qtextstream.h>
#include <qregexp.h>

static QTextStream qcin ( stdin,  IO_ReadOnly );
static QTextStream qcout( stdout, IO_WriteOnly );
static QTextStream qcerr( stderr, IO_WriteOnly );

// Group cache. Yes, I know global vars are ugly :)
bool needFlush = false;
QString accountId;
QString password;
QString autoConnect;
QString protocol;
QMap<QString, QString> pluginData;

/*
 * Function for (en/de)crypting strings for config file, taken from KMail
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
QString cryptStr(const QString &aStr)
{
	QString result;
	for (unsigned int i = 0; i < aStr.length(); i++)
		result += (aStr[i].unicode() < 0x20) ? aStr[i] :
			QChar(0x1001F - aStr[i].unicode());
	return result;
}

void parseGroup( const QString &group, const QString &rawLine )
{
	// Groups that are converted can almost certainly be removed entirely
	if ( group == "MSN" || group == "ICQ" || group == "Oscar" /* || .... */ )
	{
		accountId = "EMPTY";
		autoConnect = "false";

		if ( group == "Oscar" )
			protocol = "AIMProtocol";
		else
			protocol = group + "Protocol";

		password = QString::null;
		pluginData.clear();

		needFlush = true;

		qcout << "# DELETEGROUP [" << group << "]" << endl;
	}
	else
	{
		// Groups we don't convert. Output the raw line instead.
		qcout << rawLine << endl;
	}
}

void parseKey( const QString &group, const QString &key, const QString &value, const QString &rawLine )
{
	if ( group == "MSN" )
	{
		if ( key == "UserID" )
			accountId = value;
		else if ( key == "Password" )
			password = value;
		else if ( key == "AutoConnect" )
			autoConnect = value;
		else if ( key == "Nick" )
			pluginData[ "displayName" ] = value;

		// All other keys are ignored for MSN, as these apply to stuff that's
		// now in libkopete (and the main app) instead.
	}

	if ( group == "ICQ" )
	{
		if ( key == "UIN" )
			accountId = value;
		if ( key == "Password" )
			password = value;
		if ( key == "AutoConnect" )
			autoConnect = value;
		if ( key == "Nick" )
			pluginData[ "displayName" ] = value;
	}

	if ( group == "Oscar" )
	{
		if ( key == "ScreenName" )
			accountId = value;
		if ( key == "Password" )
			password = value;
 	}

	if ( group == "Jabber" )
	{
		if ( key == "UserID" )
			accountId = value;
		if ( key == "Password" )
			password = value;
	}

	if ( group == "Gadu" )
	{
		if ( key == "UIN" )
			accountId = value;
		if ( key == "Password" )
			password = value;
		if ( key == "Nick" )
			pluginData[ "displayName" ] = value;
	}
	/*
		FIXME: Insert all other plugins here - Martijn
	*/
	else
	{
		// Groups we don't convert. Output the raw line instead.
		qcout << rawLine << endl;
	}
}

int main()
{
	qcin.setEncoding( QTextStream::UnicodeUTF8 );
	qcout.setEncoding( QTextStream::UnicodeUTF8 );

	QString curGroup;

	QRegExp groupRegExp( "^\\[(.*)\\]" );
	QRegExp keyRegExp( "^([a-zA-Z0-9:, _-]*)\\s*=\\s*(.*)\\s*" );
	QRegExp commentRegExp( "^(#.*)?$" );

	while ( !qcin.atEnd() )
	{
		QString line = qcin.readLine();

		if ( commentRegExp.exactMatch( line ) )
		{
			qcout << line << endl;
		}
		else if ( groupRegExp.exactMatch( line ) )
		{
			if ( needFlush )
			{
				// Flush existing group first
				qcout << "[Account_" << protocol << "_" << accountId << "]" << endl;
				qcout << "Protocol=" << protocol << endl;
				qcout << "AccountId=" << accountId << endl;
				qcout << "Password=" << cryptStr( password ) << endl;
				qcout << "AutoConnect=" << autoConnect << endl;

				QMap<QString, QString>::ConstIterator it;
				for ( it = pluginData.begin(); it != pluginData.end(); ++it )
					qcout << "PluginData_" << protocol << "_" << it.key() << "=" << it.data() << endl;

				needFlush = false;
			}

			curGroup = groupRegExp.capturedTexts()[ 1 ];
			parseGroup( curGroup, line );
		}
		else if ( keyRegExp.exactMatch( line ) )
		{
			parseKey( curGroup, keyRegExp.capturedTexts()[ 1 ], keyRegExp.capturedTexts()[ 2 ], line );
		}
		else
		{
			qcerr << "** Unknown input line: " << line << endl;
		}
	}

	return 0;
}

// vim: set noet ts=4 sts=4 sw=4:

