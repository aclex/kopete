/*
    ircaccount.cpp - IRC Account

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003-2004 by Jason Keirstead <jason@keirstead.org>
    Copyright (c) 2003-2005 by Michel Hermier <michel.hermier@wanadoo.fr>

    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "ircaccount.h"

#include "ircconst.h"
#include "irccontact.h"
#include "ircprotocol.h"

#include "channellistdialog.h"

#include "kircengine.h"

#include "kopeteaccountmanager.h"
#include "kopeteaway.h"
#include "kopeteawayaction.h"
#include "kopetechatsessionmanager.h"
#include "kopetecommandhandler.h"
#include "kopetecontactlist.h"
#include "kopetemetacontact.h"
#include "kopeteuiglobal.h"
#include "kopeteview.h"
#include "kopetepassword.h"

#include <kaction.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcompletionbox.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <knotifyclient.h>
#include <kpopupmenu.h>

#include <qtextcodec.h>

using namespace IRC;
using namespace Kopete;

IRCAccount::IRCAccount(IRCProtocol *protocol, const QString &accountId, const QString &autoChan, const QString& netName, const QString &nickName)
	: PasswordedAccount(protocol, accountId, 0, true),
	  m_engine(new KIRC::Engine(this)),
	  autoConnect(autoChan),
	  commandSource(0)
{
	m_manager = 0;
	m_channelList = 0;

	m_engine->setUserName(userName());
	m_engine->setRealName(realName());

	QMap<QString, QString> replies = customCtcpReplies();
	for (QMap<QString, QString>::ConstIterator it = replies.begin(); it != replies.end(); ++it)
		m_engine->addCustomCtcp(it.key(), it.data());

	QString version=i18n("Kopete IRC Plugin %1 [http://kopete.kde.org]").arg(kapp->aboutData()->version());
	m_engine->setVersionString( version  );

	QObject::connect(m_engine, SIGNAL(successfullyChangedNick(const QString &, const QString &)),
			this, SLOT(successfullyChangedNick(const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingFailedServerPassword()),
			this, SLOT(slotFailedServerPassword()));

	QObject::connect(m_engine, SIGNAL(incomingNickInUse(const QString &)),
			this, SLOT(slotNickInUseAlert( const QString &)) );

	QObject::connect(m_engine, SIGNAL(incomingFailedNickOnLogin(const QString &)),
			this, SLOT(slotNickInUse( const QString &)) );

	QObject::connect(m_engine, SIGNAL(incomingJoinedChannel(const QString &, const QString &)),
		this, SLOT(slotJoinedUnknownChannel(const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingCtcpReply(const QString &, const QString &, const QString &)),
			this, SLOT( slotNewCtcpReply(const QString&, const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(connectionStateChanged(KIRC::ConnectionState)),
			 this, SLOT(engineConnectionStateChanged(KIRC::ConnectionState)));

	QObject::connect(m_engine, SIGNAL(receivedMessage(KIRC::MessageType, const KIRC::EntityPtr &, const KIRC::EntityPtrList &, const QString &)),
			 this, SLOT(receivedMessage(KIRC::MessageType, const KIRC::EntityPtr &, const KIRC::EntityPtrList &, const QString &)));

	currentHost = 0;

//	loadProperties();

	m_server = new IRCContact(this, m_engine->server());
	m_self = new IRCContact(this, m_engine->self());
	setMyself(m_self);
/*
	QString m_accountId = this->accountId();
	if (networkName.isEmpty() && QRegExp( "[^#+&\\s]+@[\\w-\\.]+:\\d+" ).exactMatch(m_accountId))
	{
		kdDebug(14120) << "Creating account from " << m_accountId << endl;

//		mNickName = m_accountId.section('@',0,0);
		QString serverInfo = m_accountId.section('@',1);
		QString hostName = serverInfo.section(':',0,0);

		QValueList<IRCNetwork> networks = IRCNetworkList::self()->networks();
		for (QValueList<IRCNetwork>::Iterator it = networks.begin(); it != networks.end(); ++it)
		{
			IRCNetwork net = *it;
			for (QValueList<IRCHost>::iterator it2 = net.hosts.begin(); it2 != net.hosts.end(); ++it2)
			{
				if( (*it2).host == hostName )
				{
					setNetwork(net.name);
					break;
				}
			}

			if( !networkName.isEmpty() )
				break;
		}

		if( networkName.isEmpty() )
		{
			// Could not find this host. Add it to the networks structure

			m_network = IRCNetwork();
			m_network.name = i18n("Temporary Network - %1").arg( hostName );
			m_network.description = i18n("Network imported from previous version of Kopete, or an IRC URI");

			IRCHost host;
			host.host = hostName;
			host.port = serverInfo.section(':',1).toInt();
			if (!password().cachedValue().isEmpty())
				host.password = password().cachedValue();
			host.ssl = false;

			m_network.hosts.append( host );
//			m_protocol->addNetwork( m_network );

			config->writeEntry(Config::NETWORKNAME, m_network.name);
//			config->writeEntry(Config::NICKNAME, mNickName);
		}
	}
	else if( !networkName.isEmpty() )
	{
		setNetwork(networkName);
	}
	else
	{
		kdError() << "No network name defined, and could not import network information from ID" << endl;
	}
*/

