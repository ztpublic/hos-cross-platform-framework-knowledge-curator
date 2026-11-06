'use strict';

const Module = require('module');
const path = require('path');
const fs = require('fs');
const stubs = require('./ohos-native-stubs');
const ohosRg = require('./ohos-rg');

const LOG_TAG = 'TheiaOHOS';
let logPath = path.join(__dirname, 'theia-ohos.log');

function log(event, detail) {
    const extra = detail === undefined ? '' : ` ${typeof detail === 'string' ? detail : JSON.stringify(detail)}`;
    const line = `[${LOG_TAG}] ${new Date().toISOString()} ${event}${extra}\n`;
    try {
        fs.appendFileSync(logPath, line);
    } catch {
        // ignore
    }
    try {
        console.log(line.trim());
    } catch {
        // ignore
    }
}

function isNativeRequest(request) {
    if (typeof request !== 'string') {
        return false;
    }
    const name = request.replace(/\\/g, '/');
    if (name.endsWith('.node')) {
        return true;
    }
    return /^(native-keymap|drivelist|node-pty|keytar|@parcel\/watcher|@parcel\/watcher-)/.test(name)
        || name.includes('native-keymap')
        || name.includes('node-pty')
        || name.includes('@parcel/watcher')
        || name.includes('keytar')
        || name.includes('drivelist');
}

const originalDlopen = process.dlopen.bind(process);

function ohosPtyCandidates() {
    return [
        '/data/storage/el1/bundle/libs/arm64/pty.node',
        '/data/storage/el1/bundle/electron/libs/arm64-v8a/pty.node',
        path.join(__dirname, 'lib', 'prebuilds', 'openharmony-arm64', 'pty.node'),
        path.join(__dirname, 'lib', 'backend', 'native', 'pty.node')
    ];
}

function loadOhosPty() {
    const libDirs = [
        '/data/storage/el1/bundle/libs/arm64',
        path.join(__dirname, 'lib', 'prebuilds', 'openharmony-arm64')
    ];
    const existing = libDirs.filter(dir => {
        try {
            return fs.existsSync(dir);
        } catch {
            return false;
        }
    });
    if (existing.length) {
        process.env.LD_LIBRARY_PATH = existing.join(':') + ':' + (process.env.LD_LIBRARY_PATH || '');
    }
    for (const ptyPath of ohosPtyCandidates()) {
        try {
            if (!fs.existsSync(ptyPath)) {
                continue;
            }
            const mod = { exports: {}, id: ptyPath, filename: ptyPath, loaded: false };
            originalDlopen(mod, ptyPath);
            log('pty-native-loaded', ptyPath);
            return mod.exports;
        } catch (error) {
            log('pty-try-failed', { path: ptyPath, error: String(error) });
        }
    }
    return undefined;
}

let ohosPtyExports;
function getOhosPtyExports() {
    if (ohosPtyExports !== undefined) {
        return ohosPtyExports;
    }
    try {
        ohosPtyExports = loadOhosPty() || null;
    } catch (error) {
        log('pty-native-failed', error && error.stack ? error.stack : String(error));
        ohosPtyExports = null;
    }
    return ohosPtyExports;
}

const originalLoad = Module._load;
Module._load = function (request, parent, isMain) {
    if (isNativeRequest(request)) {
        log('native-intercept', request);
        return stubs.stubForRequest(request);
    }
    return originalLoad.apply(this, arguments);
};

try {
    process.dlopen = function (mod, filename) {
        if (typeof filename === 'string' && filename.replace(/\\/g, '/').endsWith('.node')) {
            log('dlopen-intercept', filename);
            mod.exports = stubs.stubForRequest(filename);
            return;
        }
        return originalDlopen(mod, filename);
    };
} catch (error) {
    log('dlopen-patch-failed', String(error));
}

if (!process.argv.includes('--no-cluster')) {
    process.argv.push('--no-cluster');
}

