/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Dec 26 03:12:10 CLST 2001
    copyright            : (C) 2001 by Duncan Mac-Vicar Prett
    email                : duncan@puc.cl
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include "kopete.h"

static const char *description =
	I18N_NOOP("Kopete, the KDE Messenger");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
static const char *version="0.23";
	
static KCmdLineOptions options[] =
{
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{
	KAboutData aboutData( "kopete", I18N_NOOP("Kopete"),
		version, description, KAboutData::License_GPL,
		"(c) 2002, Duncan Mac-Vicar Prett", 0, 0, "duncan@kde.org"
		);

	aboutData.addAuthor ( "Duncan Mac-Vicar Prett", I18N_NOOP("Author, core developer"), "duncan@kde.org", "http://www.mac-vicar.com" );
	aboutData.addAuthor ( "Nick Betcher", I18N_NOOP("core developer, fastest plugin developer on earth."), "nbetcher@usinternet.com", "http://www.kdedevelopers.net" );
	aboutData.addAuthor ( "Ryan Cumming", I18N_NOOP("core developer"), "bodnar42@phalynx.dhs.org" );
	aboutData.addAuthor ( "Martijn Klingens", I18N_NOOP("Patches, bugfixes"), "klingens@kde.org" );
	aboutData.addAuthor ( "Stefan Gehn", I18N_NOOP("some cleanups, features and bugfixes"), "sgehn@gmx.net", "http://metz81.mine.nu" );

	aboutData.addCredit ( "Herwin Jan Steehouwer", I18N_NOOP("KxEngine ICQ code") );
	aboutData.addCredit ( "Olaf Lueg", I18N_NOOP("Kmerlin MSN Code") );
	aboutData.addCredit ( "Neil Stevens", I18N_NOOP("TAim engine AIM Code") );

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
	KUniqueApplication::addCmdLineOptions();

	Kopete kopete;
	kopete.exec();
}
