#ifndef LIBKOPETE_UI_AVATARSELECTORWIDGET.CPP
#define LIBKOPETE_UI_AVATARSELECTORWIDGET.CPP
/*
    avatarselectorwidget.cpp - Widget to manage and select user avatar

    Copyright (c) 2007      by Michaël Larouche      <larouche@kde.org>

    Kopete    (c) 2002-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/
#include "avatarselectorwidget.h"

// Qt includes
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QIcon>

// KDE includes
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kpixmapregionselectordialog.h>

#include "ui_avatarselectorwidget.h"

namespace Kopete
{
namespace UI
{

class AvatarSelectorWidgetItem : public QListWidgetItem
{
public:
	AvatarSelectorWidgetItem(QListWidget *parent)
	 : QListWidgetItem(parent, QListWidgetItem::UserType)
	{}

	void setAvatarEntry(Kopete::AvatarManager::AvatarEntry entry)
	{
		m_entry = entry;
		setText( entry.name );
		setIcon( QIcon(entry.path) );
	}

	Kopete::AvatarManager::AvatarEntry avatarEntry() const
	{
		return m_entry;
	}

private:
	Kopete::AvatarManager::AvatarEntry m_entry;
};

class AvatarSelectorWidget::Private
{
public:
	Private()
	 : selectedItem(0)
	{}

	Ui::AvatarSelectorWidget mainWidget;
	QListWidgetItem *selectedItem;
	QString currentAvatar;

	void addItem(Kopete::AvatarManager::AvatarEntry entry);
};

AvatarSelectorWidget::AvatarSelectorWidget(QWidget *parent)
 : QWidget(parent), d(new Private)
{
	d->mainWidget.setupUi(this);
	d->mainWidget.labelErrorMessage->setPixmap(KTitleWidget::ErrorMessage);
	d->mainWidget.labelErrorMessage->setHidden(true);

	// Connect signals/slots
	connect(d->mainWidget.buttonAddAvatar, SIGNAL(clicked()), this, SLOT(buttonAddAvatarClicked()));
	connect(d->mainWidget.buttonRemoveAvatar, SIGNAL(clicked()), this, SLOT(buttonRemoveAvatarClicked()));
	connect(d->mainWidget.tabAvatar, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(d->mainWidget.listUserAvatar, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(listSelectionChanged(QListWidgetItem*)));
	connect(d->mainWidget.listUserContact, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(listSelectionChanged(QListWidgetItem*)));

	connect(Kopete::AvatarManager::self(), SIGNAL(avatarAdded(Kopete::AvatarManager::AvatarEntry)), this, SLOT(avatarAdded(Kopete::AvatarManager::AvatarEntry)));
	connect(Kopete::AvatarManager::self(), SIGNAL(avatarRemoved(Kopete::AvatarManager::AvatarEntry)), this, SLOT(avatarRemoved(Kopete::AvatarManager::AvatarEntry)));

	// List avatars in lists
	Kopete::AvatarQueryJob *queryJob = new Kopete::AvatarQueryJob(this);
	connect(queryJob, SIGNAL(result(KJob*)), this, SLOT(queryJobFinished(KJob*)));
	queryJob->setQueryFilter( Kopete::AvatarManager::All );

	queryJob->start();

	// select the User tab by default
	d->mainWidget.tabAvatar->setCurrentWidget( d->mainWidget.tabUser );
}

AvatarSelectorWidget::~AvatarSelectorWidget()
{
	delete d;
}

Kopete::AvatarManager::AvatarEntry AvatarSelectorWidget::selectedEntry() const
{
	Kopete::AvatarManager::AvatarEntry result;

	if( d->selectedItem )
	{
		result = static_cast<AvatarSelectorWidgetItem*>(d->selectedItem)->avatarEntry();
	}

	return result;
}

void AvatarSelectorWidget::setCurrentAvatar(const QString &path)
{
	d->currentAvatar = path;

	//try to find the avatar in the list
	QList<QListWidgetItem*> itemList = d->mainWidget.listUserAvatar->findItems("", Qt::MatchContains);
	QList<QListWidgetItem*>::iterator it = itemList.begin();
	
	while (it != itemList.end())
	{
		AvatarSelectorWidgetItem *item = static_cast<AvatarSelectorWidgetItem*>(*it);
		if (item->avatarEntry().path == path)
		{
			item->setSelected(true);
			listSelectionChanged( item );
			return;
		}
		++it;
	}

}

void AvatarSelectorWidget::buttonAddAvatarClicked()
{
	KUrl imageUrl = KFileDialog::getImageOpenUrl( KUrl(), this );
	if( !imageUrl.isEmpty() )
	{
		// TODO: Download image
		if( !imageUrl.isLocalFile() )
		{
			d->mainWidget.labelErrorMessage->setText( i18n("You can only add avatar from local file.") );
			return;
		}

		// Crop the image
		QImage avatar = KPixmapRegionSelectorDialog::getSelectedImage( QPixmap(imageUrl.path()), 96, 96, this );

		QString imageName = imageUrl.fileName();

		Kopete::AvatarManager::AvatarEntry newEntry;
		// Remove extension from filename
		newEntry.name = imageName.left( imageName.lastIndexOf('.') );
		newEntry.image = avatar;
		newEntry.category = Kopete::AvatarManager::User;

		Kopete::AvatarManager::AvatarEntry addedEntry = Kopete::AvatarManager::self()->add( newEntry );
		if( addedEntry.path.isEmpty() )
		{
			d->mainWidget.labelErrorMessage->setText( i18n("Kopete can not add this new avatar because it could not save the avatar image in user directory.") );
			return;
		}

		// select the added entry and show the user tab
		QList<QListWidgetItem *> foundItems = d->mainWidget.listUserAvatar->findItems( addedEntry.name, Qt::MatchContains );
		if( !foundItems.isEmpty() )
		{
			AvatarSelectorWidgetItem *item = dynamic_cast<AvatarSelectorWidgetItem*>( foundItems.first() );
			if ( !item )
				return;

			item->setSelected( true );	
			// show the User tab
			d->mainWidget.tabAvatar->setCurrentWidget(d->mainWidget.tabUser);	
			listSelectionChanged( item );
		}


	}
}

void AvatarSelectorWidget::buttonRemoveAvatarClicked()
{
	// if no item was selected, just exit
	if ( !d->mainWidget.listUserAvatar->selectedItems().count() )
		return;

	// You can't remove from listUserContact, so we can always use listUserAvatar
	AvatarSelectorWidgetItem *selectedItem = dynamic_cast<AvatarSelectorWidgetItem*>( d->mainWidget.listUserAvatar->selectedItems().first() );
	if( selectedItem )
	{
		if( !Kopete::AvatarManager::self()->remove( selectedItem->avatarEntry() ) )
		{
			d->mainWidget.labelErrorMessage->setText( i18n("Kopete can not remove selected avatar.") );
			kDebug(14010) << k_funcinfo << "Removing of avatar failed for unknown reason." << endl;
		}
	}
}

void AvatarSelectorWidget::queryJobFinished(KJob *job)
{
	Kopete::AvatarQueryJob *queryJob = static_cast<Kopete::AvatarQueryJob*>(job);
	if( !queryJob->error() )
	{
		QList<Kopete::AvatarManager::AvatarEntry> avatarList = queryJob->avatarList();
		Kopete::AvatarManager::AvatarEntry entry;
		foreach(entry, avatarList)
		{
			d->addItem(entry);
		}
	}
	else
	{
		d->mainWidget.labelErrorMessage->setText( queryJob->errorText() );
	}
}

void AvatarSelectorWidget::avatarAdded(Kopete::AvatarManager::AvatarEntry newEntry)
{
	d->addItem(newEntry);
}

void AvatarSelectorWidget::avatarRemoved(Kopete::AvatarManager::AvatarEntry entryRemoved)
{
	// Same here, avatar can be only removed from listUserAvatar
	QList<QListWidgetItem *> foundItems = d->mainWidget.listUserAvatar->findItems( entryRemoved.name, Qt::MatchContains );
	if( !foundItems.isEmpty() )
	{
		kDebug(14010) << k_funcinfo << "Removing " << entryRemoved.name << " from list." << endl;

		int deletedRow = d->mainWidget.listUserAvatar->row( foundItems.first() );
		QListWidgetItem *removedItem = d->mainWidget.listUserAvatar->takeItem( deletedRow );
		delete removedItem;

		int newRow = --deletedRow;
		if( newRow < 0 )
			newRow = 0;

		// Select the previous avatar in the list, thus selecting a new avatar
		// and deselecting the avatar being removed.
		d->mainWidget.listUserAvatar->setCurrentRow( newRow );
		// Force update
		listSelectionChanged( d->mainWidget.listUserAvatar->item(newRow) );
	}
}

void AvatarSelectorWidget::listSelectionChanged(QListWidgetItem *item)
{
	if( item )
	{
		d->mainWidget.labelAvatarImage->setPixmap( item->icon().pixmap(96, 96) );
		d->selectedItem = item;
	}

	// I know sender() is evil
	// Disable Remove Avatar button when selecting an item in listUserContact.
	// I don't know anyone who will want to remove avatar received from contacts.
	if( sender() == d->mainWidget.listUserContact )
	{
		d->mainWidget.buttonRemoveAvatar->setEnabled(false);
	}
	else
	{
		d->mainWidget.buttonRemoveAvatar->setEnabled(true);
	}
}

void AvatarSelectorWidget::currentTabChanged(int index)
{
	
	// only enable the Remove button when the User tab is selected	
	if (index == d->mainWidget.tabAvatar->indexOf(d->mainWidget.tabUser)) 
		d->mainWidget.buttonRemoveAvatar->setEnabled(true);
	else
		d->mainWidget.buttonRemoveAvatar->setEnabled(false);
	
}


void AvatarSelectorWidget::Private::addItem(Kopete::AvatarManager::AvatarEntry entry)
{
	kDebug(14010) << k_funcinfo << "Entry(" << entry.name << "): " << entry.category << endl;

	QListWidget *listWidget  = 0;
	if( entry.category & Kopete::AvatarManager::User )
	{
		listWidget = mainWidget.listUserAvatar;
	}
	else if( entry.category & Kopete::AvatarManager::Contact )
	{
		listWidget = mainWidget.listUserContact;
	}
	else
	{
		return;
	}

	AvatarSelectorWidgetItem *item = new AvatarSelectorWidgetItem(listWidget);
	item->setAvatarEntry(entry);
	if (entry.path == currentAvatar)
		item->setSelected(true);
}

} // Namespace Kopete::UI

} // Namespace Kopete

#include "avatarselectorwidget.moc"
#endif // LIBKOPETE_UI/AVATARSELECTORWIDGET.CPP
