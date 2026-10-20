/////////////////////////////////////////////////////////////
//
// JupyterLab - HarmonyOS PC Electron runtime
//
// Main process. Two modes:
//   * local-server  : spawn the on-device Python / node-static server, then
//                     load the JupyterLab UI
//                     when it is ready.
//   * external      : connect to an already-running Jupyter Server URL.
//
// Product identity is JupyterLab; the node-static path reuses the
// proven JupyterLab frontend assets (Notebook 7 is Lab-based).
//
//////////////////////////////////////////////////////////////

import { app, BrowserWindow, dialog, shell, Menu } from 'electron';
import { spawn } from 'child_process';
import http from 'http';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

import {
  isOpenHarmony,
  isTruthyEnv,
  shouldDisableGpu,
  shouldSpawnLocalServer,
  getExternalServerUrl,
} from './platform.js';
import {
  resolvePythonPath,
  resolveServerPort,
  resolveToken,
  resolveRootDir,
  resolvePyLibsDir,
  resolvePipIndexUrl,
} from './serverConfig.js';
import { startStaticLabServer } from './staticLabServer.js';

function envFirst(...keys) {
  for (const key of keys) {
    const value = process.env[key];
    if (value !== undefined && value !== null && String(value) !== '') {
      return value;
    }
  }
  return undefined;
}

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const appDir = path.resolve(__dirname, '..', '..'); // .../resources/app
const assetIcon = path.join(appDir, 'assets', 'jupyterlab.png');

const openHarmony = isOpenHarmony();
const spawnLocal = shouldSpawnLocalServer();
const PYLIBS_MIRROR_VERSION = 2;
const PYTHON_MIRROR_VERSION = 1;
const RUNTIME_KERNELSPEC_VERSION = 1;
const OHOS_BUNDLE_NAME = 'org.jupyter.lab.ohos';
const HNP_PYTHON_PACKAGE_NAME = 'jupyterlab_python';
const HNP_PYTHON_PACKAGE_VERSION = '1.1';

let mainWindow = null;
let serverProcess = null;
let nodeStaticServer = null;
let startUrl = null;
let logStream = null;
let serverReady = false;
let startupHandled = false;
let serverPythonPath = null;
let fallbackStarted = false;
const serverOutputTail = [];

function rememberServerOutput(text) {
  for (const line of String(text).split('\n')) {
    if (!line) continue;
    serverOutputTail.push(line);
  }
  while (serverOutputTail.length > 40) {
    serverOutputTail.shift();
  }
}

// ----------------------------------------------------------------------------
// GPU / single instance
// ----------------------------------------------------------------------------
if (shouldDisableGpu()) {
  try {
    app.commandLine.appendSwitch('disable-gpu');
    app.commandLine.appendSwitch('disable-gpu-compositing');
    app.disableHardwareAcceleration();
  } catch (error) {
    console.warn('Unable to disable GPU acceleration.', error);
  }
}

const gotLock = app.requestSingleInstanceLock();
if (!gotLock) {
  app.quit();
} else {
  app.on('second-instance', () => {
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.focus();
    }
  });
}

// ----------------------------------------------------------------------------
// Logging
// ----------------------------------------------------------------------------
function writeLog(message) {
  const line = `[${new Date().toISOString()}] ${message}\n`;
  try {
    if (!logStream) {
      const logDir = app.getPath('userData');
      fs.mkdirSync(logDir, { recursive: true });
      logStream = fs.createWriteStream(path.join(logDir, 'jupyterlab-runtime.log'), { flags: 'a' });
    }
    logStream.write(line);
  } catch {
    /* ignore log failures */
  }
  console.info(message.trimEnd());
}

function readJsonFile(file) {
  try {
    return JSON.parse(fs.readFileSync(file, 'utf8'));
  } catch {
    return null;
  }
}

function fingerprintPyLibs(dir) {
  const summary = {
    fileCount: 0,
    totalBytes: 0,
    distInfo: [],
    sharedObjects: [],
  };

  function walk(current, relative = '') {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const full = path.join(current, entry.name);
      const rel = relative ? path.join(relative, entry.name) : entry.name;
      if (entry.isDirectory()) {
        if (!relative && entry.name.endsWith('.dist-info')) {
          summary.distInfo.push(entry.name);
        }
        walk(full, rel);
        continue;
      }
      if (!entry.isFile()) {
        continue;
      }
      const stat = fs.statSync(full);
      summary.fileCount += 1;
      summary.totalBytes += stat.size;
      if (entry.name.endsWith('.so')) {
        summary.sharedObjects.push(`${rel}:${stat.size}`);
      }
    }
  }

  walk(dir);
  summary.distInfo.sort();
  summary.sharedObjects.sort();
  return summary;
}

function sameFingerprint(a, b) {
  return JSON.stringify(a) === JSON.stringify(b);
}

function isNativeBinaryName(name) {
  return name.endsWith('.so') || name.includes('.so.');
}

function readNativeExtensionManifest() {
  return (
    readJsonFile(path.join(appDir, 'python-native-extensions.json')) || {
      version: 1,
      modules: {},
      preload: [],
    }
  );
}

function nativeLibDirCandidates() {
  const candidates = [];
  if (envFirst('JUPYTERLAB_OHOS_NATIVE_LIB_DIR', 'NOTEBOOK_OHOS_NATIVE_LIB_DIR')) {
    candidates.push(envFirst('JUPYTERLAB_OHOS_NATIVE_LIB_DIR', 'NOTEBOOK_OHOS_NATIVE_LIB_DIR'));
  }

  // HAP resources are exposed as .../bundle/resources/resfile/resources/app;
  // native libraries are exposed beside resources as .../bundle/libs/arm64.
  const bundleRoot = path.resolve(appDir, '..', '..', '..', '..');
  candidates.push(path.join(bundleRoot, 'libs', 'arm64'));
  candidates.push(path.join(bundleRoot, 'libs', 'arm64-v8a'));
  candidates.push('/data/storage/el1/bundle/libs/arm64');
  candidates.push('/data/storage/el1/bundle/libs/arm64-v8a');

  return [...new Set(candidates)];
}

function resolveNativeLibDir() {
  const candidates = nativeLibDirCandidates();
  for (const candidate of candidates) {
    try {
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    } catch {
      /* ignore unreadable candidates */
    }
  }
  return candidates[0];
}

function bundledPythonHomeDir() {
  const candidate = path.join(appDir, 'python');
  return fs.existsSync(path.join(candidate, 'bin', 'python3')) ? candidate : '';
}

function hnpPythonHomeCandidates() {
  const candidates = [];
  if (envFirst('JUPYTERLAB_OHOS_HNP_PYTHON_HOME', 'NOTEBOOK_OHOS_HNP_PYTHON_HOME')) {
    candidates.push(envFirst('JUPYTERLAB_OHOS_HNP_PYTHON_HOME', 'NOTEBOOK_OHOS_HNP_PYTHON_HOME'));
  }
  const hnpInstall = `${HNP_PYTHON_PACKAGE_NAME}_${HNP_PYTHON_PACKAGE_VERSION}`;
  for (const base of [
    process.env.HNP_PRIVATE_HOME,
    process.env.HNP_PUBLIC_HOME,
    `/data/app/${OHOS_BUNDLE_NAME}`,
    `/data/app/el1/bundle/public/${OHOS_BUNDLE_NAME}`,
    `/data/app/el1/bundle/0/hnp/${OHOS_BUNDLE_NAME}`,
  ].filter(Boolean)) {
    candidates.push(path.join(base, `${HNP_PYTHON_PACKAGE_NAME}.org`, hnpInstall));
    candidates.push(path.join(base, hnpInstall));
    candidates.push(
      path.join(base, `${HNP_PYTHON_PACKAGE_NAME}.org`, hnpInstall, 'bin', HNP_PYTHON_PACKAGE_NAME, 'python'),
    );
    candidates.push(path.join(base, hnpInstall, 'bin', HNP_PYTHON_PACKAGE_NAME, 'python'));
    candidates.push(path.join(base, 'bin', HNP_PYTHON_PACKAGE_NAME, 'python'));
  }
  return [...new Set(candidates)];
}

