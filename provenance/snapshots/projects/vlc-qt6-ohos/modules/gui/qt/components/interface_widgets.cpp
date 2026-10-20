/*****************************************************************************
 * interface_widgets.cpp : Custom widgets for the main interface
 ****************************************************************************
 * Copyright (C) 2006-2010 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Rafaël Carré <funman@videolanorg>
 *          Ilkka Ollakka <ileoo@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"
#include "components/interface_widgets.hpp"
#ifdef __OHOS__
# include "components/ohos_subtitle_bridge.hpp"
# include "components/ohos_video_texture_widget.hpp"
#endif
#include "dialogs_provider.hpp"
#include "util/customwidgets.hpp"               // qtEventToVLCKey, QVLCStackedWidget

#include "menus.hpp"             /* Popup menu on bgWidget */

#include <QDate>
#include <QLabel>
#include <QToolButton>
#include <QPalette>
#include <QEvent>
#include <QResizeEvent>
#include <QDate>
#include <QMenu>
#include <QWidgetAction>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>
#include <QSlider>
#include <QBitmap>
#include <QUrl>
#ifdef __OHOS__
# include <QEventLoop>
# include <QGuiApplication>
# include <QMetaObject>
# include <QMutexLocker>
# include <QStackedWidget>
# include <QWindow>
# include <functional>
# include <optional>
# include <QtGui/qpa/qplatformnativeinterface.h>
#endif

#if defined (QT5_HAS_X11)
# include <X11/Xlib.h>
# include <QX11Info>
# if defined(QT5_HAS_XCB)
#  include <xcb/xproto.h>
# endif
#endif
#ifdef QT5_HAS_WAYLAND
# include QPNI_HEADER
# include <QWindow>
#endif

#if defined(_WIN32)
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#endif

#include <math.h>
#include <assert.h>
#include <cstring>

#include <vlc_vout.h>
#include <vlc_vout_window.h>
#ifdef __OHOS__
# include <native_window/external_window.h>
#endif

/**********************************************************************
 * Video Widget. A simple frame on which video is drawn
 * This class handles resize issues
 **********************************************************************/

VideoWidget::VideoWidget( intf_thread_t *_p_i, QWidget* p_parent )
            : QFrame( p_parent ) , p_intf( _p_i )
{
    /* Set the policy to expand in both directions */
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMinimumSize( 0, 0 );

    layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    stable = NULL;
    p_window = NULL;
    show();
#ifdef __OHOS__
    surfaceWindow = NULL;
    nativeWindow = NULL;
    textureWindow = NULL;
    softwareVideoOutput = usesSoftwareVideoOutput();
    hardwareTextureOutput = usesHardwareTextureOutput();
    if( hardwareTextureOutput )
        registerSubtitleBridge();
    if( softwareVideoOutput )
    {
        setAttribute( Qt::WA_OpaquePaintEvent, true );
        registerSoftwareVideoOutput();
    }
    else if( !hardwareTextureOutput )
        ensureNativeSurface();
#endif
}

VideoWidget::~VideoWidget()
{
    /* Ensure we are not leaking the video output. This would crash. */
#ifdef __OHOS__
    surfaceConnection.reset();
    if( stable )
        delete stable;
    stable = NULL;
    surfaceWindow = NULL;
#else
    assert( !stable );
#endif
    assert( !p_window );
}

#ifdef __OHOS__
bool VideoWidget::usesSoftwareVideoOutput() const
{
    const QByteArray backend = qgetenv( "VLC_OHOS_VIDEO_BACKEND" );
    return backend == QByteArrayLiteral("software-vmem");
}

bool VideoWidget::usesHardwareTextureOutput() const
{
    const QByteArray backend = qgetenv( "VLC_OHOS_VIDEO_BACKEND" );
    return backend.isEmpty() || backend == QByteArrayLiteral("hardware-texture");
}

void VideoWidget::registerSoftwareVideoOutput()
{
    vlc_object_t *objects[] = {
        VLC_OBJECT( p_intf->obj.libvlc ),
        VLC_OBJECT( THEPL )
    };
    for( vlc_object_t *object : objects )
    {
        /* "window" is an internal inherited variable, not a command-line
         * configuration option. Set it only after libvlc_new() has created
         * the object tree; passing --window=none makes libvlc_new fail. */
        var_Create( object, "window", VLC_VAR_STRING );
        var_SetString( object, "window", "none" );
        var_Create( object, "vout", VLC_VAR_STRING );
        var_SetString( object, "vout", "vmem" );
        var_Create( object, "avcodec-hw", VLC_VAR_STRING );
        var_SetString( object, "avcodec-hw", "none" );
        var_Create( object, "vmem-setup", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-setup",
                        reinterpret_cast<void *>( &VideoWidget::vmemSetup ) );
        var_Create( object, "vmem-cleanup", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-cleanup",
                        reinterpret_cast<void *>( &VideoWidget::vmemCleanup ) );
        var_Create( object, "vmem-lock", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-lock",
                        reinterpret_cast<void *>( &VideoWidget::vmemLock ) );
        var_Create( object, "vmem-unlock", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-unlock",
                        reinterpret_cast<void *>( &VideoWidget::vmemUnlock ) );
        var_Create( object, "vmem-display", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-display",
                        reinterpret_cast<void *>( &VideoWidget::vmemDisplay ) );
        var_Create( object, "vmem-data", VLC_VAR_ADDRESS );
        var_SetAddress( object, "vmem-data", this );
    }
    msg_Warn( p_intf, "VLC_OHOS_VMEM software video callbacks registered" );
}

unsigned VideoWidget::vmemSetup( void **opaque, char *chroma,
                                 unsigned *width, unsigned *height,
                                 unsigned *pitches, unsigned *lines )
{
    VideoWidget *widget = static_cast<VideoWidget *>( *opaque );
    if( !widget || !width || !height || *width == 0 || *height == 0 )
        return 0;

    std::memcpy( chroma, "RV32", 4 );
    QMutexLocker locker( &widget->softwareFrameMutex );
    widget->softwareFrame = QImage( static_cast<int>( *width ),
                                    static_cast<int>( *height ),
                                    QImage::Format_RGB32 );
    if( widget->softwareFrame.isNull() )
        return 0;
    pitches[0] = static_cast<unsigned>( widget->softwareFrame.bytesPerLine() );
    lines[0] = *height;
    msg_Warn( widget->p_intf, "VLC_OHOS_VMEM setup frame=%ux%u pitch=%u",
              *width, *height, pitches[0] );
    return 1;
}

