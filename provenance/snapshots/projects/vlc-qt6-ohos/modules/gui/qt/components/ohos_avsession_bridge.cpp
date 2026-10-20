#ifdef __OHOS__

#include "components/ohos_avsession_bridge.hpp"

#include "input_manager.hpp"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>

#include <multimedia/av_session/native_avmetadata.h>

namespace {
constexpr auto BundleName = "org.videolan.vlc.qt";
constexpr auto AbilityName = "QAbility";

void logResult( const char *operation, AVSession_ErrCode result )
{
    if( result != AV_SESSION_ERR_SUCCESS )
        qWarning() << "VLC_OHOS_AVSESSION" << operation
                   << "failed:" << int( result );
}
}

OhosAvSessionBridge::OhosAvSessionBridge(
    intf_thread_t *intf, MainInputManager *mim, QObject *parent )
    : QObject( parent ), m_intf( intf ), m_mim( mim )
{
    const AVSession_ErrCode createResult = OH_AVSession_Create(
        SESSION_TYPE_AUDIO, "VLC", BundleName, AbilityName, &m_session );
    if( createResult != AV_SESSION_ERR_SUCCESS )
    {
        m_session = nullptr;
        logResult( "create", createResult );
        return;
    }

    registerCallbacks();
    InputManager *im = m_mim->getIM();
    connect( m_mim, &MainInputManager::inputChanged,
             this, &OhosAvSessionBridge::handleInputChanged );
    connect( im, &InputManager::nameChanged,
             this, &OhosAvSessionBridge::handleNameChanged );
    connect( im, &InputManager::playingStatusChanged,
             this, &OhosAvSessionBridge::handlePlayingStatusChanged );
    connect( im, &InputManager::positionUpdated,
             this, &OhosAvSessionBridge::handlePositionUpdated );

    qInfo() << "VLC_OHOS_AVSESSION created";
    if( m_mim->getInput() )
        setCurrentMedia();
}

OhosAvSessionBridge::~OhosAvSessionBridge()
{
    if( !m_session )
        return;
    unregisterCallbacks();
    if( m_active )
        logResult( "deactivate", OH_AVSession_Deactivate( m_session ) );
    logResult( "destroy", OH_AVSession_Destroy( m_session ) );
}

void OhosAvSessionBridge::registerCallbacks()
{
    for( const auto command : { CONTROL_CMD_PLAY, CONTROL_CMD_PAUSE,
                                CONTROL_CMD_STOP, CONTROL_CMD_PLAY_NEXT,
                                CONTROL_CMD_PLAY_PREVIOUS } )
        logResult( "register command", OH_AVSession_RegisterCommandCallback(
            m_session, command, &OhosAvSessionBridge::onCommand, this ) );
    logResult( "register seek", OH_AVSession_RegisterSeekCallback(
        m_session, &OhosAvSessionBridge::onSeek, this ) );
    logResult( "register forward", OH_AVSession_RegisterForwardCallback(
        m_session, &OhosAvSessionBridge::onForward, this ) );
    logResult( "register rewind", OH_AVSession_RegisterRewindCallback(
        m_session, &OhosAvSessionBridge::onRewind, this ) );
}

void OhosAvSessionBridge::unregisterCallbacks()
{
    for( const auto command : { CONTROL_CMD_PLAY, CONTROL_CMD_PAUSE,
                                CONTROL_CMD_STOP, CONTROL_CMD_PLAY_NEXT,
                                CONTROL_CMD_PLAY_PREVIOUS } )
        OH_AVSession_UnregisterCommandCallback(
            m_session, command, &OhosAvSessionBridge::onCommand );
    OH_AVSession_UnregisterSeekCallback(
        m_session, &OhosAvSessionBridge::onSeek );
    OH_AVSession_UnregisterForwardCallback(
        m_session, &OhosAvSessionBridge::onForward );
    OH_AVSession_UnregisterRewindCallback(
        m_session, &OhosAvSessionBridge::onRewind );
}

