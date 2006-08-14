/*
 * isd_server.h - ISD Server
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ISD_SERVER_H
#define _ISD_SERVER_H

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtNetwork/QTcpServer>

#include "isd_base.h"


class IVS;
class demoClient;
class demoServer;
class lockWidget;


class isdServer : public QTcpServer
{
	Q_OBJECT
public:
	isdServer( const quint16 _isd_port, const quint16 _ivs_port,
						int _argc, char * * _argv );
	~isdServer();

	bool authSecTypeItalc( socketDispatcher _sd, void * _user,
				italcAuthTypes _auth_type = ItalcAuthDSA );
	int processClient( socketDispatcher _sd, void * _user );

	static bool protocolInitialization( socketDevice & _sd,
						italcAuthTypes _auth_type,
						bool _demo_server = FALSE );

	static QByteArray s_appInternalChallenge;


private slots:
	void acceptNewConnection( void );
	void processClients( void );

	void checkForPendingActions( void );

	void demoWindowClosed( QObject * );

	// client-functions
	void startDemo( const QString & _master_host, bool _fullscreen );
	void stopDemo( void );

	void lockDisplay( void );
	void unlockDisplay( void );

	void displayTextMessage( const QString & _msg );

	void remoteControlDisplay( const QString & _ip, bool _view_only );


private:
	void allowDemoClient( const QString & _host );
	void denyDemoClient( const QString & _host );


	italcAuthTypes m_authType;
	QStringList m_allowedDemoClients;

	QMutex m_actionMutex;
	QList<QPair<ISD::commands, QString> > m_pendingActions;

	IVS * m_ivs;
	demoClient * m_demoClient;
	demoServer * m_demoServer;
	lockWidget * m_lockWidget;

} ;



bool authSecTypeItalc( socketDispatcher _sd, void * _user, italcAuthTypes _at );
int processItalcClient( socketDispatcher _sd, void * user );


#endif