function bundledHnpPythonHomeDir() {
  if (!openHarmony) {
    return '';
  }
  for (const candidate of hnpPythonHomeCandidates()) {
    try {
      if (fs.existsSync(path.join(candidate, 'bin', 'python3'))) {
        return candidate;
      }
    } catch {
      /* ignore unreadable candidates */
    }
  }
  for (const root of [process.env.HNP_PRIVATE_HOME, process.env.HNP_PUBLIC_HOME].filter(Boolean)) {
    const discovered = findHnpPythonHome(root);
    if (discovered) {
      return discovered;
    }
  }
  return '';
}

function scanHnpPythonHomes(root) {
  const seen = new Set();
  const discoveredHomes = [];
  const queue = [{ dir: root, depth: 0 }];
  while (queue.length > 0) {
    const { dir, depth } = queue.shift();
    if (!dir || seen.has(dir) || depth > 8) {
      continue;
    }
    seen.add(dir);
    let entries;
    try {
      entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch {
      continue;
    }
    const python = path.join(dir, 'bin', 'python3');
    if (fs.existsSync(python)) {
      discoveredHomes.push(dir);
    }
    for (const entry of entries) {
      if (entry.isDirectory()) {
        queue.push({ dir: path.join(dir, entry.name), depth: depth + 1 });
      }
    }
  }
  return [...new Set(discoveredHomes)];
}

function findHnpPythonHome(root) {
  const discoveredHomes = scanHnpPythonHomes(root);
  const bundledHome = discoveredHomes.find((home) => home.includes(HNP_PYTHON_PACKAGE_NAME)) || '';
  if (bundledHome) {
    writeLog(`Discovered bundled HNP Python by scanning ${root}: ${bundledHome}`);
    const otherHomes = discoveredHomes.filter((home) => home !== bundledHome);
    if (otherHomes.length > 0) {
      writeLog(`Other HNP Python homes under ${root}: ${otherHomes.join(', ')}`);
    }
    return bundledHome;
  }
  if (discoveredHomes.length > 0) {
    writeLog(`Other HNP Python homes under ${root}: ${discoveredHomes.join(', ')}`);
  }
  return '';
}

function hnpSystemPythonHomeCandidates() {
  if (!openHarmony) {
    return [];
  }
  const candidates = [];
  for (const base of [
    process.env.HNP_PUBLIC_HOME,
    process.env.HNP_PRIVATE_HOME,
    `/data/app/${OHOS_BUNDLE_NAME}`,
    `/data/app/el1/bundle/public/${OHOS_BUNDLE_NAME}`,
    `/data/app/el1/bundle/0/hnp/${OHOS_BUNDLE_NAME}`,
  ].filter(Boolean)) {
    candidates.push(path.join(base, 'python.org', 'python_3.12'));
    candidates.push(path.join(base, 'python_3.12'));
    candidates.push(path.join(base, 'ohos-sdk.org', 'ohos-sdk_26.0.0.18', 'ohos', 'native', 'llvm', 'python3'));
    try {
      candidates.push(...scanHnpPythonHomes(base));
    } catch {
      /* ignore unreadable HNP roots */
    }
  }
  return [...new Set(candidates)].filter((home) => !home.includes(HNP_PYTHON_PACKAGE_NAME));
}

function fileExists(file) {
  try {
    return Boolean(file) && fs.existsSync(file);
  } catch {
    return false;
  }
}

function bundledPythonLibraryDirs(pythonHome) {
  return [
    pythonHome && path.join(pythonHome, 'lib'),
    pythonHome && path.join(pythonHome, 'lib', 'external'),
  ].filter((candidate) => candidate && fs.existsSync(candidate));
}

function hnpLinkedPythonPath(pythonHome) {
  if (!openHarmony || !pythonHome || !pythonHome.includes(HNP_PYTHON_PACKAGE_NAME)) {
    return '';
  }
  const linked = path.join(process.env.HNP_PUBLIC_HOME || '/data/service/hnp', 'bin', 'jupyterlab-python3');
  try {
    return fs.existsSync(linked) ? linked : '';
  } catch {
    return '';
  }
}

function addPythonLaunchCandidate(candidates, label, pythonPath, pythonHome = '') {
  if (!pythonPath) {
    return;
  }
  if (path.isAbsolute(pythonPath) && !fileExists(pythonPath)) {
    return;
  }
  if (pythonHome && !fileExists(path.join(pythonHome, 'bin', 'python3'))) {
    return;
  }
  const key = `${pythonPath}\n${pythonHome || ''}`;
  if (candidates.some((candidate) => candidate.key === key)) {
    return;
  }
  candidates.push({
    key,
    label,
    pythonHome,
    pythonPath,
  });
}

function buildPythonLaunchCandidates(resolvedPythonPath, hnpBundledHome, appPythonHome) {
  const candidates = [];
  if (envFirst('JUPYTERLAB_PYTHON_PATH', 'NOTEBOOK_PYTHON_PATH')) {
    addPythonLaunchCandidate(
      candidates,
      'configured',
      resolvedPythonPath,
      process.env.PYTHONHOME || '',
    );
    return candidates;
  }

  const hnpPublicHome = process.env.HNP_PUBLIC_HOME || '/data/service/hnp';
  if (hnpBundledHome) {
    addPythonLaunchCandidate(
      candidates,
      'bundled-hnp-link',
      hnpLinkedPythonPath(hnpBundledHome),
      hnpBundledHome,
    );
    addPythonLaunchCandidate(
      candidates,
      'bundled-hnp-direct',
      path.join(hnpBundledHome, 'bin', 'python3'),
      hnpBundledHome,
    );
  }

  for (const home of hnpSystemPythonHomeCandidates()) {
    const direct = path.join(home, 'bin', 'python3');
    addPythonLaunchCandidate(candidates, `hnp-system-direct:${path.basename(home)}`, direct, home);
    addPythonLaunchCandidate(candidates, `hnp-system-direct-nohome:${path.basename(home)}`, direct, '');
    if (home.includes(path.join('python.org', 'python_3.12'))) {
      addPythonLaunchCandidate(
        candidates,
        'hnp-system-link:python3',
        path.join(hnpPublicHome, 'bin', 'python3'),
        home,
      );
      addPythonLaunchCandidate(
        candidates,
        'hnp-system-link-nohome:python3',
        path.join(hnpPublicHome, 'bin', 'python3'),
        '',
      );
    }
  }

  if (appPythonHome) {
    addPythonLaunchCandidate(
      candidates,
      'bundled-app-mirror',
      path.join(appPythonHome, 'bin', 'python3'),
      appPythonHome,
    );
  }

  addPythonLaunchCandidate(candidates, 'resolved', resolvedPythonPath, process.env.PYTHONHOME || '');
  return candidates;
}

function buildPythonEnv({
  bundledJupyterDirs,
  bundledPyLibs,
  nativeLibDir,
  pyLibs,
  pythonBootstrapDir,
  pythonHome,
  pythonPath,
  runtimeDataDir,
}) {
  const userDataDir = app.getPath('userData');
  const env = {
    ...process.env,
    HOME: userDataDir,
    XDG_CONFIG_HOME: path.join(userDataDir, 'config'),
    XDG_DATA_HOME: path.join(userDataDir, 'data'),
    XDG_RUNTIME_DIR: path.join(userDataDir, 'runtime'),
    PYTHONDONTWRITEBYTECODE: '1',
    JUPYTER_PREFER_ENV_PATH: '0',
    JUPYTERLAB_OHOS_NATIVE_LIB_DIR: nativeLibDir,
    JUPYTER_CONFIG_DIR: path.join(userDataDir, 'jupyter-config'),
    JUPYTER_DATA_DIR: runtimeDataDir,
    JUPYTER_CONFIG_PATH: [
      ...bundledJupyterDirs.config,
      process.env.JUPYTER_CONFIG_PATH,
    ]
      .filter(Boolean)
      .join(path.delimiter),
    JUPYTER_PATH: [
      runtimeDataDir,
      ...bundledJupyterDirs.data,
      process.env.JUPYTER_PATH,
    ]
      .filter(Boolean)
      .join(path.delimiter),
    JUPYTER_RUNTIME_DIR: path.join(userDataDir, 'runtime'),
    IPYTHONDIR: path.join(userDataDir, 'ipython'),
    LD_LIBRARY_PATH: [
      ...bundledPythonLibraryDirs(pythonHome),
      process.env.LD_LIBRARY_PATH,
    ]
      .filter(Boolean)
      .join(path.delimiter),
    PATH: [
      path.isAbsolute(pythonPath) ? path.dirname(pythonPath) : '',
      pythonHome ? path.join(pythonHome, 'bin') : '',
      process.env.PATH,
    ]
      .filter(Boolean)
      .join(path.delimiter),
    PYTHONPATH: [pythonBootstrapDir, bundledPyLibs, pyLibs, process.env.PYTHONPATH]
      .filter(Boolean)
      .join(path.delimiter),
  };
  if (pythonHome) {
    env.PYTHONHOME = pythonHome;
  } else {
    delete env.PYTHONHOME;
  }
  return env;
}

async function selectLaunchablePython(candidates, makeEnv) {
  if (candidates.length === 0) {
    throw new Error('No Python interpreter candidates were found.');
  }
  if (!openHarmony || candidates.length <= 1) {
    const candidate = candidates[0];
    return { candidate, env: makeEnv(candidate) };
  }
  for (const candidate of candidates) {
    const env = makeEnv(candidate);
    writeLog(
      `[probe:python-select:${candidate.label}] trying ${candidate.pythonPath} ` +
        `(home=${candidate.pythonHome || '<unset>'})`,
    );
    const result = await runProcess(
      candidate.pythonPath,
      [
        '-S',
        '-c',
        [
          'import sys',
          'print("[probe] executable", sys.executable)',
          'print("[probe] version", sys.version.replace("\\n", " "))',
          'print("[probe] prefix", sys.prefix)',
          'print("[probe] exec_prefix", sys.exec_prefix)',
        ].join('; '),
      ],
      env,
      (c) => writeLog(`[probe:python-select:${candidate.label}] ${String(c).trimEnd()}`),
    );
    if (result.error) {
      writeLog(
        `[probe:python-select:${candidate.label}] process error: ${result.error.message || result.error}`,
      );
    }
    writeLog(
      `[probe:python-select:${candidate.label}] finished code=${result.code}, ` +
        `signal=${result.signal || '<none>'}`,
    );
    if (result.code === 0) {
      writeLog(`[probe:python-select:${candidate.label}] selected`);
      return { candidate, env };
    }
  }
  throw new Error(
    'No bundled or on-device Python interpreter could be launched in the OpenHarmony app sandbox.\n\n' +
      candidates
        .map((candidate) => `${candidate.label}: ${candidate.pythonPath} (home=${candidate.pythonHome || '<unset>'})`)
        .join('\n'),
  );
}

function chmodExecutable(file) {
  try {
    fs.chmodSync(file, 0o755);
  } catch (error) {
    writeLog(`Could not chmod executable ${file}: ${error?.message || error}`);
  }
}

function prepareBundledPythonHome(sourceHome, userDataDir) {
  if (!sourceHome || !fs.existsSync(path.join(sourceHome, 'bin', 'python3'))) {
    return sourceHome || '';
  }

  if (!openHarmony) {
    return sourceHome;
  }

  const mirrorHome = path.join(userDataDir, 'bundled-python');
  const markerFile = path.join(mirrorHome, '.jupyterlab-ohos-python.json');
  const sourceFingerprint = fingerprintPyLibs(sourceHome);
  const existing = readJsonFile(markerFile);

  if (
    existing?.version === PYTHON_MIRROR_VERSION &&
    sameFingerprint(existing.sourceFingerprint, sourceFingerprint) &&
    fs.existsSync(path.join(mirrorHome, 'bin', 'python3'))
  ) {
    chmodExecutable(path.join(mirrorHome, 'bin', 'python3'));
    writeLog(`Using mirrored bundled Python: ${mirrorHome}`);
    return mirrorHome;
  }

  const tempHome = `${mirrorHome}.tmp-${Date.now()}`;
  writeLog(`Mirroring bundled Python from ${sourceHome} to ${mirrorHome}`);
  fs.rmSync(tempHome, { recursive: true, force: true });
  try {
    fs.mkdirSync(path.dirname(mirrorHome), { recursive: true });
    fs.cpSync(sourceHome, tempHome, {
      recursive: true,
      force: true,
      dereference: true,
      filter: (source) => path.basename(source) !== '__pycache__',
    });
    fs.writeFileSync(
      path.join(tempHome, '.jupyterlab-ohos-python.json'),
      JSON.stringify({ version: PYTHON_MIRROR_VERSION, sourceFingerprint }, null, 2),
    );
    fs.rmSync(mirrorHome, { recursive: true, force: true });
    fs.renameSync(tempHome, mirrorHome);
  } catch (error) {
    fs.rmSync(tempHome, { recursive: true, force: true });
    throw error;
  }

  chmodExecutable(path.join(mirrorHome, 'bin', 'python3'));
  return mirrorHome;
}

function writePythonBootstrap(userDataDir, manifest, nativeLibDir) {
  const bootstrapDir = path.join(userDataDir, 'python-bootstrap');
  fs.mkdirSync(bootstrapDir, { recursive: true });
  const rawModules = manifest?.modules && typeof manifest.modules === 'object' ? manifest.modules : {};
  const modules = {};
  for (const [moduleName, entry] of Object.entries(rawModules)) {
    if (typeof entry === 'string') {
      modules[moduleName] = { file: entry, deps: [] };
    } else if (entry && typeof entry === 'object' && typeof entry.file === 'string') {
      modules[moduleName] = {
        file: entry.file,
        deps: Array.isArray(entry.deps) ? entry.deps.filter((name) => typeof name === 'string') : [],
      };
    }
  }
  const libraries =
    manifest?.libraries && typeof manifest.libraries === 'object' ? manifest.libraries : {};
  const bootstrap = String.raw`
import importlib.abc
import importlib.machinery
import importlib.util
import os
import runpy
import sys
import types

_NATIVE_LIB_DIR = ${JSON.stringify(nativeLibDir)}
_MODULES = ${JSON.stringify(modules, null, 2)}
_LIBRARIES = ${JSON.stringify(libraries, null, 2)}
_INSTALLED = False
_PYTHON_LIBRARY_LOADED = False
_LOADED_LIBS = set()

def _log(message):
    print("[ohos-python] " + message, file=sys.stderr, flush=True)

def _native_path(name):
    native_dir = os.environ.get("JUPYTERLAB_OHOS_NATIVE_LIB_DIR") or _NATIVE_LIB_DIR
    return os.path.join(native_dir, name)

def _native_exists(name):
    candidate = _native_path(name)
    return candidate if os.path.exists(candidate) else ""

def _dlopen_flags():
    import ctypes
    return (
        getattr(ctypes, "RTLD_GLOBAL", 0)
        | getattr(os, "RTLD_GLOBAL", 0)
        | getattr(os, "RTLD_NOW", 0)
    )

def _load_python_library():
    global _PYTHON_LIBRARY_LOADED
    if _PYTHON_LIBRARY_LOADED:
        return
    import ctypes
    version = "%d.%d" % sys.version_info[:2]
    candidates = []
    for base in (sys.exec_prefix, sys.prefix, os.environ.get("PYTHONHOME")):
        if not base:
            continue
        candidates.extend([
            os.path.join(base, "lib", "libpython" + version + ".so.1.0"),
            os.path.join(base, "lib", "libpython" + version + ".so"),
        ])
    for candidate in dict.fromkeys(candidates):
        if not os.path.exists(candidate):
            continue
        try:
            ctypes.CDLL(candidate, mode=_dlopen_flags())
            _PYTHON_LIBRARY_LOADED = True
            _log("libpython loaded globally: " + candidate)
            return
        except Exception as exc:
            _log("libpython global load failed: " + candidate + " " + repr(exc))
    _PYTHON_LIBRARY_LOADED = True

def _load_support_library(name, fullname):
    if name in _LOADED_LIBS:
        return
    import ctypes
    candidate = _native_exists(name)
    if not candidate:
        message = "support lib missing for " + fullname + ": " + _native_path(name)
        _log(message)
        raise ImportError(message)
    try:
        _log("support lib loading for " + fullname + ": " + candidate)
        ctypes.CDLL(candidate, mode=_dlopen_flags())
        _LOADED_LIBS.add(name)
        _log("support lib loaded for " + fullname + ": " + candidate)
    except Exception as exc:
        _log("support lib failed for " + fullname + ": " + candidate + " " + repr(exc))
        raise

def _load_extension_deps(fullname, entry):
    deps = entry.get("deps") if isinstance(entry, dict) else []
    for name in deps or []:
        _load_support_library(name, fullname)

def _install_psutil_fallback():
    if sys.platform != "ohos" or "psutil" in sys.modules:
        return
    import collections
    import socket
    import subprocess
    import time

    module = types.ModuleType("psutil")

    class Error(Exception):
        pass

    class NoSuchProcess(Error):
        pass

    class ZombieProcess(NoSuchProcess):
        pass

    class AccessDenied(Error):
        pass

    class TimeoutExpired(Error):
        pass

    pmem = collections.namedtuple("pmem", "rss vms shared text lib data dirty")
    pfullmem = collections.namedtuple("pfullmem", "rss vms shared text lib data dirty uss pss swap")
    svmem = collections.namedtuple("svmem", "total available percent used free active inactive buffers cached shared slab")
    snicaddr = collections.namedtuple("snicaddr", "family address netmask broadcast ptp")

    def _read_int(path, default=0):
        try:
            with open(path, "r", encoding="utf-8") as handle:
                return int(handle.read().strip())
        except Exception:
            return default

    def _memory_total():
        try:
            with open("/proc/meminfo", "r", encoding="utf-8") as handle:
                for line in handle:
                    if line.startswith("MemTotal:"):
                        return int(line.split()[1]) * 1024
        except Exception:
            pass
        return 0

    class Process:
        def __init__(self, pid=None):
            self.pid = int(os.getpid() if pid is None else pid)

        def __repr__(self):
            return "psutil.Process(pid=%s)" % self.pid

        def is_running(self):
            if self.pid == os.getpid():
                return True
            try:
                os.kill(self.pid, 0)
                return True
            except OSError:
                return False

        def children(self, recursive=False):
            return []

        def cpu_percent(self, interval=None):
            if interval:
                time.sleep(interval)
            return 0.0

        def memory_info(self):
            rss = _read_int("/proc/self/statm", 0) * 4096 if self.pid == os.getpid() else 0
            return pmem(rss, rss, 0, 0, 0, 0, 0)

        def memory_full_info(self):
            info = self.memory_info()
            return pfullmem(info.rss, info.vms, info.shared, info.text, info.lib, info.data, info.dirty, info.rss, info.rss, 0)

        def send_signal(self, sig):
            os.kill(self.pid, sig)

        def terminate(self):
            self.send_signal(getattr(__import__("signal"), "SIGTERM", 15))

        def kill(self):
            self.send_signal(getattr(__import__("signal"), "SIGKILL", 9))

        def wait(self, timeout=None):
            return None

    def pid_exists(pid):
        return Process(pid).is_running()

    def pids():
        try:
            return [int(name) for name in os.listdir("/proc") if name.isdigit()]
        except Exception:
            return [os.getpid()]

    def process_iter(attrs=None, ad_value=None):
        for pid in pids():
            yield Process(pid)

    def wait_procs(procs, timeout=None, callback=None):
        gone = []
        alive = list(procs)
        return gone, alive

    def cpu_percent(interval=None, percpu=False):
        if interval:
            time.sleep(interval)
        return [0.0] * (os.cpu_count() or 1) if percpu else 0.0

    def cpu_count(logical=True):
        return os.cpu_count() or 1

    def virtual_memory():
        total = _memory_total()
        available = total
        return svmem(total, available, 0.0, 0, available, 0, 0, 0, 0, 0, 0)

    def net_if_addrs():
        return {
            "lo": [
                snicaddr(socket.AF_INET, "127.0.0.1", "255.0.0.0", None, None),
                snicaddr(socket.AF_INET6, "::1", None, None, None),
            ],
        }

    module.Error = Error
    module.NoSuchProcess = NoSuchProcess
    module.ZombieProcess = ZombieProcess
    module.AccessDenied = AccessDenied
    module.TimeoutExpired = TimeoutExpired
    module.Process = Process
    module.Popen = subprocess.Popen
    module.pid_exists = pid_exists
    module.pids = pids
    module.process_iter = process_iter
    module.wait_procs = wait_procs
    module.cpu_percent = cpu_percent
    module.cpu_count = cpu_count
    module.virtual_memory = virtual_memory
    module.net_if_addrs = net_if_addrs
    module.__version__ = "7.0.0-ohos"
    module.version_info = (7, 0, 0)
    module.STATUS_RUNNING = "running"
    module.STATUS_SLEEPING = "sleeping"
    module.AF_LINK = getattr(socket, "AF_LINK", object())
    sys.modules["psutil"] = module
    _log("installed psutil fallback for OpenHarmony")

class _NativeExtensionFinder(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        entry = _MODULES.get(fullname)
        if not entry:
            return None
        native_name = entry.get("file") if isinstance(entry, dict) else entry
        candidate = _native_exists(native_name)
        if not candidate:
            _log("extension missing for " + fullname + ": " + _native_path(native_name))
            return None
        _load_extension_deps(fullname, entry)
        _log("extension redirect: " + fullname + " -> " + candidate)
        loader = importlib.machinery.ExtensionFileLoader(fullname, candidate)
        return importlib.util.spec_from_file_location(fullname, candidate, loader=loader)

def install():
    global _INSTALLED
    if _INSTALLED:
        return
    _load_python_library()
    _install_psutil_fallback()
    try:
        sys.setdlopenflags(_dlopen_flags())
    except Exception:
        pass
    sys.meta_path.insert(0, _NativeExtensionFinder())
    _INSTALLED = True
    _log("native extension hook active: " + (os.environ.get("JUPYTERLAB_OHOS_NATIVE_LIB_DIR") or _NATIVE_LIB_DIR))

def main():
    install()
    if len(sys.argv) < 2:
        raise SystemExit("usage: python -m ohos_jupyter_bootstrap <module> [args...]")
    module = sys.argv[1]
    sys.argv = [module] + sys.argv[2:]
    runpy.run_module(module, run_name="__main__", alter_sys=True)

if __name__ == "__main__":
    main()
`.trimStart();
  const sitecustomize = [
    '# OpenHarmony loads sitecustomize before -c/-m code runs.',
    '# Keep this file passive so diagnostics can run before native hooks are installed.',
    '',
  ].join('\n');

  fs.writeFileSync(path.join(bootstrapDir, 'ohos_jupyter_bootstrap.py'), bootstrap);
  fs.writeFileSync(path.join(bootstrapDir, 'sitecustomize.py'), sitecustomize);
  return bootstrapDir;
}

function prepareBundledPyLibs(sourceDir, userDataDir) {
  if (!fs.existsSync(sourceDir)) {
    return null;
  }

  const mirrorDir = path.join(userDataDir, 'bundled-pylibs');
  const markerFile = path.join(mirrorDir, '.jupyterlab-ohos-pylibs.json');
  const sourceFingerprint = fingerprintPyLibs(sourceDir);
  const existing = readJsonFile(markerFile);

  if (
    existing?.version === PYLIBS_MIRROR_VERSION &&
    sameFingerprint(existing.sourceFingerprint, sourceFingerprint)
  ) {
    writeLog(`Using mirrored bundled pylibs: ${mirrorDir}`);
    return mirrorDir;
  }

  const tempDir = `${mirrorDir}.tmp-${Date.now()}`;
  writeLog(`Mirroring bundled pylibs from ${sourceDir} to ${mirrorDir}`);
  fs.rmSync(tempDir, { recursive: true, force: true });
  try {
    fs.mkdirSync(path.dirname(mirrorDir), { recursive: true });
    fs.cpSync(sourceDir, tempDir, {
      recursive: true,
      force: true,
      dereference: true,
      filter: (source) => {
        const name = path.basename(source);
        return name !== '__pycache__' && !isNativeBinaryName(name);
      },
    });
    fs.writeFileSync(
      path.join(tempDir, '.jupyterlab-ohos-pylibs.json'),
      JSON.stringify(
        {
          version: PYLIBS_MIRROR_VERSION,
          copiedAt: new Date().toISOString(),
          sourceDir,
          sourceFingerprint,
        },
        null,
        2,
      ) + '\n',
    );
    fs.rmSync(mirrorDir, { recursive: true, force: true });
    fs.renameSync(tempDir, mirrorDir);
  } catch (error) {
    fs.rmSync(tempDir, { recursive: true, force: true });
    throw error;
  }

  return mirrorDir;
}

function bundledJupyterDataDirs(baseDir) {
  const result = {
    config: [],
    data: [],
  };
  if (!baseDir || !fs.existsSync(baseDir)) {
    return result;
  }
  for (const entry of fs.readdirSync(baseDir, { withFileTypes: true })) {
    if (!entry.isDirectory() || !entry.name.endsWith('.data')) {
      continue;
    }
    const root = path.join(baseDir, entry.name, 'data');
    const share = path.join(root, 'share', 'jupyter');
    const etc = path.join(root, 'etc', 'jupyter');
    if (fs.existsSync(share)) {
      result.data.push(share);
    }
    if (fs.existsSync(etc)) {
      result.config.push(etc);
    }
  }
  result.config.sort();
  result.data.sort();
  return result;
}

function runtimeJupyterDataDir(userDataDir) {
  return path.join(userDataDir, 'jupyter');
}

function writeRuntimePythonKernelSpec(jupyterDataDir, pythonPath, env) {
  const kernelEnv = {};
  for (const key of [
    'PYTHONDONTWRITEBYTECODE',
    'PYTHONHOME',
    'PYTHONPATH',
    'NOTEBOOK_OHOS_NATIVE_LIB_DIR',
    'JUPYTERLAB_OHOS_NATIVE_LIB_DIR',
    'LD_LIBRARY_PATH',
    'PATH',
  ]) {
    if (env[key]) {
      kernelEnv[key] = env[key];
    }
  }

  const specs = [
    {
      name: 'python3',
      displayName: 'Python 3 (ipykernel)',
    },
    {
      name: 'python3-ohos',
      displayName: 'Python 3 (OpenHarmony ipykernel)',
    },
  ];

  for (const spec of specs) {
    const kernelDir = path.join(jupyterDataDir, 'kernels', spec.name);
    fs.mkdirSync(kernelDir, { recursive: true });
    const kernelJson = {
      argv: [
        pythonPath,
        '-m',
        'ohos_jupyter_bootstrap',
        'ipykernel_launcher',
        '-f',
        '{connection_file}',
      ],
      display_name: spec.displayName,
      language: 'python',
      env: kernelEnv,
      metadata: {
        debugger: true,
        'jupyterlab-ohos': {
          generated: true,
          version: RUNTIME_KERNELSPEC_VERSION,
        },
      },
    };

    fs.writeFileSync(
      path.join(kernelDir, 'kernel.json'),
      JSON.stringify(kernelJson, null, 2) + '\n',
    );
    writeLog(`Runtime ${spec.name} kernelspec: ${path.join(kernelDir, 'kernel.json')}`);
    writeLog(`Runtime ${spec.name} argv   : ${kernelJson.argv.join(' ')}`);
  }
}

// ----------------------------------------------------------------------------
// Server lifecycle
// ----------------------------------------------------------------------------
function startLocalServer() {
  if (openHarmony && shouldUseNodeStaticLocalServer()) {
    startNodeStaticLocalServer();
    return;
  }
  startPythonLocalServer();
}

function shouldUseNodeStaticLocalServer() {
  if (isTruthyEnv(envFirst('JUPYTERLAB_OHOS_FORCE_PYTHON', 'NOTEBOOK_OHOS_FORCE_PYTHON'))) {
    return false;
  }
  const mode = String(envFirst('JUPYTERLAB_OHOS_SERVER_MODE', 'NOTEBOOK_OHOS_SERVER_MODE') || '').toLowerCase();
  return mode === 'node-static' || mode === 'static' || mode === 'offline';
}

function startNodeStaticLocalServer({ fallbackReason = '' } = {}) {
  const port = resolveServerPort();
  const token = resolveToken();
  const rootDir = resolveRootDir(app.getPath('userData'));
  const baseUrl = `http://127.0.0.1:${port}`;
  startUrl = `${baseUrl}/lab?token=${encodeURIComponent(token)}`;

  writeLog(
    fallbackReason
      ? `Starting OpenHarmony node-static fallback server: ${fallbackReason}`
      : 'Starting OpenHarmony node-static local server.',
  );
  startStaticLabServer({
    appDir,
    fallbackReason,
    port,
    rootDir,
    token,
    userDataDir: app.getPath('userData'),
    writeLog,
  })
    .then((server) => {
      nodeStaticServer = server;
      serverReady = true;
      loadAppUrl(server.url || startUrl);
    })
    .catch((error) => {
      if (!startupHandled || fallbackReason) {
        startupHandled = true;
        showFatal(
          'Could not start the OpenHarmony JupyterLab node-static server.\n\n' +
            `${error?.message || error}`,
        );
      }
    });
}

function fallbackToNodeStaticServer(reason) {
  if (fallbackStarted) {
    return;
  }
  fallbackStarted = true;
  startupHandled = true;
  const message = String(reason?.message || reason || 'Python Jupyter Server did not start.');
  writeLog(`Falling back to OpenHarmony node-static mode. Reason: ${message}`);
  rememberServerOutput(`Fallback reason: ${message}`);
  if (serverProcess && !serverProcess.killed) {
    try {
      serverProcess.kill('SIGTERM');
    } catch {
      /* ignore */
    }
  }
  serverProcess = null;
  startNodeStaticLocalServer({ fallbackReason: message });
}

function startPythonLocalServer() {
  let pythonPath = resolvePythonPath(appDir);
  const port = resolveServerPort();
  const token = resolveToken();
  const rootDir = resolveRootDir(app.getPath('userData'));
  const pyLibs = resolvePyLibsDir(app.getPath('userData'));

  const baseUrl = `http://127.0.0.1:${port}`;
  startUrl = `${baseUrl}/lab?token=${token}`;
  const pingUrl = `${baseUrl}/lab`;

  const args = [
    '-m', 'ohos_jupyter_bootstrap', 'jupyterlab',
    '--no-browser',
    '--ServerApp.ip=127.0.0.1',
    `--ServerApp.port=${port}`,
    '--ServerApp.port_retries=0',
    '--ServerApp.open_browser=False',
    `--IdentityProvider.token=${token}`,
    `--ServerApp.root_dir=${rootDir}`,
    '--ServerApp.allow_origin=*',
    '--MappingKernelManager.default_kernel_name=python3',
  ];

  // Make both the bundled offline site (shipped in the HAP) and the writable
  // pip --target dir importable by the interpreter. OpenHarmony only allows
  // dlopen from native library locations, so pure Python is mirrored into the
  // sandbox while native Python extensions are loaded from the HAP lib dir via
  // the generated sitecustomize hook.
  let bundledPyLibs = null;
  let pythonBootstrapDir = null;
  const nativeExtensionManifest = readNativeExtensionManifest();
  const nativeLibDir = resolveNativeLibDir();
  const appPythonHome = bundledPythonHomeDir();
  const hnpPythonHome = bundledHnpPythonHomeDir();
  let preparedAppPythonHome = '';
  try {
    if (hnpPythonHome) {
      writeLog(`Found bundled HNP Python: ${hnpPythonHome}`);
    }
    if (appPythonHome) {
      preparedAppPythonHome = prepareBundledPythonHome(appPythonHome, app.getPath('userData'));
    }
    bundledPyLibs = prepareBundledPyLibs(path.join(appDir, 'pylibs'), app.getPath('userData'));
    if (bundledPyLibs) {
      args.push(`--LabApp.app_dir=${path.join(bundledPyLibs, 'jupyterlab')}`);
    }
    pythonBootstrapDir = writePythonBootstrap(
      app.getPath('userData'),
      nativeExtensionManifest,
      nativeLibDir,
    );
  } catch (error) {
    fallbackToNodeStaticServer(
      'Failed to prepare bundled Python packages.\n\n' +
        `${error?.message || error}\n\n` +
        'The bundled pylibs tree and Python native-extension bootstrap must be prepared before launching Python.',
    );
    return;
  }
  const runtimeDataDir = runtimeJupyterDataDir(app.getPath('userData'));
  const bundledJupyterDirs = bundledJupyterDataDirs(bundledPyLibs);
  const candidates = buildPythonLaunchCandidates(pythonPath, hnpPythonHome, preparedAppPythonHome);
  const makeEnv = (candidate) =>
    buildPythonEnv({
      bundledJupyterDirs,
      bundledPyLibs,
      nativeLibDir,
      pyLibs,
      pythonBootstrapDir,
      pythonHome: candidate.pythonHome,
      pythonPath: candidate.pythonPath,
      runtimeDataDir,
    });

  selectLaunchablePython(candidates, makeEnv)
    .then(({ candidate, env }) => {
      pythonPath = candidate.pythonPath;
      const pythonHome = candidate.pythonHome;
      serverPythonPath = pythonPath;

      try {
        writeRuntimePythonKernelSpec(runtimeDataDir, pythonPath, env);
      } catch (error) {
        fallbackToNodeStaticServer(
          'Failed to prepare the runtime python3 kernelspec.\n\n' +
            `${error?.message || error}\n\n` +
            'The python3 kernelspec is required so newly created notebooks can start a Python kernel.',
        );
        return null;
      }

      writeLog('JupyterLab HarmonyOS runtime');
      writeLog('--------------------------------------------------------');
      writeLog(`OpenHarmony runtime : ${openHarmony}`);
      if (openHarmony) {
        writeLog(`HNP_PRIVATE_HOME    : ${process.env.HNP_PRIVATE_HOME || '<unset>'}`);
        writeLog(`HNP_PUBLIC_HOME     : ${process.env.HNP_PUBLIC_HOME || '<unset>'}`);
        writeLog(`HNP Python candidates: ${hnpPythonHomeCandidates().join(', ') || '<none>'}`);
        writeLog(`Selected Python     : ${candidate.label}`);
      }
      writeLog(`Python path         : ${pythonPath}`);
      writeLog(`Python home         : ${pythonHome || '<external interpreter>'}`);
      writeLog(`Python bootstrap    : ${pythonBootstrapDir || '<none>'}`);
      writeLog(`PyLibs (pip target) : ${pyLibs}`);
      writeLog(`Jupyter data dir    : ${runtimeDataDir}`);
      writeLog(`JUPYTER_PATH        : ${env.JUPYTER_PATH}`);
      writeLog(`Native lib dir      : ${nativeLibDir}`);
      writeLog(
        `Native extensions   : ${Object.keys(nativeExtensionManifest.modules || {}).length} modules, ` +
          `${(nativeExtensionManifest.preload || []).length} support libs`,
      );
      writeLog(`Notebook root       : ${rootDir}`);
      writeLog(`Server URL          : ${baseUrl}`);
      writeLog(`Command             : ${pythonPath} ${args.join(' ')}`);
      writeLog('--------------------------------------------------------');

      return ensureJupyterLab(pythonPath, pyLibs, env, bundledPyLibs).then(() =>
        spawnServer(pythonPath, args, env, pingUrl),
      );
    })
    .catch((err) => {
      if (!startupHandled) {
        fallbackToNodeStaticServer(err?.message || String(err));
      }
    });
}

function spawnServer(pythonPath, args, env, pingUrl) {
  try {
    serverProcess = spawn(pythonPath, args, { env });
  } catch (error) {
    fallbackToNodeStaticServer(`Failed to launch Python (${pythonPath}).\n\n${error?.message || error}`);
    return;
  }

  serverProcess.on('error', (err) => {
    writeLog(`Server process error: ${err?.message || err}`);
    fallbackToNodeStaticServer(
      `Could not start the Jupyter Server.\n\nPython: ${pythonPath}\n\n${err?.message || err}`,
    );
  });
  serverProcess.on('exit', (code, signal) => {
    writeLog(`Server process exited (code=${code}, signal=${signal}).`);
    // If the server dies before it ever answered, surface the real cause now
    // instead of waiting for the readiness timeout.
    if (!serverReady && !startupHandled) {
      fallbackToNodeStaticServer(diagnoseStartupFailure(code));
    }
  });
  serverProcess.stdout?.setEncoding('utf8');
  serverProcess.stdout?.on('data', (chunk) => {
    rememberServerOutput(chunk);
    writeLog(`[server] ${chunk.trimEnd()}`);
  });
  serverProcess.stderr?.setEncoding('utf8');
  serverProcess.stderr?.on('data', (chunk) => {
    rememberServerOutput(chunk);
    writeLog(`[server] ${chunk.trimEnd()}`);
  });

  waitForServer(pingUrl)
    .then(() => {
      serverReady = true;
      loadAppUrl(startUrl);
    })
    .catch((err) => {
      writeLog(`Server did not become ready: ${err?.message || err}`);
      if (!serverReady && !startupHandled) {
        fallbackToNodeStaticServer(diagnoseStartupFailure(null));
      }
    });
}

// ----------------------------------------------------------------------------
// First-run bootstrap: pip install JupyterLab into a writable target dir if the
// device Python cannot import it. The app's spawn context can reach the on-device
// HNP Python, so it can install JupyterLab itself.
// ----------------------------------------------------------------------------
function runProcess(cmd, args, env, onData) {
  return new Promise((resolve) => {
    let child;
    let settled = false;
    const finish = (result) => {
      if (settled) {
        return;
      }
      settled = true;
      resolve(result);
    };
    try {
      child = spawn(cmd, args, { env });
    } catch (error) {
      finish({ code: -1, error });
      return;
    }
    child.on('error', (error) => finish({ code: -1, error }));
    child.stdout?.setEncoding('utf8');
    child.stdout?.on('data', (c) => onData?.(c));
    child.stderr?.setEncoding('utf8');
    child.stderr?.on('data', (c) => onData?.(c));
    child.on('close', (code, signal) => finish({ code, signal }));
  });
}

function probeJupyterLabImport(pythonPath, env) {
  const script = [
    'import importlib, sys, sysconfig, traceback',
    'print("[probe] executable", sys.executable)',
    'print("[probe] version", sys.version.replace("\\n", " "))',
    'print("[probe] ext_suffix", sysconfig.get_config_var("EXT_SUFFIX"))',
    'try:',
    '    import ohos_jupyter_bootstrap',
    '    ohos_jupyter_bootstrap.install()',
    '    print("[probe] bootstrap installed", getattr(ohos_jupyter_bootstrap, "__file__", None))',
    'except Exception:',
    '    print("[probe] bootstrap install FAILED")',
    '    traceback.print_exc()',
    '    raise',
    'print("[probe] meta_path")',
    'for finder in sys.meta_path[:5]:',
    '    print("[probe]  ", type(finder).__module__ + "." + type(finder).__name__)',
    'print("[probe] sys.path")',
    'for p in sys.path:',
    '    print("[probe]  ", p)',
    'try:',
    '    mod = importlib.import_module("jupyterlab")',
    '    print("[probe] import jupyterlab OK", getattr(mod, "__file__", None))',
    'except Exception:',
    '    print("[probe] import jupyterlab FAILED")',
    '    traceback.print_exc()',
    '    raise',
  ].join('\n');

  return runProcess(pythonPath, ['-c', script], env, (c) => writeLog(String(c).trimEnd()));
}

function hasJupyterLab(pythonPath, env) {
  return probeJupyterLabImport(pythonPath, env).then((r) => {
    if (r.error) {
      writeLog(`[probe] process error: ${r.error.message || r.error}`);
    }
    writeLog(`[probe] import process finished code=${r.code}, signal=${r.signal || '<none>'}`);
    return r.code === 0;
  });
}

function probePythonKernelStack(pythonPath, env) {
  const script = [
    'import importlib, traceback',
    'try:',
    '    import ohos_jupyter_bootstrap',
    '    ohos_jupyter_bootstrap.install()',
    '    for name in ("jupyterlab", "jupyter_server", "ipykernel", "jupyter_client", "jupyter_core", "IPython", "zmq", "tornado", "traitlets"):',
    '        mod = importlib.import_module(name)',
    '        print("[probe] import", name, "OK", getattr(mod, "__file__", None))',
    '    from jupyter_client.kernelspec import KernelSpecManager',
    '    specs = KernelSpecManager()',
    '    for kernel_name in ("python3", "python3-ohos"):',
    '        spec = specs.get_kernel_spec(kernel_name)',
    '        print("[probe] kernelspec", kernel_name, "argv", spec.argv)',
    '        if "ohos_jupyter_bootstrap" not in spec.argv:',
    '            raise RuntimeError(kernel_name + " kernelspec does not launch through ohos_jupyter_bootstrap")',
    'except Exception:',
    '    print("[probe] Python kernel stack FAILED")',
    '    traceback.print_exc()',
    '    raise',
  ].join('\n');
  return runProcess(pythonPath, ['-c', script], env, (c) => writeLog(String(c).trimEnd()));
}

async function ensurePythonKernelStack(pythonPath, env) {
  const result = await probePythonKernelStack(pythonPath, env);
  if (result.error) {
    throw new Error(`Failed to launch Python kernel dependency probe.\n\n${result.error.message || result.error}`);
  }
  if (result.code !== 0) {
    throw new Error(
      'Bundled Python kernel dependencies are incomplete or incompatible.\n\n' +
        'Required imports: jupyterlab, jupyter_server, ipykernel, jupyter_client, jupyter_core, IPython, zmq, tornado, traitlets.\n\n' +
        `Recent output:\n${serverOutputTail.join('\n').trim()}`,
    );
  }
}

function appendInstallLog(text) {
  rememberServerOutput(text);
  writeLog(`[pip] ${String(text).trimEnd()}`);
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents
      .executeJavaScript(`window.jupyterAppendInstallLog?.(${JSON.stringify(text)});`, true)
      .catch(() => {});
  }
}

