/*
    gwaccount.cpp - Kopete GroupWise Protocol

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Based on Testbed   
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

#include <sys/utsname.h>

#include <qdatetime.h>

#include <kaboutdata.h>
#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klineeditdlg.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>

#include "gwaccount.h"

#include <qvaluelist.h>

#include <kopeteuiglobal.h>
#include "kopeteawayaction.h"
#include "kopetecontactlist.h"
#include "kopetegroup.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "kopetepassword.h"

#include "client.h"
#include "qca.h"
#include "gwcontact.h"
#include "gwfakeserver.h"
#include "gwprotocol.h"
#include "gwclientstream.h"
#include "gwconnector.h"
#include "gwmessagemanager.h"
#include "ui/gwreceiveinvitationdialog.h"
#include "qcatlshandler.h"


GroupWiseAccount::GroupWiseAccount( GroupWiseProtocol *parent, const QString& accountID, const char *name )
: Kopete::PasswordedAccount ( parent, accountID, 0, "groupwiseaccount" )
{
	// Init the myself contact
	// FIXME: I think we should add a global self metaContact (Olivier)
	KopeteMetaContact *metaContact = new KopeteMetaContact;
	setMyself( new GroupWiseContact( this, accountId(), metaContact, "myself", 0, 0, 0 ) );
	myself()->setOnlineStatus( GroupWiseProtocol::protocol()->groupwiseOffline );
	
	m_connector = 0;
	m_QCATLS = 0;
	m_tlsHandler = 0;
	m_clientStream = 0;
	m_client= 0;
}

GroupWiseAccount::~GroupWiseAccount()
{
}

KActionMenu* GroupWiseAccount::actionMenu()
{
	KActionMenu *theActionMenu = new KActionMenu(accountId(), myself()->onlineStatus().iconFor(this) , this);
	theActionMenu->popupMenu()->insertTitle(myself()->icon(), i18n("GroupWise (%1)").arg(accountId()));
	
	theActionMenu->insert( new KAction (GroupWiseProtocol::protocol()->groupwiseAvailable.caption(),
		GroupWiseProtocol::protocol()->groupwiseAvailable.iconFor(this), 0, this, SLOT ( slotGoOnline() ), this,
		"actionGroupWiseConnect") );
	theActionMenu->insert( new KopeteAwayAction (GroupWiseProtocol::protocol()->groupwiseAway.caption(),
		GroupWiseProtocol::protocol()->groupwiseAway.iconFor(this), 0, this, SLOT ( slotGoAway( const QString & ) ), this,
		"actionGroupWiseAway") );
	theActionMenu->insert( new KopeteAwayAction (GroupWiseProtocol::protocol()->groupwiseBusy.caption(),
		GroupWiseProtocol::protocol()->groupwiseBusy.iconFor(this), 0, this, SLOT ( slotGoBusy( const QString & ) ), this,
		"actionGroupWiseBusy") );
	theActionMenu->insert( new KAction ( "A&ppear Offline", "jabber_invisible", 0, this, 
		SLOT( slotGoAppearOffline() ), this, 
		"actionGroupWiseAppearOffline") );
	theActionMenu->insert( new KAction (GroupWiseProtocol::protocol()->groupwiseOffline.caption(),
		GroupWiseProtocol::protocol()->groupwiseOffline.iconFor(this), 0, this, SLOT ( slotGoOffline() ), this,
		"actionGroupWiseOfflineDisconnect") );
	theActionMenu->insert( new KAction ( "Test rtfize()", QString::null, 0, this,
		SLOT( slotTestRTFize() ), this,
		"actionTestRTFize") );

	return theActionMenu;
}

bool GroupWiseAccount::addContactToMetaContact(const QString& contactId, const QString& displayName, KopeteMetaContact* parentContact)
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "contactId: " << contactId << " displayName: " << displayName
			<< endl;
	//GroupWiseContact* newContact = new GroupWiseContact( this, contactId, GroupWiseContact::Echo, displayName, parentContact );
	//return newContact != 0L;
	return false;
}

const int GroupWiseAccount::port() const
{
	return pluginData( KopeteAccount::protocol(), "Port" ).toInt();
}

const QString GroupWiseAccount::server() const
{
	return pluginData( KopeteAccount::protocol(), "Server" );
}

Client * GroupWiseAccount::client() const
{
	return m_client;
}

GroupWiseProtocol *GroupWiseAccount::protocol()
{
	return static_cast<GroupWiseProtocol *>( KopeteAccount::protocol() );
}

GroupWiseMessageManager * GroupWiseAccount::messageManager( const KopeteContact* user, KopeteContactPtrList others, KopeteProtocol* protocol, const QString & guid )
{
	GroupWiseMessageManager * mgr = m_managers[ guid ];
	if ( !mgr )
	{
		mgr = new GroupWiseMessageManager( user, others, protocol, guid );
		m_managers.insert( guid, mgr );
		QObject::connect( mgr, SIGNAL( destroyed( QObject * ) ), SLOT( slotMessageManagerDestroyed( QObject * ) ) );
	}
	return mgr;
}

void GroupWiseAccount::setAway( bool away, const QString & reason )
{
	if ( away )
		setStatus( GroupWise::Away, reason);
	else
		setStatus( GroupWise::Available );
}

void GroupWiseAccount::connectWithPassword( const QString &password )
{
	// set up network classes
	m_connector = new KNetworkConnector( 0 );
	//myConnector->setOptHostPort( "localhost", 8300 );
	m_connector->setOptHostPort( server(), port() );
	m_connector->setOptSSL( true );
	Q_ASSERT( QCA::isSupported(QCA::CAP_TLS) );
	m_QCATLS = new QCA::TLS;
	m_tlsHandler = new QCATLSHandler( m_QCATLS );
	m_clientStream = new ClientStream( m_connector, m_tlsHandler, 0);
	
	QObject::connect( m_connector, SIGNAL( error() ), this, SLOT( slotConnError() ) );
	QObject::connect( m_connector, SIGNAL( connected() ), this, SLOT( slotConnConnected() ) );
	
	QObject::connect (m_clientStream, SIGNAL (connectionClosed()),
				this, SLOT (slotCSDisconnected()));
	QObject::connect (m_clientStream, SIGNAL (delayedCloseFinished()),
				this, SLOT (slotCSDisconnected()));
	// Notify us when the transport layer is connected
	QObject::connect( m_clientStream, SIGNAL( connected() ), SLOT( slotCSConnected() ) );
	// it's necessary to catch this signal and tell the TLS handler to proceed
	// even if we don't check cert validity
	QObject::connect( m_tlsHandler, SIGNAL(tlsHandshaken()), SLOT( slotTLSHandshaken()) );
	// starts the client once the security layer is up, but see below
	QObject::connect( m_clientStream, SIGNAL( securityLayerActivated(int) ), SLOT( slotTLSReady(int) ) );
	// we could handle login etc in start(), in which case we would emit this signal after that
	//QObject::connect (jabberClientStream, SIGNAL (authenticated()),
	//			this, SLOT (slotCSAuthenticated ()));
	// we could also get do the actual login in response to this..
	//QObject::connect (m_clientStream, SIGNAL (needAuthParams(bool, bool, bool)),
	//			this, SLOT (slotCSNeedAuthParams (bool, bool, bool)));
	
	// not implemented: warning 
	QObject::connect( m_clientStream, SIGNAL( warning(int) ), SLOT( slotCSWarning(int) ) );
	// not implemented: error 
	QObject::connect( m_clientStream, SIGNAL( error(int) ), SLOT( slotCSError(int) ) );
	
	m_client = new Client( this );

	// NB these are prefixed with QObject:: to avoid any chance of a clash with our connect() methods.
	// TODO: Connect Client signals
	// we connected successfully
	QObject::connect( m_client, SIGNAL( loggedIn() ), SLOT( slotLoggedIn() ) );
	// folder listed
	QObject::connect( m_client, SIGNAL( folderReceived( const FolderItem & ) ), SLOT( receiveFolder( const FolderItem & ) ) );
	// contact listed
	QObject::connect( m_client, SIGNAL( contactReceived( const ContactItem & ) ), SLOT( receiveContact( const ContactItem & ) ) );
	// contact details listed
	QObject::connect( m_client, SIGNAL( contactUserDetailsReceived( const ContactDetails & ) ), SLOT( receiveContactUserDetails( const ContactDetails & ) ) );
	// contact status changed
	QObject::connect( m_client, SIGNAL( statusReceived( const QString &, Q_UINT16, const QString & ) ), SLOT( receiveStatus( const QString &, Q_UINT16 , const QString & ) ) );
	// incoming message
	QObject::connect( m_client, SIGNAL( messageReceived( const ConferenceEvent & ) ), SLOT( receiveMessage( const ConferenceEvent & ) ) );
	
	QObject::connect( m_client, SIGNAL( ourStatusChanged( GroupWise::Status, const QString &, const QString & ) ), SLOT( changeOurStatus( GroupWise::Status, const QString &, const QString & ) ) );
	// conference events
	QObject::connect( m_client, SIGNAL( conferenceCreated( const int, const QString & ) ), SIGNAL( conferenceCreated( const int, const QString & ) ) );
	QObject::connect( m_client, SIGNAL( conferenceCreationFailed( const int,  const int ) ), SIGNAL( conferenceCreationFailed( const int,  const int ) ) );
	QObject::connect( m_client, SIGNAL( invitationReceived( const ConferenceEvent & ) ), SLOT( receiveInvitation( const ConferenceEvent & ) ) );
	QObject::connect( m_client, SIGNAL( conferenceJoined( const QString &, const QStringList & ) ), SLOT( receiveConferenceJoin( const QString &, const QStringList & ) ) );

	// typing events
	QObject::connect( m_client, SIGNAL( contactTyping( const ConferenceEvent & ) ), SIGNAL( contactTyping( const ConferenceEvent & ) ) );
	QObject::connect( m_client, SIGNAL( contactNotTyping( const ConferenceEvent & ) ), SIGNAL( contactNotTyping( const ConferenceEvent & ) ) );
	QObject::connect( m_client, SIGNAL( accountDetailsReceived( const ContactDetails &) ), SLOT( receiveAccountDetails( const ContactDetails & ) ) );
	QObject::connect( m_client, SIGNAL( tempContactReceived( const ContactDetails &) ), SLOT( receiveTemporaryContact( const ContactDetails & ) ) );
	QObject::connect( m_client, SIGNAL( connectedElsewhere() ), SLOT( slotConnectedElsewhere() ) );
	
	struct utsname utsBuf;

	uname (&utsBuf);
	
	m_client->setClientName ("Kopete");
	m_client->setClientVersion ( kapp->aboutData ()->version () );
	m_client->setOSName (QString ("%1 %2").arg (utsBuf.sysname, 1).arg (utsBuf.release, 2)); 

	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Connecting to GroupWise server " << server() << ":" << port() << endl;

	NovellDN dn;
	dn.dn = "maeuschen";
	dn.server = "reiser.suse.de";
	myself()->setOnlineStatus( protocol()->groupwiseConnecting );
	m_client->connectToServer( m_clientStream, dn, true ); 
}

void GroupWiseAccount::disconnect ( KopeteAccount::DisconnectReason reason )
{
	// FIXME: this ugly sequence is a libkopete requirement
	disconnect();
	KopeteAccount::disconnect( reason );
}


void GroupWiseAccount::disconnect()
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	
	if( isConnected () )
	{
		kdDebug (GROUPWISE_DEBUG_GLOBAL) << k_funcinfo << "Still connected, closing connection..." << endl;
		/* Tell backend class to disconnect. */
		m_client->close ();
	}

	// make sure that the connection animation gets stopped if we're still
	// in the process of connecting
	myself()->setOnlineStatus( GroupWiseProtocol::protocol()->groupwiseOffline );

	/* FIXME:
	 * We should delete the XMPP::Client instance here,
	 * but active timers in psi prevent us from doing so.
	 * (in a failed connection attempt, these timers will
	 * try to access an already deleted object).
	 * Instead, the instance will lurk until the next
	 * connection attempt.
	 */
	kdDebug(GROUPWISE_DEBUG_GLOBAL) << k_funcinfo << "Disconnected." << endl;
}

