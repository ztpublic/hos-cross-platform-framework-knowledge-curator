#ifdef __OHOS__

#include "components/ohos_video_texture_widget.hpp"

#include <QMetaObject>
#include <QRectF>
#include <QVector2D>
#include <QDebug>

#include <vlc_subpicture.h>

#include <GLES2/gl2ext.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>

#include <mutex>

namespace {
constexpr char vertexShader[] = R"(
attribute vec2 aPosition;
attribute vec2 aTexCoord;
uniform mat4 uTexTransform;
varying vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoord = (uTexTransform * vec4(aTexCoord, 0.0, 1.0)).xy;
}
)";

constexpr char fragmentShader[] = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
uniform samplerExternalOES uTexture;
varying vec2 vTexCoord;
void main()
{
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

constexpr char subtitleVertexShader[] = R"(
attribute vec2 aPosition;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

constexpr char subtitleFragmentShader[] = R"(
precision mediump float;
uniform sampler2D uTexture;
varying vec2 vTexCoord;
void main()
{
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

std::mutex subtitleWindowMutex;
OhosVideoTextureWindow *subtitleWindow = nullptr;
}

OhosVideoTextureWindow::OhosVideoTextureWindow( QWindow *parent )
    : QOpenGLWindow( QOpenGLWindow::NoPartialUpdate, parent )
{
    m_subtitleExpiryTimer.setSingleShot( true );
    m_subtitleExpiryTimer.setInterval( 250 );
    connect( &m_subtitleExpiryTimer, &QTimer::timeout, this, [this]() {
        if( m_subtitleVisible )
        {
            m_subtitleVisible = false;
            update();
        }
    } );

    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    subtitleWindow = this;
}

OhosVideoTextureWindow::~OhosVideoTextureWindow()
{
    m_alive.store( false, std::memory_order_release );
    {
        std::lock_guard<std::mutex> locker( subtitleWindowMutex );
        if( subtitleWindow == this )
            subtitleWindow = nullptr;
    }
    if( context() )
    {
        makeCurrent();
        releaseGlResources();
        doneCurrent();
    }
}

void *OhosVideoTextureWindow::nativeWindow() const
{
    return m_nativeWindow;
}

void OhosVideoTextureWindow::setVideoSize( const QSize &size )
{
    if( size.isValid() && m_videoSize != size )
    {
        m_videoSize = size;
        if( m_nativeWindow )
            setNativeWindowBufferSize( size );
        update();
    }
}

void OhosVideoTextureWindow::setSubtitleFrame( const QImage &image,
                                                const QSize &sourceSize,
                                                int align, int x, int y,
                                                OhosSubtitleOverlayOwner owner )
{
    if( image.isNull() )
        return;

    m_subtitleImage = image;
    m_subtitleSourceSize = sourceSize;
    m_subtitleAlign = align;
    m_subtitleX = x;
    m_subtitleY = y;
    m_subtitleOwner = owner;
    m_subtitleTextureDirty = true;
    m_subtitleVisible = true;
    m_subtitleExpiryTimer.start();
    qWarning() << "VLC_OHOS_SUBTITLE frame" << image.size()
               << "source" << sourceSize << "align" << align;
    update();
}

void OhosVideoTextureWindow::subtitleHeartbeat(
    OhosSubtitleOverlayOwner owner )
{
    if( m_subtitleOwner != owner || m_subtitleImage.isNull() )
        return;

    const bool wasVisible = m_subtitleVisible;
    m_subtitleVisible = true;
    m_subtitleExpiryTimer.start();
    if( !wasVisible )
        update();
}

void OhosVideoTextureWindow::clearSubtitle( OhosSubtitleOverlayOwner owner )
{
    if( m_subtitleOwner != owner )
        return;
    m_subtitleExpiryTimer.stop();
    if( !m_subtitleVisible && m_subtitleImage.isNull() )
        return;

    m_subtitleVisible = false;
    m_subtitleImage = QImage();
    m_subtitleSourceSize = QSize();
    m_subtitleTextureDirty = false;
    update();
}

bool OhosVideoTextureWindow::setNativeWindowBufferSize( const QSize &size )
{
    if( !m_nativeWindow || !size.isValid() || size.isEmpty() )
        return false;

    const int32_t result = OH_NativeWindow_NativeWindowHandleOpt(
        static_cast<OHNativeWindow *>( m_nativeWindow ), SET_BUFFER_GEOMETRY,
        static_cast<int32_t>( size.width() ),
        static_cast<int32_t>( size.height() ) );
    qWarning() << "VLC_OHOS_TEXTURE buffer geometry" << size
               << "window" << m_nativeWindow << "result" << result;
    return result == 0;
}

bool OhosVideoTextureWindow::createShaderProgram()
{
    return m_program.addShaderFromSourceCode( QOpenGLShader::Vertex, vertexShader )
        && m_program.addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentShader )
        && m_program.link();
}

