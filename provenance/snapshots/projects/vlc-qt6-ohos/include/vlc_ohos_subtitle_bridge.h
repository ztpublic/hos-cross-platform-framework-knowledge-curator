/*****************************************************************************
 * vlc_ohos_subtitle_bridge.h: OHOS opaque-vout subtitle overlay bridge
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 ******************************************************************************/

#ifndef VLC_OHOS_SUBTITLE_BRIDGE_H
#define VLC_OHOS_SUBTITLE_BRIDGE_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VLC_OHOS_SUBTITLE_BRIDGE_VAR "ohos-subtitle-bridge"
#define VLC_OHOS_SUBTITLE_BRIDGE_ABI_VERSION 1u

/**
 * One VLC picture plane containing a positioned subtitle region.
 *
 * The first ABI only accepts VLC_CODEC_RGBA. The producer owns pixels and
 * only has to keep them alive until publish() returns; the GUI copies them.
 */
typedef struct vlc_ohos_subtitle_region
{
    uint32_t struct_size;
    uint32_t chroma;
    const uint8_t *pixels;
    int32_t pitch;
    uint32_t width;
    uint32_t height;
    int32_t x;
    int32_t y;
} vlc_ohos_subtitle_region_t;

/** A complete subpicture in original video coordinates. */
typedef struct vlc_ohos_subtitle_frame
{
    uint32_t struct_size;
    uint32_t source_width;
    uint32_t source_height;
    uint8_t alpha;
    uint8_t reserved[3];
    size_t region_count;
    const vlc_ohos_subtitle_region_t *regions;
} vlc_ohos_subtitle_frame_t;

/**
 * Callback table published through VLC_OHOS_SUBTITLE_BRIDGE_VAR.
 *
 * The OHOS vout retrieves this address with var_InheritAddress(). Calls may
 * originate on a VLC video-output thread; callbacks marshal work to Qt.
 */
typedef struct vlc_ohos_subtitle_bridge
{
    uint32_t struct_size;
    uint32_t abi_version;
    void *opaque;
    int (*publish)(void *opaque, const vlc_ohos_subtitle_frame_t *frame);
    void (*heartbeat)(void *opaque);
    void (*clear)(void *opaque);
} vlc_ohos_subtitle_bridge_t;

#ifdef __cplusplus
}
#endif

#endif
