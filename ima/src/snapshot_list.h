/*
 * snapshot_list.h - declaration of snapshot-list for side-bar
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SNAPSHOT_LIST_H
#define _SNAPSHOT_LIST_H

#include <QtGui/QWidget>

#include "side_bar_widget.h"
#include "paths.h"


class QLabel;
class QPushButton;
class QListWidget;


const QString SCREENSHOT_PATH = "/" + QString( ITALC_CONFIG_PATH ) +
							"/snapshots/";


class snapshotList : public sideBarWidget
{
	Q_OBJECT
public:
	snapshotList( mainWindow * _main_window, QWidget * _parent );
	virtual ~snapshotList();


public slots:
	void reloadList( void );



private slots:
	void snapshotSelected( const QString & _s );
	void snapshotDoubleClicked( const QString & _s );
	void showSnapshot( void );
	void deleteSnapshot( void );


private:
	QListWidget * m_list;
	QLabel * m_preview;
	QLabel * m_user;
	QLabel * m_date;
	QLabel * m_time;
	QLabel * m_client;
	QPushButton * m_showBtn;
	QPushButton * m_deleteBtn;
	QPushButton * m_reloadBtn;

} ;


#endif