//	setAccountLabel( QString::fromLatin1("%1@%2").arg(mNickName,networkName) );
/*
	m_awayAction = new AwayAction(i18n("Set Away"),
		m_protocol->m_UserStatusAway.iconFor(this), 0, this,
		SLOT(slotGoAway( const QString & )), this);
*/
	m_joinChannelAction = new KAction ( i18n("Join Channel..."), QString::null, 0, this,
				SLOT(slotJoinChannel()), this);
	m_searchChannelAction = new KAction ( i18n("Search Channels..."), QString::null, 0, this,
				SLOT(slotSearchChannels()), this);
}

IRCAccount::~IRCAccount()
{
	if (m_engine->isConnected())
		m_engine->quit(i18n("Plugin Unloaded"), true);
}

/*
void IRCAccount::loadProperties()
{
	KConfigGroup *config = configGroup();

	QString networkName = netName;
	if (networkName.isNull())
		networkName = config->readEntry(Config::NETWORKNAME);

	if (!nickName.isNull())
		setNickName(nickName);
//	else
//		mNickName = config->readEntry(Config::NICKNAME);

	QString codecMib = config->readEntry(Config::CODECMIB);
	//	int codecMib = config->readNumEntry(Config::CODECMIB, UTF-8);

//	m_serverNotices = (MessageDestination)config->readNumEntry( "ServerNotices", ServerWindow );
//	m_serverMessages = (MessageDestination)config->readNumEntry( "ServerMessages", ServerWindow );
//	m_informationReplies = (MessageDestination)config->readNumEntry( "InformationReplies", ActiveWindow );
//	m_errorMessages = (MessageDestination)config->readNumEntry( "ErrorMessages", ActiveWindow );
	autoShowServerWindow = config->readBoolEntry( "AutoShowServerWindow", false );

	if( !codecMib.isEmpty() )
	{
		mCodec = QTextCodec::codecForMib( codecMib.toInt() );
		m_engine->setDefaultCodec( mCodec );
	}
	else
		mCodec = 0;
}

void IRCAccount::storeProperties()
{
}
*/
int IRCAccount::codecMib() const
{
	return configGroup()->readNumEntry(Config::CODECMIB);
}

void IRCAccount::setCodecFromMib(int mib)
{
	configGroup()->writeEntry(Config::CODECMIB, mib);
	m_engine->setDefaultCodec(QTextCodec::codecForMib(mib));
}

QTextCodec *IRCAccount::codec() const
{
	return QTextCodec::codecForMib(codecMib());
}

void IRCAccount::setCodec( QTextCodec *codec )
{
	if (codec)
		setCodecFromMib(codec->mibEnum());
	else
		setCodecFromMib(-1); // MIBenum are >= 0  so we inforce an error value
}