bool OhosVideoTextureWindow::createSubtitleShaderProgram()
{
    return m_subtitleProgram.addShaderFromSourceCode(
               QOpenGLShader::Vertex, subtitleVertexShader )
        && m_subtitleProgram.addShaderFromSourceCode(
               QOpenGLShader::Fragment, subtitleFragmentShader )
        && m_subtitleProgram.link();
}

void OhosVideoTextureWindow::initializeGL()
{
    qWarning() << "VLC_OHOS_TEXTURE initializeGL size" << size()
               << "context" << context();
    initializeOpenGLFunctions();
    glClearColor( 0.f, 0.f, 0.f, 1.f );

    if( !createShaderProgram() )
        return;

    if( createSubtitleShaderProgram() )
    {
        glGenTextures( 1, &m_subtitleTexture );
        glBindTexture( GL_TEXTURE_2D, m_subtitleTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    }
    else
        qWarning() << "VLC_OHOS_SUBTITLE failed to create overlay shader";

    glGenTextures( 1, &m_externalTexture );
    glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_externalTexture );
    glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    m_nativeImage = OH_NativeImage_Create( m_externalTexture, GL_TEXTURE_EXTERNAL_OES );
    if( !m_nativeImage )
        return;

    m_nativeWindow = OH_NativeImage_AcquireNativeWindow( m_nativeImage );
    if( !m_nativeWindow )
        return;

    /* NativeImage starts with a 0x0 producer buffer. OHOSAVCodec cannot
     * dequeue its first output buffer until SET_BUFFER_GEOMETRY has supplied
     * a non-zero size (BufferQueue otherwise reports CheckBufferConfig 0x0).
     * Prefer the encoded size when VLC already reported it; the exposed GL
     * child size is a safe bootstrap value until then. */
    QSize bufferSize = m_videoSize;
    if( !bufferSize.isValid() || bufferSize.isEmpty() )
    {
        const qreal dpr = devicePixelRatio();
        bufferSize = QSize( qMax( 1, qRound( width() * dpr ) ),
                            qMax( 1, qRound( height() * dpr ) ) );
    }
    if( !setNativeWindowBufferSize( bufferSize ) )
        return;

    OH_OnFrameAvailableListener listener{};
    listener.context = this;
    listener.onFrameAvailable = &OhosVideoTextureWindow::onFrameAvailable;
    if( OH_NativeImage_SetOnFrameAvailableListener( m_nativeImage, listener ) != 0 )
        return;

    qWarning() << "VLC_OHOS_TEXTURE native image ready" << m_nativeImage
               << "window" << m_nativeWindow;
    emit nativeWindowReady( m_nativeWindow );
}