process.env.THEIA_OHOS = '1';
process.env.THEIA_NO_SPLASH = '1';
if (!process.env.SHELL || process.env.SHELL === '/nonexistent-theia-ohos-shell') {
    process.env.SHELL = '/bin/sh';
}

process.on('uncaughtException', error => {
    log('uncaughtException', error && error.stack ? error.stack : String(error));
});
process.on('unhandledRejection', error => {
    log('unhandledRejection', error && error.stack ? error.stack : String(error));
});

function sanitizeWindowOptions(options) {
    const src = options && typeof options === 'object' ? options : {};
    const webPreferences = Object.assign({}, src.webPreferences || {});
    delete webPreferences.enableDeprecatedPaste;
    delete webPreferences.enableBlinkFeatures;
    delete webPreferences.v8CacheOptions;
    const safe = Object.assign({}, src, {
        width: src.width || 1200,
        height: src.height || 800,
        frame: true,
        transparent: false,
        alwaysOnTop: false,
        webPreferences
    });
    delete safe.titleBarOverlay;
    delete safe.titleBarStyle;
    delete safe.vibrancy;
    delete safe.visualEffectState;
    delete safe.backgroundMaterial;
    delete safe.trafficLightPosition;
    delete safe.roundedCorners;
    delete safe.hiddenInMissionControl;
    log('BrowserWindow-options', {
        width: safe.width,
        height: safe.height,
        show: safe.show,
        preload: webPreferences.preload
    });
    return safe;
}

function patchElectron() {
    const electron = require('electron');
    if (!electron || !electron.app) {
        log('electron-app-missing', {});
        return electron;
    }
    try {
        const userData = electron.app.getPath('userData');
        logPath = path.join(userData, 'theia-ohos.log');
        fs.mkdirSync(userData, { recursive: true });
        log('userData', userData);
    } catch (error) {
        log('userData-failed', String(error));
    }

    electron.app.requestSingleInstanceLock = function () {
        log('polyfill', 'requestSingleInstanceLock');
        return true;
    };
    if (typeof electron.app.setAppUserModelId !== 'function') {
        electron.app.setAppUserModelId = () => undefined;
    }

    try {
        if (electron.nativeTheme) {
            Object.defineProperty(electron.nativeTheme, 'shouldUseDarkColors', {
                configurable: true,
                get() {
                    return true;
                }
            });
            electron.nativeTheme.themeSource = 'dark';
            log('polyfill', 'nativeTheme');
        }
    } catch (error) {
        log('nativeTheme-failed', String(error));
    }

    const fallbackDisplay = {
        id: 0,
        bounds: { x: 0, y: 0, width: 1920, height: 1080 },
        workArea: { x: 0, y: 0, width: 1920, height: 1080 },
        scaleFactor: 1,
        rotation: 0,
        internal: true
    };
    electron.app.once('ready', () => {
        log('app-ready', {});
        try {
            const origAll = electron.screen.getAllDisplays.bind(electron.screen);
            electron.screen.getAllDisplays = function () {
                try {
                    const displays = origAll();
                    log('getAllDisplays', { count: displays && displays.length });
                    return displays && displays.length ? displays : [fallbackDisplay];
                } catch (error) {
                    log('getAllDisplays-failed', String(error));
                    return [fallbackDisplay];
                }
            };
        } catch (error) {
            log('screen-patch-failed', String(error));
        }
    });

        if (electron.Menu) {
        const origSet = electron.Menu.setApplicationMenu && electron.Menu.setApplicationMenu.bind(electron.Menu);
        electron.Menu.setApplicationMenu = function (menu) {
            log('setApplicationMenu', !!menu);
            try {
                return origSet ? origSet(menu) : undefined;
            } catch (error) {
                log('setApplicationMenu-failed', String(error));
            }
        };
    }

    const OrigBrowserWindow = electron.BrowserWindow;
    class OhosBrowserWindow extends OrigBrowserWindow {
        constructor(options) {
            super(sanitizeWindowOptions(options));
        }
    }
    Object.getOwnPropertyNames(OrigBrowserWindow).forEach(key => {
        if (key === 'prototype' || key === 'length' || key === 'name') {
            return;
        }
        try {
            const desc = Object.getOwnPropertyDescriptor(OrigBrowserWindow, key);
            if (desc) {
                Object.defineProperty(OhosBrowserWindow, key, desc);
            }
        } catch {
            // ignore
        }
    });
    try {
        electron.BrowserWindow = OhosBrowserWindow;
        log('polyfill', 'BrowserWindow');
    } catch (error) {
        log('BrowserWindow-assign-failed', String(error));
    }
    return electron;
}

