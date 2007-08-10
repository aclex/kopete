/*  This file is part of the KDE project
    Copyright (C) 2007 Will Stephenson <wstephenson@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "tutorialplugin.h"

#include <KGenericFactory>

typedef KGenericFactory<TutorialPlugin> TutorialPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_tutorialplugin, TutorialPluginFactory( "kopete_tutorialplugin" )  )

TutorialPlugin::TutorialPlugin( QObject * parent, const QStringList & ) : Kopete::Plugin( TutorialPluginFactory::componentData(), parent )
{
}

TutorialPlugin::~TutorialPlugin()
{

}

#include "tutorialplugin.moc"