bool IRCAccount::autoShowServerWindow() const
{
	return configGroup()->readBoolEntry(QString::fromLatin1("AutoShowServerWindow"));
}

void IRCAccount::setAutoShowServerWindow(bool autoShow)
{
	configGroup()->writeEntry(QString::fromLatin1("AutoShowServerWindow"), autoShow);
}

const QString IRCAccount::nickName() const
{
	return configGroup()->readEntry(Config::NICKNAME);
}

void IRCAccount::setNickName(const QString &nickName)
{
	configGroup()->writeEntry(Config::NICKNAME, nickName);
	m_self->setNickName(nickName);
}
/*
const QString IRCAccount::altNick() const
{
	return configGroup()->readEntry(QString::fromLatin1("altNick"));
}

void IRCAccount::setAltNick( const QString &altNick )
{
	configGroup()->writeEntry(QString::fromLatin1( "altNick" ), altNick);
}
*/
const QString IRCAccount::userName() const
{
	return configGroup()->readEntry(Config::USERNAME);
}

void IRCAccount::setUserName(const QString &userName)
{
	configGroup()->writeEntry(Config::USERNAME, userName);
	m_engine->setUserName(userName);
}

const QString IRCAccount::realName() const
{
	return configGroup()->readEntry(Config::REALNAME);
}

void IRCAccount::setRealName( const QString &userName )
{
	configGroup()->writeEntry(Config::REALNAME, userName);
	m_engine->setRealName(userName);
}

const QString IRCAccount::networkName() const
{
	return m_network.name;
}
/*
void IRCAccount::setNetworkByName(const QString &network)
{
	m_network = network;
//	configGroup()->writeEntry(Config::NETWORKNAME, network.name);
//	setAccountLabel(network.name);
}

const IRCNetwork &network() const
{
	return m_network;
}
*/
void IRCAccount::setNetwork(const IRCNetwork &network)
{
//	configGroup()->writeEntry(Config::NETWORKNAME, network.name);
	m_network = network;
//	setAccountLabel(network.name);
}

KIRC::Engine *IRCAccount::engine() const
{
	return m_engine;
}

// FIXME: Move this to a dictionnary
void IRCAccount::setDefaultPart( const QString &defaultPart )
{
	configGroup()->writeEntry( QString::fromLatin1( "defaultPart" ), defaultPart );
}

// FIXME: Move this to a dictionnary
void IRCAccount::setDefaultQuit( const QString &defaultQuit )
{
	configGroup()->writeEntry( QString::fromLatin1( "defaultQuit" ), defaultQuit );
}

// FIXME: Move this to a dictionnary
const QString IRCAccount::defaultPart() const
{
	QString partMsg = configGroup()->readEntry(QString::fromLatin1("defaultPart"));
	if( partMsg.isEmpty() )
		return QString::fromLatin1("Kopete %1 : http://kopete.kde.org").arg( kapp->aboutData()->version() );
	return partMsg;
}

const QString IRCAccount::defaultQuit() const
{
	QString quitMsg = configGroup()->readEntry(QString::fromLatin1("defaultQuit"));
	if( quitMsg.isEmpty() )
		return QString::fromLatin1("Kopete %1 : http://kopete.kde.org").arg(kapp->aboutData()->version());
	return quitMsg;
}

void IRCAccount::setCustomCtcpReplies( const QMap< QString, QString > &replies ) const
{
	QStringList val;
	for( QMap< QString, QString >::ConstIterator it = replies.begin(); it != replies.end(); ++it )
	{
		m_engine->addCustomCtcp( it.key(), it.data() );
		val.append( QString::fromLatin1("%1=%2").arg( it.key() ).arg( it.data() ) );
	}

	configGroup()->writeEntry( "CustomCtcp", val );
}

const QMap< QString, QString > IRCAccount::customCtcpReplies() const
{
	QMap< QString, QString > replies;
	QStringList replyList;

	replyList = configGroup()->readListEntry( "CustomCtcp" );

	for( QStringList::Iterator it = replyList.begin(); it != replyList.end(); ++it )
		replies[ (*it).section('=', 0, 0 ) ] = (*it).section('=', 1 );

	return replies;
}

