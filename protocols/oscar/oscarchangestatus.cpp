/***************************************************************************
                          oscarchangestatus.cpp  -  description
                             -------------------
    begin                : Wed Jul 31 2002
    copyright            : (C) 2002 by twl6
    email                : twl6@po.cwru.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kglobal.h>
#include <kconfig.h>
#include <qpushbutton.h>
#include <kcombobox.h>
#include <qlineedit.h>
#include "kopeteaway.h"
#include "oscarchangestatus.h"
#include "oscarchangestatus.moc"
#include "oscarsocket.h"

OscarChangeStatus::OscarChangeStatus(QWidget *parent, const char *name ) : OscarChangeStatusBase(parent,name)
{
	connect ( cmdCancel, SIGNAL(clicked()), this, SLOT(reject()) );
	connect ( cmdOkay, SIGNAL(clicked()), this, SLOT(accept()) );

	/* Set up the SingleShot away message */
	lneSingleShot->setText("");

	KGlobal::config()->setGroup("Oscar");
	away = KGlobal::config()->readEntry("AwayMessage", "I'm currently away from my computer. Please leave a message for me when I return to my computer.");
}

OscarChangeStatus::~OscarChangeStatus()
{
}

/** Gets a status message */
QString OscarChangeStatus::getStatusMessage(void)
{
	if(lneSingleShot->text() != ""){
		return lneSingleShot->text();
	} else {
		return KopeteAway::getInstance()->getMessage(cmbSavedMessages->currentText());
	}
/*OscarChangeStatus aimcs;
	if (request == OSCAR_AWAY)
	{
		aimcs.mleMessage->setText(aimcs.away);
	}
	aimcs.setWFlags(Qt::WType_Modal);
	if (aimcs.exec() == QDialog::Accepted)
	{
		if (request == OSCAR_AWAY)
		{
			KGlobal::config()->setGroup("Oscar");
			KGlobal::config()->writeEntry("AwayMessage", aimcs.mleMessage->text());
			KGlobal::config()->sync();
		}
		return aimcs.mleMessage->text();
	}
	else
	{
		return QString("QDialog::Rejected");
	}*/

}