void GroupWiseAccount::setStatus( GroupWise::Status status, const QString & reason )
{
	if ( !(myself()->onlineStatus() == protocol()->groupwiseConnecting ) )
	{
		if ( isConnected() )
		{
			m_client->setStatus( status, reason );
			//myself()->setOnlineStatus( GroupWiseProtocol::protocol()->groupwiseAway );
		}
		else
		{
			kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Must be connected before changing status" << endl;
			m_initialStatus = status;
			m_initialReason = reason;
			connect();
		}
	}
}

void GroupWiseAccount::createConference( const int clientId, const QStringList& invitees )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	// TODO: remove this it prevents sending a list of participants with the createconf
	m_client->createConference( clientId , invitees );
}

void GroupWiseAccount::slotGoOnline()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	setStatus( GroupWise::Available );
}

void GroupWiseAccount::slotGoAway( const QString & reason )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	setStatus( GroupWise::Away, reason );
}

void GroupWiseAccount::slotGoBusy( const QString & reason )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	setStatus( GroupWise::Busy, reason );
}

void GroupWiseAccount::slotGoAppearOffline()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	setStatus( GroupWise::Offline );
}

void GroupWiseAccount::slotGoOffline()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	disconnect ( KopeteAccount::Manual );
}

void GroupWiseAccount::slotLoggedIn()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	myself()->setOnlineStatus( protocol()->groupwiseAvailable );
}