async function ensureJupyterLab(pythonPath, pyLibs, env, bundledPyLibs) {
  // Diagnostics: show the interpreter's view of the world so path/ABI problems
  // are obvious in the log.
  const bundledSource = path.join(appDir, 'pylibs');
  writeLog(`appDir = ${appDir}`);
  writeLog(`bundled pylibs source exists = ${fs.existsSync(bundledSource)} (${bundledSource})`);
  writeLog(
    `bundled pylibs import dir = ${bundledPyLibs || '<none>'} ` +
      `(${bundledPyLibs ? fs.existsSync(bundledPyLibs) : false})`,
  );
  writeLog(`PYTHONPATH = ${env.PYTHONPATH}`);
  if (openHarmony) {
    const minimalEnv = {
      ...process.env,
      LD_LIBRARY_PATH: env.LD_LIBRARY_PATH,
      PATH: env.PATH,
    };
    for (const [label, probeEnv] of [
      ['minimal', minimalEnv],
      ['configured', env],
    ]) {
      const versionProbe = await runProcess(
        pythonPath,
        ['-S', '-c', 'import sys; print(sys.version)'],
        probeEnv,
        (c) => writeLog(`[probe:${label}] ${String(c).trimEnd()}`),
      );
      if (versionProbe.error) {
        writeLog(`[probe:${label}] process error: ${versionProbe.error.message || versionProbe.error}`);
      }
      writeLog(
        `[probe:${label}] python -S finished code=${versionProbe.code}, signal=${versionProbe.signal || '<none>'}`,
      );
    }
  }
  const probe = [
    'import os',
    'import ohos_jupyter_bootstrap',
    'ohos_jupyter_bootstrap.install()',
    'import ctypes',
    'def trydl(path):',
    '    if not path:',
    '        return "SKIP"',
    '    if not os.path.exists(path):',
    '        return "MISSING"',
    '    try:',
    '        ctypes.CDLL(path)',
    '        return "DLOPEN-OK"',
    '    except Exception as e:',
    '        return "DLOPEN-FAIL " + repr(e)',
    `source = ${JSON.stringify(bundledSource)}`,
    `mirror = ${JSON.stringify(bundledPyLibs || '')}`,
    `native = ${JSON.stringify(envFirst('JUPYTERLAB_OHOS_NATIVE_LIB_DIR', 'NOTEBOOK_OHOS_NATIVE_LIB_DIR') || env.JUPYTERLAB_OHOS_NATIVE_LIB_DIR || '')}`,
    'for label, base in (("source", source), ("mirror", mirror)):',
    '    cffi = os.path.join(base, "_cffi_backend.cpython-312.so") if base else ""',
    '    print(f"[probe] {label} cffi {cffi or \'<none>\'} {trydl(cffi)}")',
    'for name in ("pylib__cffi_backend.cpython-312.so", "_cffi_backend.cpython-312.so", "_zmq.abi3.so"):',
    '    candidate = os.path.join(native, name) if native else ""',
    '    print(f"[probe] native {name} {candidate or \'<none>\'} {trydl(candidate)}")',
  ].join('\n');
  const abiProbe = await runProcess(pythonPath, ['-c', probe], env, (c) =>
    writeLog(String(c).trimEnd()),
  );
  if (abiProbe.error) {
    writeLog(`[probe] ABI process error: ${abiProbe.error.message || abiProbe.error}`);
  }
  writeLog(`[probe] ABI process finished code=${abiProbe.code}, signal=${abiProbe.signal || '<none>'}`);

  if (await hasJupyterLab(pythonPath, env)) {
    await ensurePythonKernelStack(pythonPath, env);
    writeLog('JupyterLab and Python kernel stack already importable; skipping install.');
    return;
  }

  // If we shipped an offline site but it still isn't importable, the problem is
  // path/ABI — surface it instead of trying to compile from source on-device.
  if (fs.existsSync(bundledSource)) {
    throw new Error(
      'Bundled JupyterLab is present but not importable.\n\n' +
        `appDir: ${appDir}\n` +
        `bundled source: ${bundledSource}\n` +
        `bundled import dir: ${bundledPyLibs || '<none>'}\n` +
        `PYTHONPATH: ${env.PYTHONPATH}\n\n` +
        'See runtime log [probe] lines for sys.path / ABI details.',
    );
  }

  writeLog('JupyterLab not found; starting first-run pip install.');
  const installing = path.join(appDir, 'src', 'html', 'installing.html');
  if (mainWindow && !mainWindow.isDestroyed() && fs.existsSync(installing)) {
    await mainWindow.loadFile(installing).catch(() => {});
    mainWindow.show();
  }

  // Make sure pip is available in this interpreter.
  const pipCheck = await runProcess(pythonPath, ['-m', 'pip', '--version'], env, appendInstallLog);
  if (pipCheck.code !== 0) {
    appendInstallLog('pip not found; bootstrapping with ensurepip…\n');
    await runProcess(pythonPath, ['-m', 'ensurepip', '--upgrade'], env, appendInstallLog);
  }

  const indexUrl = resolvePipIndexUrl();
  const pipArgs = [
    '-m', 'pip', 'install',
    '--target', pyLibs,
    '--upgrade',
    '--no-input',
    '--disable-pip-version-check',
    '-i', indexUrl,
    'jupyterlab',
  ];
  appendInstallLog(`Running: ${pythonPath} ${pipArgs.join(' ')}\n`);
  const result = await runProcess(pythonPath, pipArgs, env, appendInstallLog);
  if (result.error) {
    throw new Error(`Failed to launch pip.\n\n${result.error.message || result.error}`);
  }
  if (result.code !== 0) {
    throw new Error(
      `Installing JupyterLab failed (pip exit code ${result.code}).\n\n` +
        `Interpreter: ${pythonPath}\nTarget: ${pyLibs}\n\n` +
        `Recent output:\n${serverOutputTail.join('\n').trim()}`,
    );
  }

  if (!(await hasJupyterLab(pythonPath, env))) {
    throw new Error(
      'pip reported success but JupyterLab still cannot be imported.\n\n' +
        `Check that ${pyLibs} is on PYTHONPATH and matches the interpreter ABI.`,
    );
  }
  await ensurePythonKernelStack(pythonPath, env);
  writeLog('JupyterLab and Python kernel stack installed successfully.');
  // Return to the splash while the server boots.
  loadSplash();
}

// Build a human-readable diagnostic from the captured server output.
function diagnoseStartupFailure(exitCode) {
  const tail = serverOutputTail.join('\n').trim();
  const lower = tail.toLowerCase();
  let hint = '';

  if (lower.includes('no module named jupyterlab') || lower.includes('no module named')) {
    hint =
      'JupyterLab is not installed in the Python used by this app.\n\n' +
      'On the device, install it into that interpreter:\n' +
      `  ${serverPythonPath || 'python3'} -m pip install jupyterlab\n`;
  } else if (lower.includes('enoent') || (exitCode === null && !tail)) {
    hint =
      `Python could not be found or launched.\n\nInterpreter: ${serverPythonPath || 'python3'}\n` +
      'Set JUPYTERLAB_PYTHON_PATH to a valid interpreter, or bundle one in the app.';
  } else {
    hint = 'The Jupyter Server failed to start.';
  }

  const parts = [hint];
  if (exitCode !== null && exitCode !== undefined) {
    parts.push(`\nPython exit code: ${exitCode}`);
  }
  if (tail) {
    parts.push(`\nServer output:\n${tail}`);
  }
  return parts.join('\n');
}

function startExternalServer() {
  const url = getExternalServerUrl();
  if (!url) {
    showFatal(
      'No Jupyter Server configured.\n\n' +
        'Start a server (e.g. in DevBox: `jupyter lab --no-browser`) and set ' +
        'JUPYTERLAB_EXTERNAL_SERVER_URL, or enable local mode with JUPYTERLAB_OHOS_LOCAL_PYTHON=1.',
    );
    return;
  }
  startUrl = url;
  writeLog(`External Jupyter Server URL: ${url}`);
  waitForServer(url)
    .then(() => loadAppUrl(url))
    .catch(() => loadAppUrl(url)); // load anyway; the page will show its own error
}

function waitForServer(url, { timeoutMs = 60000, intervalMs = 500 } = {}) {
  const deadline = Date.now() + timeoutMs;
  return new Promise((resolve, reject) => {
    const attempt = () => {
      const req = http.get(url, { timeout: 4000 }, (res) => {
        res.resume();
        // Any HTTP response means the server is listening.
        if (res.statusCode && res.statusCode < 600) {
          resolve();
        } else {
          retry();
        }
      });
      req.on('error', retry);
      req.on('timeout', () => {
        req.destroy();
        retry();
      });
    };
    const retry = () => {
      if (Date.now() > deadline) {
        reject(new Error('timeout waiting for server'));
        return;
      }
      setTimeout(attempt, intervalMs);
    };
    attempt();
  });
}

// ----------------------------------------------------------------------------
// Window
// ----------------------------------------------------------------------------
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 820,
    show: false,
    autoHideMenuBar: true,
    icon: fs.existsSync(assetIcon) ? assetIcon : undefined,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
    },
  });

  Menu.setApplicationMenu(null);

  // Open external links (docs etc.) in the system browser.
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    if (startUrl && url.startsWith(new URL(startUrl).origin)) {
      return { action: 'allow' };
    }
    shell.openExternal(url);
    return { action: 'deny' };
  });

  mainWindow.webContents.on('did-fail-load', (_e, code, desc, url) => {
    writeLog(`did-fail-load ${code} ${desc} ${url}`);
  });
  mainWindow.webContents.on('did-start-loading', () => {
    writeLog('webContents did-start-loading');
  });
  mainWindow.webContents.on('dom-ready', () => {
    writeLog(`webContents dom-ready ${mainWindow?.webContents.getURL() || ''}`);
  });
  mainWindow.webContents.on('did-finish-load', () => {
    writeLog(`webContents did-finish-load ${mainWindow?.webContents.getURL() || ''}`);
  });
  mainWindow.webContents.on('console-message', (_event, level, message, line, sourceId) => {
    writeLog(`[renderer-console:${level}] ${message} (${sourceId}:${line})`);
  });

  loadSplash();
  mainWindow.once('ready-to-show', () => mainWindow.show());
  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function loadSplash() {
  const splash = path.join(appDir, 'src', 'html', 'splash.html');
  if (fs.existsSync(splash)) {
    mainWindow.loadFile(splash).catch(() => {});
  }
}

function loadAppUrl(url) {
  if (!mainWindow || mainWindow.isDestroyed()) {
    createWindow();
  }
  writeLog(`Loading ${url}`);
  mainWindow.loadURL(url);
}

function showFatal(message) {
  writeLog(`FATAL: ${message}`);
  const errorPage = path.join(appDir, 'src', 'html', 'server_error.html');
  if (mainWindow && !mainWindow.isDestroyed() && fs.existsSync(errorPage)) {
    mainWindow.loadFile(errorPage, { query: { message } }).catch(() => {});
    mainWindow.show();
  } else {
    dialog.showErrorBox('JupyterLab', message);
  }
}

// ----------------------------------------------------------------------------
// App events
// ----------------------------------------------------------------------------
app.whenReady().then(() => {
  createWindow();
  // Dev-only: export the on-device Python prefix (headers/libs) so the build
  // host can cross-compile extensions against the exact target ABI.
  if (envFirst('JUPYTERLAB_OHOS_EXTRACT_PYTHON', 'NOTEBOOK_OHOS_EXTRACT_PYTHON') === '1') {
    extractPythonPrefix();
    return;
  }
  if (spawnLocal) {
    startLocalServer();
  } else {
    startExternalServer();
  }

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
      if (startUrl) loadAppUrl(startUrl);
    }
  });
});

