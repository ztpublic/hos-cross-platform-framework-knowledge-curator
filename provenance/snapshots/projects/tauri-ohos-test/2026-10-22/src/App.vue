<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { convertFileSrc, invoke, isTauri } from "@tauri-apps/api/core";
import { getIdentifier, getName, getTauriVersion, getVersion } from "@tauri-apps/api/app";
import { emit, listen, type UnlistenFn } from "@tauri-apps/api/event";
import {
  appDataDir,
  appCacheDir,
  appConfigDir,
  appLocalDataDir,
  basename,
  BaseDirectory,
  dirname,
  extname,
  homeDir,
  join,
  normalize,
  resolve,
  tempDir,
} from "@tauri-apps/api/path";
import { availableMonitors, currentMonitor, getAllWindows, getCurrentWindow, primaryMonitor } from "@tauri-apps/api/window";
import { getAllWebviewWindows, getCurrentWebviewWindow } from "@tauri-apps/api/webviewWindow";
import { openUrl } from "@tauri-apps/plugin-opener";
import {
  copyFile,
  exists,
  mkdir,
  readDir,
  readTextFile,
  remove,
  rename,
  stat,
  writeTextFile,
} from "@tauri-apps/plugin-fs";
import {
  AppWindow,
  BookOpen,
  CheckCircle2,
  CircleDashed,
  Database,
  FileStack,
  FolderSearch,
  Hammer,
  MonitorCog,
  RefreshCcw,
  Rocket,
  Signal,
  Sparkles,
  Waypoints,
  XCircle,
} from "@lucide/vue";

type TestState = "idle" | "running" | "pass" | "fail";

interface TestResult {
  state: TestState;
  summary: string;
  durationMs?: number;
}

interface TestCase {
  id: string;
  title: string;
  category: string;
  run: () => Promise<string>;
}

const reveal = ref(false);
const launching = ref(false);
const openingDocs = ref(false);
const logLines = ref<string[]>([]);
const testResults = ref<Record<string, TestResult>>({});

onMounted(() => {
  requestAnimationFrame(() => {
    reveal.value = true;
  });
});

const unlistenList: UnlistenFn[] = [];

onBeforeUnmount(() => {
  unlistenList.forEach((off) => off());
});

function fsProbeRootName(): string {
  return "api-probe";
}

