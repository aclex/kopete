//
// C++ Implementation: %{MODULE}
//
// Description:
//
//
// Author: Roie Kerstein <sf_kersteinroie@bezeqint.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "addbookmarkspreferences.h"
#include "addbookmarksprefsui.h"
#include "addbookmarksplugin.h"
#include <kgenericfactory.h>
#include <kopetepluginmanager.h>
#include <kopetecontactlist.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <qlistbox.h>
#include <qnamespace.h>
#include <qradiobutton.h>


typedef KGenericFactory<BookmarksPreferences> BookmarksPreferencesFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kopete_addbookmarks, BookmarksPreferencesFactory("kcm_kopete_addbookmarks") )

BookmarksPreferences::BookmarksPreferences(QWidget *parent, const char *name, const QStringList &args)
 : KCModule(BookmarksPreferencesFactory::instance(), parent, args)
{
	( new QVBoxLayout (this) )->setAutoAdd( true );
	p_dialog = new BookmarksPrefsUI( this );
	load();
	connect( p_dialog->yesButton, SIGNAL( toggled(bool) ), this, SLOT( slotSetStatusChanged() ));
	connect( p_dialog->noButton, SIGNAL( toggled(bool) ), this, SLOT( slotSetStatusChanged() ));
	connect( p_dialog->onlySelectedButton, SIGNAL( toggled(bool) ), this, SLOT( slotSetStatusChanged() ));
	connect( p_dialog->onlyNotSelectedButton, SIGNAL( toggled(bool) ), this, SLOT( slotSetStatusChanged() ));
	connect( p_dialog->contactList, SIGNAL( selectionChanged() ), this, SLOT( slotSetStatusChanged() ));
	connect( this, SIGNAL(PreferencesChanged()), Kopete::PluginManager::self()->plugin("kopete_addbookmarks") , SLOT(slotReloadSettings()));
}


BookmarksPreferences::~BookmarksPreferences()
{
}

void BookmarksPreferences::save()
{
	QStringList list;
	QStringList::iterator it;

	
	m_settings.setFolderForEachContact( (BookmarksPrefsSettings::UseSubfolders)p_dialog->buttonGroup1->selectedId() );
	if(m_settings.isFolderForEachContact()==BookmarksPrefsSettings::OnlyContactsInList || m_settings.isFolderForEachContact()==BookmarksPrefsSettings::OnlyContactsNotInList ){
		for( uint i = 0; i < p_dialog->contactList->count() ; ++i ){
			if( p_dialog->contactList->isSelected( i ) ){
				list += p_dialog->contactList->text( i );
			}
		}
		m_settings.setContactsList( list );
	}
	m_settings.save();
	emit PreferencesChanged(); 
	emit KCModule::changed(false);
}

void BookmarksPreferences::slotSetStatusChanged()
{
	if ( p_dialog->buttonGroup1->selectedId() == 1  || p_dialog->buttonGroup1->selectedId() == 0)
		p_dialog->contactList->setEnabled(false);
	else
		p_dialog->contactList->setEnabled(true);
	
	emit KCModule::changed(true);
}

void BookmarksPreferences::load()
{
	QStringList list;
	QStringList::iterator it;
	QListBoxItem* item;
	
	m_settings.load();
	p_dialog->buttonGroup1->setButton(m_settings.isFolderForEachContact());
	if( p_dialog->contactList->count() == 0 ){
		p_dialog->contactList->insertStringList( Kopete::ContactList::self()->contacts() );
	}
	p_dialog->contactList->clearSelection();
	p_dialog->contactList->setEnabled(m_settings.isFolderForEachContact() != BookmarksPrefsSettings::OnlyContactsInList || m_settings.isFolderForEachContact()==BookmarksPrefsSettings::OnlyContactsNotInList );
	list = m_settings.getContactsList();
	for( it = list.begin() ; it != list.end() ; ++it){
		if( item = p_dialog->contactList->findItem(*it, Qt::ExactMatch ) ){
			p_dialog->contactList->setSelected( item, true );
		}
	}
	emit KCModule::changed(false);
}

#include "addbookmarkspreferences.moc"
