#ifndef VLC_QT_OHOS_VIDEO_TEXTURE_WIDGET_HPP_
#define VLC_QT_OHOS_VIDEO_TEXTURE_WIDGET_HPP_

#ifdef __OHOS__

#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWindow>
#include <QSize>
#include <QTimer>

#include <atomic>

struct OH_NativeImage;

enum class OhosSubtitleOverlayOwner
{
    Text,
    Bitmap,
};

class OhosVideoTextureWindow final : public QOpenGLWindow,
                                     protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OhosVideoTextureWindow( QWindow *parent = nullptr );
    ~OhosVideoTextureWindow() override;

    void *nativeWindow() const;
    void setVideoSize( const QSize &size );
    void setSubtitleFrame( const QImage &image, const QSize &sourceSize,
                           int align, int x, int y,
                           OhosSubtitleOverlayOwner owner );
    void subtitleHeartbeat( OhosSubtitleOverlayOwner owner );
    void clearSubtitle( OhosSubtitleOverlayOwner owner );

signals:
    void nativeWindowReady( void *nativeWindow );
    void firstFramePresented();

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    static void onFrameAvailable( void *context );
    void releaseGlResources();
    bool createShaderProgram();
    bool createSubtitleShaderProgram();
    bool setNativeWindowBufferSize( const QSize &size );
    void drawSubtitle();
    void uploadSubtitleTexture();

    QOpenGLShaderProgram m_program;
    QOpenGLShaderProgram m_subtitleProgram;
    OH_NativeImage *m_nativeImage = nullptr;
    void *m_nativeWindow = nullptr;
    unsigned int m_externalTexture = 0;
    unsigned int m_subtitleTexture = 0;
    QSize m_videoSize;
    QImage m_subtitleImage;
    QSize m_subtitleSourceSize;
    int m_subtitleAlign = 0;
    int m_subtitleX = 0;
    int m_subtitleY = 0;
    QTimer m_subtitleExpiryTimer;
    std::atomic_uint m_pendingFrames{ 0 };
    std::atomic_bool m_firstFrameLogged{ false };
    bool m_hasFrame = false;
    bool m_firstFramePresented = false;
    bool m_firstSubtitlePresented = false;
    bool m_subtitleTextureDirty = false;
    bool m_subtitleVisible = false;
    OhosSubtitleOverlayOwner m_subtitleOwner =
        OhosSubtitleOverlayOwner::Text;
    std::atomic_bool m_alive{ true };
};

/* Called from VLC's SPU rendering thread. The implementation queues all
 * window access to the Qt GUI thread. */
bool OhosVideoTexturePublishSubtitle( const QImage &image,
                                      const QSize &sourceSize,
                                      int align, int x, int y );
void OhosVideoTextureSubtitleHeartbeat();
bool OhosVideoTexturePublishBitmapSubtitle( const QImage &image,
                                            const QSize &sourceSize,
                                            int align, int x, int y );
void OhosVideoTextureBitmapSubtitleHeartbeat();
void OhosVideoTextureClearBitmapSubtitle();

#endif

#endif