void GroupWiseAccount::slotConnError()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
}

void GroupWiseAccount::slotConnConnected()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
}

void GroupWiseAccount::slotCSDisconnected()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Disconnected from Groupwise server." << endl;
}

void GroupWiseAccount::slotCSConnected()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Connected to Groupwise server." << endl;
	
}

void GroupWiseAccount::slotCSError( int error )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Got error from ClientStream:" << error << endl;
}

void GroupWiseAccount::slotCSWarning( int warning )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "Got warning from ClientStream:" << warning << endl;
}

void GroupWiseAccount::slotTLSHandshaken()
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "TLS handshake complete" << endl;
	int validityResult = m_QCATLS->certificateValidityResult ();

	if( validityResult == QCA::TLS::Valid )
	{
		kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << "Certificate is valid, continuing." << endl;
		// valid certificate, continue
		m_tlsHandler->continueAfterHandshake ();
	}
	else
	{
		kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << "Certificate is not valid, continuing anyway" << endl;
		// certificate is not valid, query the user
		/*			if(handleTLSWarning (validityResult, server (), myself()->contactId ()) == KMessageBox::Continue)
					{*/
		m_tlsHandler->continueAfterHandshake ();
		/*			}
					else
					{
					disconnect ( KopeteAccount::Manual );
					}*/
	}
}

