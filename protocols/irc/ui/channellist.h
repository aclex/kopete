 /*
    channellist.h - IRC Channel Search Widget

    Copyright (c) 2004      by Jason Keirstead <jason@keirstead.org>

    Kopete    (c) 2004      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef CHANNELLIST_H
#define CHANNELLIST_H

#include <qwidget.h>
#include <qmap.h>
#include <qpair.h>

namespace KIRC
{
class Engine;
}

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QListView;
class QSpinBox;
class QListViewItem;

class ChannelList
	: public QWidget
{
	Q_OBJECT

	public:
		ChannelList( QWidget *parent, KIRC::Engine *engine );

	public slots:
		void search();
		void reset();
		void clear();

	signals:
		void channelDoubleClicked( const QString &channel );
		void channelSelected( const QString &channel );

	private slots:
		void slotItemDoubleClicked( QListViewItem * i );
		void slotItemSelected( QListViewItem * i );
		void slotChannelListed( const QString & channel, uint users, const QString & topic );
		void slotListEnd();
		void slotDisconnected();
		void slotSearchCache();

	private:
		void checkSearchResult( const QString & channel, uint users, const QString & topic );

		QLabel* textLabel1_2;
		QLineEdit* channelSearch;
		QSpinBox* numUsers;
		QPushButton* mSearchButton;
		QListView* mChannelList;
		QVBoxLayout* ChannelListLayout;
		QHBoxLayout* layout72_2;
		KIRC::Engine *m_engine;
		bool mSearching;
		QString mSearch;
		uint mUsers;
		QMap< QString, QPair< uint, QString > > channelCache;
		QMap< QString, QPair< uint, QString > >::const_iterator cacheIterator;
};

#endif
