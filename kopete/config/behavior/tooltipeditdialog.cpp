/*
    tooltipeditdialog.cpp  -  Kopete Tooltip Editor

    Copyright (c) 2004 by Stefan Gehn <metz AT gehn.net>
    Kopete    (c) 2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "tooltipeditdialog.h"
#include "tooltipeditwidget.h"

#include <kopeteprefs.h>

#include <qapplication.h>
#include <qtoolbutton.h>
#include <qheader.h>
#include <qstringlist.h>

#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>

class TooltipItem : public KListViewItem
{
	public:
		TooltipItem(KListView *parent, const QString& label, const QString& propertyName)
			: KListViewItem(parent, label),
				mPropName(propertyName)
		{
		}

		TooltipItem(KListView *parent, QListViewItem *item, const QString& label, const QString& propertyName)
			: KListViewItem(parent, item, label),
				mPropName(propertyName)
		{
		}

		QString propertyName() const { return mPropName; }
	private:
		QString mPropName;
};



TooltipEditDialog::TooltipEditDialog(QWidget *parent, const char* name)
	: KDialogBase(parent, name, true, i18n("Tooltip Editor"), Ok|Cancel, Ok, true)
{
	mMainWidget = new TooltipEditWidget(this, "TooltipEditDialog::mMainWidget");
	setMainWidget(mMainWidget);

	// TODO: Make this list global to Kopete, these are the known properties
	// plus some special ones (starting with a capitcal letter)
	QMap<QString, QString> propmap;
	propmap["FormattedName"]		= i18n("Full Name");
	propmap["FormattedIdleTime"]	= i18n("Idle Time");
	propmap["firstName"]				= i18n("First Name");
	propmap["lastName"]				= i18n("Last Name");
	propmap["emailAddress"]			= i18n("Email Address");
	propmap["privPhoneNum"]			= i18n("Private Phone");
	propmap["privFaxNum"]			= i18n("Private Fax");
	propmap["privMobileNum"]		= i18n("Private Mobile");
	propmap["awayMessage"]			= i18n("Away Message");
	propmap["ircChannel"]			= i18n("Channel");
	propmap["onlineSince"]			= i18n("Online Since");

	QStringList usedKeys = KopetePrefs::prefs()->toolTipContents();
	mMainWidget->lstUsedItems->header()->hide();
	mMainWidget->lstUnusedItems->header()->hide();
	mMainWidget->lstUsedItems->setSorting( -1 );
	mMainWidget->lstUnusedItems->setSorting( 0 );

	// first fill the "used" list
	QStringList::Iterator usedIt=usedKeys.end();
	do
	{
		usedIt--;
		if(propmap.contains(*usedIt)) // only add if that property key is really known
			new TooltipItem(mMainWidget->lstUsedItems, propmap[*usedIt], *usedIt);
	} while(usedIt != usedKeys.begin());

	// tthen iterate over all known properties and insert the remaining ones
	// into the "unused" list
	QMap<QString, QString>::Iterator it;
	for(it = propmap.begin(); it != propmap.end(); ++it)
	{
		if(usedKeys.contains(it.key())==0)
			new TooltipItem(mMainWidget->lstUnusedItems, it.data(), it.key());
	}

	connect(mMainWidget->lstUnusedItems, SIGNAL(selectionChanged(QListViewItem *)),
		this, SLOT(slotUnusedSelected(QListViewItem *)));
	connect(mMainWidget->lstUsedItems, SIGNAL(selectionChanged(QListViewItem *)),
		this, SLOT(slotUsedSelected(QListViewItem *)));

	QIconSet iconSet;
	iconSet = SmallIconSet("up");
	mMainWidget->tbUp->setIconSet(iconSet);
	mMainWidget->tbUp->setEnabled(false);
	mMainWidget->tbUp->setAutoRepeat(true);
	connect(mMainWidget->tbUp, SIGNAL(clicked()), SLOT(slotUpButton()));

	iconSet = SmallIconSet("down");
	mMainWidget->tbDown->setIconSet(iconSet);
	mMainWidget->tbDown->setEnabled(false);
	mMainWidget->tbDown->setAutoRepeat(true);
	connect(mMainWidget->tbDown, SIGNAL(clicked()), SLOT(slotDownButton()));

	iconSet = QApplication::reverseLayout() ? SmallIconSet("back") : SmallIconSet("forward");
	mMainWidget->tbAdd->setIconSet(iconSet);
	mMainWidget->tbAdd->setEnabled(false);
	connect(mMainWidget->tbAdd, SIGNAL(clicked()), SLOT(slotAddButton()));

	iconSet = QApplication::reverseLayout() ? SmallIconSet("forward") : SmallIconSet("back");
	mMainWidget->tbRemove->setIconSet(iconSet);
	mMainWidget->tbRemove->setEnabled(false);
	connect(mMainWidget->tbRemove, SIGNAL(clicked()), SLOT(slotRemoveButton()));

	connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));
}

void TooltipEditDialog::slotOkClicked()
{
	QStringList oldList = KopetePrefs::prefs()->toolTipContents();
	QStringList newList;
	QListViewItemIterator it(mMainWidget->lstUsedItems);
	QString keyname;

	while(it.current())
	{
		keyname = static_cast<TooltipItem *>(it.current())->propertyName();
		newList += keyname;
		kdDebug(14000) << k_funcinfo <<
			"Adding key '" << keyname << "' to tooltip list" << endl;
		++it;
	}

	if(oldList != newList)
	{
		KopetePrefs::prefs()->setToolTipContents(newList);
		emit changed(true);
		kdDebug(14000) << k_funcinfo << "tooltip fields changed, emitting changed()" << endl;
	}
}


void TooltipEditDialog::slotUnusedSelected(QListViewItem *item)
{
	//mMainWidget->tbRemove->setEnabled(false);
	mMainWidget->tbAdd->setEnabled(item!=0);
}

void TooltipEditDialog::slotUsedSelected(QListViewItem *item)
{
	mMainWidget->tbRemove->setEnabled(item!=0);
	//mMainWidget->tbAdd->setEnabled(false);
	if (item)
	{
		mMainWidget->tbUp->setEnabled(item->itemAbove() != 0);
		mMainWidget->tbDown->setEnabled(item->itemBelow() != 0);
	}
	else
	{
		mMainWidget->tbUp->setEnabled(false);
		mMainWidget->tbDown->setEnabled(false);
	}
}

void TooltipEditDialog::slotUpButton()
{
	QListViewItem *item = mMainWidget->lstUsedItems->currentItem();
	QListViewItem *prev = item->itemAbove();
	if(prev == 0) // we are first item already
		return;

	prev->moveItem(item);
	slotUsedSelected(item);
}

void TooltipEditDialog::slotDownButton()
{
	QListViewItem *item = mMainWidget->lstUsedItems->currentItem();
	QListViewItem *next = item->itemBelow();
	if(next == 0) // we are last item already
		return;

	item->moveItem(next);
	slotUsedSelected(item);
}

void TooltipEditDialog::slotAddButton()
{
	TooltipItem *item = static_cast<TooltipItem *>(mMainWidget->lstUnusedItems->currentItem());
	kdDebug(14000) << k_funcinfo << endl;

	// build a new one in the "used" list
	new TooltipItem(mMainWidget->lstUsedItems, item->text(0), item->propertyName());

	// remove the old one from "unused" list
	mMainWidget->lstUnusedItems->takeItem(item);
	delete item;
}

void TooltipEditDialog::slotRemoveButton()
{
	TooltipItem *item = static_cast<TooltipItem *>(mMainWidget->lstUsedItems->currentItem());
	kdDebug(14000) << k_funcinfo << endl;

	// build a new one in the "unused" list
	new TooltipItem(mMainWidget->lstUnusedItems, item->text(0), item->propertyName());

	// remove the old one from "used" list
	mMainWidget->lstUsedItems->takeItem(item);
	delete item;
}

#include "tooltipeditdialog.moc"
