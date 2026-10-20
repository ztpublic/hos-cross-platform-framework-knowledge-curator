/////////////////////////////////////////////////////////////
//
// JupyterLab - OpenHarmony / HarmonyOS PC platform helpers
//
//////////////////////////////////////////////////////////////

export function isOpenHarmony() {
  if (
    process.env.JUPYTERLAB_OPENHARMONY === '1' ||
    process.env.NOTEBOOK_OPENHARMONY === '1' ||
    process.env.OPENHARMONY === '1'
  ) {
    return true;
  }
  const p = String(process.platform || '').toLowerCase();
  return p.includes('ohos') || p.includes('openharmony') || p.includes('harmony');
}

export function isTruthyEnv(value) {
  if (value === undefined || value === null) return false;
  const s = String(value).trim().toLowerCase();
  return s === '1' || s === 'true' || s === 'yes' || s === 'on';
}

/**
 * Prefer JUPYTERLAB_* (Lab product). NOTEBOOK_* accepted for shared packaging scripts.
 */
export function envFirst(...keys) {
  for (const key of keys) {
    const value = process.env[key];
    if (value !== undefined && value !== null && String(value) !== '') {
      return value;
    }
  }
  return undefined;
}

export function shouldDisableGpu() {
  const explicit = envFirst('JUPYTERLAB_DISABLE_GPU', 'NOTEBOOK_DISABLE_GPU');
  if (explicit !== undefined) {
    return isTruthyEnv(explicit);
  }
  // OpenHarmony Electron: GPU path is still fragile → default off.
  return isOpenHarmony();
}

if (shouldDisableGpu()) {
  try {
    const { app } = await import('electron');
    app.disableHardwareAcceleration();
  } catch {
    // Not in Electron main yet, or electron unavailable in unit tests.
  }
}

export function shouldSpawnLocalServer() {
  if (getExternalServerUrl()) {
    return false;
  }
  const flag = envFirst('JUPYTERLAB_OHOS_LOCAL_PYTHON', 'NOTEBOOK_OHOS_LOCAL_PYTHON');
  if (flag !== undefined) {
    return isTruthyEnv(flag);
  }
  return true;
}

export function getExternalServerUrl() {
  return envFirst('JUPYTERLAB_EXTERNAL_SERVER_URL', 'NOTEBOOK_EXTERNAL_SERVER_URL') || '';
}
