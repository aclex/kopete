/* This file is part of ksirc
   Copyright (c) 2001 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/*
Color parser code courtesy of ksirc <http://www.kde.org>
Modified by Jason Keirstead <jason@keirstead.org>
*/

// $Id$

#include <qstring.h>
#include <qcolor.h>
#include <qregexp.h>

#include <kdebug.h>
#include "ksparser.h"

QString KSParser::parse( const QString &message )
{
    QString res;
    m_tags.clear();
    m_attributes.clear();

    for (unsigned int i = 0; i < message.length();)
    {
        QChar ch = message[i++];

        if (ch.latin1() == 0x03 || ch == '~' && i < message.length())
        {
            QChar next = message[ i++ ];
            if (next.latin1() >= 0x30 && next.latin1() <= 0x39)
            {
                int fg = -1, len;
                int bg = -1;
                QRegExp colorRex("^[0-9]+");
                if (colorRex.search(message.mid(--i)) >= 0)
                {
                    len = colorRex.matchedLength();
                    fg = message.mid(i, len).toInt();
                    i += len;
                }
                if (message[i] == ',')
                {
                    if (colorRex.search(message.mid(++i)) >= 0)
                    {
                        len = colorRex.matchedLength();
                        bg = message.mid(i, len).toInt();
                        i += len;
                    }
                }
                QColor c = ircColor(fg);
                if ( c.isValid() )
                    res += pushTag( "font", QString( "color=\"%1\"" ).arg( c.name() ) );
                //else
                //        res += popTag( "font" );

                c = ircColor(bg);
                if ( c.isValid() )
                    res += pushTag( "font", QString( "bgcolor=\"%1\"" ).arg( c.name() ) );
            }
            else if (ch.latin1() == 0x03)
                res += popTag( "font" );
            else if (ch == '~')
            {
                switch (next)
                {
                case 'c':
                    res += popTag( "font" );
                    break;
                case 'C':
                    res += popAll();
                    break;
                case 'r': break;
                case 's': break;
                case 'b':
		case 'B':
                    res += toggleTag( "b" );
                    break;
                case 'u':
		case 'U':
                    res += toggleTag( "u" );
                    break;
                case 'n':
                    //res += pushTag( "font", QString( "color=\"%1\"" ).arg( ksopts->nickForeground.name() ) );
                    break;
                case 'o':
                    //res += pushTag( "font", QString( "color=\"%1\"" ).arg( ksopts->ownNickColor.name() ) );
                    break;
                case '~':
                    res += ch;
                }
            }
        }
        else
            res += ch;

    }

    res.append( popAll() );

    return res;
}

QString KSParser::pushTag(const QString &tag, const QString &attributes)
{
    QString res;
    m_tags.push(tag);
    if (!m_attributes.contains(tag))
        m_attributes.insert(tag, attributes);
    else if (!attributes.isEmpty())
        m_attributes.replace(tag, attributes);
    res.append("<" + tag);
    if (!m_attributes[tag].isEmpty())
        res.append(" " + m_attributes[tag]);
    return res + ">";
}

QString KSParser::popTag(const QString &tag)
{
    if (!m_tags.contains(tag))
        return QString::null;

    QString res;
    QValueStack<QString> savedTags;
    while (m_tags.top() != tag)
    {
        savedTags.push(m_tags.pop());
        res.append("</" + savedTags.top() + ">");
    }
    res.append("</" + m_tags.pop() + ">");
    m_attributes.remove(tag);
    while (!savedTags.isEmpty())
        res.append(pushTag(savedTags.pop()));
    return res;
}

QString KSParser::toggleTag(const QString &tag)
{
    return m_attributes.contains(tag) ? popTag(tag) : pushTag(tag);
}

QString KSParser::popAll()
{
    QString res;
    while (!m_tags.isEmpty())
        res.append("</" + m_tags.pop() + ">");
    m_attributes.clear();
    return res;
}

QColor KSParser::ircColor(int code)
{
	switch( code )
	{
		case 0:
			return Qt::white;
		case 1:
			return Qt::black;
		case 2:
			return QColor("darkblue");
		case 3:
			return QColor("darkgreen");
		case 4:
			return Qt::red;
		case 5:
			return QColor("darkred");
		case 6:
			return QColor("darkmagenta");
		case 7:
			return QColor("orange");
		case 8:
			return Qt::yellow;
		case 9:
			return Qt::green;
		case 10:
			return QColor("darkcyan");
		case 11:
			return Qt::cyan;
		case 12:
			return Qt::blue;
		case 13:
			return Qt::magenta;
		case 14:
			return QColor("darkgray");
		case 15:
			return Qt::gray;
		default:
			return QColor();
	}
}
