/*
 * vncview.cpp - VNC-viewer-widget
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


#define XK_KOREAN
#include "rfb/keysym.h"

#include "vncview.h"
#include "ivs_connection.h"
#include "messagebox.h"
#include "qt_user_events.h"
#include "progress_widget.h"

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>



vncView::vncView( const QString & _host, bool _view_only, QWidget * _parent ) :
	QWidget( _parent, Qt::X11BypassWindowManagerHint ),
	m_viewOnly( _view_only ),
	m_buttonMask( 0 )
{
	m_establishingConnection = new progressWidget(
		tr( "Establishing connection to %1 ..." ).arg( _host ),
					":/resources/watch%1.png", 16, this );

	m_connection = new ivsConnection( _host, ivsConnection::QualityHigh,
									this );
	m_connection->open();

	if( !m_viewOnly )
	{
		// only connect slots for sending events, if we're not in
		// view-only mode
		connect( this,
			SIGNAL( pointerEvent( Q_UINT16, Q_UINT16, Q_UINT16 ) ),
			m_connection,
			SLOT( sendPointerEvent( Q_UINT16, Q_UINT16,
								Q_UINT16 ) ) );
		connect( this, SIGNAL( keyEvent( Q_UINT32, bool ) ),
			m_connection, SLOT( sendKeyEvent( Q_UINT32, bool ) ) );
	}

	setMouseTracking( TRUE );
	//setWidgetAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute( Qt::WA_NoSystemBackground, true );
	setAttribute( Qt::WA_DeleteOnClose, true );
	showMaximized();

	QSize parent_size = size();
	if( parentWidget() != NULL )
	{
		parent_size = parentWidget()->size();
	}
	resize( qMin( m_connection->framebufferSize().width(),
						parent_size.width() ),
			qMin( m_connection->framebufferSize().height(),
						parent_size.height() ) );

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), m_connection,
			SLOT( sendIncrementalFramebufferUpdateRequest() ) );
	if( _view_only )
	{
		t->start( 80 );
	}
	else
	{
		// when remote-controlling we need more updates for smoother
		// usage
		t->start( 50 );
	}

	framebufferUpdate();
}




vncView::~vncView()
{
	delete m_connection;
}




void vncView::framebufferUpdate( void )
{
	// not yet connected or connection lost while handling messages?
	if( m_connection->state() != ivsConnection::Connected ||
			!m_connection->handleServerMessages( FALSE, 3 ) )
	{
		// as the "establishing connection"-progress-widget is semi-
		// transparent and has a non-rectangular shape, we have to
		// disable paint-on-screen-capability as long as we display this
		// progress-widget
		setAttribute( Qt::WA_PaintOnScreen, false );
		m_establishingConnection->show();
		// try to open the connection
		m_connection->open();
	}

	if( m_connection->state() == ivsConnection::Connected )
	{
		m_establishingConnection->hide();
		// after we hid the progress-widget, we may use direct painting
		// again
		setAttribute( Qt::WA_PaintOnScreen, true );
		if( parentWidget() )
		{
			// if we have a parent it's likely remoteControlWidget
			// which needs resize-events for updating its toolbar
			// properly
			parentWidget()->resize( parentWidget()->size() );
		}
	}

	// check whether to scroll because mouse-cursor is at an egde which
	// doesn't correspond to the framebuffer's edge
	QPoint mp = mapFromGlobal( QCursor::pos() );
	const int MAGIC_MARGIN = 6;
	if( mp.x() <= MAGIC_MARGIN && m_viewOffset.x() > 0 )
	{
		m_viewOffset.setX( qMax( 0, m_viewOffset.x() -
						( MAGIC_MARGIN - mp.x() ) ) );
		update();
	}
	else if( mp.x() > width() - MAGIC_MARGIN && m_viewOffset.x() <=
			m_connection->framebufferSize().width() - width() )
	{
		m_viewOffset.setX( qMin( m_viewOffset.x() +
					( MAGIC_MARGIN + mp.x() - width() ),
			m_connection->framebufferSize().width() - width() ) );
		update();
	}

	if( mp.y() <= MAGIC_MARGIN )
	{
		if( m_viewOffset.y() > 0 )
		{
			m_viewOffset.setY( qMax( 0, m_viewOffset.y() -
						( MAGIC_MARGIN - mp.y() ) ) );
			update();
		}
		else if( mp.y() < 2 )
		{
			// special signal for allowing parent-widgets to
			// show a toolbar etc.
			emit mouseAtTop();
		}
	}
	else if( mp.y() > height() - MAGIC_MARGIN && m_viewOffset.y() <=
			m_connection->framebufferSize().height() - height() )
	{
		m_viewOffset.setY( qMin( m_viewOffset.y() +
					( MAGIC_MARGIN + mp.y() - height() ),
			m_connection->framebufferSize().height() - height() ) );
		update();
	}

	// we want to update everything in 20 ms
	QTimer::singleShot( 20, this, SLOT( framebufferUpdate() ) );
}




// our builtin keyboard-handler
void vncView::keyPressEvent( QKeyEvent * _ke )
{
	int key = 0;

	switch( _ke->key() )
	{
		// modifiers are handled separately
		case Qt::Key_Shift:
		case Qt::Key_Control:
		case Qt::Key_Meta:
		case Qt::Key_Alt:
			QWidget::keyPressEvent( _ke );
			return;
		case Qt::Key_Escape: key = XK_Escape; break;
		case Qt::Key_Tab: key = XK_Tab; break;
		case Qt::Key_Backtab: key = XK_ISO_Left_Tab; break;
		case Qt::Key_Backspace: key = XK_BackSpace; break;
		case Qt::Key_Return: key = XK_Return; break;
		case Qt::Key_Insert: key = XK_Insert; break;
		case Qt::Key_Delete: key = XK_Delete; break;
		case Qt::Key_Pause: key = XK_Pause; break;
		case Qt::Key_Print: key = XK_Print; break;
		case Qt::Key_Home: key = XK_Home; break;
		case Qt::Key_End: key = XK_End; break;
		case Qt::Key_Left: key = XK_Left; break;
		case Qt::Key_Up: key = XK_Up; break;
		case Qt::Key_Right: key = XK_Right; break;
		case Qt::Key_Down: key = XK_Down; break;
		case Qt::Key_PageUp: key = XK_Prior; break;
		case Qt::Key_PageDown: key = XK_Next; break;
		case Qt::Key_CapsLock: key = XK_Caps_Lock; break;
		case Qt::Key_NumLock: key = XK_Num_Lock; break;
		case Qt::Key_ScrollLock: key = XK_Scroll_Lock; break;
		case Qt::Key_Super_L: key = XK_Super_L; break;
		case Qt::Key_Super_R: key = XK_Super_R; break;
		case Qt::Key_Menu: key = XK_Menu; break;
		case Qt::Key_Hyper_L: key = XK_Hyper_L; break;
		case Qt::Key_Hyper_R: key = XK_Hyper_R; break;
		case Qt::Key_Help: key = XK_Help; break;
		case Qt::Key_AltGr: key = XK_ISO_Level3_Shift; break;
		case Qt::Key_Multi_key: key = XK_Multi_key; break;
		case Qt::Key_SingleCandidate: key = XK_SingleCandidate; break;
		case Qt::Key_MultipleCandidate: key = XK_MultipleCandidate; break;
		case Qt::Key_PreviousCandidate: key = XK_PreviousCandidate; break;
		case Qt::Key_Mode_switch: key = XK_Mode_switch; break;
		case Qt::Key_Kanji: key = XK_Kanji; break;
		case Qt::Key_Muhenkan: key = XK_Muhenkan; break;
		case Qt::Key_Henkan: key = XK_Henkan; break;
		case Qt::Key_Romaji: key = XK_Romaji; break;
		case Qt::Key_Hiragana: key = XK_Hiragana; break;
		case Qt::Key_Katakana: key = XK_Katakana; break;
		case Qt::Key_Hiragana_Katakana: key = XK_Hiragana_Katakana; break;
		case Qt::Key_Zenkaku: key = XK_Zenkaku; break;
		case Qt::Key_Hankaku: key = XK_Hankaku; break;
		case Qt::Key_Zenkaku_Hankaku: key = XK_Zenkaku_Hankaku; break;
		case Qt::Key_Touroku: key = XK_Touroku; break;
		case Qt::Key_Massyo: key = XK_Massyo; break;
		case Qt::Key_Kana_Lock: key = XK_Kana_Lock; break;
		case Qt::Key_Kana_Shift: key = XK_Kana_Shift; break;
		case Qt::Key_Eisu_Shift: key = XK_Eisu_Shift; break;
		case Qt::Key_Eisu_toggle: key = XK_Eisu_toggle; break;
		case Qt::Key_Hangul: key = XK_Hangul; break;
		case Qt::Key_Hangul_Start: key = XK_Hangul_Start; break;
		case Qt::Key_Hangul_End: key = XK_Hangul_End; break;
		case Qt::Key_Hangul_Hanja: key = XK_Hangul_Hanja; break;
		case Qt::Key_Hangul_Jamo: key = XK_Hangul_Jamo; break;
		case Qt::Key_Hangul_Romaja: key = XK_Hangul_Romaja; break;
		case Qt::Key_Hangul_Jeonja: key = XK_Hangul_Jeonja; break;
		case Qt::Key_Hangul_Banja: key = XK_Hangul_Banja; break;
		case Qt::Key_Hangul_PreHanja: key = XK_Hangul_PreHanja; break;
		case Qt::Key_Hangul_PostHanja: key = XK_Hangul_PostHanja; break;
		case Qt::Key_Hangul_Special: key = XK_Hangul_Special; break;
		case Qt::Key_Dead_Grave: key = XK_dead_grave; break;
		case Qt::Key_Dead_Acute: key = XK_dead_acute; break;
		case Qt::Key_Dead_Circumflex: key = XK_dead_circumflex; break;
		case Qt::Key_Dead_Tilde: key = XK_dead_tilde; break;
		case Qt::Key_Dead_Macron: key = XK_dead_macron; break;
		case Qt::Key_Dead_Breve: key = XK_dead_breve; break;
		case Qt::Key_Dead_Abovedot: key = XK_dead_abovedot; break;
		case Qt::Key_Dead_Diaeresis: key = XK_dead_diaeresis; break;
		case Qt::Key_Dead_Abovering: key = XK_dead_abovering; break;
		case Qt::Key_Dead_Doubleacute: key = XK_dead_doubleacute; break;
		case Qt::Key_Dead_Caron: key = XK_dead_caron; break;
		case Qt::Key_Dead_Cedilla: key = XK_dead_cedilla; break;
		case Qt::Key_Dead_Ogonek: key = XK_dead_ogonek; break;
		case Qt::Key_Dead_Iota: key = XK_dead_iota; break;
		case Qt::Key_Dead_Voiced_Sound: key = XK_dead_voiced_sound; break;
		case Qt::Key_Dead_Semivoiced_Sound: key = XK_dead_semivoiced_sound; break;
		case Qt::Key_Dead_Belowdot: key = XK_dead_belowdot; break;
	}

	if( _ke->key() >= Qt::Key_F1 && _ke->key() <= Qt::Key_F35 )
	{
		key = XK_F1 + _ke->key() - Qt::Key_F1;
	}
	else if( key == 0 )
	{
		key = _ke->text().utf16()[0];//[0].toAscii();
	}
	if( key != 0 )
	{
		pressMods( QApplication::keyboardModifiers(), true );
		emit keyEvent( key, true );
		emit keyEvent( key, false );
		pressMods( QApplication::keyboardModifiers(), false );
		_ke->accept();
	}
}




void vncView::mouseMoveEvent( QMouseEvent * _me )
{
	mouseEvent( _me );
	_me->ignore();
}




void vncView::mousePressEvent( QMouseEvent * _me )
{
	mouseEvent( _me );
	_me->accept();
}




void vncView::mouseReleaseEvent( QMouseEvent * _me )
{
	mouseEvent( _me );
	_me->accept();
}




void vncView::mouseDoubleClickEvent( QMouseEvent * _me )
{
	mouseEvent( _me );
	_me->accept();
}





void vncView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

	// avoid nasty through-shining-window-effect when not connected yet
	if( m_connection->screen().isNull() )
	{
		p.fillRect( rect(), Qt::black );
		return;
	}

	// only paint requested region of image
	p.drawImage( _pe->rect().topLeft(), m_connection->screen(),
			_pe->rect().translated( m_viewOffset ),
			Qt::ThresholdDither );

	const QPixmap & cursor = m_connection->cursorShape();
	const QRect cursor_rect = QRect( m_connection->cursorPos() -
						m_connection->cursorHotSpot(),
					cursor.size() ).translated(
								-m_viewOffset );
	// parts of cursor within updated region?
	if( _pe->rect().intersects( cursor_rect ) )
	{
		// then repaint it
		p.drawPixmap( cursor_rect.topLeft(), cursor );
	}

	// draw black borders if neccessary
	const int fbw = m_connection->framebufferSize().width();
	if( fbw < width() )
	{
		p.fillRect( fbw, 0, width() - fbw, height(), Qt::black );
	}
	const int fbh = m_connection->framebufferSize().height();
	if( fbh < height() )
	{
		p.fillRect( 0, fbh, fbw, height() - fbh, Qt::black );
	}
}




void vncView::resizeEvent( QResizeEvent * _re )
{
	const int max_x = m_connection->framebufferSize().width() - width();
	const int max_y = m_connection->framebufferSize().height() - height();
	if( m_viewOffset.x() > max_x || m_viewOffset.y() > max_y )
	{
		m_viewOffset = QPoint(
				qMax( 0, qMin( m_viewOffset.x(), max_x ) ),
				qMax( 0, qMin( m_viewOffset.y(), max_y ) ) );
		update();
	}

	m_establishingConnection->move( 10,
			height() - 10 - m_establishingConnection->height() );

	QWidget::resizeEvent( _re );
}




void vncView::wheelEvent( QWheelEvent * _we )
{
	emit pointerEvent( _we->x(), _we->y(), m_buttonMask |
		( ( _we->delta() < 0 ) ? rfbButton5Mask : rfbButton4Mask ) );
	emit pointerEvent( _we->x(), _we->y(), m_buttonMask );
	
	_we->accept();
}




void vncView::customEvent( QEvent * _e )
{
	if( _e->type() == regionChangedEvent().type() )
	{
		QWidget::update( dynamic_cast<regionChangedEvent *>( _e )->
			changedRegion().boundingRect().translated(
							-m_viewOffset ) );
		_e->accept();
	}
	else
	{
		QWidget::customEvent( _e );
	}
}




void vncView::pressMods( Qt::KeyboardModifiers _mod, bool _pressed )
{
	struct modifierXlate
	{
		Qt::KeyboardModifier qt;
		int x;
	} const modmap[] =
		{
			{ Qt::ShiftModifier, XK_Shift_L },
			{ Qt::ControlModifier, XK_Control_L },
			{ Qt::AltModifier, XK_Alt_L },
			{ Qt::MetaModifier, XK_Meta_L }
		} ;

	for( Q_UINT8 i = 0; i < sizeof( modmap )/sizeof( modifierXlate ); ++i )
	{
		if( _mod & modmap[i].qt )
		{
			emit keyEvent( modmap[i].x, _pressed );
		}
	}
}




void vncView::mouseEvent( QMouseEvent * _me )
{
	struct buttonXlate
	{
		Qt::MouseButton qt;
		int rfb;
	} const map[] =
		{
			{ Qt::LeftButton, rfbButton1Mask },
			{ Qt::MidButton, rfbButton2Mask },
			{ Qt::RightButton, rfbButton3Mask }
		} ;

	if( _me->type() != QEvent::MouseMove )
	{
		for( Q_UINT8 i = 0; i < sizeof(map)/sizeof(buttonXlate); ++i )
		{
			if( _me->button() == map[i].qt )
			{
				if( _me->type() == QEvent::MouseButtonPress ||
				_me->type() == QEvent::MouseButtonDblClick )
				{
					m_buttonMask |= map[i].rfb;
				}
				else
				{
					m_buttonMask&=~map[i].rfb;
				}
			}
		}
	}

	emit pointerEvent( _me->x() + m_viewOffset.x(),
				_me->y() + m_viewOffset.y(), m_buttonMask );
}



#include "vncview.moc"
