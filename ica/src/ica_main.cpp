/*
 * ica_main.cpp - main-file for ICA (iTALC Client Application)
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtNetwork/QTcpServer>

#include "remote_control_widget.h"
#include "isd_server.h"
#include "ivs.h"


int ISDMain( int argc, char * * argv )
{
	QApplication app( argc, argv );

	QTranslator app_tr;
	app_tr.load( ":/resources/" + QLocale::system().name().left( 2 ) +
									".qm" );
	app.installTranslator( &app_tr );

	app.setQuitOnLastWindowClosed( FALSE );

	int isd_port = PortOffsetISD;
	int ivs_port = PortOffsetIVS;

	QStringListIterator arg_it( QApplication::arguments() );
	while( arg_it.hasNext() )
	{
		const QString & a = arg_it.next();
		if( a == "-isdport" && arg_it.hasNext() )
		{
			isd_port = arg_it.next().toInt();
		}
		else if( a == "-ivsport" && arg_it.hasNext() )
		{
			ivs_port = arg_it.next().toInt();
		}
		else if( a == "-rctrl" && arg_it.hasNext() )
		{
			new remoteControlWidget( arg_it.next() );
			return( app.exec() );
		}
	}

	new isdServer( isd_port, ivs_port, argc, argv );

	return( app.exec() );
}



// platform-specific startup-code follows

#ifdef BUILD_WIN32

#include <windows.h>


extern HINSTANCE	hAppInstance;
extern DWORD		mainthreadId;


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
						PSTR szCmdLine, int iCmdShow )
{
	// save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();

	QStringList cmdline = QString( szCmdLine ).toLower().split( ' ' );

	char * * argv = new char *[cmdline.size()];
	int argc = 0;
	for( QStringList::iterator it = cmdline.begin(); it != cmdline.end();
								++it, ++argc )
	{
		argv[argc] = new char[it->length() + 1];
		strcpy( argv[argc], it->toAscii().constData() );
	}

	return( ISDMain( argc, argv ) );
}


#else


int main( int argc, char * * argv )
{
	return( ISDMain( argc, argv ) );
}


#endif