try {
    const cp = require('child_process');
    const origSpawnSync = cp.spawnSync;
    const origExecFileSync = cp.execFileSync;
    const origSpawn = cp.spawn;
    const origExecFile = cp.execFile;
    const origFork = cp.fork;

    function spawnArgs(command, args, options) {
        if (Array.isArray(args)) {
            return { args, options: options || {} };
        }
        return { args: [], options: args && typeof args === 'object' ? args : {} };
    }

    cp.spawn = function (command, args, options) {
        if (ohosRg.isRipgrepCommand(command)) {
            const parsed = spawnArgs(command, args, options);
            log('rg-spawn', parsed.args);
            return ohosRg.spawnRipgrep(parsed.args, parsed.options);
        }
        return origSpawn.apply(this, arguments);
    };
    cp.execFile = function (command, args, options, callback) {
        if (ohosRg.isRipgrepCommand(command)) {
            const parsed = spawnArgs(command, args, options);
            log('rg-execFile', parsed.args);
            return ohosRg.spawnRipgrep(parsed.args, parsed.options);
        }
        return origExecFile.apply(this, arguments);
    };
    cp.fork = function (modulePath, args, options) {
        log('fork', { modulePath, argsCount: Array.isArray(args) ? args.length : 0 });
        return origFork.apply(this, arguments);
    };

    cp.execFileSync = function (command) {
        const cmd = String(command || '');
        if (/sh$|zsh$|bash$|shell/.test(cmd) && !ohosRg.isRipgrepCommand(cmd)) {
            const pathEnv = process.env.PATH || '/system/bin:/usr/bin:/bin';
            log('execFileSync-stub', cmd);
            return Buffer.from(`PATH=${pathEnv}\n`);
        }
        return origExecFileSync.apply(this, arguments);
    };
    cp.spawnSync = function (command) {
        const cmd = String(command || '');
        if (/sh$|zsh$|bash$|shell/.test(cmd) && !ohosRg.isRipgrepCommand(cmd)) {
            const pathEnv = process.env.PATH || '/system/bin:/usr/bin:/bin';
            log('spawnSync-stub', cmd);
            return {
                status: 0,
                stdout: `PATH=${pathEnv}\n`,
                stderr: '',
                pid: 0,
                signal: null,
                error: undefined,
                output: [null, `PATH=${pathEnv}\n`, '']
            };
        }
        return origSpawnSync.apply(this, arguments);
    };
    log('polyfill', 'spawnSync');
} catch (error) {
    log('spawnSync-patch-failed', String(error));
}

try {
    patchElectron();
} catch (error) {
    log('electron-patch-failed', error && error.stack ? error.stack : String(error));
}

log('ohos-main', {
    pid: process.pid,
    platform: process.platform,
    arch: process.arch,
    electron: process.versions && process.versions.electron,
    node: process.versions && process.versions.node,
    modules: process.versions && process.versions.modules,
    execPath: process.execPath,
    argv: process.argv
});

