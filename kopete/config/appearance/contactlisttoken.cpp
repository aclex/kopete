/*
    Kopete ContactList Token

    Copyright (c) 2009 by Roman Jarosz <kedgedev@gmail.com>

    Kopete    (c) 2009 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "contactlisttoken.h"

#include <QFont>
#include <QMenu>

#include <KAction>
#include <KLocale>

#include "kopeteitemdelegate.h"

const QString ActionSmallName = QLatin1String( "ActionSmall" );

Token * ContactListTokenFactory::createToken(const QString &text, const QString &iconName, int value, QWidget *parent)
{
	return new ContactListToken( text, iconName, value, parent );
}

ContactListToken::ContactListToken( const QString &text, const QString &iconName, int value, QWidget *parent )
: TokenWithLayout( text, iconName, value, parent ), m_small( false )
{
}

void ContactListToken::fillMenu( QMenu * menu )
{
	KAction *smallAction = new KAction( KIcon( "format-text-bold"), i18n( "Small" ), menu );
	smallAction->setObjectName( ActionSmallName );
	smallAction->setCheckable( true );
	smallAction->setChecked( m_small );

	menu->addAction( smallAction );
	TokenWithLayout::fillMenu( menu );
}

void ContactListToken::menuExecuted( const QAction* action )
{
	TokenWithLayout::menuExecuted( action );
	if( action->objectName() == ActionSmallName )
		setSmall( action->isChecked() );
}

bool ContactListToken::small() const
{
	return m_small;
}

void ContactListToken::setSmall( bool small )
{
	if ( m_small == small )
		return;

	m_small = small;
	QFont font( ( small ) ? KopeteItemDelegate::smallFont( this->font() ) : KopeteItemDelegate::normalFont( this->font() ) );
	font.setBold( bold() );
	font.setItalic( italic() );
	m_label->setFont( font );
    emit changed();
}

#include "contactlisttoken.moc"