void IRCAccount::setConnectCommands( const QStringList &commands ) const
{
	configGroup()->writeEntry( "ConnectCommands", commands );
}

const QStringList IRCAccount::connectCommands() const
{
	return configGroup()->readListEntry( "ConnectCommands" );
}

void IRCAccount::setMessageDestinations( int serverNotices, int serverMessages,
			     int informationReplies, int errorMessages )
{
/*
	KConfigGroup *config =  configGroup();
	config->writeEntry( "ServerNotices", serverNotices );
	config->writeEntry( "ServerMessages", serverMessages );
	config->writeEntry( "InformationReplies", informationReplies );
	config->writeEntry( "ErrorMessages", errorMessages );

	m_serverNotices = (MessageDestination)serverNotices;
	m_serverMessages = (MessageDestination)serverMessages;
	m_informationReplies = (MessageDestination)informationReplies;
	m_errorMessages = (MessageDestination)errorMessages;
*/
}

KActionMenu *IRCAccount::actionMenu()
{
	QString menuTitle = QString::fromLatin1( " %1 <%2> " ).arg( accountId() ).arg( myself()->onlineStatus().description() );

	KActionMenu *mActionMenu = Account::actionMenu();

	m_joinChannelAction->setEnabled( isConnected() );
	m_searchChannelAction->setEnabled( isConnected() );

	mActionMenu->popupMenu()->insertSeparator();
	mActionMenu->insert(m_joinChannelAction);
	mActionMenu->insert(m_searchChannelAction);
	mActionMenu->insert( new KAction ( i18n("Show Server Window"), QString::null, 0, this, SLOT(slotShowServerWindow()), mActionMenu ) );

//	if (m_engine->isConnected() && m_engine->useSSL())
	{
		mActionMenu->insert( new KAction ( i18n("Show Security Information"), "", 0, m_engine,
			SLOT(showInfoDialog()), mActionMenu ) );
	}

	return mActionMenu;
}

void IRCAccount::connectWithPassword(const QString &password)
{
	//TODO:  honor the initial status

	if ( m_engine->isConnected() )
	{
		if( isAway() )
			setAway( false );
	}
	else if ( m_engine->isDisconnected() )
	{
//		if( m_network )
		{
/*
			QValueList<IRCHost*> &hosts = m_network->hosts;
			if( hosts.count() == 0 )
			{
				KMessageBox::queuedMessageBox(
					UI::Global::mainWidget(), KMessageBox::Error,
					i18n("<qt>The network associated with this account, <b>%1</b>, has no valid hosts. Please ensure that the account has a valid network.</qt>").arg(m_network->name),
					i18n("Network is Empty"), 0 );
			}
			else if( currentHost == hosts.count() )
			{
			    KMessageBox::queuedMessageBox(
				    UI::Global::mainWidget(), KMessageBox::Error,
			    i18n("<qt>Kopete could not connect to any of the servers in the network associated with this account (<b>%1</b>). Please try again later.</qt>").arg(m_network->name),
			    i18n("Network is Unavailable"), 0 );

			    currentHost = 0;
			}
			else
			{
				// if prefer SSL is set, sort by SSL first
				if (configGroup()->readBoolEntry("PreferSSL"))
				{
					typedef QValueList<IRCHost*> IRCHostList;
					IRCHostList sslFirst;
					IRCHostList::iterator it;
					for ( it = hosts.begin(); it != hosts.end(); ++it )
					{
						if ( (*it)->ssl == true )
						{
							sslFirst.append( *it );
							it = hosts.remove( it );
						}
					}
					for ( it = hosts.begin(); it != hosts.end(); ++it )
						sslFirst.append( *it );

					hosts = sslFirst;
				}

				IRCHost *host = hosts[ currentHost++ ];
				appendInternalMessage( i18n("Connecting to %1...").arg( host->host ) );
				if( host->ssl )
					appendInternalMessage( i18n("Using SSL") );

				m_engine->setPassword(password);
//				m_engine->connectToServer( host->host, host->port, mNickName, host->ssl );
			}
*/
		}
//		else
//		{
//			kdWarning() << "No network defined!" << endl;
//		}
	}
}