const frontendHtml = path.join(__dirname, 'lib', 'frontend', 'index.html');
const preloadJs = path.join(__dirname, 'lib', 'frontend', 'preload.js');
const backendMain = path.join(__dirname, 'lib', 'backend', 'main.js');
if (!fs.existsSync(frontendHtml)) {
    log('missing-frontend', frontendHtml);
    throw new Error(`Theia frontend missing: ${frontendHtml}`);
}

const { pathToFileURL } = require('node:url');
const crypto = require('node:crypto');
const electron = require('electron');
let userDataDir = path.join(__dirname, 'data');
try {
    userDataDir = electron.app.getPath('userData');
} catch {
    // keep fallback
}

// HarmonyOS Electron 点关闭会把页面卸载后再加载同一 URL，窗口不销毁、进程不退。
// 关闭必须走 app.exit，不能只调 BrowserWindow.close()。
let quitting = false;
let closeInProgress = false;
let intentionalReload = false;
let frontendReady = false;
let closeBounceTimer;

function remainingWindows() {
    try {
        return electron.BrowserWindow.getAllWindows().filter(win => {
            try {
                return !win.isDestroyed();
            } catch {
                return false;
            }
        });
    } catch {
        return [];
    }
}

function closeAllWindows() {
    for (const win of remainingWindows()) {
        try {
            if (!win.isDestroyed()) {
                win.close();
            }
        } catch (error) {
            log('close-window-failed', String(error));
        }
    }
}

function quitTheia(reason) {
    if (quitting) {
        log('quit-already', { reason, pid: process.pid });
        return;
    }
    quitting = true;
    closeInProgress = false;
    log('quit', { reason, pid: process.pid });
    // app.quit() 在鸿蒙宿主上会卡住事件循环，setTimeout 杀进程也跑不到。
    // 先放行 BrowserWindow.close（宿主 AppWindow.CloseWindow → terminateSelf），再 SIGKILL。
    closeAllWindows();
    try {
        process.kill(process.pid, 'SIGKILL');
        log('sigkill-sent', { pid: process.pid });
    } catch (error) {
        log('sigkill-failed', String(error));
    }
    try {
        process.exit(0);
    } catch {
        // ignore
    }
}

function requestRendererClose(win) {
    return new Promise(resolve => {
        if (!win || win.isDestroyed() || !win.webContents || win.webContents.isDestroyed()) {
            resolve(true);
            return;
        }
        const id = Date.now();
        const confirmChannel = `ohos-confirm-close-${id}`;
        const cancelChannel = `ohos-cancel-close-${id}`;
        let settled = false;
        const finish = result => {
            if (settled) {
                return;
            }
            settled = true;
            clearTimeout(timer);
            try {
                electron.ipcMain.removeAllListeners(confirmChannel);
                electron.ipcMain.removeAllListeners(cancelChannel);
            } catch {
                // ignore
            }
            resolve(result);
        };
        const timer = setTimeout(() => {
            log('close-handshake-timeout', {});
            finish(true);
        }, 12000);
        electron.ipcMain.once(confirmChannel, () => {
            log('close-handshake-confirm', {});
            finish(true);
        });
        electron.ipcMain.once(cancelChannel, () => {
            log('close-handshake-cancel', {});
            finish(false);
        });
        try {
            // StopReason.Close === 0
            win.webContents.send('RequestClose', 0, confirmChannel, cancelChannel);
            log('RequestClose-sent', {});
        } catch (error) {
            log('RequestClose-failed', String(error));
            finish(true);
        }
    });
}

async function beginClose(win, source) {
    if (quitting) {
        return;
    }
    if (closeInProgress) {
        log('close-already-in-progress', source);
        return;
    }
    closeInProgress = true;
    log('begin-close', source);
    let allow = true;
    try {
        if (frontendReady) {
            allow = await requestRendererClose(win);
        }
    } catch (error) {
        log('begin-close-failed', String(error));
        allow = true;
    }
    if (!allow) {
        closeInProgress = false;
        log('close-cancelled', source);
        return;
    }
    quitTheia(source);
}