void OhosAvSessionBridge::handleInputChanged( bool hasInput )
{
    if( hasInput )
        setCurrentMedia();
    else if( m_session && m_active )
        updatePlaybackState( PLAYBACK_STATE_COMPLETED );
}

void OhosAvSessionBridge::setCurrentMedia()
{
    if( !m_session || !m_mim->getInput() )
        return;
    input_item_t *item = m_mim->currentInputItem();
    if( !item )
        return;

    char *uri = input_item_GetURI( item );
    m_url = uri ? QUrl::fromEncoded( QByteArray( uri ), QUrl::StrictMode )
                : QUrl();
    free( uri );
    if( !m_url.isValid() )
        return;

    m_title = m_mim->getIM()->getName();
    if( m_title.isEmpty() )
        m_title = QFileInfo( m_url.path() ).completeBaseName();
    m_metadataDurationMs = -1;
    m_lastPositionUpdateMs = 0;
    if( !m_active )
    {
        const AVSession_ErrCode result = OH_AVSession_Activate( m_session );
        m_active = result == AV_SESSION_ERR_SUCCESS;
        if( !m_active )
        {
            logResult( "activate", result );
            return;
        }
    }

    const qint64 durationMs = qMax<int64_t>(
        0, var_GetInteger( m_mim->getInput(), "length" ) / 1000 );
    updateMetadata( durationMs );
    updatePlaybackState( PLAYBACK_STATE_PREPARING );
}

void OhosAvSessionBridge::handleNameChanged( const QString &title )
{
    if( title.isEmpty() || title == m_title )
        return;
    m_title = title;
    m_metadataDurationMs = -1;
}

void OhosAvSessionBridge::updateMetadata( qint64 durationMs )
{
    if( !m_session || !m_active || !m_url.isValid()
        || durationMs == m_metadataDurationMs )
        return;

    OH_AVMetadataBuilder *builder = nullptr;
    if( OH_AVMetadataBuilder_Create( &builder ) != AVMETADATA_SUCCESS
        || !builder )
        return;
    const QByteArray assetId = m_url.toString( QUrl::FullyEncoded ).toUtf8();
    const QByteArray title = m_title.toUtf8();
    OH_AVMetadataBuilder_SetAssetId( builder, assetId.constData() );
    OH_AVMetadataBuilder_SetTitle( builder, title.constData() );
    OH_AVMetadataBuilder_SetDuration( builder, qMax<qint64>( 0, durationMs ) );
    OH_AVMetadataBuilder_SetSkipIntervals( builder, SECONDS_10 );

    OH_AVMetadata *metadata = nullptr;
    const auto generated = OH_AVMetadataBuilder_GenerateAVMetadata(
        builder, &metadata );
    OH_AVMetadataBuilder_Destroy( builder );
    if( generated != AVMETADATA_SUCCESS || !metadata )
        return;
    const AVSession_ErrCode result = OH_AVSession_SetAVMetadata(
        m_session, metadata );
    OH_AVMetadata_Destroy( metadata );
    if( result == AV_SESSION_ERR_SUCCESS )
        m_metadataDurationMs = durationMs;
    else
        logResult( "set metadata", result );
}

void OhosAvSessionBridge::updatePlaybackState( AVSession_PlaybackState state )
{
    if( m_session && m_active )
        logResult( "set playback state",
                   OH_AVSession_SetPlaybackState( m_session, state ) );
}

void OhosAvSessionBridge::handlePlayingStatusChanged( int status )
{
    switch( status )
    {
    case OPENING_S:
        updatePlaybackState( PLAYBACK_STATE_PREPARING );
        break;
    case PLAYING_S:
        updatePlaybackState( PLAYBACK_STATE_PLAYING );
        break;
    case PAUSE_S:
        updatePlaybackState( PLAYBACK_STATE_PAUSED );
        break;
    case END_S:
    case ERROR_S:
        updatePlaybackState( PLAYBACK_STATE_COMPLETED );
        break;
    default:
        break;
    }
}