void IRCAccount::engineConnectionStateChanged(KIRC::ConnectionState newstate)
{
	kdDebug(14120) << k_funcinfo << endl;

	mySelf()->updateStatus();

	switch (newstate)
	{
	case KIRC::Idle:
		// Do nothing.
		break;
	case KIRC::Connecting:
	{
		if (autoShowServerWindow())
			myServer()->startChat();
		break;
	}
	case KIRC::Authentifying:
		break;
	case KIRC::Connected:
		{
			//Reset the host so re-connection will start over at first server
			currentHost = 0;
//			m_contactManager->addToNotifyList( m_engine->nickName() );

			// HACK! See bug #85200 for details. Some servers still cannot accept commands
			// after the 001 is sent, you need to wait until all the init junk is done.
			// Unfortunatly, there is no way for us to know when it is done (it could be
			// spewing out any number of replies), so just try delaying it
//			QTimer::singleShot( 250, this, SLOT( slotPerformOnConnectCommands() ) );
		}
		break;
	case KIRC::Closing:
//		mySelf()->setOnlineStatus( m_protocol->m_UserStatusOffline );
//		m_contactManager->removeFromNotifyList( m_engine->nickName() );

//		if (m_contactManager && !autoConnect.isNull())
//			AccountManager::self()->removeAccount( this );
		break;
//	case KIRC::Timeout:
		//Try next server
//		connect();
//		break;
	}
}

void IRCAccount::slotPerformOnConnectCommands()
{
	ChatSession *manager = myServer()->manager(Contact::CanCreate);
	if (!manager)
		return;

	if (!autoConnect.isEmpty())
		CommandHandler::commandHandler()->processMessage( QString::fromLatin1("/join %1").arg(autoConnect), manager);

	QStringList commands(connectCommands());
	for (QStringList::Iterator it=commands.begin(); it != commands.end(); ++it)
		CommandHandler::commandHandler()->processMessage(*it, manager);
}

void IRCAccount::disconnect()
{
	quit();
}

void IRCAccount::quit( const QString &quitMessage )
{
	kdDebug(14120) << "Quitting IRC: " << quitMessage << endl;

	if( quitMessage.isNull() || quitMessage.isEmpty() )
		m_engine->quit( defaultQuit() );
	else
		m_engine->quit( quitMessage );
}

void IRCAccount::setAway(bool isAway, const QString &awayMessage)
{
	kdDebug(14120) << k_funcinfo << isAway << " " << awayMessage << endl;
	if(m_engine->isConnected())
	{
//		static_cast<IRCUserContact *>( myself() )->setAway( isAway );
		engine()->away(isAway, awayMessage);
	}
}

/*
 * Ask for server password, and reconnect
 */
void IRCAccount::slotFailedServerPassword()
{
	// JLN
	password().setWrong();
	connect();
}
void IRCAccount::slotGoAway( const QString &reason )
{
	setAway(true, reason);
}

void IRCAccount::slotShowServerWindow()
{
	m_server->startChat();
}

bool IRCAccount::isConnected()
{
	return m_engine->isConnected();
}

void IRCAccount::setOnlineStatus(const OnlineStatus& status , const QString &reason)
{
	kdDebug(14120) << k_funcinfo << endl;

	#warning REWRITE ME
	if ( status.status() == OnlineStatus::Online && myself()->onlineStatus().status() == OnlineStatus::Offline )
		connect();
	else if ( status.status() == OnlineStatus::Offline )
		disconnect();
	else if ( status.status() == OnlineStatus::Away )
		slotGoAway( reason );
}

void IRCAccount::successfullyChangedNick(const QString &oldnick, const QString &newnick)
{
//	kdDebug(14120) << k_funcinfo << "Changing nick to " << newnick << endl;
//	mNickName = newnick;
//	mySelf()->setNickName( mNickName );
//	m_contactManager->removeFromNotifyList( oldnick );
//	m_contactManager->addToNotifyList( newnick );
}