void GroupWiseAccount::slotTLSReady( int secLayerCode )
{
	// i don't know what secLayerCode is for...
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	m_client->start( server(), port(), accountId(), password().cachedValue() );
}

void GroupWiseAccount::receiveMessage( const ConferenceEvent & event )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " got a message in conference: " << event.guid << ",  from: " << event.user << ", message is: " << event.message << endl;
	GroupWiseContact * contactFrom = static_cast<GroupWiseContact *>( contacts()[ event.user ] );
	Q_ASSERT( contactFrom );
	contactFrom->handleIncomingMessage( event );
/*	else
	{
		kdDebug (GROUPWISE_DEBUG_GLOBAL) << k_funcinfo << event.user << " is unknown to us, requesting details so we can create a temporary contact." << endl;
		m_pendingEvents.append( event );
		// get their details
		QStringList userDNsList;
		userDNsList.append( event.user );
		// the client will signal contactUserDetailsReceived when the details arrive, 
		// and we'll add a temporary contact in receiveContactUserDetails, before coming back to this method
		m_client->requestDetails( userDNsList );
	}*/
}

void GroupWiseAccount::receiveFolder( const FolderItem & folder )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo 
			<< " objectId: " << folder.id 
			<< " sequence: " << folder.sequence 
			<< " parentId: " << folder.parentId 
			<< " displayName: " << folder.name << endl;
	if ( folder.parentId != 0 )
	{
		kdWarning( GROUPWISE_DEBUG_GLOBAL ) << " - received a nested folder.  These werre not supported in GroupWise or Kopete as of Sept 2004, aborting! (parentId = " << folder.parentId << ")" << endl;
		return;
	}
	
	bool found = false;
	QPtrList<KopeteGroup> groupList = KopeteContactList::contactList()->groups();
	for ( KopeteGroup *grp = groupList.first(); grp; grp = groupList.next() )
		if ( grp->displayName() == folder.name )
		{
			grp->setPluginData( protocol(), accountId() + " objectId", QString::number( folder.id ) );
			found = true;
			break;
		}

	if ( found )
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - folder already exists locally" << endl;
	else
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - creating local folder" << endl;
		KopeteGroup * grp = new KopeteGroup( folder.name );
		grp->setPluginData( protocol(), accountId() + " objectId", QString::number( folder.id ) );
		KopeteContactList::contactList()->addGroup( grp );
	}
}