function attachWindowLifetime(win) {
    if (!win || win._ohosLifetimeAttached) {
        return;
    }
    win._ohosLifetimeAttached = true;
    win.on('close', event => {
        log('window-close-event', { quitting, closeInProgress });
        if (quitting) {
            return;
        }
        event.preventDefault();
        beginClose(win, 'window-close').catch(error => log('begin-close-catch', String(error)));
    });
    win.on('closed', () => {
        const left = remainingWindows().length;
        log('window-closed-event', { remaining: left, quitting });
        if (!quitting && left === 0) {
            quitTheia('window-closed');
        }
    });
}

try {
    electron.app.on('browser-window-created', (_event, win) => {
        attachWindowLifetime(win);
    });
    electron.app.on('window-all-closed', () => {
        log('window-all-closed', { pid: process.pid, quitting });
        quitTheia('window-all-closed');
    });
    electron.app.on('before-quit', () => {
        quitting = true;
        log('before-quit', {});
    });
} catch (error) {
    log('app-lifetime-hook-failed', String(error));
}

const theiaHome = path.join(userDataDir, 'home');
const theiaConfig = path.join(userDataDir, '.theia');
const workspaceDir = path.join(userDataDir, 'workspace');
try {
    fs.mkdirSync(theiaHome, { recursive: true });
    fs.mkdirSync(theiaConfig, { recursive: true });
    fs.mkdirSync(workspaceDir, { recursive: true });
    const sample = path.join(workspaceDir, 'hello.md');
    fs.writeFileSync(sample, [
        '# Theia on HarmonyOS PC',
        '',
        'hello.md for M1 open/edit/save.',
        '',
        'ohos-search-token-74',
        ''
    ].join('\n'));
    fs.writeFileSync(path.join(workspaceDir, 'sample.js'), [
        'function helloOhos() {',
        "    return 'ohos-search-token-74';",
        '}',
        '',
        'module.exports = { helloOhos };',
        ''
    ].join('\n'));
    fs.writeFileSync(path.join(workspaceDir, 'notes.txt'), 'probe notes ohos-search-token-74\n');
} catch (error) {
    log('mkdir-home-failed', String(error));
}
try {
    const desktopProbe = '/storage/Users/currentUser/Desktop/theia-ohos-probe.txt';
    fs.writeFileSync(desktopProbe, 'ohos-search-token-74\n');
    log('desktop-probe-written', desktopProbe);
} catch (error) {
    log('desktop-probe-failed', String(error));
}
process.env.HOME = theiaHome;
process.env.USER = process.env.USER || 'theia';
process.env.THEIA_CONFIG_DIR = theiaConfig;
try {
    require('os').homedir = () => theiaHome;
} catch {
    // ignore
}
log('home', { HOME: process.env.HOME, THEIA_CONFIG_DIR: process.env.THEIA_CONFIG_DIR, workspaceDir });

function probeFork() {
    const childPath = path.join(__dirname, 'fork-child.js');
    if (!fs.existsSync(childPath)) {
        log('fork-probe-skip', 'missing fork-child.js');
        return;
    }
    try {
        const { fork } = require('child_process');
        const child = fork(childPath, [], { stdio: ['ignore', 'pipe', 'pipe', 'ipc'], timeout: 8000 });
        let stdout = '';
        let stderr = '';
        if (child.stdout) {
            child.stdout.on('data', chunk => {
                stdout += String(chunk);
            });
        }
        if (child.stderr) {
            child.stderr.on('data', chunk => {
                stderr += String(chunk);
            });
        }
        const timer = setTimeout(() => {
            try {
                child.kill();
            } catch {
                // ignore
            }
            log('fork-probe-timeout', { pid: child.pid, stdout, stderr });
        }, 8000);
        child.on('message', message => log('fork-probe-message', message));
        child.on('exit', (code, signal) => {
            clearTimeout(timer);
            log('fork-probe-exit', { code, signal, pid: child.pid, stdout: stdout.slice(0, 500), stderr: stderr.slice(0, 500) });
        });
        child.on('error', error => {
            clearTimeout(timer);
            log('fork-probe-error', String(error));
        });
        log('fork-probe-started', { pid: child.pid, childPath });
    } catch (error) {
        log('fork-probe-failed', error && error.stack ? error.stack : String(error));
    }
}
probeFork();

