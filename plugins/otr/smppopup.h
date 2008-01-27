/***************************************************************************
 *   Copyright (C) 2007 by Michael Zanetti 
 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SMPPOPUP_H
#define SMPPOPUP_H

/**
  * @author Michael Zanetti
  */

extern "C"{
#include "libotr/proto.h"
}

#include "kopetechatsession.h"

#include "otrlchatinterface.h"
#include "ui_smppopup.h"

class SMPPopup: public KDialog
{
	Q_OBJECT
public:
	SMPPopup(QWidget *parent = 0, ConnContext *context = 0, Kopete::ChatSession *session = 0, bool initiate = true );
	~SMPPopup();

public slots:
  /*$PUBLIC_SLOTS$*/
	virtual void	cancelSMP();
	virtual void	respondSMP();
	virtual void	openHelp();
	virtual void	manualAuth();

private:
	Ui::SMPPopup ui;
	ConnContext *context;
	Kopete::ChatSession *session;
	bool initiate;

};

#endif //SMPPOPUP_H
