// -*- Mode: c++-mode; c-basic-offset: 2; indent-tabs-mode: t; tab-width: 2; -*-
//
// Copyright (C) 2003 Grzegorz Jaskiewicz 	<gj at pointblue.com.pl>
// Copyright (C) 	2002-2003	 Zack Rusin 	<zack@kde.org>
//
// gaducontact.cpp
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#include <klocale.h>
#include <kaction.h>
#include <kdebug.h>
#include <kfiledialog.h>

#include "gaduaccount.h"
#include "gaduprotocol.h"
#include "gaducontact.h"
#include "gadupubdir.h"
#include "gadueditcontact.h"
#include "gaducontactlist.h"
#include "gadusession.h"

#include "kopetemessagemanagerfactory.h"
#include "kopetegroup.h"
#include "kopetemetacontact.h"
#include "kopetestdaction.h"
#include "kopeteuiglobal.h"

#include "userinfodialog.h"

using Kopete::UserInfoDialog;

GaduContact::GaduContact( uin_t uin, const QString& name, KopeteAccount* account, KopeteMetaContact* parent )
: KopeteContact( account, QString::number( uin ), parent ), uin_( uin )
{
	msgManager_ = 0L;
	account_ = static_cast<GaduAccount*>( account );

	remote_ip	= 0;
	remote_port	= 0;
	version		= 0;
	image_size	= 0;

	thisContact_.append( this );

	initActions();

	// don't call libkopete functions like these until the object is fully
	// constructed. all GaduContact construction must be above this point.
	setFileCapable( false );

	//offline
	setOnlineStatus( GaduProtocol::protocol()->convertStatus( 0 ) );

	setDisplayName( name );
}

QString
GaduContact::identityId() const
{
	return parentIdentity_;
}

void
GaduContact::setParentIdentity( const QString& id)
{
	parentIdentity_ = id;
}

uin_t
GaduContact::uin() const
{
	return uin_;
}

void
GaduContact::sendFile( const KURL &sourceURL, const QString &fileName, uint fileSize )
{
	QString filePath;

	if ( !sourceURL.isValid () ) {
		filePath = KFileDialog::getOpenFileName( QString::null , "*", 0L, i18n ( "Kopete File Transfer" ) );
	}
	else {
		filePath = sourceURL.path( -1 );
	}

	QFile file ( filePath );

	if ( file.exists () ) {
	// transfer the bastard
	}
}


void
GaduContact::changedStatus( KGaduNotify* newstatus )
{
	if ( newstatus->description.isNull() ) {
		setOnlineStatus( GaduProtocol::protocol()->convertStatus( newstatus->status ) );
		removeProperty( GaduProtocol::protocol()->propAwayMessage );
	}
	else {
		setOnlineStatus( GaduProtocol::protocol()->convertStatus( newstatus->status ) );
		setProperty( GaduProtocol::protocol()->propAwayMessage, newstatus->description );
	}

	remote_ip	= newstatus->remote_ip;
	remote_port	= newstatus->remote_port;
	version		= newstatus->version;
	image_size	= newstatus->image_size;

	setFileCapable( newstatus->fileCap );

	kdDebug(14100) << "uin:" << uin() << " port: " << remote_port << " remote ip: " <<  remote_ip << " image size: " << image_size << "  version: "  << version  << endl;

}

unsigned int 
GaduContact::contactIp()
{
	return remote_ip;
}
unsigned short 
GaduContact::contactPort()
{
	return remote_port;
}

KopeteMessageManager*
GaduContact::manager( bool canCreate )
{
	if ( !msgManager_ && canCreate ) {
		msgManager_ = KopeteMessageManagerFactory::factory()->create( account_->myself(), thisContact_, GaduProtocol::protocol() );
		connect( msgManager_, SIGNAL( messageSent( KopeteMessage&, KopeteMessageManager*) ),
			 this, SLOT( messageSend( KopeteMessage&, KopeteMessageManager*) ) );
		connect( msgManager_, SIGNAL( destroyed() ),  this, SLOT( slotMessageManagerDestroyed() ) );

	}
	return msgManager_;
}

void
GaduContact::slotMessageManagerDestroyed()
{
	msgManager_ = 0L;
}

void
GaduContact::initActions()
{
	actionSendMessage_	= KopeteStdAction::sendMessage( this, SLOT( execute() ), this, "actionMessage" );
	actionInfo_		= KopeteStdAction::contactInfo( this, SLOT( slotUserInfo() ), this, "actionInfo" );
}

void
GaduContact::messageReceived( KopeteMessage& msg )
{
	manager()->appendMessage( msg );
}

void
GaduContact::messageSend( KopeteMessage& msg, KopeteMessageManager* mgr )
{
	if ( msg.plainBody().isEmpty() ) {
		return;
	}
	mgr->appendMessage( msg );
	account_->sendMessage( uin_, msg );
}

bool
GaduContact::isReachable()
{
	return account_->isConnected();
}

