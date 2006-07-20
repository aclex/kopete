/*
    Kopete Yahoo Protocol

    Copyright (c) 2005 by Matt Rogers                 <mattr@kde.org>
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

#ifndef YAHOOWEBCAMDIALOG_H_
#define YAHOOWEBCAMDIALOG_H_

#include <qstring.h>
//Added by qt3to4:
#include <QPixmap>
#include <QLabel>
#include <kdialog.h>


class QPixmap;
class QWidget;
class YahooContact;

namespace Kopete
{
	class WebcamWidget;
}

class YahooWebcamDialog : public KDialog
{
Q_OBJECT
public:
	YahooWebcamDialog( const QString &, QWidget* parent = 0 );
	~YahooWebcamDialog();
	
	void setViewer( const QStringList & );
public slots:
	void newImage( const QPixmap &image );
	void webcamClosed( int );
	void webcamPaused();
signals:
	void closingWebcamDialog();
	
private:
	Kopete::WebcamWidget *m_imageContainer;
	QLabel *m_Viewer;
	QString contactName;
	
};

#endif
//kate: indent-mode csands; auto-insert-doxygen on;
