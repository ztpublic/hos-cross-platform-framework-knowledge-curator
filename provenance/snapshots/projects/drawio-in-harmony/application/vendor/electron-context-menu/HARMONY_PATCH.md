# Local compatibility patch

Based on the complete npm `electron-context-menu@5.1.0` package, licensed MIT
(see `license`). The upstream menu implementation and options are preserved.

The sole runtime patch selects Electron 37's `clipboard.write({text, bookmark})`
when `ClipboardItem` is absent; Electron 44 retains its original representations.
Both sync throws and asynchronous rejections reach the original menu error handler.
On platforms without bookmark formats, plain URL text remains available as in
Electron's documented legacy clipboard API.

Node's minimum is 22.12 after auditing the shipped JavaScript and production
dependencies; all other language/API use is available in Node 22. The independent
test and application smoke run must be repeated for runtime changes. No global
Electron objects are patched, and no menu features are removed.