bool IRCAccount::createContact(const QString &contactId, MetaContact *metac)
{
/*	if (contactId == mNickName)
	{
		KMessageBox::error( UI::Global::mainWidget(),
			i18n("\"You are not allowed to add yourself to your contact list."), i18n("IRC Plugin")
		);

		return false;
	}*/
	IRCContact *contact = getContact(contactId, metac);
/*
	if (contact->metaContact() != metac )
	{//This should NEVER happen
		MetaContact *old = contact->metaContact();
		contact->setMetaContact(metac);
		ContactPtrList children = old->contacts();
		if (children.isEmpty())
			ContactList::self()->removeMetaContact( old );
	}
	else */ if (contact->metaContact()->isTemporary())
		metac->setTemporary(false);

	return true;
}

void IRCAccount::setCurrentCommandSource( ChatSession *session )
{
	commandSource = session;
}

ChatSession *IRCAccount::currentCommandSource()
{
	return commandSource;
}

void IRCAccount::appendErrorMessage(const QString &message)
{
	#warning FIXME make a reall message.
	appendInternalMessage(message);
}

void IRCAccount::appendInternalMessage(const QString &message)
{
	ContactPtrList members;
	members.append(m_server);
	Message msg(m_server, members, message, Message::Internal, Message::RichText, CHAT_VIEW);
	m_server->appendMessage(msg);
}

IRCContact *IRCAccount::myServer() const
{
	return m_server;
}

IRCContact *IRCAccount::mySelf() const
{
	return static_cast<IRCContact *>( myself() );
}

IRCContact *IRCAccount::getContact(const QString &name, MetaContact *metac)
{
	kdDebug(14120) << k_funcinfo << name << endl;
	return getContact(m_engine->getEntity(name), metac);
}

IRCContact *IRCAccount::getContact(KIRC::EntityPtr entity, MetaContact *metac)
{
	IRCContact *contact = 0;

	#warning Do the search code here.

	if (!contact)
	{
		#warning Make a temporary meta contact if metac is null
		contact = new IRCContact(this, entity, metac);
		m_contacts.append(contact);
	}

	QObject::connect(contact, SIGNAL(destroyed(IRCContact *)), SLOT(destroyed(IRCContact *)));
	return contact;
}

void IRCAccount::destroyed(IRCContact *contact)
{
	m_contacts.remove(contact);
}

void IRCAccount::receivedMessage(
		KIRC::MessageType type,
		const KIRC::EntityPtr &from,
		const KIRC::EntityPtrList &to,
		const QString &message)
{
	IRCContact *fromContact = getContact(from);
/*
	Message msg(fromContact, manager()->members(), message, Kopete::Message::Inbound,
		    Kopete::Message::RichText, CHAT_VIEW);

		toContact->appendMessage(msg);
*/
}
/*
void IRCChannelContact::newAction(const QString &from, const QString &action)
{
	IRCAccount *account = ircAccount();

	IRCUserContact *f = account->contactManager()->findUser(from);
	Kopete::Message::MessageDirection dir =
		(f == account->mySelf()) ? Kopete::Message::Outbound : Kopete::Message::Inbound;
	Kopete::Message msg(f, manager()->members(), action, dir, Kopete::Message::RichText,
			    CHAT_VIEW, Kopete::Message::TypeAction);
	appendMessage(msg);
}

void IRCUserContact::newAction(const QString &to, const QString &action)
{
	IRCAccount *account = ircAccount();

	IRCContact *t = account->contactManager()->findUser(to);

	Kopete::Message::MessageDirection dir =
		(this == account->mySelf()) ? Kopete::Message::Outbound : Kopete::Message::Inbound;
	Kopete::Message msg(this, t, action, dir, Kopete::Message::RichText,
			CHAT_VIEW, Kopete::Message::TypeAction);

	//Either this is from me to a guy, or from a guy to me. Either way its a PM
	if (dir == Kopete::Message::Outbound)
		t->appendMessage(msg);
	else
		appendMessage(msg);
}
*/
#include "ircaccount.moc"