void OhosVideoTextureWindow::onFrameAvailable( void *context )
{
    auto *window = static_cast<OhosVideoTextureWindow *>( context );
    if( !window || !window->m_alive.load( std::memory_order_acquire ) )
        return;
    window->m_pendingFrames.fetch_add( 1, std::memory_order_release );
    if( !window->m_firstFrameLogged.exchange( true, std::memory_order_acq_rel ) )
        qWarning() << "VLC_OHOS_TEXTURE first frame available";
    /* The callback runs on a BufferQueue thread. The functor overload is
     * guaranteed to execute in this QWindow's GUI thread; string lookup of
     * the inherited, non-invokable update() can silently fail. */
    QMetaObject::invokeMethod( window, [window]() {
        if( window->m_alive.load( std::memory_order_acquire ) )
            window->update();
    }, Qt::QueuedConnection );
}

void OhosVideoTextureWindow::paintGL()
{
    glClear( GL_COLOR_BUFFER_BIT );
    if( !m_nativeImage || !m_externalTexture || !m_program.isLinked() )
        return;

    /* Consume every notified buffer so the codec's finite output queue does
     * not fill and stall. A/V compensation belongs to VLC's input clock, not
     * to throttling this BufferQueue consumer. */
    const unsigned int pending = m_pendingFrames.exchange(
        0, std::memory_order_acq_rel );
    bool consumedFrame = false;
    for( unsigned int i = 0; i < pending; ++i )
    {
        if( OH_NativeImage_UpdateSurfaceImage( m_nativeImage ) != 0 )
            break;
        m_hasFrame = true;
        consumedFrame = true;
    }
    if( consumedFrame && !m_firstFramePresented )
    {
        m_firstFramePresented = true;
        emit firstFramePresented();
    }
    if( !m_hasFrame )
        return;

    float textureMatrixData[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    /* Decoder buffers may have an aligned stride and a visible crop. Use the
     * buffer matrix so both the producer transform and that crop are applied. */
    if( OH_NativeImage_GetBufferMatrix( m_nativeImage, textureMatrixData ) != 0 )
        return;

    float scaleX = 1.f;
    float scaleY = 1.f;
    if( m_videoSize.isValid() && height() > 0 )
    {
        const float videoAspect = float( m_videoSize.width() ) / m_videoSize.height();
        const float widgetAspect = float( width() ) / height();
        if( widgetAspect > videoAspect )
            scaleX = videoAspect / widgetAspect;
        else
            scaleY = widgetAspect / videoAspect;
    }

    const float vertices[] = {
        -scaleX, -scaleY, 0.f, 0.f,
         scaleX, -scaleY, 1.f, 0.f,
        -scaleX,  scaleY, 0.f, 1.f,
         scaleX,  scaleY, 1.f, 1.f
    };

    m_program.bind();
    m_program.setUniformValue( "uTexture", 0 );
    /* NativeImage returns an OpenGL column-major matrix. QMatrix4x4's float
     * constructor consumes row-major values, which transposes the translation
     * into W and clamps most samples to one texture edge. Upload it directly. */
    glUniformMatrix4fv( m_program.uniformLocation( "uTexTransform" ), 1,
                        GL_FALSE, textureMatrixData );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_externalTexture );

    const int position = m_program.attributeLocation( "aPosition" );
    const int texCoord = m_program.attributeLocation( "aTexCoord" );
    m_program.enableAttributeArray( position );
    m_program.enableAttributeArray( texCoord );
    m_program.setAttributeArray( position, GL_FLOAT, vertices, 2, 4 * sizeof(float) );
    m_program.setAttributeArray( texCoord, GL_FLOAT, vertices + 2, 2, 4 * sizeof(float) );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    m_program.disableAttributeArray( position );
    m_program.disableAttributeArray( texCoord );
    m_program.release();

    drawSubtitle();
}

void OhosVideoTextureWindow::uploadSubtitleTexture()
{
    if( !m_subtitleTextureDirty || !m_subtitleTexture
        || m_subtitleImage.isNull() )
        return;

    QImage rgba = m_subtitleImage;
    if( rgba.format() != QImage::Format_RGBA8888 )
        rgba = rgba.convertToFormat( QImage::Format_RGBA8888 );
    m_subtitleImage = rgba;

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_subtitleTexture );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
                  rgba.width(), rgba.height(), 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, rgba.constBits() );
    m_subtitleTextureDirty = false;
}

