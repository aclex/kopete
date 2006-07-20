/*
    Kopete Oscar Protocol
    changevisibilitytask.cpp - Changes the visibility of the account via SSI

    Copyright (c) 2005 Matt Rogers <mattr@kde.org>

    Kopete (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "changevisibilitytask.h"

#include <QList>
#include <kdebug.h>
#include "buffer.h"
#include "client.h"
#include "connection.h"
#include "oscartypeclasses.h"
#include "oscartypes.h"
#include "oscarutils.h"
#include "contactmanager.h"
#include "transfer.h"


ChangeVisibilityTask::ChangeVisibilityTask(Task* parent): Task(parent)
{
	m_sequence = 0;
	m_visible = true;
}


ChangeVisibilityTask::~ChangeVisibilityTask()
{
}

void ChangeVisibilityTask::setVisible( bool visible )
{
	m_visible = visible;
}

bool ChangeVisibilityTask::forMe(const Transfer* transfer) const
{
	const SnacTransfer* st = dynamic_cast<const SnacTransfer*>( transfer );
	if ( !st )
		return false;
	
	SNAC s = st->snac(); //cheat
	if ( s.family == 0x0013 && s.subtype == 0x000E )
		return true;
	else
		return false;
}

bool ChangeVisibilityTask::take(Transfer* transfer)
{
	if ( forMe( transfer ) )
	{
		setTransfer( transfer );
		setSuccess( 0, QString::null );
		setTransfer( 0 );
		return true;
	}
	else
	{
		setError( 0, QString::null );
		return false;
	}
}

void ChangeVisibilityTask::onGo()
{
	ContactManager* manager = client()->ssiManager();
	OContact item = manager->visibilityItem();
	OContact newContact;
	if ( !item )
	{
		kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Didn't find a visibility item" << endl;
		return;
	}
	
	//remove the old item and add the new item indicating the
	//change in visibility.
	manager->removeItem( item );
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "found visibility item. changing setting" << endl;
	newContact = OContact( item.name(), item.gid(), item.bid(), item.type(), QList<TLV>(), 0 );
	QList<TLV> newList;
	QList<TLV>::const_iterator it = item.tlvList().begin(), listEnd = item.tlvList().end();
	for ( ; it != listEnd; ++it )
	{
		if ( ( *it ).type != 0x00CA )
		{
			TLV t = ( *it );
			kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "adding TLV of type" << t.type << endl;
			newList.append( t );
		}
	}
	
	Buffer c8tlv;
	BYTE visibleByte = m_visible ? 0x04 : 0x03;
	c8tlv.addByte( visibleByte );
	
	TLV c8( 0x00CA, c8tlv.length(), c8tlv.buffer() );
	
	newList.append( c8 );
	newContact.setTLVList( newList );
	manager->newItem( newContact );
	sendEditStart();
	
	Buffer* b = new Buffer();
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0013, 0x0009, 0x0000, client()->snacSequence() };
	m_sequence = s.id;
	b->addWord( 0 );
	b->addWord( newContact.gid() );
	b->addWord( newContact.bid() );
	b->addWord( newContact.type() );
	b->addWord( newContact.tlvListLength() );
	
	QList<TLV>::const_iterator it2 =  newContact.tlvList().begin();
	QList<TLV>::const_iterator listEnd2 = newContact.tlvList().end();
	for( ; it2 != listEnd2; ++it2 )
		b->addTLV( ( *it2 ) );
	
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Sending visibility update" << endl;
	Transfer* t = createTransfer( f, s, b );
	send( t );
	sendEditEnd();
}

void ChangeVisibilityTask::sendEditStart()
{
	SNAC editStartSnac = { 0x0013, 0x0011, 0x0000, client()->snacSequence() };
	FLAP editStart = { 0x02, 0, 0 };
	Buffer* emptyBuffer = new Buffer;
	Transfer* t1 = createTransfer( editStart, editStartSnac, emptyBuffer );
	send( t1 );
}

void ChangeVisibilityTask::sendEditEnd()
{
	SNAC editEndSnac = { 0x0013, 0x0012, 0x0000, client()->snacSequence() };
	FLAP editEnd = { 0x02, 0, 0 };
	Buffer* emptyBuffer = new Buffer;
	Transfer *t5 = createTransfer( editEnd, editEndSnac, emptyBuffer );
	send( t5 );
}

//kate: indent-mode csands; space-indent off; replace-tabs off; tab-width 4;
