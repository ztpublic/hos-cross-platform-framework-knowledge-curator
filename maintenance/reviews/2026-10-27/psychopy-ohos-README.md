# PsychoPy Studio — HarmonyOS Port

[简体中文](./README-CN.md) | [Project Homepage](https://aik358.github.io/psychopy-studio-harmonyos/)

## Overview

**PsychoPy Studio OH** is a HarmonyOS port of the open-source [PsychoPy](https://www.psychopy.org/) experiment builder for psychology and neuroscience. It packages the Builder, Coder, and Runner as a native HAP via the **Electron-OH** runtime (Chromium 132, Electron 34.8.4).

> **Current: v2026.1.2** — branch `2026.1.2` | Package: `com.a9iska.psychopy`

## Status (2026-07-24)

### Core Pipeline: ✅ Fully Working

| Feature | Status |
|---------|:------:|
| Python 3.12.8 WebSocket liaison | ✅ |
| psychopy-lib 2026.1.2 full import | ✅ |
| Builder (35 components) → writeScript → Run-in-Browser | ✅ |
| PsychoJS ESM experiment execution | ✅ |
| Auto pip-install missing Python packages | ✅ |
| File system access (Desktop/Documents/Downloads) | ✅ |
| Python Terminal + diagnostics | ✅ |

### Framework Upgrade: ✅ Electron 34.8.4

| Item | Old | New |
|------|-----|-----|
| `libelectron.so` | ~153 MB (2025) | **159 MB (2026-06-26)** |
| `libadapter.so` | ~1.5 MB | **5.2 MB** |
| New adapters | — | EtsBridge, WebApp, NodeHandle, Popup, KVStore, CommandLine |
| `libsndfile.so` | ❌ missing (soft-fail) | ✅ **bundled (v1.2.2)** |

### UI: ✅ Chinese Localization + Tablet Mode

| Feature | Status |
|---------|:------:|
| i18n framework (Svelte 5 runes, SSR-safe) | ✅ |
| Menu / Ribbon / Dialog translation (zh_CN + en_US) | ✅ |
| Language switcher in top-nav bar | ✅ |
| Page zoom slider (50%–200%) | ✅ |
| Tablet mode detection (HarmonyOS + no Python) | ✅ |
| Tablet mode banner (bottom-right pill) | ✅ |
| View switching bug fix (removed full-reload fallback) | ✅ |

### AppGallery Publishing: ⏳ In Review

| Item | Status |
|------|:------:|
| Package rename → `com.a9iska.psychopy` | ✅ |
| Multi-size icons (48~1024px) | ✅ |
| Chinese localization (zh_CN) | ✅ |
| Privacy policy + attribution docs | ✅ |
| ACL permission: ALLOW_WRITABLE_CODE_MEMORY | ⏳ under review |
| Release signing + .app built | ⏳ pending ACL approval |

## Architecture

```
┌─────────────────────────────────────────┐
│  ArkUI → Electron-OH (Chromium 132)      │
│  ┌─────────────────────────────────────┐ │
│  │  harmony-python.js (main process)   │ │
│  │  ├─ liaison_shim.py (WebSocket)     │ │
│  │  ├─ psychopy-lib 2026.1.2           │ │
│  │  ├─ PsychoJS HTTP server            │ │
│  │  └─ SvelteKit frontend              │ │
│  └─────────────────────────────────────┘ │
│  ┌─────────────────────────────────────┐ │
│  │  System Python 3.12.8 (HNP)         │ │
│  │  numpy / scipy / matplotlib / ...   │ │
│  └─────────────────────────────────────┘ │
│  ┌─────────────────────────────────────┐ │
│  │  Native libs (arm64-v8a)            │ │
│  │  libelectron / libadapter           │ │
│  │  libffmpeg / libsndfile / libc++    │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

## Known Issues

| Issue | Status |
|-------|:------:|
| Tablet mode crash (MatePad Edge) | ⚠️ need ACL permission |
| Camera / Microphone | ⚠️ untested with new libsndfile |
| Pavlovia OAuth | ⏳ pending |
| Theme CSS may fail to load on HarmonyOS WebView | ⚠️ fallback variables in place |

## Recent Updates (2026-07-24)

Build `2026.1.2` (branch `2026.1.2`) — this push supersedes the previous `436fd47` baseline with:

- **ESM browser-run hardening (no Python)**: `psychojs-browser/startServer` now emits a proper ES-module experiment — `<script type="module" src="experiment.js">` plus `psychojs-2026.1.2.js` — so *Run-in-Browser* works identically with or without a Python environment. The PsychoJS path (`psychojs-2026.1.2.js`) is unchanged by design.
- **Python experiment assembly**: `python/psychopy_worker.py` (`generate` / `conditions` subcommands) assembles the same ESM experiment directory when a Python env + `.psyexp` are available, byte-for-byte matching the no-Python path.
- **Tablet vs PC mode separation**: Bundle mode (embedded HNP, feasibility-only) is now decoupled from true no-Python tablet mode. Without Python the app gracefully hides Python-only features, but **Run-in-Browser always works**; the bottom-right hint reads "平板模式" instead of "Python not found".
- **Native libraries**: added `libfreetype.so.6` (PIL / fonts) and `libusb-1.0.so.0` (pyusb), with cross-compile scripts under `tools/`.

## Build Instructions

**Prerequisites:** DevEco Studio 6.1+, HarmonyOS device (ARM64).

```bash
git clone https://gitcode.com/A9iska/psychopy-oh.git
cd psychopy-oh && git checkout 2026.1.2
ohpm install
cd web_engine/src/main/resources/resfile/resources/app
npm install && npx vite build
cd ../../../../../../../..
# DevEco: File → Project Structure → Signing Configs → auto-sign → Apply
# DevEco: Build → Build HAP(s)
```

## Branches

| Branch | Description |
|--------|-------------|
| `2026.1.2` | **Active** — Full pipeline, Electron 34.8.4, AppGallery prep |
| `main` | Landing page + docs |
| `v0.1.6` | Previous stable |

## Attribution

PsychoPy is © Open Science Tools Ltd / Jonathan Peirce, licensed under GPLv3.
This HarmonyOS port is by [A9iska](https://gitcode.com/A9iska).
Source: [https://gitcode.com/A9iska/psychopy-oh](https://gitcode.com/A9iska/psychopy-oh)

## License

GPL v3 — same as upstream PsychoPy.

---

> **Last updated: 2026-07-24**