void GroupWiseAccount::receiveContact( const ContactItem & contact )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo 
			<< " objectId: " << contact.id 
			<< ", sequence: " << contact.sequence 
			<< ", parentId: " << contact.parentId 
			<< ", dn: " << contact.dn 
			<< ", displayName: " << contact.displayName << endl;
			
	// HACK: lowercased DN
	GroupWiseContact * c = static_cast<GroupWiseContact *>( contacts()[ contact.dn.lower() ] );
	if ( c )
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - found contact in list, checking to see it's in this group " << endl;
		// check the metacontact is in the group this listing-of-the-contact is in...
		KopeteMetaContact *metaContact = c->metaContact();
		KopeteGroupList groupList = KopeteContactList::contactList()->groups();
		for ( KopeteGroup *grp = groupList.first(); grp; grp = groupList.next() )
		{
			kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - seeking in group named: " << grp->displayName() << ", id: " << grp->pluginData( protocol(), accountId() + " objectId" ).toInt() << " our parentId is: " << contact.parentId << endl;
			if ( grp->pluginData( protocol(), accountId() + " objectId" ).toInt() == contact.parentId )
			{
				kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - matches, adding." << endl;
				metaContact->addToGroup( grp ); //addToGroup() is safe to call if already a member
				break;
			}
		}
		// TODO: need to update parentId? and sequence
	}
	else
	{
		KopeteMetaContact *metaContact = new KopeteMetaContact();
		// HACK: lowercased DN
		GroupWiseContact * c = new GroupWiseContact( this, contact.dn.lower(), metaContact, contact.displayName, contact.id, contact.parentId, contact.sequence );
		KopeteGroupList groupList = KopeteContactList::contactList()->groups();
		for ( KopeteGroup *grp = groupList.first(); grp; grp = groupList.next() )
		{
			if ( grp->pluginData( protocol(), accountId() + " objectId" ).toInt() == contact.parentId )
			{
				metaContact->addToGroup( grp );
				break;
			}
		}
		KopeteContactList::contactList()->addMetaContact( metaContact );
	}
}


