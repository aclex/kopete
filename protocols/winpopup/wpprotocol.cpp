/***************************************************************************
                          wpprotocol.cpp  -  WP Plugin
                             -------------------
    begin                : Fri Apr 26 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@kde.org

    Based on code from   : (C) 2002 by Duncan Mac-Vicar Prett
    email                : duncan@kde.org
 ***************************************************************************

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// QT Includes
#include <qcursor.h>
#include <qprocess.h>
#include <qfile.h>
#include <qregexp.h>

// KDE Includes
#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kpushbutton.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>

// Kopete Includes
#include "kopetemetacontact.h"
#include "kopeteaccountmanager.h"

// Local Includes
#include "wpprotocol.h"
#include "wpdebug.h"
#include "wpcontact.h"
#include "wpaddcontact.h"
#include "wppreferences.h"
#include "wpeditaccount.h"

class KPopupMenu;

WPProtocol *WPProtocol::sProtocol = 0;

K_EXPORT_COMPONENT_FACTORY(kopete_wp, KGenericFactory<WPProtocol>);

// WP Protocol
WPProtocol::WPProtocol(QObject *parent, QString name, QStringList) : KopeteProtocol(parent, name),
	WPOnline(  KopeteOnlineStatus::Online,  25, this, 0,  "wp_available", i18n( "Go O&nline" ),   i18n( "Online" ) ),
	WPAway(    KopeteOnlineStatus::Away,    25, this, 1,  "wp_away",      i18n( "Go &Away" ),     i18n( "Away" ) ),
	WPOffline( KopeteOnlineStatus::Offline, 25, this, 2,  "wp_offline",   i18n( "Go O&ffline" ),  i18n( "Offline" ) )
{
	DEBUG(WPDMETHOD, "WPProtocol::WPProtocol()");
	
	sProtocol = this;

	// Load Status Actions
//	initActions();
	// TODO: Maybe use this in the future?

	// Set up initial settings
	KGlobal::config()->setGroup("WinPopup");
	QString theSMBClientPath = KGlobal::config()->readEntry("SMBClientPath", "/usr/bin/smbclient");
	QString theInitialSearchHost = KGlobal::config()->readEntry("InitialSearchHost", "127.0.0.1");
	int theHostCheckFrequency = KGlobal::config()->readNumEntry("HostCheckFrequency", 60);
	int theMessageCheckFrequency = KGlobal::config()->readNumEntry("MessageCheckFrequency", 5);
	KGlobal::config()->writeEntry("SMBClientPath", theSMBClientPath);
	KGlobal::config()->writeEntry("InitialSearchHost", theInitialSearchHost);
	KGlobal::config()->writeEntry("HostCheckFrequency", theHostCheckFrequency);
	KGlobal::config()->writeEntry("MessageCheckFrequency", theMessageCheckFrequency);

	// Create preferences menu
	mPrefs = new WPPreferences("wp_icon", this);
	QObject::connect(mPrefs, SIGNAL(saved(void)), this, SLOT(slotSettingsChanged(void)));

	// Call slotSettingsChanged() to get it all registered.
	slotSettingsChanged();

	addAddressBookField( "messaging/winpopup", KopetePlugin::MakeIndexField );
}

// Destructor
WPProtocol::~WPProtocol()
{
	DEBUG(WPDMETHOD, "WPProtocol::~WPProtocol()");
	
	sProtocol = 0L;
}

KopeteWinPopup *WPProtocol::createInterface(const QString &theHostName)
{
	KGlobal::config()->setGroup("WinPopup");
	QString theSMBClientPath = KGlobal::config()->readEntry("SMBClientPath", "/usr/bin/smbclient");
	QString theInitialSearchHost = KGlobal::config()->readEntry("InitialSearchHost", "127.0.0.1");
	int theHostCheckFrequency = KGlobal::config()->readNumEntry("HostCheckFrequency", 60);
	int theMessageCheckFrequency = KGlobal::config()->readNumEntry("MessageCheckFrequency", 5);
	KopeteWinPopup *newOne = new KopeteWinPopup(theSMBClientPath, theInitialSearchHost, theHostName, theHostCheckFrequency, theMessageCheckFrequency);
	theInterfaces.append(newOne);
	return newOne;
}

void WPProtocol::destroyInterface(KopeteWinPopup *theInterface)
{
	theInterfaces.removeRef(theInterface);
	delete theInterface;
}

void WPProtocol::deserializeContact( KopeteMetaContact *metaContact, const QMap<QString, QString> &serializedData,
	const QMap<QString, QString> & /* addressBookData */ )
{
	QString contactId = serializedData[ "contactId" ];
	QString accountId = serializedData[ "accountId" ];

	WPAccount *theAccount = static_cast<WPAccount *>(KopeteAccountManager::manager()->findAccount(protocol()->pluginId(), accountId));
	if(!theAccount)
	{	DEBUG(WPDINFO, "Account " << accountId << " not found");
		return;
	}
	
	if(theAccount->contacts()[contactId])
	{	DEBUG(WPDINFO, "User " << contactId << " already in contacts map");
		return;
	}

	theAccount->addContact(contactId, serializedData["displayName"], metaContact, serializedData["group"]);
}

void WPProtocol::init()
{
	DEBUG(WPDMETHOD, "WPProtocol::init()");
}

bool WPProtocol::unload()
{
	DEBUG(WPDMETHOD, "WPProtocol::unload()");

	return KopeteProtocol::unload();
}

EditAccountWidget *WPProtocol::createEditAccountWidget(KopeteAccount *account, QWidget *parent)
{
	return new WPEditAccount(this, account, parent);
}

KopeteAccount *WPProtocol::createNewAccount(const QString &accountId)
{
	return new WPAccount(this, accountId);
}

void WPProtocol::slotSettingsChanged()
{
	DEBUG(WPDMETHOD, "WPProtocol::slotSettingsChanged()");

	KGlobal::config()->setGroup("WinPopup");
	for(KopeteWinPopup *i = theInterfaces.first(); i != theInterfaces.last(); i = theInterfaces.next())
	{	i->setSMBClientPath(KGlobal::config()->readEntry("SMBClientPath", "/usr/bin/smbclient"));
		i->setInitialSearchHost(KGlobal::config()->readEntry("InitialSearchHost", "127.0.0.1"));
		i->setHostCheckFrequency(KGlobal::config()->readNumEntry("HostCheckFrequency", 60));
		i->setMessageCheckFrequency(KGlobal::config()->readNumEntry("MessageCheckFrequency", 5));
	}
}

void WPProtocol::installSamba()
{
	DEBUG(WPDMETHOD, "WPPreferences::installSamba()");

	QStringList args;
	args += KStandardDirs::findExe("winpopup-install.sh");
	args += KStandardDirs::findExe("winpopup-send.sh");
	if (KApplication::kdeinitExecWait("kdesu", args) == 0)
		KMessageBox::information(mPrefs, i18n("The Samba configuration file is modified."), i18n("Configuration succeeded"));
	else
		KMessageBox::error(mPrefs, i18n("Updating the Samba configuration file failed."), i18n("Configuration failed"));
}

#include "wpprotocol.moc"

// vim: set noet ts=4 sts=4 sw=4:

