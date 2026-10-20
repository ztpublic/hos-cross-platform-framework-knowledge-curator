/////////////////////////////////////////////////////////////
//
// JupyterLab - lightweight OpenHarmony local server
//
// Serves the bundled JupyterLab frontend and a small Jupyter Server-compatible
// REST surface from Electron/Node. This avoids the Python native extension stack
// on OpenHarmony devices where pyzmq/libsodium can crash during startup.
//
//////////////////////////////////////////////////////////////

import crypto from 'crypto';
import fs from 'fs';
import http from 'http';
import path from 'path';
import { pathToFileURL, URL } from 'url';
import vm from 'vm';

const JSON_HEADERS = {
  'Content-Type': 'application/json; charset=utf-8',
  'Cache-Control': 'no-store',
};

const TEXT_HEADERS = {
  'Content-Type': 'text/plain; charset=utf-8',
  'Cache-Control': 'no-store',
};

const MIME_TYPES = {
  '.css': 'text/css; charset=utf-8',
  '.gif': 'image/gif',
  '.html': 'text/html; charset=utf-8',
  '.ico': 'image/x-icon',
  '.jpeg': 'image/jpeg',
  '.jpg': 'image/jpeg',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.map': 'application/json; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.png': 'image/png',
  '.svg': 'image/svg+xml; charset=utf-8',
  '.ttf': 'font/ttf',
  '.wasm': 'application/wasm',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.zip': 'application/zip',
};

const NOTEBOOK_TEMPLATE = {
  cells: [],
  metadata: {
    kernelspec: {
      display_name: 'Python 3',
      language: 'python',
      name: 'python3',
    },
    language_info: {
      codemirror_mode: {
        name: 'ipython',
        version: 3,
      },
      file_extension: '.py',
      mimetype: 'text/x-python',
      name: 'python',
      nbconvert_exporter: 'python',
      pygments_lexer: 'ipython3',
    },
  },
  nbformat: 4,
  nbformat_minor: 5,
};

const DEFAULT_DISABLED_EXTENSIONS = [
  '@jupyterlab/application-extension:browser',
  '@jupyterlab/apputils-extension:announcements',
  '@jupyterlab/apputils-extension:licenses-plugin',
  '@jupyterlab/apputils-extension:running-sessions-status',
  '@jupyterlab/completer-extension',
  '@jupyterlab/console-extension',
  '@jupyterlab/debugger-extension',
  '@jupyterlab/extensionmanager-extension',
  '@jupyterlab/fileeditor-extension:language-server',
  '@jupyterlab/inspector-extension',
  '@jupyterlab/lsp-extension',
  '@jupyterlab/notebook-extension:language-server',
  '@jupyterlab/pluginmanager-extension',
  '@jupyterlab/running-extension',
  '@jupyterlab/terminal-extension',
  '@jupyterlab/tooltip-extension:consoles',
  '@jupyterlab/tooltip-extension:notebooks',
  '@jupyterlab/workspaces-extension:sidebar',
];

const PYODIDE_PRELUDE = [
  'import os, sys',
  'class _OhosMiniIPython:',
  '    def __init__(self):',
  '        self.user_ns = globals()',
  '    def run_line_magic(self, name, line=""):',
  '        if name == "pwd":',
  '            return os.getcwd()',
  '        if name == "who":',
  '            names = sorted(k for k in globals() if not k.startswith("_") and k not in {"In", "Out", "get_ipython"})',
  '            print(" ".join(names))',
  '            return None',
  '        raise NotImplementedError("Unsupported magic %" + name)',
  '    def system(self, command):',
  '        raise NotImplementedError("Shell commands are unavailable in the OpenHarmony in-app Python runtime")',
  '_ohos_ipython = _OhosMiniIPython()',
  'def get_ipython():',
  '    return _ohos_ipython',
].join('\n');

const SKULPT_PRELUDE = [
  'class _OhosMiniIPython:',
  '    def __init__(self):',
  '        self.user_ns = globals()',
  '    def run_line_magic(self, name, line=""):',
  '        if name == "pwd":',
  '            return "/"',
  '        if name == "who":',
  '            names = sorted(k for k in globals().keys() if not k.startswith("_") and k not in ("In", "Out", "get_ipython"))',
  '            print(" ".join(names))',
  '            return None',
  '        raise NotImplementedError("Unsupported magic %" + name)',
  '    def system(self, command):',
  '        raise NotImplementedError("Shell commands are unavailable in the OpenHarmony in-app Python runtime")',
  '_ohos_ipython = _OhosMiniIPython()',
  'def get_ipython():',
  '    return _ohos_ipython',
].join('\n');

export function startStaticLabServer(options) {
  const server = new StaticLabServer(options);
  return server.start();
}

class StaticLabServer {
  constructor(options) {
    this.appDir = options.appDir;
    this.port = options.port;
    this.rootDir = options.rootDir;
    this.token = options.token || '';
    this.userDataDir = options.userDataDir;
    this.writeLog = options.writeLog || (() => {});
    this.fallbackReason = options.fallbackReason || '';

    this.labDir = path.join(this.appDir, 'pylibs', 'jupyterlab');
    this.staticDir = path.join(this.labDir, 'static');
    this.schemasDir = path.join(this.labDir, 'schemas');
    this.themesDir = path.join(this.labDir, 'themes');
    this.workspacesDir = path.join(this.userDataDir, 'workspaces');
    this.settingsDir = path.join(this.userDataDir, 'settings');
    this.pyodideDir = path.join(this.appDir, 'pyodide');
    this.skulptDir = path.join(this.appDir, 'skulpt');

    this.schemaCache = null;
    this.settingsCache = new Map();
    this.httpServer = null;
    this.kernels = new Map();
    this.sessions = new Map();
    this.pyodideJobs = new Map();
    this.pyodidePromise = null;
    this.pyodidePreludeInstalled = false;
    this.skulptPromise = null;
    this.executionCount = 0;
  }

  start() {
    this._assertBundledAssets();
    this._ensureWritableDirs();

    this.httpServer = http.createServer((req, res) => this._handleRequest(req, res));
    this.httpServer.on('upgrade', (req, socket) => this._handleUpgrade(req, socket));

    return new Promise((resolve, reject) => {
      const onError = (error) => {
        this.httpServer?.off('error', onError);
        reject(error);
      };
      this.httpServer.once('error', onError);
      this.httpServer.listen(this.port, '127.0.0.1', () => {
        this.httpServer?.off('error', onError);
        const baseUrl = `http://127.0.0.1:${this.port}`;
        this.writeLog('JupyterLab OpenHarmony node-static runtime');
        this.writeLog('--------------------------------------------------------');
        this.writeLog(`Static lab dir      : ${this.labDir}`);
        this.writeLog(`Static assets dir   : ${this.staticDir}`);
        this.writeLog(`Schemas dir         : ${this.schemasDir}`);
        this.writeLog(`Themes dir          : ${this.themesDir}`);
        this.writeLog(`Notebook root       : ${this.rootDir}`);
        this.writeLog(`User settings dir   : ${this.settingsDir}`);
        this.writeLog(`Workspaces dir      : ${this.workspacesDir}`);
        this.writeLog(`Server URL          : ${baseUrl}`);
        this.writeLog(`Python runtime      : ${this._runtimeLabel()}`);
        this.writeLog('Terminal            : disabled in node-static mode');
        this.writeLog('--------------------------------------------------------');
        resolve({
          close: () => this.close(),
          url: `${baseUrl}/lab?token=${encodeURIComponent(this.token)}`,
        });
      });
    });
  }

  close() {
    if (!this.httpServer) {
      return;
    }
    try {
      this.httpServer.close();
    } catch {
      /* ignore */
    }
    this.httpServer = null;
  }

  _runtimeLabel() {
    if (this._hasPyodide()) {
      return 'Pyodide';
    }
    if (this._hasSkulpt()) {
      return 'Skulpt';
    }
    return 'unavailable';
  }

  _hasPyodide() {
    return typeof WebAssembly !== 'undefined' && fs.existsSync(path.join(this.pyodideDir, 'pyodide.mjs'));
  }

  _hasSkulpt() {
    return fs.existsSync(path.join(this.skulptDir, 'skulpt.min.js'));
  }

  _assertBundledAssets() {
    if (!fs.existsSync(this.staticDir)) {
      throw new Error(`Bundled JupyterLab static assets not found: ${this.staticDir}`);
    }
    if (!this._findMainBundle()) {
      throw new Error(`Bundled JupyterLab main bundle not found in: ${this.staticDir}`);
    }
  }

  _ensureWritableDirs() {
    fs.mkdirSync(this.rootDir, { recursive: true });
    fs.mkdirSync(this.workspacesDir, { recursive: true });
    fs.mkdirSync(this.settingsDir, { recursive: true });
  }

  _handleRequest(req, res) {
    const requestUrl = new URL(req.url || '/', `http://127.0.0.1:${this.port}`);
    const pathname = decodeURIComponent(requestUrl.pathname);

    this._withRequestBody(req)
      .then((body) => this._route(req, res, requestUrl, pathname, body))
      .catch((error) => {
        this.writeLog(`node-static request failed ${req.method} ${pathname}: ${error?.stack || error}`);
        this._sendJson(res, error?.status || 500, {
          message: error?.message || String(error),
          reason: error?.status ? 'Request Error' : 'Internal Server Error',
        });
      });
  }

  async _route(req, res, requestUrl, pathname, body) {
    if (req.method === 'OPTIONS') {
      this._sendEmpty(res, 204);
      return;
    }

    if (
      pathname === '/' ||
      pathname === '/lab' ||
      pathname.startsWith('/lab/tree/') ||
      pathname.startsWith('/lab/workspaces/') ||
      pathname.startsWith('/doc/')
    ) {
      this._sendHtml(res, this._renderLabHtml());
      return;
    }

    if (pathname.startsWith('/static/')) {
      this._serveFile(res, this.staticDir, pathname.slice('/static/'.length));
      return;
    }

    if (pathname.startsWith('/themes/')) {
      this._serveFile(res, this.themesDir, pathname.slice('/themes/'.length));
      return;
    }

    if (pathname.startsWith('/schemas/')) {
      this._serveFile(res, this.schemasDir, pathname.slice('/schemas/'.length));
      return;
    }

    if (pathname.startsWith('/files/')) {
      this._serveFile(res, this.rootDir, pathname.slice('/files/'.length));
      return;
    }

    if (pathname.startsWith('/pyodide/')) {
      this._serveFile(res, this.pyodideDir, pathname.slice('/pyodide/'.length));
      return;
    }

    if (pathname === '/api' || pathname === '/api/') {
      this._sendJson(res, 200, {
        version: '4.0.0',
      });
      return;
    }

    if (pathname === '/api/status') {
      this._sendJson(res, 200, {
        connections: 0,
        fallback_reason: this.fallbackReason,
        kernels: this.kernels.size,
        last_activity: new Date().toISOString(),
        mode: 'openharmony-node-python-js',
        started: new Date().toISOString(),
      });
      return;
    }

    if (pathname === '/api/me') {
      this._sendJson(res, 200, {
        identity: {
          avatar_url: '',
          color: '#5c6bc0',
          display_name: 'OpenHarmony',
          initials: 'OH',
          name: 'OpenHarmony',
          username: 'openharmony',
        },
        permissions: {},
      });
      return;
    }

    if (pathname === '/api/diagnostics/log') {
      this._handleDiagnosticsLog(req, res, body);
      return;
    }

    if (pathname === '/api/diagnostics/runtime-log') {
      this._handleRuntimeLog(req, res);
      return;
    }

    if (pathname === '/api/pyodide/jobs' || pathname === '/api/pyodide/jobs/') {
      this._handlePyodideJobs(req, res);
      return;
    }

    if (pathname.startsWith('/api/pyodide/jobs/')) {
      this._handlePyodideJob(req, res, pathname, body);
      return;
    }

    if (pathname.startsWith('/api/config/')) {
      this._handleConfig(req, res, pathname, body);
      return;
    }

    if (pathname === '/api/kernelspecs' || pathname === '/api/kernelspecs/') {
      this._sendJson(res, 200, this._kernelSpecs());
      return;
    }

    if (pathname === '/api/kernels' || pathname === '/api/kernels/') {
      if (req.method === 'GET') {
        this._sendJson(res, 200, [...this.kernels.values()].map((kernel) => this._kernelModel(kernel)));
      } else if (req.method === 'POST') {
        const payload = this._parseJsonBody(body, {});
        const kernel = this._startKernel(payload?.name || payload?.kernel?.name || 'python3');
        this._sendJson(res, 201, this._kernelModel(kernel));
      } else {
        this._sendJson(res, 405, { message: 'Method not allowed' });
      }
      return;
    }

    if (pathname.startsWith('/api/kernels/')) {
      this._handleKernel(req, res, pathname);
      return;
    }

    if (pathname === '/api/sessions' || pathname === '/api/sessions/') {
      if (req.method === 'GET') {
        this._sendJson(res, 200, [...this.sessions.values()].map((session) => this._sessionModel(session)));
      } else if (req.method === 'POST') {
        const session = this._startSession(this._parseJsonBody(body, {}));
        this._sendJson(res, 201, this._sessionModel(session));
      } else {
        this._sendJson(res, 405, { message: 'Method not allowed' });
      }
      return;
    }

    if (pathname.startsWith('/api/sessions/')) {
      this._handleSession(req, res, pathname, body);
      return;
    }

    if (pathname === '/api/terminals' || pathname === '/api/terminals/') {
      if (req.method === 'GET') {
        this._sendJson(res, 200, []);
      } else {
        this._sendJson(res, 503, this._serviceUnavailable('Terminals are unavailable in OpenHarmony node-static mode.'));
      }
      return;
    }

    if (pathname.startsWith('/api/terminals/')) {
      this._sendJson(res, req.method === 'DELETE' ? 204 : 503, req.method === 'DELETE' ? undefined : this._serviceUnavailable('Terminals are unavailable in OpenHarmony node-static mode.'));
      return;
    }

    if (pathname === '/api/nbconvert' || pathname === '/api/nbconvert/') {
      this._sendJson(res, 200, {});
      return;
    }

    if (pathname === '/api/events' || pathname === '/api/events/') {
      this._sendEmpty(res, req.method === 'POST' ? 204 : 200);
      return;
    }

    if (pathname === '/lab/api/translations' || pathname === '/lab/api/translations/') {
      this._sendJson(res, 200, {
        data: {
          en: {
            displayName: 'English',
            nativeName: 'English',
          },
        },
        message: '',
      });
      return;
    }

    if (pathname.startsWith('/lab/api/translations/')) {
      this._sendJson(res, 200, {
        data: {},
        message: '',
      });
      return;
    }

    if (pathname === '/lab/api/settings' || pathname === '/lab/api/settings/') {
      this._handleSettingsList(req, res, requestUrl);
      return;
    }

    if (pathname.startsWith('/lab/api/settings/')) {
      this._handleSingleSetting(req, res, pathname, body);
      return;
    }

    if (pathname === '/lab/api/workspaces' || pathname === '/lab/api/workspaces/') {
      this._handleWorkspacesList(req, res);
      return;
    }

    if (pathname.startsWith('/lab/api/workspaces/')) {
      this._handleWorkspace(req, res, pathname, body);
      return;
    }

    if (pathname === '/api/contents' || pathname === '/api/contents/' || pathname.startsWith('/api/contents/')) {
      this._handleContents(req, res, requestUrl, pathname, body);
      return;
    }

    if (pathname.startsWith('/api/resolvePath')) {
      const pathParam = requestUrl.searchParams.get('path') || '';
      this._sendJson(res, 200, { path: pathParam });
      return;
    }

    if (pathname === '/api/licenses' || pathname === '/api/licenses/') {
      this._sendJson(res, 200, { bundles: {} });
      return;
    }

    this.writeLog(`node-static 404 ${req.method} ${pathname}`);
    this._sendJson(res, 404, {
      message: `No route for ${req.method} ${pathname}`,
      reason: 'Not Found',
    });
  }

  _handleUpgrade(req, socket) {
    const requestUrl = new URL(req.url || '/', `http://127.0.0.1:${this.port}`);
    const kernelMatch = requestUrl.pathname.match(/^\/api\/kernels\/([^/]+)\/channels\/?$/);
    if (requestUrl.pathname !== '/api/events/subscribe' && !kernelMatch) {
      socket.destroy();
      return;
    }

    const key = req.headers['sec-websocket-key'];
    if (!key) {
      socket.destroy();
      return;
    }

    const accept = crypto
      .createHash('sha1')
      .update(`${key}258EAFA5-E914-47DA-95CA-C5AB0DC85B11`)
      .digest('base64');
    socket.write(
      [
        'HTTP/1.1 101 Switching Protocols',
        'Connection: Upgrade',
        'Upgrade: websocket',
        `Sec-WebSocket-Accept: ${accept}`,
        '',
        '',
      ].join('\r\n'),
    );
    if (kernelMatch) {
      this._attachKernelSocket(decodeURIComponent(kernelMatch[1]), socket);
    }
  }

  _renderLabHtml() {
    const mainBundle = this._findMainBundle();
    const hash = mainBundle.replace(/^main\./, '').replace(/\.js$/, '');
    const pageConfig = this._pageConfig();
    const configJson = JSON.stringify(pageConfig).replace(/</g, '\\u003c');
    const staticUrl = pageConfig.fullStaticUrl;
    return [
      '<!doctype html>',
      '<html lang="en">',
      '<head>',
      '<meta charset="utf-8">',
      '<title>JupyterLab</title>',
      '<meta name="viewport" content="width=device-width,initial-scale=1">',
      `<script id="jupyter-config-data" type="application/json">${configJson}</script>`,
      `<script>${this._diagnosticsScript()}</script>`,
      `<link rel="icon" type="image/x-icon" href="${staticUrl}/favicons/favicon.ico" class="idle favicon">`,
      `<link rel="" type="image/x-icon" href="${staticUrl}/favicons/favicon-busy-1.ico" class="busy favicon">`,
      this._hasPyodide()
        ? `<script>${this._pyodideBridgeScript()}</script>`
        : '',
      `<script defer="defer" src="${staticUrl}/${mainBundle}?v=${hash}" onload="window.__jlOhosDiag&&window.__jlOhosDiag('main.loaded',{src:this.src})" onerror="window.__jlOhosDiag&&window.__jlOhosDiag('main.error',{src:this.src})"></script>`,
      '</head>',
      '<body class="jp-ThemedContainer">',
      '<script>',
      '(function(){var location=window.location;var search=location.search;if(search.length<=1){return;}',
      "var query='?'+search.slice(1).split('&').filter(function(param){return param.split('=')[0]!=='token';}).join('&');",
      "var url=location.origin+location.pathname+(query!=='?'?query:'')+location.hash;",
      'if(url!==location.href){window.history.replaceState({}, "", url);}})();',
      '</script>',
      '</body>',
      '</html>',
    ].join('');
  }

  _diagnosticsScript() {
    return [
      '(function(){',
      "var endpoint='/api/diagnostics/log';",
      'function simplify(value){',
      '  if(value instanceof Error){return value.stack||value.message||String(value);}',
      "  if(value===undefined){return 'undefined';}",
      "  if(value===null){return 'null';}",
      "  if(typeof value==='string'){return value;}",
      '  try{return JSON.stringify(value);}catch(_){return String(value);}',
      '}',
      'function send(kind,payload){',
      '  try{',
      '    var body=JSON.stringify({kind:kind,payload:payload,time:new Date().toISOString()});',
      '    fetch(endpoint,{method:"POST",headers:{"Content-Type":"application/json"},body:body}).catch(function(){});',
      '  }catch(_){}',
      '}',
      'window.__jlOhosDiag=send;',
      "send('bootstrap',{href:location.href});",
      "['error','warn'].forEach(function(level){",
      '  var original=console[level];',
      '  console[level]=function(){',
      "    send('console.'+level,{message:Array.prototype.slice.call(arguments).map(simplify).join(' ')});",
      '    return original.apply(console,arguments);',
      '  };',
      '});',
      "window.addEventListener('error',function(event){",
      "  send('window.error',{message:event.message,source:event.filename,line:event.lineno,column:event.colno,error:simplify(event.error)});",
      '});',
      "window.addEventListener('unhandledrejection',function(event){send('unhandledrejection',{reason:simplify(event.reason)});});",
      "window.addEventListener('DOMContentLoaded',function(){send('dom.content.loaded',{bodyClass:document.body&&document.body.className});});",
      "window.addEventListener('load',function(){send('window.load',{title:document.title});});",
      'var originalFetch=window.fetch;',
      'window.fetch=function(input,init){',
      "  var url=typeof input==='string'?input:(input&&input.url)||'';",
      "  var method=(init&&init.method)||(input&&input.method)||'GET';",
      '  var started=Date.now();',
      '  return originalFetch.apply(this,arguments).then(function(response){',
      "    if(url.indexOf(endpoint)===-1&&(url.indexOf('/api/')!==-1||url.indexOf('/lab/api/')!==-1||response.status>=400)){",
      "      send('fetch',{method:method,url:url,status:response.status,ms:Date.now()-started});",
      '    }',
      '    return response;',
      '  }).catch(function(error){',
      "    send('fetch.error',{method:method,url:url,error:simplify(error),ms:Date.now()-started});",
      '    throw error;',
      '  });',
      '};',
      '})();',
    ].join('');
  }

  _pyodideBridgeScript() {
    return [
      '(function(){',
      "var jobsEndpoint='/api/pyodide/jobs';",
      'var pyodidePromise=null;',
      'var active={};',
      'function sendDiag(kind,payload){try{window.__jlOhosDiag&&window.__jlOhosDiag(kind,payload);}catch(_){}}',
      'function magicToPython(code){',
      '  return String(code||"").split("\\n").map(function(line){',
      '    var trimmed=line.trim();',
      '    if(trimmed==="%pwd"){return "import os\\nos.getcwd()";}',
      '    if(trimmed==="%who"){return "print(\\", \\".join(sorted(k for k in globals() if not k.startswith(\\"_\\") and k not in {\\"In\\",\\"Out\\",\\"get_ipython\\"})))";}',
      '    if(trimmed.indexOf("%time ")===0){return "import time as _jl_time\\n_jl_start=_jl_time.perf_counter()\\n_jl_value=("+trimmed.slice(6)+")\\nprint(\\"CPU times: %.3f ms\\" % ((_jl_time.perf_counter()-_jl_start)*1000))\\n_jl_value";}',
      '    return line;',
      '  }).join("\\n");',
      '}',
      'async function getPyodide(){',
      '  if(!pyodidePromise){',
      "    pyodidePromise=import('/pyodide/pyodide.mjs').then(function(mod){",
      "      return mod.loadPyodide({indexURL:'/pyodide/'}).then(async function(pyodide){",
      '        await pyodide.runPythonAsync(' + JSON.stringify(PYODIDE_PRELUDE) + ');',
      "        sendDiag('pyodide.ready',{version:pyodide.runPython('sys.version')});",
      '        return pyodide;',
      '      });',
      '    });',
      '  }',
      '  return pyodidePromise;',
      '}',
      'function stringifyResult(value){',
      "  if(value===undefined||value===null){return '';}",
      "  try{return String(value);}catch(_){return '[object]';}",
      '}',
      'async function postResult(id,payload){',
      "  await fetch(jobsEndpoint+'/'+encodeURIComponent(id),{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});",
      '}',
      'async function runJob(job){',
      '  if(active[job.id]){return;}',
      '  active[job.id]=true;',
      '  var stdout=[];',
      '  var stderr=[];',
      '  try{',
      '    var pyodide=await getPyodide();',
      '    pyodide.setStdout({batched:function(text){stdout.push(text);}});',
      '    pyodide.setStderr({batched:function(text){stderr.push(text);}});',
      '    var result=await pyodide.runPythonAsync(magicToPython(job.code));',
      '    var text=stringifyResult(result);',
      '    if(result&&typeof result.destroy==="function"){result.destroy();}',
      "    await postResult(job.id,{ok:true,result:text,hasResult:text!=='',stdout:stdout,stderr:stderr});",
      "    sendDiag('pyodide.job.done',{id:job.id});",
      '  }catch(error){',
      "    await postResult(job.id,{ok:false,error:{name:error&&error.name||'Error',message:error&&error.message||String(error),stack:error&&error.stack||''},stdout:stdout,stderr:stderr});",
      "    sendDiag('pyodide.job.error',{id:job.id,message:error&&error.message||String(error)});",
      '  }finally{',
      '    delete active[job.id];',
      '  }',
      '}',
      'async function poll(){',
      '  try{',
      "    var response=await fetch(jobsEndpoint,{cache:'no-store'});",
      '    if(response.ok){',
      '      var jobs=await response.json();',
      '      jobs.forEach(runJob);',
      '    }',
      '  }catch(error){',
      "    sendDiag('pyodide.poll.error',{message:error&&error.message||String(error)});",
      '  }finally{',
      '    setTimeout(poll,300);',
      '  }',
      '}',
      'poll();',
      '})();',
    ].join('');
  }

  _pageConfig() {
    return {
      allow_hidden_files: true,
      appName: 'JupyterLab',
      appNamespace: 'jupyterlab',
      appSettingsDir: '',
      appUrl: 'lab',
      appendToken: 'false',
      baseUrl: '/',
      buildAvailable: false,
      buildCheck: false,
      cacheFiles: false,
      collaborative: false,
      customCss: false,
      delete_to_trash: false,
      deferredExtensions: [],
      disabledExtensions: DEFAULT_DISABLED_EXTENSIONS,
      devMode: false,
      docUrl: 'doc',
      exposeAppInBrowser: false,
      extensionManager: { name: 'readonly', can_install: false, install_path: '' },
      federated_extensions: [],
      fullLabextensionsUrl: '/lab/extensions',
      fullLicensesUrl: '/api/licenses',
      fullMathjaxUrl: '',
      fullStaticUrl: '/static',
      hubHost: '',
      hubPrefix: '',
      hubServerName: '',
      hubUser: '',
      labextensionsUrl: 'lab/extensions',
      licensesUrl: 'api/licenses',
      mathjaxConfig: '',
      mathjaxUrl: '',
      mode: 'multiple-document',
      news: { disabled: true },
      notebookVersion: JSON.stringify([7, 0, 0, '']),
      quitButton: false,
      schemasDir: '',
      serverMode: 'openharmony-node-python-js',
      serverRoot: this.rootDir,
      settingsUrl: 'api/settings',
      shareUrl: '/',
      staticDir: this.staticDir,
      staticUrl: 'static',
      terminalsAvailable: false,
      templatesDir: '',
      themesDir: this.themesDir,
      themesUrl: 'themes',
      token: this.token,
      translationsApiUrl: 'lab/api/translations',
      treeUrl: 'tree',
      userSettingsDir: this.settingsDir,
      workspace: 'default',
      workspacesDir: this.workspacesDir,
      wsUrl: `ws://127.0.0.1:${this.port}/`,
    };
  }

  _findMainBundle() {
    try {
      return fs.readdirSync(this.staticDir).find((name) => /^main\..*\.js$/.test(name));
    } catch {
      return '';
    }
  }

  _handleConfig(req, res, pathname, body) {
    const section = pathname.slice('/api/config/'.length);
    const file = this._configFile(section);
    if (req.method === 'GET') {
      this._sendJson(res, 200, this._readJson(file, {}));
      return;
    }
    if (req.method === 'PATCH' || req.method === 'PUT') {
      const current = this._readJson(file, {});
      const patch = this._parseJsonBody(body, {});
      const next = { ...current, ...patch };
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, JSON.stringify(next, null, 2) + '\n');
      this._sendJson(res, 200, next);
      return;
    }
    this._sendMethodNotAllowed(res);
  }

  _handleDiagnosticsLog(req, res, body) {
    if (req.method !== 'POST') {
      this._sendMethodNotAllowed(res);
      return;
    }
    const payload = this._parseJsonBody(body, null);
    if (payload && typeof payload === 'object') {
      const kind = typeof payload.kind === 'string' ? payload.kind : 'message';
      this.writeLog(`[frontend:${kind}] ${JSON.stringify(payload.payload ?? {})}`);
    } else {
      this.writeLog(`[frontend] ${body?.toString('utf8') || ''}`);
    }
    this._sendEmpty(res, 204);
  }

  _handleRuntimeLog(req, res) {
    if (req.method !== 'GET') {
      this._sendMethodNotAllowed(res);
      return;
    }
    const logFile = path.join(this.userDataDir, 'jupyterlab-runtime.log');
    try {
      const log = fs.readFileSync(logFile, 'utf8');
      this._sendText(res, 200, log.slice(-128 * 1024));
    } catch (error) {
      this._sendJson(res, 404, {
        message: `Runtime log not available: ${error?.message || error}`,
        reason: 'Not Found',
      });
    }
  }

  _configFile(section) {
    const safe = section.replace(/[^A-Za-z0-9_.-]/g, '_') || 'default';
    return path.join(this.userDataDir, 'config', `${safe}.json`);
  }

  _handleSettingsList(req, res, requestUrl) {
    if (req.method !== 'GET') {
      this._sendMethodNotAllowed(res);
      return;
    }
    const idsOnly = requestUrl.searchParams.get('ids_only') === 'true';
    const plugins = this._listSettingPlugins(idsOnly);
    this._sendJson(res, 200, {
      settings: plugins,
      warnings: [],
    });
  }

  _handleSingleSetting(req, res, pathname, body) {
    const id = pathname.slice('/lab/api/settings/'.length);
    if (req.method === 'GET') {
      const plugin = this._settingPlugin(id);
      if (!plugin) {
        this._sendJson(res, 404, { message: `Settings not found: ${id}` });
        return;
      }
      this._sendJson(res, 200, plugin);
      return;
    }
    if (req.method === 'PUT') {
      const payload = this._parseJsonBody(body, {});
      const raw = typeof payload.raw === 'string' ? payload.raw : '{}';
      this._writeSettingRaw(id, raw);
      this.settingsCache.delete(id);
      this._sendEmpty(res, 204);
      return;
    }
    this._sendMethodNotAllowed(res);
  }

  _listSettingPlugins(idsOnly) {
    const entries = this._schemaEntries();
    return entries.map(({ id }) => (idsOnly ? { id } : this._settingPlugin(id))).filter(Boolean);
  }

  _settingPlugin(id) {
    if (this.settingsCache.has(id)) {
      return this.settingsCache.get(id);
    }
    const entry = this._schemaEntries().find((item) => item.id === id);
    if (!entry) {
      return null;
    }
    const schema = this._readJson(entry.file, {});
    const raw = this._readSettingRaw(id);
    const user = this._parseJsonText(raw, {});
    const composite = this._composeDefaults(schema, user);
    const plugin = {
      created: null,
      data: {
        composite,
        user,
      },
      id,
      last_modified: null,
      raw,
      schema,
      settings: user,
      version: this._schemaVersion(entry.file),
    };
    this.settingsCache.set(id, plugin);
    return plugin;
  }

  _schemaEntries() {
    if (this.schemaCache) {
      return this.schemaCache;
    }
    const entries = [];
    this._walkFiles(this.schemasDir, (file, relative) => {
      if (!relative.endsWith('.json')) {
        return;
      }
      const withoutExt = relative.slice(0, -'.json'.length).split(path.sep).join('/');
      const slash = withoutExt.lastIndexOf('/');
      if (slash === -1) {
        entries.push({ file, id: withoutExt });
        return;
      }
      entries.push({
        file,
        id: `${withoutExt.slice(0, slash)}:${withoutExt.slice(slash + 1)}`,
      });
    });
    entries.sort((a, b) => a.id.localeCompare(b.id));
    this.schemaCache = entries;
    return entries;
  }

  _readSettingRaw(id) {
    const file = this._settingFile(id);
    if (!fs.existsSync(file)) {
      return '{}';
    }
    try {
      return fs.readFileSync(file, 'utf8') || '{}';
    } catch {
      return '{}';
    }
  }

  _writeSettingRaw(id, raw) {
    const file = this._settingFile(id);
    fs.mkdirSync(path.dirname(file), { recursive: true });
    fs.writeFileSync(file, raw || '{}');
  }

  _settingFile(id) {
    const normalized = id.replace(':', '/');
    const safeParts = normalized
      .split('/')
      .filter(Boolean)
      .map((part) => part.replace(/[^A-Za-z0-9@_.-]/g, '_'));
    return path.join(this.settingsDir, ...safeParts) + '.jupyterlab-settings';
  }

  _schemaVersion(schemaFile) {
    const packageFile = path.join(path.dirname(schemaFile), 'package.json.orig');
    const packageJson = this._readJson(packageFile, null);
    return packageJson?.version || 'N/A';
  }

  _composeDefaults(schema, user) {
    const composite = {};
    const properties = schema?.properties && typeof schema.properties === 'object' ? schema.properties : {};
    for (const [key, value] of Object.entries(properties)) {
      if (Object.prototype.hasOwnProperty.call(value, 'default')) {
        composite[key] = value.default;
      }
    }
    return {
      ...composite,
      ...(user && typeof user === 'object' && !Array.isArray(user) ? user : {}),
    };
  }

  _handleWorkspacesList(req, res) {
    if (req.method !== 'GET') {
      this._sendMethodNotAllowed(res);
      return;
    }
    const values = this._listWorkspaces();
    this._sendJson(res, 200, {
      workspaces: {
        ids: values.map((workspace) => workspace.metadata.id),
        values,
      },
    });
  }

  _handleWorkspace(req, res, pathname, body) {
    const id = pathname.slice('/lab/api/workspaces/'.length) || 'default';
    const file = this._workspaceFile(id);
    if (req.method === 'GET') {
      this._sendJson(res, 200, this._readWorkspace(id, file));
      return;
    }
    if (req.method === 'PUT') {
      const incoming = this._parseJsonBody(body, {});
      const now = new Date().toISOString();
      const workspace = {
        data: incoming.data && typeof incoming.data === 'object' ? incoming.data : {},
        metadata: {
          ...(incoming.metadata && typeof incoming.metadata === 'object' ? incoming.metadata : {}),
          id,
          last_modified: now,
        },
      };
      if (!workspace.metadata.created) {
        workspace.metadata.created = fs.existsSync(file) ? this._readWorkspace(id, file).metadata.created : now;
      }
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, JSON.stringify(workspace, null, 2) + '\n');
      this._sendEmpty(res, 204);
      return;
    }
    if (req.method === 'DELETE') {
      fs.rmSync(file, { force: true });
      this._sendEmpty(res, 204);
      return;
    }
    this._sendMethodNotAllowed(res);
  }

  _listWorkspaces() {
    const values = [];
    this._walkFiles(this.workspacesDir, (file, relative) => {
      if (!relative.endsWith('.json')) {
        return;
      }
      const id = relative.slice(0, -'.json'.length).split(path.sep).join('/');
      values.push(this._readWorkspace(id, file));
    });
    if (!values.some((workspace) => workspace.metadata.id === 'default')) {
      values.push(this._emptyWorkspace('default'));
    }
    values.sort((a, b) => a.metadata.id.localeCompare(b.metadata.id));
    return values;
  }

  _readWorkspace(id, file) {
    const workspace = this._readJson(file, null);
    if (workspace?.metadata?.id && workspace.data && typeof workspace.data === 'object') {
      return workspace;
    }
    return this._emptyWorkspace(id);
  }

  _emptyWorkspace(id) {
    const now = new Date().toISOString();
    return {
      data: {},
      metadata: {
        created: now,
        id,
        last_modified: now,
      },
    };
  }

  _workspaceFile(id) {
    const safeParts = id
      .split('/')
      .filter(Boolean)
      .map((part) => part.replace(/[^A-Za-z0-9_.-]/g, '_'));
    return path.join(this.workspacesDir, ...(safeParts.length ? safeParts : ['default'])) + '.json';
  }

  _handleContents(req, res, requestUrl, pathname, body) {
    const contentPath = pathname.startsWith('/api/contents/')
      ? pathname.slice('/api/contents/'.length)
      : '';
    const checkpoint = this._checkpointRequest(contentPath);
    if (checkpoint) {
      this._handleCheckpoint(req, res, checkpoint);
      return;
    }

    if (req.method === 'GET') {
      const includeContent = requestUrl.searchParams.get('content') !== '0';
      const format = requestUrl.searchParams.get('format') || undefined;
      const type = requestUrl.searchParams.get('type') || undefined;
      this._sendJson(res, 200, this._contentModel(contentPath, { content: includeContent, format, type }));
      return;
    }

    if (req.method === 'PUT') {
      const model = this._parseJsonBody(body, {});
      const existed = fs.existsSync(this._contentPath(contentPath));
      const saved = this._saveContent(contentPath, model);
      this._sendJson(res, existed ? 200 : 201, saved);
      return;
    }

    if (req.method === 'POST') {
      const payload = this._parseJsonBody(body, {});
      if (payload.copy_from) {
        this._sendJson(res, 201, this._copyContent(payload.copy_from, contentPath));
        return;
      }
      this._sendJson(res, 201, this._newUntitled(contentPath, payload));
      return;
    }

    if (req.method === 'PATCH') {
      const payload = this._parseJsonBody(body, {});
      if (typeof payload.path !== 'string') {
        this._sendJson(res, 400, { message: 'PATCH requires a path field.' });
        return;
      }
      this._sendJson(res, 200, this._renameContent(contentPath, payload.path));
      return;
    }

    if (req.method === 'DELETE') {
      const full = this._contentPath(contentPath);
      fs.rmSync(full, { recursive: true, force: true });
      this._sendEmpty(res, 204);
      return;
    }

    this._sendMethodNotAllowed(res);
  }

  _checkpointRequest(contentPath) {
    const parts = contentPath.split('/').filter(Boolean);
    const index = parts.indexOf('checkpoints');
    if (index === -1) {
      return null;
    }
    return {
      checkpointId: parts[index + 1] || '',
      path: parts.slice(0, index).join('/'),
    };
  }

  _handleCheckpoint(req, res, checkpoint) {
    if (req.method === 'GET') {
      this._sendJson(res, 200, []);
      return;
    }
    if (req.method === 'POST' && !checkpoint.checkpointId) {
      this._sendJson(res, 201, {
        id: 'checkpoint',
        last_modified: new Date().toISOString(),
      });
      return;
    }
    if (req.method === 'POST' || req.method === 'DELETE') {
      this._sendEmpty(res, 204);
      return;
    }
    this._sendMethodNotAllowed(res);
  }

  _contentModel(contentPath, options = {}) {
    const full = this._contentPath(contentPath);
    if (!fs.existsSync(full)) {
      throw new HttpError(404, `No such file or directory: ${contentPath || '/'}`);
    }
    const stat = fs.statSync(full);
    const iso = stat.mtime.toISOString();
    const name = path.posix.basename(contentPath || '');
    if (stat.isDirectory()) {
      const children = options.content
        ? fs
            .readdirSync(full, { withFileTypes: true })
            .filter((entry) => !entry.name.startsWith('.'))
            .map((entry) => this._contentModel(path.posix.join(contentPath, entry.name), { content: false }))
            .sort((a, b) => (a.type === b.type ? a.name.localeCompare(b.name) : a.type === 'directory' ? -1 : 1))
        : null;
      return {
        content: children,
        created: iso,
        format: 'json',
        last_modified: iso,
        mimetype: null,
        name,
        path: contentPath,
        size: undefined,
        type: 'directory',
        writable: true,
      };
    }

    const type = options.type || (contentPath.endsWith('.ipynb') ? 'notebook' : 'file');
    const model = {
      content: null,
      created: iso,
      format: null,
      last_modified: iso,
      mimetype: type === 'notebook' ? null : this._mimeType(full),
      name,
      path: contentPath,
      size: stat.size,
      type,
      writable: true,
    };
    if (!options.content) {
      return model;
    }

    if (type === 'notebook') {
      model.format = 'json';
      model.content = this._readJson(full, NOTEBOOK_TEMPLATE);
      return model;
    }

    const binary = options.format === 'base64' || this._isLikelyBinary(full);
    model.format = binary ? 'base64' : 'text';
    model.content = fs.readFileSync(full, binary ? 'base64' : 'utf8');
    return model;
  }

  _saveContent(contentPath, model) {
    const full = this._contentPath(contentPath);
    const existed = fs.existsSync(full);
    fs.mkdirSync(path.dirname(full), { recursive: true });

    if (model.type === 'directory') {
      fs.mkdirSync(full, { recursive: true });
    } else if (model.type === 'notebook') {
      const content = model.content && typeof model.content === 'object' ? model.content : NOTEBOOK_TEMPLATE;
      fs.writeFileSync(full, JSON.stringify(content, null, 2) + '\n');
    } else {
      const content = typeof model.content === 'string' ? model.content : '';
      if (model.format === 'base64') {
        fs.writeFileSync(full, Buffer.from(content, 'base64'));
      } else {
        fs.writeFileSync(full, content, 'utf8');
      }
    }

    return this._contentModel(contentPath, { content: false, type: model.type || (existed && contentPath.endsWith('.ipynb') ? 'notebook' : undefined) });
  }

  _newUntitled(parentPath, options) {
    const type = options.type || 'file';
    const ext = type === 'notebook' ? '.ipynb' : options.ext || '.txt';
    const base = type === 'directory' ? 'Untitled Folder' : type === 'notebook' ? 'Untitled' : 'untitled';
    const dir = this._contentPath(parentPath);
    fs.mkdirSync(dir, { recursive: true });
    const name = this._unusedName(dir, base, type === 'directory' ? '' : ext);
    const rel = path.posix.join(parentPath, name);
    const full = this._contentPath(rel);
    if (type === 'directory') {
      fs.mkdirSync(full, { recursive: true });
    } else if (type === 'notebook') {
      fs.writeFileSync(full, JSON.stringify(NOTEBOOK_TEMPLATE, null, 2) + '\n');
    } else {
      fs.writeFileSync(full, '', 'utf8');
    }
    return this._contentModel(rel, { content: false, type });
  }

  _copyContent(fromPath, toDirPath) {
    const from = this._contentPath(fromPath);
    const toDir = this._contentPath(toDirPath);
    if (!fs.existsSync(from)) {
      throw new HttpError(404, `No such file: ${fromPath}`);
    }
    fs.mkdirSync(toDir, { recursive: true });
    const parsed = path.parse(fromPath);
    const name = this._unusedName(toDir, `${parsed.name}-Copy`, parsed.ext);
    const rel = path.posix.join(toDirPath, name);
    const to = this._contentPath(rel);
    fs.cpSync(from, to, { recursive: true });
    return this._contentModel(rel, { content: false });
  }

  _renameContent(fromPath, toPath) {
    const from = this._contentPath(fromPath);
    const to = this._contentPath(toPath);
    if (!fs.existsSync(from)) {
      throw new HttpError(404, `No such file: ${fromPath}`);
    }
    fs.mkdirSync(path.dirname(to), { recursive: true });
    fs.renameSync(from, to);
    return this._contentModel(toPath, { content: false });
  }

  _unusedName(dir, base, ext) {
    let name = `${base}${ext}`;
    let index = 1;
    while (fs.existsSync(path.join(dir, name))) {
      name = `${base}${index}${ext}`;
      index += 1;
    }
    return name;
  }

  _contentPath(contentPath) {
    const normalized = path
      .posix
      .normalize(`/${contentPath || ''}`)
      .replace(/^\/+/, '');
    const full = path.resolve(this.rootDir, normalized);
    const relative = path.relative(this.rootDir, full);
    if (relative.startsWith('..') || path.isAbsolute(relative)) {
      throw new HttpError(403, `Path escapes notebook root: ${contentPath}`);
    }
    return full;
  }

  _startKernel(name = 'python3') {
    const normalizedName = name === 'python' ? 'python3' : name;
    if (!this._availableKernelNames().includes(normalizedName)) {
      throw new HttpError(404, `No such kernel: ${name}`);
    }
    const now = new Date().toISOString();
    const kernel = {
      id: crypto.randomUUID(),
      name: normalizedName,
      execution_state: 'idle',
      connections: 0,
      last_activity: now,
    };
    this.kernels.set(kernel.id, kernel);
    return kernel;
  }

  _kernelModel(kernel) {
    return {
      id: kernel.id,
      name: kernel.name,
      execution_state: kernel.execution_state,
      connections: kernel.connections,
      last_activity: kernel.last_activity,
    };
  }

  _handleKernel(req, res, pathname) {
    const parts = pathname.split('/').map((part) => decodeURIComponent(part));
    const id = parts[3];
    const action = parts[4] || '';
    const kernel = this.kernels.get(id);
    if (!kernel) {
      this._sendJson(res, 404, { message: `Kernel does not exist: ${id}` });
      return;
    }
    if (!action) {
      if (req.method === 'GET') {
        this._sendJson(res, 200, this._kernelModel(kernel));
      } else if (req.method === 'DELETE') {
        this.kernels.delete(id);
        for (const [sessionId, session] of this.sessions) {
          if (session.kernel.id === id) {
            this.sessions.delete(sessionId);
          }
        }
        this._sendEmpty(res, 204);
      } else {
        this._sendJson(res, 405, { message: 'Method not allowed' });
      }
      return;
    }
    if (req.method === 'POST' && (action === 'interrupt' || action === 'restart')) {
      kernel.execution_state = 'idle';
      kernel.last_activity = new Date().toISOString();
      this._sendJson(res, 200, action === 'restart' ? this._kernelModel(kernel) : {});
      return;
    }
    this._sendJson(res, 404, { message: `No kernel route: ${pathname}` });
  }

  _startSession(body) {
    const requested = body.kernel || {};
    const kernel = requested.id && this.kernels.get(requested.id)
      ? this.kernels.get(requested.id)
      : this._startKernel(requested.name || 'python3');
    const now = new Date().toISOString();
    const session = {
      id: crypto.randomUUID(),
      path: body.path || '',
      name: body.name || '',
      type: body.type || 'notebook',
      kernel,
      notebook: {
        path: body.path || '',
        name: body.name || '',
      },
      last_activity: now,
    };
    this.sessions.set(session.id, session);
    return session;
  }

  _sessionModel(session) {
    return {
      id: session.id,
      path: session.path,
      name: session.name,
      type: session.type,
      kernel: this._kernelModel(session.kernel),
      notebook: session.notebook,
    };
  }

  _handleSession(req, res, pathname, body) {
    const id = decodeURIComponent(pathname.split('/')[3] || '');
    const session = this.sessions.get(id);
    if (!session) {
      this._sendJson(res, 404, { message: `Session does not exist: ${id}` });
      return;
    }
    if (req.method === 'GET') {
      this._sendJson(res, 200, this._sessionModel(session));
      return;
    }
    if (req.method === 'DELETE') {
      this.sessions.delete(id);
      this._sendEmpty(res, 204);
      return;
    }
    if (req.method === 'PATCH') {
      const payload = this._parseJsonBody(body, {});
      if (payload?.path !== undefined) {
        session.path = payload.path;
        session.notebook.path = payload.path;
      }
      if (payload?.name !== undefined) {
        session.name = payload.name;
        session.notebook.name = payload.name;
      }
      if (payload?.kernel) {
        session.kernel = payload.kernel.id && this.kernels.get(payload.kernel.id)
          ? this.kernels.get(payload.kernel.id)
          : this._startKernel(payload.kernel.name || 'python3');
      }
      this._sendJson(res, 200, this._sessionModel(session));
      return;
    }
    this._sendJson(res, 405, { message: 'Method not allowed' });
  }

  _handlePyodideJobs(req, res) {
    if (req.method !== 'GET') {
      this._sendJson(res, 405, { message: 'Method not allowed' });
      return;
    }
    const jobs = [...this.pyodideJobs.values()]
      .filter((job) => job.status === 'pending')
      .map((job) => ({ id: job.id, code: job.code }));
    this._sendJson(res, 200, jobs);
  }

  _handlePyodideJob(req, res, pathname, body) {
    if (req.method !== 'POST') {
      this._sendJson(res, 405, { message: 'Method not allowed' });
      return;
    }
    const payload = this._parseJsonBody(body, {});
    const id = decodeURIComponent(pathname.split('/')[4] || '');
    const job = this.pyodideJobs.get(id);
    if (!job) {
      this._sendJson(res, 404, { message: `Pyodide job does not exist: ${id}` });
      return;
    }
    this.pyodideJobs.delete(id);
    clearTimeout(job.timeout);
    if (payload?.ok) {
      job.resolve({
        result: payload.result === undefined ? '' : String(payload.result),
        hasResult: Boolean(payload.hasResult),
        stdout: Array.isArray(payload.stdout) ? payload.stdout.map(String) : [],
        stderr: Array.isArray(payload.stderr) ? payload.stderr.map(String) : [],
      });
    } else {
      const error = new Error(payload?.error?.message || 'Pyodide browser execution failed.');
      error.name = payload?.error?.name || 'Error';
      error.stdout = Array.isArray(payload?.stdout) ? payload.stdout.map(String) : [];
      error.stderr = Array.isArray(payload?.stderr) ? payload.stderr.map(String) : [];
      job.reject(error);
    }
    this._sendEmpty(res, 204);
  }

  async _loadPyodide() {
    if (!this.pyodidePromise) {
      const modulePath = path.join(this.pyodideDir, 'pyodide.mjs');
      if (!fs.existsSync(modulePath)) {
        throw new Error(`Bundled Pyodide runtime is missing: ${modulePath}`);
      }
      this.pyodidePromise = import(pathToFileURL(modulePath).href).then((mod) =>
        mod.loadPyodide({ indexURL: `${this.pyodideDir}/` }),
      );
    }
    const pyodide = await this.pyodidePromise;
    if (!this.pyodidePreludeInstalled) {
      await pyodide.runPythonAsync(PYODIDE_PRELUDE);
      this.pyodidePreludeInstalled = true;
    }
    return pyodide;
  }

  _preprocessInteractivePython(code) {
    return String(code || '')
      .replace(/\r\n/g, '\n')
      .split('\n')
      .map((line) => {
        const trimmed = line.trim();
        if (trimmed === '%pwd') {
          return 'import os\nos.getcwd()';
        }
        if (trimmed === '%who') {
          return 'print(", ".join(sorted(k for k in globals() if not k.startswith("_") and k not in {"In", "Out", "get_ipython"})))';
        }
        if (trimmed.startsWith('%time ')) {
          const expression = trimmed.slice('%time '.length);
          return [
            'import time as _jl_time',
            '_jl_start = _jl_time.perf_counter()',
            `_jl_value = (${expression})`,
            'print("CPU times: %.3f ms" % ((_jl_time.perf_counter() - _jl_start) * 1000))',
            '_jl_value',
          ].join('\n');
        }
        return line;
      })
      .join('\n');
  }

  _preprocessSkulptPython(code) {
    return String(code || '')
      .replace(/\r\n/g, '\n')
      .split('\n')
      .map((line) => {
        const trimmed = line.trim();
        if (trimmed === '%pwd') {
          return '"/"';
        }
        if (trimmed === '%who') {
          return 'print(" ".join(sorted(k for k in globals().keys() if not k.startswith("_") and k not in ("In", "Out", "get_ipython"))))';
        }
        if (trimmed.startsWith('%time ')) {
          return trimmed.slice('%time '.length);
        }
        return line;
      })
      .join('\n');
  }

  _runPythonInBrowser(code) {
    return new Promise((resolve, reject) => {
      const id = crypto.randomUUID();
      const timeout = setTimeout(() => {
        this.pyodideJobs.delete(id);
        reject(new Error('Timed out waiting for the WebView Pyodide executor.'));
      }, 45000);
      this.pyodideJobs.set(id, {
        id,
        code: this._preprocessInteractivePython(code),
        status: 'pending',
        resolve,
        reject,
        timeout,
      });
    });
  }

  async _runPythonInNode(code) {
    const stdout = [];
    const stderr = [];
    const pyodide = await this._loadPyodide();
    pyodide.setStdout({ batched: (text) => stdout.push(text) });
    pyodide.setStderr({ batched: (text) => stderr.push(text) });
    const result = await pyodide.runPythonAsync(this._preprocessInteractivePython(code));
    let text = '';
    let hasResult = false;
    if (result !== undefined && result !== null) {
      text = String(result);
      hasResult = true;
      result?.destroy?.();
    }
    return { result: text, hasResult, stdout, stderr };
  }

  async _loadSkulpt() {
    if (!this.skulptPromise) {
      this.skulptPromise = Promise.resolve().then(() => {
        const skulptPath = path.join(this.skulptDir, 'skulpt.min.js');
        const stdlibPath = path.join(this.skulptDir, 'skulpt-stdlib.js');
        if (!fs.existsSync(skulptPath) || !fs.existsSync(stdlibPath)) {
          throw new Error(`Bundled Skulpt runtime is missing: ${this.skulptDir}`);
        }
        const context = {
          clearTimeout,
          console,
          setTimeout,
        };
        context.global = context;
        context.globalThis = context;
        context.window = context;
        vm.createContext(context);
        vm.runInContext(fs.readFileSync(skulptPath, 'utf8'), context, { filename: skulptPath });
        vm.runInContext(fs.readFileSync(stdlibPath, 'utf8'), context, { filename: stdlibPath });
        if (!context.Sk) {
          throw new Error('Skulpt did not initialize.');
        }
        return context.Sk;
      });
    }
    return this.skulptPromise;
  }

  _transformPythonForNotebook(code) {
    const normalized = String(code || '').replace(/\r\n/g, '\n');
    const lines = normalized.split('\n');
    let lastIndex = -1;
    for (let index = lines.length - 1; index >= 0; index -= 1) {
      if (lines[index].trim()) {
        lastIndex = index;
        break;
      }
    }
    if (lastIndex < 0) {
      return { code: normalized, expression: false };
    }
    const line = lines[lastIndex];
    const trimmed = line.trim();
    const statementPattern = /^(assert|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|lambda|nonlocal|pass|raise|return|try|while|with|yield)\b/;
    const assignmentPattern = /(^|[^=!<>])=(?!=)|\+=|-=|\*=|\/=|%=|\*\*=|\/\/=|:=/;
    if (/^\s/.test(line) || trimmed.endsWith(':') || statementPattern.test(trimmed) || assignmentPattern.test(trimmed)) {
      return { code: normalized, expression: false };
    }
    lines[lastIndex] = `${line.slice(0, line.length - trimmed.length)}__jl_expr__ = (${trimmed})`;
    return { code: lines.join('\n'), expression: true };
  }

  _shouldPersistSkulptCode(code) {
    const trimmed = String(code || '').trim();
    return /^(class|def|from|import)\b/.test(trimmed) || /(^|[^=!<>])=(?!=)|\+=|-=|\*=|\/=|%=|\*\*=|\/\/=|:=/.test(trimmed);
  }

  async _runPythonInSkulpt(kernel, code) {
    const Sk = await this._loadSkulpt();
    const stdout = [];
    const stderr = [];
    const executableCode = this._preprocessSkulptPython(code);
    const transformed = this._transformPythonForNotebook(executableCode);
    const prelude = kernel.skulptPrelude || '';
    const source = [SKULPT_PRELUDE, prelude, transformed.code].filter(Boolean).join('\n');
    Sk.configure({
      __future__: Sk.python3,
      output: (text) => stdout.push(text),
      read: (name) => {
        const files = Sk.builtinFiles?.files || {};
        if (files[name]) {
          return files[name];
        }
        throw new Error(`Skulpt file not found: ${name}`);
      },
    });
    const module = await Sk.misceval.asyncToPromise(() =>
      Sk.importMainWithBody('<stdin>', false, source, true),
    );
    let result = '';
    let hasResult = false;
    if (transformed.expression) {
      const value = module?.$d?.__jl_expr__;
      if (value !== undefined) {
        result = String(Sk.misceval.objectRepr(value));
        hasResult = true;
      }
    }
    if (this._shouldPersistSkulptCode(executableCode)) {
      kernel.skulptPrelude = [kernel.skulptPrelude || '', executableCode].filter(Boolean).join('\n');
    }
    return { result, hasResult, stdout, stderr };
  }

  async _runPython(kernel, code) {
    const runtime = this._runtimeNameForKernel(kernel);
    let triedBrowserPyodide = false;
    if (runtime === 'pyodide' && typeof WebAssembly !== 'undefined') {
      try {
        return await this._runPythonInNode(code);
      } catch (error) {
        this.writeLog(`Node Pyodide execution unavailable: ${error?.message || error}`);
        if (!/WebAssembly is not defined/i.test(String(error?.message || error))) {
          try {
            triedBrowserPyodide = true;
            return await this._runPythonInBrowser(code);
          } catch (browserError) {
            this.writeLog(`Browser Pyodide execution unavailable, falling back to Skulpt: ${browserError?.message || browserError}`);
          }
        }
      }
    }
    if (runtime === 'pyodide' && !triedBrowserPyodide) {
      try {
        return await this._runPythonInBrowser(code);
      } catch (error) {
        this.writeLog(`Browser Pyodide execution unavailable, falling back to Skulpt: ${error?.message || error}`);
      }
    }
    return this._runPythonInSkulpt(kernel, code);
  }

  _attachKernelSocket(kernelId, socket) {
    const kernel = this.kernels.get(kernelId);
    if (!kernel) {
      socket.end();
      return;
    }
    kernel.connections += 1;
    socket.__jlBuffer = Buffer.alloc(0);
    socket.on('data', (chunk) => {
      const result = this._decodeWebSocketFrames(Buffer.concat([socket.__jlBuffer, chunk]), socket);
      socket.__jlBuffer = result.remaining;
      for (const payload of result.messages) {
        this._handleKernelMessage(kernel, socket, payload).catch((error) => {
          this.writeLog(`JavaScript Python kernel message failed: ${error?.stack || error}`);
        });
      }
    });
    socket.on('close', () => {
      kernel.connections = Math.max(0, kernel.connections - 1);
    });
  }

  async _handleKernelMessage(kernel, socket, payload) {
    let message;
    try {
      message = JSON.parse(payload);
    } catch {
      return;
    }
    const msgType = message?.header?.msg_type;
    if (msgType === 'kernel_info_request') {
      const runtime = this._runtimeLabelForKernel(kernel);
      this._wsSend(socket, this._kernelMessage('shell', 'kernel_info_reply', message, {
        status: 'ok',
        protocol_version: '5.3',
        implementation: runtime.toLowerCase(),
        implementation_version: '0',
        language_info: {
          name: 'python',
          version: '3',
          mimetype: 'text/x-python',
          file_extension: '.py',
          pygments_lexer: 'ipython3',
          codemirror_mode: { name: 'ipython', version: 3 },
        },
        banner: `Python 3 (${runtime}) on OpenHarmony`,
        help_links: [],
      }));
      return;
    }
    if (msgType === 'execute_request') {
      await this._executePython(kernel, socket, message);
      return;
    }
    if (msgType === 'shutdown_request') {
      this._wsSend(socket, this._kernelMessage('shell', 'shutdown_reply', message, {
        restart: false,
      }));
    }
  }

  async _executePython(kernel, socket, parent) {
    const code = String(parent?.content?.code || '');
    const executionCount = ++this.executionCount;
    kernel.execution_state = 'busy';
    kernel.last_activity = new Date().toISOString();
    this._wsSend(socket, this._kernelMessage('iopub', 'status', parent, { execution_state: 'busy' }));
    this._wsSend(socket, this._kernelMessage('iopub', 'execute_input', parent, {
      code,
      execution_count: executionCount,
    }));
    try {
      const execution = await this._runPython(kernel, code);
      for (const text of execution.stdout) {
        this._wsSend(socket, this._kernelMessage('iopub', 'stream', parent, { name: 'stdout', text }));
      }
      for (const text of execution.stderr) {
        this._wsSend(socket, this._kernelMessage('iopub', 'stream', parent, { name: 'stderr', text }));
      }
      if (execution.hasResult) {
        this._wsSend(socket, this._kernelMessage('iopub', 'execute_result', parent, {
          execution_count: executionCount,
          data: { 'text/plain': execution.result },
          metadata: {},
        }));
      }
      this._wsSend(socket, this._kernelMessage('shell', 'execute_reply', parent, {
        status: 'ok',
        execution_count: executionCount,
        user_expressions: {},
        payload: [],
      }));
    } catch (error) {
      const message = String(error?.message || error);
      for (const text of error?.stdout || []) {
        this._wsSend(socket, this._kernelMessage('iopub', 'stream', parent, { name: 'stdout', text }));
      }
      for (const text of error?.stderr || []) {
        this._wsSend(socket, this._kernelMessage('iopub', 'stream', parent, { name: 'stderr', text }));
      }
      this._wsSend(socket, this._kernelMessage('iopub', 'error', parent, {
        ename: error?.name || 'Error',
        evalue: message,
        traceback: [message],
      }));
      this._wsSend(socket, this._kernelMessage('shell', 'execute_reply', parent, {
        status: 'error',
        execution_count: executionCount,
        ename: error?.name || 'Error',
        evalue: message,
        traceback: [message],
      }));
    } finally {
      kernel.execution_state = 'idle';
      kernel.last_activity = new Date().toISOString();
      this._wsSend(socket, this._kernelMessage('iopub', 'status', parent, { execution_state: 'idle' }));
    }
  }

  _kernelMessage(channel, msgType, parent, content) {
    return {
      channel,
      header: {
        msg_id: crypto.randomUUID(),
        username: 'openharmony',
        session: parent?.header?.session || crypto.randomUUID(),
        date: new Date().toISOString(),
        msg_type: msgType,
        version: '5.3',
      },
      parent_header: parent?.header || {},
      metadata: {},
      content,
      buffers: [],
    };
  }

  _decodeWebSocketFrames(buffer, socket) {
    const messages = [];
    let offset = 0;
    while (offset + 2 <= buffer.length) {
      const first = buffer[offset];
      const second = buffer[offset + 1];
      const opcode = first & 0x0f;
      const masked = Boolean(second & 0x80);
      let length = second & 0x7f;
      let headerLength = 2;
      if (length === 126) {
        if (offset + 4 > buffer.length) {
          break;
        }
        length = buffer.readUInt16BE(offset + 2);
        headerLength = 4;
      } else if (length === 127) {
        if (offset + 10 > buffer.length) {
          break;
        }
        length = Number(buffer.readBigUInt64BE(offset + 2));
        headerLength = 10;
      }
      const maskLength = masked ? 4 : 0;
      const frameLength = headerLength + maskLength + length;
      if (offset + frameLength > buffer.length) {
        break;
      }
      let payload = buffer.subarray(offset + headerLength + maskLength, offset + frameLength);
      if (masked) {
        const mask = buffer.subarray(offset + headerLength, offset + headerLength + 4);
        payload = Buffer.from(payload.map((byte, index) => byte ^ mask[index % 4]));
      }
      if (opcode === 0x1) {
        messages.push(payload.toString('utf8'));
      } else if (opcode === 0x8) {
        return { messages, remaining: Buffer.alloc(0) };
      } else if (opcode === 0x9) {
        this._wsSendRaw(socket, payload, 0x0a);
      }
      offset += frameLength;
    }
    return { messages, remaining: buffer.subarray(offset) };
  }

  _wsSend(socket, value) {
    this._wsSendRaw(socket, Buffer.from(JSON.stringify(value), 'utf8'), 0x1);
  }

  _wsSendRaw(socket, payload, opcode = 0x1) {
    const length = payload.length;
    let header;
    if (length < 126) {
      header = Buffer.from([0x80 | opcode, length]);
    } else if (length < 65536) {
      header = Buffer.alloc(4);
      header[0] = 0x80 | opcode;
      header[1] = 126;
      header.writeUInt16BE(length, 2);
    } else {
      header = Buffer.alloc(10);
      header[0] = 0x80 | opcode;
      header[1] = 127;
      header.writeBigUInt64BE(BigInt(length), 2);
    }
    socket.write(Buffer.concat([header, payload]));
  }

  _kernelSpecs() {
    const kernelspecs = {};
    for (const name of this._availableKernelNames()) {
      const runtime = this._runtimeLabelForKernel({ name });
      kernelspecs[name] = {
        name,
        resources: {},
        spec: {
          argv: [],
          display_name: `Python 3 (${runtime})`,
          env: {},
          language: 'python',
          metadata: {
            debugger: false,
            implementation: runtime.toLowerCase(),
          },
        },
      };
    }
    return {
      default: 'python3',
      kernelspecs,
    };
  }

  _availableKernelNames() {
    const names = ['python3'];
    if (this._hasPyodide()) {
      names.push('python3-pyodide');
    }
    if (this._hasSkulpt()) {
      names.push('python3-skulpt');
    }
    return names;
  }

  _runtimeNameForKernel(kernel) {
    if (kernel?.name === 'python3-skulpt') {
      return 'skulpt';
    }
    if (kernel?.name === 'python3-pyodide' && this._hasPyodide()) {
      return 'pyodide';
    }
    return this._hasPyodide() ? 'pyodide' : 'skulpt';
  }

  _runtimeLabelForKernel(kernel) {
    return this._runtimeNameForKernel(kernel) === 'pyodide' ? 'Pyodide' : 'Skulpt';
  }

  _serviceUnavailable(message) {
    return {
      message,
      fallback_reason: this.fallbackReason,
      reason: 'Service Unavailable',
    };
  }

  _serveFile(res, root, relativePath) {
    const full = this._safeJoin(root, relativePath);
    if (!fs.existsSync(full) || fs.statSync(full).isDirectory()) {
      this._sendJson(res, 404, { message: `File not found: ${relativePath}` });
      return;
    }
    res.writeHead(200, {
      'Cache-Control': 'public, max-age=31536000',
      'Content-Type': this._mimeType(full),
    });
    fs.createReadStream(full).pipe(res);
  }

  _safeJoin(root, relativePath) {
    const normalized = path.normalize(relativePath || '').replace(/^(\.\.[/\\])+/, '');
    const full = path.resolve(root, normalized);
    const relative = path.relative(root, full);
    if (relative.startsWith('..') || path.isAbsolute(relative)) {
      throw new HttpError(403, `Path escapes root: ${relativePath}`);
    }
    return full;
  }

  _mimeType(file) {
    return MIME_TYPES[path.extname(file).toLowerCase()] || 'application/octet-stream';
  }

  _isLikelyBinary(file) {
    const sample = fs.readFileSync(file).subarray(0, 256);
    return sample.includes(0);
  }

  _walkFiles(dir, onFile, relative = '') {
    if (!fs.existsSync(dir)) {
      return;
    }
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = path.join(dir, entry.name);
      const rel = relative ? path.join(relative, entry.name) : entry.name;
      if (entry.isDirectory()) {
        this._walkFiles(full, onFile, rel);
      } else if (entry.isFile()) {
        onFile(full, rel);
      }
    }
  }

  _withRequestBody(req) {
    return new Promise((resolve, reject) => {
      const chunks = [];
      req.on('data', (chunk) => chunks.push(Buffer.from(chunk)));
      req.on('end', () => resolve(Buffer.concat(chunks)));
      req.on('error', reject);
    });
  }

  _parseJsonBody(body, fallback) {
    return this._parseJsonText(body?.toString('utf8') || '', fallback);
  }

  _parseJsonText(text, fallback) {
    if (!text || !text.trim()) {
      return fallback;
    }
    try {
      return JSON.parse(text);
    } catch {
      return fallback;
    }
  }

  _readJson(file, fallback) {
    try {
      return JSON.parse(fs.readFileSync(file, 'utf8'));
    } catch {
      return fallback;
    }
  }

  _sendHtml(res, html) {
    res.writeHead(200, {
      'Cache-Control': 'no-store',
      'Content-Type': 'text/html; charset=utf-8',
    });
    res.end(html);
  }

  _sendJson(res, status, data) {
    if (status === 204) {
      this._sendEmpty(res, 204);
      return;
    }
    res.writeHead(status, JSON_HEADERS);
    res.end(JSON.stringify(data ?? {}) + '\n');
  }

  _sendText(res, status, text) {
    res.writeHead(status, TEXT_HEADERS);
    res.end(String(text ?? ''));
  }

  _sendEmpty(res, status) {
    res.writeHead(status, status === 204 ? { 'Cache-Control': 'no-store' } : TEXT_HEADERS);
    res.end();
  }

  _sendMethodNotAllowed(res) {
    this._sendJson(res, 405, {
      message: 'Method not allowed',
      reason: 'Method Not Allowed',
    });
  }

  _escapeHtml(value) {
    return String(value)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }
}

class HttpError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}
