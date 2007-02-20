/*
    kopetechatwindow.cpp - Chat Window

    Copyright (c) 2002-2006 by Olivier Goffart       <ogoffart@kde.org>
    Copyright (c) 2003-2004 by Richard Smith         <kde@metafoo.co.uk>
    Copyright (C) 2002      by James Grant
    Copyright (c) 2002      by Stefan Gehn           <metz@gehn.net>
    Copyright (c) 2002-2004 by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qtimer.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qfileinfo.h>
#include <QDockWidget>

//Added by qt3to4:
#include <QPixmap>
#include <QTextStream>
#include <QCloseEvent>
#include <Q3PtrList>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QMenu>

#include <kactioncollection.h>
#include <kapplication.h>
#include <kcursor.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kconfig.h>
#include <kmenu.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kwin.h>
#include <ktemporaryfile.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kstatusbar.h>
#include <kpushbutton.h>
#include <ktabwidget.h>
#include <kstandarddirs.h>
#include <kdialog.h>
#include <kstringhandler.h>
#include <ksqueezedtextlabel.h>
#include <kstandardshortcut.h>
#include <kglobalsettings.h>
#include <khbox.h>
#include <kvbox.h>
#include <ktoolbar.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
// #include <k3widgetaction.h>

#include "chatmessagepart.h"
#include "chattexteditpart.h"
#include "chatview.h"
#include "kopeteapplication.h"
#include "kopetebehaviorsettings.h"
#include "kopetechatwindow.h"
#include "kopeteemoticonaction.h"
#include "kopetegroup.h"
#include "kopetechatsession.h"
#include "kopetemetacontact.h"
#include "kopetepluginmanager.h"
#include "kopeteprotocol.h"
#include "kopetestdaction.h"
#include "kopeteviewmanager.h"
#include "sidebarwidget.h"

#include <qtoolbutton.h>
#include <kxmlguifactory.h>

typedef QMap<Kopete::Account*,KopeteChatWindow*> AccountMap;
typedef QMap<Kopete::Group*,KopeteChatWindow*> GroupMap;
typedef QMap<Kopete::MetaContact*,KopeteChatWindow*> MetaContactMap;
typedef Q3PtrList<KopeteChatWindow> WindowList;

namespace
{
	AccountMap accountMap;
	GroupMap groupMap;
	MetaContactMap mcMap;
	WindowList windows;
}

KopeteChatWindow *KopeteChatWindow::window( Kopete::ChatSession *manager )
{
	bool windowCreated = false;
	KopeteChatWindow *myWindow;

	//Take the first and the first? What else?
	Kopete::Group *group = 0L;
	Kopete::ContactPtrList members = manager->members();
	Kopete::MetaContact *metaContact = members.first()->metaContact();

	if ( metaContact )
	{
		Kopete::GroupList gList = metaContact->groups();
		group = gList.first();
	}

	switch( Kopete::BehaviorSettings::self()->chatWindowGroupPolicy() )
	{
		//Open chats from the same protocol in the same window
		case Kopete::BehaviorSettings::EnumChatWindowGroupPolicy::GroupByAccount:
			if( accountMap.contains( manager->account() ) )
				myWindow = accountMap[ manager->account() ];
			else
				windowCreated = true;
			break;

		//Open chats from the same group in the same window
		case Kopete::BehaviorSettings::EnumChatWindowGroupPolicy::GroupByGroup:
			if( group && groupMap.contains( group ) )
				myWindow = groupMap[ group ];
			else
				windowCreated = true;
			break;

		//Open chats from the same metacontact in the same window
		case Kopete::BehaviorSettings::EnumChatWindowGroupPolicy::GroupByMetaContact:
			if( mcMap.contains( metaContact ) )
				myWindow = mcMap[ metaContact ];
			else
				windowCreated = true;
			break;

		//Open all chats in the same window
		case Kopete::BehaviorSettings::EnumChatWindowGroupPolicy::GroupAll:
			if( windows.isEmpty() )
				windowCreated = true;
			else
			{
				//Here we are finding the window with the most tabs and
				//putting it there. Need this for the cases where config changes
				//midstream

				int viewCount = -1;
				for ( KopeteChatWindow *thisWindow = windows.first(); thisWindow; thisWindow = windows.next() )
				{
					if( thisWindow->chatViewCount() > viewCount )
					{
						myWindow = thisWindow;
						viewCount = thisWindow->chatViewCount();
					}
				}
			}
			break;

		//Open every chat in a new window
		case Kopete::BehaviorSettings::EnumChatWindowGroupPolicy::OpenNewWindow:
		default:
			windowCreated = true;
			break;
	}

	if ( windowCreated )
	{
		myWindow = new KopeteChatWindow();

		if ( !accountMap.contains( manager->account() ) )
			accountMap.insert( manager->account(), myWindow );

		if ( !mcMap.contains( metaContact ) )
			mcMap.insert( metaContact, myWindow );

		if ( group && !groupMap.contains( group ) )
			groupMap.insert( group, myWindow );
	}

//	kDebug( 14010 ) << k_funcinfo << "Open Windows: " << windows.count() << endl;

	return myWindow;
}

KopeteChatWindow::KopeteChatWindow( QWidget *parent )
	: KMainWindow( parent )
{
	m_activeView = 0L;
	m_popupView = 0L;
	backgroundFile = 0L;
	updateBg = true;
	m_tabBar = 0L;

	initActions();

	m_sideBar = new SidebarWidget(this);
	m_sideBar->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
	m_sideBar->setObjectName("SideBar"); //object name is required for automatic position and settings save.

	addDockWidget(Qt::RightDockWidgetArea, m_sideBar);

	KVBox *vBox = new KVBox( this );
	vBox->setLineWidth( 0 );
	vBox->setSpacing( 0 );
	vBox->setFrameStyle( QFrame::NoFrame );
	// set default window size.  This could be removed by fixing the size hints of the contents
	resize( 500, 500 );
	setCentralWidget( vBox );

	mainArea = new QFrame( vBox );
	mainArea->setLineWidth( 0 );
	mainArea->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	mainLayout = new QVBoxLayout( mainArea );

	if ( Kopete::BehaviorSettings::self()->chatWindowShowSendButton() )
	{
		//Send Button
		m_button_send = new KPushButton( i18n("Send"), statusBar() );
		m_button_send->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
		m_button_send->setEnabled( false );
		m_button_send->setFont( statusBar()->font() );
		m_button_send->setFixedHeight( statusBar()->sizeHint().height() );
		connect( m_button_send, SIGNAL( clicked() ), this, SLOT( slotSendMessage() ) );
		statusBar()->addPermanentWidget( m_button_send, 0 );
	}
	else
		m_button_send = 0L;

	m_status_text = new KSqueezedTextLabel( i18n("Ready."), statusBar() );
	m_status_text->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	m_status_text->setFont( statusBar()->font() );
	m_status_text->setFixedHeight( statusBar()->sizeHint().height() );
	statusBar()->addWidget( m_status_text, 1 );

	windows.append( this );
	windowListChanged();

	m_alwaysShowTabs = KGlobal::config()->group( "ChatWindowSettings" ).
                           readEntry( QLatin1String("AlwaysShowTabs"), false );
//	kDebug( 14010 ) << k_funcinfo << "Open Windows: " << windows.count() << endl;

	setupGUI( static_cast<StandardWindowOptions>(ToolBar | Keys | StatusBar | Save | Create) , "kopetechatwindow.rc" );

	//has to be done after the setupGUI, in order to have the toolbar set up to restore window settings.
	readOptions();
}

KopeteChatWindow::~KopeteChatWindow()
{
	kDebug( 14010 ) << k_funcinfo << endl;

	emit( closing( this ) );

	for( AccountMap::Iterator it = accountMap.begin(); it != accountMap.end(); )
	{
		AccountMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.value() == this )
			accountMap.remove( mayDeleteIt.key() );
	}

	for( GroupMap::Iterator it = groupMap.begin(); it != groupMap.end(); )
	{
		GroupMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.value() == this )
			groupMap.remove( mayDeleteIt.key() );
	}

	for( MetaContactMap::Iterator it = mcMap.begin(); it != mcMap.end(); )
	{
		MetaContactMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.value() == this )
			mcMap.remove( mayDeleteIt.key() );
	}

	windows.remove( this );
	windowListChanged();

//	kDebug( 14010 ) << "Open Windows: " << windows.count() << endl;

	saveOptions();

	if( backgroundFile )
	{
		delete backgroundFile;
	}

	delete anim;
}

void KopeteChatWindow::windowListChanged()
{
	// update all windows' Move Tab to Window action
	for ( Q3PtrListIterator<KopeteChatWindow> it( windows ); *it; ++it )
		(*it)->checkDetachEnable();
}

void KopeteChatWindow::slotNickComplete()
{
	if( m_activeView )
		m_activeView->nickComplete();
}

void KopeteChatWindow::slotTabContextMenu( QWidget *tab, const QPoint &pos )
{
	m_popupView = static_cast<ChatView*>( tab );

	KMenu *popup = new KMenu;
	popup->addTitle( KStringHandler::rsqueeze( m_popupView->caption() ) );
	popup->addAction( actionContactMenu );
	popup->addSeparator();
	popup->addAction( actionTabPlacementMenu );
	popup->addAction( tabDetach );
	popup->addAction( actionDetachMenu );
	popup->addAction( tabClose );
	popup->exec( pos );

	delete popup;
	m_popupView = 0;
}

ChatView *KopeteChatWindow::activeView()
{
	return m_activeView;
}

void KopeteChatWindow::initActions(void)
{
	KActionCollection *coll = actionCollection();

	createStandardStatusBarAction();

	chatSend = new KAction( KIcon("mail_send"), i18n( "&Send Message" ), coll );
        coll->addAction( "chat_send", chatSend );
	connect( chatSend, SIGNAL( triggered(bool) ), SLOT( slotSendMessage() ) );
	//Default to 'Return' for sending messages
	chatSend->setShortcut( QKeySequence(Qt::Key_Return) );
	chatSend->setEnabled( false );

 	KStandardAction::save ( this, SLOT(slotChatSave()), coll );
 	KStandardAction::print ( this, SLOT(slotChatPrint()), coll );
	KAction* quitAction = KStandardAction::quit ( this, SLOT(close()), coll );
	quitAction->setText( i18n("Close All Chats") );

	tabClose = KStandardAction::close ( this, SLOT(slotChatClosed()), coll );
        coll->addAction( "tabs_close", tabClose );

	tabRight=new KAction( i18n( "&Activate Next Tab" ), coll );
        coll->addAction( "tabs_right", tabRight );
	tabRight->setShortcut( KStandardShortcut::tabNext() );
	tabRight->setEnabled( false );
	connect( tabRight, SIGNAL(triggered(bool)), this, SLOT(slotNextTab()) );

	tabLeft=new KAction( i18n( "&Activate Previous Tab" ), coll );
        coll->addAction( "tabs_left", tabLeft );
	tabLeft->setShortcut( KStandardShortcut::tabPrev() );
	tabLeft->setEnabled( false );
	connect( tabLeft, SIGNAL(triggered(bool)), this, SLOT(slotPreviousTab()) );

	nickComplete = new KAction( i18n( "Nic&k Completion" ), coll );
        coll->addAction( "nick_complete", nickComplete );
	connect( nickComplete, SIGNAL(triggered(bool)), this, SLOT(slotNickComplete()));
	nickComplete->setShortcut( QKeySequence( Qt::Key_Tab ) );

	tabDetach = new KAction( KIcon("tab_breakoff"), i18n( "&Detach Chat" ), coll );
        coll->addAction( "tabs_detach", tabDetach );
	tabDetach->setEnabled( false );
	connect( tabDetach, SIGNAL(triggered(bool)), this, SLOT( slotDetachChat() ));

	actionDetachMenu = new KActionMenu( KIcon("tab_breakoff"), i18n( "&Move Tab to Window" ), coll );
        coll->addAction( "tabs_detachmove", actionDetachMenu );
	actionDetachMenu->setDelayed( false );

	connect ( actionDetachMenu->menu(), SIGNAL(aboutToShow()), this, SLOT(slotPrepareDetachMenu()) );
	connect ( actionDetachMenu->menu(), SIGNAL(activated(int)), this, SLOT(slotDetachChat(int)) );

	actionTabPlacementMenu = new KActionMenu( i18n( "&Tab Placement" ), coll );
        coll->addAction( "tabs_placement", actionTabPlacementMenu );
	connect ( actionTabPlacementMenu->menu(), SIGNAL(aboutToShow()), this, SLOT(slotPreparePlacementMenu()) );
	connect ( actionTabPlacementMenu->menu(), SIGNAL(activated(int)), this, SLOT(slotPlaceTabs(int)) );

	tabDetach->setShortcut( QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B) );

	KStandardAction::cut( this, SLOT(slotCut()), coll);
	KStandardAction::copy( this, SLOT(slotCopy()), coll);
	KStandardAction::paste( this, SLOT(slotPaste()), coll);

	KAction* action;
	action = new KAction( KIcon("charset"), i18n( "Set Default &Font..." ), coll );
        coll->addAction( "format_font", action );
	connect( action, SIGNAL(triggered(bool)), this, SLOT(slotSetFont()) );

	action = new KAction( KIcon("pencil"), i18n( "Set Default Text &Color..." ), coll );
        coll->addAction( "format_fgcolor", action );
	connect( action, SIGNAL(triggered(bool)), this, SLOT(slotSetFgColor()) );

	action = new KAction( KIcon("fill"), i18n( "Set &Background Color..." ), coll );
        coll->addAction( "format_bgcolor", action );
	connect( action, SIGNAL(triggered()), this, SLOT(slotSetBgColor()) );

	historyUp = new KAction( i18n( "Previous History" ), coll );
        coll->addAction( "history_up", historyUp );
	historyUp->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_Up) );
	connect( historyUp, SIGNAL(triggered(bool)), this, SLOT(slotHistoryUp()) );

	historyDown = new KAction( i18n( "Next History" ), coll );
        coll->addAction( "history_down", historyDown );
	historyDown->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_Down) );
	connect( historyDown, SIGNAL(triggered(bool)), this, SLOT(slotHistoryDown()) );

	action = KStandardAction::prior( this, SLOT( slotPageUp() ), coll );
        coll->addAction( "scroll_up", action );
	action = KStandardAction::next( this, SLOT( slotPageDown() ), coll );
        coll->addAction( "scroll_down", action );

	KStandardAction::showMenubar( this, SLOT(slotViewMenuBar()), coll );

	membersLeft = new KToggleAction( i18n( "Place to Left of Chat Area" ), coll );
        coll->addAction( "options_membersleft", membersLeft );
	connect( membersLeft, SLOT(toggled(bool)), this, SLOT(slotViewMembersLeft()) );

	membersRight = new KToggleAction( i18n( "Place to Right of Chat Area" ), coll );
        coll->addAction( "options_membersright", membersRight );
	connect( membersRight, SLOT(toggled(bool)), this, SLOT(slotViewMembersRight()) );

	toggleMembers = new KToggleAction( i18n( "Show" ), coll );
        coll->addAction( "options_togglemembers", toggleMembers );
	toggleMembers->setCheckedState( KGuiItem( i18n("Hide") ) );
	connect( toggleMembers, SLOT(toggled(bool)), this, SLOT(slotToggleViewMembers()) );

	toggleAutoSpellCheck = new KToggleAction( i18n( "Automatic Spell Checking" ), coll );
        coll->addAction( "enable_auto_spell_check", toggleAutoSpellCheck );
	toggleAutoSpellCheck->setChecked( true );
	connect( toggleAutoSpellCheck, SIGNAL(triggered(bool)), this, SLOT(toggleAutoSpellChecking()) );

	actionSmileyMenu = new KopeteEmoticonAction( coll );
        coll->addAction( "format_smiley", actionSmileyMenu );
	actionSmileyMenu->setDelayed( false );
	connect(actionSmileyMenu, SIGNAL(activated(const QString &)), this, SLOT(slotSmileyActivated(const QString &)));

	actionContactMenu = new KActionMenu(i18n("Co&ntacts"), coll );
        coll->addAction( "contacts_menu", actionContactMenu );
	actionContactMenu->setDelayed( false );
	connect ( actionContactMenu->menu(), SIGNAL(aboutToShow()), this, SLOT(slotPrepareContactMenu()) );

	KopeteStdAction::preferences( coll , "settings_prefs" );

	//The Sending movie
	normalIcon = QPixmap( BarIcon( QLatin1String( "kopete" ) ) );
#if 0
	animIcon = KGlobal::iconLoader()->loadMovie( QLatin1String( "newmessage" ), K3Icon::Toolbar);

	// Pause the animation because otherwise it's running even when we're not
	// showing it. This eats resources, and also triggers a pixmap leak in
	// QMovie in at least Qt 3.1, Qt 3.2 and the current Qt 3.3 beta
	if( !animIcon.isNull() )  //and another QT bug:  it crash if we pause a null movie
		animIcon.pause();
#endif
	// we can't set the tool bar as parent, if we do, it will be deleted when we configure toolbars
	anim = new QLabel( QString::null, 0L );
	anim->setObjectName( QLatin1String("kde toolbar widget") );
	anim->setMargin(5);
	anim->setPixmap( normalIcon );

	KAction *animAction = new KAction( i18n("Toolbar Animation"), coll );
        coll->addAction( "toolbar_animation", animAction );
	animAction->setDefaultWidget( anim );

	//toolBar()->insertWidget( 99, anim->width(), anim );
	//toolBar()->alignItemRight( 99 );
}

const QString KopeteChatWindow::fileContents( const QString &path ) const
{
 	QString contents;
	QFile file( path );
	if ( file.open( QIODevice::ReadOnly ) )
	{
		QTextStream stream( &file );
		contents = stream.readAll();
		file.close();
	}

	return contents;
}

void KopeteChatWindow::slotStopAnimation( ChatView* view )
{
	if( view == m_activeView )
		anim->setPixmap( normalIcon );
}

void KopeteChatWindow::slotUpdateSendEnabled()
{
	if ( !m_activeView ) return;

	bool enabled = m_activeView->canSend();
	chatSend->setEnabled( enabled );
	if(m_button_send)
		m_button_send->setEnabled( enabled );
}

void KopeteChatWindow::toggleAutoSpellChecking()
{
	if ( !m_activeView )
		return;

	bool currentSetting = m_activeView->editPart()->autoSpellCheckEnabled();
	m_activeView->editPart()->toggleAutoSpellCheck( !currentSetting );
	updateSpellCheckAction();
}

void KopeteChatWindow::updateSpellCheckAction()
{
	if ( !m_activeView )
		return;

	if ( m_activeView->editPart()->isRichTextEnabled() )
	{
		toggleAutoSpellCheck->setEnabled( false );
		toggleAutoSpellCheck->setChecked( false );
		m_activeView->editPart()->toggleAutoSpellCheck( false );
	}
	else
	{
		toggleAutoSpellCheck->setEnabled( true );
		if ( Kopete::BehaviorSettings::self()->spellCheck() )
		{
			kDebug(14000) << k_funcinfo << "spell check enabled" << endl;
			toggleAutoSpellCheck->setChecked( true );
			m_activeView->editPart()->toggleAutoSpellCheck(true);
		}
		else
		{
			kDebug(14000) << k_funcinfo << "spell check disabled" << endl;
			toggleAutoSpellCheck->setChecked( false );
			m_activeView->editPart()->toggleAutoSpellCheck(false);
		}
	}
}

void KopeteChatWindow::slotHistoryUp()
{
	if( m_activeView )
		m_activeView->editPart()->historyUp();
}

void KopeteChatWindow::slotHistoryDown()
{
	if( m_activeView )
		m_activeView->editPart()->historyDown();
}

void KopeteChatWindow::slotPageUp()
{
	if( m_activeView )
		m_activeView->messagePart()->pageUp();
}

void KopeteChatWindow::slotPageDown()
{
	if( m_activeView )
		m_activeView->messagePart()->pageDown();
}

void KopeteChatWindow::slotCut()
{
	m_activeView->cut();
}

void KopeteChatWindow::slotCopy()
{
	m_activeView->copy();
}

void KopeteChatWindow::slotPaste()
{
	m_activeView->paste();
}


void KopeteChatWindow::slotSetFont()
{
	m_activeView->setFont();
}

void KopeteChatWindow::slotSetFgColor()
{
	m_activeView->setFgColor();
}

void KopeteChatWindow::slotSetBgColor()
{
	m_activeView->setBgColor();
}

void KopeteChatWindow::setStatus(const QString &text)
{
	m_status_text->setText(text);
}

void KopeteChatWindow::createTabBar()
{
	if( !m_tabBar )
	{
		KConfigGroup cg( KGlobal::config(), QLatin1String("ChatWindowSettings") );

		m_tabBar = new KTabWidget( mainArea );
		m_tabBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
		m_tabBar->setHoverCloseButton(cg.readEntry( QLatin1String("HoverClose"), false ));
		m_tabBar->setTabReorderingEnabled(true);
		m_tabBar->setAutomaticResizeTabs(true);
		connect( m_tabBar, SIGNAL( closeRequest( QWidget* )), this, SLOT( slotCloseChat( QWidget* ) ) );

		QToolButton* m_rightWidget = new QToolButton( m_tabBar );
		connect( m_rightWidget, SIGNAL( clicked() ), this, SLOT( slotChatClosed() ) );
		m_rightWidget->setIcon( SmallIcon( "tab_remove" ) );
		m_rightWidget->adjustSize();
		m_rightWidget->setToolTip( i18n("Close the current tab") );
		m_tabBar->setCornerWidget( m_rightWidget, Qt::TopRightCorner );

		mainLayout->addWidget( m_tabBar );
		m_tabBar->show();
		connect ( m_tabBar, SIGNAL(currentChanged(QWidget *)), this, SLOT(setActiveView(QWidget *)) );
		connect ( m_tabBar, SIGNAL(contextMenu(QWidget *, const QPoint & )), this, SLOT(slotTabContextMenu( QWidget *, const QPoint & )) );

		for( ChatView *view = chatViewList.first(); view; view = chatViewList.next() )
			addTab( view );

		if( m_activeView )
			m_tabBar->setCurrentWidget( m_activeView );
		else
			setActiveView( chatViewList.first() );

		int tabPosition = cg.readEntry( QLatin1String("Tab Placement") , 0 );
		slotPlaceTabs( tabPosition );
	}
}

void KopeteChatWindow::slotCloseChat( QWidget *chatView )
{
	static_cast<ChatView*>( chatView )->closeView();
}

void KopeteChatWindow::addTab( ChatView *view )
{
	QList<Kopete::Contact*> chatMembers=view->msgManager()->members();
	Kopete::Contact *c=0L;
	foreach( Kopete::Contact *contact , chatMembers )
	{
		if(!c || c->onlineStatus() < contact->onlineStatus())
			c=contact;
	}
	QPixmap pluginIcon = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c) : SmallIcon( view->msgManager()->protocol()->pluginIcon() );

	view->setParent( m_tabBar );
    view->setWindowFlags( 0 );
    view->move( QPoint() );
    view->show();

	m_tabBar->addTab( view, pluginIcon, view->caption() );
	if( view == m_activeView )
		view->show();
	else
		view->hide();
	connect( view, SIGNAL( captionChanged( bool ) ), this, SLOT( updateChatLabel() ) );
	connect( view, SIGNAL( updateStatusIcon( ChatView* ) ), this, SLOT( slotUpdateCaptionIcons( ChatView* ) ) );
	view->setCaption( view->caption(), false );
}

void KopeteChatWindow::setPrimaryChatView( ChatView *view )
{
	//TODO figure out what else we have to save here besides the font
	//reparent clears a lot of stuff out
	QFont savedFont = view->font();
	view->setParent( mainArea );
    view->setWindowFlags( 0 );
    view->move( QPoint() );
	view->setFont( savedFont );
	view->show();

	mainLayout->addWidget( view );
	setActiveView( view );
}

void KopeteChatWindow::deleteTabBar()
{
	if( m_tabBar )
	{
		disconnect ( m_tabBar, SIGNAL(currentChanged(QWidget *)), this, SLOT(setActiveView(QWidget *)) );
		disconnect ( m_tabBar, SIGNAL(contextMenu(QWidget *, const QPoint & )), this, SLOT(slotTabContextMenu( QWidget *, const QPoint & )) );

		if( !chatViewList.isEmpty() )
			setPrimaryChatView( chatViewList.first() );

		m_tabBar->deleteLater();
		m_tabBar = 0L;
	}
}

void KopeteChatWindow::attachChatView( ChatView* newView )
{
	chatViewList.append( newView );

	if ( !m_alwaysShowTabs && chatViewList.count() == 1 )
		setPrimaryChatView( newView );
	else
	{
		if ( !m_tabBar )
			createTabBar();
		else
			addTab( newView );
		newView->setActive( false );
	}

	newView->setMainWindow( this );
	newView->editWidget()->installEventFilter( this );

	KCursor::setAutoHideCursor( newView->editWidget(), true, true );
	connect( newView, SIGNAL(captionChanged( bool)), this, SLOT(slotSetCaption(bool)) );
	connect( newView, SIGNAL(messageSuccess( ChatView* )), this, SLOT(slotStopAnimation( ChatView* )) );
	connect( newView, SIGNAL(rtfEnabled( ChatView*, bool ) ), this, SLOT( slotRTFEnabled( ChatView*, bool ) ) );
	connect( newView, SIGNAL(updateStatusIcon( ChatView* ) ), this, SLOT(slotUpdateCaptionIcons( ChatView* ) ) );
	connect( newView, SIGNAL(updateChatState( ChatView*, int ) ), this, SLOT( updateChatState( ChatView*, int ) ) );

	updateSpellCheckAction();
	checkDetachEnable();
	newView->loadChatSettings();
	connect( newView, SIGNAL(autoSpellCheckEnabled( ChatView*, bool ) ),
	         this, SLOT( slotAutoSpellCheckEnabled( ChatView*, bool ) ) );
}

void KopeteChatWindow::checkDetachEnable()
{
	bool haveTabs = (chatViewList.count() > 1);
	tabDetach->setEnabled( haveTabs );
	tabLeft->setEnabled( haveTabs );
	tabRight->setEnabled( haveTabs );
	actionTabPlacementMenu->setEnabled( m_tabBar != 0 );

	bool otherWindows = (windows.count() > 1);
	actionDetachMenu->setEnabled( otherWindows );
}

void KopeteChatWindow::detachChatView( ChatView *view )
{
	if( !chatViewList.removeRef( view ) )
		return;

	disconnect( view, SIGNAL(captionChanged( bool)), this, SLOT(slotSetCaption(bool)) );
	disconnect( view, SIGNAL( updateStatusIcon( ChatView* ) ), this, SLOT( slotUpdateCaptionIcons( ChatView* ) ) );
	disconnect( view, SIGNAL( updateChatState( ChatView*, int ) ), this, SLOT( updateChatState( ChatView*, int ) ) );
	view->editWidget()->removeEventFilter( this );

	if( m_tabBar )
	{
		int curPage = m_tabBar->currentIndex();
		QWidget *page = m_tabBar->currentWidget();

		// if the current view is to be detached, switch to a different one
		if( page == view )
		{
			if( curPage > 0 )
				m_tabBar->setCurrentIndex( curPage - 1 );
			else
				m_tabBar->setCurrentIndex( curPage + 1 );
		}

		m_tabBar->removePage( view );

		if( m_tabBar->currentWidget() )
			setActiveView( static_cast<ChatView*>(m_tabBar->currentWidget()) );
	}

	if( chatViewList.isEmpty() )
		close();
	else if( !m_alwaysShowTabs && chatViewList.count() == 1)
		deleteTabBar();

	checkDetachEnable();
}

void KopeteChatWindow::slotDetachChat( int newWindowIndex )
{
	KopeteChatWindow *newWindow = 0L;
	ChatView *detachedView;

	if( m_popupView )
		detachedView = m_popupView;
	else
		detachedView = m_activeView;

	if( !detachedView )
		return;

	//if we don't do this, we might crash
// 	createGUI(0L);
	guiFactory()->removeClient(detachedView->msgManager());

	if( newWindowIndex == -1 )
	{
		newWindow = new KopeteChatWindow();
		newWindow->setObjectName( QLatin1String("KopeteChatWindow") );
	}
	else
		newWindow = windows.at( newWindowIndex );

	newWindow->show();
	newWindow->raise();

	detachChatView( detachedView );
	newWindow->attachChatView( detachedView );
}

void KopeteChatWindow::slotPreviousTab()
{
	int curPage = m_tabBar->currentIndex();
	if( curPage > 0 )
		m_tabBar->setCurrentIndex( curPage - 1 );
	else
		m_tabBar->setCurrentIndex( m_tabBar->count() - 1 );
}

void KopeteChatWindow::slotNextTab()
{
	int curPage = m_tabBar->currentIndex();
	if( curPage == ( m_tabBar->count() - 1 ) )
		m_tabBar->setCurrentIndex( 0 );
	else
		m_tabBar->setCurrentIndex( curPage + 1 );
}

void KopeteChatWindow::slotSetCaption( bool active )
{
	if( active && m_activeView )
	{
		setCaption( m_activeView->caption(), false );
	}
}

void KopeteChatWindow::updateBackground( const QPixmap &pm )
{
	if( updateBg )
	{
		updateBg = false;
		if( backgroundFile != 0L )
		{
			delete backgroundFile;
		}

		backgroundFile = new KTemporaryFile();
		backgroundFile->setSuffix(".bmp");
		backgroundFile->open();
		pm.save( backgroundFile, "BMP" );
		QTimer::singleShot( 100, this, SLOT( slotEnableUpdateBg() ) );
	}
}

void KopeteChatWindow::setActiveView( QWidget *widget )
{
	ChatView *view = static_cast<ChatView*>(widget);

	if( m_activeView == view )
		return;

	if(m_activeView)
	{
		disconnect( m_activeView, SIGNAL( canSendChanged(bool) ), this, SLOT( slotUpdateSendEnabled() ) );
		guiFactory()->removeClient(m_activeView->msgManager());
		m_activeView->saveChatSettings();
	}

	guiFactory()->addClient(view->msgManager());
// 	createGUI( view->editPart() );

	if( m_activeView )
		m_activeView->setActive( false );

	m_activeView = view;

	if( !chatViewList.contains( view ) )
		attachChatView( view );

	connect( m_activeView, SIGNAL( canSendChanged(bool) ), this, SLOT( slotUpdateSendEnabled() ) );

	//Tell it it is active
	m_activeView->setActive( true );

	//Update icons to match
	slotUpdateCaptionIcons( m_activeView );

#if 0
	if ( m_activeView->sendInProgress() && !animIcon.isNull() )
	{
		anim->setMovie( &animIcon );
		animIcon.unpause();
	}
	else
	{
		anim->setPixmap( normalIcon );
		if( !animIcon.isNull() )
			animIcon.pause();
	}
#endif
	if ( m_alwaysShowTabs || chatViewList.count() > 1 )
	{
		if( !m_tabBar )
			createTabBar();

		m_tabBar->setCurrentWidget( m_activeView );
	}

	setCaption( m_activeView->caption() );
	setStatus( m_activeView->statusText() );
	m_activeView->setFocus();
	updateSpellCheckAction();
	slotUpdateSendEnabled();
	m_activeView->editPart()->reloadConfig();
	m_activeView->loadChatSettings();

	emit chatSessionChanged(m_activeView->msgManager());
}

void KopeteChatWindow::slotUpdateCaptionIcons( ChatView *view )
{
	if ( !view )
		return; //(pas de charité)

	QList<Kopete::Contact*> chatMembers=view->msgManager()->members();
	Kopete::Contact *c=0L;
	foreach ( Kopete::Contact *contact , chatMembers )
	{
		if(!c || c->onlineStatus() < contact->onlineStatus())
			c=contact;
	}

	if ( view == m_activeView )
 	{
		QPixmap icon16 = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c , 16) :
		                     SmallIcon( view->msgManager()->protocol()->pluginIcon() );
		QPixmap icon32 = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c , 32) :
		                     SmallIcon( view->msgManager()->protocol()->pluginIcon() );
#ifdef Q_OS_UNIX
		KWin::setIcons( winId(), icon32, icon16 );
#endif
	}

	if ( m_tabBar )
		m_tabBar->setTabIcon(m_tabBar->indexOf( view ), c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c ) :
		                                   SmallIcon( view->msgManager()->protocol()->pluginIcon() ) );
}

void KopeteChatWindow::slotChatClosed()
{
	if( m_popupView )
		m_popupView->closeView();
	else
		m_activeView->closeView();
}

void KopeteChatWindow::slotPrepareDetachMenu(void)
{
	QMenu *detachMenu = actionDetachMenu->menu();
	detachMenu->clear();

	for ( unsigned id=0; id < windows.count(); id++ )
	{
		KopeteChatWindow *win = windows.at( id );
		if( win != this )
			detachMenu->insertItem( win->windowTitle(), id );
	}
}

void KopeteChatWindow::slotSendMessage()
{
	if ( m_activeView && m_activeView->canSend() )
	{
#if 0
		if( !animIcon.isNull() )
		{
			anim->setMovie( &animIcon );
			animIcon.unpause();
		}
#endif
		m_activeView->sendMessage();
	}
}

void KopeteChatWindow::slotPrepareContactMenu(void)
{
	KMenu *contactsMenu = actionContactMenu->menu();
	contactsMenu->clear();

	Kopete::Contact *contact;
	Kopete::ContactPtrList m_them;

	if( m_popupView )
		m_them = m_popupView->msgManager()->members();
	else
		m_them = m_activeView->msgManager()->members();

	//TODO: don't display a menu with one contact in it, display that
	// contact's menu instead. Will require changing text and icon of
	// 'Contacts' action, or something cleverer.
	uint contactCount = 0;

	foreach(contact, m_them)
	{
		KMenu *p = contact->popupMenu();
		connect ( actionContactMenu->menu(), SIGNAL(aboutToHide()),
			p, SLOT(deleteLater() ) );

		if( contact->metaContact() )
			contactsMenu->insertItem( contact->onlineStatus().iconFor( contact ) , contact->metaContact()->displayName(), p );
		else
			contactsMenu->insertItem( contact->onlineStatus().iconFor( contact ) , contact->contactId(), p );

		//FIXME: This number should be a config option
		if( ++contactCount == 15 && contact != m_them.last() )
		{
			KActionMenu *moreMenu = new KActionMenu( KIcon("folder_open"), i18n("More..."), this);
			connect ( actionContactMenu->menu(), SIGNAL(aboutToHide()),
				moreMenu, SLOT(deleteLater() ) );
			contactsMenu->addAction( moreMenu );
			contactsMenu = moreMenu->menu();
			contactCount = 0;
		}
	}
}

void KopeteChatWindow::slotPreparePlacementMenu()
{
	QMenu *placementMenu = actionTabPlacementMenu->menu();
	placementMenu->clear();

	placementMenu->insertItem( i18n("Top"), 0 );
	placementMenu->insertItem( i18n("Bottom"), 1 );
}

void KopeteChatWindow::slotPlaceTabs( int placement )
{
	if( m_tabBar )
	{

		if( placement == 0 )
			m_tabBar->setTabPosition( QTabWidget::Top );
		else
			m_tabBar->setTabPosition( QTabWidget::Bottom );

		saveOptions();
	}
}

void KopeteChatWindow::readOptions()
{
	// load and apply config file settings affecting the appearance of the UI
//	kDebug(14010) << k_funcinfo << endl;
	applyMainWindowSettings( KGlobal::config()->group( QLatin1String( "KopeteChatWindow" ) ) );
	//config->setGroup( QLatin1String("ChatWindowSettings") );
}

void KopeteChatWindow::saveOptions()
{
//	kDebug(14010) << k_funcinfo << endl;

        KConfigGroup cg( KGlobal::config(), QLatin1String( "KopeteChatWindow" ) );

	// saves menubar,toolbar and statusbar setting
	saveMainWindowSettings( cg  );
        cg.changeGroup( QLatin1String("ChatWindowSettings") );
	if( m_tabBar )
		cg.writeEntry ( QLatin1String("Tab Placement"), (int)m_tabBar->tabPosition() );

	cg.sync();
}

void KopeteChatWindow::slotChatSave()
{
//	kDebug(14010) << "KopeteChatWindow::slotChatSave()" << endl;
	if( isActiveWindow() && m_activeView )
		m_activeView->messagePart()->save();
}

void KopeteChatWindow::windowActivationChange( bool )
{
	if( isActiveWindow() && m_activeView )
		m_activeView->setActive( true );
}

void KopeteChatWindow::slotChatPrint()
{
	m_activeView->messagePart()->print();
}

void KopeteChatWindow::slotToggleStatusBar()
{
	if (statusBar()->isVisible())
		statusBar()->hide();
	else
		statusBar()->show();
}

void KopeteChatWindow::slotViewMenuBar()
{
	if( !menuBar()->isHidden() )
		menuBar()->hide();
	else
		menuBar()->show();
}

void KopeteChatWindow::slotSmileyActivated(const QString &sm)
{
	if ( !sm.isNull() )
		m_activeView->addText( ' ' + sm + ' ' );
	//we are adding space around the emoticon becasue our parser only display emoticons not in a word.
}

void KopeteChatWindow::slotRTFEnabled( ChatView* cv, bool enabled)
{
	if ( cv != m_activeView )
		return;

	if ( enabled )
		toolBar( "formatToolBar" )->show();
	else
		toolBar( "formatToolBar" )->hide();
	updateSpellCheckAction();
}

void KopeteChatWindow::slotAutoSpellCheckEnabled( ChatView* view, bool isEnabled )
{
	if ( view != m_activeView )
		return;

	toggleAutoSpellCheck->setEnabled( isEnabled );
	toggleAutoSpellCheck->setChecked( isEnabled );
	m_activeView->editPart()->toggleAutoSpellCheck( isEnabled );
}

bool KopeteChatWindow::queryClose()
{
	bool canClose = true;

//	kDebug( 14010 ) << " Windows left open:" << endl;
//	for( QPtrListIterator<ChatView> it( chatViewList ); it; ++it)
//		kDebug( 14010 ) << "  " << *it << " (" << (*it)->caption() << ")" << endl;

	for( Q3PtrListIterator<ChatView> it( chatViewList ); it; )
	{
		ChatView *view = *it;
		// move out of the way before view is removed
		++it;

		// FIXME: This should only check if it *can* close
		// and not start closing if the close can be aborted halfway, it would
		// leave us with half the chats open and half of them closed. - Martijn

		// if the view is closed, it is removed from chatViewList for us
		if ( !view->closeView() )
		{
			kDebug() << k_funcinfo << "Closing view failed!" << endl;
			canClose = false;
		}
	}
	return canClose;
}

bool KopeteChatWindow::queryExit()
{
	KopeteApplication *app = static_cast<KopeteApplication *>( kapp );
 	if ( app->sessionSaving()
		|| app->isShuttingDown() /* only set if KopeteApplication::quitKopete() or
									KopeteApplication::commitData() called */
		|| !Kopete::BehaviorSettings::self()->showSystemTray() /* also close if our tray icon is hidden! */
		|| isHidden() )
	{
		Kopete::PluginManager::self()->shutdown();
		return true;
	}
	else
		return false;
}

