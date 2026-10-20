/////////////////////////////////////////////////////////////
//
// JupyterLab - server configuration helpers (OHOS Electron)
//
//////////////////////////////////////////////////////////////

import fs from 'fs';
import path from 'path';
import crypto from 'crypto';
import { envFirst, isOpenHarmony } from './platform.js';

export function resolvePythonPath(appDir) {
  const fromEnv = envFirst('JUPYTERLAB_PYTHON_PATH', 'NOTEBOOK_PYTHON_PATH');
  if (fromEnv) {
    return fromEnv;
  }

  const bundled = path.join(appDir, 'python', 'bin', 'python3');
  if (fs.existsSync(bundled)) {
    return bundled;
  }

  return 'python3';
}

export function resolveServerPort() {
  const raw = envFirst('JUPYTERLAB_PORT', 'NOTEBOOK_PORT', 'PORT');
  const n = raw ? Number(raw) : 8888;
  return Number.isFinite(n) && n > 0 ? n : 8888;
}

export function resolveToken() {
  const fromEnv = envFirst('JUPYTERLAB_TOKEN', 'NOTEBOOK_TOKEN', 'JUPYTER_TOKEN');
  if (fromEnv !== undefined) {
    return String(fromEnv);
  }
  return crypto.randomBytes(24).toString('hex');
}

/**
 * Notebook / Lab files root. Prefer a writable userData path on device.
 */
export function resolveRootDir(userDataDir) {
  const fromEnv = envFirst('JUPYTERLAB_ROOT_DIR', 'NOTEBOOK_ROOT_DIR');
  if (fromEnv) {
    return fromEnv;
  }
  const base = userDataDir || process.cwd();
  return path.join(base, 'notebooks');
}

/**
 * Site-packages / frontend resources mirror directory.
 */
export function resolvePyLibsDir(appDir, userDataDir) {
  const fromEnv = envFirst('JUPYTERLAB_PYLIBS_DIR', 'NOTEBOOK_PYLIBS_DIR');
  if (fromEnv) {
    return fromEnv;
  }
  const bundled = path.join(appDir, 'pylibs');
  if (fs.existsSync(bundled)) {
    return bundled;
  }
  return path.join(userDataDir || process.cwd(), 'pylibs');
}

export function resolvePipIndexUrl() {
  return (
    envFirst('JUPYTERLAB_PIP_INDEX_URL', 'NOTEBOOK_PIP_INDEX_URL', 'PIP_INDEX_URL') ||
    'https://pypi.org/simple'
  );
}

export function defaultServerMode() {
  const mode = String(
    envFirst('JUPYTERLAB_OHOS_SERVER_MODE', 'NOTEBOOK_OHOS_SERVER_MODE') || '',
  ).toLowerCase();
  if (mode === 'python' || mode === 'external' || mode === 'node-static') {
    return mode;
  }
  // On OpenHarmony, prefer node-static to avoid pyzmq/libsodium crashes.
  return isOpenHarmony() ? 'node-static' : 'python';
}