QPtrList<KAction>*
GaduContact::customContextMenuActions()
{
	QPtrList<KAction> *fakeCollection = new QPtrList<KAction>();
	//show profile
	KAction* actionShowProfile = new KAction( i18n("Show Profile") , "info", 0,
						this, SLOT( slotShowPublicProfile() ),
						this, "actionShowPublicProfile" );

	fakeCollection->append( actionShowProfile );

	KAction* actionEditContact = new KAction( i18n("Edit...") , "edit", 0,
						this, SLOT( slotEditContact() ),
						this, "actionEditContact" );

	fakeCollection->append( actionEditContact );

	return fakeCollection;
}

void
GaduContact::slotEditContact()
{
	new GaduEditContact( static_cast<GaduAccount*>(account()), this, Kopete::UI::Global::mainWidget() );
}

void
GaduContact::slotShowPublicProfile()
{
	account_->slotSearch( uin_ );
}

void
GaduContact::slotUserInfo()
{
	/// FIXME: use more decent information here
	UserInfoDialog *dlg = new UserInfoDialog( i18n( "Gadu contact" ) );

	dlg->setName( metaContact()->displayName() );
	dlg->setId( QString::number( uin_ ) );
	dlg->setStatus( onlineStatus().description() );
	dlg->setAwayMessage( description_ );
	dlg->show();
}

void
GaduContact::slotDeleteContact()
{
	account_->removeContact( this );
	deleteLater();
}

void
GaduContact::serialize( QMap<QString, QString>& serializedData, QMap<QString, QString>& )
{
	serializedData[ "email" ]	= property( GaduProtocol::protocol()->propEmail ).value().toString();
	serializedData[ "FirstName"  ]	= property( GaduProtocol::protocol()->propFirstName ).value().toString();
	serializedData[ "SecondName" ]	= property( GaduProtocol::protocol()->propLastName ).value().toString();
	serializedData[ "telephone" ]	= property( GaduProtocol::protocol()->propPhoneNr ).value().toString();
	serializedData[ "ignored" ]	= ignored_ ? "true" : "false";
}

bool
GaduContact::setContactDetails( const GaduContactsList::ContactLine* cl )
{
	setProperty( GaduProtocol::protocol()->propEmail, cl->email );
	setProperty( GaduProtocol::protocol()->propFirstName, cl->firstname );
	setProperty( GaduProtocol::protocol()->propLastName, cl->surname );
	setProperty( GaduProtocol::protocol()->propPhoneNr, cl->phonenr );
	//setProperty( "ignored", i18n( "ignored" ), cl->ignored ? "true" : "false" );
	ignored_ = cl->ignored;
	//setProperty( "nickName", i18n( "nick name" ), cl->nickname );

	return true;
}

GaduContactsList::ContactLine*
GaduContact::contactDetails()
{
	KopeteGroupList		groupList;
	QString			groups;

	GaduContactsList::ContactLine* cl = new GaduContactsList::ContactLine;

	cl->firstname	= property( GaduProtocol::protocol()->propFirstName ).value().toString();
	cl->surname	= property( GaduProtocol::protocol()->propLastName ).value().toString();
	//cl->nickname	= property( "nickName" ).value().toString();
	cl->email	= property( GaduProtocol::protocol()->propEmail ).value().toString();
	cl->phonenr	= property( GaduProtocol::protocol()->propPhoneNr ).value().toString();
	cl->ignored	= ignored_; //( property( "ignored" ).value().toString() == "true" );

	cl->uin		= QString::number( uin_ );
	cl->displayname	= metaContact()->displayName();

	groupList = metaContact()->groups();

	KopeteGroup* gr;
	for ( gr = groupList.first (); gr ; gr = groupList.next () ) {
// if present in any group, don't export to top level
// FIXME: again, probably bug in libkopete
// in case of topLevel group, KopeteGroup::displayName() returns "TopLevel" ineasted of just " " or "/"
// imo TopLevel group should be detected like i am doing that below
		if ( gr!=KopeteGroup::topLevel() ) {
			groups += gr->displayName()+",";
		}
	}

	if ( groups.length() ) {
		groups.truncate( groups.length()-1 );
	}
	cl->group = groups;

	return cl;
}

QString
GaduContact::findBestContactName( const GaduContactsList::ContactLine* cl )
{
	QString name;

	if ( cl == NULL ) {
		return name;
	}

	if ( cl->uin.isEmpty() ) {
		return name;
	}

	name = cl->uin;

	if ( cl->displayname.length() ) {
		name = cl->displayname;
	}
	else {
		// no name either
		if ( cl->nickname.isEmpty() ) {
			// maybe we can use fistname + surname ?
			if ( cl->firstname.isEmpty() && cl->surname.isEmpty() ) {
				name = cl->uin;
			}
			// what a shame, i have to use UIN than :/
			else {
				if ( cl->firstname.isEmpty() ) {
					name = cl->surname;
				}
				else {
					if ( cl->surname.isEmpty() ) {
						name = cl->firstname;
					}
					else {
						name = cl->firstname + " " + cl->surname;
					}
				}
			}
		}
		else {
			name = cl->nickname;
		}
	}

	return name;
}

void
GaduContact::messageAck()
{
	manager()->messageSucceeded();
}

void
GaduContact::setIgnored( bool val )
{
	ignored_ = val;
}

bool
GaduContact::ignored()
{
	return ignored_;
}

#include "gaducontact.moc"
