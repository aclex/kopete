/*
	kopetemessagemanager.h - Creates chat sessions

	Copyright   : (c) 2002 by Duncan Mac-Vicar Prett
	Email       : duncan@kde.org

	*************************************************************************
	*                                                                       *
	* This program is free software; you can redistribute it and/or modify  *
	* it under the terms of the GNU General Public License as published by  *
	* the Free Software Foundation; either version 2 of the License, or     *
	* (at your option) any later version.                                   *
	*                                                                       *
	*************************************************************************
*/

#ifndef KOPETEMESSAGEMANAGERFACTORY_H
#define KOPETEMESSAGEMANAGERFACTORY_H

#include <qobject.h>
#include "kopetemessagemanager.h"

/**
  *@author Duncan Mac-Vicar Prett
  */
class KopeteMessageManager;
class KopeteMessage;
class KopeteContact;

typedef QPtrList<KopeteContact>        KopeteContactList;
typedef QPtrList<KopeteMessage>        KopeteMessageList;
typedef QPtrList<KopeteMessageManager> KopeteMessageManagerList;

class KopeteMessageManagerFactory : public QObject
{
public:
	
	KopeteMessageManagerFactory();
	~KopeteMessageManagerFactory();
	
	/**
	 * Create a new chat session. Provided is the initial list of contacts in
	 * the session. If a session with exactly these contacts already exists,
	 * it will be reused. Otherwise a new session is created.
	 */
	KopeteMessageManager* create( const KopeteContactList &contacts );
	
	/**
	 * Get a list of all open sessions
	 */
	static const KopeteMessageManagerList& sessions();

private:

	KopeteMessageManagerList mSessionList;
	
	
};

#endif