void OhosAvSessionBridge::handlePositionUpdated(
    float, int64_t time, int lengthSeconds )
{
    if( !m_session || !m_active || time < 0 )
        return;
    const qint64 durationMs = qMax( 0, lengthSeconds ) * INT64_C(1000);
    updateMetadata( durationMs );

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if( now - m_lastPositionUpdateMs < 1000 )
        return;
    m_lastPositionUpdateMs = now;
    AVSession_PlaybackPosition position = {
        qMax<int64_t>( 0, time / 1000 ), now
    };
    logResult( "set position", OH_AVSession_SetPlaybackPosition(
        m_session, &position ) );
}

void OhosAvSessionBridge::seekAbsolute( qint64 positionMs )
{
    input_thread_t *input = m_mim->getInput();
    if( input )
        var_SetInteger( input, "time", qMax<qint64>( 0, positionMs ) * 1000 );
}

void OhosAvSessionBridge::seekRelative( qint64 deltaMs )
{
    input_thread_t *input = m_mim->getInput();
    if( !input )
        return;
    const qint64 currentMs = var_GetInteger( input, "time" ) / 1000;
    seekAbsolute( currentMs + deltaMs );
}

AVSessionCallback_Result OhosAvSessionBridge::onCommand(
    OH_AVSession *, AVSession_ControlCommand command, void *userData )
{
    auto *self = static_cast<OhosAvSessionBridge *>( userData );
    if( !self )
        return AVSESSION_CALLBACK_RESULT_FAILURE;
    QMetaObject::invokeMethod( self,
        [self, command]() { self->dispatchCommand( command ); },
        Qt::QueuedConnection );
    return AVSESSION_CALLBACK_RESULT_SUCCESS;
}

AVSessionCallback_Result OhosAvSessionBridge::onSeek(
    OH_AVSession *, uint64_t time, void *userData )
{
    auto *self = static_cast<OhosAvSessionBridge *>( userData );
    if( !self )
        return AVSESSION_CALLBACK_RESULT_FAILURE;
    QMetaObject::invokeMethod( self,
        [self, time]() { self->seekAbsolute( qint64( time ) ); },
        Qt::QueuedConnection );
    return AVSESSION_CALLBACK_RESULT_SUCCESS;
}

AVSessionCallback_Result OhosAvSessionBridge::onForward(
    OH_AVSession *, uint32_t time, void *userData )
{
    auto *self = static_cast<OhosAvSessionBridge *>( userData );
    if( !self )
        return AVSESSION_CALLBACK_RESULT_FAILURE;
    QMetaObject::invokeMethod( self,
        [self, time]() { self->seekRelative( qint64( time ) * 1000 ); },
        Qt::QueuedConnection );
    return AVSESSION_CALLBACK_RESULT_SUCCESS;
}

AVSessionCallback_Result OhosAvSessionBridge::onRewind(
    OH_AVSession *, uint32_t time, void *userData )
{
    auto *self = static_cast<OhosAvSessionBridge *>( userData );
    if( !self )
        return AVSESSION_CALLBACK_RESULT_FAILURE;
    QMetaObject::invokeMethod( self,
        [self, time]() { self->seekRelative( -qint64( time ) * 1000 ); },
        Qt::QueuedConnection );
    return AVSESSION_CALLBACK_RESULT_SUCCESS;
}

void OhosAvSessionBridge::dispatchCommand( AVSession_ControlCommand command )
{
    switch( command )
    {
    case CONTROL_CMD_PLAY:
        m_mim->play();
        break;
    case CONTROL_CMD_PAUSE:
        m_mim->pause();
        break;
    case CONTROL_CMD_STOP:
        m_mim->stop();
        break;
    case CONTROL_CMD_PLAY_NEXT:
        m_mim->next();
        break;
    case CONTROL_CMD_PLAY_PREVIOUS:
        m_mim->prev();
        break;
    default:
        break;
    }
}

#endif