void KopeteChatWindow::closeEvent( QCloseEvent * e )
{
	// if there's a system tray applet and we are not shutting down then just do what needs to be done if a
	// window is closed.
	KopeteApplication *app = static_cast<KopeteApplication *>( kapp );
	if ( Kopete::BehaviorSettings::self()->showSystemTray() && !app->isShuttingDown() && !app->sessionSaving() ) {
//		hide();
		// BEGIN of code borrowed from KMainWindow::closeEvent
		// Save settings if auto-save is enabled, and settings have changed
		if ( settingsDirty() && autoSaveSettings() )
			saveAutoSaveSettings();

		if ( queryClose() ) {
			e->accept();
		}
		// END of code borrowed from KMainWindow::closeEvent
	}
	else
		KMainWindow::closeEvent( e );
}


void KopeteChatWindow::updateChatState( ChatView* cv, int newState )
{
	if ( m_tabBar )
	{
		switch( newState )
		{
			case ChatView::Highlighted:
			//	m_tabBar->setTabColor( cv, Qt::blue );
				break;
			case ChatView::Message:
			//	m_tabBar->setTabColor( cv, Qt::red );
				break;
			case ChatView::Changed:
			//	m_tabBar->setTabColor( cv, Qt::darkRed );
				break;
			case ChatView::Typing:
			//	m_tabBar->setTabColor( cv, Qt::darkGreen );
				break;
			case ChatView::Normal:
			default:
			//	m_tabBar->setTabColor( cv, KGlobalSettings::textColor() );
				break;
		}
	}
}

void KopeteChatWindow::updateChatTooltip( ChatView* cv )
{
	if ( m_tabBar )
		m_tabBar->setTabToolTip( m_tabBar->indexOf( cv ), QString::fromLatin1("<qt>%1</qt>").arg( cv->caption() ) );
}

void KopeteChatWindow::updateChatLabel()
{
	const ChatView* cv = dynamic_cast<const ChatView*>( sender() );
	if ( !cv || !m_tabBar )
		return;

	ChatView* chat  = const_cast<ChatView*>( cv );
	if ( m_tabBar )
	{
		m_tabBar->setTabText( m_tabBar->indexOf( chat ), chat->caption() );
		if ( m_tabBar->count() < 2 || m_tabBar->currentWidget() == static_cast<const QWidget *>(cv) )
			setCaption( chat->caption() );
	}
}

#include "kopetechatwindow.moc"

// vim: set noet ts=4 sts=4 sw=4:

