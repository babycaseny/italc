/*
 * main_window.cpp - main-file for iTALC-Application
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

class QColorGroup;

#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCloseEvent>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSplashScreen>
#include <QtGui/QSplitter>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QWorkspace>
#include <QtNetwork/QHostAddress>

#include "main_window.h"
#include "client_manager.h"
#include "dialogs.h"
#include "italc_side_bar.h"
#include "overview_widget.h"
#include "user_list.h"
#include "snapshot_list.h"
#include "config_widget.h"
#include "support_widget.h"
#include "messagebox.h"
#include "tool_button.h"
#include "isd_connection.h"


bool mainWindow::ensureConfigPathExists( void )
{
	QDir home = QDir::home();
	// does a file/dir named ITALC_CONFIG_PATH exist?
	if( home.exists( ITALC_CONFIG_PATH ) == FALSE )
	{
		// no, then create dir
		return( home.mkdir( ITALC_CONFIG_PATH ) );
	}
	// exists, so test, whether it is a file
	else if( home.cd( ITALC_CONFIG_PATH ) == FALSE )
	{
		// it is, so we remove it
		if( home.remove( ITALC_CONFIG_PATH ) == FALSE )
		{
			// remove failed, we give up
			return( FALSE );
		}
		return( home.mkdir( ITALC_CONFIG_PATH ) );
	}
	return( TRUE );
}




mainWindow::mainWindow() :
	QMainWindow(),
	m_openedTabInSideBar( 1 ),
	m_localISD( NULL )
{
	setWindowTitle( tr( "iTALC" ) + " " + VERSION );
	setWindowIcon( QPixmap( ":/resources/splash.png" ) );

	if( mainWindow::ensureConfigPathExists() == FALSE )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		messageBox::information( tr( "No write-access" ),
			tr( "Could not read/write or create directory %1! "
			"For running iTALC, make sure you have write-access "
			"to your home-directory and to %1 (if it "
				"already exists)." ).arg( ITALC_CONFIG_PATH ).
						arg( ITALC_CONFIG_PATH ) );
		return;
	}

	if( isdConnection::initAuthentication() == FALSE )
	{
		if( splashScreen != NULL )
		{
			splashScreen->hide();
		}
		messageBox::information( tr( "New key-pair generated" ),
			tr( 	"No authentication-keys were found or your "
				"old ones contained errors, so a new key-"
				"pair has been generated. Please have your "
				"administrator to distribute your public key. "
				"Otherwise you won't be able to access "
						"computers using iTALC." ) );
	}

	m_localISD = new isdConnection( QHostAddress( QHostAddress::LocalHost
								).toString() );
	if( m_localISD->open() != isdConnection::Connected )
	{
		messageBox::information( tr( "No service-daemon running" ),
			tr( 	"There seems to be no service-daemon running "
				"on this computer, which is neccessary for "
				"running iTALC properly. Please make sure "
				"a service-daemon is started at system-startup "
				"and try again." ) );
		return;
	}

	m_localISD->demoServerStop();
	m_localISD->demoServerRun();



	QWidget * hbox = new QWidget( this );
	QHBoxLayout * hbox_layout = new QHBoxLayout( hbox );
	hbox_layout->setMargin( 0 );

	// create splitter, which is used for splitting sidebar-workspaces
	// from main-workspace
	m_splitter = new QSplitter( Qt::Horizontal, hbox );
#if QT_VERSION >= 0x030200
	m_splitter->setChildrenCollapsible( FALSE );
#endif

	// create sidebar
	m_sideBar = new italcSideBar( italcSideBar::Vertical, hbox,
								m_splitter );
	m_sideBar->setStyle( italcSideBar::VSNET/*KDEV3ICON*/ );



	m_workspace = new QWorkspace( m_splitter );
	m_workspace->setScrollBarsEnabled( TRUE );


	printf("here3\n");
	QWidget * twp = m_sideBar->tabWidgetParent();
	// now create all sidebar-workspaces
	m_overviewWidget = new overviewWidget( this, twp );
	printf("here4\n");
	m_clientManager = new clientManager( this, twp );
	printf("here5\n");
	m_userList = new userList( this, twp );
	printf("here6\n");
	m_snapshotList = new snapshotList( this, twp );
	printf("here7\n");
	m_configWidget = new configWidget( this, twp );
	printf("here8\n");
	m_supportWidget = new supportWidget( this, twp );
	printf("here9\n");

	// append sidebar-workspaces to sidebar
	int id = 0;
	m_sideBar->appendTab( m_overviewWidget, ++id );
	m_sideBar->appendTab( m_clientManager, ++id );
	m_sideBar->appendTab( m_userList, ++id );
	m_sideBar->appendTab( m_snapshotList, ++id );
	m_sideBar->appendTab( m_configWidget, ++id );
	m_sideBar->appendTab( m_supportWidget, ++id );
	m_sideBar->setPosition( italcSideBar::Left );
	m_sideBar->setTab( m_openedTabInSideBar, TRUE );

	printf("here10\n");
	setCentralWidget( hbox );
	hbox_layout->addWidget( m_sideBar );
	hbox_layout->addWidget( m_splitter );




	printf("here6\n");
	// create the action-toolbar
	m_toolBar = new QToolBar( tr( "Actions" ), this );
	m_toolBar->setObjectName( "maintoolbar" );

	addToolBar( Qt::TopToolBarArea, m_toolBar );

	QToolButton * scr = new QToolButton;
	scr->setToolButtonStyle( Qt::ToolButtonTextOnly );
	scr->setText( tr( "Switch\nclassroom" ) );
	scr->setMenu( m_clientManager->quickSwitchMenu() );
	scr->setPopupMode( toolButton::InstantPopup );
	scr->setWhatsThis( tr( "Click on this button, to switch between "
							"classrooms." ) );


	QButtonGroup * mode_group = new QButtonGroup( this );

	toolButton * overview_mode = new toolButton(
			QPixmap( ":/resources/overview_mode.png" ),
			tr( "Overview mode" ),
			tr( "This is the default mode in iTALC and allows you "
				"to have an overview over all visible "
				"computers." ),
			NULL, NULL, m_toolBar );

	toolButton * fsdemo_mode = new toolButton(
			QPixmap( ":/resources/fullscreen_demo.png" ),
			tr( "Fullscreen demo" ),
			tr( "In this mode your screen is being displayed on "
				"all shown computers. Furthermore the users "
				"aren't able to do something else as all input "
				"devices are locked in this mode." ),
			NULL, NULL, m_toolBar );

	toolButton * windemo_mode = new toolButton(
			QPixmap( ":/resources/window_demo.png" ),
			tr( "Window demo" ),
			tr( "In this mode your screen being displayed in a "
				"window on all shown computers. The users are "
				"able to switch to other windows and thus "
				"can continue to work." ),
			NULL, NULL, m_toolBar );

	toolButton * lock_mode = new toolButton(
			QPixmap( ":/resources/locked.png" ),
			tr( "Lock desktops" ),
			tr( "To have all user's full attention you can lock "
				"their desktops using this button. "
				"In this mode all input devices are locked and "
				"the screen is black." ),
			NULL, NULL, m_toolBar );

	overview_mode->setCheckable( TRUE );
	fsdemo_mode->setCheckable( TRUE );
	windemo_mode->setCheckable( TRUE );
	lock_mode->setCheckable( TRUE );

	mode_group->addButton( overview_mode, client::Mode_Overview );
	mode_group->addButton( fsdemo_mode, client::Mode_FullscreenDemo );
	mode_group->addButton( windemo_mode, client::Mode_WindowDemo );
	mode_group->addButton( lock_mode, client::Mode_Locked );

	overview_mode->setChecked( TRUE );
	connect( mode_group, SIGNAL( buttonClicked( int ) ),
		m_clientManager, SLOT( changeGlobalClientMode( int ) ) );



	toolButton * text_msg = new toolButton(
			QPixmap( ":/resources/text_message.png" ),
			tr( "Send text message" ),
			tr( "Use this button to send a text message to all "
				"users e.g. to tell them new tasks etc." ),
			m_clientManager, SLOT( sendMessage() ), m_toolBar );

