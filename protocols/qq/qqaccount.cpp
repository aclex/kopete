/*
    qqaccount.cpp - Kopete QQ Protocol

    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/


#include "qqaccount.h"

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kactionmenu.h>
#include <kmenu.h>

#include "kopetemetacontact.h"
#include "kopetecontactlist.h"

#include "qqcontact.h"
#include "qqfakeserver.h"
#include "qqprotocol.h"


QQAccount::QQAccount( QQProtocol *parent, const QString& accountID )
: Kopete::Account ( parent, accountID )
{
	// Init the myself contact
	setMyself( new QQContact( this, accountId(), QQContact::Null, accountId(), Kopete::ContactList::self()->myself() ) );
	myself()->setOnlineStatus( QQProtocol::protocol()->qqOffline );
	m_server = new QQFakeServer();;
}

QQAccount::~QQAccount()
{
	delete m_server;
}

KActionMenu* QQAccount::actionMenu()
{
	KActionMenu *mActionMenu = Kopete::Account::actionMenu();

	mActionMenu->kMenu()->insertSeparator();

	KAction *action;
	
	action = new KAction (KIcon("qq_showvideo"), i18n ("Show my own video..."), 0, "actionShowVideo");
	QObject::connect( action, SIGNAL(triggered(bool)), this, SLOT(slotShowVideo()) );
	mActionMenu->addAction(action);
	action->setEnabled( isConnected() );

	return mActionMenu;
}

bool QQAccount::createContact(const QString& contactId, Kopete::MetaContact* parentContact)
{
	QQContact* newContact = new QQContact( this, contactId, QQContact::Echo, parentContact->displayName(), parentContact );
	return newContact != 0L;
}

void QQAccount::setAway( bool away, const QString & /* reason */ )
{
	if ( away )
		slotGoAway();
	else
		slotGoOnline();
}

void QQAccount::setOnlineStatus(const Kopete::OnlineStatus& status, const Kopete::StatusMessage &reason )
{
	if ( status.status() == Kopete::OnlineStatus::Online &&
			myself()->onlineStatus().status() == Kopete::OnlineStatus::Offline )
		slotGoOnline();
	else if (status.status() == Kopete::OnlineStatus::Online &&
			myself()->onlineStatus().status() == Kopete::OnlineStatus::Away )
		setAway( false, reason.message() );
	else if ( status.status() == Kopete::OnlineStatus::Offline )
		slotGoOffline();
	else if ( status.status() == Kopete::OnlineStatus::Away )
		slotGoAway( /* reason */ );
}

void QQAccount::setStatusMessage(const Kopete::StatusMessage& statusMessage)
{
	/* Not used in qq */
}

void QQAccount::connect( const Kopete::OnlineStatus& /* initialStatus */ )
{
	kDebug ( 14210 ) << k_funcinfo << endl;
	myself()->setOnlineStatus( QQProtocol::protocol()->qqOnline );
	QObject::connect ( m_server, SIGNAL ( messageReceived( const QString & ) ),
			this, SLOT ( receivedMessage( const QString & ) ) );
}

void QQAccount::disconnect()
{
	kDebug ( 14210 ) << k_funcinfo << endl;
	myself()->setOnlineStatus( QQProtocol::protocol()->qqOffline );
	QObject::disconnect ( m_server, 0, 0, 0 );
}

QQFakeServer * QQAccount::server()
{
	return m_server;
}

void QQAccount::slotGoOnline ()
{
	kDebug ( 14210 ) << k_funcinfo << endl;

	if (!isConnected ())
		connect ();
	else
		myself()->setOnlineStatus( QQProtocol::protocol()->qqOnline );
	updateContactStatus();
}

void QQAccount::slotGoAway ()
{
	kDebug ( 14210 ) << k_funcinfo << endl;

	if (!isConnected ())
		connect();

	myself()->setOnlineStatus( QQProtocol::protocol()->qqAway );
	updateContactStatus();
}


void QQAccount::slotGoOffline ()
{
	kDebug ( 14210 ) << k_funcinfo << endl;

	if (isConnected ())
		disconnect ();
	updateContactStatus();
}

void QQAccount::slotShowVideo ()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;

	if (isConnected ())
		QQWebcamDialog *qqWebcamDialog = new QQWebcamDialog(0, 0, "QQ video window");
	updateContactStatus();
}

void QQAccount::receivedMessage( const QString &message )
{
	// Look up the contact the message is from
	QString from;
	QQContact* messageSender;

	from = message.section( ':', 0, 0 );
	Kopete::Contact* contact = contacts()[from];
	messageSender = dynamic_cast<QQContact *>( contact );

	kDebug( 14210 ) << k_funcinfo << " got a message from " << from << ", " << messageSender << ", is: " << message << endl;
	// Pass it on to the contact to process and display via a KMM
	if ( messageSender )
		messageSender->receivedMessage( message );
	else
		kWarning(14210) << k_funcinfo << "unable to look up contact for delivery" << endl;
}

void QQAccount::updateContactStatus()
{
	QHashIterator<QString, Kopete::Contact*>itr( contacts() );
	for ( ; itr.hasNext(); ) {
		itr.next();
		itr.value()->setOnlineStatus( myself()->onlineStatus() );
	}
}


#include "qqaccount.moc"