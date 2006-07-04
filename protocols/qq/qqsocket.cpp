/*
     
    qqsocket.cpp - Base class for the sockets used in QQ
    forked from msnsocket.cpp
    
    Copyright (c) 2006         Hui Jin		      <blueangel.jin@gmail.com>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart at kde.org>
    Copyright (c) 2005		by Gregg Edghill 		  <gregg.edghill@gmail.com>

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    Portions of this code are taken from KMerlin,
              (c) 2001      by Olaf Lueg              <olueg@olsd.de>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "qqsocket.h"
#include "qqprotocol.h"

#include <QTimer>
#include <QByteArray>

#include <kdebug.h>
#include <kconfig.h>
#include <kbufferedsocket.h>
#include <kserversocket.h>
#include <kresolver.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>

#include "kopeteuiglobal.h"

using namespace KNetwork;

QQSocket::QQSocket(QObject* parent)  : QObject (parent)
{
	m_onlineStatus = Disconnected;
	m_socket = 0L;
}

QQSocket::~QQSocket()
{
	doneDisconnect();
	if ( m_socket )
		m_socket->deleteLater();
}

void QQSocket::connect( const QString &server, uint port )
{
	if ( m_onlineStatus == Connected || m_onlineStatus == Connecting )
	{
		kWarning( 14140 ) << k_funcinfo << "Already connected or connecting! Not connecting again." << endl;
		return;
	}

	if( m_onlineStatus == Disconnecting )
	{
		// Cleanup first.
		// FIXME: More generic!!!
		kWarning( 14140 ) << k_funcinfo << "We're still disconnecting! Deleting socket the hard way first." << endl;
		delete m_socket;
	}
	setOnlineStatus( Connecting );
	m_id = 5; // FIXME:Don't use the magic #, use random number instead.
	m_server = server;
	m_port = port;
	m_socket = new KBufferedSocket( server, QString::number(port) );
	m_socket->enableRead( true );

	// enableWrite eats the CPU, and we only need it when the queue is
	// non-empty, so disable it until we have actual data in the queue
	m_socket->enableWrite( false );

	QObject::connect( m_socket, SIGNAL( readyRead() ),             this, SLOT( slotDataReceived() ) );
	QObject::connect( m_socket, SIGNAL( readyWrite() ),            this, SLOT( slotReadyWrite() ) );
	QObject::connect( m_socket, SIGNAL( hostFound() ),	       this, SLOT( slotHostFound() ) );
	QObject::connect( m_socket, SIGNAL( connected( const KNetwork::KResolverEntry &) ), this, SLOT( slotConnectionSuccess() ) );
	QObject::connect( m_socket, SIGNAL( gotError( int ) ),         this, SLOT( slotSocketError( int ) ) );
	QObject::connect( m_socket, SIGNAL( closed( ) ),               this, SLOT( slotSocketClosed( ) ) );

	aboutToConnect();

	// start the asynchronous connection
	m_socket->connect();
}

void QQSocket::disconnect()
{
	if ( m_socket )
		m_socket->close();
	else
		slotSocketClosed();
}

void QQSocket::aboutToConnect()
{
	/* Empty default implementation */
}

void QQSocket::doneConnect()
{
	setOnlineStatus( Connected );
}

void QQSocket::doneDisconnect()
{
	setOnlineStatus( Disconnected );
}

void QQSocket::setOnlineStatus( QQSocket::OnlineStatus status )
{
	if ( m_onlineStatus == status )
		return;

	m_onlineStatus = status;
	emit onlineStatusChanged( status );
}


void QQSocket::sendPacket( const QByteArray& data )
{
	kDebug(14140) << k_funcinfo << data <<  endl;
	m_sendQueue.append( data );
	m_socket->enableWrite(true);
}

void QQSocket::slotSocketError( int error )
{
	kWarning( 14140 ) << k_funcinfo << "Error: " << error << " (" << m_socket->errorString() << ")" << endl;

	if(!KSocketBase::isFatalError(error))
		return;
		//we only care about fatal error

	QString errormsg = i18n( "There was an error while connecting to the QQ server.\nError message:\n" );
	if ( error == KSocketBase::LookupFailure )
		errormsg += i18n( "Unable to lookup %1", m_socket->peerResolver().nodeName() );
	else
		errormsg +=  m_socket->errorString() ;

	//delete m_socket;
	m_socket->deleteLater();
	m_socket = 0L;

	setOnlineStatus( Disconnected );
	emit connectionFailed();
	//like if the socket is closed
	emit socketClosed();

	//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Error, errormsg, i18n( "QQ Plugin" ) );
	emit errorMessage( ErrorNormal, errormsg );
}