void GroupWiseAccount::receiveAccountDetails( const ContactDetails & details )
{
	if ( details.cn == accountId() )
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " - got our details in contact list, updating them" << endl;
		GroupWiseContact * detailsOwner= static_cast<GroupWiseContact *>( myself() );
		detailsOwner->updateDetails( details );
		// Very important, without knowing our DN we can't do much else
		Q_ASSERT( !details.dn.isEmpty() );
		m_client->setUserDN( details.dn );
		return;
	}
	else
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " - passed someone else's details in contact list!" << endl;
	}
}

void GroupWiseAccount::receiveContactUserDetails( const ContactDetails & details )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo 
		<< "Auth attribute: " << details.authAttribute
		<< ", Away message: " << details.awayMessage
		<< ", CN" << details.cn
		<< ", DN" << details.dn
		<< ", fullName" << details.fullName
		<< ", surname" << details.surname
		<< ", givenname" << details.givenName
		<< ", status" << details.status
		<< endl;
	// HACK: lowercased DN 
	if ( !details.dn.isNull() )
	{
		// are the details for someone in our contact list?
		GroupWiseContact * detailsOwner= static_cast<GroupWiseContact *>( contacts()[ details.dn ] );
		if( detailsOwner )
		{
			kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - updating details for " << details.dn << endl;
			detailsOwner->updateDetails( details );
		}
		else
		{
			kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - got details for " << details.dn << ", but they aren't in our contact list!" << endl;
		}
	}
}

void GroupWiseAccount::receiveTemporaryContact( const ContactDetails & details )
{
	GroupWiseContact * c = static_cast<GroupWiseContact *>( contacts()[ details.dn.lower() ] );
	if ( !c && details.dn != accountId() )
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << "Got a temporary contact DN: " << details.dn << endl;
		// the client is telling us about a temporary contact we need to know about so add them 
		KopeteMetaContact *metaContact = new KopeteMetaContact ();
	
		metaContact->setTemporary (true);
	
		// Because we never know if the details will contain a fullname
		QString displayName = details.fullName;
		if ( displayName.isEmpty() )
			displayName = ( details.givenName + " " + details.surname );

		new GroupWiseContact( this, details.dn, metaContact, displayName, 0, 0, 0 );
		KopeteContactList::contactList ()->addMetaContact (metaContact);
		// the contact details probably don't contain status - but we can ask for it
		if ( details.status == GroupWise::Invalid )
			m_client->requestStatus( details.dn );
	}
	else
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << "Notified of existing temporary contact DN: " << details.dn << endl;
}

// any pending messages" << endl;
// 			// we asked for the user's details because we got a message from them out of the blue
// 			// look for any events from this user, add the user and deliver the event(s)
// 			QValueListIterator< ConferenceEvent > it;
// 			bool handledDetails = false;
// 			for ( it = m_pendingEvents.begin(); it != m_pendingEvents.end(); ++it )
// 			{
// 				ConferenceEvent event = *it;
// 				if ( !( event.user.isNull() || event.user != details.dn ) )
// 				{
// 					// we may have multiple events for this contact, but only create one temporary contact
// 					if ( !detailsOwner )
// 					{
// 						KopeteMetaContact *metaContact = new KopeteMetaContact ();
// 		
// 						metaContact->setTemporary (true);
// 		
// 						detailsOwner = new GroupWiseContact( this, details.dn, metaContact, details.fullName, 0, 0, 0 );
// 						KopeteContactList::contactList ()->addMetaContact (metaContact);
// 						// the contact details probably don't contain status - but we can ask for it
// 						if ( details.status == GroupWise::Invalid )
// 							m_client->requestStatus( details.dn );
// 					}
// 					// now the message can be received
// 					receiveMessage( event );
// 					handledDetails = true;
// 				}
// 				else
// 					kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - user DN: " << event.user << " received DN: " << details.dn << endl;
// 			}
// 			if ( !handledDetails )
// 				kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - received details, but there was no pending event matching them" << endl;
// 		}
// 	}
// }