void OhosVideoTextureWindow::drawSubtitle()
{
    if( !m_subtitleVisible || m_subtitleImage.isNull()
        || !m_subtitleTexture || !m_subtitleProgram.isLinked() )
        return;

    uploadSubtitleTexture();

    const qreal dpr = devicePixelRatio();
    const QSize viewportSize( qMax( 1, qRound( width() * dpr ) ),
                              qMax( 1, qRound( height() * dpr ) ) );
    QSize sourceSize = m_subtitleSourceSize;
    if( !sourceSize.isValid() || sourceSize.isEmpty() )
        sourceSize = m_videoSize;
    if( !sourceSize.isValid() || sourceSize.isEmpty() )
        return;

    const qreal sourceAspect = qreal( sourceSize.width() ) / sourceSize.height();
    const qreal viewportAspect = qreal( viewportSize.width() ) / viewportSize.height();
    QRectF videoRect;
    if( viewportAspect > sourceAspect )
    {
        const qreal videoWidth = viewportSize.height() * sourceAspect;
        videoRect = QRectF( (viewportSize.width() - videoWidth) / 2.0, 0.0,
                            videoWidth, viewportSize.height() );
    }
    else
    {
        const qreal videoHeight = viewportSize.width() / sourceAspect;
        videoRect = QRectF( 0.0, (viewportSize.height() - videoHeight) / 2.0,
                            viewportSize.width(), videoHeight );
    }

    const qreal scale = videoRect.height() / sourceSize.height();
    const QSizeF subtitleSize( m_subtitleImage.width() * scale,
                               m_subtitleImage.height() * scale );
    qreal subtitleX;
    if( m_subtitleAlign & SUBPICTURE_ALIGN_LEFT )
        subtitleX = videoRect.left() + m_subtitleX * scale;
    else if( m_subtitleAlign & SUBPICTURE_ALIGN_RIGHT )
        subtitleX = videoRect.right() - subtitleSize.width()
                  - m_subtitleX * scale;
    else
        subtitleX = videoRect.center().x() - subtitleSize.width() / 2.0;

    qreal subtitleY;
    if( m_subtitleAlign & SUBPICTURE_ALIGN_TOP )
        subtitleY = videoRect.top() + m_subtitleY * scale;
    else if( m_subtitleAlign & SUBPICTURE_ALIGN_BOTTOM )
        subtitleY = videoRect.bottom() - subtitleSize.height()
                  - m_subtitleY * scale;
    else
        subtitleY = videoRect.center().y() - subtitleSize.height() / 2.0;

    QRectF subtitleRect( subtitleX, subtitleY,
                         subtitleSize.width(), subtitleSize.height() );
    if( subtitleRect.left() < videoRect.left() )
        subtitleRect.moveLeft( videoRect.left() );
    if( subtitleRect.right() > videoRect.right() )
        subtitleRect.moveRight( videoRect.right() );
    if( subtitleRect.top() < videoRect.top() )
        subtitleRect.moveTop( videoRect.top() );
    if( subtitleRect.bottom() > videoRect.bottom() )
        subtitleRect.moveBottom( videoRect.bottom() );

    const float left = float( 2.0 * subtitleRect.left()
                              / viewportSize.width() - 1.0 );
    const float right = float( 2.0 * subtitleRect.right()
                               / viewportSize.width() - 1.0 );
    const float top = float( 1.0 - 2.0 * subtitleRect.top()
                             / viewportSize.height() );
    const float bottom = float( 1.0 - 2.0 * subtitleRect.bottom()
                                / viewportSize.height() );
    const float vertices[] = {
        left,  bottom, 0.f, 1.f,
        right, bottom, 1.f, 1.f,
        left,  top,    0.f, 0.f,
        right, top,    1.f, 0.f
    };

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    m_subtitleProgram.bind();
    m_subtitleProgram.setUniformValue( "uTexture", 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_subtitleTexture );

    const int position = m_subtitleProgram.attributeLocation( "aPosition" );
    const int texCoord = m_subtitleProgram.attributeLocation( "aTexCoord" );
    m_subtitleProgram.enableAttributeArray( position );
    m_subtitleProgram.enableAttributeArray( texCoord );
    m_subtitleProgram.setAttributeArray(
        position, GL_FLOAT, vertices, 2, 4 * sizeof(float) );
    m_subtitleProgram.setAttributeArray(
        texCoord, GL_FLOAT, vertices + 2, 2, 4 * sizeof(float) );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    m_subtitleProgram.disableAttributeArray( position );
    m_subtitleProgram.disableAttributeArray( texCoord );
    m_subtitleProgram.release();
    glDisable( GL_BLEND );
    if( !m_firstSubtitlePresented )
    {
        m_firstSubtitlePresented = true;
        qWarning() << "VLC_OHOS_SUBTITLE first overlay presented";
    }
}

void OhosVideoTextureWindow::releaseGlResources()
{
    if( m_nativeImage )
        OH_NativeImage_UnsetOnFrameAvailableListener( m_nativeImage );
    /* The window returned by OH_NativeImage_AcquireNativeWindow is owned by
     * the NativeImage. Destroying it separately can invalidate the consumer
     * while the codec still holds the producer surface. */
    m_nativeWindow = nullptr;
    if( m_nativeImage )
        OH_NativeImage_Destroy( &m_nativeImage );
    if( m_externalTexture )
    {
        glDeleteTextures( 1, &m_externalTexture );
        m_externalTexture = 0;
    }
    if( m_subtitleTexture )
    {
        glDeleteTextures( 1, &m_subtitleTexture );
        m_subtitleTexture = 0;
    }
    m_program.removeAllShaders();
    m_subtitleProgram.removeAllShaders();
}

bool OhosVideoTexturePublishSubtitle( const QImage &image,
                                      const QSize &sourceSize,
                                      int align, int x, int y )
{
    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    OhosVideoTextureWindow *window = subtitleWindow;
    if( !window )
        return false;

    return QMetaObject::invokeMethod( window,
        [window, image, sourceSize, align, x, y]() {
            window->setSubtitleFrame(
                image, sourceSize, align, x, y,
                OhosSubtitleOverlayOwner::Text );
        }, Qt::QueuedConnection );
}

void OhosVideoTextureSubtitleHeartbeat()
{
    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    OhosVideoTextureWindow *window = subtitleWindow;
    if( !window )
        return;

    QMetaObject::invokeMethod( window, [window]() {
        window->subtitleHeartbeat( OhosSubtitleOverlayOwner::Text );
    }, Qt::QueuedConnection );
}

bool OhosVideoTexturePublishBitmapSubtitle( const QImage &image,
                                            const QSize &sourceSize,
                                            int align, int x, int y )
{
    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    OhosVideoTextureWindow *window = subtitleWindow;
    if( !window )
        return false;

    return QMetaObject::invokeMethod( window,
        [window, image, sourceSize, align, x, y]() {
            window->setSubtitleFrame(
                image, sourceSize, align, x, y,
                OhosSubtitleOverlayOwner::Bitmap );
        }, Qt::QueuedConnection );
}

void OhosVideoTextureBitmapSubtitleHeartbeat()
{
    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    OhosVideoTextureWindow *window = subtitleWindow;
    if( !window )
        return;

    QMetaObject::invokeMethod( window, [window]() {
        window->subtitleHeartbeat( OhosSubtitleOverlayOwner::Bitmap );
    }, Qt::QueuedConnection );
}

void OhosVideoTextureClearBitmapSubtitle()
{
    std::lock_guard<std::mutex> locker( subtitleWindowMutex );
    OhosVideoTextureWindow *window = subtitleWindow;
    if( !window )
        return;

    QMetaObject::invokeMethod( window, [window]() {
        window->clearSubtitle( OhosSubtitleOverlayOwner::Bitmap );
    }, Qt::QueuedConnection );
}

#endif
