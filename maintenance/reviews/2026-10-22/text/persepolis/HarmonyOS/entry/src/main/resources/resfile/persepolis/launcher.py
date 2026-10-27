# Persepolis for HarmonyOS - Python entry script (executed by pyqtlauncher)
# Stage A: stdlib C modules sanity (R1), Stage B: PyQt5 binding sanity (N1),
# Stage C: cross-thread signal (R7), Stage D: full persepolis UI startup
# (mirrors upstream persepolis.py main(), with OHOS adaptations).
import os
import sys
import traceback

import _ohos_hilog


def log(msg, level=1):
    _ohos_hilog.hilog_print(str(msg), level)


def _install_subwindow_adoption(mainwindow):
    """P009: adopt persepolis's parentless top-level sub-windows as dialog
    children of the main window, so the OHOS QPA routes them to SubWindow
    instead of spawning new main windows (W3). Installed AFTER the main
    window exists; interception happens on first show()/exec_() from Python,
    i.e. before the platform window is created (W1)."""
    from PyQt5 import QtWidgets
    from PyQt5.QtCore import Qt, QPoint

    def _adopt(w, allowed_types):
        if w is mainwindow or w.parentWidget() is not None:
            return
        # window-type filter: plain Qt::Window (the *_Ui(QWidget) sub-windows)
        # plus Qt::Dialog (QMessageBox etc. on the exec_ path). QMenu/Popup,
        # ToolTip etc. are transient and handled by Qt/the OHOS QPA itself.
        if int(w.windowFlags() & Qt.WindowType_Mask) not in allowed_types:
            return
        try:
            geom = w.geometry()
            w.setParent(mainwindow)
            w.setWindowFlag(Qt.Dialog, True)
            # setParent() clobbers geometry; restore, translating the saved
            # global coords into main-window-relative coords
            origin = mainwindow.mapToGlobal(QPoint(0, 0))
            w.setGeometry(geom.translated(-origin.x(), -origin.y()))
            log("P009: adopted %s as subwindow" % w.metaObject().className(), 1)
        except Exception:
            for line in traceback.format_exc().splitlines():
                log(line, 4)

    _types_show = (int(Qt.Window),)
    _types_exec = (int(Qt.Dialog), int(Qt.Window))

    orig_show = QtWidgets.QWidget.show

    def patched_show(self):
        if self.parentWidget() is None and self.isWindow():
            _adopt(self, _types_show)
        return orig_show(self)

    QtWidgets.QWidget.show = patched_show

    # parentless QMessageBox()/QDialog().exec_() (e.g. closeAction confirm)
    # never goes through QWidget.show from Python - hook exec_ as well
    orig_exec = QtWidgets.QDialog.exec_

    def patched_exec(self):
        # P026: QMessageBox is EXEMPT from adoption. When exec_'d from a
        # tray-menu action slot, the reparent + geometry shuffle collapsed
        # the box into a tiny frameless sliver (title clipped, no content)
        # on the OHOS QPA. Modal boxes manage their own windowing; the
        # adoption mechanism targets persistent sub-windows only.
        from PyQt5.QtWidgets import QMessageBox as _QMessageBox
        if isinstance(self, _QMessageBox):
            return orig_exec(self)
        if self.parentWidget() is None and self.isWindow():
            _adopt(self, _types_exec)
        return orig_exec(self)

    QtWidgets.QDialog.exec_ = patched_exec