void VideoWidget::vmemCleanup( void *opaque )
{
    VideoWidget *widget = static_cast<VideoWidget *>( opaque );
    if( !widget )
        return;
    QMutexLocker locker( &widget->softwareFrameMutex );
    widget->softwareFrame = QImage();
    QMetaObject::invokeMethod( widget, "update", Qt::QueuedConnection );
}

void *VideoWidget::vmemLock( void *opaque, void **planes )
{
    VideoWidget *widget = static_cast<VideoWidget *>( opaque );
    if( !widget )
        return NULL;
    widget->softwareFrameMutex.lock();
    *planes = widget->softwareFrame.bits();
    return NULL;
}

void VideoWidget::vmemUnlock( void *opaque, void *, void *const * )
{
    VideoWidget *widget = static_cast<VideoWidget *>( opaque );
    if( widget )
        widget->softwareFrameMutex.unlock();
}

void VideoWidget::vmemDisplay( void *opaque, void * )
{
    VideoWidget *widget = static_cast<VideoWidget *>( opaque );
    if( widget )
        QMetaObject::invokeMethod( widget, "presentSoftwareFrame",
                                   Qt::QueuedConnection );
}

void VideoWidget::presentSoftwareFrame()
{
    QStackedWidget *stack = qobject_cast<QStackedWidget *>( parentWidget() );
    if( stack && stack->currentWidget() != this )
        stack->setCurrentWidget( this );
    update();
}

bool VideoWidget::ensureHardwareTextureSurface( const QSize &videoSize )
{
    if( textureWindow && nativeWindow )
    {
        textureWindow->setVideoSize( videoSize );
        return true;
    }

    if( !textureWindow )
    {
        /* QOpenGLWidget creates an extra top-level QWindow on OHOS, which
         * makes Qt start a second Ability and abort on devices without
         * multi-instance support. Embed a QOpenGLWindow as a child first,
        * following the proven native-surface container path. */
        textureWindow = new OhosVideoTextureWindow;
        textureWindow->setVideoSize( videoSize );
        textureWindow->setFlags( Qt::FramelessWindowHint );
        /* The OHOS platform plugin uses this marker for embedded video
         * windows whose ArkUI node must track the native container instead
         * of retaining the point geometry used during construction. */
        textureWindow->setProperty( "_q_vlc_ohos_parent_fill", true );
        surfaceWindow = textureWindow;
        stable = QWidget::createWindowContainer( textureWindow, this );
        stable->setContextMenuPolicy( Qt::PreventContextMenu );
        stable->setAutoFillBackground( false );
        stable->setAttribute( Qt::WA_NativeWindow, true );
        connect( textureWindow, &OhosVideoTextureWindow::nativeWindowReady,
                 this, [this]( void *window ) {
            nativeWindow = window;
            registerNativeSurface( window );
        } );
        connect( textureWindow, &OhosVideoTextureWindow::firstFramePresented,
                 this, [this]() {
            /* TODO: The first OHOSAVCodec session attached to a fresh
             * NativeImage can render a washed-out/cropped stream, while a
             * second codec session on the same surface is correct. Until the
             * producer/consumer color metadata transition is fixed upstream,
             * warm the process-wide texture surface with one automatic
             * stop/play cycle after its first consumed frame. */
            static bool firstSessionRestarted = false;
            if( firstSessionRestarted
                || qgetenv( "VLC_OHOS_TEXTURE_FIRST_OPEN_WARMUP" ) == "0" )
                return;
            firstSessionRestarted = true;
            msg_Warn( p_intf,
                      "VLC_OHOS_TEXTURE restarting first playback after surface warm-up" );
            QTimer::singleShot( 0, this, [this]() {
                playlist_Lock( THEPL );
                playlist_item_t *current = playlist_CurrentPlayingItem( THEPL );
                if( current )
                    playlist_ViewPlay( THEPL, nullptr, current );
                playlist_Unlock( THEPL );
            } );
        } );
        layout->addWidget( stable );
        layout->activate();
    }
    else
        textureWindow->setVideoSize( videoSize );

    /* QOpenGLWindow creates its context on first exposure. The video page is
     * normally still behind the background page while VLC requests a window,
     * so expose it now to create the off-screen decoder surface. */
    QStackedWidget *stack = qobject_cast<QStackedWidget *>( parentWidget() );
    if( stack && stack->currentWidget() != this )
        stack->setCurrentWidget( this );
    show();
    stable->show();

    /* createWindowContainer can expose its QWindow before the stack/layout
     * has delivered the first resize on OHOS. A 0x0 EGL window never reaches
     * initializeGL(), so the NativeImage decoder surface is never created.
     * Seed a real child geometry before the first exposure; subsequent sizes
     * are kept in sync by QWindowContainer and resizeEvent(). */
    layout->activate();
    QSize bootstrapSize = stable->size();
    if( !bootstrapSize.isValid() || bootstrapSize.isEmpty() )
        bootstrapSize = size();
    if( !bootstrapSize.isValid() || bootstrapSize.isEmpty() )
        bootstrapSize = QSize( 16, 16 );
    textureWindow->setGeometry( QRect( QPoint( 0, 0 ), bootstrapSize ) );
    msg_Warn( p_intf,
              "VLC_OHOS_TEXTURE bootstrap video=%dx%d stable=%dx%d gl=%dx%d",
              width(), height(), stable->width(), stable->height(),
              textureWindow->width(), textureWindow->height() );
    textureWindow->create();
    textureWindow->show();
    textureWindow->update();

    if( !nativeWindow )
    {
        QEventLoop waitForSurface;
        QTimer::singleShot( 1500, &waitForSurface, &QEventLoop::quit );
        QTimer poll;
        connect( &poll, &QTimer::timeout, &waitForSurface, [&]() {
            if( nativeWindow ) waitForSurface.quit();
        } );
        poll.start( 10 );
        waitForSurface.exec();
    }

    if( nativeWindow )
        msg_Warn( p_intf, "VLC_OHOS_TEXTURE decoder window ready: %p", nativeWindow );
    else
        msg_Err( p_intf, "VLC_OHOS_TEXTURE failed to create decoder window" );
    return nativeWindow != NULL;
}

