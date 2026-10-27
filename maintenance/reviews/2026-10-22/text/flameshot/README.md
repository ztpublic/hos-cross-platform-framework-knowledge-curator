# flameshot for HarmonyOS (OHOS)

> A HarmonyOS / OpenHarmony port of [flameshot](https://flameshot.org)
> (the popular Qt screenshot tool, 30k+ stars) built with **Qt 6.12 for OHOS**.
> Original upstream README preserved in [README_original.md](./README_original.md).

## Status

| | |
|---|---|
| Port status | **running on device** — 10/10 test cases passed (2026-09-23, HUAWEI MateBook Pro, HarmonyOS PC) |
| Bundle name | `com.ohos.flameshot` |
| Target ABI | `arm64-v8a` only |
| Qt OHOS SDK | Qt 6.12.0 prebuilt installer (`harmonyos_arm64_v8a` + `mingw_64` host) |
| Verified on | HUAWEI MateBook Pro (HAD-W24, HarmonyOS 6.1.0 / API 23) |

## Screenshot

![flameshot running on HarmonyOS PC](screenshots/flameshot-ohos-1.jpeg)
- Device: HUAWEI MateBook Pro (HAD-W24) | Verified: 2026-09-23

## What this fork adds

- **`HarmonyOS/`** — DevEco glue project (AppScope / entry / hvigor):
  - `entry/src/main/cpp/CMakeLists.txt` — builds `libflameshot.so` from the
    upstream tree at the repo root, links `Qt6::QOhosPlatformIntegrationPlugin`,
    embeds freshly compiled `.qm` translations, patches `DT_NEEDED` absolute
    paths post-link (`patch_needed.py`), and **auto-stages the runtime `.so`
    closure at configure time** (Qt libs + QPA/style/svg plugins from the Qt
    SDK, ICU/freetype/fontconfig/... from additional-packages, `libc++.so`
    from the OHOS NDK — no binaries are committed to this repo).
  - `QAbility.ets` — scene-recovery guard + runtime permission request.
  - Stubs for `QtColorWidgets` (color wheel) and `KDSingleApplication`.
- **Source patches on the upstream tree** (see commit
  `port(ohos): OHOS source overlay`): DBus guards (`Q_OS_OHOS`), `main.cpp`
  font loading + argc adaptation, launcher/capture-widget window choreography
  (keep-alive strategy + hide-after-export), public-directory save fallback
  chain with png-first picker defaults, pin (floating screenshot) mode,
  App Launcher tool → system preview direct open, zh_CN/zh_TW translations.

## Features on OHOS

- Region / fullscreen / screen capture with the full annotation toolbar
  (pen, shapes, text, blur, counter, undo/redo, color wheel)
- Save via system picker to public dirs (Pictures/Desktop/Documents) with
  `YYYY-MM-DD_HH-MM-SS.png` naming, or copy to clipboard
- **Pin** screenshots as always-on-top floating windows (drag / rotate /
  opacity / context menu, multiple pins)
- Open captures directly in the **system preview (HiPreview)** via the
  toolbar "open with" button
- System tray integration (left-click capture, menu), launcher window
- Chinese (zh_CN / zh_TW) UI embedded at build time

### Adapted for the platform

- Desktop-entry based "open with" app list → opens the system preview
  directly (OHOS has no desktop-entry app registry)
- DBus daemon / update checker disabled (guarded by `Q_OS_OHOS`)
- Imgur upload disabled at runtime (no TLS backend in this build)

## Prerequisites

1. **Qt 6.12.0 for OHOS prebuilt SDK** — installer layout:
   `<root>/6.12.0/harmonyos_arm64_v8a` (target) + `<root>/6.12.0/mingw_64`
   (host tools).
2. **additional-packages** — third-party runtime closure (ICU / freetype /
   fontconfig / libpng / brotli / expat) for Qt 6.12 OHOS, installed next to
   the SDK installer root.
3. **[DevEco Studio](https://developer.huawei.com/consumer/cn/deveco-studio/)**
   (SDK API 23+), or CLI: `hvigorw` + JDK.

## Build

```powershell
cd HarmonyOS
./setup.ps1     # fills <QT6_OHOS_SDK>/<QT6_HOST>/<ADDITIONAL_PACKAGES>/<DEVECO_SDK>
                # placeholders in entry/build-profile.json5 (params or env vars;
                # see the script header for the expected layout)

# CLI build (or open HarmonyOS/ in DevEco Studio):
$env:DEVECO_SDK_HOME = "<DevEco>/sdk"       # e.g. D:\qtsoftware\DevEcoStudio\sdk
$env:JAVA_HOME       = "<DevEco>/jbr"
hvigorw assembleHap --no-daemon
# -> entry/build/default/outputs/default/entry-default-unsigned.hap (~43 MB)
```

The produced HAP is unsigned — sign it with your own debug/release profile
(DevEco Studio: File > Project Structure > Signing Configs) and install via
DevEco or `hdc install`.

## Verification

Device-verified on HUAWEI MateBook Pro (2026-09-23): capture flows, editor
tools, save-to-public-dirs (`.png` opens directly in HiPreview), clipboard,
pin mode, tray menu, launcher lifecycle, long-background re-open, Chinese UI —
10/10 core test cases pass. See the porting knowledge base (ohos-qt-kb) for
the full test matrix and known platform limitations (e.g. scene-recovery
blank window after very long background sessions is a Qt OHOS QPA
limitation, guarded against crashes by `QAbility.ets`).

## Sync with upstream

This branch (`ohos-port`) = upstream `master` (2d478061, 2026-09-18) pinned
to the device-verified snapshot + the port overlay commits. To rebase onto a
newer upstream:

```bash
git remote add upstream https://github.com/flameshot-org/flameshot.git
git fetch upstream && git rebase upstream/master   # resolve conflicts in the patched files
```

## License

GPL-3.0-or-later (same as upstream flameshot).
