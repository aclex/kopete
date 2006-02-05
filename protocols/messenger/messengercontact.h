/*
 * messengercontact.h - Windows Live Messenger Kopete Contact.
 *
 * Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>
 * 
 * Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>
 *
 *************************************************************************
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 *************************************************************************
 */
#ifndef MESSENGERCONTACT_H
#define MESSENGERCONTACT_H

#include <QMap>
#include <QList>

#include <kopetecontact.h>

class KAction;

namespace Kopete
{
	class ChatSession;
	class MetaContact;
}
class MessengerAccount;

class MessengerContact : public Kopete::Contact
{
	Q_OBJECT
public:
	MessengerContact(MessengerAccount *account, const QString &contactId, Kopete::MetaContact *parent);

	virtual bool isReachable();
    virtual void serialize(QMap< QString, QString >& serializedData, QMap< QString, QString >& addressBookData);
	
	virtual QList<KAction *> *customContextMenuActions();
	virtual Kopete::ChatSession *manager( CanCreateFlags canCreate = CannotCreate );
};
#endif