void VideoWidget::logOhosGeometry( const char *stage ) const
{
    const QWidget *topWidget = window();
    const QRect topGeometry = topWidget ? topWidget->geometry() : QRect();
    const QRect videoGeometry = geometry();
    const QRect stableGeometry = stable ? stable->geometry() : QRect();
    const QPoint stableInTop = stable && topWidget
        ? stable->mapTo( const_cast<QWidget *>( topWidget ), QPoint( 0, 0 ) )
        : QPoint();
    const QWindow *stableWindow = stable ? stable->windowHandle() : NULL;
    const QRect stableWindowGeometry = stableWindow ? stableWindow->geometry() : QRect();
    const QRect surfaceGeometry = surfaceWindow ? surfaceWindow->geometry() : QRect();
    const QWindow *surfaceParent = surfaceWindow
        ? qobject_cast<const QWindow *>( surfaceWindow->parent() ) : NULL;
    const QRect surfaceParentGeometry = surfaceParent ? surfaceParent->geometry() : QRect();
    const QSize reportedSize = physicalSize();

    msg_Warn( p_intf,
              "VLC_OHOS_GEOMETRY vlc stage=%s top=%d,%d,%d,%d "
              "video=%d,%d,%d,%d stable=%d,%d,%d,%d stableInTop=%d,%d "
              "stableWindow=%p:%d,%d,%d,%d surface=%p:%d,%d,%d,%d "
              "surfaceParent=%p:%d,%d,%d,%d reported=%d,%d dpr=%.4f",
              stage,
              topGeometry.x(), topGeometry.y(), topGeometry.width(), topGeometry.height(),
              videoGeometry.x(), videoGeometry.y(), videoGeometry.width(), videoGeometry.height(),
              stableGeometry.x(), stableGeometry.y(), stableGeometry.width(), stableGeometry.height(),
              stableInTop.x(), stableInTop.y(),
              static_cast<const void *>( stableWindow ),
              stableWindowGeometry.x(), stableWindowGeometry.y(),
              stableWindowGeometry.width(), stableWindowGeometry.height(),
              static_cast<const void *>( surfaceWindow ),
              surfaceGeometry.x(), surfaceGeometry.y(),
              surfaceGeometry.width(), surfaceGeometry.height(),
              static_cast<const void *>( surfaceParent ),
              surfaceParentGeometry.x(), surfaceParentGeometry.y(),
              surfaceParentGeometry.width(), surfaceParentGeometry.height(),
              reportedSize.width(), reportedSize.height(), devicePixelRatioF() );
}

void VideoWidget::registerNativeSurface( void *surface )
{
    if( !surface )
        return;
    nativeWindow = surface;
    const int32_t scalingResult = hardwareTextureOutput ? 0
        : OH_NativeWindow_NativeWindowSetScalingModeV2(
            static_cast<OHNativeWindow *>( nativeWindow ),
            OH_SCALING_MODE_SCALE_TO_WINDOW_V2 );
    if( scalingResult != 0 )
        msg_Warn( p_intf, "failed to set OHOS surface scale-to-window mode: %d", scalingResult );
    vlc_object_t *rootObject = VLC_OBJECT( p_intf->obj.libvlc );
    vlc_object_t *playlistObject = VLC_OBJECT( THEPL );
    var_Create( rootObject, "drawable-ohos-nativewindow-ptr", VLC_VAR_ADDRESS );
    var_SetAddress( rootObject, "drawable-ohos-nativewindow-ptr", nativeWindow );
    var_Create( playlistObject, "drawable-ohos-nativewindow-ptr", VLC_VAR_ADDRESS );
    var_SetAddress( playlistObject, "drawable-ohos-nativewindow-ptr", nativeWindow );
    if( p_window )
        p_window->handle.onativewindow = nativeWindow;
    msg_Warn( p_intf, "registered OHOS decoder window %p backend=%s result=%d",
              nativeWindow,
              hardwareTextureOutput ? "hardware-texture" : "native-surface",
              scalingResult );
    logOhosGeometry( "surface-registered" );
}

void VideoWidget::registerSubtitleBridge()
{
    const vlc_ohos_subtitle_bridge_t *subtitleBridge =
        OhosSubtitleBridgeInstance();
    vlc_object_t *objects[] = {
        VLC_OBJECT( p_intf->obj.libvlc ),
        VLC_OBJECT( THEPL )
    };
    for( vlc_object_t *object : objects )
    {
        var_Create( object, VLC_OHOS_SUBTITLE_BRIDGE_VAR,
                    VLC_VAR_ADDRESS );
        var_SetAddress( object, VLC_OHOS_SUBTITLE_BRIDGE_VAR,
                        const_cast<vlc_ohos_subtitle_bridge_t *>(
                            subtitleBridge ) );
    }
    msg_Warn( p_intf,
              "VLC_OHOS_SUBTITLE RGBA bridge registered early abi=%u address=%p",
              subtitleBridge->abi_version,
              static_cast<const void *>( subtitleBridge ) );
}

bool VideoWidget::ensureNativeSurface()
{
    if( stable && nativeWindow )
        return true;

    surfaceWindow = new QWindow;
    surfaceWindow->setFlags( Qt::FramelessWindowHint );
    /* Consumed by the patched OHOS platform plugin. This surface is the
     * content of a native QWidget container and must always fill that native
     * parent; ordinary embedded QWindows keep the stock point geometry. */
    surfaceWindow->setProperty( "_q_vlc_ohos_parent_fill", true );
    stable = QWidget::createWindowContainer( surfaceWindow, this );
    stable->setContextMenuPolicy( Qt::PreventContextMenu );
    stable->setAutoFillBackground( false );
    stable->setAttribute( Qt::WA_NativeWindow, true );
    stable->setMouseTracking( true );
    setMouseTracking( true );
    layout->addWidget( stable );
    layout->activate();

    /* QWindowContainer owns the embedded window geometry. In particular,
     * stable is a native widget, so surfaceWindow is parented to stable's
     * QWindow and its geometry must be relative to that parent. Setting a
     * top-level QWidget position here offsets the ArkUI node a second time
     * and also races QWindowContainer's resize handling. */
    surfaceWindow->create();

    using SetSurfaceConsumer = std::shared_ptr<void> (*)(
        QWindow *, QObject *, std::function<void(std::optional<void *>)> );
    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    const auto platformFunction = nativeInterface
        ? nativeInterface->platformFunction( QByteArrayLiteral("setSurfaceConsumer") )
        : NULL;
    if( !platformFunction )
        return false;
    const auto setSurfaceConsumer = reinterpret_cast<SetSurfaceConsumer>( platformFunction );
    surfaceConnection = setSurfaceConsumer(
        surfaceWindow, this, [this]( std::optional<void *> surface ) {
            registerNativeSurface( surface.value_or( nullptr ) );
        } );
    if( !surfaceConnection )
        return false;

    if( !nativeWindow )
    {
        QEventLoop waitForSurface;
        QTimer::singleShot( 1000, &waitForSurface, &QEventLoop::quit );
        QTimer poll;
        connect( &poll, &QTimer::timeout, &waitForSurface, [&]() {
            if( nativeWindow ) waitForSurface.quit();
        } );
        poll.start( 10 );
        waitForSurface.exec();
    }
    if( !nativeWindow )
        return false;

    registerNativeSurface( nativeWindow );
    return true;
}

