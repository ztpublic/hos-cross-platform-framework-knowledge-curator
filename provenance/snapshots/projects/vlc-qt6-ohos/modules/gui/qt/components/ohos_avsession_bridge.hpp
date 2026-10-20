#ifndef VLC_QT_OHOS_AVSESSION_BRIDGE_HPP_
#define VLC_QT_OHOS_AVSESSION_BRIDGE_HPP_

#ifdef __OHOS__

#include <QObject>
#include <QUrl>

#include <multimedia/av_session/native_avsession.h>

class MainInputManager;
struct intf_thread_t;

class OhosAvSessionBridge final : public QObject
{
    Q_OBJECT

public:
    OhosAvSessionBridge( intf_thread_t *intf, MainInputManager *mim,
                         QObject *parent = nullptr );
    ~OhosAvSessionBridge() override;

private slots:
    void handleInputChanged( bool hasInput );
    void handleNameChanged( const QString &title );
    void handlePlayingStatusChanged( int status );
    void handlePositionUpdated( float position, int64_t time,
                                int lengthSeconds );
    void seekAbsolute( qint64 positionMs );
    void seekRelative( qint64 deltaMs );

private:
    static AVSessionCallback_Result onCommand(
        OH_AVSession *session, AVSession_ControlCommand command,
        void *userData );
    static AVSessionCallback_Result onSeek(
        OH_AVSession *session, uint64_t seekTime, void *userData );
    static AVSessionCallback_Result onForward(
        OH_AVSession *session, uint32_t seekTime, void *userData );
    static AVSessionCallback_Result onRewind(
        OH_AVSession *session, uint32_t seekTime, void *userData );

    void registerCallbacks();
    void unregisterCallbacks();
    void setCurrentMedia();
    void updateMetadata( qint64 durationMs );
    void updatePlaybackState( AVSession_PlaybackState state );
    void dispatchCommand( AVSession_ControlCommand command );

    intf_thread_t *m_intf;
    MainInputManager *m_mim;
    OH_AVSession *m_session = nullptr;
    QUrl m_url;
    QString m_title;
    qint64 m_metadataDurationMs = -1;
    qint64 m_lastPositionUpdateMs = 0;
    bool m_active = false;
};

#endif

#endif