void GroupWiseAccount::receiveStatus( const QString & contactId, Q_UINT16 status, const QString &awayMessage )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << "got status for: " << contactId << ", status: " << status << ", away message: " << awayMessage << endl;
	GroupWiseContact * c = static_cast<GroupWiseContact *>( contacts()[ contactId ] );
	if ( c )
	{
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " - their KOS is: " << protocol()->gwStatusToKOS( status ).description() << endl;
		c->setOnlineStatus( protocol()->gwStatusToKOS( status ) );
		c->setProperty( protocol()->propAwayMessage, awayMessage );
	}
	else
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << " couldn't find " << contactId << endl;
}

void GroupWiseAccount::changeOurStatus( GroupWise::Status status, const QString & awayMessage, const QString & autoReply )
{
	myself()->setOnlineStatus( protocol()->gwStatusToKOS( status ) );
	myself()->setProperty( protocol()->propAwayMessage, awayMessage );
	myself()->setProperty( protocol()->propAutoReply, autoReply );
}

void GroupWiseAccount::sendMessage( const QString &guid, const KopeteMessage & message )
{
	kdDebug ( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << endl;
	// make an outgoing message
	GroupWise::OutgoingMessage outMsg;
	outMsg.guid = guid;
	outMsg.message = message.plainBody();
	outMsg.rtfMessage = protocol()->rtfizeText( message.plainBody() );
	// make a list of DNs to send to
	QStringList addresseeDNs;
	KopeteContactPtrList addressees = message.to();
	for ( KopeteContact * contact = addressees.first(); contact; contact = addressees.next() )
		addresseeDNs.append( contact->contactId() );
	// send the message 
	m_client->sendMessage( addresseeDNs, outMsg );
}

void GroupWiseAccount::slotConnectedElsewhere()
{
	KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information,
				i18n( "The parameter is the user's own account id for this protocol", "You have been disconnected from GroupWise Messenger because you signed in as %1 elsewhere" ).arg( accountId() ) , i18n ("Signed in as %1 elsewhere").arg( accountId() ) );
	disconnect();
}

void GroupWiseAccount::receiveInvitation( const ConferenceEvent & event )
{
	// ask the user if they want to accept the invitation or not
	// TODO: make a pretty KDialogBase'd solution 
	ReceiveInvitationDialog * dlg = new ReceiveInvitationDialog( this, event, Kopete::UI::Global::mainWidget(), "invitedialog" );
	dlg->show();
}

void GroupWiseAccount::receiveConferenceJoin( const QString & guid, const QStringList & participants )
{
	// get a new GWMM
	KopeteContactPtrList others;
	GroupWiseMessageManager * mgr = messageManager( myself(), others, protocol(), guid );
	// find each contact and add them to the GWMM, and tell them they are in the conference
	for ( QValueList<QString>::ConstIterator it = participants.begin(); it != participants.end(); ++it )
	{
		GroupWiseContact * c = static_cast<GroupWiseContact *>( contacts()[ *it ] );
		if ( c )
		{
			mgr->addContact( c, true );
			c->joinConference( guid );
		}
		else
			kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " couldn't find a contact for DN: " << *it << endl;
	}
	mgr->view( true );
}

void GroupWiseAccount::slotMessageManagerDestroyed( QObject * obj )
{
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " address is: " << obj << endl;
	// the message manager is one of ours
	GroupWiseMessageManager * mgr = static_cast< GroupWiseMessageManager* >( obj );
	kdDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo << " it's one of ours, got its deleted signal" << endl;
	// leave the conference
	m_client->leaveConference( mgr->guid() );
	m_managers.remove( mgr->guid() );
}

void GroupWiseAccount::slotTestRTFize()
{
	bool ok;
	const QString query = QString::fromLatin1("Enter a string to rtfize:");
	QString testText = KLineEditDlg::getText( query, QString::null, &ok, Kopete::UI::Global::mainWidget() );
	if ( ok )
		kdDebug( GROUPWISE_DEBUG_GLOBAL ) << "Converted text is: '" << protocol()->rtfizeText( testText ) << "'" << endl;
}

#include "gwaccount.moc"
