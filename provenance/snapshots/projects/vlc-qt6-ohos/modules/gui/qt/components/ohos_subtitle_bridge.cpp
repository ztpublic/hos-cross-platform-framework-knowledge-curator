#ifdef __OHOS__

#include "components/ohos_subtitle_bridge.hpp"
#include "components/ohos_video_texture_widget.hpp"

#include <QImage>
#include <QDebug>
#include <QPainter>
#include <QRect>

#include <vlc_common.h>
#include <vlc_subpicture.h>

#include <climits>
#include <atomic>
#include <cstddef>

namespace {

constexpr size_t maximumRegionCount = 64;
constexpr uint32_t maximumSourceDimension = 32768;
std::atomic_bool firstPublishLogged{ false };
std::atomic_bool firstPublishResultLogged{ false };

bool validRegion( const vlc_ohos_subtitle_region_t &region )
{
    if( region.struct_size < sizeof( vlc_ohos_subtitle_region_t )
        || region.chroma != VLC_CODEC_RGBA || !region.pixels
        || region.width == 0 || region.height == 0
        || region.width > INT_MAX || region.height > INT_MAX
        || region.pitch <= 0 || region.pitch > INT_MAX )
        return false;

    const uint64_t minimumPitch = uint64_t( region.width ) * 4u;
    return minimumPitch <= INT_MAX
        && uint64_t( region.pitch ) >= minimumPitch;
}

int publishSubtitle( void *, const vlc_ohos_subtitle_frame_t *frame )
{
    if( frame && !firstPublishLogged.exchange( true ) )
        qWarning() << "VLC_OHOS_SUBTITLE bridge publish received"
                   << "source" << frame->source_width << frame->source_height
                   << "alpha" << frame->alpha
                   << "regions" << frame->region_count;

    if( !frame
        || frame->struct_size < sizeof( vlc_ohos_subtitle_frame_t )
        || !frame->regions || frame->region_count == 0
        || frame->region_count > maximumRegionCount
        || frame->source_width == 0 || frame->source_height == 0
        || frame->source_width > maximumSourceDimension
        || frame->source_height > maximumSourceDimension )
        return -1;

    const QSize sourceSize( static_cast<int>( frame->source_width ),
                            static_cast<int>( frame->source_height ) );
    const QRect sourceRect( QPoint( 0, 0 ), sourceSize );
    QRect bounds;
    for( size_t i = 0; i < frame->region_count; ++i )
    {
        const vlc_ohos_subtitle_region_t &region = frame->regions[i];
        if( !validRegion( region ) )
            continue;
        const QRect regionRect( region.x, region.y,
                                static_cast<int>( region.width ),
                                static_cast<int>( region.height ) );
        bounds = bounds.united( regionRect.intersected( sourceRect ) );
    }
    if( bounds.isEmpty() )
        return -1;

    QImage overlay( bounds.size(), QImage::Format_RGBA8888 );
    if( overlay.isNull() )
        return -1;
    overlay.fill( Qt::transparent );

    QPainter painter( &overlay );
    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
    painter.setOpacity( frame->alpha / 255.0 );
    for( size_t i = 0; i < frame->region_count; ++i )
    {
        const vlc_ohos_subtitle_region_t &region = frame->regions[i];
        if( !validRegion( region ) )
            continue;

        const QRect regionRect( region.x, region.y,
                                static_cast<int>( region.width ),
                                static_cast<int>( region.height ) );
        const QRect visibleRect = regionRect.intersected( sourceRect );
        if( visibleRect.isEmpty() )
            continue;

        const QImage regionImage( region.pixels,
                                  static_cast<int>( region.width ),
                                  static_cast<int>( region.height ),
                                  region.pitch, QImage::Format_RGBA8888 );
        const QRect sourcePart( visibleRect.topLeft() - regionRect.topLeft(),
                                visibleRect.size() );
        painter.drawImage( visibleRect.topLeft() - bounds.topLeft(),
                           regionImage, sourcePart );
    }
    painter.end();

    const bool queued = OhosVideoTexturePublishBitmapSubtitle(
        overlay, sourceSize,
        SUBPICTURE_ALIGN_TOP | SUBPICTURE_ALIGN_LEFT,
        bounds.x(), bounds.y() );
    if( !firstPublishResultLogged.exchange( true ) )
        qWarning() << "VLC_OHOS_SUBTITLE bridge publish queued=" << queued
                   << "bounds" << bounds;
    return queued ? 0 : -1;
}

void heartbeatSubtitle( void * )
{
    OhosVideoTextureBitmapSubtitleHeartbeat();
}

void clearSubtitle( void * )
{
    OhosVideoTextureClearBitmapSubtitle();
}

const vlc_ohos_subtitle_bridge_t bridge = {
    sizeof( vlc_ohos_subtitle_bridge_t ),
    VLC_OHOS_SUBTITLE_BRIDGE_ABI_VERSION,
    nullptr,
    publishSubtitle,
    heartbeatSubtitle,
    clearSubtitle,
};

} // namespace

const vlc_ohos_subtitle_bridge_t *OhosSubtitleBridgeInstance()
{
    return &bridge;
}

#endif