/*	m_toolBar->addAction( QPixmap( ":/resources/distribute_file.png" ),
						tr( "Distribute" ),
							m_clientManager,
							SLOT( distributeFile() )
			)->setWhatsThis( tr( "If you want to distribute a file "
					"to all pupils (e.g. a worksheet to "
					"be filled out), you can click on this "
					"button. Afterwards you can select the "
					"file you want to distribute and then "
					"this file is copied into \"%1\" in "
					"every pupils home-directory."
					).arg( client::tr( "material" ) ) );

	QAction * cf = m_toolBar->addAction(
				QPixmap( ":/resources/collect_files.png" ),
						tr( "Collect" ),
						m_clientManager,
						SLOT( collectFiles() ) );
	cf->setWhatsThis( tr( "For collecting a specific file "
				"from every pupil click on this "
				"button. Then you can enter the "
				"filename (wildcards are possible) and "
				"the file is copied from each pupil if "
				"it exists. All collected files are "
				"stored in the directory \"%1\" in "
				"your home-directory." ).arg (
					client::tr( "collected-files" ) ) );

	QMenu * file_collect_menu = new QMenu( this );
	file_collect_menu->addAction( QPixmap( ":/resources/users.png" ),
					tr( "Collect files from users logged "
									"in" ),
						m_clientManager,
						SLOT( collectFiles() ) );

	file_collect_menu->addAction( QPixmap( ":/resources/filesave.png" ),
					tr( "Collect files from users in "
						"exported user-list" ),
					m_clientManager,
					SLOT( collectFilesFromUserList() ) );
	cf->setMenu( file_collect_menu );
	//cf->setMenuDelay( 1 );*/


	toolButton * power_on = new toolButton(
			QPixmap( ":/resources/power_on.png" ),
			tr( "Power on computers" ),
			tr( "Click this button to power on all visible "
				"computers. This way you do not have to turn "
				"on each computer by hand." ),
			m_clientManager, SLOT( powerOnClients() ), m_toolBar );

	toolButton * reboot = new toolButton(
			QPixmap( ":/resources/reboot.png" ),
			tr( "Reboot computers" ),
			tr( "When clicking this button all shown computers "
				"are rebooted." ),
			m_clientManager, SLOT( rebootClients() ), m_toolBar );

	toolButton * power_off = new toolButton(
			QPixmap( ":/resources/power_off.png" ),
			tr( "Power off computers" ),
			tr( "To power off all shown computers (e.g. after "
				"the lesson has finished) you can click this "
				"button." ),
			m_clientManager, SLOT( powerOffClients() ), m_toolBar );


	toolButton * adjust_size = new toolButton(
			QPixmap( ":/resources/adjust_size.png" ),
			tr( "Adjust windows and their size" ),
			tr( "When clicking this button the biggest possible "
				"size for the screen-windows is adjusted. "
				"Furthermore all windows are aligned." ),
			m_clientManager, SLOT( adjustWindows() ), m_toolBar );

