/*
    kopetecontact.cpp - Kopete Contact

    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002 by Martijn Klingens       <klingens@kde.org>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontact.h"
#include "kopetemetacontact.h"
#include "kopete.h"

#include <qtimer.h>
#include <qpixmap.h>
#include <qimage.h>
#include <klistview.h>
#include <kdebug.h>
#include <klocale.h>

KopeteContact::KopeteContact( const QString &protocolId, KopeteMetaContact *parent )
	: QObject( parent )
{
	connect(this, SIGNAL(incomingEvent(KopeteEvent *)), kopeteapp, SLOT(notifyEvent(KopeteEvent *)));

	m_metaContact = parent;
	m_protocolId = protocolId;
}

KopeteContact::~KopeteContact()
{
}

void KopeteContact::setDisplayName( const QString &name )
{
	m_displayName = name;
	emit displayNameChanged( name );
}

QString KopeteContact::displayName() const
{
	return m_displayName;
}

KopeteContact::ContactStatus KopeteContact::status() const
{
	return Online;
}

QString KopeteContact::statusText() const
{
	ContactStatus stat = status();

	//kdDebug() << "[KopeteContact] statusText() with status= " << stat << endl;

	switch( stat )
	{
	case Online:
		return i18n("Online");
	case Away:
		return i18n("Away");
	case Offline:
	default:
		return i18n("Offline");
	}
}

QString KopeteContact::toXML()
{
	QString xml;
	QString displayname = displayName();
	QString data = this->data();

	xml = "<contact";

	kdDebug() << "KC: " << protocol() << endl;

	if ( m_protocolId.isNull() )
	{
		xml = xml + " protocol=\"" + protocol() + "\"";

		if ( ! displayname.isNull() )
			xml = xml + " name=\"" + displayName() + "\"";
		if ( ! data.isNull() )
			xml = xml + " data=\"" + this->data() + "\"";
	}
	else
	{
		return QString::null;
	}

	xml = xml + "/>";
	return xml;
}


QString KopeteContact::statusIcon() const
{
	return "unknown";
}

QPixmap KopeteContact::scaledStatusIcon(int size)
{
    if ( (this->status() != m_cachedOldStatus) || ( size != m_cachedSize ) )
	{
		QImage afScal = ((QPixmap(SmallIcon(this->statusIcon()))).convertToImage()).smoothScale( size, size);
		m_cachedScaledIcon = QPixmap(afScal);
		m_cachedOldStatus = this->status();
		m_cachedSize = size;
		return m_cachedScaledIcon;
	}
	else
	{
		return m_cachedScaledIcon;	
	}
}

int KopeteContact::importance() const
{
	ContactStatus stat = status();

	kdDebug() << "[KopeteContact] importance() with status= " << stat << endl;

	if (stat == Online)
		return 20;

	if (stat == Away)
		return 10;

	if (stat == Offline)
		return 0;

	return 0;
}

QStringList KopeteContact::groups()
{
	return QStringList();
}

void KopeteContact::addToGroup( const QString & /* group */ )
{
	kdDebug() << "KopeteContact::addToGroup: WARNING: "
		<< "Default implementation called! Function not implemented?" << endl;
}

void KopeteContact::removeFromGroup( const QString & /* group */ )
{
	kdDebug() << "KopeteContact::removeFromGroup: WARNING: "
		<< "Default implementation called! Function not implemented?" << endl;
}

void KopeteContact::moveToGroup( const QString & /* from */,
	const QString & /* to */ )
{
	kdDebug() << "KopeteContact::moveToGroup: WARNING: "
		<< "Default implementation called! Function not implemented?" << endl;
}


#include "kopetecontact.moc"

// vim: set noet ts=4 sts=4 sw=4:

