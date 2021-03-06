/*
    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef _KOPETE_VERSION_H_
#define _KOPETE_VERSION_H_

#define KOPETE_VERSION_STRING "1.8.1"
#define KOPETE_VERSION_MAJOR 1
#define KOPETE_VERSION_MINOR 8
#define KOPETE_VERSION_RELEASE 1
#define KOPETE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

#define KOPETE_VERSION \
  KOPETE_MAKE_VERSION(KOPETE_VERSION_MAJOR,KOPETE_VERSION_MINOR,KOPETE_VERSION_RELEASE)

#define KOPETE_IS_VERSION(a,b,c) ( KOPETE_VERSION >= KOPETE_MAKE_VERSION(a,b,c) )


#endif // _KOPETE_VERSION_H_