async function runFeatureProbe(contents) {
    const probePath = path.join(__dirname, 'ohos-feature-probe.js');
    if (!fs.existsSync(probePath)) {
        log('feature-probe-skip', 'missing ohos-feature-probe.js');
        return;
    }
    try {
        const code = fs.readFileSync(probePath, 'utf8');
        const result = await contents.executeJavaScript(code, true);
        log('feature-probe', result);
        try {
            fs.writeFileSync(path.join(userDataDir, 'feature-probe.json'), JSON.stringify(result, null, 2));
        } catch (error) {
            log('feature-probe-write-failed', String(error));
        }
    } catch (error) {
        log('feature-probe-failed', error && error.stack ? error.stack : String(error));
    }
}

function registerTheiaIpc(electron) {
    const { ipcMain, BrowserWindow, clipboard, dialog, shell } = electron;
    const token = { value: require('node:crypto').randomUUID() };

    ipcMain.on('WebContentMetadata', event => {
        event.returnValue = String(event.sender.id);
    });
    ipcMain.on('GetSecurityToken', event => {
        event.returnValue = token.value;
    });
    ipcMain.handle('AttachSecurityToken', async (_event, endpoint) => {
        try {
            await electron.session.defaultSession.cookies.set({
                url: endpoint,
                name: 'THEIA_ELECTRON_TOKEN',
                value: JSON.stringify(token),
                httpOnly: true,
                sameSite: 'no_restriction'
            });
        } catch (error) {
            log('AttachSecurityToken-failed', String(error));
        }
    });
    ipcMain.handle('GetTitleStyleAtStartup', () => 'native');
    ipcMain.handle('GetZoomLevel', event => {
        try {
            return event.sender.getZoomLevel();
        } catch {
            return 0;
        }
    });
    ipcMain.handle('OpenPopup', () => 0);
    ipcMain.handle('ClosePopup', () => undefined);
    ipcMain.handle('ShowOpenDialog', async (_event, options) => {
        try {
            const properties = [];
            if (options && options.openFiles) {
                properties.push('openFile');
            }
            if (options && options.openFolders) {
                properties.push('openDirectory');
            }
            const result = await dialog.showOpenDialog({
                title: options && options.title,
                defaultPath: options && options.path,
                properties: properties.length ? properties : ['openDirectory']
            });
            return result.canceled ? [] : result.filePaths;
        } catch (error) {
            log('ShowOpenDialog-failed', String(error));
            return [];
        }
    });
    ipcMain.handle('ShowSaveDialog', async (_event, options) => {
        try {
            const result = await dialog.showSaveDialog({
                title: options && options.title,
                defaultPath: options && options.path
            });
            return result.canceled ? undefined : result.filePath;
        } catch (error) {
            log('ShowSaveDialog-failed', String(error));
            return undefined;
        }
    });

    const syncBool = (event, fn) => {
        try {
            const win = BrowserWindow.fromWebContents(event.sender);
            event.returnValue = win ? fn(win) : false;
        } catch {
            event.returnValue = false;
        }
    };
    ipcMain.on('IsMaximized', event => syncBool(event, w => w.isMaximized()));
    ipcMain.on('IsFullScreenable', event => syncBool(event, w => w.isFullScreenable()));
    ipcMain.on('IsFullScreen', event => syncBool(event, w => w.isFullScreen()));
    ipcMain.on('ReadClipboard', event => {
        try {
            event.returnValue = clipboard.readText();
        } catch {
            event.returnValue = '';
        }
    });
    ipcMain.on('WriteClipboard', (_event, text) => {
        try {
            clipboard.writeText(String(text || ''));
        } catch (error) {
            log('WriteClipboard-failed', String(error));
        }
    });

    const windowOp = (event, fn) => {
        try {
            const win = BrowserWindow.fromWebContents(event.sender);
            if (win) {
                fn(win);
            }
        } catch (error) {
            log('window-op-failed', String(error));
        }
    };
    ipcMain.on('Minimize', event => windowOp(event, w => w.minimize()));
    ipcMain.on('Maximize', event => windowOp(event, w => w.maximize()));
    ipcMain.on('UnMaximize', event => windowOp(event, w => w.unmaximize()));
    ipcMain.on('Close', event => {
        log('ipc-Close', {});
        const win = BrowserWindow.fromWebContents(event.sender);
        beginClose(win, 'ipc-Close').catch(error => log('begin-close-catch', String(error)));
    });
    ipcMain.on('ToggleFullScreen', event => windowOp(event, w => w.setFullScreen(!w.isFullScreen())));
    ipcMain.on('SetMenuBarVisible', (event, vis) => windowOp(event, w => {
        if (typeof w.setMenuBarVisibility === 'function') {
            w.setMenuBarVisibility(!!vis);
        }
    }));
    ipcMain.on('SetMenu', (event, _menuId, menu) => {
        try {
            const win = BrowserWindow.fromWebContents(event.sender);
            if (win && typeof win.setMenu === 'function' && !menu) {
                win.setMenu(null);
            }
        } catch (error) {
            log('SetMenu-failed', String(error));
        }
    });
    ipcMain.on('FocusWindow', event => windowOp(event, w => w.focus()));
    ipcMain.on('ToggleDevtools', event => {
        try {
            event.sender.toggleDevTools();
        } catch (error) {
            log('ToggleDevtools-failed', String(error));
        }
    });
    ipcMain.on('SetZoomLevel', (event, level) => {
        try {
            event.sender.setZoomLevel(level);
        } catch (error) {
            log('SetZoomLevel-failed', String(error));
        }
    });
    ipcMain.on('SetTitleStyle', () => undefined);
    ipcMain.on('SetBackgroundColor', (event, color) => {
        try {
            BrowserWindow.fromWebContents(event.sender)?.setBackgroundColor(color);
        } catch {
            // ignore
        }
    });
    ipcMain.on('SetTheme', () => undefined);
    ipcMain.on('ShowItemInFolder', (_event, fsPath) => {
        try {
            shell.showItemInFolder(fsPath);
        } catch (error) {
            log('ShowItemInFolder-failed', String(error));
        }
    });
    ipcMain.on('OpenWithSystemApp', (_event, uri) => {
        try {
            shell.openExternal(uri);
        } catch (error) {
            log('OpenWithSystemApp-failed', String(error));
        }
    });
    ipcMain.on('ApplicationStateChanged', (_event, state) => {
        log('app-state', state);
        if (state === 'closing_window' && !intentionalReload && !quitting) {
            clearTimeout(closeBounceTimer);
            closeBounceTimer = setTimeout(() => {
                if (!quitting && !intentionalReload) {
                    log('close-bounce', 'page unloaded but process still alive');
                    quitTheia('close-bounce');
                }
            }, 1500);
        }
        if (state === 'ready') {
            frontendReady = true;
            if (closeBounceTimer && !intentionalReload && !quitting) {
                clearTimeout(closeBounceTimer);
                closeBounceTimer = undefined;
                log('reloaded-after-close', {});
                quitTheia('reloaded-after-close');
                return;
            }
            intentionalReload = false;
            clearTimeout(closeBounceTimer);
            closeBounceTimer = undefined;
            if (!_event.sender._ohosProbeStarted) {
                _event.sender._ohosProbeStarted = true;
                setTimeout(() => {
                    runFeatureProbe(_event.sender).catch(error => log('feature-probe-catch', String(error)));
                }, 4000);
            }
        }
    });
    ipcMain.on('RequestReload', (event, newUrl) => {
        intentionalReload = true;
        log('RequestReload', newUrl || '');
        try {
            if (newUrl) {
                event.sender.loadURL(newUrl);
            } else {
                event.sender.reload();
            }
        } catch (error) {
            intentionalReload = false;
            log('RequestReload-failed', String(error));
        }
    });
    ipcMain.on('Restart', event => {
        intentionalReload = true;
        log('Restart', {});
        windowOp(event, w => w.reload());
    });
    ipcMain.on('IpcConnection', () => undefined);
    ipcMain.on('UpdateRecentWorkspaces', () => undefined);
    log('polyfill', 'theia-ipc');
    return token;
}