function fsStamp(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

const tests: TestCase[] = [
  {
    id: "invoke-greet",
    title: "Invoke Rust Command",
    category: "core",
    run: async () => {
      const reply = await invoke<string>("greet", { name: "Tauri API Probe" });
      if (!reply.includes("Tauri API Probe")) {
        throw new Error("unexpected greet reply");
      }
      return reply;
    },
  },
  {
    id: "app-meta",
    title: "Read App Metadata",
    category: "app",
    run: async () => {
      const [name, version, identifier, tauriVersion] = await Promise.all([
        getName(),
        getVersion(),
        getIdentifier(),
        getTauriVersion(),
      ]);
      return `${name} ${version} (${identifier}) / tauri ${tauriVersion}`;
    },
  },
  {
    id: "core-runtime",
    title: "Runtime Environment",
    category: "core",
    run: async () => {
      const runningInTauri = isTauri();
      if (!runningInTauri) {
        throw new Error("not running inside tauri runtime");
      }
      return "isTauri=true";
    },
  },
  {
    id: "path-basic",
    title: "Read Runtime Paths",
    category: "path",
    run: async () => {
      const [appDir, home, temp] = await Promise.all([appDataDir(), homeDir(), tempDir()]);
      const reportPath = await join(appDir, "reports", "api-check.json");
      return `home=${home} | temp=${temp} | report=${reportPath}`;
    },
  },
  {
    id: "path-utils",
    title: "Path Utility Roundtrip",
    category: "path",
    run: async () => {
      const sample = await normalize("logs//runtime/../runtime/health.log");
      const resolved = await resolve("logs", "runtime", "health.log");
      const parent = await dirname(resolved);
      const file = await basename(resolved);
      const ext = await extname(resolved);

      if (!sample.includes("runtime") || file !== "health.log" || ext !== "log") {
        throw new Error("path utilities returned invalid output");
      }

      return `normalized=${sample} | parent=${parent}`;
    },
  },
  {
    id: "path-app-matrix",
    title: "App Directory Matrix",
    category: "path",
    run: async () => {
      const [config, data, localData, cache] = await Promise.all([
        appConfigDir(),
        appDataDir(),
        appLocalDataDir(),
        appCacheDir(),
      ]);

      const allValid = [config, data, localData, cache].every((path) => path.length > 0);
      if (!allValid) {
        throw new Error("empty app directory path detected");
      }

      return `config=${config} | data=${data} | localData=${localData} | cache=${cache}`;
    },
  },
  {
    id: "fs-write-read",
    title: "FS Write + Read Text",
    category: "fs",
    run: async () => {
      const dir = `${fsProbeRootName()}/${fsStamp()}`;
      const file = `${dir}/sample.txt`;
      const payload = `tauri-fs-probe-${Date.now()}`;

      await mkdir(dir, { baseDir: BaseDirectory.AppData, recursive: true });
      await writeTextFile(file, payload, { baseDir: BaseDirectory.AppData });
      const readBack = await readTextFile(file, { baseDir: BaseDirectory.AppData });
      await remove(dir, { baseDir: BaseDirectory.AppData, recursive: true });

      if (readBack !== payload) {
        throw new Error("filesystem readback mismatch");
      }

      return `readback ok (${readBack.length} chars)`;
    },
  },
  {
    id: "fs-copy-rename",
    title: "FS Copy + Rename + Stat",
    category: "fs",
    run: async () => {
      const dir = `${fsProbeRootName()}/${fsStamp()}`;
      const src = `${dir}/a.txt`;
      const copied = `${dir}/b.txt`;
      const renamed = `${dir}/c.txt`;

      await mkdir(dir, { baseDir: BaseDirectory.AppData, recursive: true });
      await writeTextFile(src, "copy-rename-probe", { baseDir: BaseDirectory.AppData });
      await copyFile(src, copied, { fromPathBaseDir: BaseDirectory.AppData, toPathBaseDir: BaseDirectory.AppData });
      await rename(copied, renamed, { oldPathBaseDir: BaseDirectory.AppData, newPathBaseDir: BaseDirectory.AppData });
      const fileInfo = await stat(renamed, { baseDir: BaseDirectory.AppData });
      await remove(dir, { baseDir: BaseDirectory.AppData, recursive: true });

      if (!fileInfo.isFile || fileInfo.size <= 0) {
        throw new Error("filesystem stat validation failed");
      }

      return `renamed file size=${fileInfo.size}B`;
    },
  },
  {
    id: "fs-list-clean",
    title: "FS List + Cleanup",
    category: "fs",
    run: async () => {
      const dir = `${fsProbeRootName()}/${fsStamp()}`;
      const file1 = `${dir}/one.txt`;
      const file2 = `${dir}/two.txt`;

      await mkdir(dir, { baseDir: BaseDirectory.AppData, recursive: true });
      await Promise.all([
        writeTextFile(file1, "one", { baseDir: BaseDirectory.AppData }),
        writeTextFile(file2, "two", { baseDir: BaseDirectory.AppData }),
      ]);

      const entries = await readDir(dir, { baseDir: BaseDirectory.AppData });
      const names = entries.map((it) => it.name).filter(Boolean) as string[];

      await remove(dir, { baseDir: BaseDirectory.AppData, recursive: true });
      const stillExists = await exists(dir, { baseDir: BaseDirectory.AppData });

      if (!names.includes("one.txt") || !names.includes("two.txt") || stillExists) {
        throw new Error("directory listing or cleanup check failed");
      }

      return `entries=${names.sort().join(",")}`;
    },
  },
  {
    id: "event-loop",
    title: "Emit And Listen Event",
    category: "event",
    run: async () => {
      const eventName = `api-check-${Date.now()}-${Math.random().toString(36).slice(2)}`;
      const payload = `ok-${Date.now()}`;
      let off: UnlistenFn | undefined;

      const received = new Promise<string>((resolvePayload, rejectPayload) => {
        const timeoutId = setTimeout(() => {
          rejectPayload(new Error("event timeout"));
        }, 1500);

        void listen<string>(eventName, (event) => {
          clearTimeout(timeoutId);
          resolvePayload(event.payload);
        })
          .then((unlisten) => {
            off = unlisten;
            unlistenList.push(unlisten);
            return emit(eventName, payload);
          })
          .catch((error: unknown) => {
            clearTimeout(timeoutId);
            rejectPayload(error);
          });
      });

      const actual = await received;
      off?.();
      if (off) {
        const index = unlistenList.indexOf(off);
        if (index >= 0) {
          unlistenList.splice(index, 1);
        }
      }

      if (actual !== payload) {
        throw new Error(`payload mismatch: ${actual}`);
      }
      return `event payload received: ${actual}`;
    },
  },
  {
    id: "window-registry",
    title: "Window Registry Scan",
    category: "window",
    run: async () => {
      const appWindow = getCurrentWindow();
      const windows = await getAllWindows();
      const labels = windows.map((window) => window.label);

      if (!labels.includes(appWindow.label)) {
        throw new Error("current window missing from registry");
      }

      return `windows=${labels.join(",")}`;
    },
  },
  {
    id: "webview-registry",
    title: "Webview Registry Scan",
    category: "webview",
    run: async () => {
      const currentWebview = getCurrentWebviewWindow();
      const webviews = await getAllWebviewWindows();
      const labels = webviews.map((webview) => webview.label);

      if (!labels.includes(currentWebview.label)) {
        throw new Error("current webview missing from registry");
      }

      return `webviews=${labels.join(",")}`;
    },
  },
  {
    id: "window-info",
    title: "Read Window Metrics",
    category: "window",
    run: async () => {
      const appWindow = getCurrentWindow();
      const [scale, size, outerSize, title] = await Promise.all([
        appWindow.scaleFactor(),
        appWindow.innerSize(),
        appWindow.outerSize(),
        appWindow.title(),
      ]);
      return `${title} | in:${size.width}x${size.height} out:${outerSize.width}x${outerSize.height} @${scale}`;
    },
  },
  {
    id: "window-position",
    title: "Read Window Positions",
    category: "window",
    run: async () => {
      const appWindow = getCurrentWindow();
      const [innerPos, outerPos] = await Promise.all([appWindow.innerPosition(), appWindow.outerPosition()]);
      return `inner=(${innerPos.x},${innerPos.y}) outer=(${outerPos.x},${outerPos.y})`;
    },
  },
  {
    id: "window-monitors",
    title: "Monitor Inventory",
    category: "window",
    run: async () => {
      const [current, primary, monitors] = await Promise.all([
        currentMonitor(),
        primaryMonitor(),
        availableMonitors(),
      ]);

      if (monitors.length === 0) {
        throw new Error("no monitor info returned");
      }

      const currentName = current?.name ?? "unknown";
      const primaryName = primary?.name ?? "unknown";
      return `count=${monitors.length} current=${currentName} primary=${primaryName}`;
    },
  },
  {
    id: "core-file-url",
    title: "Convert File Src",
    category: "core",
    run: async () => {
      const demoPath = await join(await tempDir(), "api-probe", "preview.png");
      const url = convertFileSrc(demoPath);

      if (!url.includes("preview.png") || url === demoPath) {
        throw new Error("file source conversion failed");
      }

      return url;
    },
  },
  {
    id: "window-flags",
    title: "Window Flag Snapshot",
    category: "window",
    run: async () => {
      const appWindow = getCurrentWindow();
      const [fullscreen, minimized, maximized, focused, visible, decorated, resizable, maximizable, minimizable, closable, alwaysOnTop, theme] = await Promise.all([
        appWindow.isFullscreen(),
        appWindow.isMinimized(),
        appWindow.isMaximized(),
        appWindow.isFocused(),
        appWindow.isVisible(),
        appWindow.isDecorated(),
        appWindow.isResizable(),
        appWindow.isMaximizable(),
        appWindow.isMinimizable(),
        appWindow.isClosable(),
        appWindow.isAlwaysOnTop(),
        appWindow.theme(),
      ]);

      return `fullscreen=${fullscreen} minimized=${minimized} maximized=${maximized} focused=${focused} visible=${visible} decorated=${decorated} resizable=${resizable} maximizable=${maximizable} minimizable=${minimizable} closable=${closable} alwaysOnTop=${alwaysOnTop} theme=${theme ?? "null"}`;
    },
  },
  {
    id: "window-title-roundtrip",
    title: "Window Title Roundtrip",
    category: "window",
    run: async () => {
      const appWindow = getCurrentWindow();
      const original = await appWindow.title();
      const patched = `${original} - probe`;

      await appWindow.setTitle(patched);
      const observed = await appWindow.title();
      await appWindow.setTitle(original);

      if (observed !== patched) {
        throw new Error("window title roundtrip failed");
      }

      return `title set/restore ok (${original})`;
    },
  },
  {
    id: "window-self-emit",
    title: "Window Self Emit",
    category: "event",
    run: async () => {
      const appWindow = getCurrentWindow();
      const eventName = `window-self-${Date.now()}-${Math.random().toString(36).slice(2)}`;
      const payload = `bridge-${Date.now()}`;
      let off: UnlistenFn | undefined;

      const received = new Promise<string>((resolvePayload, rejectPayload) => {
        const timeoutId = setTimeout(() => {
          rejectPayload(new Error("window emit timeout"));
        }, 1500);

        void appWindow.listen<string>(eventName, (event) => {
          clearTimeout(timeoutId);
          resolvePayload(event.payload);
        })
          .then((unlisten) => {
            off = unlisten;
            unlistenList.push(unlisten);
            return appWindow.emitTo(appWindow.label, eventName, payload);
          })
          .catch((error: unknown) => {
            clearTimeout(timeoutId);
            rejectPayload(error);
          });
      });

      const actual = await received;
      off?.();
      if (off) {
        const index = unlistenList.indexOf(off);
        if (index >= 0) {
          unlistenList.splice(index, 1);
        }
      }
      if (actual !== payload) {
        throw new Error(`window payload mismatch: ${actual}`);
      }

      return `window payload received: ${actual}`;
    },
  },
  {
    id: "parallel-batch",
    title: "Parallel API Batch",
    category: "core",
    run: async () => {
      const startedAt = performance.now();
      const [version, name, appDir, temp, scale] = await Promise.all([
        getVersion(),
        getName(),
        appDataDir(),
        tempDir(),
        getCurrentWindow().scaleFactor(),
      ]);
      const elapsed = Math.round(performance.now() - startedAt);
      return `${name}@${version} | appDir=${appDir} | temp=${temp} | scale=${scale} | ${elapsed}ms`;
    },
  },
];

for (const item of tests) {
  testResults.value[item.id] = {
    state: "idle",
    summary: "Not started",
  };
}

const passCount = computed(() => Object.values(testResults.value).filter((it) => it.state === "pass").length);
const failCount = computed(() => Object.values(testResults.value).filter((it) => it.state === "fail").length);
const runningCount = computed(() => Object.values(testResults.value).filter((it) => it.state === "running").length);
const doneCount = computed(() => passCount.value + failCount.value);

function pushLog(message: string): void {
  const ts = new Date().toLocaleTimeString();
  logLines.value = [`${ts}  ${message}`, ...logLines.value].slice(0, 40);
}

async function runSingle(test: TestCase): Promise<void> {
  const startedAt = performance.now();
  testResults.value[test.id] = { state: "running", summary: "Running..." };

  try {
    const summary = await test.run();
    const durationMs = Math.round(performance.now() - startedAt);
    testResults.value[test.id] = { state: "pass", summary, durationMs };
    pushLog(`${test.title} passed in ${durationMs}ms`);
  } catch (error) {
    const durationMs = Math.round(performance.now() - startedAt);
    const summary = error instanceof Error ? error.message : String(error);
    testResults.value[test.id] = { state: "fail", summary, durationMs };
    pushLog(`${test.title} failed: ${summary}`);
  }
}

async function runAll(): Promise<void> {
  launching.value = true;
  pushLog("Start full API diagnostic");
  for (const test of tests) {
    await runSingle(test);
  }
  pushLog("Full API diagnostic finished");
  launching.value = false;
}

function fallbackOpenDocs(url: string): void {
  const popup = window.open(url, "_blank", "noopener,noreferrer");
  if (!popup) {
    window.location.assign(url);
  }
}

async function openDocs(): Promise<void> {
  const docsUrl = "https://v2.tauri.app/reference/javascript/api/";
  openingDocs.value = true;

  try {
    await openUrl(docsUrl);
    pushLog("API Docs opened by opener plugin");
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    pushLog(`API Docs opener failed: ${message}; fallback to web open`);
    fallbackOpenDocs(docsUrl);
  } finally {
    openingDocs.value = false;
  }
}

function resultBadge(state: TestState): string {
  if (state === "pass") return "text-emerald-700 bg-emerald-50 ring-emerald-200";
  if (state === "fail") return "text-rose-700 bg-rose-50 ring-rose-200";
  if (state === "running") return "text-amber-700 bg-amber-50 ring-amber-200";
  return "text-slate-600 bg-slate-100 ring-slate-200";
}

function stateLabel(state: TestState): string {
  if (state === "pass") return "PASS";
  if (state === "fail") return "FAIL";
  if (state === "running") return "RUNNING";
  return "IDLE";
}
</script>

<template>
  <main
    class="relative min-h-screen overflow-hidden bg-[radial-gradient(circle_at_top_right,_#fef3c7_0%,_#fffbeb_30%,_#eef2ff_65%,_#e0f2fe_100%)] px-4 py-6 sm:px-8 sm:py-10"
  >
    <div class="pointer-events-none absolute -left-10 top-8 h-44 w-44 rounded-full bg-cyan-200/40 blur-3xl"></div>
    <div class="pointer-events-none absolute -right-16 top-20 h-56 w-56 rounded-full bg-amber-200/50 blur-3xl"></div>

    <section
      class="relative mx-auto flex w-full max-w-6xl flex-col gap-4 rounded-3xl border border-slate-200/70 bg-white/85 p-4 shadow-[0_30px_100px_-40px_rgba(15,23,42,0.35)] backdrop-blur md:p-6"
      :class="reveal ? 'translate-y-0 opacity-100' : 'translate-y-3 opacity-0'"
    >
      <header class="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 rounded-full bg-slate-900 px-3 py-1 text-xs font-semibold tracking-wide text-white">
            <Rocket class="h-3.5 w-3.5" />
            Tauri OHOS Frontend Probe
          </div>
          <h1
            class="text-2xl font-bold leading-tight text-slate-900 sm:text-3xl [font-family:'Space_Grotesk','Noto_Sans_SC',sans-serif]"
          >
            API Test Board
          </h1>
          <p class="text-sm text-slate-600">
            Extended board for core, path, filesystem, event, and window capability verification.
          </p>
        </div>

        <div class="flex flex-wrap items-center gap-2">
          <button
            class="inline-flex items-center gap-2 rounded-xl border border-slate-300 bg-white px-3 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-400 hover:bg-slate-50 disabled:opacity-60"
            type="button"
            :disabled="launching || openingDocs"
            @click="openDocs"
          >
            <BookOpen class="h-4 w-4" :class="openingDocs ? 'animate-pulse' : ''" />
            {{ openingDocs ? "Opening Docs" : "API Docs" }}
          </button>
          <button
            class="inline-flex items-center gap-2 rounded-xl bg-slate-900 px-3 py-2 text-sm font-medium text-white transition hover:bg-slate-700 disabled:cursor-not-allowed disabled:bg-slate-500"
            type="button"
            :disabled="launching"
            @click="runAll"
          >
            <RefreshCcw class="h-4 w-4" :class="launching ? 'animate-spin' : ''" />
            {{ launching ? "Running" : "Run All" }}
          </button>
        </div>
      </header>

      <section class="grid grid-cols-2 gap-3 md:grid-cols-5">
        <article class="rounded-2xl border border-slate-200 bg-slate-50 p-3">
          <p class="text-xs uppercase tracking-wide text-slate-500">Total</p>
          <p class="mt-1 text-2xl font-bold text-slate-900">{{ tests.length }}</p>
        </article>
        <article class="rounded-2xl border border-emerald-200 bg-emerald-50 p-3">
          <p class="text-xs uppercase tracking-wide text-emerald-700">Pass</p>
          <p class="mt-1 text-2xl font-bold text-emerald-700">{{ passCount }}</p>
        </article>
        <article class="rounded-2xl border border-rose-200 bg-rose-50 p-3">
          <p class="text-xs uppercase tracking-wide text-rose-700">Fail</p>
          <p class="mt-1 text-2xl font-bold text-rose-700">{{ failCount }}</p>
        </article>
        <article class="rounded-2xl border border-amber-200 bg-amber-50 p-3">
          <p class="text-xs uppercase tracking-wide text-amber-700">Running</p>
          <p class="mt-1 text-2xl font-bold text-amber-700">{{ runningCount }}</p>
        </article>
        <article class="rounded-2xl border border-sky-200 bg-sky-50 p-3">
          <p class="text-xs uppercase tracking-wide text-sky-700">Done</p>
          <p class="mt-1 text-2xl font-bold text-sky-700">{{ doneCount }}</p>
        </article>
      </section>

      <section class="grid gap-3 lg:grid-cols-[1.45fr_1fr]">
        <div class="space-y-3">
          <article
            v-for="test in tests"
            :key="test.id"
            class="rounded-2xl border border-slate-200 bg-white p-4 transition hover:-translate-y-0.5 hover:shadow-md"
          >
            <div class="flex items-start justify-between gap-3">
              <div>
                <p class="text-xs uppercase tracking-wide text-slate-500">{{ test.category }}</p>
                <h2 class="mt-1 text-base font-semibold text-slate-900">{{ test.title }}</h2>
              </div>
              <span
                class="inline-flex items-center gap-1 rounded-full px-2 py-1 text-xs font-semibold ring-1"
                :class="resultBadge(testResults[test.id]?.state ?? 'idle')"
              >
                <CircleDashed v-if="testResults[test.id]?.state === 'running'" class="h-3.5 w-3.5 animate-spin" />
                <CheckCircle2 v-else-if="testResults[test.id]?.state === 'pass'" class="h-3.5 w-3.5" />
                <XCircle v-else-if="testResults[test.id]?.state === 'fail'" class="h-3.5 w-3.5" />
                <Signal v-else class="h-3.5 w-3.5" />
                {{ stateLabel(testResults[test.id]?.state ?? 'idle') }}
              </span>
            </div>

            <p class="mt-2 min-h-10 break-all text-sm text-slate-600">
              {{ testResults[test.id]?.summary }}
            </p>

            <div class="mt-3 flex items-center justify-between">
              <p class="text-xs text-slate-500">
                {{ testResults[test.id]?.durationMs ? `${testResults[test.id]?.durationMs} ms` : "-" }}
              </p>
              <button
                type="button"
                class="inline-flex items-center gap-1 rounded-lg border border-slate-300 px-2.5 py-1.5 text-xs font-medium text-slate-700 transition hover:border-slate-400 hover:bg-slate-50 disabled:opacity-60"
                :disabled="launching"
                @click="runSingle(test)"
              >
                <Hammer class="h-3.5 w-3.5" />
                Run
              </button>
            </div>
          </article>
        </div>

        <aside class="space-y-3">
          <article class="rounded-2xl border border-slate-200 bg-white p-4">
            <h3 class="flex items-center gap-2 text-sm font-semibold text-slate-800">
              <MonitorCog class="h-4 w-4" />
              Coverage Scope
            </h3>
            <ul class="mt-3 space-y-2 text-sm text-slate-600">
              <li class="flex items-start gap-2">
                <Waypoints class="mt-0.5 h-4 w-4 text-sky-600" />
                Path APIs validate runtime directories, app directory matrix and path composition semantics.
              </li>
              <li class="flex items-start gap-2">
                <Database class="mt-0.5 h-4 w-4 text-emerald-600" />
                FS APIs verify write, read, copy, rename, list and cleanup flows.
              </li>
              <li class="flex items-start gap-2">
                <AppWindow class="mt-0.5 h-4 w-4 text-amber-600" />
                Window APIs validate registry, metrics, positions, monitor inventory and title roundtrip behaviors.
              </li>
              <li class="flex items-start gap-2">
                <Sparkles class="mt-0.5 h-4 w-4 text-violet-600" />
                Runtime detection, webview registry, file source conversion and window event bridging verify concurrent workloads.
              </li>
            </ul>
          </article>

          <article class="rounded-2xl border border-slate-200 bg-white p-4">
            <h3 class="flex items-center gap-2 text-sm font-semibold text-slate-800">
              <FolderSearch class="h-4 w-4" />
              API Mix
            </h3>
            <div class="mt-3 grid grid-cols-2 gap-2 text-xs">
              <p class="inline-flex items-center gap-1 rounded-lg bg-slate-100 px-2 py-1 text-slate-700">
                <FileStack class="h-3.5 w-3.5" />
                Core + App
              </p>
              <p class="inline-flex items-center gap-1 rounded-lg bg-sky-100 px-2 py-1 text-sky-700">
                <Waypoints class="h-3.5 w-3.5" />
                Path
              </p>
              <p class="inline-flex items-center gap-1 rounded-lg bg-emerald-100 px-2 py-1 text-emerald-700">
                <Database class="h-3.5 w-3.5" />
                FS Plugin
              </p>
              <p class="inline-flex items-center gap-1 rounded-lg bg-amber-100 px-2 py-1 text-amber-700">
                <AppWindow class="h-3.5 w-3.5" />
                Window
              </p>
            </div>
          </article>

          <article class="rounded-2xl border border-slate-200 bg-slate-900 p-4 text-slate-100">
            <h3 class="text-sm font-semibold">Execution Log</h3>
            <div class="mt-3 max-h-72 space-y-2 overflow-auto pr-1">
              <p v-if="logLines.length === 0" class="text-xs text-slate-300">No logs yet.</p>
              <p v-for="line in logLines" :key="line" class="text-xs leading-5 text-slate-300">
                {{ line }}
              </p>
            </div>
          </article>
        </aside>
      </section>
    </section>
  </main>
</template>