/*	m_toolBar->addAction( QPixmap( ":/resources/inc_client_size.png" ),
					tr( "Increase" ),
						m_clientManager,
						SLOT( increaseClientSize() )
			)->setWhatsThis( tr( "When clicking this button, the "
					"size of all visible client-windows is "
					"increased, if they do not already "
					"have the biggest possible size. " ) );

	m_toolBar->addAction( QPixmap( ":/resources/dec_client_size.png" ),
					tr( "Decrease" ),
						m_clientManager,
						SLOT( decreaseClientSize() )
			)->setWhatsThis( tr( "When clicking this button, the "
					"size of the visible client-windows is "
					"decreased, if they do not already "
					"have the smallest possible size." ) );
*/
	m_toolBar->addWidget( scr );

	m_toolBar->addSeparator();
	m_toolBar->addWidget( overview_mode );
	m_toolBar->addWidget( fsdemo_mode );
	m_toolBar->addWidget( windemo_mode );
	m_toolBar->addWidget( lock_mode );
	m_toolBar->addSeparator();
	m_toolBar->addWidget( text_msg );
	m_toolBar->addSeparator();
	m_toolBar->addWidget( power_on );
	m_toolBar->addWidget( reboot );
	m_toolBar->addWidget( power_off );
	m_toolBar->addSeparator();
	m_toolBar->addWidget( adjust_size );

	printf("here4\n");
	restoreState( m_clientManager->winCfg().toAscii() );

	QTimer::singleShot( 1000, m_clientManager, SLOT( updateClients() ) );

	m_updateThread = new updateThread( this );

}




mainWindow::~mainWindow()
{
}




void mainWindow::closeEvent( QCloseEvent * _ce )
{
	// make sure, no screens are locked, demos running etc.
	m_clientManager->changeGlobalClientMode( client::Mode_Overview );

	m_updateThread->terminate();
	m_updateThread->wait();

	m_clientManager->doCleanupWork();
	_ce->accept();

	m_localISD->demoServerStop();
	delete m_localISD;
	m_localISD = NULL;
}





void mainWindow::aboutITALC( void )
{
	aboutDialog().exec();
}







mainWindow::updateThread::updateThread( mainWindow * _main_window ) :
	QThread(),
	m_mainWindow( _main_window )
{
	start();
}




void mainWindow::updateThread::run( void )
{
	while( 1 )
	{
		m_mainWindow->m_localISD->handleServerMessages();

		// reload all clients...

		QVector<client *> clients =
			m_mainWindow->getClientManager()->visibleClients();

		// loop through all clients
		for( QVector<client *>::iterator it = clients.begin();
						it != clients.end(); ++it )
		{
			// reload current client
			( *it )->processCmd( client::Reload, CONFIRM_NO );
		}

		m_mainWindow->m_userList->reload();

		if( client::reloadSnapshotList() )
		{
			m_mainWindow->m_snapshotList->reloadList();
		}
		client::resetReloadOfSnapshotList();

		// now sleep before reloading clients again
		QThread::sleep(
			m_mainWindow->getClientManager()->updateInterval() );

		// now do cleanup-work
		m_mainWindow->getClientManager()->doCleanupWork();
	}
}




#include "main_window.moc"
#include "italc_side_bar.moc"