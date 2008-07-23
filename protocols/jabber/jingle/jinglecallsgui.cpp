 /*
  * jinglecallsgui.cpp - A GUI which displays all Jingle calls.
  *
  * Copyright (c) 2008 by Detlev Casanova <detlev.casanova@gmail.com>
  *
  * Kopete    (c) by the Kopete developers  <kopete-devel@kde.org>
  *
  * *************************************************************************
  * *                                                                       *
  * * This program is free software; you can redistribute it and/or modify  *
  * * it under the terms of the GNU General Public License as published by  *
  * * the Free Software Foundation; either version 2 of the License, or     *
  * * (at your option) any later version.                                   *
  * *                                                                       *
  * *************************************************************************
  */
#include "jinglecallsgui.h"
#include "jinglecallsmanager.h"
#include "jabberjinglesession.h"
#include "jabberjinglecontent.h"

#include "jinglesession.h"
#include "jinglecontent.h"

#include <KDebug>

static QString stateToString(XMPP::JingleSession::State s)
{
	//TODO : more precise states...
	switch (s)
	{
	case XMPP::JingleSession::Pending :
		return "Pending";
	case XMPP::JingleSession::Active :
		return "Active";
	default :
		return "Unknown";
	}
}

JingleCallsGui::JingleCallsGui(JingleCallsManager* parent)
: m_callsManager(parent)
{
	ui.setupUi(this);
	setupActions();

	JingleCallsModel *model = new JingleCallsModel(m_callsManager->jabberSessions());
	ui.treeView->setModel(model);
}

JingleCallsGui::~JingleCallsGui()
{

}

void JingleCallsGui::setupActions()
{
	// Create a new session
	QAction *newSession = new QAction(tr("Add Content"), this);
	ui.toolBar->addAction(newSession);
	connect(newSession, SIGNAL(triggered()), this, SLOT(slotNewSession()));

	// Adds a content to the session.
	QAction *addContent = new QAction(tr("New Session"), this);
	ui.toolBar->addAction(addContent);
	connect(addContent, SIGNAL(triggered()), this, SLOT(slotAddContent()));

	// Terminate the whole session.
	QAction *terminate = new QAction(tr("Terminate"), this);
	ui.toolBar->addAction(terminate);
	connect(terminate, SIGNAL(triggered()), this, SLOT(slotTerminate()));

	// ^ Actions on sessions
	ui.toolBar->addSeparator();
	// v Actions on contents
	
	// Remove the content.
	QAction *remove = new QAction(tr("Remove"), this);
	ui.toolBar->addAction(remove);
	connect(remove, SIGNAL(triggered()), this, SLOT(slotRemove()));

	ui.toolBar->addSeparator();

	// quit
	QAction *close = new QAction(tr("Close"), this);
	ui.toolBar->addAction(close);
	connect(close, SIGNAL(triggered()), this, SLOT(slotClose()));
}

void JingleCallsGui::slotNewSession()
{
	//TODO:Implement me !
}

void JingleCallsGui::slotAddContent()
{
	//TODO:Implement me !
}

void JingleCallsGui::slotTerminate()
{
	kDebug() << "Terminate session";
	TreeItem *item = static_cast<TreeItem*>(ui.treeView->currentIndex().internalPointer());
	if (item->session() == 0)
		return;
	item->session()->jingleSession()->terminate();
	removeSession(item->session());
	//TODO:Implement me !
	
}

void JingleCallsGui::slotRemove()
{
	//TODO:Implement me !
}

void JingleCallsGui::slotClose()
{
	hide();
}