#endif

void VideoWidget::sync( void )
{
    /* Make sure the X server has processed all requests.
     * This protects other threads using distinct connections from getting
     * the video widget window in an inconsistent states. */
#ifdef QT5_HAS_X11
    if( QX11Info::isPlatformX11() )
        XSync( QX11Info::display(), False );
#endif
}

/**
 * Request the video to avoid the conflicts
 **/
bool VideoWidget::request( struct vout_window_t *p_wnd,
                           unsigned videoWidth, unsigned videoHeight )
{
#ifndef __OHOS__
    Q_UNUSED( videoWidth );
    Q_UNUSED( videoHeight );
#endif
    if( p_window )
    {
        msg_Dbg( p_intf, "embedded video already in use" );
        return false;
    }
    assert( !p_window );

    /* The owner of the video window needs a stable handle (WinId). Reparenting
     * in Qt4-X11 changes the WinId of the widget, so we need to create another
     * dummy widget that stays within the reparentable widget. */
#ifdef __OHOS__
    if( softwareVideoOutput )
        return false;
    if( hardwareTextureOutput )
    {
        if( !ensureHardwareTextureSurface(
                QSize( static_cast<int>( videoWidth ),
                       static_cast<int>( videoHeight ) ) ) )
            return false;
    }
    else if( !ensureNativeSurface() )
        return false;
#else
    stable = new QWidget();
#endif
#ifndef __OHOS__
    stable->setContextMenuPolicy( Qt::PreventContextMenu );
    QPalette plt = palette();
    plt.setColor( QPalette::Window, Qt::black );
    stable->setPalette( plt );
    stable->setAutoFillBackground(true);
    /* Force the widget to be native so that it gets a winId() */
    stable->setAttribute( Qt::WA_NativeWindow, true );
    /* Indicates that the widget wants to draw directly onto the screen.
       Widgets with this attribute set do not participate in composition
       management */
    /* This is currently disabled on X11 as it does not seem to improve
     * performance, but causes the video widget to be transparent... */
#if !defined (QT5_HAS_X11)
    stable->setAttribute( Qt::WA_PaintOnScreen, true );
#else
    stable->setMouseTracking( true );
    setMouseTracking( true );
#endif
    layout->addWidget( stable );
#endif

    sync();
    p_window = p_wnd;

    p_wnd->type = p_intf->p_sys->voutWindowType;
    switch( p_wnd->type )
    {
        case VOUT_WINDOW_TYPE_XID:
            p_wnd->handle.xid = stable->winId();
            p_wnd->display.x11 = NULL;
            break;
        case VOUT_WINDOW_TYPE_HWND:
            p_wnd->handle.hwnd = (void *)stable->winId();
            break;
        case VOUT_WINDOW_TYPE_NSOBJECT:
            p_wnd->handle.nsobject = (void *)stable->winId();
            break;
#ifdef __OHOS__
        case VOUT_WINDOW_TYPE_OHOS_NATIVE:
            p_wnd->handle.onativewindow = nativeWindow;
            break;
#endif
#ifdef QT5_HAS_WAYLAND
        case VOUT_WINDOW_TYPE_WAYLAND:
        {
            /* Ensure only the video widget is native (needed for Wayland) */
            stable->setAttribute( Qt::WA_DontCreateNativeAncestors, true);

            QWindow *window = stable->windowHandle();
            assert(window != NULL);
            window->create();

            QPlatformNativeInterface *qni = qApp->platformNativeInterface();
            assert(qni != NULL);

            p_wnd->handle.wl = static_cast<wl_surface*>(
                qni->nativeResourceForWindow(QByteArrayLiteral("surface"),
                                             window));
            p_wnd->display.wl = static_cast<wl_display*>(
                qni->nativeResourceForIntegration(QByteArrayLiteral("wl_display")));
            break;
        }
#endif
        default:
            vlc_assert_unreachable();
    }

    enable_mouse_events = var_InheritBool(p_window, "mouse-events");
    return true;
}

QPaintEngine *VideoWidget::paintEngine() const
{
#ifdef __OHOS__
    if( softwareVideoOutput || hardwareTextureOutput )
        return QFrame::paintEngine();
#endif
    return NULL;
}

void VideoWidget::paintEvent( QPaintEvent *event )
{
#ifdef __OHOS__
    if( softwareVideoOutput )
    {
        Q_UNUSED( event );
        QPainter painter( this );
        painter.fillRect( rect(), Qt::black );
        QMutexLocker locker( &softwareFrameMutex );
        if( softwareFrame.isNull() )
            return;
        const QSize targetSize = softwareFrame.size().scaled(
            size(), Qt::KeepAspectRatio );
        const QRect targetRect(
            QPoint( (width() - targetSize.width()) / 2,
                    (height() - targetSize.height()) / 2 ),
            targetSize );
        painter.setRenderHint( QPainter::SmoothPixmapTransform, true );
        painter.drawImage( targetRect, softwareFrame );
        return;
    }
#endif
    QFrame::paintEvent( event );
}

QSize VideoWidget::physicalSize() const
{
#ifdef QT5_HAS_X11
    if ( QX11Info::isPlatformX11() )
    {
        Display *p_x_display = QX11Info::display();
        Window x_window = stable->winId();
        XWindowAttributes x_attributes;

        XGetWindowAttributes( p_x_display, x_window, &x_attributes );

        return QSize( x_attributes.width, x_attributes.height );
    }
#endif
#if defined(_WIN32)
    HWND hwnd;
    RECT rect;

    QWindow *window = windowHandle();
    hwnd = static_cast<HWND>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window));

    GetClientRect(hwnd, &rect);

    return QSize( rect.right, rect.bottom );
#endif

    QSize current_size = size();

#   if HAS_QT56
    /* Android-like scaling */
    current_size *= devicePixelRatioF();
#   else
    /* OSX-like scaling */
    current_size *= devicePixelRatio();
#   endif

    return current_size;
}

void WindowResized(vout_window_t *, const QSize&);
void WindowOrphaned(vout_window_t *);

void VideoWidget::reportSize()
{
    if( !p_window )
        return;
#ifdef __OHOS__
    if( hardwareTextureOutput )
        return;
#endif

    QSize size = physicalSize();
#ifdef __OHOS__
    logOhosGeometry( "report-size" );
#endif
    WindowResized(p_window, size);
}

/* Set the Widget to the correct Size */
/* Function has to be called by the parent
   Parent has to care about resizing itself */