void QQSocket::slotDataReceived()
{
	kDebug(14140) << k_funcinfo << "DATA RECEIVED! " << endl;
	int avail = m_socket->bytesAvailable();
	if ( avail < 0 )
	{
		// error!
		kWarning( 14140 ) << k_funcinfo << "bytesAvailable() returned " << avail
		 	<< ". This should not happen!" << endl
			<< "Are we disconnected? Backtrace:" << endl << kBacktrace() << endl;
		return;
	}

	char *buffer = new char[ avail + 1 ];
	int ret = m_socket->read( buffer, avail );

	if ( ret < 0 )
	{
		kWarning( 14140 ) << k_funcinfo << "read() returned " << ret << "!" <<endl;
	}
	else if ( ret == 0 )
	{
		kWarning( 14140 ) << k_funcinfo << "read() returned no data!" <<endl;
	}
	else
	{
		if ( avail )
		{
			if ( ret != avail)
			{
				kWarning( 14140 ) << k_funcinfo << avail << " bytes were reported available, "
					<< "but read() returned only " << ret << " bytes! Proceeding anyway." << endl;
			}
		}
		else
		{
			kDebug( 14140 ) << k_funcinfo << "Read " << ret << " bytes into 4kb block." << endl;
		}
		
		// FIXME: Here we assume that the packet is fetched in one shot, 
		// and no producer/consumer race condition, is this true ?
		// TODO: memory overhead, use a smart pointer here later.
		QByteArray buf(buffer, ret );

		// FIXME: do we need a incoming message pool right now ?

		parsePacket(buf);
	}

	// Cleanup.
	delete[] buffer;
}


void QQSocket::handleError( uint code, uint /* id */ )
{
	kDebug(14140) << k_funcinfo << endl;
	QString msg;

	switch ( code )
	{
	default:
		// FIXME: if the error causes a disconnect, it will crash, but we can't disconnect every time
		msg = i18n( "Unhandled QQ error code %1 \n"
			"Please fill a bug report with a detailed description and if possible the last console debug output.", code );
		break;
	}

	if ( !msg.isEmpty() )
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Error, msg, i18n( "QQ Plugin" ) );
		emit errorMessage( ErrorNormal, msg );

	return;
}

void QQSocket::slotReadyWrite()
{
	kDebug(14140) << k_funcinfo << endl;
	if ( !m_sendQueue.isEmpty() )
	{
		// If the command queue is not empty, retrieve the first command.
		QList<QByteArray>::Iterator it = m_sendQueue.begin();
		
		// Otherwise, send the command normally.
		kDebug( 14141 ) << k_funcinfo << "Sending command: " << QString( *it ).trimmed() << endl;
		m_socket->write( *it, ( *it ).size() );
		m_sendQueue.erase( it );

		// If the queue is empty agalin stop waiting for readyWrite signals
		// because of the CPU usage
		if ( m_sendQueue.isEmpty() )
			m_socket->enableWrite( false );
	}
	else
		m_socket->enableWrite( false );
}


void QQSocket::slotConnectionSuccess()
{
	kDebug ( 14140 ) << "slotConnectionSuccess: calling doneConnect()" << endl;
	doneConnect();
}

void QQSocket::slotHostFound()
{
        // nothing to do
}

void QQSocket::slotSocketClosed()
{
    kDebug( 14140 ) << k_funcinfo << "Socket closed. " << endl;

	if ( !m_socket ||  m_onlineStatus == Disconnected )
	{
		kDebug( 14140 ) << k_funcinfo << "Socket already deleted or already disconnected" << endl;
		return;
	}

	doneDisconnect();

	m_socket->deleteLater();
	m_socket = 0L;

	emit socketClosed();
}

QString QQSocket::getLocalIP()
{
	if ( !m_socket )
		return QString::null;

	const KSocketAddress address = m_socket->localAddress();

	QString ip = address.nodeName();

	kDebug( 14140 ) << k_funcinfo << "IP: " << ip  <<endl;
	return ip;
}

#include "qqsocket.moc"

// vim: set noet ts=4 sts=4 sw=4:
