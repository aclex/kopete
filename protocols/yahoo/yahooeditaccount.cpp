/*
    yahooeditaccount.cpp - UI Page to edit a Yahoo account

    Copyright (c) 2003 by Matt Rogers <mattrogers@sbcglobal.net>
    Copyright (c) 2002 by Gav Wood <gav@kde.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

// QT Includes
#include <qcheckbox.h>
#include <qlineedit.h>

// KDE Includes
#include <kdebug.h>
#include <kmessagebox.h>

// Kopete Includes
#include <addcontactpage.h>

// Local Includes
#include "yahooaccount.h"
#include "yahoocontact.h"
#include "yahooeditaccount.h"

// Yahoo Add Contact page
YahooEditAccount::YahooEditAccount(YahooProtocol *protocol, KopeteAccount *theAccount, QWidget *parent, const char *name): YahooEditAccountBase(parent), EditAccountWidget(theAccount)
{
	kdDebug(14180) << "YahooEditAccount::YahooEditAccount(<protocol>, <theAccount>, <parent>, " << name << ")";

	theProtocol = protocol;
	if(m_account)
	{	mScreenName->setText(m_account->accountId());
		mScreenName->setReadOnly(true) ; //the accountId is Constent
		if(m_account->rememberPassword())
			mPassword->setText(m_account->getPassword());
		mAutoConnect->setChecked(m_account->autoLogin());
		mRememberPassword->setChecked(true);
	}
	show();
}

bool YahooEditAccount::validateData()
{
	kdDebug(14180) << "YahooEditAccount::validateData()";

	if(mScreenName->text() == "")
	{	KMessageBox::sorry(this, i18n("<qt>You must enter a valid screen name</qt>"), i18n("Yahoo"));
		return false;
	}
	if(mPassword->text() == "")
	{	KMessageBox::sorry(this, i18n("<qt>You must enter a valid password</qt>"), i18n("Yahoo"));
		return false;
	}
	return true;
}

KopeteAccount *YahooEditAccount::apply()
{
	kdDebug(14180) << "YahooEditAccount::apply()";

	if(!m_account)
		m_account = new YahooAccount(theProtocol, mScreenName->text());
//	else
//		m_account->setAccountId(mScreenName->text());
	m_account->setAutoLogin(mAutoConnect->isChecked());
	if(mRememberPassword->isChecked())
		m_account->setPassword(mPassword->text());

	return m_account;
}

#include "yahooeditaccount.moc"
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