function waitForFinishLoad(contents, timeoutMs) {
    return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
            contents.removeListener('did-finish-load', onOk);
            contents.removeListener('did-fail-load', onFail);
            reject(new Error(`did-finish-load timeout ${timeoutMs}ms url=${contents.getURL()}`));
        }, timeoutMs);
        const onOk = () => {
            clearTimeout(timer);
            contents.removeListener('did-fail-load', onFail);
            resolve(contents.getURL());
        };
        const onFail = (_event, code, desc, url) => {
            clearTimeout(timer);
            contents.removeListener('did-finish-load', onOk);
            reject(new Error(`did-fail-load ${code} ${desc} ${url}`));
        };
        contents.once('did-finish-load', onOk);
        contents.once('did-fail-load', onFail);
    });
}

electron.app.whenReady().then(async () => {
    log('app-ready-show-frontend', frontendHtml);
    registerTheiaIpc(electron);
    const window = new electron.BrowserWindow({
        width: 1280,
        height: 800,
        title: 'Theia',
        backgroundColor: '#1e1e1e',
        webPreferences: {
            preload: fs.existsSync(preloadJs) ? preloadJs : undefined,
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: false,
            webSecurity: false
        }
    });
    attachWindowLifetime(window);
    window.webContents.on('did-fail-load', (_event, code, desc, url) => {
        log('did-fail-load', { code, desc, url });
    });
    window.webContents.on('did-finish-load', () => {
        log('did-finish-load', window.webContents.getURL());
    });
    window.webContents.on('console-message', (_event, level, message) => {
        log('renderer', String(message).slice(0, 500));
    });

    if (!fs.existsSync(backendMain)) {
        throw new Error(`Theia backend missing: ${backendMain}`);
    }
    log('require-backend', backendMain);
    require(backendMain);
    log('backend-returned', {});
    const address = await globalThis.serverAddress;
    log('backend-address', address);
    const port = address && address.port;
    if (!port) {
        throw new Error('backend did not bind a port');
    }

    try {
        const token = { value: crypto.randomUUID() };
        await electron.session.defaultSession.cookies.set({
            url: `http://localhost:${port}`,
            name: 'THEIA_ELECTRON_TOKEN',
            value: JSON.stringify(token),
            httpOnly: true,
            sameSite: 'no_restriction'
        });
        log('security-token-cookie', { port });
    } catch (error) {
        log('security-token-failed', String(error));
    }

    // Electron 工作台约定：file://index.html?port=<backend>，WebSocket 连 localhost:port。
    // 不要用 http://127.0.0.1（慢、且会丢掉 ?port 查询串）。
    const fileUrl = `${pathToFileURL(frontendHtml).href}?port=${port}`;
    log('load-file-url', fileUrl);
    const pending = waitForFinishLoad(window.webContents, 120000);
    await window.loadURL(fileUrl);
    const loadedUrl = await pending;
    log('frontend-loaded', loadedUrl);
}).catch(error => {
    log('whenReady-failed', error && error.stack ? error.stack : String(error));
});
