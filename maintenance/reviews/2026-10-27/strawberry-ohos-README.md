# Strawberry — HarmonyOS (OHOS) Port

> Fork of [Strawberry Music Player](https://github.com/strawberrymusicplayer/strawberry) adding a HarmonyOS / OpenHarmony build via **Qt for OHOS**.
> Original upstream README preserved in [README_original.md](./README_original.md).

## Status

**Port status:** running on device — test.mp3 plays end-to-end (typefind → ffmpeg decode → OpenSL ES output)<br>
**Verified:** 2026-08-08 on HUAWEI MateBook Fold (OHOS 6.0), Qt 6.12 OHOS build, GStreamer 1.20.3 + FFmpeg 4.4<br>
**Bundle name:** `org.qtproject.example.qcpainterbench`  **Target ABI:** `arm64-v8a`

## Running screenshot

![Strawberry on OHOS](screenshots/strawberry-ohos.png)

## What this fork adds

- **`HarmonyOS/`** — DevEco (hvigor) glue HAP project: `AppScope/`, `entry/` (napi stub `hello.cpp` + ArkTS qability/qabilitystage/pages + resources), `build-profile.json5` (unsigned template), `hvigorfile.ts`.
- **`stubs/`** — compile-boundary stubs (ICU/KDSingleApplication/sqlite3/qstrnicmp) used so the upstream CMake cross-compiles without the full Linux desktop deps; `stubs.cmake` attaches them + wires the **real** cross-compiled GLib/GStreamer includes via `OHOS_GST_INSTALL` / `OHOS_GLIB_INSTALL` env vars.
- **Source changes** (kept minimal, guarded by `OHOS`/`Q_OS_OHOS`):
  - `CMakeLists.txt` — `add_compile_definitions(__OHOS__)` (so Qt's `qsystemdetection.h` cascade yields `Q_OS_OHOS`, enabling the OHOS code paths); `qt_add_executable`; a `POST_BUILD` step runs `cmake/fix_ohos_needed_paths.py` to rewrite full-build-path Qt6 `DT_NEEDED` entries to basenames (the Qt6 OHOS dev-build `.so` lack SONAME → absolute-path NEEDED → device `dlopen` fails → startup crash).
  - `src/engine/gstengine.cpp` — `#ifdef Q_OS_OHOS` selects `openslessink` (OHOS has no `autoaudiosink`).
  - `src/engine/gststartup.cpp` — `#if defined(Q_OS_OHOS)` block: `dladdr`-derives the bundle `gstreamer-1.0/` plugin dir → `GST_PLUGIN_PATH`/`GST_PLUGIN_SYSTEM_PATH`, writable `GST_REGISTRY`, `GST_DEBUG`/`GST_DEBUG_FILE`.
  - `cmake/fix_ohos_needed_paths.py` — the post-link NEEDED rewriter (idempotent).
- The **Qt6 `.so` runtime + GStreamer plugins + FFmpeg `.so` are NOT committed** — they are build outputs (see Build).

## ⚠️ This is not a one-click clone-and-build

Strawberry uses GStreamer for audio. On OHOS, the upstream GStreamer `typefind` for mp3 is **intentionally disabled** (`ohos.opt.compat.0001` in `third_party_gstreamer`'s `gsttypefindfunctions.c`: *"Use the demux and typefind of ffmpeg to process MP3"*). So **FFmpeg (gst-libav) is required** — not optional — for mp3 typefind + decode. You must cross-compile the GStreamer stack first.

### Prerequisites

1. **Qt 6.12 for OHOS** (a self-built or prebuilt OHOS SDK, e.g. `C:/Qt/Qt6.12.0-arm64-v8a-a11y`). Qt6 OHOS `.so` lack SONAME → the `fix_ohos_needed_paths.py` post-link step is mandatory.
2. **OHOS NDK** (`C:/ohos-sdk-20/native` — clang + sysroot + `libOpenSLES.so`).
3. **DevEco Studio** (hvigorw + jbr + node + sdk).
4. **Cross-compiled GStreamer stack** (from OpenHarmony `third_party_*`):
   - `third_party_gstreamer` (core + gstplugins_base: playbin/decodebin/typefindfunctions/...) — built with `-DOHOS_OPT_COMPAT`.
   - `third_party_ffmpeg` (FFmpeg 4.4, `ohos_config.sh` configure pattern + musl sed hacks).
   - `gst_libav` (the libav plugin → `libgstlibav.so`, links FFmpeg; meson, needs `gstreamer-tag` dep added).
   - `gstplugins_bad/sys/opensles` (`libgstopensles.so`) — **must be ported to OHOS `SL_IID_OH_BUFFERQUEUE`/`SLOHBufferQueueItf`** (OHOS does NOT support the standard `SL_IID_BUFFERQUEUE` → `SL_RESULT_FEATURE_UNSUPPORTED (0x0c)`); OHOS OpenSL ES is a partial impl, so play-interface position/event/mute calls must be made non-fatal.
   - Source: https://gitee.com/openharmony/third_party_gstreamer (+ third_party_glib + third_party_pcre2 + gst_libav is inside third_party_gstreamer; third_party_ffmpeg separate).
5. Env: `OHOS_GST_INSTALL`, `OHOS_GLIB_INSTALL` (the cross-compiled install dirs).

### Build

```bash
# 1) Cross-compile the GStreamer stack (FFmpeg + gst-libav + opensles port) per the above.
#    Install into e.g. _temp/{ffmpeg,gst,glib,pcre2,orc}-install.

# 2) Build libstrawberry.so (the app) with the OHOS toolchain:
export OHOS_GST_INSTALL=<path>/gst-install
export OHOS_GLIB_INSTALL=<path>/glib-install
cmake -S . -B build-ohos \
  -DOHOS=ON \
  -DCMAKE_TOOLCHAIN_FILE=<ohos-ndk>/build/cmake/ohos.toolchain.cmake \
  -DQT6_OHOS_SDK=<path>/Qt6.12.0-arm64-v8a-a11y   # adjust to your Qt6 OHOS SDK
cmake --build build-ohos --target strawberry -j
# → build-ohos/libstrawberry.so (post-build auto-rewrites Qt6 NEEDED → basenames)

# 3) Stage native libs into the HAP (libstrawberry.so + Qt6 .so + GStreamer plugins + FFmpeg .so):
cp build-ohos/libstrawberry.so            HarmonyOS/entry/libs/arm64-v8a/
cp <gst-install>/lib/gstreamer-1.0/*.so   HarmonyOS/entry/libs/arm64-v8a/gstreamer-1.0/
cp <gst-install>/lib/libgst*-1.0.so.0     HarmonyOS/entry/libs/arm64-v8a/
cp <ffmpeg-install>/lib/libav*.so.*      HarmonyOS/entry/libs/arm64-v8a/
# (+ Qt6 .so + libglib/libpcre2 etc. into HarmonyOS/entry/libs/arm64-v8a/)

# 4) Assemble + sign + install the HAP (DevEco hvigorw):
cd HarmonyOS
node <DevEco>/tools/hvigor/bin/hvigorw.js --mode module -p module=entry@default assembleHap -p buildMode=debug --no-daemon
hdc install -r entry/build/default/outputs/default/entry-default-signed.hap
hdc shell aa start -b org.qtproject.example.qcpainterbench -a QAbility
```

**Success:** app launches, plays an mp3 (e.g. push one into the sandbox `files/Music/`), no `autoaudiosink`/`MPEG-4 Video decoder missing` popup.

## Porting notes (key gotchas)

- **`__OHOS__` define is mandatory** — without it Qt leaves `Q_OS_OHOS` undefined → all `#if defined(Q_OS_OHOS)` is dead code → strawberry picks `autoaudiosink` (not packaged on OHOS) → "could not create the element autoaudiosink" popup. The `add_compile_definitions(__OHOS__)` in `CMakeLists.txt` fixes it.
- **Qt6 no-SONAME → startup crash** — Qt6 OHOS dev-build `.so` have no SONAME → the linker records the absolute build path as `DT_NEEDED` → device musl `ldso` can't resolve a Windows path → `dlopen(libstrawberry.so)` fails → crash. `cmake/fix_ohos_needed_paths.py` (POST_BUILD) rewrites those to basenames.
- **mp3 needs FFmpeg** — OHOS disabled the built-in mp3 `typefind` expecting ffmpeg; without `gst-libav`, an ID3-tagged mp3 is misidentified as MPEG-4 video. `libgstlibav.so` provides `avdemux_mp3` (accepts `application/x-id3`) + `avdec_mp3`.
- **openslessink → OHOS BufferQueue port** — OHOS OpenSL ES uses `SL_IID_OH_BUFFERQUEUE`/`SLOHBufferQueueItf`, not the standard `SL_IID_BUFFERQUEUE`; the callback gains a `size` param. See `gstplugins_bad/sys/opensles/openslesringbuffer.c`. Non-essential play-interface calls (`SetPositionUpdatePeriod`/`SetCallbackEventsMask`/event `RegisterCallback`/`SetMute`) return `0x0c` (FEATURE_UNSUPPORTED) on OHOS — must be non-fatal.
- **Sandbox file access** — the app can only read files inside its sandbox; a "desktop" file must be staged into `/data/app/el2/100/base/<bundle>/files/Music/` (+ the playlist `url` pointed there) for playback.

## Sync with upstream

```bash
git remote add upstream https://github.com/strawberrymusicplayer/strawberry.git
git fetch upstream && git merge upstream/<default-branch>
```

## License

Upstream: GPLv3 (see [COPYING](./COPYING) + [README_original.md](./README_original.md)). OHOS glue follows the same license.