void JingleCallsGui::addSession(JabberJingleSession* sess)
{
	kDebug() << "Add session" << (int) sess;
	QModelIndex rootIndex = ui.treeView->rootIndex().child(0, 0);
	TreeItem *rootItem = static_cast<TreeItem*>(rootIndex.internalPointer());
	if (!rootItem)
	{
		kDebug() << "Root item is Null :s";
		return;
	}

	QList<QVariant> sessData;
	sessData << sess->jingleSession()->to().full();
	sessData << stateToString(sess->jingleSession()->state());
	sessData << sess->upTime().toString("HH:mm"); //FIXME:find a better formatting : don't show 0 at the begining (no 00:03)
	TreeItem *sessItem = new TreeItem(sessData, rootItem);
	sessItem->setSessionPtr(sess);

	for (int i = 0; i < sess->contents().count(); i++)
	{
		QList<QVariant> contData;
		contData << sess->contents()[i]->contentName();
		TreeItem *contItem = new TreeItem(contData, sessItem);
		contItem->setContentPtr(sess->contents()[i]);
		sessItem->appendChild(contItem);
	}

	rootItem->appendChild(sessItem);
	ui.treeView->update(ui.treeView->model()->index(rootItem->childCount(), 0));
	ui.treeView->update(ui.treeView->model()->index(rootItem->childCount(), 1));
	ui.treeView->update(ui.treeView->model()->index(rootItem->childCount(), 2));
}

void JingleCallsGui::removeSession(JabberJingleSession* sess)
{
	kDebug() << "Remove session" << (int) sess;
	//Don't delete it, just remove it from the QTreeView.
}

/*DEPRECATED(sessions retrieved from m_callsManager)*/void JingleCallsGui::setSessions(const QList<JabberJingleSession*>& sessions)
{

}

// Model/View

// JingleCallsModel :

JingleCallsModel::JingleCallsModel(const QList<JabberJingleSession*> &data, QObject *parent)
 : QAbstractItemModel(parent)
{
	kDebug() << "Create Model";
	QList<QVariant> rootData;
	rootData << "Session with" << "State" << "Time";
	rootItem = new TreeItem(rootData);
	setModelUp(data);
}

void JingleCallsModel::setModelUp(const QList<JabberJingleSession*> &sessions)
{
	//FIXME:Later, each TreeItem could keep a pointer on the session or content it is linked to so edition is more easy.
	kDebug() << "Setting Model up with" << sessions.count() << "sessions";
	for (int i = 0; i < sessions.count(); i++)
	{
		QList<QVariant> sessData;
		sessData << sessions[i]->session()->to().full();
		sessData << stateToString(sessions[i]->session()->state());
		sessData << sessions[i]->upTime().toString("HH:mm"); //FIXME:find a better formatting : don't show 0 at the begining (no 00:03)
		TreeItem *sessItem = new TreeItem(sessData, rootItem);
		sessItem->setSessionPtr(sessions[i]);
		for (int j = 0; j < sessions[i]->contents().count(); j++)
		{
			QList<QVariant> contData;
			contData << sessions[i]->contents()[j]->contentName();
			TreeItem *contItem = new TreeItem(contData, sessItem);
			contItem->setContentPtr(sessions[i]->contents()[j]);
			sessItem->appendChild(contItem);
		}
		rootItem->appendChild(sessItem);
	}
}

JingleCallsModel::~JingleCallsModel()
{
	delete rootItem;
}

QModelIndex JingleCallsModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	TreeItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex JingleCallsModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int JingleCallsModel::rowCount(const QModelIndex& parent) const
{
	TreeItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int JingleCallsModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	else
		return rootItem->columnCount();
}

QVariant JingleCallsModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

	return item->data(index.column());
}

QVariant JingleCallsModel::headerData(int section, Qt::Orientation orientation,
			       int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}


// TreeItem

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
{
	parentItem = parent;
	itemData = data;
	contentPtr = 0L;
	sessionPtr = 0L;
}

TreeItem::~TreeItem()
{

}

void TreeItem::setContentPtr(JabberJingleContent* c)
{
	contentPtr = c;
}

void TreeItem::setSessionPtr(JabberJingleSession* s)
{
	sessionPtr = s;
}

void TreeItem::appendChild(TreeItem* child)
{
	childItems.append(child);
}

TreeItem *TreeItem::child(int row)
{
	return childItems.value(row);
}

int TreeItem::childCount() const
{
	return childItems.count();
}

int TreeItem::row() const
{
	if (parentItem)
		return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

	return 0;
}

int TreeItem::columnCount() const
{
	return itemData.count();
}

QVariant TreeItem::data(int column) const
{
	return itemData.value(column);
}

TreeItem *TreeItem::parent()
{
	return parentItem;
}