def main():
    log("=== persepolis launcher.py start ===", 1)

    # PyRun_FileExFlags does not set __main__.__file__ (only runpy/import do);
    # persepolis's getExecPath() depends on it. sys.argv[0] carries the
    # script path (set by pyqtlauncher via PySys_SetArgvEx).
    if not hasattr(sys.modules['__main__'], '__file__'):
        sys.modules['__main__'].__file__ = sys.argv[0]

    # P016 diagnostics: tee stderr to a sandbox file. PyQt5 calls qFatal on
    # unhandled exceptions in C++-invoked Python (e.g. QThread.run), and the
    # pre-abort traceback goes to sys.stderr - hilog buffer gets flushed by
    # the crash storm, but the file survives for post-mortem analysis.
    class _TeeErr(object):
        def __init__(self, inner):
            self._inner = inner

        def write(self, text):
            try:
                self._inner.write(text)
            except Exception:
                pass
            if text and text.strip():
                try:
                    with open('/data/storage/el2/base/files/crash.log', 'a') as f:
                        f.write(text)
                except Exception:
                    pass

        def flush(self):
            try:
                self._inner.flush()
            except Exception:
                pass

    sys.stderr = _TeeErr(sys.stderr)

    # ---- Stages A/B/C: early-porting verification scaffolding (R1/N1/R7) ----
    # P015 (startup cost): these ran on EVERY launch - stage C alone costs
    # ~1.6s (QApplication + event loop + hard-coded singleShot(1500) exit).
    # The risks they cover are long proven on device; gate behind
    # PERSEPOLIS_OHOS_SELFTEST=1 for occasional sanity runs only.
    from PyQt5 import QtCore, QtWidgets

    app = None
    if os.environ.get('PERSEPOLIS_OHOS_SELFTEST') == '1':
        # Stage A: stdlib C extension modules (R1 sanity)
        import hashlib
        import sqlite3
        import ssl
        log("A: ssl %s / sqlite3 %s" % (ssl.OPENSSL_VERSION, sqlite3.sqlite_version), 1)

        # Stage B: PyQt5 binding (N1 sanity)
        log("B: PyQt5 on Qt %s" % QtCore.QT_VERSION_STR, 1)

        # Stage C: Python threading + cross-thread Qt signal (R7)
        import threading

        app = QtWidgets.QApplication(sys.argv)
        got = {"flag": False}

        def _on(msg):
            got["flag"] = True
            log("C: %s" % msg, 1)

        class _Receiver(QtCore.QObject):
            sig = QtCore.pyqtSignal(str)

        recv = _Receiver()
        recv.sig.connect(_on)

        def _worker():
            import time
            time.sleep(0.3)
            recv.sig.emit("cross-thread-signal-ok")

        threading.Thread(target=_worker, daemon=True).start()
        QtCore.QTimer.singleShot(1500, app.quit)  # early exit for stage C only
        app.exec_()
        if not got["flag"]:
            log("C: FAILED (no cross-thread signal in 1.5s)", 4)
            return 2
        log("C: OK", 1)

    # ---- Stage D: full persepolis startup (mirrors upstream main()) ----
    import platform as _platform
    log("D: platform.system()=%r HOME=%r" % (_platform.system(), os.environ.get("HOME")), 1)

    # P012/A2: bundle-internal executables (ffmpeg etc.) are invisible to
    # upstream's bare-name subprocess calls (ffmpegVersion /
    # findExternalAppPath / muxer all do PATH lookup). Prepend the app libs
    # directory to PATH so packaged binaries are found without touching any
    # upstream call site. LD_LIBRARY_PATH makes the child process resolve
    # its NEEDED .so set (libav*/codec libs, deployed flat, SONAME-named)
    # from the same directory.
    if _platform.system() == 'HarmonyOS':
        _bundle_libs = '/data/storage/el1/bundle/libs/arm64'
        os.environ['PATH'] = _bundle_libs + ':' + os.environ.get('PATH', '')
        os.environ['LD_LIBRARY_PATH'] = _bundle_libs + ':' + os.environ.get('LD_LIBRARY_PATH', '')
        log("D: PATH/LD_LIBRARY_PATH prepended with %s" % _bundle_libs, 1)

        # P012/A1: HAP-installed binaries land WITHOUT the exec bit (hvigor
        # packs from NTFS; installer applies rw-r--r--), so execve of the
        # bundled ffmpeg fails with EACCES. Two-level remedy:
        # 1) copy the executables to the writable app sandbox + chmod 755,
        #    probe-execute, and prepend that dir to PATH - then every
        #    invocation (persepolis muxer AND yt-dlp internals) execs
        #    natively;
        # 2) if sandbox exec is denied (SELinux), fall back to routing
        #    useful_tools.runApplication through the system dynamic loader
        #    (/system/lib/ld-musl-aarch64.so.1 needs no x bit - device
        #    verified, full ffmpeg 9.0.2 banner).
        import shutil as _shutil
        _exec_dir = '/data/storage/el2/base/files/exec'
        _exec_marker = _exec_dir + '/.probe_ok'
        _ohos_native_exec = os.path.exists(_exec_marker)
        try:
            if not os.path.isdir(_exec_dir):
                os.makedirs(_exec_dir)
            for _exe in ('ffmpeg', 'ffprobe'):
                _src = _bundle_libs + '/' + _exe
                _dst = _exec_dir + '/' + _exe
                if os.path.exists(_src):
                    if not os.path.exists(_dst) or os.path.getmtime(_dst) < os.path.getmtime(_src):
                        _shutil.copy2(_src, _dst)
                    os.chmod(_dst, 0o755)
            if not _ohos_native_exec:
                # one-time probe (cached via marker: ffmpeg -version costs
                # ~300ms of library loading, skip on later launches)
                import subprocess as _sp
                _probe = _sp.run([_exec_dir + '/ffmpeg', '-version'],
                                 capture_output=True, timeout=20)
                if _probe.returncode == 0:
                    _ohos_native_exec = True
                    with open(_exec_marker, 'w') as _mf:
                        _mf.write('ok')
                    log("P012: sandbox exec OK - ffmpeg available natively "
                        "(yt-dlp postprocessing covered)", 1)
                else:
                    log("P012: sandbox exec rc=%d" % _probe.returncode, 4)
        except Exception:
            for _line in traceback.format_exc().splitlines():
                log("P012: " + _line, 4)

        if _ohos_native_exec:
            os.environ['PATH'] = _exec_dir + ':' + os.environ['PATH']

        if not _ohos_native_exec:
            # fallback: loader-routed invocation for persepolis's own
            # ffmpeg/ffprobe calls. All upstream call sites (ffmpegVersion,
            # muxer, RunApplicationThread) are internal to useful_tools and
            # resolve runApplication through module globals, so patching the
            # module attribute covers them.
            from persepolis.scripts import useful_tools as _ut_mod
            _orig_run_application = _ut_mod.runApplication
            _ohos_loader = '/system/lib/ld-musl-aarch64.so.1'

            def _run_application_ohos(command_argument):
                if command_argument and command_argument[0] in ('ffmpeg', 'ffprobe'):
                    _argv = [_ohos_loader,
                             _bundle_libs + '/' + command_argument[0]] + list(command_argument[1:])
                    return _orig_run_application(_argv)
                return _orig_run_application(command_argument)

            _ut_mod.runApplication = _run_application_ohos
            log("P012: loader-routed ffmpeg fallback installed "
                "(yt-dlp internal ffmpeg merge NOT covered)", 4)

    # P012/B3: defer the heavyweight yt_dlp import out of the startup path.
    # mainwindow.py / video_finder_addlink.py / ytdlp_downloader.py all do
    # `import yt_dlp` at module top; pre-populating sys.modules with a lazy
    # module makes those imports bind instantly and defers the real load to
    # first attribute access (which happens inside the guarded video finder
    # threads below). Falls back to normal import if the machinery fails.
    try:
        import importlib.util as _ilu
        _ytdlp_spec = _ilu.find_spec('yt_dlp')
        if _ytdlp_spec is not None and _ytdlp_spec.loader is not None:
            _ytdlp_spec.loader = _ilu.LazyLoader(_ytdlp_spec.loader)
            _ytdlp_mod = _ilu.module_from_spec(_ytdlp_spec)
            sys.modules['yt_dlp'] = _ytdlp_mod
            _ytdlp_spec.loader.exec_module(_ytdlp_mod)
            log("D: yt_dlp import deferred (LazyLoader)", 1)
    except Exception:
        for line in traceback.format_exc().splitlines():
            log(line, 4)

    # HiDPI (upstream persepolis.py does this before QApplication creation;
    # MateBook-Pro-class 3K screens need scaling or the window looks tiny)
    os.environ["QT_ENABLE_HIGHDPI_SCALING"] = "1"
    QtWidgets.QApplication.setHighDpiScaleFactorRoundingPolicy(
        QtCore.Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    # first window binds to the system main window automatically
    # (First Window Rule, W2)
    persepolis_app = app if app is not None else QtWidgets.QApplication(sys.argv)

    # initialization writes default QSettings + creates DB + folders.
    from persepolis.scripts import initialization  # noqa: F401
    from persepolis.scripts.mainwindow import MainWindow
    from persepolis.scripts import logger

    # P021 (clipboard): upstream reads QClipboard from a QThread run()
    # body (CheckClipboardStateThread) - Qt only supports clipboard access
    # from the GUI thread; on the OHOS QPA each access marshals through
    # the JS thread bridge and deadlocks/mis-behaves from a worker thread
    # (tray "Import Links from Clipboard" never opened the AddLink window).
    # Replace the whole importLinksFromClipboard entry: run
    # importLinksFromClipboard2 SYNCHRONOUSLY on the main thread (both the
    # tray menu item and the File menu item trigger the same action slot,
    # which is main-thread). This also removes the start()-before-connect()
    # signal race that the thread introduced. CheckClipBoardThread (the
    # periodic clipboard auto-watch, off by default) is disabled too.
    from persepolis.scripts import mainwindow as _mainwindow_mod

    def _import_links_from_clipboard(self, menu=None):
        # show main window (upstream behavior; note showNormal() may not
        # visually un-maximize on the OHOS QPA - tracked as P024)
        self.showMainWindow()
        # read the clipboard and dispatch ON THE MAIN THREAD
        try:
            self.importLinksFromClipboard2()
        except Exception:
            import traceback as _tb
            for _line in _tb.format_exc().splitlines():
                log("P021: " + _line, 4)

    _mainwindow_mod.MainWindow.importLinksFromClipboard = _import_links_from_clipboard

    def _clipboard_watch_run(self):
        log("P021: clipboard auto-watch disabled on HarmonyOS", 1)

    _mainwindow_mod.CheckClipBoardThread.run = _clipboard_watch_run

    # P020: "shutdown/sleep after download" (after_pushButton in MainWindow /
    # ProgressWindow / VideoFinderProgressWindow) runs desktop commands
    # (sudo/shutdown) that don't exist on OHOS - the `os_type != WINDOWS`
    # branch would raise FileNotFoundError inside an unguarded slot, which
    # aborts the whole app on PyQt5 (qFatal, same class as P016/P018), and
    # shutdown.py's platform branches would leave the app stuck in its
    # shutdown wait state. Disable the feature entry points (monkey-patch
    # keeps all OHOS glue here instead of patching three upstream files).
    from persepolis.scripts.progress import ProgressWindow as _ProgressWindow
    from persepolis.scripts.video_finder_progress import VideoFinderProgressWindow as _VFProgressWindow

    def _ohos_unsupported_after(name):
        def _noop(self, *args, **kwargs):
            log("P020: %s is not supported on HarmonyOS" % name, 1)
        return _noop

    _after_noop = _ohos_unsupported_after('shutdown-after-download')
    MainWindow.afterPushButtonPressed = _after_noop
    _ProgressWindow.afterPushButtonPressed = _after_noop
    _VFProgressWindow.afterPushButtonPressed = _after_noop

    # P026: upstream showVideoFinderAddLinkWindow's "yt-dlp/ffmpeg is not
    # installed" branch creates a BARE QMessageBox() (no icon, no parent,
    # exec from the action emission stack) - that combination renders as an
    # EMPTY box on the OHOS QPA (window frame/centering ok, no label/button
    # content; deferring out of the stack did not help). The duplicate-link
    # box in addlink.py callBack (icon + exec) works fine on device, so
    # rebuild the error branch with that proven recipe: icon + parent.
    _orig_show_vf_window = MainWindow.showVideoFinderAddLinkWindow

    def _p026_show_vf_window(self, input_dict=None, menu=None):
        from persepolis.scripts import mainwindow as _mw_mod
        if _mw_mod.youtube_dl_is_installed and _mw_mod.ffmpeg_is_installed:
            _orig_show_vf_window(self, input_dict, menu)
            return

        _parts = []
        if not _mw_mod.youtube_dl_is_installed:
            _parts.append('yt-dlp is not installed!')
        if not _mw_mod.ffmpeg_is_installed:
            _parts.append('ffmpeg is not installed!')

        _box = QtWidgets.QMessageBox(self)
        _box.setIcon(QtWidgets.QMessageBox.Warning)
        _box.setText('\n'.join(_parts))
        _box.setWindowTitle('Error!')
        self._p026_error_box = _box  # keep a reference
        _box.exec_()

    MainWindow.showVideoFinderAddLinkWindow = _p026_show_vf_window

    # P012/B1+B2: the video-finder worker threads are unguarded QThreads -
    # any exception inside (yt-dlp network/parse, ffmpeg muxing, spider)
    # aborts the whole app on PyQt5 (qFatal, P016 class). Wrap their run()
    # with a guard that persists the traceback to the crash log sink. For
    # VideoFinder also try to push an 'error' muxing status to the DB so
    # the progress window reflects the failure.
    from persepolis.scripts.video_finder import VideoFinder as _VideoFinder
    from persepolis.scripts.video_finder_addlink import (
        MediaListFetcherThread as _MediaListFetcherThread,
        FileSizeFetcherThread as _FileSizeFetcherThread)

    def _guard_qthread_run(_cls, _label):
        _orig_run = _cls.run

        def _guarded_run(self):
            try:
                _orig_run(self)
            except Exception:
                import traceback as _tb
                _tb_text = _tb.format_exc()
                log("GUARD: %s.run raised (caught):\n%s" % (_label, _tb_text), 4)
                try:
                    with open('/data/storage/el2/base/files/download-thread-crash.log', 'a') as _f:
                        _f.write('==== %s.run ====\n%s\n' % (_label, _tb_text))
                except Exception:
                    pass
                if _label == 'VideoFinder':
                    try:
                        self.video_finder_dictionary['muxing_status'] = 'error'
                        self.main_window.persepolis_db.updateVideoFinderTable(
                            [self.video_finder_dictionary])
                    except Exception:
                        pass

        _cls.run = _guarded_run

    _guard_qthread_run(_VideoFinder, 'VideoFinder')
    _guard_qthread_run(_MediaListFetcherThread, 'MediaListFetcherThread')
    _guard_qthread_run(_FileSizeFetcherThread, 'FileSizeFetcherThread')

    # P027: upstream video_finder_addlink.py hardcodes '/tmp' as the
    # thumbnail directory on non-Windows (line 60). The OHOS sandbox /tmp
    # is NOT writable - yt-dlp's writethumbnail failed with EACCES
    # ("Permission denied: '/tmp/170.jpg'"), the resulting empty glob then
    # raised a swallowed IndexError and the thumbnail never showed.
    # Redirect to persepolis_tmp (same dir as the yt-dlp cookie jar;
    # VideoFinderAddLink.closeEvent removes the thumbnail file).
    from persepolis.scripts import video_finder_addlink as _vfa_mod
    _vfa_mod.tmp_folder_system = _vfa_mod.persepolis_tmp
    log("P027: video finder thumbnail dir -> %s" % _vfa_mod.persepolis_tmp, 1)

    from PyQt5.QtCore import QFile, QTextStream, QSettings

    persepolis_app.setOrganizationName('com.github.persepolisdm.persepolis')
    persepolis_app.setApplicationName('PersepolisDM')
    persepolis_app.setApplicationVersion('3.0.0-ohos')

    persepolis_setting = QSettings('persepolis_download_manager', 'persepolis')

    # P006: upstream defaults to hamburger-menu mode ('settings/show-menubar'
    # = 'no' persisted by initialization on first run -> mainwindow.py hides
    # QMenuBar and shows toolBar2). The OHOS port defaults the menubar to ON
    # (see patches/useful_tools.py); flip the persisted 'no' once for installs
    # created before the default changed. User toggles afterwards win.
    if persepolis_setting.value('settings/show-menubar', 'no') == 'no' \
            and not persepolis_setting.contains('ohos/menubar-default-migrated'):
        persepolis_setting.setValue('settings/show-menubar', 'yes')
        persepolis_setting.setValue('ohos/menubar-default-migrated', 'yes')
        persepolis_setting.sync()
        log("D: migrated settings/show-menubar no -> yes (OHOS menubar default)", 1)

    # P022 (i18n): upstream persepolis.py main() installs the ui_<locale>
    # QTranslator BEFORE constructing the main window - our launcher mirror
    # missed this step, so the main window (menubar titles, table headers)
    # stayed English while sub-windows (each installing their own
    # translator in __init__) partially translated.
    from PyQt5.QtCore import QTranslator, QLocale, QCoreApplication
    _locale = str(persepolis_setting.value('settings/locale', 'en_US'))
    QLocale.setDefault(QLocale(_locale))
    _translator = QTranslator()
    _loaded = _translator.load(':/translations/locales/ui_' + _locale, 'ts')
    log("P022: translator load ui_%s -> %s" % (_locale, _loaded), 1)
    if _loaded:
        QCoreApplication.installTranslator(_translator)

    # P025 (i18n completeness): the upstream ui_zh_CN.ts is missing a batch
    # of main-window strings (menubar titles, table headers, several menu /
    # tray actions - verified MISSING in the .ts source; desktop shows
    # English for those too). Patch them with a local QTranslator subclass
    # keyed by source text (all entries semantically unambiguous). Category
    # TREE item names (All/Single/Scheduled Downloads) are DATABASE VALUES
    # compared by hardcoded strings throughout the app - they must stay
    # untranslated. Installed AFTER the .qm translator so it takes lookup
    # precedence (translators are searched LIFO); unmatched entries fall
    # through to the .qm.
    # P026 debug switch: PERSEPOLIS_OHOS_LOCAL_ZH=0 disables the local
    # translator to A/B-test the empty-QMessageBox symptom.
    if _locale.startswith('zh') and os.environ.get('PERSEPOLIS_OHOS_LOCAL_ZH', '1') == '1':
        _ZH_STRINGS = {
            # menubar titles ( '&' variants for the QMenuBar, plain for the
            # hamburger MenuWidget )
            '&File': '文件(&F)', 'File': '文件',
            '&Edit': '编辑(&E)', 'Edit': '编辑',
            '&View': '查看(&V)', 'View': '查看',
            '&Download': '下载(&D)', 'Download': '下载',
            '&Queue': '队列(&Q)', 'Queue': '队列',
            'V&ideo Finder': '视频查找(&I)', 'Video Finder': '视频查找',
            '&Help': '帮助(&H)', 'Help': '帮助',
            'Sort by': '排序方式',
            # download table headers + category header + sort submenu
            'File Name': '文件名',
            'Status': '状态',
            'Size': '大小',
            'File Size': '文件大小',
            'Downloaded': '已下载',
            'Percentage': '进度',
            'Connections': '连接数',
            'Transfer Rate': '传输速度',
            'Estimated Time Left': '剩余时间',
            'Link': '链接',
            'First Try Date': '首次尝试日期',
            'Last Try Date': '最后尝试日期',
            'Download Status': '下载状态',
            'Category': '类别',
            # menu / tray actions (upstream .ts has the undotted variants
            # only; the actual action texts carry the '...' suffix)
            'Add New Download Link...': '新建下载链接...',
            'Import Links from Text File...': '从文本文件导入链接...',
            'Import Links from Clipboard...': '从剪贴板导入链接...',
            'Import Links From Clipboard': '从剪贴板导入链接',
            'Find Video Links...': '查找视频链接...',
            'Open Default Download Folder': '打开默认下载文件夹',
            'Stop All Active Downloads': '停止所有活动下载',
            'Minimize to system tray': '最小化到系统托盘',
            'Show main Window': '显示主窗口',
            'Exit': '退出',
            # P025b: upstream ui_zh_CN.ts gap, round 2 - menu items that
            # fell back to English after the None chain fix (Edit/View/
            # Help/Queue menus + table context menu actions)
            'Clear Download List': '清空下载列表',
            'Preferences': '首选项',
            'Show Menubar': '显示菜单栏',
            'Show Side Panel': '显示侧边栏',
            'Show System Tray Icon': '显示系统托盘图标',
            'About': '关于',
            'Report an Issue': '报告问题',
            'Show Log File': '显示日志文件',
            'Create New Queue...': '新建队列...',
            'Start Queue': '启动队列',
            'Stop Queue': '停止队列',
            'Remove Queue': '移除队列',
            'Start this queue': '启动此队列',
            'Stop this queue': '停止此队列',
            'Remove this queue': '移除此队列',
            'Open File...': '打开文件...',
            'Stop Download': '停止下载',
            'Delete Selected Download Files': '删除所选下载文件',
            'Move Selected Download Files to Another Folder...': '将所选下载文件移动到其他文件夹...',
            'Move Selected Download Files to Another Folder': '将所选下载文件移动到其他文件夹',
            'Remove Selected Downloads from List': '从列表移除所选下载',
            'Move Selected Items Up': '上移所选项目',
            'Move Selected Items Down': '下移所选项目',
            'Download video or audio from Youtube, Vimeo, etc.': '从 Youtube、Vimeo 等网站下载视频或音频',
        }

        class _LocalZhTranslator(QTranslator):
            def translate(self, context, sourceText, disambiguation=None, n=-1):
                _t = _ZH_STRINGS.get(sourceText)
                if _t is not None:
                    return _t
                # CRITICAL (P025 regression fix): return None on miss. Qt's
                # translator chain stops at the first NON-NULL result
                # (qcoreapplication.cpp: isNull check, not isEmpty) - an
                # empty-but-non-null QString means "translated to nothing"
                # and BREAKS the chain. super().translate() returns null on
                # C++ side, but PyQt5's str round-trip loses the null-ness
                # ('' -> non-null empty), which blanked every .qm-only menu
                # item. None maps back to a null QString and lets the .qm
                # translator (and the English fallback) run. PC-verified
                # with the real ui_zh_CN.qm: empty->chain broken,
                # None->chain OK.
                return None

        _local_zh = _LocalZhTranslator(persepolis_app)
        QCoreApplication.installTranslator(_local_zh)
        log("P025: local zh translator installed (%d entries)" % len(_ZH_STRINGS), 1)

    # style: Fusion + Dark Fusion QSS (matches upstream default fallback)
    available_styles = list(QtWidgets.QStyleFactory.keys())
    style = str(persepolis_setting.value('settings/style', 'Fusion'))
    color_scheme = str(persepolis_setting.value('settings/color-scheme', 'Dark Fusion'))
    if style not in available_styles:
        style = 'Fusion'
        color_scheme = 'Dark Fusion'
    try:
        persepolis_app.setStyle(style)
    except Exception as e:
        log("D: setStyle(%s) failed: %s" % (style, e), 4)

    # A/B probe resolved: the upstream dark QSS is NOT the cause of the
    # invisible menubar/toolbar (same symptom without it - ohos-style
    # palette issue, cf. SciDAVis menubar-white-text case). Re-enable QSS
    # and append explicit overrides for the affected widgets.
    apply_dark_qss = os.environ.get('PERSEPOLIS_OHOS_QSS', '1') == '1'
    base_qss = ''
    if apply_dark_qss and color_scheme == 'Dark Fusion':
        qss_file = QFile(":/dark_style.qss")
        qss_file.open(QFile.ReadOnly | QFile.Text)
        base_qss = QTextStream(qss_file).readAll()
        qss_file.close()
    elif apply_dark_qss and color_scheme == 'Light Fusion':
        qss_file = QFile(":/light_style.qss")
        qss_file.open(QFile.ReadOnly | QFile.Text)
        base_qss = QTextStream(qss_file).readAll()
        qss_file.close()

    # P014: the OHOS platform theme injects class-specific fonts that dwarf
    # the app default - QMenuBar/QMenu/QMenuItem get makeMenuFont()=16pt and
    # QPushButton gets makePushButtonFont()=16pt (qohosplatformtheme.cpp:916-
    # 920), while the rest of the UI resolves the (smaller) app default font.
    # Re-align the affected classes via QSS to the resolved application
    # default size so it follows the system font settings (proven approach,
    # ark port problem 15; QSS overrides theme font injection - cf. qBT 3/11).
    # NOTE (P019): no colors here - upstream dark_style.qss / light_style.qss
    # carry complete per-scheme palettes for QMenuBar/QMenu/QToolBar/
    # QStatusBar; hard-coded colors here would override the light theme.
    from PyQt5.QtGui import QFontInfo
    app_font_pt = QFontInfo(persepolis_app.font()).pointSize()
    if app_font_pt <= 0:
        app_font_pt = 9
    log("D: app default font %dpt (menu/pushbutton theme font was 16pt)" % app_font_pt, 1)
    ohos_fix_qss = """
QMenuBar { min-height: 28px; font-size: %(pt)dpt; }
QMenuBar::item { font-size: %(pt)dpt; }
QMenu { font-size: %(pt)dpt; }
QMenu::item { font-size: %(pt)dpt; }
QToolBar { min-height: 40px; spacing: 4px; padding: 2px; }
QPushButton { font-size: %(pt)dpt; }
""" % {'pt': app_font_pt}
    persepolis_app.setStyleSheet(base_qss + ohos_fix_qss)

    log("D: launching MainWindow (style=%s, scheme=%s)..." % (style, color_scheme), 1)

    try:
        mainwindow = MainWindow(False, persepolis_app, persepolis_setting)
        # NOTE (P006 postmortem): the menubar was never in "native" mode -
        # OHOS QPlatformTheme has no createPlatformMenuBar(), so
        # isNativeMenuBar() is always false and setNativeMenuBar(False) is a
        # no-op. It was hidden by persepolis's own hamburger-menu default
        # (settings/show-menubar='no', handled by the migration above).

        # P009: OHOS treats every parentless top-level QWidget as a NEW MAIN
        # WINDOW (own taskbar entry, window-management side effects - golden
        # rule W3). persepolis creates ~10 sub-windows (AddLink/Setting/
        # About/Log/Progress/TextQueue/...) as parentless top-level QWidgets.
        # The OHOS QPA routes windows that have a Qt parent + Dialog window
        # type to SubWindow via its syntheticParent path (window-model
        # "Transient Parent" pattern) - equivalent to the C++-only
        # tagWindowOrWidgetAsSubWindowOf(). Adopt them on first show(),
        # BEFORE the platform window is created (W1: tagging/parenting must
        # precede show()/winId()).
        _install_subwindow_adoption(mainwindow)

        mainwindow.show()
        # W4: show() first, then maximize (never start directly fullscreen).
        # Maximizing also fits the desktop expectation on large 2in1 screens.
        mainwindow.showMaximized()
    except Exception:
        for line in traceback.format_exc().splitlines():
            log(line, 4)
        return 1

    log("D: MainWindow shown, entering event loop", 1)

    def _dump_layout():
        from PyQt5.QtGui import QFontInfo
        mb = mainwindow.menuBar()
        log("diag: menuBar visible=%s h=%d actions=%d" % (
            mb.isVisible(), mb.height(), len(mb.actions())))
        log("diag: font pt app=%d menuBar=%d (aligned=%s)" % (
            QFontInfo(persepolis_app.font()).pointSize(),
            QFontInfo(mb.font()).pointSize(),
            QFontInfo(mb.font()).pointSize() == QFontInfo(persepolis_app.font()).pointSize()))
        for tb in mainwindow.findChildren(QtWidgets.QToolBar):
            log("diag: toolBar '%s' visible=%s h=%d actions=%d" % (
                tb.objectName(), tb.isVisible(), tb.height(), len(tb.actions())))
        # P006: icon engine sanity - isNull means SVG plugin missing
        ico = mainwindow.addlinkAction.icon()
        log("diag: addlinkAction icon isNull=%s availableSizes=%d" % (
            ico.isNull(), len(ico.availableSizes())))

    QtCore.QTimer.singleShot(3000, _dump_layout)

    log("=== persepolis launcher.py startup complete ===", 1)
    return persepolis_app.exec_()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except SystemExit:
        raise
    except Exception:
        for line in traceback.format_exc().splitlines():
            log(line, 4)
        sys.exit(1)