// Dev-only helper: tar the device Python's include/ + libpython + sysconfig data
// into the app sandbox so it can be pulled with `hdc file recv` and used as the
// cross-compilation target on the build host.
function extractPythonPrefix() {
  const pythonPath = resolvePythonPath(appDir);
  const outDir = app.getPath('userData');
  const script = [
    'import os, sys, sysconfig, tarfile',
    `out = os.path.join(${JSON.stringify(outDir)}, "pyprefix.tar.gz")`,
    'inc = sysconfig.get_path("include")',
    'plat_inc = sysconfig.get_path("platinclude")',
    'stdlib = sysconfig.get_path("stdlib")',
    'prefix = sys.prefix',
    'libdir = sysconfig.get_config_var("LIBDIR") or os.path.join(prefix, "lib")',
    'def add(t, p, arc):',
    '    if p and os.path.exists(p):',
    '        t.add(p, arcname=arc)',
    'with tarfile.open(out, "w:gz") as t:',
    '    add(t, inc, "include")',
    '    add(t, plat_inc, "platinclude")',
    '    # libpython shared/static + sysconfig data for crossenv',
    '    import glob',
    '    for f in glob.glob(os.path.join(libdir, "libpython3.12*")):',
    '        add(t, f, os.path.join("lib", os.path.basename(f)))',
    '    for f in glob.glob(os.path.join(stdlib, "_sysconfigdata*.py")):',
    '        add(t, f, os.path.join("sysconfig", os.path.basename(f)))',
    '    add(t, os.path.join(prefix, "lib", "python3.12", "config-3.12"), "config")',
    'print("WROTE", out)',
    'print("PREFIX", prefix)',
    'print("INCLUDE", inc)',
    'print("LIBDIR", libdir)',
  ].join('\n');

  writeLog(`Extracting Python prefix using ${pythonPath} -> ${outDir}/pyprefix.tar.gz`);
  runProcess(pythonPath, ['-c', script], process.env, (c) => writeLog(`[extract] ${String(c).trimEnd()}`))
    .then((r) => writeLog(`Extraction finished (code=${r.code}).`));
}

function stopServer() {
  if (nodeStaticServer) {
    writeLog('Stopping node-static JupyterLab server.');
    try {
      nodeStaticServer.close();
    } catch {
      /* ignore */
    }
    nodeStaticServer = null;
  }

  if (serverProcess && !serverProcess.killed) {
    writeLog('Stopping Jupyter Server process.');
    try {
      serverProcess.kill('SIGTERM');
    } catch {
      /* ignore */
    }
  }
}

app.on('window-all-closed', () => {
  stopServer();
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', stopServer);
process.on('exit', stopServer);