void VideoWidget::setSize( unsigned int w, unsigned int h )
{
#ifdef __OHOS__
    if( hardwareTextureOutput && textureWindow )
        textureWindow->setVideoSize( QSize( static_cast<int>( w ),
                                            static_cast<int>( h ) ) );
    /* This widget is a page owned by QStackedWidget. Resizing the page itself
     * leaves it at the source-video size when an OHOS top-level resize is
     * asynchronous or rejected. Ask MainInterface to resize the top level and
     * let the stack layout keep this page fitted to its actual contents rect. */
    emit sizeChanged( w, h );
    logOhosGeometry( "set-size-request" );
    if( !hardwareTextureOutput )
        reportSize();
    return;
#endif

    /* If the size changed, resizeEvent will be called, otherwise not,
     * in which case we need to tell the vout what the size actually is
     */
    if( (unsigned)size().width() == w && (unsigned)size().height() == h )
    {
        reportSize();
        return;
    }

    resize( w, h );
    emit sizeChanged( w, h );
    /* Work-around a bug?misconception? that would happen when vout core resize
       twice to the same size and would make the vout not centered.
       This cause a small flicker.
       See #3621
     */
    if( (unsigned)size().width() == w && (unsigned)size().height() == h )
        updateGeometry();
    sync();
}

bool VideoWidget::nativeEvent( const QByteArray& eventType, void* message, qintptr* )
{
#if defined(QT5_HAS_X11)
# if defined(QT5_HAS_XCB)
    if ( eventType == "xcb_generic_event_t" )
    {
        const xcb_generic_event_t* xev = static_cast<const xcb_generic_event_t*>( message );

        if ( xev->response_type == XCB_CONFIGURE_NOTIFY )
            reportSize();
    }
# endif
#endif
#ifdef _WIN32
    if ( eventType == "windows_generic_MSG" )
    {
        MSG* msg = static_cast<MSG*>( message );
        if ( msg->message == WM_SIZE )
            reportSize();
    }
#endif
    // Let Qt handle that event in any case
    return false;
}

void VideoWidget::resizeEvent( QResizeEvent *event )
{
    QWidget::resizeEvent( event );

#ifdef __OHOS__
    if( hardwareTextureOutput )
    {
        if( textureWindow && event->size().isValid() && !event->size().isEmpty() )
            textureWindow->setGeometry( QRect( QPoint( 0, 0 ), event->size() ) );
        return;
    }
#endif
    if ( p_intf->p_sys->voutWindowType == VOUT_WINDOW_TYPE_XID ||
        p_intf->p_sys->voutWindowType == VOUT_WINDOW_TYPE_HWND )
        return;
    reportSize();
}

int VideoWidget::qtMouseButton2VLC( Qt::MouseButton qtButton )
{
    if( p_window == NULL )
        return -1;
    switch( qtButton )
    {
        case Qt::LeftButton:
            return 0;
        case Qt::RightButton:
            return 2;
        case Qt::MiddleButton:
            return 1;
        default:
            return -1;
    }
}

void VideoWidget::mouseReleaseEvent( QMouseEvent *event )
{
    if ( !p_window || !enable_mouse_events )
    {
        event->ignore();
        return;
    }

    int vlc_button = qtMouseButton2VLC( event->button() );
    if( vlc_button >= 0 )
    {
        vout_window_ReportMouseReleased( p_window, vlc_button );
        event->accept();
        return;
    }

    event->ignore();
}

void VideoWidget::mousePressEvent( QMouseEvent* event )
{
    if ( !p_window || !enable_mouse_events )
    {
        event->ignore();
        return;
    }

    int vlc_button = qtMouseButton2VLC( event->button() );
    if( vlc_button >= 0 )
    {
        vout_window_ReportMousePressed( p_window, vlc_button );
        event->accept();
        return;
    }

    event->ignore();
}

void VideoWidget::mouseMoveEvent( QMouseEvent *event )
{
    if ( !p_window || !enable_mouse_events )
    {
        event->ignore();
        return;
    }

    {
        QPointF current_pos = event->position();

#if HAS_QT56
        current_pos *= devicePixelRatioF();
#else
        current_pos *= devicePixelRatio();
#endif
        vout_window_ReportMouseMoved( p_window, current_pos.x(), current_pos.y() );
        event->accept();
        return;
    }

    event->ignore();
}

void VideoWidget::mouseDoubleClickEvent( QMouseEvent *event )
{
    if ( !p_window || !enable_mouse_events )
    {
        event->ignore();
        return;
    }

    int vlc_button = qtMouseButton2VLC( event->button() );
    if( vlc_button >= 0 )
    {
        vout_window_ReportMouseDoubleClick( p_window, vlc_button );
        event->accept();
        return;
    }

    event->ignore();
}


void VideoWidget::release( bool forced )
{
    msg_Dbg( p_intf, "video widget is %s", forced ? "orphaned" : "released" );

    if( stable )
    {
        if( forced )
            WindowOrphaned(p_window);
#ifdef __OHOS__
        p_window = NULL;
#else
        layout->removeWidget( stable );
        stable->deleteLater();
        stable = NULL;
        p_window = NULL;
#endif
    }

    updateGeometry();
}

/**********************************************************************
 * Background Widget. Show a simple image background. Currently,
 * it's album art if present or cone.
 **********************************************************************/

BackgroundWidget::BackgroundWidget( intf_thread_t *_p_i )
    :QWidget( NULL ), p_intf( _p_i ), b_expandPixmap( false ), b_withart( true )
{
    /* A dark background */
    setAutoFillBackground( true );
    QPalette plt = palette();
    plt.setColor( QPalette::Active, QPalette::Window , Qt::black );
    plt.setColor( QPalette::Inactive, QPalette::Window , Qt::black );
    setPalette( plt );

    /* Init the cone art */
    updateDefaultArt( ":/logo/vlc128.png" );
    updateArt( "" );

    /* fade in animator */
    setProperty( "opacity", 1.0 );
    fadeAnimation = new QPropertyAnimation( this, "opacity", this );
    fadeAnimation->setDuration( 1000 );
    fadeAnimation->setStartValue( 0.0 );
    fadeAnimation->setEndValue( 1.0 );
    fadeAnimation->setEasingCurve( QEasingCurve::OutSine );
    CONNECT( fadeAnimation, valueChanged( const QVariant & ),
             this, update() );

    CONNECT( THEMIM->getIM(), artChanged( QString ),
             this, updateArt( const QString& ) );
    CONNECT( THEMIM->getIM(), nameChanged( const QString& ),
             this, titleUpdated( const QString & ) );
}

void BackgroundWidget::updateArt( const QString& url )
{
    if ( !url.isEmpty() )
        pixmapUrl = url;
    else
        pixmapUrl = defaultArt;
    update();
}

void BackgroundWidget::updateDefaultArt( const QString& url )
{
    if ( !url.isEmpty() )
        defaultArt = url;
    update();
}

