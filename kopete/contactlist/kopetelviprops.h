/*
    kopetelviprops.h

    Kopete Contactlist Properties GUI for Groups and MetaContacts

    Copyright (c) 2002-2003 by Stefan Gehn            <metz AT gehn.net>
    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETELVIPROPS_H
#define KOPETELVIPROPS_H

#include <kdialog.h>
#include <kabc/sound.h>

#include "kopetemetacontact.h"

#include "kopetegvipropswidget.h"
#include "kopetemetalvipropswidget.h"

class CustomNotificationProps;
class KPushButton;
class KopeteGroupViewItem;
class KopeteMetaContactLVI;
class KopeteAddressBookExport;
class KUrlRequester;

namespace Kopete { class Contact; }

class KopeteGVIProps: public KDialog
{
	Q_OBJECT

	public:
		KopeteGVIProps(KopeteGroupViewItem *gvi, QWidget *parent);
		~KopeteGVIProps();

	private:
		CustomNotificationProps * mNotificationProps;
		KopeteGVIPropsWidget *mainWidget;
		KopeteGroupViewItem *item;
		bool m_dirty;

	private slots:
		void slotOkClicked();
		void slotUseCustomIconsToggled(bool on);
		void slotIconChanged();
};


class KopeteMetaLVIProps: public KDialog
{
	Q_OBJECT

	public:
		KopeteMetaLVIProps(KopeteMetaContactLVI *gvi, QWidget *parent);
		~KopeteMetaLVIProps();

	private:
		CustomNotificationProps * mNotificationProps;
		QPushButton *mFromKABC;
		KopeteMetaLVIPropsWidget *mainWidget;
		KopeteMetaContactLVI *item;
		KopeteAddressBookExport *mExport;
		KABC::Sound mSound;
		int m_countPhotoCapable;
		QMap<int, Kopete::Contact *> m_withPhotoContacts;
		QString mAddressBookUid; // the currently selected addressbook UID
		
		Kopete::MetaContact::PropertySource selectedNameSource() const;
		Kopete::MetaContact::PropertySource selectedPhotoSource() const;
		Kopete::Contact* selectedNameSourceContact() const;
		Kopete::Contact* selectedPhotoSourceContact() const;
	private slots:
		void slotOkClicked();
		void slotUseCustomIconsToggled( bool on );
		void slotClearAddresseeClicked();
		void slotClearPhotoClicked();
		void slotSelectAddresseeClicked();
		void slotExportClicked();
		void slotImportClicked();
		void slotFromKABCClicked();
		void slotOpenSoundDialog( KUrlRequester *requester );
		void slotLoadNameSources();
		void slotLoadPhotoSources();
		void slotEnableAndDisableWidgets();
};

#endif