void BackgroundWidget::titleUpdated( const QString& title )
{
    /* don't ask */
    if( var_InheritBool( p_intf, "qt-icon-change" ) && !title.isEmpty() )
    {
        int i_pos = title.indexOf( "Ki" /* Bps */ "ll", 0, Qt::CaseInsensitive );
        if( i_pos != -1 &&
            i_pos + 5 == title.indexOf( "Bi" /* directional */ "ll",
                                       i_pos, Qt::CaseInsensitive ) )
                updateDefaultArt( ":/logo/vlc128-kb.png" );
        else if( QDate::currentDate().dayOfYear() >= QT_XMAS_JOKE_DAY )
                updateDefaultArt( ":/logo/vlc128-xmas.png" );
        else
                updateDefaultArt( ":/logo/vlc128.png" );
    }
}

void BackgroundWidget::showEvent( QShowEvent * e )
{
    Q_UNUSED( e );
    if ( b_withart ) fadeAnimation->start();
}

void BackgroundWidget::paintEvent( QPaintEvent *e )
{
    if ( !b_withart )
    {
        /* we just want background autofill */
        QWidget::paintEvent( e );
        return;
    }

    int i_maxwidth, i_maxheight;
    QPixmap pixmap = QPixmap( pixmapUrl );
    QPainter painter(this);

#if HAS_QT56
    qreal dpr = devicePixelRatioF();
#else
    qreal dpr = devicePixelRatio();
#endif
    pixmap.setDevicePixelRatio( dpr );

    i_maxwidth  = __MIN( maximumWidth(), width() ) - MARGIN * 2;
    i_maxheight = __MIN( maximumHeight(), height() ) - MARGIN * 2;

    painter.setOpacity( property( "opacity" ).toFloat() );

    if ( height() > MARGIN * 2 )
    {
        /* Scale down the pixmap if the widget is too small */
        if( pixmap.width() > i_maxwidth || pixmap.height() > i_maxheight )
        {
            pixmap = pixmap.scaled( i_maxwidth * dpr, i_maxheight * dpr ,
                            Qt::KeepAspectRatio, Qt::SmoothTransformation );
        }
        else
        if ( b_expandPixmap &&
             pixmap.width() < width() && pixmap.height() < height() )
        {
            pixmap = pixmap.scaled(
                    (width() - MARGIN * 2) * dpr,
                    (height() - MARGIN * 2) * dpr ,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        else if (dpr != 1.0)
        {
            pixmap = pixmap.scaled( pixmap.width() * dpr, pixmap.height() * dpr,
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation );
        }

        painter.drawPixmap(
                MARGIN + ( i_maxwidth - ( pixmap.width() / dpr ) ) / 2,
                MARGIN + ( i_maxheight - ( pixmap.height() / dpr ) ) / 2,
                pixmap);
    }
    QWidget::paintEvent( e );
}

void BackgroundWidget::contextMenuEvent( QContextMenuEvent *event )
{
    THEDP->setPopupMenu();
    event->accept();
}

EasterEggBackgroundWidget::EasterEggBackgroundWidget( intf_thread_t *p_intf )
    : BackgroundWidget( p_intf )
{
    flakes = new QList<flake *>();
    i_rate = 2;
    i_speed = 1;
    b_enabled = false;
    timer = new QTimer( this );
    timer->setInterval( 100 );
    CONNECT( timer, timeout(), this, spawnFlakes() );
    if ( isVisible() && b_enabled ) timer->start();
    defaultArt = QString( ":/logo/vlc128-xmas.png" );
    updateArt( "" );
}

EasterEggBackgroundWidget::~EasterEggBackgroundWidget()
{
    timer->stop();
    delete timer;
    reset();
    delete flakes;
}

void EasterEggBackgroundWidget::showEvent( QShowEvent *e )
{
    if ( b_enabled ) timer->start();
    BackgroundWidget::showEvent( e );
}

void EasterEggBackgroundWidget::hideEvent( QHideEvent *e )
{
    timer->stop();
    reset();
    BackgroundWidget::hideEvent( e );
}

void EasterEggBackgroundWidget::resizeEvent( QResizeEvent *e )
{
    reset();
    BackgroundWidget::resizeEvent( e );
}

void EasterEggBackgroundWidget::animate()
{
    b_enabled = true;
    if ( isVisible() ) timer->start();
}

void EasterEggBackgroundWidget::spawnFlakes()
{
    if ( ! isVisible() ) return;

    double w = (double) width() / RAND_MAX;

    int i_spawn = QRandomGenerator::global()->bounded( i_rate + 1 );

    QList<flake *>::iterator it = flakes->begin();
    while( it != flakes->end() )
    {
        flake *current = *it;
        current->point.setY( current->point.y() + i_speed );
        if ( current->point.y() + i_speed >= height() )
        {
            delete current;
            it = flakes->erase( it );
        }
        else
            ++it;
    }

    if ( flakes->size() < MAX_FLAKES )
    for ( int i=0; i<i_spawn; i++ )
    {
        flake *f = new flake;
        f->point.setX( QRandomGenerator::global()->generateDouble() * w );
        f->b_fat = QRandomGenerator::global()->bounded( 100 ) < 33;
        flakes->append( f );
    }
    update();
}

void EasterEggBackgroundWidget::reset()
{
    while ( !flakes->isEmpty() )
        delete flakes->takeFirst();
}

void EasterEggBackgroundWidget::paintEvent( QPaintEvent *e )
{
    QPainter painter(this);

    painter.setBrush( QBrush( QColor(Qt::white) ) );
    painter.setPen( QPen(Qt::white) );

    QList<flake *>::const_iterator it = flakes->constBegin();
    while( it != flakes->constEnd() )
    {
        const flake * const f = *(it++);
        if ( f->b_fat )
        {
            /* Xsnow like :p */
            painter.drawPoint( f->point.x(), f->point.y() -1 );
            painter.drawPoint( f->point.x() + 1, f->point.y() );
            painter.drawPoint( f->point.x(), f->point.y() +1 );
            painter.drawPoint( f->point.x() - 1, f->point.y() );
        }
        else
        {
            painter.drawPoint( f->point );
        }
    }

    BackgroundWidget::paintEvent( e );
}

SpeedLabel::SpeedLabel( intf_thread_t *_p_intf, QWidget *parent )
           : QLabel( parent ), p_intf( _p_intf )
{
    tooltipStringPattern = qtr( "Current playback speed: %1\nClick to adjust" );

    /* Create the Speed Control Widget */
    speedControl = new SpeedControlWidget( p_intf, this );
    speedControlMenu = new QMenu( this );

    widgetAction = new QWidgetAction( speedControl );
    widgetAction->setDefaultWidget( speedControl );
    speedControlMenu->addAction( widgetAction );

    /* Change the SpeedRate in the Label */
    CONNECT( THEMIM->getIM(), rateChanged( float ), this, setRate( float ) );

    DCONNECT( THEMIM, inputChanged( bool ),
              speedControl, activateOnState() );

    setContentsMargins(4, 0, 4, 0);
    setRate( var_InheritFloat( THEPL, "rate" ) );
}

SpeedLabel::~SpeedLabel()
{
    widgetAction->setParent( this );
    delete speedControlMenu;
}

/****************************************************************************
 * Small right-click menu for rate control
 ****************************************************************************/

void SpeedLabel::showSpeedMenu( QPoint pos )
{
    speedControlMenu->exec( QCursor::pos() - pos
                            + QPoint( -70 + width()/2, height() ) );
}

void SpeedLabel::setRate( float rate )
{
    QString str;
    str.setNum( rate, 'f', 2 );
    str.append( "x" );
    setText( str );
    setToolTip( tooltipStringPattern.arg( str ) );
    speedControl->updateControls( rate );
}

/**********************************************************************
 * Speed control widget
 **********************************************************************/
SpeedControlWidget::SpeedControlWidget( intf_thread_t *_p_i, QWidget *_parent )
                    : QFrame( _parent ), p_intf( _p_i )
{
    QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Maximum );
    sizePolicy.setHorizontalStretch( 0 );
    sizePolicy.setVerticalStretch( 0 );

    speedSlider = new QSlider( this );
    speedSlider->setSizePolicy( sizePolicy );
    speedSlider->setMinimumSize( QSize( 140, 20 ) );
    speedSlider->setOrientation( Qt::Horizontal );
    speedSlider->setTickPosition( QSlider::TicksBelow );

    speedSlider->setRange( -34, 34 );
    speedSlider->setSingleStep( 1 );
    speedSlider->setPageStep( 1 );
    speedSlider->setTickInterval( 17 );

    CONNECT( speedSlider, valueChanged( int ), this, updateRate( int ) );

    QToolButton *normalSpeedButton = new QToolButton( this );
    normalSpeedButton->setMaximumSize( QSize( 26, 16 ) );
    normalSpeedButton->setAutoRaise( true );
    normalSpeedButton->setText( "1x" );
    normalSpeedButton->setToolTip( qtr( "Revert to normal play speed" ) );

    CONNECT( normalSpeedButton, clicked(), this, resetRate() );

    QToolButton *slowerButton = new QToolButton( this );
    slowerButton->setMaximumSize( QSize( 26, 16 ) );
    slowerButton->setAutoRaise( true );
    slowerButton->setToolTip( tooltipL[SLOWER_BUTTON] );
    slowerButton->setIcon( QIcon( iconL[SLOWER_BUTTON] ) );
    CONNECT( slowerButton, clicked(), THEMIM->getIM(), slower() );

    QToolButton *fasterButton = new QToolButton( this );
    fasterButton->setMaximumSize( QSize( 26, 16 ) );
    fasterButton->setAutoRaise( true );
    fasterButton->setToolTip( tooltipL[FASTER_BUTTON] );
    fasterButton->setIcon( QIcon( iconL[FASTER_BUTTON] ) );
    CONNECT( fasterButton, clicked(), THEMIM->getIM(), faster() );

/*    spinBox = new QDoubleSpinBox();
    spinBox->setDecimals( 2 );
    spinBox->setMaximum( 32 );
    spinBox->setMinimum( 0.03F );
    spinBox->setSingleStep( 0.10F );
    spinBox->setAlignment( Qt::AlignRight );

    CONNECT( spinBox, valueChanged( double ), this, updateSpinBoxRate( double ) ); */

    QGridLayout* speedControlLayout = new QGridLayout( this );
    speedControlLayout->addWidget( speedSlider, 0, 0, 1, 3 );
    speedControlLayout->addWidget( slowerButton, 1, 0 );
    speedControlLayout->addWidget( normalSpeedButton, 1, 1, 1, 1, Qt::AlignRight );
    speedControlLayout->addWidget( fasterButton, 1, 2, 1, 1, Qt::AlignRight );
    //speedControlLayout->addWidget( spinBox );
    speedControlLayout->setContentsMargins( 0, 0, 0, 0 );
    speedControlLayout->setSpacing( 0 );

    lastValue = 0;

    activateOnState();
}

void SpeedControlWidget::activateOnState()
{
    speedSlider->setEnabled( THEMIM->getIM()->hasInput() );
    //spinBox->setEnabled( THEMIM->getIM()->hasInput() );
}

void SpeedControlWidget::updateControls( float rate )
{
    if( speedSlider->isSliderDown() )
    {
        //We don't want to change anything if the user is using the slider
        return;
    }

    double value = 17 * log( rate ) / log( 2. );
    int sliderValue = (int) ( ( value > 0 ) ? value + .5 : value - .5 );

    if( sliderValue < speedSlider->minimum() )
    {
        sliderValue = speedSlider->minimum();
    }
    else if( sliderValue > speedSlider->maximum() )
    {
        sliderValue = speedSlider->maximum();
    }
    lastValue = sliderValue;

    speedSlider->setValue( sliderValue );
    //spinBox->setValue( rate );
}

void SpeedControlWidget::updateRate( int sliderValue )
{
    if( sliderValue == lastValue )
        return;
    lastValue = sliderValue;

    double speed = pow( 2, (double)sliderValue / 17 );
    int rate = INPUT_RATE_DEFAULT / speed;

    THEMIM->getIM()->setRate(rate);
    //spinBox->setValue( var_InheritFloat( THEPL, "rate" ) );
}

void SpeedControlWidget::updateSpinBoxRate( double r )
{
    var_SetFloat( THEPL, "rate", r );
}

void SpeedControlWidget::resetRate()
{
    THEMIM->getIM()->setRate( INPUT_RATE_DEFAULT );
}

CoverArtLabel::CoverArtLabel( QWidget *parent, intf_thread_t *_p_i )
    : QLabel( parent ), p_intf( _p_i ), p_item( NULL )
{
    setContextMenuPolicy( Qt::ActionsContextMenu );
    CONNECT( THEMIM->getIM(), artChanged( input_item_t * ),
             this, showArtUpdate( input_item_t * ) );

    setMinimumHeight( 128 );
    setMinimumWidth( 128 );
    setScaledContents( false );
    setAlignment( Qt::AlignCenter );

    QAction *action = new QAction( qtr( "Download cover art" ), this );
    CONNECT( action, triggered(), this, askForUpdate() );
    addAction( action );

    action = new QAction( qtr( "Add cover art from file" ), this );
    CONNECT( action, triggered(), this, setArtFromFile() );
    addAction( action );

    p_item = THEMIM->currentInputItem();
    if( p_item )
    {
        input_item_Hold( p_item );
        showArtUpdate( p_item );
    }
    else
        showArtUpdate( "" );
}

CoverArtLabel::~CoverArtLabel()
{
    QList< QAction* > artActions = actions();
    foreach( QAction *act, artActions )
        removeAction( act );
    if ( p_item ) input_item_Release( p_item );
}

void CoverArtLabel::setItem( input_item_t *_p_item )
{
    if ( p_item ) input_item_Release( p_item );
    p_item = _p_item;
    if ( p_item ) input_item_Hold( p_item );
}

void CoverArtLabel::showArtUpdate( const QString& url )
{
    QPixmap pix;
    if( !url.isEmpty() && pix.load( url ) )
    {
        pix = pix.scaled( minimumWidth(), minimumHeight(),
                          Qt::KeepAspectRatioByExpanding,
                          Qt::SmoothTransformation );
    }
    else
    {
        pix = QPixmap( ":/noart.png" );
    }
    setPixmap( pix );
}

void CoverArtLabel::showArtUpdate( input_item_t *_p_item )
{
    /* not for me */
    if ( _p_item != p_item )
        return;

    QString url;
    if ( _p_item ) url = THEMIM->getIM()->decodeArtURL( _p_item );
    showArtUpdate( url );
}

void CoverArtLabel::askForUpdate()
{
    THEMIM->getIM()->requestArtUpdate( p_item, true );
}

void CoverArtLabel::setArtFromFile()
{
    if( !p_item )
        return;

    QUrl fileUrl = QFileDialog::getOpenFileUrl( this, qtr( "Choose Cover Art" ),
        p_intf->p_sys->filepath, qtr( "Image Files (*.gif *.jpg *.jpeg *.png)" ) );

    if( fileUrl.isEmpty() )
        return;

    THEMIM->getIM()->setArt( p_item, fileUrl.toString() );
}

void CoverArtLabel::clear()
{
    showArtUpdate( "" );
}

TimeLabel::TimeLabel( intf_thread_t *_p_intf, TimeLabel::Display _displayType  )
    : ClickableQLabel()
    , p_intf( _p_intf )
    , cachedPos( -1 )
    , cachedTime( 0 )
    , cachedLength( 0 )
    , displayType( _displayType )
{
    b_remainingTime = false;
    if( _displayType != TimeLabel::Elapsed )
        b_remainingTime = getSettings()->value( "MainWindow/ShowRemainingTime", false ).toBool();
    switch( _displayType ) {
        case TimeLabel::Elapsed:
            setText( " --:-- " );
            setToolTip( qtr("Elapsed time") );
            break;
        case TimeLabel::Remaining:
            setText( " --:-- " );
            setToolTip( qtr("Total/Remaining time")
                        + QString("\n-")
                        + qtr("Click to toggle between total and remaining time")
                      );
            break;
        case TimeLabel::Both:
            setText( " --:--/--:-- " );
            setToolTip( QString( "- " )
                + qtr( "Click to toggle between elapsed and remaining time" )
                + QString( "\n- " )
                + qtr( "Double click to jump to a chosen time position" ) );
            break;
    }
    setAlignment( Qt::AlignRight | Qt::AlignVCenter );

    CONNECT( THEMIM->getIM(), seekRequested( float ),
             this, setDisplayPosition( float ) );

    CONNECT( THEMIM->getIM(), positionUpdated( float, int64_t, int ),
              this, setDisplayPosition( float, int64_t, int ) );

    connect( this, SIGNAL( broadcastRemainingTime( bool ) ),
         THEMIM->getIM(), SIGNAL( remainingTimeChanged( bool ) ) );

    CONNECT( THEMIM->getIM(), remainingTimeChanged( bool ),
              this, setRemainingTime( bool ) );

    setStyleSheet( "QLabel { padding-left: 4px; padding-right: 4px; }" );
}

void TimeLabel::setRemainingTime( bool remainingTime )
{
    if( displayType != TimeLabel::Elapsed )
    {
        b_remainingTime = remainingTime;
        refresh();
    }
}

void TimeLabel::refresh()
{
    setDisplayPosition( cachedPos, cachedTime, cachedLength );
}

void TimeLabel::setDisplayPosition( float pos, int64_t t, int length )
{
    cachedPos = pos;
    if( pos == -1.f )
    {
        setMinimumSize( QSize( 0, 0 ) );
        if( displayType == TimeLabel::Both )
            setText( "--:--/--:--" );
        else
            setText( "--:--" );
        return;
    }

    int time = t / 1000000;

    secstotimestr( psz_length, length );
    secstotimestr( psz_time, ( b_remainingTime && length ) ? length - time
                                                           : time );

    // compute the minimum size that will be required for the psz_length
    // and use it to enforce a minimal size to avoid "dancing" widgets
    QSize minsize( 0, 0 );
    if ( length > 0 )
    {
        QMargins margins = contentsMargins();
        minsize += QSize(
                  fontMetrics().size( 0, QString( psz_length ), 0, 0 ).width(),
                  sizeHint().height()
                );
        minsize += QSize( margins.left() + margins.right() + 8, 0 ); /* +padding */

        if ( b_remainingTime )
            minsize += QSize( fontMetrics().size( 0, "-", 0, 0 ).width(), 0 );
    }

    switch( displayType )
    {
        case TimeLabel::Elapsed:
            setMinimumSize( minsize );
            setText( QString( psz_time ) );
            break;
        case TimeLabel::Remaining:
            if( b_remainingTime )
            {
                setMinimumSize( minsize );
                setText( QString("-") + QString( psz_time ) );
            }
            else
            {
                setMinimumSize( QSize( 0, 0 ) );
                setText( QString( psz_length ) );
            }
            break;
        case TimeLabel::Both:
        default:
            QString timestr = QString( "%1%2/%3" )
            .arg( QString( (b_remainingTime && length) ? "-" : "" ) )
            .arg( QString( psz_time ) )
            .arg( QString( ( !length && time ) ? "--:--" : psz_length ) );

            setText( timestr );
            break;
    }
    cachedLength = length;
    cachedTime = t;
}

void TimeLabel::setDisplayPosition( float pos )
{
    int64_t time = pos * cachedLength * 1000000;
    setDisplayPosition( pos, time, cachedLength );
}

void TimeLabel::toggleTimeDisplay()
{
    b_remainingTime = !b_remainingTime;
    getSettings()->setValue( "MainWindow/ShowRemainingTime", b_remainingTime );
    emit broadcastRemainingTime( b_remainingTime );
}
