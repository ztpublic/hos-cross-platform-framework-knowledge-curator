/**
 * Eazytec is pleased to support the open source community by making CPF-KMP-CMP available.
 * Copyright (C) 2026 Eazytec Co., Ltd. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.cpf.kmp.tools.ohos.debugger
import com.intellij.execution.ui.ConsoleViewContentType
import com.intellij.execution.ui.RunnerLayoutUi
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.roots.ProjectRootManager
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.openapi.diagnostic.Logger
import com.intellij.ui.content.Content
import com.intellij.util.Alarm
import com.intellij.openapi.extensions.ExtensionPointName
import com.intellij.icons.AllIcons
import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.AnAction
import com.intellij.openapi.actionSystem.AnActionEvent
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.actionSystem.ToggleAction
import com.intellij.openapi.actionSystem.Toggleable
import com.intellij.xdebugger.XDebugProcess
import com.intellij.xdebugger.XDebugSession
import com.intellij.xdebugger.XDebuggerManager
import com.intellij.xdebugger.XSourcePosition
import com.intellij.xdebugger.breakpoints.XBreakpointHandler
import com.intellij.xdebugger.breakpoints.XBreakpointListener
import com.intellij.xdebugger.breakpoints.XBreakpointProperties
import com.intellij.xdebugger.breakpoints.XBreakpointType
import com.intellij.xdebugger.breakpoints.XLineBreakpoint
import org.jetbrains.kotlin.idea.debugger.breakpoints.KotlinLineBreakpointType
import com.intellij.xdebugger.evaluation.XDebuggerEditorsProvider
import com.intellij.xdebugger.frame.XSuspendContext
import com.intellij.xdebugger.ui.XDebugTabLayouter
import org.cpf.kmp.tools.ohos.debugger.debugui.LldbVariablesScrollPrefetchSessionListener
import org.cpf.kmp.tools.ohos.debugger.disassembly.DisassemblyEditorKeys
import org.cpf.kmp.tools.ohos.debugger.disassembly.DisassemblyFileType
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsBreakpointHandler
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsConnectServerClient
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsDebugBackend
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsLineBreakpointType
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsMultiInstanceManager
import org.cpf.kmp.tools.ohos.debugger.arkts.ArkTsPositionMapper
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLanguageDebugCoordinator
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLanguageBackend
import org.cpf.kmp.tools.ohos.debugger.stepinto.StepIntoBoundaryRouter
import org.cpf.kmp.tools.ohos.debugger.stepinto.StepIntoContext
import org.cpf.kmp.tools.ohos.debugger.stepinto.StepIntoTarget
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLangBlocker
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLangBlockerCode
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLangJumpTiming
import org.cpf.kmp.tools.ohos.debugger.arkts.CrossLangTrace
import org.cpf.kmp.tools.ohos.device.HarmonyDevice
import org.cpf.kmp.tools.ohos.log.hilog.parser.HiLogParser
import org.cpf.kmp.tools.ohos.sync.HarmonyLocalPropertiesSupport
import org.cpf.kmp.tools.ohos.sync.OhosProjectPath
import org.cpf.kmp.tools.ohos.sync.DevEcoPathService
import java.lang.reflect.Method
import java.nio.file.Files
import java.nio.file.Paths
import java.nio.file.StandardOpenOption
import java.util.concurrent.CompletableFuture
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

/**
 * LLDB debug process (JNI).
 *
 * Calls the C++ LLDB API via JNI instead of the previous CLI approach.
 * Main differences:
 * 1. JNI handles instead of command strings
 * 2. Platform setup via JNI APIs directly
 * 3. Events via waitForEvent instead of parsing stdout
 */
class LLDBDebugProcess(
    session: XDebugSession,
    private val executablePath: String,
    private val environmentVariables: Map<String, String> = emptyMap(),
    /** LLDB interpreter lines from Harmony run configuration (after attach, before breakpoint sync). */
    internal val lldbStartupCommands: List<String> = emptyList(),
) : XDebugProcess(session) {

    companion object {
        private val LOG = Logger.getInstance(LLDBDebugProcess::class.java)
        /** Virtual thread id for ArkTS CDP pause (LLDB inferior may still be RUNNING). */
        const val ARK_TS_VIRTUAL_THREAD_ID = 1

        private val LIGHT_VIRTUAL_FILE_CLASS: Class<*>? = run {
            try {
                Class.forName("com.intellij.openapi.vfs.LightVirtualFile")
            } catch (_: ClassNotFoundException) {
                null
            }
        }

        /** IntelliJ `com.intellij.xdebugger.breakpointType` extension point. */
        private val XDEBUG_BREAKPOINT_TYPE_EP =
            ExtensionPointName.create<XBreakpointType<*, *>>("com.intellij.xdebugger.breakpointType")

        /** Heuristic: min non-runtime frames before recursion checks apply. */
        private const val RECURSION_MIN_NON_RUNTIME_FRAMES = 14

        /**
         * Consecutive identical user frames (same name + file + line, with valid source info) after stripping
         * Kotlin runtime glue. Only this pattern indicates real runaway recursion; counting repeats by
         * **function name alone** was removed — deep stacks often reuse generic names (`invoke`, lambdas)
         * without recursion, which falsely tripped the old heuristic on slow startup / framework code.
         */
        private const val RECURSION_CONSECUTIVE_IDENTICAL_FRAMES = 32

        /**
         * Quiet-continues while waiting for the one-shot next line after `delay`.
         * Other-thread `probeCoroutineCreated` is not counted (see [bumpResumeSkipCount]).
         */
        private const val MAX_COROUTINE_RESUME_SKIPS = 32


        private const val PLEASE_WAIT_DOT_INTERVAL_MS = 400L

        /** Pad `\r` line so shorter frames clear leftover chars (dots + ` (Ns)`). */
        private const val PLEASE_WAIT_LINE_PAD = 48

        /** Fixed-width slot for animated dots so ` (Ns)` does not shift. */
        private const val PLEASE_WAIT_DOTS_FIELD_WIDTH = 3

        /** Default thread count in debug console listing before "Show All Threads" toggle. */
        const val THREAD_LIST_PREVIEW_COUNT = 3

        /** Max wait for background source-jar map warmup before continuing attach. */
        private const val SOURCE_MAP_WARMUP_WAIT_MS = 500L

        /**
         * 2026.1+ adds [XDebugSession.positionReached] `(context, attract)` for split debugger / toolbar sync.
         * `pluginSinceBuild` 231 classpath only has the single-arg overload — resolve the two-arg method at runtime when present.
         */
        private val positionReachedAttractMethod: Method? = run {
            try {
                XDebugSession::class.java.getMethod(
                    "positionReached",
                    XSuspendContext::class.java,
                    Boolean::class.javaPrimitiveType,
                )
            } catch (_: Throwable) {
                null
            }
        }
    }

    private val serviceWrapper = LLDBServiceWrapper(this)
    private val listener = LLDBListener(this, serviceWrapper)

    /** Kotlin coroutine debugging (SR005); collection is opt-in from the Frames toolbar. */
    private val coroutineSupport =
        org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSessionSupport(this, serviceWrapper)
    private val breakpointHandler = LLDBBreakpointHandler(this)
    private val kotlinBreakpointHandler = LLDBKotlinBreakpointHandler(this, breakpointHandler)
    private val arkTsBreakpointHandler = ArkTsBreakpointHandler(this)

    /** IDE breakpoint edits for Kotlin line breakpoints (sync conditions / re-enable to LLDB). */
    private val lineBreakpointIdeListener = object : XBreakpointListener<XLineBreakpoint<XBreakpointProperties<*>>> {
        override fun breakpointChanged(breakpoint: XLineBreakpoint<XBreakpointProperties<*>>) {
            breakpointHandler.onIdeBreakpointChanged(breakpoint)
            syncArkTsBreakpointIfNeeded(breakpoint)
        }
    }

    /** Sync `.ets` / `.ts` breakpoints to ArkTS CDP (all IDE line breakpoint types). */
    private val arkTsLineBreakpointIdeListener = object : XBreakpointListener<XLineBreakpoint<XBreakpointProperties<*>>> {
        override fun breakpointAdded(breakpoint: XLineBreakpoint<XBreakpointProperties<*>>) {
            syncArkTsBreakpointIfNeeded(breakpoint)
        }

        override fun breakpointRemoved(breakpoint: XLineBreakpoint<XBreakpointProperties<*>>) {
            if (!OhosKmpCrossLanguageDebug.isEnabled()) return
            val backend = arkTsDebugBackend ?: return
            val path = breakpoint.sourcePosition?.file?.path ?: return
            if (!ArkTsPositionMapper.isArkTsSourcePath(path)) return
            backend.unregisterBreakpoint(path, breakpoint.line)
        }

        override fun breakpointChanged(breakpoint: XLineBreakpoint<XBreakpointProperties<*>>) {
            syncArkTsBreakpointIfNeeded(breakpoint)
        }
    }

    private val positionMapper = LLDBPositionMapper(session.project)
    private val executionGutterHighlighter = LLDBExecutionGutterHighlighter(session)
    private val variablesScrollPrefetchListener = LldbVariablesScrollPrefetchSessionListener(session)

    private val suspendedThreads = ConcurrentHashMap<Int, Boolean>()
    @Volatile private var firstStopNotified = false
    @Volatile private var stopped = false
    @Volatile private var attachEventLoopStarted = false

    /** Cross-language: [onReady] / TTDR fires once after LLDB continue + event loop (CDP continues async). */
    private val crossLangLldbStartupReadyOnce = AtomicBoolean(false)

    @Volatile
    private var crossLangCdpProtocolReady: Boolean = false

    private val crossLangCdpFailureReportedOnce = AtomicBoolean(false)

    @Volatile
    private var crossLangPrewarmFuture: CompletableFuture<CrossLangBridgePrewarmSnapshot>? = null

    @Volatile
    private var connectServerArmedDuringPrewarm: Boolean = false

    private val attachPreambleLock = Any()

    @Volatile
    private var attachPreambleScheduled: Boolean = false

    @Volatile
    private var attachPreambleFinished: Boolean = false

    @Volatile
    private var pendingAttachProcessId: String? = null

    @Volatile
    private var platformAttachChainStarted: Boolean = false

    /** Background source-map warmup started after ProcessLibs mirror; consumed before breakpoint sync. */
    @Volatile private var sourceJarMapWarmup: CompletableFuture<SourceMapPrepareResult>? = null
    private val sourceMapInjectLock = Any()
    @Volatile private var injectedSourceMapRules: List<SourceMapRule> = emptyList()
    private val stepIntoBoundaryRouter: StepIntoBoundaryRouter = StepIntoBoundaryRouter.default()

    /** One-shot breakpoint while [stepIntoBoundaryPending] (cross-module Step Into via target BP + continue). */
    @Volatile private var stepIntoBoundaryBreakpointId: Int? = null
    @Volatile private var stepIntoBoundaryPending: Boolean = false

    /**
     * User Continue/Step: one JNI command at a time until the next LLDB stop (matches queued VM RPCs in Flutter).
     * Prevents duplicate continue/step while the first call is still on the EDT or before [handleStopped] runs.
     */
    private val userContinueOrStepLock = Any()
    private var userContinueOrStepPending = false

    private var webSocketClient: WebSocketDebugClient? = null
    private var connectServerClient: ArkTsConnectServerClient? = null
    private var arkTsDebugBackend: ArkTsDebugBackend? = null
    private var crossLanguageCoordinator: CrossLanguageDebugCoordinator? = null
    private var debugModeContext: DebugModeContext? = null

    /** `hit <module> <ms>` or `miss <reason> <ms>` from the attach-time one-shot breakpoint. */
    @Volatile internal var softwareBreakpointWarmupSummary: String = "not-run"

    /**
     * Debug (`aa process -D`) loads libentry.so only after the launch continue.
     * A one-shot on its constructor pays the first software-breakpoint stop in that
     * mapping during startup. The event loop resumes past it without showing a pause.
     */
    @Volatile private var libentryLoadWarmupPending: Boolean = false
    @Volatile private var libentryLoadWarmupBreakpointId: Int = 0

    /** [System.nanoTime] of the continue that releases the process after attach. 0 = not yet. */
    @Volatile private var launchContinueNs: Long = 0L
    @Volatile private var launchContinueStopLogged: Boolean = false
    private var multiInstanceManager: ArkTsMultiInstanceManager? = null

    /** Set when [handoffArkTsToNative] is waiting for LLDB to stop at the native entry address. */
    @Volatile
    private var nativeLldbHandoffInFlight: Boolean = false

    /** One-shot LLDB BP on native symbol while CDP stepInto runs (fallback when nativeCalling CDP event missing). */
    @Volatile
    private var nativeCatchBreakpointId: Int? = null
    private val nativeCatchExtraBreakpointIds = mutableListOf<Int>()

    @Volatile
    private var nativeCatchHandoffPending: Boolean = false

    /** Parsed libentry/libkn entry VA from armed symbol BP list (parallel runToAddress during CDP stepInto). */
    @Volatile
    private var nativeCatchEntryAddress: Long = 0L

    /** Invalidates in-flight [prepareNativeCatchBeforeStepIntoWithTimeout] when a newer arm or timeout occurs. */
    private val nativeCatchArmGeneration = AtomicInteger(0)

    /** Last native-catch arm summary for [CrossLangBlockerCode.NATIVE_ENTRY_SKIPPED] diagnostics. */
    @Volatile
    private var lastNativeCatchDiagSummary: String = ""

    internal fun getLastNativeCatchDiagSummary(): String = lastNativeCatchDiagSummary

    private var lldbTerminalView: LLDBTerminalView? = null
    private var lldbTerminalContent: Content? = null
    private var coroutinesDebugPanel: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel? = null
    private var coroutinesDebugContent: Content? = null
    /** Layout handed to [createTabLayouter]; do not re-read it via [XDebugSession.getUI] in split mode. */
    private var coroutinesLayoutUi: RunnerLayoutUi? = null
    @Volatile private var coroutinesDebugTabRegistered = false
    private var coroutinesTabSelectionListenerAttached = false
    private var lldbDisassemblyView: LLDBDisassemblyView? = null
    /** Editor tab key; prefer in-memory file so the tab shows no project path (window mapping only). */
    private var disassemblyEditorFile: VirtualFile? = null
    /** User explicitly selected the Disassembly tab — enables instruction-level Step Over/Into/Out. */
    @Volatile private var disassemblyEditorUserPinned = false
    /**
     * After a stop on glue (no mappable Kotlin source), F8/F7 use instruction-level stepping even if the
     * Disassembly tab was opened programmatically (toolbar focus may still be on a `.kt` editor).
     */
    @Volatile private var glueDisassemblyInstructionStepping = false
    /** True while [focusDisassemblyEditor] is opening the tab programmatically (not user click). */
    @Volatile private var disassemblyEditorProgrammaticFocus = false
    /** Bumps on each stop/dismiss so stale [focusDisassemblyEditor] EDT tasks no-op. */
    @Volatile private var disassemblyFocusGeneration = 0

    /** Console line `Please wait` + fixed-width dot animation (`.  ` / `.. ` / `...`) until attach completes or session stops. */
    private val pleaseWaitAlarm = Alarm(Alarm.ThreadToUse.SWING_THREAD, session.project)
    private val lldbTabRegisterAlarm = Alarm(Alarm.ThreadToUse.SWING_THREAD, session.project)
    @Volatile private var pleaseWaitAnimationActive = false
    private var pleaseWaitDotPhase = 0
    @Volatile private var lldbTerminalTabRegistered = false
    private var lldbTabRegisterRetryCount = 0

    /**
     * HarmonyOS Kotlin/Native [println] goes to HiLog, not inferior stdio — LLDB [GetSTDOUT] stays empty.
     * Stream `hdc shell hilog -P <pid>` into the Debug Console while this session is active.
     */
    @Volatile private var debugHilogMirrorProcess: Process? = null
    private var debugHilogMirrorThread: Thread? = null
    private var pleaseWaitStartMillis = 0L

    private val pleaseWaitAnimator = object : Runnable {
        override fun run() {
            if (!pleaseWaitAnimationActive || stopped || session.project.isDisposed) return
            val cv = session.consoleView ?: run {
                stopPleaseWaitConsoleAnimation()
                return
            }
            val dotsField = when (pleaseWaitDotPhase++ % 3) {
                0 -> ".  "
                1 -> ".. "
                else -> "..."
            }
            val elapsedSec =
                ((System.currentTimeMillis() - pleaseWaitStartMillis) / 1000L).toInt().coerceAtLeast(0)
            val line = "\rPlease wait$dotsField (${elapsedSec}s)"
            try {
                cv.print(line.padEnd(PLEASE_WAIT_LINE_PAD), ConsoleViewContentType.SYSTEM_OUTPUT)
            } catch (_: Throwable) {
            }
            if (pleaseWaitAnimationActive && !stopped && !session.project.isDisposed) {
                pleaseWaitAlarm.addRequest(this, PLEASE_WAIT_DOT_INTERVAL_MS)
            }
        }
    }

    /** Stack from the last suspend; used for one [XDebugSession.positionReached] refresh when toggling hide-library-frames. */
    @Volatile private var lastSuspendedThreadId: Int? = null
    @Volatile private var lastSuspendedStackFrames: List<StackFrame>? = null
    /** Matches the last [LLDBSuspendContext]; used by [requestFramesListRefresh] to keep the top-frame IDE source fallback. */
    @Volatile private var lastTopFrameIdeSourceOverride: XSourcePosition? = null
    /** Last stop where stack top mapped to Kotlin/project source; anchor when stepping glue in Disassembly. */
    @Volatile private var lastKnownMappableIdeSource: XSourcePosition? = null

    /** After F7 Step Into, skip callee `suspend fun … {` header line (Quail / Android Studio). */
    @Volatile private var skipCalleeHeaderOnNextStepInto: Boolean = false
    /** Native LLDB step-in in flight (e.g. into `delay`); glue stops must not quiet-continue away. */
    @Volatile private var pendingNativeStepInto: Boolean = false
    @Volatile private var stepIntoOriginLine1Based: Int = 0
    @Volatile private var stepIntoOriginFile: String = ""

    /** Step-outs that landed in `[K][Suspend]Functions`. Capped so we do not loop. */
    @Volatile private var suspendCallingConventionHops: Int = 0
    /** True while a coroutine resume breakpoint is armed; the next `step` stop is the suspend-return and is auto-continued. */
    @Volatile private var pendingCoroutineResume = false
    @Volatile private var coroutineResumeAutoContinueCount = 0
    /** 1-based line the one-shot resume breakpoint is waiting for; 0 when unknown. */
    @Volatile private var pendingCoroutineResumeLine: Int = 0
    /** Step Out: suspend **call** line; [pendingCoroutineResumeLine] is the caret (usually next statement). */
    @Volatile private var pendingCoroutineResumeCallSiteLine: Int = 0
    @Volatile private var pendingCoroutineResumeFile: String = ""
    /** 1-based line we stepped away from (`delay` at 303/305); must not count as "arrived". */
    @Volatile private var pendingCoroutineResumeOriginLine: Int = 0
    /** Step Over left a suspend call (`delay`); resume may skip the armed next line and land on `}`. */
    @Volatile private var pendingCoroutineResumeOriginWasSuspendCall: Boolean = false
    /** Survives [clearPendingCoroutineResume] — pins UI on println when resume overshoots to `}`. */
    @Volatile private var stepOverSuspendCallPinLine: Int = 0
    @Volatile private var stepOverSuspendCallPinFile: String = ""
    @Volatile private var stepOverSuspendCallPinOriginSymbol: String = ""
    /** Continuation captured on the suspend-call line before resume (frame 0 is still invokeSuspend). */
    @Volatile private var stepOverSuspendCallSpilledContinuationAddress: Long = 0L
    /** Pin landed on `#suspend` epilogue with empty DWARF — fill Variables from `--current-spilled`. */
    @Volatile private var stepOverSuspendCallSpilledLocals: Boolean = false
    /** Thread that armed the one-shot; other-thread `probeCoroutineCreated` must not burn the skip budget. */
    @Volatile private var pendingCoroutineResumeThreadId: Int = 0
    /** Last-statement Step Over: wait for the parent SM, not a concrete next line. */
    @Volatile private var pendingCoroutineResumeLastStatement: Boolean = false
    /** Shift+F8 Step Out: surface the caller state machine, not last-statement epilogue hops. */
    @Volatile private var pendingCoroutineStepOut: Boolean = false
    /** User BPs below a suspend call line were disabled for native Step Out — restore on next pause. */
    @Volatile private var restoreBreakpointsAfterNativeCallSiteStepOut: Boolean = false
    /** Step Out back to `launch { suspendCall() }` — epilogue Step Over then Continue to machinery line. */
    @Volatile private var launchBlockStepOutContinueChase: Boolean = false
    /**
     * Step Out to an outer suspend call (167) from inside the callee body (172+): caller line BPs
     * alone cannot unwind — must native Step Out first, then chase pending caller breakpoints.
     */
    @Volatile private var plainCallerStepOutNativeUnwind: Boolean = false
    @Volatile private var pendingLaunchBlockStepOut: Boolean = false
    /** Executable line after the suspend call (e.g. 394 `}`); caret stays on [pendingCoroutineResumeLine]. */
    @Volatile private var pendingLaunchBlockMachineryLine: Int = 0
    /** Set when [trySurfaceLaunchBlockStepOutFromStack] synthesizes a user caret; cleared after UI stop. */
    @Volatile private var launchBlockSyntheticCaretPendingUi: Boolean = false
    /** `invokeSuspend` we stepped away from; parent resume is a different symbol. */
    @Volatile private var pendingCoroutineResumeOriginSymbol: String = ""
    /**
     * DWARF line of the caller state-machine PC armed for `return` in a plain callee.
     * Often the `start()` call (140) while the caret should move to `await()` (141).
     */
    @Volatile private var pendingCallerStopLine: Int = 0
    /** Step Over of `return part` is waiting for that caller PC, not a file:line breakpoint. */
    @Volatile private var pendingCallerResumeArmed: Boolean = false
    /** One hop from parent `label` (line 0) to the next DWARF user line in that function. */
    @Volatile private var pendingParentPrologueHopDone: Boolean = false
    /**
     * Last user Step Over/Into used a native LLDB stepper. Hitting 305 during that step leaves
     * the plan on the thread; the next Continue single-steps libc/Compose unless we discard it.
     */
    @Volatile private var leftoverNativeStepPlanLikely: Boolean = false
    /** True between a pending-resume Continue and the next STOPPED; avoids nested Continue + UI ping-pong. */
    @Volatile private var pendingResumeContinueInFlight = false
    /**
     * After the one-shot resume line is shown (C05 306), park/Compose stops must not steal the
     * editor cursor or Variables. Cleared on the next user Step/Continue/Pause.
     */
    @Volatile private var stickyUserSourcePause = false
    /**
     * After Step Out lands on a user breakpoint at a suspend *call* line (e.g. demo 167), Step Over
     * may coroutine-arm past the call so the launch block finishes and the UI thread can run again.
     * While false (first hit at the call-site BP), Step Over stays blocked so F8 does not run away.
     */
    @Volatile private var userSuspendCallBpStepOverAllowed = false
    /** Bumped when sticky is set/cleared so a queued quiet Continue cannot run past the resume line. */
    @Volatile private var quietContinueGeneration = 0
    /** Step Out Continue issued but caller never surfaced — abandon and restore user breakpoints. */
    private val stepOutWatchdogAlarm = Alarm(Alarm.ThreadToUse.POOLED_THREAD, session.project)
    @Volatile private var lastPendingThreadScanMs: Long = 0
    /**
     * LLDB already resumed (RUNNING event or we issued Continue). A second ContinueProcess while
     * SBFrame objects are dying is the C05 SIGSEGV in `StackFrame::GetRecognizedFrame`.
     */
    @Volatile private var lldbProcessRunning = false

    /**
     * Tracks the time the inferior actually spent running (async execution) across the current
     * user step/continue. This is subtracted from the IDE Elapsed time so it reflects only the
     * debugger's own overhead rather than the program's `delay`/dispatch time.
     */
    @Volatile private var asyncRunningStartNanos: Long = 0L
    @Volatile private var accumulatedAsyncRunningNanos: Long = 0L

    internal fun markAsyncRunningStart() {
        asyncRunningStartNanos = System.nanoTime()
    }

    internal fun markAsyncRunningStop() {
        val start = asyncRunningStartNanos
        if (start != 0L) {
            accumulatedAsyncRunningNanos += System.nanoTime() - start
            asyncRunningStartNanos = 0L
        }
    }

    internal fun resetAsyncRunningAccumulator() {
        accumulatedAsyncRunningNanos = 0L
        asyncRunningStartNanos = 0L
    }

    internal fun getAsyncExecutionMs(): Long = accumulatedAsyncRunningNanos / 1_000_000L


    /**
     * Whether to hide library/external frames in the Frames list for this session (not persisted; reset to true per [LLDBDebugProcess]).
     */
    @Volatile private var hideLibraryExternalFrames: Boolean = true

    /**
     * DevEco-style: append cached ArkTS frames under native Frames when true (path A — CDP cache only).
     */
    @Volatile private var showArkTsStackFrames: Boolean = false

    internal fun isHideLibraryExternalFrames(): Boolean = hideLibraryExternalFrames

    internal fun setHideLibraryExternalFrames(value: Boolean) {
        hideLibraryExternalFrames = value
    }

    internal fun isShowArkTsStackFrames(): Boolean = showArkTsStackFrames

    internal fun setShowArkTsStackFrames(value: Boolean) {
        showArkTsStackFrames = value
    }

    /**
     * True when the current native (LLDB) stop is in Kotlin — used to hide
     * "Show ArkTS Stack Frame" (that toggle is for the ets→C/C++ NAPI boundary).
     */
    internal fun isCurrentNativeStopKotlin(): Boolean =
        isNativeStopKotlin(lastSuspendedStackFrames, lastTopFrameIdeSourceOverride)

    internal fun isNativeStopKotlin(
        stackFrames: List<StackFrame>?,
        topFrameIdeSource: XSourcePosition?,
    ): Boolean {
        topFrameIdeSource?.file?.path?.let { path ->
            if (path.replace('\\', '/').endsWith(".kt", ignoreCase = true)) return true
        }
        val topFile = stackFrames?.firstOrNull()?.file ?: return false
        return topFile.replace('\\', '/').endsWith(".kt", ignoreCase = true)
    }

    /** Debug console thread list: when false, only the first [THREAD_LIST_PREVIEW_COUNT] threads are listed. */
    @Volatile private var showAllThreadsInList: Boolean = false

    internal fun isShowAllThreadsInList(): Boolean = showAllThreadsInList

    internal fun setShowAllThreadsInList(value: Boolean) {
        showAllThreadsInList = value
    }

    /**
     * Toolbar actions that apply while paused (frame filter, thread list toggle).
     * [XDebugSession.isSuspended] is often still false after [positionReached] on this backend.
     */
    internal fun isSuspendToolbarActionsEnabled(): Boolean {
        if (stopped || !serviceWrapper.isReady()) return false
        if (session.isSuspended) return true
        if (crossLanguageCoordinator?.isArkTsSuspended() == true) return true
        return lastSuspendedThreadId != null && !lastSuspendedStackFrames.isNullOrEmpty()
    }

    // ===== Kotlin coroutine debugging (SR005) =====

    internal fun getCoroutineSupport(): org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSessionSupport =
        coroutineSupport

    /**
     * Coroutine stacks already collected for the current stop.
     *
     * Read by [LLDBSuspendContext.getExecutionStacks] on the EDT, so it must never hit the device;
     * an empty list simply means the background walk has not finished (or is switched off).
     */
    internal fun getCachedCoroutineExecutionStacks(): List<org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineExecutionStack> =
        coroutineSupport.getCachedExecutionStacks()

    /** Combined Threads & Variables often shows only this name; include the coroutine count when available. */
    internal fun threadStackDisplayName(threadId: Int): String {
        val n = coroutineSupport.getCachedExecutionStacks().size
        return if (n > 0) "Thread $threadId · $n coroutine(s)" else "Thread $threadId"
    }

    /**
     * Try to turn a step into a coroutine resume (AR003).
     *
     * @param atSuspendCall whether the current source line is a suspend call. State-machine Step
     *   Over always prefers a next-line one-shot (suspend *and* `println` / allocation).
     * @return true when a one-shot resume breakpoint was armed, meaning the caller should resume
     *   rather than step. Any failure returns false so the ordinary stepping path still runs.
     */
    private fun armCoroutineStep(
        kind: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind,
        threadId: Int,
        atSuspendCall: Boolean = false,
        nextLine: Int = 0,
        filePath: String = "",
        lastStatement: Boolean = false,
        alsoArmFollowingLine: Boolean = false,
        explicitOriginLine1Based: Int = 0,
    ): Boolean = try {
        val armed = coroutineSupport.tryArmCoroutineStep(
            kind = kind,
            threadId = threadId,
            topFrameSymbol = lastSuspendedStackFrames?.firstOrNull { frame ->
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isStateMachineFrame(frame.name) ||
                    org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isDemangledSuspendContinuationFrame(frame.name)
            }?.name ?: lastSuspendedStackFrames?.firstOrNull()?.name,
            atSuspendCall = atSuspendCall,
            nextLine = nextLine,
            filePath = filePath,
            lastStatement = lastStatement,
            alsoArmFollowingLine = alsoArmFollowingLine,
        )
        if (armed) {
            val originLine = when {
                explicitOriginLine1Based > 0 -> explicitOriginLine1Based
                else -> (lastTopFrameIdeSourceOverride?.line ?: -1) + 1
            }
            val originSymbol = lastSuspendedStackFrames?.firstOrNull()?.name.orEmpty()
            // lazyAsyncCase:142 — last editor line before `}`; nextLine=0. Chasing line 0 never
            // surfaces and leaves the inferior stopped after the click callback (app frozen).
            val plainLastStatementContinue = lastStatement && nextLine <= 0 &&
                kind == org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER
            if (plainLastStatementContinue) {
                leftoverNativeStepPlanLikely = true
                stickyUserSourcePause = false
                syncNativePendingResumeAutoContinue(enabled = false)
                coroutineStepLog(
                    "armed plain last-statement Continue — no pending chase " +
                        "(origin=$originLine file=$filePath symbol=$originSymbol)",
                )
            } else {
                pendingCoroutineResume = true
                coroutineResumeAutoContinueCount = 0
                pendingCoroutineResumeLine = nextLine
                pendingCoroutineResumeFile = filePath
                pendingCoroutineResumeOriginLine = originLine
                pendingCoroutineResumeOriginWasSuspendCall =
                    kind == org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER && atSuspendCall
                pendingCoroutineResumeThreadId = threadId
                pendingCoroutineResumeLastStatement =
                    lastStatement && kind == org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER
                pendingCoroutineStepOut = kind == org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OUT
                if (pendingCoroutineStepOut) {
                    resolveStepOutCallerTarget(threadId, filePath, originLine, originSymbol)?.let {
                            (callerFile, callSiteLine) ->
                        assignStepOutCallerPending(callerFile, callSiteLine)
                    }
                    if (pendingCoroutineResumeLine <= 0) {
                        coroutineStepPipeline(
                            "step-out WARN caller line unresolved (fn=" +
                                "${org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.suspendBodyFunctionNameFromSymbol(originSymbol)}) " +
                                "origin=$originLine — chase may fail",
                        )
                    }
                    armStepOutCallerEditorBreakpoints()
                }
                pendingCoroutineResumeOriginSymbol = originSymbol
                pendingParentPrologueHopDone = false
                pendingCallerStopLine = 0
                pendingCallerResumeArmed = false
                leftoverNativeStepPlanLikely = pendingCoroutineStepOut
                stickyUserSourcePause = false
                syncNativePendingResumeAutoContinue(enabled = true)
                if (kind == org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER &&
                    atSuspendCall && nextLine > 0 && filePath.isNotEmpty()
                ) {
                    stepOverSuspendCallPinLine = nextLine
                    stepOverSuspendCallPinFile = filePath
                    stepOverSuspendCallPinOriginSymbol = originSymbol
                    stepOverSuspendCallSpilledContinuationAddress =
                        coroutineSupport.readCurrentContinuationAddress()
                    coroutineStepPipeline(
                        "step-over suspend-call pin armed $filePath:$nextLine (origin=$originLine) " +
                            "continuation=0x${stepOverSuspendCallSpilledContinuationAddress.toString(16)}",
                    )
                }
                coroutineStepLog(
                    "armed kind=$kind originLine=$originLine nextLine=$nextLine file=$filePath " +
                        "atSuspendCall=$atSuspendCall lastStatement=$lastStatement " +
                        "symbol=$originSymbol",
                )
            }
        }
        armed
    } catch (t: Throwable) {
        LOG.warn("coroutine step planning failed; using native step", t)
        false
    }

    /**
     * Text of the source line the debugger is currently stopped on, or null when the top frame has
     * no IDE source. Used to decide whether a coroutine step-over sits on a suspend call.
     */
    private fun currentTopFrameSourceLineText(): String? {
        val sourcePosition = lastTopFrameIdeSourceOverride ?: return null
        val file = sourcePosition.file
        if (!file.isValid) return null
        val lineIndex = sourcePosition.line // IntelliJ XSourcePosition is 0-based.
        return try {
            com.intellij.openapi.application.ReadAction.compute<String?, RuntimeException> {
                val document =
                    com.intellij.openapi.fileEditor.FileDocumentManager.getInstance().getDocument(file)
                if (document == null || lineIndex < 0 || lineIndex >= document.lineCount) {
                    return@compute null
                }
                val start = document.getLineStartOffset(lineIndex)
                val end = document.getLineEndOffset(lineIndex)
                document.getText(com.intellij.openapi.util.TextRange(start, end))
            }
        } catch (t: Throwable) {
            LOG.debug("failed to read current source line for coroutine step", t)
            null
        }
    }

    private fun clearPendingCoroutineResume() {
        if (pendingCoroutineResume) {
            coroutineStepLog(
                "clear pending resume (was waiting for $pendingCoroutineResumeFile:$pendingCoroutineResumeLine " +
                    "from origin $pendingCoroutineResumeOriginLine, skips=$coroutineResumeAutoContinueCount)",
            )
        }
        pendingCoroutineResume = false
        coroutineResumeAutoContinueCount = 0
        pendingCoroutineResumeLine = 0
        pendingCoroutineResumeCallSiteLine = 0
        pendingCoroutineResumeFile = ""
        pendingCoroutineResumeOriginLine = 0
        pendingCoroutineResumeOriginWasSuspendCall = false
        pendingCoroutineResumeThreadId = 0
        pendingCoroutineResumeLastStatement = false
        pendingCoroutineStepOut = false
        pendingCoroutineResumeOriginSymbol = ""
        pendingParentPrologueHopDone = false
        pendingCallerStopLine = 0
        pendingCallerResumeArmed = false
        pendingResumeContinueInFlight = false
        launchBlockStepOutContinueChase = false
        plainCallerStepOutNativeUnwind = false
        pendingLaunchBlockStepOut = false
        pendingLaunchBlockMachineryLine = 0
        launchBlockSyntheticCaretPendingUi = false
        syncNativePendingResumeAutoContinue(enabled = false)
        try {
            breakpointHandler.restoreStepSuspendedBreakpoints()
        } catch (t: Throwable) {
            LOG.debug("restoreStepSuspendedBreakpoints failed: ${t.message}")
        }
        // Drop leftover Step Over one-shots (next-line / parent address) so a later Continue
        // is not stolen by a stale step target instead of the next user breakpoint.
        try {
            coroutineSupport.cancelArmedStepBreakpoint()
        } catch (t: Throwable) {
            LOG.debug("cancelArmedStepBreakpoint failed: ${t.message}")
        }
        stepOutWatchdogAlarm.cancelAllRequests()
    }

    /**
     * User breakpoint on a demo trigger line (e.g. 167) must not inherit coroutine Step Over
     * targets at 171–175 from an earlier session — those internal BPs skip the user line.
     */
    private fun onUserOwnedBreakpointPause(reason: String) {
        coroutineStepPipeline("reset coroutine step artifacts ($reason)")
        leftoverNativeStepPlanLikely = false
        plainCallerStepOutNativeUnwind = false
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        restoreBreakpointsAfterNativeCallSiteStepOut = false
        clearStepOverSuspendCallPin()
        stepOverSuspendCallSpilledLocals = false
        stepOverSuspendCallSpilledContinuationAddress = 0L
        clearPendingCoroutineResume()
        userSuspendCallBpStepOverAllowed = reason.contains("step-out", ignoreCase = true)
        clearStickyUserSourcePause()
        suspendInteriorUserBreakpointsAfterCallSitePause(reason)
    }

    /**
     * Demo call-site (167) + interior (173): while paused on the suspend *call* line, keep callee
     * breakpoints disabled so F8/Continue does not skip straight to 173 on the next launch cycle.
     * [startStepInto] restores them via [clearPendingCoroutineResume] before stepping in.
     */
    private fun suspendInteriorUserBreakpointsAfterCallSitePause(reason: String) {
        if (reason.contains("startStepInto", ignoreCase = true)) return
        val pausedForStepOutReturn = reason.contains("step-out", ignoreCase = true)
        val pausedOnUserBp = reason.contains("user breakpoint", ignoreCase = true)
        if (!pausedForStepOutReturn && !pausedOnUserBp) return
        val pos = lastTopFrameIdeSourceOverride ?: lastKnownMappableIdeSource ?: return
        val file = pos.file?.path.orEmpty()
        val line = pos.line + 1
        if (file.isEmpty() || line <= 0) return
        val lineText = sourceLineTextAt(file, line)
        if (!org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
                .looksLikeSuspendCall(lineText)
        ) {
            return
        }
        if (org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
                .looksLikeKnownLibrarySuspendCall(lineText)
        ) {
            return
        }
        if (breakpointHandler.findBreakpoint(file, line) == null) return
        val count = breakpointHandler.suspendUserBreakpointsInFileWithLineAfter(file, line)
        if (count > 0) {
            coroutineStepLog(
                "call-site pause — suspended $count interior user bp(s) after $file:$line ($reason)",
            )
        }
    }

    /** Fresh inferior or session end — do not inherit step-over disable / armed one-shots. */
    private fun resetInferiorCoroutineStepState(reason: String) {
        coroutineStepPipeline("reset inferior coroutine step state ($reason)")
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        userSuspendCallBpStepOverAllowed = false
        plainCallerStepOutNativeUnwind = false
        leftoverNativeStepPlanLikely = false
        restoreBreakpointsAfterNativeCallSiteStepOut = false
        clearStickyUserSourcePause()
        clearStepOverSuspendCallPin()
        clearPendingCoroutineResume()
        runCatching { breakpointHandler.resetStepSuspendTracking() }
        runCatching { coroutineSupport.cancelArmedStepBreakpoint() }
    }

    private fun scheduleStepOutPendingWatchdog(threadId: Int) {
        if (!pendingCoroutineStepOut) return
        stepOutWatchdogAlarm.cancelAllRequests()
        val gen = quietContinueGeneration
        val tid = threadId
        val timeoutMs = if (pendingLaunchBlockStepOut) 15_000L else 8_000L
        stepOutWatchdogAlarm.addRequest({
            if (stopped || session.project.isDisposed) return@addRequest
            if (!pendingCoroutineStepOut || !pendingCoroutineResume) return@addRequest
            if (gen != quietContinueGeneration) return@addRequest
            abandonStuckStepOut(
                tid,
                "step-out watchdog — no caller stop (restore breakpoints, timeout=${timeoutMs}ms)",
            )
        }, timeoutMs)
    }

    /**
     * Step Out chased caller (405/167) but inferior kept running: drop pending, re-enable user BPs,
     * restore paused UI so demo buttons work again.
     */
    private fun abandonStuckStepOut(threadId: Int, reason: String) {
        coroutineStepPipeline(reason)
        coroutineStepLog(reason)
        stepOutWatchdogAlarm.cancelAllRequests()
        restoreBreakpointsAfterNativeCallSiteStepOut = false
        clearPendingCoroutineResume()
        clearStickyUserSourcePause()
        pendingResumeContinueInFlight = false
        leftoverNativeStepPlanLikely = false
        clearUserContinueOrStepPending()
        if (threadId > 0 && !stopped) {
            isolateSuspended(threadId)
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
    }

    private fun markStickyUserSourcePause(reason: String) {
        stickyUserSourcePause = true
        pendingResumeContinueInFlight = false
        syncNativePendingResumeAutoContinue(enabled = false)
        val gen = ++quietContinueGeneration
        coroutineStepLog("sticky user pause ($reason) gen=$gen")
        // Do not discard thread plans here: interpreter commands on the event thread race
        // waitForEvent and can run after the process has already resumed (C05 306
        // "Command requires a process which is currently stopped"), which leaves Variables
        // stuck on Collecting data.
    }

    /**
     * Thread-list JNI walks SBFrame and changes the selected thread. That races
     * [org.cpf.kmp.tools.ohos.debugger.LLDBServiceWrapper.getFrameVariablesPage] after a
     * coroutine resume stop (C05 306: getSelectedThreadId jumped to 29364, Variables never returned).
     */
    internal fun shouldSkipStopSideEffects(): Boolean = pendingCoroutineResume || stickyUserSourcePause

    /**
     * Prefer continuation spilled fields over DWARF locals on the just-resumed user line.
     * First-entry stops (C05 303) are not sticky, so live DWARF `a=10` is kept.
     */
    internal fun shouldPreferContinuationSpilledLocals(): Boolean = stickyUserSourcePause

    internal fun shouldPreferStepOverSuspendCallSpilledLocals(): Boolean = stepOverSuspendCallSpilledLocals

    internal fun getStepOverSuspendCallSpilledContinuationAddress(): Long =
        stepOverSuspendCallSpilledContinuationAddress

    /**
     * RUNNING after the user line is already shown: keep the paused UI.
     *
     * Do **not** `SBProcess::Stop` here. Stop races [getFrameVariablesPage] / [getStackTrace]
     * (C05 306: `stopProcess ok=0`, Frames stuck on Loading, Variables on Collecting data).
     * Native `ContinueProcess` already no-ops when the process is running; leftover RUNNING
     * events are ignored for toolbar state only.
     *
     * @return true when this RUNNING must not clear the paused UI.
     */
    internal fun holdStickyPauseOnRunning(): Boolean {
        if (pendingCoroutineResume || stickyUserSourcePause) {
            coroutineStepLog("RUNNING during pending/sticky — keep paused UI, no SBProcess::Stop")
            return true
        }
        return false
    }

    private fun clearStickyUserSourcePause() {
        if (!stickyUserSourcePause && quietContinueGeneration == 0) return
        if (stickyUserSourcePause) {
            coroutineStepLog("clear sticky user pause gen=$quietContinueGeneration")
        }
        stickyUserSourcePause = false
        stepOverSuspendCallSpilledLocals = false
        stepOverSuspendCallSpilledContinuationAddress = 0L
        quietContinueGeneration++
        pendingResumeContinueInFlight = false
    }

    /** JNI waitForEvent Continues park/probe only while a next-line resume is pending and not sticky. */
    private fun syncNativePendingResumeAutoContinue(enabled: Boolean) {
        try {
            serviceWrapper.setAutoContinueInternalCoroutineStops(enabled)
        } catch (t: Throwable) {
            LOG.debug("setAutoContinueInternalCoroutineStops($enabled) failed: ${t.message}")
        }
    }

    /**
     * Event-loop preview of a STOPPED stack. Pins the resume target immediately so a following
     * park/Compose event cannot clear the variable cache or Continue past println.
     *
     * @return true when the caller must drop this stop (no cache clear, no UI, no Continue).
     */
    /**
     * Event-loop preview of a STOPPED stack. Pins the resume target immediately so a following
     * park/Compose event cannot clear the variable cache or Continue past println.
     */
    internal fun classifyIncomingCoroutineStop(
        stackFrames: List<StackFrame>,
        reason: String = "unknown",
    ): org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action {
        val top = stackFrames.firstOrNull()
        if (pendingCoroutineResume && top != null && frameMatchesPendingResume(top)) {
            markStickyUserSourcePause("resume target preview ${top.file}:${top.line}")
        }
        val topHasUserSource = top != null &&
            top.line > 0 &&
            LLDBStackFrameFilter.hasUserSourcePath(top.file) &&
            !isLibraryGlueFrame(top.name)
        if (pendingCoroutineResume && topHasUserSource &&
            shouldSurfacePendingCoroutineResumeStop(stackFrames, reason)
        ) {
            markStickyUserSourcePause("resume target preview ${top.file}:${top.line}")
            return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.FALLTHROUGH
        }
        if (org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isUnspecifiedStopReason(reason) &&
            !isCoroutineProbeStop(stackFrames)
        ) {
            // Rapid F8 often delivers a transient unknown/empty STOPPED while the process is
            // still stepping. Falling through would discard thread plans on a running inferior
            // ("Command requires a process which is currently stopped") and thrash the UI.
            if (stackFrames.isEmpty()) {
                coroutineStepLog(
                    "incoming $reason empty-stack stop — hold (ignore spurious mid-step event)",
                )
                return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.HOLD
            }
            coroutineStepLog(
                "incoming $reason stop — fall through (do not quiet-continue None past user BP)",
            )
            return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.FALLTHROUGH
        }
        if (pendingLaunchBlockStepOut && pendingCoroutineResume && isLibraryGlueStop(stackFrames)) {
            coroutineStepPipeline(
                "step-out launch-block runtime stop — fall through to pin caret " +
                    "${stackFrames.firstOrNull()?.file}:${stackFrames.firstOrNull()?.line}",
            )
            return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.FALLTHROUGH
        }
        val action = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.decide(
            topIsProbe = isCoroutineProbeStop(stackFrames),
            topIsGlue = isLibraryGlueStop(stackFrames),
            emptyStack = stackFrames.isEmpty(),
            pendingResume = pendingCoroutineResume,
            stickyUserPause = stickyUserSourcePause,
            leftoverNativeStep = leftoverNativeStepPlanLikely,
            nativeStepIntoInFlight = pendingNativeStepInto,
        )
        when (action) {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.HOLD -> {
                coroutineStepLog(
                    "hold sticky pause — ignore incoming ${top?.name} ${top?.file}:${top?.line}, " +
                        "no cache clear / no UI / no Continue",
                )
            }
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.QUIET_CONTINUE -> {
                coroutineStepLog(
                    "incoming quiet-continue — skip cache clear / UI ${top?.name} ${top?.file}:${top?.line}",
                )
            }
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.FALLTHROUGH -> Unit
        }
        return action
    }

    /**
     * Continue a park/probe/glue stop without going through [handleStopped]
     * (no cache clear, no positionReached). Runs on the LLDB event-loop thread so Continue
     * does not wait for the EDT (C07 delay Step Over).
     */
    internal fun quietContinueIncomingStop(threadId: Int, stackFrames: List<StackFrame>, reason: String = "unknown") {
        if (stopped || session.project.isDisposed) return
        val surfaced = applyProbeOrPendingResumeSkip(threadId, stackFrames, reason)
        if (surfaced != null) {
            coroutineStepPipeline(
                "step-out launch-block surfacing from quiet-continue path " +
                    "${surfaced.second.firstOrNull()?.file}:${surfaced.second.firstOrNull()?.line}",
            )
            val tid = surfaced.first
            val frames = surfaced.second
            com.intellij.openapi.application.ApplicationManager.getApplication().invokeLater {
                if (!stopped && !session.project.isDisposed) {
                    handleStopped(tid, reason, frames)
                }
            }
        }
    }

    private fun coroutineStepLog(message: String) {
        // Stdout only when plugin debug is on — never spam the Debug Console.
        pluginDbgPrintln("[Coroutines][step] $message")
    }

    /** Step-out diagnostics always visible in Debug Console (not gated on plugin debug). */
    private fun coroutineStepPipeline(message: String) {
        session.printHarmonyDebugPipelineLine("[Coroutines][step] $message")
        pluginDbgPrintln("[Coroutines][step] $message")
    }

    /**
     * True when this stop already landed on the one-shot resume target, so it must be shown
     * rather than auto-continued.
     */
    private fun isPendingCoroutineResumeTarget(stackFrames: List<StackFrame>): Boolean {
        if (pendingCoroutineResumeLine <= 0) return false
        val top = stackFrames.firstOrNull() ?: return false
        return frameMatchesPendingResume(top)
    }

    private fun frameMatchesPendingResume(frame: StackFrame): Boolean {
        if (isLibraryGlueFrame(frame.name)) return false
        // Last-statement arms `nextLine=0`. Matching App.kt:0 would pin sticky pause on the
        // parent resume PC and then block the quiet-continue that never should have run.
        if (pendingCoroutineResumeLine <= 0) return false
        if (frame.line != pendingCoroutineResumeLine) return false
        return frameFileMatchesPending(frame.file)
    }

    /**
     * After Step Over/Into on `delay`, keep going until user source is the *top* frame.
     * If the one-shot at 304 is missed, App.kt:305 (next delay / user BP) must still be shown —
     * treating it as “state-machine epilogue” Continues past `println` into Compose / OH_Drawing.
     */
    private fun shouldSurfacePendingCoroutineResumeStop(stackFrames: List<StackFrame>, reason: String): Boolean {
        val top = stackFrames.firstOrNull()
        if (top == null) {
            coroutineStepLog("surface? no — empty stack")
            return false
        }
        val fileMatches = frameFileMatchesPending(top.file)
        val topSourceForPolicy = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
            ?: sourceLineTextAt(pendingCoroutineResumeFile, top.line.coerceAtLeast(1))
        val closingBrace = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isClosingBraceLine(
            topSourceForPolicy,
        )
        val reasonBp = reason.equals("breakpoint", ignoreCase = true)
        val userBp = if (reasonBp) breakpointHandler.findBreakpoint(top.file, top.line) != null else false
        val surface = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.shouldSurfacePendingResume(
            topIsGlue = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isGlueOrNoUserSourceFrame(
                top.name,
                top.file,
                top.line,
            ),
            topIsStateMachine = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isStateMachineFrame(top.name),
            topLine = top.line,
            fileMatches = fileMatches,
            originLine = pendingCoroutineResumeOriginLine,
            targetLine = pendingCoroutineResumeLine,
            sourceLineIsClosingBrace = closingBrace,
            reasonIsBreakpoint = reasonBp,
            hasUserBreakpointAtTop = userBp,
            lastStatementResume = pendingCoroutineResumeLastStatement,
            originSymbol = pendingCoroutineResumeOriginSymbol,
            topSymbol = top.name,
            topIsCoroutineLambda = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                .isCoroutineLambdaFrame(top.name),
            topHasUserSource = top.line > 0 &&
                LLDBStackFrameFilter.hasUserSourcePath(top.file) &&
                !isLibraryGlueFrame(top.name),
            callerStopLine = pendingCallerStopLine,
            callerResumeArmed = pendingCallerResumeArmed,
            stepOutResume = pendingCoroutineStepOut,
            launchBlockMachineryLine = pendingLaunchBlockMachineryLine,
            stepOutCallSiteLine = pendingCoroutineResumeCallSiteLine,
            originWasSuspendCallStepOver = pendingCoroutineResumeOriginWasSuspendCall,
        )
        coroutineStepLog(
            if (surface) {
                "surface? yes — ${top.file}:${top.line} ${top.name} (waiting for $pendingCoroutineResumeLine)"
            } else {
                "surface? no — ${top.file}:${top.line} ${top.name} (waiting for $pendingCoroutineResumeLine, reason=$reason)"
            },
        )
        if (pendingLaunchBlockStepOut) {
            coroutineStepPipeline(
                if (surface) {
                    "step-out launch-block surface YES ${top.file}:${top.line} ${top.name} " +
                        "(want ${pendingCoroutineResumeLine}/machinery=$pendingLaunchBlockMachineryLine)"
                } else {
                    "step-out launch-block surface NO ${top.file}:${top.line} ${top.name} reason=$reason"
                },
            )
        }
        return surface
    }

    /**
     * User breakpoint that must stay suspended. The `fun` line is rewritten onto the statement
     * the user actually marked (`private fun leftPart` → `val part = 12`).
     * The line we just stepped from is excluded: Continue has to leave that breakpoint.
     */
    private fun userBreakpointStopToSurface(
        stackFrames: List<StackFrame>,
        reason: String,
    ): List<StackFrame>? {
        if (!reason.equals("breakpoint", ignoreCase = true)) return null
        val top = stackFrames.firstOrNull() ?: return null
        if (top.file.isEmpty() || top.line <= 0) return null
        if (isLibraryGlueFrame(top.name)) return null
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val line = policy.userBreakpointLineToSurface(top.line, sourceLineTextAt(top.file, top.line)) { candidate ->
            breakpointHandler.findBreakpoint(top.file, candidate) != null
        }
        if (line <= 0 || line == pendingCoroutineResumeOriginLine) return null
        if (line == top.line) return stackFrames
        return listOf(top.copy(line = line)) + stackFrames.drop(1)
    }

    /**
     * Step Over already left this line, but Continue hit the same user breakpoint again.
     * Disable it for this resume so the next user line (or `rightPart`) can stop with Frames.
     *
     * @return true when this stop was consumed.
     */
    private fun stepPastOriginUserBreakpoint(threadId: Int, top: StackFrame): Boolean {
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val line = policy.userBreakpointLineToSurface(top.line, sourceLineTextAt(top.file, top.line)) { candidate ->
            breakpointHandler.findBreakpoint(top.file, candidate) != null
        }
        if (line <= 0 || line != pendingCoroutineResumeOriginLine) return false
        val suspended = breakpointHandler.suspendUserBreakpointForStep(top.file, line)
        val plainCallee = policy.isPlainUserCalleeFrame(
            topIsGlue = isLibraryGlueFrame(top.name),
            topIsStateMachine = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                .isStateMachineFrame(top.name),
            topIsCoroutineLambda = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                .isCoroutineLambdaFrame(top.name),
        )
        coroutineStepLog(
            "origin user breakpoint re-hit ${top.file}:$line — " +
                (if (suspended) "disabled for this resume" else "disable missed, plainCallee=$plainCallee"),
        )
        if (threadId > 0) isolateResumed(threadId)
        val moved = if (suspended || !plainCallee) {
            continuePendingResumeQuietly(threadId)
        } else {
            serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Over)
        }
        if (!moved) {
            coroutineStepLog("step past origin breakpoint failed; restoring pause")
            clearPendingCoroutineResume()
            markStickyUserSourcePause("step past origin breakpoint failed")
            isolateSuspended(threadId)
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
        return true
    }

    /** 1-based LLDB line of [filePath], or null when the document is unavailable. */
    private fun sourceLineTextAt(filePath: String?, line1Based: Int): String? {
        val path = filePath?.trim().orEmpty()
        if (path.isEmpty() || line1Based <= 0) return null
        return try {
            com.intellij.openapi.application.ReadAction.compute<String?, RuntimeException> {
                val normalized = path.replace('\\', '/')
                val vf = com.intellij.openapi.vfs.LocalFileSystem.getInstance().findFileByPath(normalized)
                    ?: lastTopFrameIdeSourceOverride?.file?.takeIf { file ->
                        file.name.equals(normalized.substringAfterLast('/'), ignoreCase = true)
                    }
                    ?: return@compute null
                val document =
                    com.intellij.openapi.fileEditor.FileDocumentManager.getInstance().getDocument(vf)
                    ?: return@compute null
                val idx = line1Based - 1
                if (idx < 0 || idx >= document.lineCount) return@compute null
                val start = document.getLineStartOffset(idx)
                val end = document.getLineEndOffset(idx)
                document.getText(com.intellij.openapi.util.TextRange(start, end))
            }
        } catch (t: Throwable) {
            LOG.debug("failed to read source line $path:$line1Based", t)
            null
        }
    }

    private fun isCoroutineProbeStop(stackFrames: List<StackFrame>): Boolean {
        val top = stackFrames.firstOrNull() ?: return false
        return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isProbeFrame(top.name)
    }

    private fun isLibraryGlueFrame(symbolName: String?): Boolean =
        org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isLibraryGlueFrame(symbolName)

    private fun isLibraryGlueStop(stackFrames: List<StackFrame>): Boolean {
        val top = stackFrames.firstOrNull() ?: return false
        return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isGlueOrNoUserSourceFrame(
            top.name,
            top.file,
            top.line,
        )
    }

    private fun bumpResumeSkipCount(threadId: Int): Int {
        val armedTid = pendingCoroutineResumeThreadId
        if (armedTid > 0 && threadId > 0 && threadId != armedTid) {
            coroutineStepLog(
                "quiet continue other thread=$threadId (armed on $armedTid) — not counted " +
                    "(skips stay $coroutineResumeAutoContinueCount)",
            )
            return coroutineResumeAutoContinueCount
        }
        coroutineResumeAutoContinueCount++
        return coroutineResumeAutoContinueCount
    }

    /**
     * All-stop can report the UI thread (`Modifier.foldIn`) while the KN worker is already on
     * the resume line. Prefer that worker so we do not Continue past the one-shot at 304/306.
     */
    private fun findThreadAtPendingResumeTarget(currentThreadId: Int): Pair<Int, List<StackFrame>>? {
        if (pendingCoroutineResumeLine <= 0) return null
        val threads = try {
            val now = System.currentTimeMillis()
            if (now - lastPendingThreadScanMs < 80L) {
                coroutineStepLog("thread scan skipped (throttled)")
                return null
            }
            lastPendingThreadScanMs = now
            serviceWrapper.getThreadListPreview(
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.DEFAULT_MAX_THREADS,
            )
        } catch (t: Throwable) {
            coroutineStepLog("thread scan failed: ${t.message}")
            return null
        }
        for (thread in threads) {
            val tid = thread.threadId
            val infos = buildList {
                thread.topFrame?.let { add(it) }
                thread.previewFrames?.let { addAll(it) }
            }
            for ((index, info) in infos.withIndex()) {
                val name = info.functionName.orEmpty()
                if (isLibraryGlueFrame(name)) continue
                val file = info.filePath.orEmpty()
                val line = info.line
                if (line != pendingCoroutineResumeLine) continue
                if (!frameFileMatchesPending(file)) continue
                val ordered = infos.drop(index)
                val frames = ordered.map { preview ->
                    StackFrame(
                        id = preview.frameId,
                        name = preview.functionName ?: "unknown",
                        file = preview.filePath ?: "",
                        line = preview.line,
                        column = preview.column,
                    )
                }
                coroutineStepLog(
                    "found resume target on thread=$tid (current=$currentThreadId) " +
                        "${name} $file:$line previewFrames=${frames.size} (dropped $index glue frames)",
                )
                return tid to frames
            }
        }
        coroutineStepLog("thread scan: no thread at $pendingCoroutineResumeFile:$pendingCoroutineResumeLine (listed=${threads.size})")
        return null
    }

    private fun frameFileMatchesPending(file: String): Boolean {
        if (pendingCoroutineResumeFile.isEmpty()) return false
        if (file.isEmpty()) return pendingCoroutineStepOut || pendingCoroutineResume
        val expected = pendingCoroutineResumeFile.substringAfterLast('/').substringAfterLast('\\')
        val actual = file.substringAfterLast('/').substringAfterLast('\\')
        return expected.equals(actual, ignoreCase = true) ||
            file == pendingCoroutineResumeFile ||
            pendingCoroutineResumeFile.endsWith(file)
    }

    private fun clearStepOverSuspendCallPin() {
        stepOverSuspendCallPinLine = 0
        stepOverSuspendCallPinFile = ""
        stepOverSuspendCallPinOriginSymbol = ""
    }

    private fun coroutineStateMachineToken(originSymbol: String): String? {
        if (originSymbol.isEmpty()) return null
        val pattern = Regex("""(\$[\w.]+\${'$'}COROUTINE\$\d+)""")
        return pattern.find(originSymbol)?.groupValues?.get(1)
    }

    private fun findInvokeSuspendFrameForStepOverPin(
        stackFrames: List<StackFrame>,
        pinFile: String,
        pinLine: Int,
        originSymbol: String,
    ): StackFrame? {
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        if (originSymbol.isNotEmpty()) {
            stackFrames.firstOrNull { policy.isSameStateMachineSymbol(originSymbol, it.name) }?.let { return it }
        }
        val coroutineToken = coroutineStateMachineToken(originSymbol)
        if (coroutineToken != null) {
            stackFrames.firstOrNull { f ->
                f.name.contains(coroutineToken) && classifier.isStateMachineFrame(f.name)
            }?.let { return it }
        }
        return stackFrames.firstOrNull { f ->
            classifier.isStateMachineFrame(f.name) &&
                (sourceFileBasenameMatches(f.file, pinFile) || f.file.isEmpty()) &&
                f.line in (pinLine - 3)..(pinLine + 1)
        }
    }

    /** Truncated STOP previews often omit the `$COROUTINE$…#invokeSuspend` row that owns locals. */
    private fun maybeExpandStackForStepOverPin(initial: List<StackFrame>): List<StackFrame> {
        val pinLine = stepOverSuspendCallPinLine
        val pinFile = stepOverSuspendCallPinFile
        if (pinLine <= 0 || pinFile.isEmpty()) return initial
        val origin = stepOverSuspendCallPinOriginSymbol.ifEmpty { pendingCoroutineResumeOriginSymbol }
        if (findInvokeSuspendFrameForStepOverPin(initial, pinFile, pinLine, origin) != null) {
            return initial
        }
        val tid = lastSuspendedThreadId ?: return initial
        return try {
            val latch = java.util.concurrent.CountDownLatch(1)
            var out = initial
            serviceWrapper.getStackTrace(tid) { trace ->
                out = trace.ifEmpty { initial }
                latch.countDown()
            }
            if (latch.await(3, java.util.concurrent.TimeUnit.SECONDS)) out else initial
        } catch (_: Throwable) {
            initial
        }
    }

    private fun sourceFileBasenameMatches(file: String, expectedFullPath: String): Boolean {
        if (expectedFullPath.isEmpty()) return true
        if (file.isEmpty()) return true
        val expected = expectedFullPath.substringAfterLast('/').substringAfterLast('\\')
        val actual = file.substringAfterLast('/').substringAfterLast('\\')
        return expected.equals(actual, ignoreCase = true) ||
            file == expectedFullPath ||
            expectedFullPath.endsWith(file)
    }

    /**
     * KN/OHOS LLDB may resume past the armed println line; pin the editor caret even when
     * [pendingCoroutineResume] was already cleared by JNI quiet-continue chains.
     */
    private fun applyStepOverSuspendCallTargetPin(stackFrames: List<StackFrame>): List<StackFrame> {
        val pinLine = stepOverSuspendCallPinLine
        if (pinLine <= 0) return stackFrames
        val pinFile = stepOverSuspendCallPinFile
        val stackFramesExpanded = maybeExpandStackForStepOverPin(stackFrames)
        val top = stackFramesExpanded.firstOrNull() ?: return stackFrames
        if (!sourceFileBasenameMatches(top.file, pinFile)) return stackFramesExpanded
        if (top.line == pinLine) {
            coroutineStepPipeline("step-over suspend-call pin satisfied at $pinFile:$pinLine")
            clearStepOverSuspendCallPin()
            return stackFramesExpanded
        }
        if (top.line > pinLine && top.line <= pinLine + 2) {
            val origin = stepOverSuspendCallPinOriginSymbol.ifEmpty { pendingCoroutineResumeOriginSymbol }
            val smFrame = findInvokeSuspendFrameForStepOverPin(stackFramesExpanded, pinFile, pinLine, origin)
            val pinnedTop = if (smFrame != null && smFrame.id != top.id) {
                smFrame.copy(
                    line = pinLine,
                    file = top.file.ifEmpty { smFrame.file },
                )
            } else {
                top.copy(line = pinLine)
            }
            coroutineStepPipeline(
                "step-over suspend-call UI pin ${top.file}:${top.line} → $pinFile:$pinLine " +
                    "(frame=${top.name.take(80)}" +
                    (if (smFrame != null && smFrame.id != top.id) {
                        " varsFrame=${smFrame.name.take(80)} id=${smFrame.id}"
                    } else if (smFrame == null) {
                        " varsFrame=missing stack=${stackFramesExpanded.size}"
                    } else {
                        ""
                    }) +
                    ")",
            )
            if (smFrame == null || smFrame.id == top.id) {
                stepOverSuspendCallSpilledLocals = true
                markStickyUserSourcePause("step-over suspend-call pin locals $pinFile:$pinLine")
            }
            clearStepOverSuspendCallPin()
            val reordered = if (smFrame != null && smFrame.id != top.id) {
                listOf(pinnedTop) + stackFramesExpanded.filter { it.id != smFrame.id }
            } else {
                listOf(pinnedTop) + stackFramesExpanded.drop(1)
            }
            return reordered
        }
        return stackFramesExpanded
    }

    /**
     * After native Step Out from a `launch { suspend() }` callee, LLDB often stops on
     * `BaseContinuationImpl` (glue). The launch lambda may still be on the stack or in the USB dump.
     */
    private fun trySurfaceLaunchBlockStepOutFromStack(
        threadId: Int,
        stackFrames: List<StackFrame>,
    ): Pair<Int, List<StackFrame>>? {
        if (!pendingLaunchBlockStepOut || !pendingCoroutineStepOut || pendingCoroutineResumeLine <= 0) {
            return null
        }
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val originSm = pendingCoroutineResumeOriginSymbol
        val wantLine = pendingCoroutineResumeLine
        val machinery = pendingLaunchBlockMachineryLine
        val origin = pendingCoroutineResumeOriginLine

        fun pinCaret(frame: StackFrame): StackFrame {
            val line = when {
                machinery > 0 && frame.line == machinery -> wantLine
                frame.line == wantLine -> wantLine
                frame.line in 1 until origin -> wantLine
                else -> wantLine
            }
            return frame.copy(line = line)
        }

        fun isCallerCandidate(frame: StackFrame): Boolean {
            if (policy.isSameStateMachineSymbol(originSm, frame.name)) return false
            if (frame.file.isNotEmpty() && !frameFileMatchesPending(frame.file)) return false
            if (frame.line == wantLine || (machinery > 0 && frame.line == machinery)) return true
            if (frame.line in 1 until origin &&
                (classifier.isCoroutineLambdaFrame(frame.name) ||
                    classifier.isStateMachineFrame(frame.name))
            ) {
                return true
            }
            return frameFileMatchesPending(frame.file) &&
                classifier.isCoroutineLambdaFrame(frame.name)
        }

        for (frame in stackFrames) {
            if (!isCallerCandidate(frame)) continue
            val top = pinCaret(frame)
            coroutineStepPipeline(
                "step-out launch-block surfaced from stack ${frame.file}:${frame.line} → caret ${top.line}",
            )
            clearPendingCoroutineResume()
            leftoverNativeStepPlanLikely = false
            markStickyUserSourcePause("launch block step out")
            return threadId to listOf(top) + stackFrames.filter { it !== frame }
        }

        coroutineSupport.logicalCallerFrameForStepOut(threadId, origin)?.let { (file, line) ->
            if (line <= 0 || line >= origin || !frameFileMatchesPending(file)) return@let null
            val name = stackFrames.firstOrNull { isCallerCandidate(it) }?.name
                ?: stackFrames.drop(1).firstOrNull { !classifier.isLibraryGlueFrame(it.name) }?.name
                ?: "launch-block-caller"
            val top = StackFrame(
                id = 0,
                name = name,
                file = pendingCoroutineResumeFile.ifEmpty { file },
                line = wantLine,
            )
            coroutineStepPipeline(
                "step-out launch-block surfaced from dump $file:$line → caret $wantLine",
            )
            clearPendingCoroutineResume()
            leftoverNativeStepPlanLikely = false
            markStickyUserSourcePause("launch block step out dump")
            return threadId to listOf(top) + stackFrames
        }

        val top = stackFrames.firstOrNull()
        val glueTop = top == null ||
            classifier.isLibraryGlueFrame(top.name) ||
            top.file.replace('\\', '/').contains("ContinuationImpl")
        if (glueTop && pendingCoroutineResumeFile.isNotEmpty()) {
            val synthetic = StackFrame(
                id = 0,
                name = "launch-block-caller",
                file = pendingCoroutineResumeFile,
                line = wantLine,
            )
            coroutineStepPipeline(
                "step-out launch-block synthetic caret ${pendingCoroutineResumeFile}:$wantLine " +
                    "(runtime trampoline after native Step Out)",
            )
            positionMapper.parseSourcePosition(pendingCoroutineResumeFile, wantLine)?.let { pos ->
                lastKnownMappableIdeSource = pos
                lastTopFrameIdeSourceOverride = pos
            }
            launchBlockSyntheticCaretPendingUi = true
            clearPendingCoroutineResume()
            leftoverNativeStepPlanLikely = false
            glueDisassemblyInstructionStepping = false
            launchBlockStepOutContinueChase = false
            markStickyUserSourcePause("launch block step out synthetic")
            return threadId to listOf(synthetic) + stackFrames
        }
        return null
    }

    /**
     * Quiet Continue for the delay-resume wait. Do **not** discard thread plans on every skip:
     * Discard+Continue on each probe/Compose stop re-enters `handleStopped` as a `step` with no
     * source, which opens Disassembly, then the next user-frame restore closes it — tab ping-pong
     * and an eventual IDE crash. Plans are discarded once when the user first Steps.
     *
     * Never Continue if LLDB already resumed (probe AutoContinue). A second Continue on a running
     * process is the SIGSEGV in `liblldb` `StackFrame::GetRecognizedFrame`.
     */
    private fun continuePendingResumeQuietly(threadId: Int): Boolean {
        // User line already surfaced (sticky) and no resume target — release epilogue glue stops.
        if (stickyUserSourcePause && !pendingCoroutineResume) {
            clearStickyUserSourcePause()
            coroutineStepLog("quiet continue — clear sticky epilogue pause")
        } else if (stickyUserSourcePause) {
            coroutineStepLog("quiet continue skipped — sticky user pause")
            return false
        }
        if (pendingResumeContinueInFlight) {
            coroutineStepLog("quiet continue skipped — continue already in flight")
            return true
        }
        val gen = quietContinueGeneration
        // Always issue Continue here. `lldbProcessRunning` is a Java hint from the *previous*
        // Continue and is still true when this STOPPED arrives, so treating it as success leaves
        // LLDB paused (C05: epilogue at `}`) with the IDE showing "Frames are not available".
        // Native ContinueProcess already no-ops if the process is genuinely RUNNING.
        pendingResumeContinueInFlight = true
        lldbProcessRunning = true
        if (gen != quietContinueGeneration || stickyUserSourcePause) {
            coroutineStepLog("quiet continue cancelled (generation changed or sticky pause)")
            pendingResumeContinueInFlight = false
            lldbProcessRunning = false
            return false
        }
        val ok = serviceWrapper.resumeThread(threadId, null)
        if (!ok) {
            pendingResumeContinueInFlight = false
            lldbProcessRunning = false
        }
        coroutineStepLog("quiet continue thread=$threadId ok=$ok gen=$gen")
        return ok
    }

    /**
     * Parent `label` PC is often DWARF line 0. Arm a one-shot at the next **caller** user line
     * (C04 295 → 360 `c05SpilledMany()`, not 357 `c04SpilledUser()` again) and Continue once.
     * Returns true when Continue was issued.
     */
    private fun hopToNextUserSourceAfterParentResume(threadId: Int): Boolean {
        if (pendingParentPrologueHopDone) return false
        pendingParentPrologueHopDone = true
        return try {
            if (threadId > 0) {
                serviceWrapper.selectThread(threadId)
            }
            val raw = serviceWrapper.runInterpreterCommandForOutput(
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.sourceAfterPc(),
            )
            val loc = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.parseSourceAfterPc(raw)
            coroutineStepLog(
                "source-after-pc raw=${raw?.trim()} addr=0x${java.lang.Long.toHexString(loc.address)} " +
                    "file=${loc.file} line=${loc.line}",
            )
            if (!org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.isLikelyCodeAddress(loc.address)) {
                return false
            }
            val hopFile = loc.file.ifEmpty { pendingCoroutineResumeFile }
            val hopText = sourceLineTextAt(hopFile, loc.line)
            val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            if (!policy.isValidParentResumeUserLine(loc.line, hopText)) {
                coroutineStepLog(
                    "source-after-pc rejected ${loc.file}:${loc.line} (prologue/header/brace) — no hop",
                )
                return false
            }
            val targetLine = policy.advancePastReturnedCallSite(
                pendingCoroutineResumeOriginSymbol,
                loc.line,
                hopText,
                { line -> sourceLineTextAt(hopFile, line) },
            )
            val targetText = if (targetLine == loc.line) hopText else sourceLineTextAt(hopFile, targetLine)
            if (!policy.isValidParentResumeUserLine(targetLine, targetText)) {
                return false
            }
            val output = if (targetLine != loc.line && hopFile.isNotEmpty()) {
                coroutineStepLog("parent hop skip returned call site ${loc.line} → $targetLine")
                serviceWrapper.runInterpreterCommandForOutput(
                    org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.lineBreakpoint(hopFile, targetLine),
                )
            } else {
                serviceWrapper.runInterpreterCommandForOutput(
                    org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.addressBreakpoint(loc.address),
                )
            }
            if (output == null) return false
            coroutineSupport.trackArmedStepBreakpoint(output)
            pendingCoroutineResumeLine = targetLine
            if (hopFile.isNotEmpty()) pendingCoroutineResumeFile = hopFile
            coroutineStepLog(
                "parent prologue hop — one-shot $pendingCoroutineResumeFile:$pendingCoroutineResumeLine " +
                    "addr=0x${java.lang.Long.toHexString(loc.address)} output=${output.trim()}",
            )
            isolateResumed(threadId)
            val continued = continuePendingResumeQuietly(threadId)
            if (!continued) {
                pendingCoroutineResumeLine = 0
            }
            continued
        } catch (t: Throwable) {
            coroutineStepLog("parent prologue hop failed: ${t.message}")
            false
        }
    }

    /**
     * Step Over of `await` lands in `Function2` at `[K][Suspend]Functions` and the IDE opens
     * Disassembly. That frame is the suspend trampoline. Continue to the next Kotlin line
     * (`gate.complete`) instead of showing it.
     *
     * @return true when this stop was consumed.
     */
    private fun leaveSuspendCallingConvention(
        threadId: Int,
        stackFrames: List<StackFrame>,
        reason: String,
    ): Boolean {
        val top = stackFrames.firstOrNull() ?: return false
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        if (!policy.isKotlinSuspendCallingConventionFrame(top.name, top.file)) return false
        if (!reason.equals("step", ignoreCase = true) && !reason.equals("breakpoint", ignoreCase = true)) {
            return false
        }
        suspendCallingConventionHops++
        if (suspendCallingConventionHops > 4) {
            coroutineStepLog("suspend calling convention hop limit — restore last Kotlin line")
            suspendCallingConventionHops = 0
            clearPendingCoroutineResume()
            if (threadId > 0) isolateSuspended(threadId)
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
            return true
        }
        val origin = lastTopFrameIdeSourceOverride ?: lastKnownMappableIdeSource
        val file = origin?.file?.path.orEmpty()
        val originLine = (origin?.line ?: -1) + 1
        val target = if (file.isNotEmpty() && originLine > 0) {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                .resolveLambdaStepOverPastSuspend(originLine) { line -> sourceLineTextAt(file, line) }
        } else {
            null
        }
        if (target != null && !target.lastStatement && target.nextLine1Based > originLine) {
            pendingCoroutineResume = true
            pendingCoroutineResumeLine = target.nextLine1Based
            pendingCoroutineResumeFile = file
            pendingCoroutineResumeOriginLine = originLine
            pendingCoroutineResumeThreadId = threadId
            pendingCoroutineResumeLastStatement = false
            pendingCoroutineResumeOriginSymbol = lastSuspendedStackFrames?.firstOrNull()?.name.orEmpty()
            pendingCallerStopLine = 0
            pendingCallerResumeArmed = false
            coroutineResumeAutoContinueCount = 0
            syncNativePendingResumeAutoContinue(enabled = true)
            if (continueToCallerUserLine(threadId, file, target.nextLine1Based)) {
                coroutineStepLog(
                    "suspend calling convention ${top.name} — one-shot $file:${target.nextLine1Based}, no Disassembly",
                )
                return true
            }
            clearPendingCoroutineResume()
        }
        coroutineStepLog("suspend calling convention ${top.name} ${top.file}:${top.line} — step out, no Disassembly")
        if (threadId > 0) isolateResumed(threadId)
        val stepped = serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Out)
        if (!stepped) {
            suspendCallingConventionHops = 0
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
        return true
    }

    /** One-shot at the caller line after the returned call site (357 → 360). */
    private fun continueToCallerUserLine(threadId: Int, file: String, line: Int): Boolean {
        if (threadId <= 0 || file.isEmpty() || line <= 1) return false
        val text = sourceLineTextAt(file, line)
        if (!org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isValidParentResumeUserLine(line, text)) {
            return false
        }
        val output = serviceWrapper.runInterpreterCommandForOutput(
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.lineBreakpoint(file, line),
        ) ?: return false
        if (!org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.breakpointHasLocations(output)) {
            val id = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.parseBreakpointId(output)
            if (id > 0) {
                serviceWrapper.runInterpreterCommandForOutput(
                    org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineCommands.deleteBreakpoint(id),
                )
            }
            coroutineStepLog("one-shot $file:$line has no locations — not continuing")
            return false
        }
        coroutineSupport.trackArmedStepBreakpoint(output)
        pendingCoroutineResumeLine = line
        pendingCoroutineResumeFile = file
        coroutineStepLog(
            "last-statement skip call site — one-shot $file:$line output=${output.trim()}",
        )
        isolateResumed(threadId)
        val continued = continuePendingResumeQuietly(threadId)
        if (!continued) pendingCoroutineResumeLine = 0
        return continued
    }

    /** Step Out target is caller [pendingCoroutineResumeCallSiteLine] but PC/DWARF is still in the callee body. */
    private fun stepOutStopStillInsideCalleeInterior(top: StackFrame): Boolean {
        if (!pendingCoroutineStepOut) return false
        val callSite = pendingCoroutineResumeCallSiteLine
        val origin = pendingCoroutineResumeOriginLine
        if (callSite <= 0 || origin <= callSite || top.line <= 0) return false
        if (top.line <= callSite || top.line >= origin) return false
        return frameFileMatchesPending(top.file) ||
            sourceFileBasename(top.file) == sourceFileBasename(pendingCoroutineResumeFile)
    }

    /**
     * One native Step Out from 173 often lands on 171 (callee header) — pop once more toward caller.
     */
    private fun tryContinuePlainCallerStepOutNativeUnwind(threadId: Int, top: StackFrame): Boolean {
        if (!pendingCoroutineStepOut || !leftoverNativeStepPlanLikely) return false
        if (!stepOutStopStillInsideCalleeInterior(top)) return false
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        if (!classifier.isStateMachineFrame(top.name)) return false
        if (!policy.isSameStateMachineSymbol(pendingCoroutineResumeOriginSymbol, top.name)) return false
        coroutineStepPipeline(
            "step-out still inside callee ${top.file}:${top.line} — native Step Out again " +
                "(target call=$pendingCoroutineResumeCallSiteLine origin=$pendingCoroutineResumeOriginLine)",
        )
        if (threadId > 0) isolateResumed(threadId)
        val option = if (useInstructionLevelStepping()) {
            LLDBServiceWrapper.StepOption.InstructionOver
        } else {
            LLDBServiceWrapper.StepOption.Out
        }
        val ok = serviceWrapper.resumeThread(threadId, option)
        if (ok && pendingCoroutineStepOut) {
            scheduleStepOutPendingWatchdog(threadId)
        }
        return ok
    }

    /**
     * Pin the caret to the statement Step Over should show.
     *
     * Parent `label` restore is DWARF line 0/1 (`package`). The return address of `start()`
     * is often still labeled as the call (line 140) even though the next statement is line 141.
     */
    private fun rewriteTopFrameLineIfPendingTarget(stackFrames: List<StackFrame>): List<StackFrame> {
        val top = stackFrames.firstOrNull() ?: return stackFrames
        if (!pendingCoroutineStepOut && pendingCoroutineResume && pendingCoroutineResumeLine > 0) {
            val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            val topText = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
                ?: sourceLineTextAt(pendingCoroutineResumeFile, top.line.coerceAtLeast(1))
            val targetFile = pendingCoroutineResumeFile.ifEmpty { top.file }
            val targetText = if (targetFile.isNotEmpty()) {
                sourceLineTextAt(targetFile, pendingCoroutineResumeLine)
            } else {
                null
            }
            val fileOk = frameFileMatchesPending(top.file) || top.file.isEmpty()
            if (fileOk &&
                pendingCoroutineResumeOriginWasSuspendCall &&
                top.line == pendingCoroutineResumeLine + 1
            ) {
                coroutineStepPipeline(
                    "step-over suspend-call overshoot ${top.file}:${top.line} → " +
                        "$pendingCoroutineResumeFile:$pendingCoroutineResumeLine",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
            if (fileOk &&
                !pendingCoroutineResumeOriginWasSuspendCall &&
                top.line == pendingCoroutineResumeLine + 1 &&
                policy.isValidParentResumeUserLine(pendingCoroutineResumeLine, targetText)
            ) {
                coroutineStepPipeline(
                    "step-over no-dwarf next-line pin ${top.file}:${top.line} → " +
                        "$pendingCoroutineResumeFile:$pendingCoroutineResumeLine",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
            if (fileOk &&
                policy.isStepOverResumeLineOvershoot(
                    top.line,
                    pendingCoroutineResumeLine,
                    topText,
                    targetText,
                )
            ) {
                coroutineStepLog(
                    "step-over overshoot ${top.line} → $pendingCoroutineResumeLine (resume line missed)",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
        }
        if (pendingCoroutineStepOut && stepOutStopStillInsideCalleeInterior(top)) {
            val callSite = pendingCoroutineResumeCallSiteLine
            val caret = when {
                pendingCoroutineResumeLine > callSite -> pendingCoroutineResumeLine
                callSite > 0 -> callSite
                pendingCoroutineResumeLine > 0 -> pendingCoroutineResumeLine
                else -> top.line
            }
            coroutineStepLog("step-out rewrite callee interior ${top.line} → $caret")
            val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
            val parentSm = stackFrames.drop(1).firstOrNull { frame ->
                classifier.isStateMachineFrame(frame.name) &&
                    callSite > 0 &&
                    frame.line == callSite &&
                    (frameFileMatchesPending(frame.file) ||
                        sourceFileBasename(frame.file) == sourceFileBasename(pendingCoroutineResumeFile))
            }
            if (parentSm != null && caret > callSite) {
                val pinnedTop = parentSm.copy(
                    line = caret,
                    file = pendingCoroutineResumeFile.ifEmpty { parentSm.file },
                )
                return listOf(pinnedTop) + stackFrames.filter { it.id != parentSm.id }
            }
            return listOf(top.copy(line = caret)) + stackFrames.drop(1)
        }
        if (pendingCoroutineResumeLine <= 1 || top.line == pendingCoroutineResumeLine) return stackFrames
        if (pendingCoroutineStepOut &&
            pendingCoroutineResumeCallSiteLine > 0 &&
            top.line == pendingCoroutineResumeCallSiteLine &&
            pendingCoroutineResumeLine > pendingCoroutineResumeCallSiteLine &&
            (frameFileMatchesPending(top.file) || top.file.isEmpty())
        ) {
            coroutineStepLog(
                "step-out rewrite call site ${top.line} → $pendingCoroutineResumeLine",
            )
            return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
        }
        if (pendingCoroutineStepOut &&
            (frameFileMatchesPending(top.file) || top.file.isEmpty())
        ) {
            val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
            val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            if (top.line <= 1 && pendingCoroutineResumeLine > 1) {
                coroutineStepLog(
                    "step-out rewrite dwarf line ${top.line} → $pendingCoroutineResumeLine",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
            val topText = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
            if (policy.isSuspendFunctionHeaderLine(topText) && pendingCoroutineResumeLine > top.line) {
                coroutineStepLog(
                    "step-out rewrite fun header ${top.line} → $pendingCoroutineResumeLine",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
            if (policy.isSuspendFunctionHeaderLine(topText) &&
                pendingCoroutineResumeCallSiteLine > 0 &&
                top.line > pendingCoroutineResumeCallSiteLine &&
                top.line != pendingCoroutineResumeCallSiteLine &&
                (frameFileMatchesPending(top.file) ||
                    sourceFileBasename(top.file) == sourceFileBasename(pendingCoroutineResumeFile))
            ) {
                val caretLine = pendingCoroutineResumeCallSiteLine
                coroutineStepLog(
                    "step-out rewrite suspend header ${top.line} → $caretLine " +
                        "(pending caret=${pendingCoroutineResumeLine})",
                )
                return listOf(top.copy(line = caretLine)) + stackFrames.drop(1)
            }
            val parentSm = classifier.isStateMachineFrame(top.name) &&
                !policy.isSameStateMachineSymbol(pendingCoroutineResumeOriginSymbol, top.name)
            val nearCallSite = kotlin.math.abs(top.line - pendingCoroutineResumeLine) <= 3
            if (parentSm && nearCallSite) {
                coroutineStepLog(
                    "step-out rewrite top frame line ${top.line} → $pendingCoroutineResumeLine for call site",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
            if (pendingLaunchBlockMachineryLine > 0 && top.line == pendingLaunchBlockMachineryLine) {
                coroutineStepLog(
                    "step-out rewrite launch machinery ${top.line} → $pendingCoroutineResumeLine",
                )
                return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
            }
        }
        val header = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isParentPrologueStop(
            top.line,
            sourceLineTextAt(top.file, top.line.coerceAtLeast(1)),
        )
        val callerReturn = pendingCallerResumeArmed &&
            pendingCoroutineResumeLine > top.line &&
            top.line > 1 &&
            pendingCoroutineResumeLine - top.line <= 2 &&
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isStateMachineFrame(top.name)
        if (!header && !callerReturn) return stackFrames
        if (!frameFileMatchesPending(top.file) && top.file.isNotEmpty()) return stackFrames
        coroutineStepLog("rewrite top frame line ${top.line} → $pendingCoroutineResumeLine for caret")
        return listOf(top.copy(line = pendingCoroutineResumeLine)) + stackFrames.drop(1)
    }

    /**
     * Filter probe / pending-resume stops before any IDE UI or extra Continue.
     *
     * JNI delivers park/`probeCoroutine*` as STOPPED without Continue. Java must resume while
     * the one-shot line is still pending. After the user line is shown (sticky), HOLD so a
     * second Continue cannot run past println.
     *
     * @return thread+frames to surface, or null when this stop is fully handled (caller returns).
     */
    private fun applyProbeOrPendingResumeSkip(
        threadId: Int,
        stackFrames: List<StackFrame>,
        reason: String,
    ): Pair<Int, List<StackFrame>>? {
        if (pendingLaunchBlockStepOut && pendingCoroutineResume) {
            trySurfaceLaunchBlockStepOutFromStack(threadId, stackFrames)?.let { return it }
        }
        val topIsProbe = isCoroutineProbeStop(stackFrames)
        val topIsGlue = isLibraryGlueStop(stackFrames)
        val action = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.decide(
            topIsProbe = topIsProbe,
            topIsGlue = topIsGlue,
            emptyStack = stackFrames.isEmpty(),
            pendingResume = pendingCoroutineResume,
            stickyUserPause = stickyUserSourcePause,
            leftoverNativeStep = leftoverNativeStepPlanLikely,
            nativeStepIntoInFlight = pendingNativeStepInto,
        )
        when (action) {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.HOLD -> {
                if (pendingCoroutineResume && stackFrames.isEmpty() && !stickyUserSourcePause) {
                    abandonPendingResumeAsRunning(threadId, "empty stack while pending resume")
                    return null
                }
                coroutineStepLog(
                    "hold sticky/pending pause — ignore ${stackFrames.firstOrNull()?.name} " +
                        "${stackFrames.firstOrNull()?.file}:${stackFrames.firstOrNull()?.line}, no Continue/UI",
                )
                return null
            }
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.QUIET_CONTINUE -> {
                if (org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isUnspecifiedStopReason(reason) &&
                    !topIsProbe &&
                    !pendingCoroutineStepOut
                ) {
                    coroutineStepLog("skip quiet-continue for unspecified stop reason=$reason")
                    return threadId to stackFrames
                }
                return quietContinueInternalStop(threadId, stackFrames)
            }
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.Action.FALLTHROUGH -> Unit
        }

        if (pendingCoroutineResume) {
            val topEarly = stackFrames.firstOrNull()
            if (pendingCoroutineStepOut && topEarly != null &&
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isUnspecifiedStopReason(reason)
            ) {
                val sameOrigin = topEarly.line == pendingCoroutineResumeOriginLine &&
                    frameFileMatchesPending(topEarly.file)
                if (sameOrigin && pendingLaunchBlockStepOut) {
                    coroutineStepPipeline(
                        "step-out launch-block still at origin ${topEarly.line} — Continue (not quiet-continue loop)",
                    )
                    isolateResumed(threadId)
                    if (continueDiscardingStepPlans(threadId)) return null
                } else if (sameOrigin) {
                    coroutineStepPipeline(
                        "step-out spurious reason=$reason at ${topEarly.file}:${topEarly.line} — quiet continue",
                    )
                    isolateResumed(threadId)
                    if (continuePendingResumeQuietly(threadId)) return null
                }
            }
            val userStop = userBreakpointStopToSurface(stackFrames, reason)
            if (userStop != null) {
                if (pendingCoroutineStepOut) {
                    val top = userStop.firstOrNull()
                    val matchesPendingCaller = top != null && pendingCoroutineResumeLine > 0 &&
                        (top.line == pendingCoroutineResumeLine ||
                            (pendingCoroutineResumeCallSiteLine > 0 &&
                                top.line == pendingCoroutineResumeCallSiteLine)) &&
                        frameFileMatchesPending(top.file)
                    if (matchesPendingCaller) {
                        val surfaced = rewriteTopFrameLineIfPendingTarget(userStop)
                        onUserOwnedBreakpointPause(
                            "step-out caller user bp ${top.file}:${top.line}",
                        )
                        coroutineStepPipeline(
                            "step-out surfaced caller at user bp ${top.file}:${top.line} ${top.name}",
                        )
                        return threadId to surfaced
                    }
                    coroutineStepLog(
                        "step-out user breakpoint ${top?.file}:${top?.line} — " +
                            "suspend and quiet-continue (keep pending)",
                    )
                    if (top != null) {
                        runCatching { breakpointHandler.suspendUserBreakpointForStep(top.file, top.line) }
                        isolateResumed(threadId)
                        if (continuePendingResumeQuietly(threadId)) return null
                    }
                } else {
                    val topUser = userStop.firstOrNull()
                    val noDwarfFallbackHit = topUser != null &&
                        pendingCoroutineResumeLine > 0 &&
                        !pendingCoroutineResumeOriginWasSuspendCall &&
                        topUser.line == pendingCoroutineResumeLine + 1 &&
                        org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                            .isValidParentResumeUserLine(
                                pendingCoroutineResumeLine,
                                sourceLineTextAt(pendingCoroutineResumeFile, pendingCoroutineResumeLine),
                            )
                    val surfaced = if (noDwarfFallbackHit) {
                        rewriteTopFrameLineIfPendingTarget(userStop)
                    } else {
                        userStop
                    }
                    coroutineStepPipeline(
                        "step-over user bp at ${topUser?.file}:${topUser?.line} " +
                            "pending=$pendingCoroutineResumeLine " +
                            "originWasSuspendCall=$pendingCoroutineResumeOriginWasSuspendCall " +
                            "noDwarfFallbackHit=$noDwarfFallbackHit",
                    )
                    coroutineStepLog(
                        "user breakpoint ${topUser?.file}:${topUser?.line} — surface (do not quiet-continue)",
                    )
                    onUserOwnedBreakpointPause(
                        "user breakpoint ${topUser?.file}:${topUser?.line}",
                    )
                    return threadId to surfaced
                }
            }
            if (pendingCoroutineStepOut) {
                val top = stackFrames.firstOrNull()
                if (top != null && tryContinuePlainCallerStepOutNativeUnwind(threadId, top)) {
                    return null
                }
                if (top != null && stepOutStopStillInsideCalleeInterior(top)) {
                    val surfaced = rewriteTopFrameLineIfPendingTarget(stackFrames)
                    val shown = surfaced.firstOrNull()
                    if (pendingCoroutineResumeCallSiteLine > 0 &&
                        shown?.line == pendingCoroutineResumeCallSiteLine
                    ) {
                        onUserOwnedBreakpointPause("step-out callee interior → call site")
                    } else {
                        clearPendingCoroutineResume()
                        leftoverNativeStepPlanLikely = false
                    }
                    coroutineStepPipeline(
                        "step-out surfaced caller (callee interior → call site) " +
                            "${shown?.file}:${shown?.line}",
                    )
                    return threadId to surfaced
                }
            }
            if (shouldSurfacePendingCoroutineResumeStop(stackFrames, reason)) {
                val top = stackFrames.firstOrNull()
                val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                if (top != null && policy.isPlainUserCalleeFrame(
                        topIsGlue = isLibraryGlueStop(stackFrames),
                        topIsStateMachine = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                            .isStateMachineFrame(top.name),
                        topIsCoroutineLambda = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                            .isCoroutineLambdaFrame(top.name),
                    ) && top.line != pendingCoroutineResumeOriginLine &&
                    !stepOverSuspendCallSpilledLocals
                ) {
                    coroutineStepLog(
                        "plain user callee ${top.file}:${top.line} ${top.name} — surface",
                    )
                    clearPendingCoroutineResume()
                    markStickyUserSourcePause("plain user callee")
                    return threadId to stackFrames
                }
                if (top != null && pendingCoroutineStepOut) {
                    val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                    val topIsSm = classifier.isStateMachineFrame(top.name)
                    val topIsLambda = classifier.isCoroutineLambdaFrame(top.name)
                    val sameSm = policy.isSameStateMachineSymbol(pendingCoroutineResumeOriginSymbol, top.name)
                    val topText = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
                    val launchHeaderMisland = policy.isCoroutineScopeLaunchHeaderLine(topText) &&
                        pendingCoroutineResumeOriginLine > top.line
                    if (launchHeaderMisland) {
                        val file = top.file.ifEmpty { pendingCoroutineResumeFile }
                        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(file, line) }
                        val chaseLine = when {
                            pendingCoroutineResumeLine > pendingCoroutineResumeOriginLine ->
                                pendingCoroutineResumeLine
                            else -> policy.resolveStepOverTarget(
                                pendingCoroutineResumeOriginLine,
                                lineTextAt,
                            ).nextLine1Based
                        }
                        if (chaseLine > pendingCoroutineResumeOriginLine && file.isNotEmpty()) {
                            pendingCoroutineResumeLine = chaseLine
                            pendingCoroutineResumeFile = file
                            coroutineStepPipeline(
                                "step-out skip launch header ${top.line} — chase $file:$chaseLine",
                            )
                            isolateResumed(threadId)
                            if (continuePendingResumeQuietly(threadId)) return null
                        }
                    }
                    val matchesTarget = pendingCoroutineResumeLine > 0 &&
                        (top.line == pendingCoroutineResumeLine ||
                            (pendingCoroutineResumeCallSiteLine > 0 &&
                                top.line == pendingCoroutineResumeCallSiteLine)) &&
                        frameFileMatchesPending(top.file)
                    val matchesMachinery = pendingLaunchBlockMachineryLine > 0 &&
                        top.line == pendingLaunchBlockMachineryLine &&
                        frameFileMatchesPending(top.file)
                    var isCallerFrame = (topIsSm || topIsLambda) && !sameSm &&
                        top.line != pendingCoroutineResumeOriginLine &&
                        (top.line > 0 || pendingCoroutineResumeLine > 0)
                    if (isCallerFrame && policy.isCoroutineScopeLaunchHeaderLine(topText) &&
                        pendingCoroutineResumeOriginLine > top.line
                    ) {
                        isCallerFrame = false
                    }
                    if (matchesTarget || matchesMachinery || isCallerFrame) {
                        val surfaced = rewriteTopFrameLineIfPendingTarget(stackFrames)
                        val shown = surfaced.firstOrNull()
                        if (pendingCoroutineResumeCallSiteLine > 0 &&
                            shown?.line == pendingCoroutineResumeCallSiteLine
                        ) {
                            onUserOwnedBreakpointPause("step-out surfaced caller")
                        } else {
                            clearPendingCoroutineResume()
                            leftoverNativeStepPlanLikely = false
                        }
                        coroutineStepPipeline(
                            "step-out surfaced caller ${shown?.file}:${shown?.line} ${shown?.name}",
                        )
                        coroutineStepLog("step-out surfaced caller ${top.file}:${top.line} ${top.name}")
                        return threadId to surfaced
                    }
                    if (topIsSm && sameSm) {
                        if (policy.isSuspendFunctionHeaderLine(topText) &&
                            pendingCoroutineResumeCallSiteLine > 0 &&
                            top.line > pendingCoroutineResumeCallSiteLine &&
                            top.line < pendingCoroutineResumeOriginLine
                        ) {
                            val surfaced = rewriteTopFrameLineIfPendingTarget(stackFrames)
                            onUserOwnedBreakpointPause("step-out surfaced caller (callee suspend header)")
                            coroutineStepPipeline(
                                "step-out surfaced caller (callee suspend header) " +
                                    "${surfaced.firstOrNull()?.file}:${surfaced.firstOrNull()?.line}",
                            )
                            return threadId to surfaced
                        }
                        coroutineStepLog(
                            "step-out skip callee ${top.file}:${top.line} — quiet continue to caller",
                        )
                    }
                } else if (top != null && pendingCoroutineResumeLastStatement) {
                    val topText = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
                    val prologue = policy.isParentPrologueStop(top.line, topText)
                    if (prologue) {
                        if (hopToNextUserSourceAfterParentResume(threadId)) {
                            return null
                        }
                        // B37 restored the callee last line (295/306) here — that is a wasted F8.
                        // KMP Android keeps stepping through synthetic restore / call-site silently.
                        coroutineStepLog(
                            "parent prologue hop missed — quiet continue to caller next statement",
                        )
                    } else {
                        val file = top.file.ifEmpty { pendingCoroutineResumeFile }
                        val advanced = policy.advancePastReturnedCallSite(
                            pendingCoroutineResumeOriginSymbol,
                            top.line,
                            topText,
                            { line -> sourceLineTextAt(file, line) },
                        )
                        if (advanced != top.line && continueToCallerUserLine(threadId, file, advanced)) {
                            return null
                        }
                        if (!policy.isReturnedCallSiteSourceLine(pendingCoroutineResumeOriginSymbol, topText) &&
                            policy.isValidParentResumeUserLine(top.line, topText)
                        ) {
                            val surfaced = rewriteTopFrameLineIfPendingTarget(stackFrames)
                            clearPendingCoroutineResume()
                            markStickyUserSourcePause("resume target")
                            return threadId to surfaced
                        }
                        coroutineStepLog(
                            "last-statement at returned call site ${top.file}:${top.line} — quiet continue",
                        )
                    }
                } else {
                    val surfaced = rewriteTopFrameLineIfPendingTarget(stackFrames)
                    clearPendingCoroutineResume()
                    markStickyUserSourcePause("resume target")
                    return threadId to surfaced
                }
            }
            if (threadId <= 0) {
                coroutineStepLog("pending skip with invalid threadId — ignore, no Continue")
                return null
            }
            val top = stackFrames.firstOrNull()
            if (!pendingCoroutineStepOut && pendingCoroutineResume && top != null) {
                val overshot = rewriteTopFrameLineIfPendingTarget(stackFrames)
                if (overshot.firstOrNull()?.line == pendingCoroutineResumeLine &&
                    top.line > pendingCoroutineResumeLine
                ) {
                    coroutineStepPipeline(
                        "step-over overshoot surfaced ${pendingCoroutineResumeFile}:" +
                            "$pendingCoroutineResumeLine (was ${top.file}:${top.line})",
                    )
                    clearPendingCoroutineResume()
                    markStickyUserSourcePause("resume target")
                    return threadId to overshot
                }
            }
            if (pendingCoroutineStepOut && pendingLaunchBlockStepOut && top != null) {
                val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                val sameSm = policy.isSameStateMachineSymbol(pendingCoroutineResumeOriginSymbol, top.name)
                val closing = policy.isClosingBraceLine(sourceLineTextAt(top.file, top.line.coerceAtLeast(1)))
                if (sameSm && top.line != pendingCoroutineResumeOriginLine && closing) {
                    coroutineStepPipeline(
                        "step-out launch-block callee closing ${top.file}:${top.line} — Continue to caller",
                    )
                    isolateResumed(threadId)
                    if (continueDiscardingStepPlans(threadId)) return null
                }
            }
            val skips = bumpResumeSkipCount(threadId)
            if (top != null && stepPastOriginUserBreakpoint(threadId, top)) {
                return null
            }
            coroutineStepLog(
                "resume skip $reason at ${top?.file}:${top?.line} name=${top?.name} — " +
                    "quiet continue #$skips (waiting for line $pendingCoroutineResumeLine)",
            )
            isolateResumed(threadId)
            if (skips > MAX_COROUTINE_RESUME_SKIPS) {
                abandonPendingResumeAsRunning(threadId, "too many resume auto-continues")
                return null
            }
            if (continuePendingResumeQuietly(threadId)) {
                return null
            }
            coroutineStepLog("quiet continue failed; restoring pause so Frames stay available")
            clearPendingCoroutineResume()
            markStickyUserSourcePause("quiet continue failed")
            isolateSuspended(threadId)
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
            return null
        }

        return threadId to stackFrames
    }

    /**
     * Skip-limit: stop chasing glue, keep the last user Frames/Variables.
     * Do **not** [markEngineRunning] — that empties Frames while probes keep QUIET_CONTINUE.
     */
    private fun abandonPendingResumeAsRunning(threadId: Int, reason: String) {
        val lastStmtNoLine = pendingCoroutineResumeLastStatement && pendingCoroutineResumeLine <= 0
        coroutineStepLog("$reason; restoring last user pause lastStmtNoLine=$lastStmtNoLine")
        leftoverNativeStepPlanLikely = lastStmtNoLine
        clearPendingCoroutineResume()
        clearStickyUserSourcePause()
        if (lastStmtNoLine && threadId > 0 && !stopped) {
            coroutineStepLog("last-statement pending line 0 — Continue to finish click callback")
            isolateResumed(threadId)
            if (continueDiscardingStepPlans(threadId)) {
                clearUserContinueOrStepPending()
                return
            }
        }
        if (threadId > 0) isolateSuspended(threadId)
        markStickyUserSourcePause("skip-limit restore last user pause")
        clearUserContinueOrStepPending()
        restoreIdePauseAfterFailedContinueOrStep(threadId)
    }

    /**
     * Park / `probeCoroutine*` / Compose glue while waiting for the one-shot resume line.
     * Always issue Continue: JNI now delivers these stops without resuming, and the Java
     * `lldbProcessRunning` hint from the previous Step is stale on this STOPPED.
     */
    private fun quietContinueInternalStop(
        threadId: Int,
        stackFrames: List<StackFrame>,
    ): Pair<Int, List<StackFrame>>? {
        // Do not walk all threads here. `getThreads(max=128)` on the park stop raced
        // waitForEvent (C05 306: listed=33, then leftover Continue past println).
        if (pendingLaunchBlockStepOut) {
            trySurfaceLaunchBlockStepOutFromStack(threadId, stackFrames)?.let { return it }
        }
        val glueName = stackFrames.firstOrNull()?.name ?: "<empty>"
        if (pendingLaunchBlockStepOut) {
            val top = stackFrames.firstOrNull()
            coroutineStepPipeline(
                "step-out launch-block glue/probe — Continue ${top?.file}:${top?.line} $glueName",
            )
        }
        val skips = bumpResumeSkipCount(threadId)
        if (skips > MAX_COROUTINE_RESUME_SKIPS) {
            abandonPendingResumeAsRunning(threadId, "too many glue/probe auto-continues; dropping pending resume")
            return null
        }
        coroutineStepLog(
            "glue/probe stop ($glueName) — quiet continue #$coroutineResumeAutoContinueCount " +
                "(JNI delivered STOPPED; Java Continue required), no Disassembly UI",
        )
        if (threadId > 0) isolateResumed(threadId)
        val continued = if (pendingLaunchBlockStepOut) {
            continueDiscardingStepPlans(threadId)
        } else {
            continuePendingResumeQuietly(threadId)
        }
        if (!continued) {
            coroutineStepLog("glue/probe quiet continue failed; waiting for next STOPPED (no UI)")
        }
        return null
    }

    private fun selectPendingResumeThread(switched: Pair<Int, List<StackFrame>>): Pair<Int, List<StackFrame>> {
        val (targetTid, targetFrames) = switched
        coroutineStepLog("glue/probe stop: switching to thread $targetTid at resume target")
        try {
            serviceWrapper.selectThread(targetTid)
            serviceWrapper.setCurrentThreadId(targetTid)
        } catch (t: Throwable) {
            coroutineStepLog("selectThread($targetTid) failed: ${t.message}")
        }
        if (shouldSurfacePendingCoroutineResumeStop(targetFrames, "breakpoint")) {
            clearPendingCoroutineResume()
            markStickyUserSourcePause("resume target on other thread")
            return targetTid to targetFrames
        }
        return targetTid to targetFrames
    }

    /**
     * Continue after dropping leftover native step plans.
     *
     * Hitting a breakpoint *during* a Step Over leaves that stepper on the thread. A plain
     * Continue then completes the plan at the coroutine epilogue (`}`) instead of running to
     * the next user/one-shot breakpoint (C05: 304 after the first `delay`, then 305).
     */
    /**
     * 真正的协程帧才跳过 Kotlin 表达式编译。普通 lambda（点击回调、`filter` / `map`）
     * 和 `$bridge-DN` 不算。从状态机或 `invoke#suspend` 调进来的普通函数仍然跳过。
     *
     * TODO: Ordinary Kotlin/Native stops keep [KotlinEvalResumeGate.ALLOW_EXPRESSION_COMPILE] at
     * `"1"` so Evaluate Expression can still run `k2native`.
     */
    private fun syncCoroutineExpressionCompile(frames: List<StackFrame>) {
        val names = frames.map { it.name }
        val skip = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
            .skipsKotlinExpressionCompile(names)
        KotlinEvalResumeGate.setExpressionCompileAllowed(!skip)
        if (skip) {
            val hit = names.firstOrNull {
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isExpressionSkipFrame(it)
            }
            coroutineStepLog("skip kotlin expression compile — $hit")
        }
    }

    private fun continueDiscardingStepPlans(threadId: Int): Boolean {
        KotlinEvalResumeGate.request()
        // C07 delay F8: leftover is already false — skip Python + 8× CLI discard (USB).
        // Keep discard only when a previous native Step Over left a thread plan on the thread.
        if (leftoverNativeStepPlanLikely) {
            coroutineStepLog("discard leftover thread plans before resume Continue")
            try {
                coroutineSupport.discardLeftoverThreadPlans()
            } catch (t: Throwable) {
                LOG.warn("discard leftover thread plans failed; continuing anyway", t)
            }
        }
        leftoverNativeStepPlanLikely = false
        lldbProcessRunning = true
        val startedNs = System.nanoTime()
        printPluginDebugConsole("[startStepOver] continue begin thread=$threadId")
        pluginDbgPrintln("[Coroutines][step] continue begin thread=$threadId")
        val continued = serviceWrapper.resumeThread(threadId, null)
        val elapsedMs = (System.nanoTime() - startedNs) / 1_000_000.0
        val elapsedText = String.format(java.util.Locale.US, "%.1f", elapsedMs)
        printPluginDebugConsole("[startStepOver] continue end ok=$continued ms=$elapsedText")
        pluginDbgPrintln(
            "[Coroutines][step] continue end ok=$continued ms=$elapsedText",
        )
        return continued
    }

    private fun discardLeftoverNativeStepPlanIfNeeded() {
        if (!leftoverNativeStepPlanLikely) return
        // todo0920 Never discard while the inferior is already running — LLDB CLI then fails and the
        // USB round-trips stall the next Step Over (continuous F8 freeze).
        if (lldbProcessRunning || stopped) {
            leftoverNativeStepPlanLikely = false
            coroutineStepLog("skip discard leftover plans (processRunning=$lldbProcessRunning stopped=$stopped)")
            return
        }
        leftoverNativeStepPlanLikely = false
        coroutineStepLog("discard leftover native step plan after user-visible stop")
        try {
            coroutineSupport.discardLeftoverThreadPlans()
        } catch (t: Throwable) {
            LOG.warn("discard leftover native step plan failed", t)
        }
    }



    /** Thread list from the last STOPPED event; used to re-print debug output when toggling Show All Threads. */
    @Volatile private var lastStoppedAllThreads: Array<org.cpf.kmp.tools.ohos.debugger.jni.ThreadInfo>? = null

    /** Last STOP stack preview was capped in waitForEvent; full stack loaded lazily via getStackTrace. */
    @Volatile private var lastStopStackTruncated: Boolean = false
    @Volatile private var lastStopTotalFrameCount: Int = 0

    private val fullStackLoadLock = Any()
    @Volatile private var fullStackLoadInFlight: Boolean = false
    private val fullStackLoadWaiters = mutableListOf<(List<StackFrame>?) -> Unit>()

    internal fun rememberStoppedAllThreads(threads: Array<org.cpf.kmp.tools.ohos.debugger.jni.ThreadInfo>?) {
        lastStoppedAllThreads = threads
    }

    /**
     * Fetch and print thread list (preview or full per [showAllThreadsInList]).
     * @param includeConsole when true, emit the same detail block to the debug session Console
     *   (used when the user toggles Show All Threads); stop path keeps this false to avoid spam.
     */
    internal fun refreshThreadListDebugOutput(includeConsole: Boolean = false) {
        if (!isSuspendToolbarActionsEnabled()) return
        ApplicationManager.getApplication().executeOnPooledThread {
            if (!isSuspendToolbarActionsEnabled()) return@executeOnPooledThread
            val threads = fetchThreadListForDebugOutput()
            if (threads.isEmpty()) return@executeOnPooledThread
            val total = serviceWrapper.getThreadCount()
            rememberStoppedAllThreads(threads)
            val console = if (includeConsole) session.consoleView else null
            ApplicationManager.getApplication().invokeLater {
                if (!isSuspendToolbarActionsEnabled()) return@invokeLater
                logLldbThreadList(
                    threads,
                    showAllThreadsInList,
                    totalThreadCount = total,
                    console = console,
                )
            }
        }
    }

    internal fun getCachedThreadListSize(): Int = lastStoppedAllThreads?.size ?: 0

    internal fun setLastStopStackTruncation(truncated: Boolean, totalFrameCount: Int) {
        lastStopStackTruncated = truncated
        lastStopTotalFrameCount = totalFrameCount
        synchronized(fullStackLoadLock) {
            fullStackLoadInFlight = false
            fullStackLoadWaiters.clear()
        }
    }

    internal fun getLastSuspendedStackFrames(): List<StackFrame>? = lastSuspendedStackFrames

    internal fun getLastSuspendedThreadId(): Int? = lastSuspendedThreadId

    /**
     * Fetching 200+ Compose frames over JNI after a missed coroutine resume freezes Frames on
     * "Loading..." and Variables on "Collecting data...". Skip the lazy walk when the stop is
     * already library glue (it should have been auto-continued; this is a safety net), and also
     * when the truncated preview already has a user / state-machine top (C05 last-statement hop).
     */
    internal fun shouldSkipLazyFullStackLoad(preview: List<StackFrame>): Boolean {
        val top = preview.firstOrNull()
        val skip = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.shouldSkipLazyFullStackLoad(
            pendingResume = pendingCoroutineResume,
            stickyUserPause = stickyUserSourcePause,
            topIsGlue = top != null && isLibraryGlueFrame(top.name),
            topIsStateMachine = top != null &&
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isStateMachineFrame(top.name),
            topHasUserSource = top != null && LLDBStackFrameFilter.hasUserSourcePath(top.file),
        )
        if (skip) {
            coroutineStepLog("skip full-stack load — pending/sticky/SM/user preview, use stop preview")
        }
        return skip
    }

    /** Last stop whose top frame mapped to Kotlin/project source; gutter anchor while stepping glue in Disassembly. */
    internal fun getLastKnownMappableIdeSource(): XSourcePosition? = lastKnownMappableIdeSource

    internal fun isLastStopStackTruncated(): Boolean = lastStopStackTruncated

    internal fun getLastStopTotalFrameCount(): Int = lastStopTotalFrameCount

    /** Filled asynchronously after STOP when native preview omitted [GetNumFrames]. */
    internal fun updateLastStopTotalFrameCount(totalFrameCount: Int) {
        if (totalFrameCount > 0) {
            lastStopTotalFrameCount = totalFrameCount
        }
    }

    /** e.g. `8/64`, `8+` when total not yet resolved, or `5` when not truncated. */
    internal fun formatStopStackPreviewLabel(shown: Int): String {
        if (!lastStopStackTruncated) {
            return shown.toString()
        }
        val total = lastStopTotalFrameCount
        return if (total > shown) "$shown/$total" else "$shown+"
    }

    /**
     * Loads the full stack via JNI [getStackTrace] when the STOP preview was truncated.
     * Safe to call repeatedly; concurrent callers share one in-flight load.
     */
    internal fun loadFullStackIfTruncatedAsync(callback: (List<StackFrame>?) -> Unit) {
        if (!lastStopStackTruncated) {
            callback(lastSuspendedStackFrames)
            return
        }
        val tid = lastSuspendedThreadId
        if (tid == null || tid <= 0) {
            callback(lastSuspendedStackFrames)
            return
        }
        var startLoad = false
        synchronized(fullStackLoadLock) {
            fullStackLoadWaiters.add(callback)
            if (!fullStackLoadInFlight) {
                fullStackLoadInFlight = true
                startLoad = true
            }
        }
        if (!startLoad) {
            return
        }
        ApplicationManager.getApplication().executeOnPooledThread {
            if (stopped || session.project.isDisposed) {
                finishFullStackLoad(null)
                return@executeOnPooledThread
            }
            serviceWrapper.getStackTrace(tid) { full ->
                if (full.isEmpty()) {
                    finishFullStackLoad(lastSuspendedStackFrames)
                    return@getStackTrace
                }
                ApplicationManager.getApplication().invokeLater {
                    if (stopped || session.project.isDisposed) {
                        finishFullStackLoad(null)
                        return@invokeLater
                    }
                    val previewSize = lastSuspendedStackFrames?.size ?: 0
                    if (full.size > previewSize) {
                        rememberSuspendedStack(tid, full, lastTopFrameIdeSourceOverride)
                        lastStopStackTruncated = false
                        pluginDbgPrintln(
                            "[loadFullStack] lazy frames=${full.size} (preview was $previewSize)",
                        )
                    }
                    finishFullStackLoad(lastSuspendedStackFrames)
                }
            }
        }
    }

    private fun finishFullStackLoad(result: List<StackFrame>?) {
        val waiters: List<(List<StackFrame>?) -> Unit>
        synchronized(fullStackLoadLock) {
            fullStackLoadInFlight = false
            waiters = fullStackLoadWaiters.toList()
            fullStackLoadWaiters.clear()
        }
        for (cb in waiters) {
            cb(result)
        }
    }

    private fun fetchThreadListForDebugOutput(): Array<org.cpf.kmp.tools.ohos.debugger.jni.ThreadInfo> {
        val list = if (showAllThreadsInList) {
            serviceWrapper.getThreadList()
        } else {
            serviceWrapper.getThreadListPreview(THREAD_LIST_PREVIEW_COUNT)
        }
        return list.toTypedArray()
    }

    /** True after [stop] has been entered; skip JNI / UI updates while tearing down. */
    internal fun isEngineStopped(): Boolean = stopped

    /**
     * Notify the session that execution suspended. Prefer `positionReached(context, attract)` on newer IDEs;
     * fall back to single-arg on older platforms (compile baseline 231).
     *
     * @param attract When false, avoid IDE async navigation to source (used with [focusDisassemblyEditor] so
     *   library/synthetic stops do not ping-pong away from the Disassembly tab).
     */
    private fun positionReachedNotifySuspend(context: XSuspendContext, attract: Boolean = true) {
        if (restoreBreakpointsAfterNativeCallSiteStepOut) {
            restoreBreakpointsAfterNativeCallSiteStepOut = false
            runCatching { breakpointHandler.restoreStepSuspendedBreakpoints() }
        }
        val m = positionReachedAttractMethod
        if (m != null) {
            try {
                m.invoke(session, context, attract)
                return
            } catch (e: Throwable) {
                LOG.warn("positionReached(suspendContext, $attract) failed, using single-arg", e)
            }
        }
        session.positionReached(context)
    }

    /** User clicked Stop or project disposed — do not run further attach / JNI startup steps. */
    private fun attachStartupAborted(): Boolean =
        stopped || session.project.isDisposed

    private fun startPleaseWaitConsoleAnimation() {
        val cv = session.consoleView ?: return
        pleaseWaitAnimationActive = true
        pleaseWaitDotPhase = 0
        pleaseWaitStartMillis = System.currentTimeMillis()
        pleaseWaitAlarm.cancelAllRequests()
        try {
            cv.print(
                "\rPlease wait${" ".repeat(PLEASE_WAIT_DOTS_FIELD_WIDTH)} (0s)".padEnd(PLEASE_WAIT_LINE_PAD),
                ConsoleViewContentType.SYSTEM_OUTPUT
            )
        } catch (_: Throwable) {
        }
        pleaseWaitAlarm.addRequest(pleaseWaitAnimator, PLEASE_WAIT_DOT_INTERVAL_MS)
    }

    /**
     * Ends the animated "Please wait" line (cancels timer, newline so following output is aligned).
     * Safe to call multiple times.
     */
    private fun stopPleaseWaitConsoleAnimation() {
        if (!pleaseWaitAnimationActive) return
        pleaseWaitAnimationActive = false
        pleaseWaitAlarm.cancelAllRequests()
        try {
            session.consoleView?.print("\n", ConsoleViewContentType.SYSTEM_OUTPUT)
        } catch (_: Throwable) {
        }
    }

    init {
        serviceWrapper.listener = listener

        listener.onStopped = { threadId, reason, stackFrames ->
            handleStopped(threadId, reason, stackFrames)
        }

        listener.onOutput = { message ->
            LOG.info("[LLDB output] $message")
        }

        listener.onTerminated = {
            endRunLikeToolbarStopIfNeededThenStopSession()
        }

        session.addSessionListener(executionGutterHighlighter)
        session.addSessionListener(variablesScrollPrefetchListener)

        registerIdeBreakpointListeners()

        LOG.info("LLDBDebugProcess initialized (JNI mode)")
    }

    @Suppress("UNCHECKED_CAST")
    private fun registerIdeBreakpointListeners() {
        val bm = XDebuggerManager.getInstance(session.project).breakpointManager
        val types = XDEBUG_BREAKPOINT_TYPE_EP.extensionList
        for (typeInstance in types) {
            if (typeInstance !is KotlinLineBreakpointType) continue
            bm.addBreakpointListener(
                typeInstance as XBreakpointType<XLineBreakpoint<XBreakpointProperties<*>>, XBreakpointProperties<*>>,
                lineBreakpointIdeListener
            )
        }
        if (OhosKmpCrossLanguageDebug.isEnabled()) {
            bm.addBreakpointListener(
                ArkTsLineBreakpointType.INSTANCE as XBreakpointType<XLineBreakpoint<XBreakpointProperties<*>>, XBreakpointProperties<*>>,
                arkTsLineBreakpointIdeListener,
            )
        }
    }

    @Suppress("UNCHECKED_CAST")
    private fun unregisterIdeBreakpointListeners() {
        if (session.project.isDisposed) return
        val bm = XDebuggerManager.getInstance(session.project).breakpointManager
        val types = XDEBUG_BREAKPOINT_TYPE_EP.extensionList
        for (typeInstance in types) {
            if (typeInstance !is KotlinLineBreakpointType) continue
            bm.removeBreakpointListener(
                typeInstance as XBreakpointType<XLineBreakpoint<XBreakpointProperties<*>>, XBreakpointProperties<*>>,
                lineBreakpointIdeListener
            )
        }
        if (OhosKmpCrossLanguageDebug.isEnabled()) {
            bm.removeBreakpointListener(
                ArkTsLineBreakpointType.INSTANCE as XBreakpointType<XLineBreakpoint<XBreakpointProperties<*>>, XBreakpointProperties<*>>,
                arkTsLineBreakpointIdeListener,
            )
        }
    }

    /**
     * Target process exited/crashed (LLDB event). Match the red Stop button when the deployment
     * [ProcessHandler] is still active (e.g. script exited 0 without [ProcessHandler.notifyProcessTerminated]).
     * If the user already clicked Stop, [ProcessHandler.isProcessTerminated] is true — skip [destroyProcess]
     * so we do not interfere with that path.
     */
    private fun endRunLikeToolbarStopIfNeededThenStopSession() {
        try {
            val handler = processHandler
            if (handler != null && !handler.isProcessTerminated) {
                handler.destroyProcess()
            }
        } catch (e: Throwable) {
            LOG.warn("endRunLikeToolbarStopIfNeededThenStopSession: ${e.message}")
        }
        session.stop()
    }

    private fun rememberSuspendedStack(
        threadId: Int,
        stackFrames: List<StackFrame>,
        topFrameIdeSource: XSourcePosition? = null,
        reason: String? = null,
    ) {
        lastSuspendedThreadId = threadId
        lastSuspendedStackFrames = stackFrames
        lastTopFrameIdeSourceOverride = topFrameIdeSource
        notifyCoroutinesDebugTab(stackFrames, reason)
    }

    private fun clearSuspendedStackCache() {
        lastSuspendedThreadId = null
        lastSuspendedStackFrames = null
        lastTopFrameIdeSourceOverride = null
        lastKnownMappableIdeSource = null
    }

    private data class StopPresentationPlan(
        val executionHighlightSource: XSourcePosition?,
        val preferDisassembly: Boolean,
        val glueInstructionStepping: Boolean,
        val steppedAwayFromUserCode: Boolean,
    )

    /**
     * Decide Disassembly vs source editor and which line gets the execution marker.
     * While stepping cinterop/K/N glue in Disassembly, keep the yellow arrow on [lastKnownMappableIdeSource]
     * (same idea as VS / Android Studio when native frames have no source).
     * When instruction stepping escapes into the app event loop (e.g. epoll_wait), restore the source editor.
     */
    private fun planStopPresentation(
        reason: String,
        stackFrames: List<StackFrame>,
        hitBreakpoint: com.intellij.xdebugger.breakpoints.XLineBreakpoint<*>?,
        nativeHandoff: Boolean,
        nativeCatchStop: Boolean,
    ): StopPresentationPlan {
        val topSource = resolveTopFrameIdeSource(stackFrames, hitBreakpoint)
        if (topSource != null) {
            lastKnownMappableIdeSource = topSource
        }
        val steppedAway = hasSteppedAwayFromUserDebugContext(stackFrames)
        val top = stackFrames.firstOrNull()
        val topMappable = top != null && frameHasMappableIdeSource(top)
        if (!topMappable && lastKnownMappableIdeSource != null &&
            reason.equals("step", ignoreCase = true) &&
            (stickyUserSourcePause || pendingCoroutineResume || pendingLaunchBlockStepOut)
        ) {
            val topHighlight = top?.let { resolveTopFrameIdeSource(listOf(it), hitBreakpoint) }
            if (topHighlight != null) {
                lastKnownMappableIdeSource = topHighlight
                return StopPresentationPlan(
                    executionHighlightSource = topHighlight,
                    preferDisassembly = false,
                    glueInstructionStepping = false,
                    steppedAwayFromUserCode = false,
                )
            }
            return StopPresentationPlan(
                executionHighlightSource = lastKnownMappableIdeSource,
                preferDisassembly = false,
                glueInstructionStepping = false,
                steppedAwayFromUserCode = false,
            )
        }
        val topName = top?.name.orEmpty()
        val kotlinNativeCatch = nativeCatchStop && topName.startsWith("kfun:kotlin.")
        val preferDisasmRaw = shouldPreferDisassemblyOnStop(reason, stackFrames) &&
            !nativeHandoff && (!nativeCatchStop || kotlinNativeCatch)
        val preferDisassembly = preferDisasmRaw && !steppedAway
        // Only the "escaped to runtime" case needs to pull the editor back to source. For a glue
        // (no-source) stop, pinning the execution point on the previous Kotlin line makes XDebugger
        // select that source editor, so the Disassembly tab can never become current
        // (log: "Cannot focus editor ... currentComposite=Stepsc1Inner.kt").
        val highlight = topSource ?: when {
            steppedAway -> lastKnownMappableIdeSource
            else -> null
        }
        return StopPresentationPlan(
            executionHighlightSource = highlight,
            preferDisassembly = preferDisassembly,
            glueInstructionStepping = preferDisassembly,
            steppedAwayFromUserCode = steppedAway,
        )
    }

    /** Still inside Kotlin/Native bridge / user SO — OK to stay in Disassembly + nexti. */
    private fun stackStillInKotlinNativeBridge(stackFrames: List<StackFrame>): Boolean {
        return stackFrames.take(24).any { frame ->
            frameHasMappableIdeSource(frame) || isLikelyKotlinRuntimeGlue(frame.name) ||
                isKnCinteropExportGlue(frame.name)
        }
    }

    private fun isKnCinteropExportGlue(functionName: String): Boolean {
        val n = functionName.trim()
        if (n.isEmpty() || n == "unknown") return false
        if (n.endsWith("_trigger", ignoreCase = true)) return true
        if (n.contains("N2K", ignoreCase = true) || n.contains("K2N", ignoreCase = true)) return true
        return false
    }

    /** App main-loop / OHOS framework idle — instruction step likely left the debugged call path. */
    private fun isRuntimeSchedulerOrIdleFrame(functionName: String): Boolean {
        val n = functionName.trim().lowercase()
        if (n.isEmpty() || n == "unknown") return false
        if (n.contains("epoll_wait") || n == "ioctl" || n.contains("poll") || n.contains("select")) return true
        if (n.startsWith("ohos::") || n.startsWith("std::__")) return true
        if (n.contains("eventqueue") || n.contains("epolliowaiter")) return true
        return false
    }

    private fun hasSteppedAwayFromUserDebugContext(stackFrames: List<StackFrame>): Boolean {
        if (stackFrames.isEmpty()) return false
        val top = stackFrames[0]
        if (frameHasMappableIdeSource(top)) return false
        if (isLikelyKotlinRuntimeGlue(top.name) || isKnCinteropExportGlue(top.name)) return false
        if (stackStillInKotlinNativeBridge(stackFrames)) return false
        return isRuntimeSchedulerOrIdleFrame(top.name)
    }

    private fun notifySteppedAwayFromUserCode(topFrame: StackFrame, highlight: XSourcePosition?) {
        val loc = highlight?.let { pos ->
            "${pos.file.name}:${pos.line + 1}"
        } ?: "last Kotlin source line"
        pluginDbgPrintln(
            "[handleStopped] stepped away from K/N glue into runtime (top=${topFrame.name}); " +
                "restoring editor to $loc",
        )
        try {
            session.consoleView?.print(
                "Harmony LLDB: Instruction step left Kotlin/cinterop glue (now at ${topFrame.name}). " +
                    "Editor restored to $loc — use Run to Line or a breakpoint to continue debugging.\n",
                ConsoleViewContentType.SYSTEM_OUTPUT,
            )
        } catch (_: Throwable) {
        }
    }

    private fun tryBeginUserContinueOrStep(): Boolean {
        synchronized(userContinueOrStepLock) {
            if (stopped) {
                CrossLangTrace.log(session, "tryBeginContinueOrStep BLOCKED: engine stopped", error = true)
                return false
            }
            if (userContinueOrStepPending) {
                pluginDbgPrintln("[execution] skip duplicate Continue/Step (previous JNI not finished)")
                CrossLangTrace.log(session, "tryBeginContinueOrStep BLOCKED: previous step/continue pending", error = true)
                return false
            }
            if (crossLanguageCoordinator?.isArkTsSuspended() == true) {
                userContinueOrStepPending = true
                CrossLangTrace.log(session, "tryBeginContinueOrStep OK (ArkTS suspended)")
                return true
            }
            val tid = getCurrentThreadId()
            if (tid == null) {
                CrossLangTrace.log(session, "tryBeginContinueOrStep BLOCKED: no current thread", error = true)
                return false
            }
            if (!isThreadSuspended(tid)) {
                CrossLangTrace.log(
                    session,
                    "tryBeginContinueOrStep BLOCKED: thread $tid not suspended (LLDB path)",
                    error = true,
                )
                return false
            }
            userContinueOrStepPending = true
            CrossLangTrace.log(session, "tryBeginContinueOrStep OK (LLDB thread $tid suspended)")
            return true
        }
    }

    private fun clearUserContinueOrStepPending() {
        synchronized(userContinueOrStepLock) {
            userContinueOrStepPending = false
        }
        // Do not clear [skipCalleeHeaderOnNextStepInto] here — LLDBListener clears pending before
        // handleStopped; the flag must survive until [applyStepIntoCalleeHeaderPresentation].
    }

    private fun clearPendingNativeStepInto() {
        pendingNativeStepInto = false
        stepIntoOriginLine1Based = 0
        stepIntoOriginFile = ""
    }

    private fun frameFileMatchesStepIntoOrigin(file: String): Boolean {
        if (stepIntoOriginFile.isEmpty()) return false
        if (file.isEmpty()) return false
        val expected = stepIntoOriginFile.substringAfterLast('/').substringAfterLast('\\')
        val actual = file.substringAfterLast('/').substringAfterLast('\\')
        return expected.equals(actual, ignoreCase = true) ||
            file == stepIntoOriginFile ||
            stepIntoOriginFile.endsWith(file)
    }

    private fun applyStepIntoEpilogueLineRewrite(
        stackFrames: List<StackFrame>,
        reason: String,
    ): List<StackFrame> {
        if (!pendingNativeStepInto) return stackFrames
        if (!reason.equals("step", ignoreCase = true)) {
            clearPendingNativeStepInto()
            return stackFrames
        }
        val top = stackFrames.firstOrNull() ?: return stackFrames
        if (stepIntoOriginLine1Based <= 0 || !frameFileMatchesStepIntoOrigin(top.file)) {
            return stackFrames
        }
        val landedText = sourceLineTextAt(top.file, top.line.coerceAtLeast(1))
        val originText = sourceLineTextAt(stepIntoOriginFile, stepIntoOriginLine1Based)
        val unwind = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector.isSuspendUnwind(
            stepIntoOriginLine1Based,
            originText,
            top.line,
            landedText,
        )
        if (!unwind) {
            return stackFrames
        }
        val next = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            .resolveStepOverTarget(stepIntoOriginLine1Based) { line ->
                sourceLineTextAt(stepIntoOriginFile, line)
            }.nextLine1Based
        clearPendingNativeStepInto()
        if (next <= stepIntoOriginLine1Based) return stackFrames
        pluginDbgPrintln(
            "[stepInto] suspend unwind epilogue ${top.line} → next statement $next " +
                "(origin=$stepIntoOriginLine1Based)",
        )
        coroutineStepLog("step-into suspend unwind ${top.line} → $next")
        return listOf(top.copy(line = next)) + stackFrames.drop(1)
    }

    private fun applyStepIntoCalleeHeaderPresentation(
        stackFrames: List<StackFrame>,
        reason: String,
    ): Pair<List<StackFrame>, XSourcePosition?> {
        if (!skipCalleeHeaderOnNextStepInto) return stackFrames to null
        if (!reason.equals("step", ignoreCase = true)) {
            skipCalleeHeaderOnNextStepInto = false
            return stackFrames to null
        }
        skipCalleeHeaderOnNextStepInto = false
        val top = stackFrames.firstOrNull() ?: return stackFrames to null
        if (top.line <= 0 || top.file.isEmpty()) return stackFrames to null
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(top.file, line) }
        val bodyLine = policy.firstBodyLineAfterSuspendFunctionHeader(top.line, lineTextAt)
        if (bodyLine <= top.line) return stackFrames to null
        val adjusted = listOf(top.copy(line = bodyLine)) + stackFrames.drop(1)
        val pos = positionMapper.parseSourcePosition(top.file, bodyLine)
        clearPendingNativeStepInto()
        pluginDbgPrintln("[stepInto] skip callee header ${top.line} → $bodyLine (first body line)")
        coroutineStepLog("step-into skip callee header ${top.line} → $bodyLine")
        return adjusted to pos
    }

    /**
     * JNI continue/step failed after the IDE may have switched to “running”; restore paused UI from cached stack.
     */
    private fun restoreIdePauseAfterFailedContinueOrStep(threadId: Int) {
        executionGutterHighlighter.clearStepPreview()
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        val app = ApplicationManager.getApplication()
        val restore = Runnable {
            if (stopped || session.project.isDisposed) return@Runnable
            isolateSuspended(threadId)
            val frames = lastSuspendedStackFrames
            val tid = lastSuspendedThreadId ?: threadId
            if (frames != null && frames.isNotEmpty()) {
                try {
                    session.positionReached(LLDBSuspendContext(this, tid, frames, lastTopFrameIdeSourceOverride))
                } catch (e: Exception) {
                    LOG.warn("restoreIdePauseAfterFailedContinueOrStep: ${e.message}")
                }
            } else {
                try {
                    session.rebuildViews()
                } catch (_: Throwable) {
                }
            }
        }
        if (app.isDispatchThread) {
            restore.run()
        } else {
            app.invokeLater(restore)
        }
    }

    /**
     * Called when the user toggles Hide Library/External Frames: re-invokes [XDebugSession.positionReached] with cached stack so Frames reflects the new filter.
     * When the preview was truncated, loads the full stack first so filtering applies to all frames.
     */
    fun requestFramesListRefresh() {
        if (stopped || session.project.isDisposed || !isSuspendToolbarActionsEnabled()) return
        if (lastStopStackTruncated) {
            loadFullStackIfTruncatedAsync {
                ApplicationManager.getApplication().invokeLater {
                    refreshFramesListAfterFullStackReady()
                }
            }
            return
        }
        refreshFramesListAfterFullStackReady()
    }

    private fun refreshFramesListAfterFullStackReady() {
        if (stopped || session.project.isDisposed || !isSuspendToolbarActionsEnabled()) return
        val tid = lastSuspendedThreadId ?: return
        val frames = lastSuspendedStackFrames ?: return
        if (frames.isEmpty()) return
        try {
            session.rebuildViews()
        } catch (_: Throwable) {
        }
        try {
            val suspendContext = crossLanguageCoordinator?.wrapLldbSuspendContext(
                tid,
                frames,
                lastTopFrameIdeSourceOverride,
            ) ?: LLDBSuspendContext(this, tid, frames, lastTopFrameIdeSourceOverride)
            session.positionReached(suspendContext)
        } catch (e: Throwable) {
            LOG.warn("requestFramesListRefresh: ${e.message}")
        }
    }

    override fun sessionInitialized() {
        registerLLDBTerminalTab()
        prepareDisassemblyEditorTab()

        // In split debugger mode (2026.1+), delaying this flag can keep toolbar actions disabled
        // after a valid suspend due to frontend/backend state races. Keep pause supported by default.
        try {
            session.setPauseActionSupported(true)
        } catch (_: Throwable) {
        }

        val deviceId = environmentVariables["HDC_UTID"] ?: "unknown"
        session.consoleView?.print(
            "Activating Activity for debug on device $deviceId\n",
            com.intellij.execution.ui.ConsoleViewContentType.SYSTEM_OUTPUT
        )
        startPleaseWaitConsoleAnimation()

        LOG.info("sessionInitialized: executablePath=$executablePath")

        if (executablePath == "0") {
            LOG.info("Deferred start: waiting for external startAttachSequence()")
            return
        }

        ApplicationManager.getApplication().executeOnPooledThread {
            Thread.sleep(50)

            if (attachStartupAborted()) {
                pluginDbgPrintln("[sessionInitialized] attach startup aborted before sequence (user stopped or project disposed)")
                return@executeOnPooledThread
            }

            if (isAttachMode(executablePath)) {
                startAttachSequence(executablePath)
            } else {
                LOG.error("Unsupported debug mode: $executablePath")
                session.stop()
            }
        }
    }

    private fun isAttachMode(path: String): Boolean {
        if (path.toIntOrNull() != null) return true
        if (path.startsWith("attach:") || path.startsWith("pid:")) return true
        if (path.contains(".") && !path.contains("/") && !java.io.File(path).exists()) return true
        return false
    }

    /**
     * Attach debug sequence (JNI).
     *
     * Flow:
     * 1. initDebugger
     * 2. selectPlatform → connectPlatform → setExecSearchPath
     * 3. attachToProcess
     * 4. load Kotlin script (after attach succeeds)
     * 5. sync breakpoints → continue → start event loop
     */
    fun startAttachSequence(processId: String) {
        if (attachStartupAborted()) {
            pluginDbgPrintln("[startAttachSequence] aborted before start (user stopped during startup)")
            return
        }
        LOG.info("=== Starting attach debug sequence (JNI) ===")
        pluginDbgPrintln("[startAttachSequence] target process: $processId")
        pendingAttachProcessId = processId
        beginAttachPipelinePreambleIfNeeded()
        tryStartPlatformAttachIfPending()
    }

    /**
     * ProcessLibs mirror + source-map warmup + LLDB JNI init, overlapped with device lldb-server push / hdc fport prewarm.
     */
    private fun beginAttachPipelinePreambleIfNeeded() {
        synchronized(attachPreambleLock) {
            if (attachPreambleScheduled) return
            attachPreambleScheduled = true
        }
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER IDE: LLDB attach preamble scheduled " +
                "(ProcessLibs + source-map warmup + JNI — parallel with device bridge)",
        )
        ApplicationManager.getApplication().executeOnPooledThread {
            try {
                if (attachStartupAborted()) return@executeOnPooledThread
                if (!runProcessLibsMirrorBeforeAttach()) {
                    pluginDbgPrintln("[attach-preamble] ⚠ continuing despite ProcessLibs failure")
                }
                if (attachStartupAborted()) return@executeOnPooledThread
                startSourceJarMapWarmup(getExecSearchPaths())
                serviceWrapper.onConnected = onConn@{
                    if (attachStartupAborted()) {
                        pluginDbgPrintln("[onConnected] aborted — skip platform/attach (user stopped during LLDB init)")
                        return@onConn
                    }
                    LOG.info("LLDB JNI initialized")
                    tryStartPlatformAttachIfPending()
                }
                serviceWrapper.start(environmentVariables)
            } finally {
                attachPreambleFinished = true
                tryStartPlatformAttachIfPending()
            }
        }
    }

    private fun tryStartPlatformAttachIfPending() {
        val processId = pendingAttachProcessId ?: return
        if (platformAttachChainStarted) return
        if (!serviceWrapper.isConnected()) return
        synchronized(attachPreambleLock) {
            if (platformAttachChainStarted) return
            platformAttachChainStarted = true
        }
        configureHarmonyPlatformAndAttach(processId)
    }

    /**
     * Configure Harmony platform and attach (JNI).
     *
     * Replaces the previous four CLI steps with direct JNI calls.
     */
    private fun configureHarmonyPlatformAndAttach(processId: String) {
        if (attachStartupAborted()) {
            pluginDbgPrintln("[configureHarmonyPlatformAndAttach] aborted at entry")
            return
        }
        LOG.info("=== Configure Harmony platform and attach (JNI) ===")
        pluginDbgPrintln("\n========== [Attach] platform setup (JNI) ==========")
        // 0411 added
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: JNI platform + attach (4 steps: select → connect → exec path → attach)",
        )

        val execSearchPaths = getExecSearchPaths()

        // Step 1: platform select
        pluginDbgPrintln("[Attach] [1/4] platform select remote-ohos")
        val selectOk = serviceWrapper.selectPlatform("remote-ohos")
        if (!selectOk) {
            LOG.error("platform select failed")
            pluginDbgPrintln("[Attach] ✗ platform select failed")
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [1/4] platform select FAILED",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            session.stop()
            return
        }
        pluginDbgPrintln("[Attach] ✓ platform select OK")
        // 0411 added
        session.printHarmonyDebugPipelineLine("$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [1/4] platform select OK (remote-ohos)")
        if (attachStartupAborted()) {
            pluginDbgPrintln("[Attach] aborted after platform select")
            return
        }

        // Step 2: platform connect
        pluginDbgPrintln("[Attach] [2/4] platform connect")
        val connectOk = serviceWrapper.connectPlatform("unix-abstract-connect:///lldb-server/platform.sock")
        if (!connectOk) {
            LOG.error("platform connect failed")
            pluginDbgPrintln("[Attach] ✗ platform connect failed")
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [2/4] platform connect FAILED (lldb-server unix socket)",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            session.stop()
            return
        }
        pluginDbgPrintln("[Attach] ✓ platform connect OK")
        // 0411 added
        session.printHarmonyDebugPipelineLine("$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [2/4] platform connect OK (unix-abstract lldb-server)")
        if (attachStartupAborted()) {
            pluginDbgPrintln("[Attach] aborted after platform connect")
            return
        }

        // Step 3: replace exec-search-paths for this session (clear then append each dir).
        // JVM skips SBDebugger::Terminate, so without clear the previous project's path remains.
        pluginDbgPrintln("[Attach] [3/4] clear + setExecSearchPath (${execSearchPaths.size} dir(s)):")
        execSearchPaths.forEachIndexed { index, path ->
            pluginDbgPrintln("[Attach]   [$index] $path")
        }
        warnIfExecSearchPathsIncomplete(execSearchPaths)
        serviceWrapper.handleInterpreterCommand("settings clear target.exec-search-paths")
        serviceWrapper.handleInterpreterCommand("settings clear target.source-map")
        // LLDB default (true) makes ThreadPlanStepInRange step *out* of a callee with no DWARF line
        // info instead of stopping in it (ThreadPlanShouldStopHere.eStepInAvoidNoDebug + its
        // DefaultShouldStopHereCallback). A cinterop `@CName` export such as `stepsc2_trigger` is a
        // sourceless stub, so F7 fell through to the caller's next line instead of Disassembly.
        pluginDbgPrintln("[Attach] set target.process.thread.step-in-avoid-nodebug=false (step into sourceless callees → Disassembly)")
        serviceWrapper.handleInterpreterCommand("settings set target.process.thread.step-in-avoid-nodebug false")
        execSearchPaths.forEach { serviceWrapper.setExecSearchPath(it) }
        pluginDbgPrintln("[Attach] ✓ setExecSearchPath done")
        // 0411 added
        session.printHarmonyDebugPipelineLine("$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [3/4] setExecSearchPath OK")
        if (attachStartupAborted()) {
            pluginDbgPrintln("[Attach] aborted after setExecSearchPath")
            return
        }

        // Step 4: attach
        val pid = processId.removePrefix("attach:").removePrefix("pid:").trim().toIntOrNull()
        if (pid == null) {
            LOG.error("Invalid PID: $processId")
            pluginDbgPrintln("[Attach] ✗ invalid PID: $processId")
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: invalid PID \"$processId\"",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            session.stop()
            return
        }

        pluginDbgPrintln("[Attach] [4/4] attach PID=$pid")
        // 0411 added
        session.printHarmonyDebugPipelineLine("$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [4/4] attachToProcess(pid=$pid)")

        serviceWrapper.attachToProcess(pid) { success ->
            if (attachStartupAborted()) {
                pluginDbgPrintln("[Attach] aborted in attach callback — skip script/breakpoints/continue")
                stopPleaseWaitConsoleAnimation()
                return@attachToProcess
            }
            if (!success) {
                LOG.error("Attach failed")
                pluginDbgPrintln("[Attach] ✗ Attach failed")
                val err = serviceWrapper.lastAttachErrorMessage().trim().ifBlank { "(see IDE log: Attach failed)" }
                stopPleaseWaitConsoleAnimation()
                session.consoleView?.print(
                    "Failed to attach to process $pid: $err\n",
                    com.intellij.execution.ui.ConsoleViewContentType.ERROR_OUTPUT
                )
                session.stop()
                return@attachToProcess
            }

            pluginDbgPrintln("[Attach] ✓ Attach OK")
            // lldb-server's first software-breakpoint stop takes seconds. Pay it now,
            // while we are still stopped and the event loop has not started, then
            // delete the one-shot so clicks are not stuck.
            primeSoftwareBreakpointBeforeUserCanStep()
            armLibentryLoadWarmupIfDebug()

            if (attachStartupAborted()) {
                pluginDbgPrintln("[Attach] aborted after attach success — skip script/breakpoints/continue")
                stopPleaseWaitConsoleAnimation()
                return@attachToProcess
            }

            stopPleaseWaitConsoleAnimation()
            session.consoleView?.print(
                "Attached to process $pid\n",
                com.intellij.execution.ui.ConsoleViewContentType.SYSTEM_OUTPUT
            )
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                if (OhosKmpDebugConsoleHilogMirror.isEnabled()) {
                    "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: attached — load Kotlin/Native script, sync breakpoints, continue, HiLog mirror"
                } else {
                    "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: attached — load Kotlin/Native script, sync breakpoints, continue (HiLog mirror disabled)"
                },
            )
            if (debugModeContext == null) {
                session.consoleView?.print(
                    "Please refresh the mobile page\n",
                    com.intellij.execution.ui.ConsoleViewContentType.SYSTEM_OUTPUT
                )
            }

            applySourceJarSourceMaps {
                if (attachStartupAborted()) {
                    pluginDbgPrintln("[Attach] aborted after source-map — skip script/breakpoints/continue")
                    return@applySourceJarSourceMaps
                }

            serviceWrapper.loadKotlinScriptAndUserStartupThen(lldbStartupCommands) {
                if (attachStartupAborted()) {
                    pluginDbgPrintln("[Attach] aborted after Kotlin script — skip breakpoint sync / continue / event loop")
                    return@loadKotlinScriptAndUserStartupThen
                }
                LOG.info("Kotlin script load finished")
                breakpointHandler.syncAllBreakpointsFromProject {
                    if (attachStartupAborted()) {
                        pluginDbgPrintln("[Attach] aborted after breakpoint sync — skip continue / event loop")
                        return@syncAllBreakpointsFromProject
                    }
                    LOG.info("Breakpoint sync done")
                    pluginDbgPrintln("[Attach] ✓ Kotlin breakpoint sync done")
                    resetInferiorCoroutineStepState("attach breakpoint sync")

                    if (debugModeContext != null && OhosKmpCrossLanguageDebug.isEnabled()) {
                        pluginDbgPrintln("[Attach] cross-language: LLDB continue then ArkTS CDP (saveAll before runIfWaiting)")
                        connectArkTsCdpAfterLldbContinue(
                            pid = pid,
                            onReady = {
                                session.printHarmonyDebugPipelineLine(
                                    "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: LLDB startup complete " +
                                        "(ArkTS CDP handshake continues in background; TTDR after CDP ready)",
                                )
                            },
                            onFailed = { err ->
                                if (isAttachToRunningApp()) {
                                    reportAttachCdpDegraded(err)
                                    notifyCrossLangLldbStartupReady {
                                        session.printHarmonyDebugPipelineLine(
                                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: LLDB startup complete (LLDB-only)",
                                        )
                                        HarmonyDebugPipelineTiming.printDebugReady(session)
                                    }
                                } else {
                                    LOG.warn("Cross-language bridge failed before LLDB ready: $err")
                                    session.printHarmonyDebugPipelineLine(
                                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS bridge FAILED — $err",
                                        ConsoleViewContentType.ERROR_OUTPUT,
                                    )
                                }
                            },
                        )
                    } else {
                        finishLldbAttachContinueAndEventLoop(pid)
                        if (debugModeContext != null) {
                            executeDebugModePostContinueSteps()
                        } else {
                            HarmonyDebugPipelineTiming.printDebugReady(session)
                        }
                    }
                }
            }
            }
        }
    }

    private fun resolveAttachAbiDir(): String =
        OhosProcessLibsMirror.resolveAbiDirForDevice(environmentVariables["HDC_UTID"].orEmpty())

    private fun runProcessLibsMirrorBeforeAttach(): Boolean {
        val abiDir = resolveAttachAbiDir()

        pluginDbgPrintln("[Attach] [0/4] ProcessLibs mirror → intermediates/libs/default/$abiDir")
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [0/4] ProcessLibs mirror (intermediates/libs)",
        )
        val result = OhosProcessLibsMirror.mirrorForAbi(
            project = session.project,
            abiDir = abiDir,
            onLogLine = { line -> pluginDbgPrintln("[Attach] [ProcessLibs] $line") },
        )
        if (result.success) {
            pluginDbgPrintln("[Attach] ✓ ${result.summary}")
            session.printHarmonyDebugPipelineLine("$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [0/4] ProcessLibs OK")
            OhosExecSearchPathCache.getInstance(session.project).scheduleRefresh("after ProcessLibs mirror")
            OhosEvalKlibCache.getInstance(session.project).scheduleRefresh("after ProcessLibs mirror")
            return true
        }
        pluginDbgPrintln("[Attach] ✗ ${result.summary}")
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: [0/4] ProcessLibs FAILED — ${result.summary}",
            ConsoleViewContentType.ERROR_OUTPUT,
        )
        return false
    }

    /**
     * Merge DevEco ProcessLibs dir, kmp HAR libs, entry/libs, CMake link dirs, and KMP debugShared outputs.
     * ABI: physical device / default → arm64-v8a; Windows + emulator → x86_64.
     */
    private fun getExecSearchPaths(): List<String> {
        val project = session.project
        val ohRoot = OhosProjectPath.getOhosProjectPath(project)
            ?: OhosProjectPath.getDefaultOhosPath(project)
            ?: project.basePath?.let { Paths.get(it, "harmonyApp") }
            ?: return emptyList()

        val abiDir = resolveAttachAbiDir()

        val kmpModule = HarmonyLocalPropertiesSupport.getKmpGradleModule(project)
        val kmpModules = linkedSetOf(kmpModule, "composeApp", "stdlib", "runtime")
        val cmakeLinkDirs = OhosExecSearchPathCache.getInstance(project).getDirectories(abiDir)

        return OhosExecSearchPathResolver.collectExecSearchPaths(
            ohRoot = ohRoot,
            projectBase = project.basePath?.let { Paths.get(it) },
            abiDir = abiDir,
            kmpGradleModules = kmpModules.toList(),
            cmakeLinkDirs = cmakeLinkDirs,
        )
    }

    private fun startSourceJarMapWarmup(execSearchPaths: List<String>) {
        val abiDir = resolveAttachAbiDir()
        val projectRoot = session.project.basePath?.let { Paths.get(it) }
        sourceJarMapWarmup = CompletableFuture.supplyAsync(
            {
                val workspace = runCatching {
                    projectRoot?.let {
                        OhosWorkspaceSourceMapSupport.prepare(it, execSearchPaths, cancelled = { attachStartupAborted() })
                    }
                }.getOrElse {
                    SourceMapPrepareResult(emptyList(), listOf("[source-map] workspace inference failed: ${it.message}"))
                } ?: SourceMapPrepareResult(emptyList(), emptyList())
                val jars = OhosSourceJarSourceMapSupport.prepareSourceMaps(
                    project = session.project,
                    execSearchPaths = execSearchPaths,
                    abiDir = abiDir,
                    cancelled = { attachStartupAborted() },
                )
                SourceMapPrepareResult(workspace.rules + jars.rules, workspace.logLines + jars.logLines)
            },
            com.intellij.util.concurrency.AppExecutorUtil.getAppExecutorService(),
        )
    }

    private fun applySourceJarSourceMaps(onComplete: () -> Unit) {
        val warmup = sourceJarMapWarmup
        if (warmup == null) {
            onComplete()
            return
        }
        try {
            val result = if (warmup.isDone) {
                warmup.get()
            } else {
                warmup.get(SOURCE_MAP_WARMUP_WAIT_MS, TimeUnit.MILLISECONDS)
            }
            injectPreparedSourceMapRules(result)
            onComplete()
        } catch (_: java.util.concurrent.TimeoutException) {
            pluginDbgPrintln("[source-map] warmup still running — continue attach; map will apply when ready")
            printSourceMapLogs(listOf("[source-map] deferred — inference still running (>${SOURCE_MAP_WARMUP_WAIT_MS}ms)"))
            onComplete()
            warmup.whenComplete { prepared, error ->
                if (attachStartupAborted() || stopped) return@whenComplete
                if (error != null) {
                    pluginDbgPrintln("[source-map] deferred warmup failed: ${error.message}")
                    return@whenComplete
                }
                ApplicationManager.getApplication().executeOnPooledThread {
                    if (attachStartupAborted() || stopped) return@executeOnPooledThread
                    injectPreparedSourceMapRules(prepared)
                    if (prepared.rules.isNotEmpty()) breakpointHandler.rebindAfterSourceMap()
                }
            }
        } catch (e: Exception) {
            pluginDbgPrintln("[source-map] warmup failed: ${e.message}")
            onComplete()
        }
    }

    private fun injectPreparedSourceMapRules(result: SourceMapPrepareResult?) {
        val prepared = result ?: return
        printSourceMapLogs(prepared.logLines)
        injectedSourceMapRules = prepared.rules
        if (prepared.rules.isEmpty()) return
        synchronized(sourceMapInjectLock) {
            for (rule in prepared.rules) {
                val ok = serviceWrapper.handleInterpreterCommand(rule.lldbCommand())
                pluginDbgPrintln("[source-map] inject ${rule.stem}: ${if (ok) "OK" else "FAIL"}")
            }
        }
    }

    private fun printSourceMapLogs(lines: List<String>) {
        if (lines.isEmpty()) return
        val cv = session.consoleView ?: run {
            lines.forEach { pluginDbgPrintln(it) }
            return
        }
        lines.forEach { line ->
            pluginDbgPrintln(line)
            try {
                cv.print("$line\n", ConsoleViewContentType.SYSTEM_OUTPUT)
            } catch (_: Throwable) {
            }
        }
    }

    private fun warnIfExecSearchPathsIncomplete(execSearchPaths: List<String>) {
        if (execSearchPaths.isEmpty()) {
            pluginDbgPrintln(
                "[Attach] ⚠ no exec-search-path dirs found; run publishDebugBinariesToHarmonyApp " +
                    "and DevEco build (ProcessLibs), or append paths via LLDB Startup Commands",
            )
            return
        }
        val hasProcessLibs = execSearchPaths.any { path ->
            path.replace('\\', '/').contains("/intermediates/libs/default/")
        }
        if (!hasProcessLibs) {
            pluginDbgPrintln(
                "[Attach] ⚠ intermediates/libs missing from exec-search-paths; " +
                    "breakpoints may stay pending until DevEco ProcessLibs runs",
            )
        }
    }

    // ===== Step phase timing (OVER / INTO only; enable with ohosKmpStepTiming) =====

    @Volatile private var stepPhaseId: Int = 0
    @Volatile private var stepPhaseStartNs: Long = 0L
    @Volatile private var stepPhaseLastNs: Long = 0L
    @Volatile private var stepPhaseKind: String = ""

    /**
     * Optimistic caret is enabled only for Step Over / Step Into.
     * Step Out is fast enough and the provisional arrow caused small glitches — never show it there.
     */
    @Volatile private var stepPreviewAllowed: Boolean = false

    /** Last continue/step JNI return time; start of the app-run measurement. */
    @Volatile private var lastContinueIssuedNs: Long = 0L
    /** Time the STOPPED event reached the listener; end of the app-run measurement. */
    @Volatile private var lastStopArrivedNs: Long = 0L

    /**
     * App execution span (continue JNI returned → STOPPED received) of the current stop.
     * Elapsed subtracts only this, so native continue and event delivery stay visible.
     */
    @Volatile private var lastStepAppRunMs: Long = 0L
    /** Resume timestamp that [lastStepAppRunMs] belongs to; -1 = unknown. */
    @Volatile private var lastStepAppRunRunStartNs: Long = -1L

    internal fun noteContinueIssued() {
        lastContinueIssuedNs = System.nanoTime()
    }

    internal fun noteStopEventArrived() {
        lastStopArrivedNs = System.nanoTime()
    }

    /** App-run span for [runStartNs], or 0 when it belongs to another run / not measured. */
    internal fun appRunMsForRun(runStartNs: Long): Long =
        if (lastStepAppRunRunStartNs == runStartNs) lastStepAppRunMs else 0L

    private fun stepPhaseMs(v: Double): String =
        String.format(java.util.Locale.US, "%.3f", v)

    /** Start a new step-phase trace. Cheap no-op unless `ohosKmpStepTiming` is on. */
    internal fun beginStepPhaseTrace(kind: String) {
        if (!OhosKmpStepTiming.isEnabled()) {
            stepPhaseStartNs = 0L
            return
        }
        val now = System.nanoTime()
        stepPhaseId++
        stepPhaseStartNs = now
        stepPhaseLastNs = now
        stepPhaseKind = kind
        stepDbgPrintln("[TIMING][step-phase] #$stepPhaseId $kind BEGIN")
    }

    /** Log one phase boundary with delta (since previous mark) and total (since step entry). */
    internal fun stepPhaseMark(label: String) {
        if (!OhosKmpStepTiming.isEnabled()) return
        val start = stepPhaseStartNs
        if (start == 0L) return
        val now = System.nanoTime()
        val delta = (now - stepPhaseLastNs) / 1_000_000.0
        val total = (now - start) / 1_000_000.0
        stepPhaseLastNs = now
        stepDbgPrintln(
            "[TIMING][step-phase] #$stepPhaseId $stepPhaseKind $label " +
                "delta=${stepPhaseMs(delta)}ms total=${stepPhaseMs(total)}ms",
        )
    }

    internal fun endStepPhaseTrace(label: String) {
        stepPhaseMark(label)
        stepPhaseStartNs = 0L
    }

    /**
     * The next step target is resolved (line + address known) before the inferior runs again.
     * Paint the optimistic caret immediately; the real stop corrects or removes it.
     */
    internal fun onStepTargetKnown(filePath: String, line: Int) {
        stepPhaseMark("target-known $filePath:$line")
        if (!stepPreviewAllowed) return
        executionGutterHighlighter.previewStepTarget(filePath, line)
    }

    // ========== Execution control ==========

    override fun startStepOver(context: XSuspendContext?) {
        beginStepPhaseTrace("OVER")
        executionGutterHighlighter.clearStepPreview()
        pluginDbgPrintln(
            "[startStepOver] ENTRY arkTs=${crossLanguageCoordinator?.isArkTsSuspended()} " +
                "sessionSuspended=${session.isSuspended}",
        )
        if (delegateExecutionToCrossLanguage { stepOver(it) }) return
        // Continue while a Kotlin eval holds LLDB's target mutex blocks this EDT call and
        // freezes Android Studio (yieldCase line 39 `score`).
        KotlinEvalResumeGate.request()
        ApplicationManager.getApplication().executeOnPooledThread {
            try {
                startStepOverLldb(context)
            } catch (t: Throwable) {
                LOG.warn("startStepOver failed off EDT", t)
                printPluginDebugConsole(
                    "[startStepOver] failed ${t.javaClass.simpleName}: ${t.message}",
                )
                clearUserContinueOrStepPending()
                val tid = getCurrentThreadId() ?: 0
                restoreIdePauseAfterFailedContinueOrStep(tid)
            }
        }
    }

    internal fun stepOverLldbOnly(context: XSuspendContext?) = startStepOverLldb(context)

    /**
     * 调用方是界面点击回调（App.kt 按钮），不是当前函数里的下一行。
     */
    private fun callerIsExternalClick(caller: StackFrame): Boolean {
        val file = caller.file.replace('\\', '/')
        return file.endsWith("/App.kt") || file == "App.kt"
    }

    /**
     * 普通函数最后一行后面没有代码。继续运行，不再停在外部 App.kt。
     * 进程离开暂停后，Step Over 与 Step Into 置灰。
     *
     * @return 这次 Step Over 已处理完毕。
     */
    private fun resumeAfterLastPlainLine(threadId: Int): Boolean {
        printPluginDebugConsole(
            "[startStepOver] last line has no following code — resume, do not stop in App.kt",
        )
        clearStickyUserSourcePause()
        clearPendingCoroutineResume()
        val ok = continueDiscardingStepPlans(threadId)
        if (!ok) {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
        return true
    }

    /**
     * `return` from a plain function called by `async { lazyBody() }`.
     *
     * Native Step Over returns into the lambda epilogue and the process keeps running.
     * A file:line breakpoint on the next statement binds `await()`'s suspend point, which a
     * completed lazy body never executes. Stop at the caller state machine's return PC when
     * that frame is on the stack. When it is not, arm the next statement and the one after
     * it and Continue — staying on `return` made Step Over a no-op.
     *
     * @return true when this Step Over was fully handled.
     */
    private fun stepOverReturnIntoCoroutine(threadId: Int): Boolean {
        val frames = lastSuspendedStackFrames.orEmpty()
        val top = frames.firstOrNull() ?: return false
        val caller = frames.getOrNull(1) ?: return false
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        if (classifier.isCoroutineLambdaFrame(top.name) || classifier.isStateMachineFrame(top.name)) return false
        val callerIsCoroutine = classifier.isCoroutineLambdaFrame(caller.name) ||
            classifier.isStateMachineFrame(caller.name)
        if (!callerIsCoroutine) {
            // 普通函数最后一行。调用方是 App.kt 的点击回调时，原生 Step Over 会停在按钮上。
            // 与普通调试 / KMP Android 一致：后面没有代码就结束这次暂停，Step Over 与 Step Into 置灰。
            if (callerIsExternalClick(caller)) return resumeAfterLastPlainLine(threadId)
            return false
        }
        val target = nextLineAfterEnclosingStateMachine(frames)
        if (target == null) {
            // `scope.cancel()` 是普通函数最后一行，调用方没有协程父行。
            // 停在当前行会让每次 Step Over 都无效；原生 Step Over 又会进入 App.kt 按钮。
            return resumeAfterLastPlainLine(threadId)
        }
        val selectedSymbol = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            .selectCallerStateMachineSymbol(
                frames.map { Triple(it.name, it.file, it.line) },
                target.file,
                target.line,
            )
        val callerSymbol = selectedSymbol.ifEmpty { coroutineSupport.lastUserStateMachineSymbol() }
        printPluginDebugConsole(
            "[startStepOver] return into coroutine caller=${callerSymbol.ifEmpty { "<none>" }} " +
                "from=${if (selectedSymbol.isNotEmpty()) "stack" else "last-breakpoint"} " +
                "target=${target.file}:${target.line}",
        )
        val resumed = try {
            // threadId is the LLDB tid. selectThread() treats it as an index and fails (ok=0).
            coroutineSupport.armCallerResume(target.line, callerSymbol, threadId)
        } catch (t: Throwable) {
            LOG.warn("arm return-into-coroutine pc failed", t)
            null
        }
        if (resumed == null) {
            // The parent invokeSuspend is not a live frame (inlined, or its name does not
            // contain `$invokeCOROUTINE$`). Staying here made every Step Over a no-op.
            // `await()`'s suspend PC is often not executed once the Deferred is complete, so
            // also arm the following statement.
            val followingLine = target.line + 1
            val armedTarget = coroutineSupport.armEditorLine(target.file, target.line, replace = true)
            val armedFollowing = coroutineSupport.armEditorLine(target.file, followingLine, replace = false)
            if (!armedTarget && !armedFollowing) {
                printPluginDebugConsole(
                    "[startStepOver] return into coroutine parent frame missing and " +
                        "${target.file}:${target.line}/$followingLine have no locations — native step",
                )
                return false
            }
            val originLine = (lastTopFrameIdeSourceOverride?.line ?: -1) + 1
            pendingCoroutineResume = true
            coroutineResumeAutoContinueCount = 0
            pendingCoroutineResumeLine = target.line
            pendingCoroutineResumeFile = target.file
            pendingCoroutineResumeOriginLine = originLine
            pendingCoroutineResumeThreadId = threadId
            pendingCoroutineResumeLastStatement = false
            pendingCoroutineResumeOriginSymbol = frames.firstOrNull()?.name.orEmpty()
            pendingParentPrologueHopDone = false
            pendingCallerStopLine = 0
            pendingCallerResumeArmed = true
            leftoverNativeStepPlanLikely = true
            stickyUserSourcePause = false
            syncNativePendingResumeAutoContinue(enabled = true)
            printPluginDebugConsole(
                "[startStepOver] return into coroutine parent frame missing — continue to " +
                    "${target.file}:${target.line}" +
                    (if (armedFollowing) " and $followingLine" else "") +
                    " armed=${if (armedTarget) target.line else "-"}/" +
                    (if (armedFollowing) followingLine.toString() else "-"),
            )
            val continued = continueDiscardingStepPlans(threadId)
            if (!continued) {
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return true
        }
        val showLine = if (resumed.pcLine >= target.line) resumed.pcLine else target.line
        val originLine = (lastTopFrameIdeSourceOverride?.line ?: -1) + 1
        pendingCoroutineResume = true
        coroutineResumeAutoContinueCount = 0
        pendingCoroutineResumeLine = showLine
        pendingCoroutineResumeFile = target.file
        pendingCoroutineResumeOriginLine = originLine
        pendingCoroutineResumeThreadId = threadId
        pendingCoroutineResumeLastStatement = false
        pendingCoroutineResumeOriginSymbol = frames.firstOrNull()?.name.orEmpty()
        pendingParentPrologueHopDone = false
        pendingCallerStopLine = resumed.pcLine
        pendingCallerResumeArmed = true
        // The stop on `return part` finished a native step. Drop that plan so Continue
        // cannot step out through the lambda and run the rest of the click callback.
        leftoverNativeStepPlanLikely = true
        stickyUserSourcePause = false
        syncNativePendingResumeAutoContinue(enabled = true)
        printPluginDebugConsole(
            "[startStepOver] return into coroutine pc=0x${resumed.pc.toString(16)} " +
                "at line ${resumed.pcLine} → show ${target.file}:$showLine",
        )
        val ok = continueDiscardingStepPlans(threadId)
        if (!ok) {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
        return true
    }

    private data class CoroutineResumeLine(val file: String, val line: Int)

    /** Next editor line of the nearest state-machine frame under the plain function. */
    private fun nextLineAfterEnclosingStateMachine(frames: List<StackFrame>): CoroutineResumeLine? {
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lambda = frames.drop(1).firstOrNull {
            classifier.isCoroutineLambdaFrame(it.name) && it.line > 1 && it.file.isNotEmpty()
        }
        for (frame in frames.drop(1)) {
            if (!classifier.isStateMachineFrame(frame.name)) continue
            if (frame.line <= 1 || frame.file.isEmpty()) continue
            val sameFile = lambda == null || sourceFileBasename(frame.file) == sourceFileBasename(lambda.file)
            if (sameFile && lambda != null && frame.line <= lambda.line) {
                val afterStart = lineAfterNamedCall(frame.file, lambda.line, "start")
                if (afterStart != null) return afterStart
                continue
            }
            val text = sourceLineTextAt(frame.file, frame.line)
            if (policy.isClosingBraceLine(text) || policy.isIgnorableSourceLine(text)) continue
            val target = policy.resolveStepOverTarget(frame.line) { line -> sourceLineTextAt(frame.file, line) }
            if (target.lastStatement || target.nextLine1Based <= frame.line) continue
            return CoroutineResumeLine(frame.file, target.nextLine1Based)
        }
        if (lambda != null) return lineAfterNamedCall(lambda.file, lambda.line, "start")
        return null
    }

    /** The statement after [name](…), searching a few lines below [fromLine]. */
    private fun lineAfterNamedCall(file: String, fromLine: Int, name: String): CoroutineResumeLine? {
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val call = Regex("""\b${Regex.escape(name)}\s*\(""")
        for (line in (fromLine + 1)..(fromLine + 8)) {
            val text = sourceLineTextAt(file, line) ?: break
            if (!call.containsMatchIn(text)) continue
            val target = policy.resolveStepOverTarget(line) { sourceLineTextAt(file, it) }
            if (target.lastStatement || target.nextLine1Based <= line) return null
            return CoroutineResumeLine(file, target.nextLine1Based)
        }
        return null
    }

    private fun sourceFileBasename(path: String): String =
        path.substringAfterLast('/').substringAfterLast('\\')

    /**
     * Parent coroutine frame line to show after Step Out (e.g. launch block at 167 while inside
     * `suspendStepInside` at 175).
     */
    private fun resolveStepOutCallerTarget(
        threadId: Int,
        calleeFile: String,
        calleeOriginLine: Int,
        calleeOriginSymbol: String,
    ): Pair<String, Int>? {
        if (calleeOriginLine <= 1) return null
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(calleeFile, line) }
        var currentFn = classifier.suspendBodyFunctionNameFromSymbol(calleeOriginSymbol)
        if (currentFn.isEmpty()) {
            currentFn = classifier.readableNativeStackLabel(calleeOriginSymbol)
        }
        if (currentFn.isEmpty()) {
            currentFn = classifier.readableSuspendFunctionName(calleeOriginSymbol)
        }
        if (currentFn.isEmpty() || currentFn.contains('/') || currentFn.contains('.')) {
            currentFn = policy.inferEnclosingSuspendFunctionName(calleeOriginLine, lineTextAt)
        }
        if (currentFn.isNotEmpty() && calleeFile.isNotEmpty()) {
            val callerOfCurrent = policy.findSuspendCallSiteAbove(calleeOriginLine, currentFn, lineTextAt)
            coroutineStepLog(
                "step-out caller lookup currentFn=$currentFn callerOfCurrent=$callerOfCurrent " +
                    "origin=$calleeOriginLine symbol=$calleeOriginSymbol",
            )
            if (callerOfCurrent > 0 &&
                policy.isValidStepOutCallerLine(callerOfCurrent, calleeOriginLine, lineTextAt)
            ) {
                return calleeFile to callerOfCurrent
            }
        }
        coroutineSupport.logicalCallerFrameForStepOut(threadId, calleeOriginLine)?.let { (file, line) ->
            if (policy.isValidStepOutCallerLine(line, calleeOriginLine, lineTextAt)) {
                return file to line
            }
        }
        val calleeBase = sourceFileBasename(calleeFile)
        var bestSameFile: Pair<String, Int>? = null
        for (frame in getLastSuspendedStackFrames().orEmpty().drop(1)) {
            if (classifier.isLibraryGlueFrame(frame.name)) continue
            if (frame.line <= 1 || frame.file.isEmpty()) continue
            if (sourceFileBasename(frame.file) == calleeBase && frame.line < calleeOriginLine) {
                if (bestSameFile == null || frame.line > bestSameFile.second) {
                    bestSameFile = frame.file to frame.line
                }
            }
            if (!classifier.isStateMachineFrame(frame.name) &&
                !classifier.isCoroutineLambdaFrame(frame.name)
            ) {
                continue
            }
            if (sourceFileBasename(frame.file) == calleeBase && frame.line >= calleeOriginLine) continue
            val text = sourceLineTextAt(frame.file, frame.line)
            if (policy.isClosingBraceLine(text) || policy.isIgnorableSourceLine(text)) continue
            if (!policy.isValidStepOutCallerLine(frame.line, calleeOriginLine, lineTextAt)) continue
            return frame.file to frame.line
        }
        bestSameFile?.let {
            if (policy.isValidStepOutCallerLine(it.second, calleeOriginLine, lineTextAt) &&
                currentFn.isNotEmpty() &&
                policy.findSuspendCallSiteAbove(calleeOriginLine, currentFn, lineTextAt) == it.second
            ) {
                return it
            }
        }
        return null
    }

    /**
     * Step Out pending: breakpoints on the suspend **call** line; caret on the next user statement
     * (e.g. 203 `suspendStepOutInner()` → show 204, not 203 — avoids Step Into / Step Out loop).
     */
    private fun assignStepOutCallerPending(callerFile: String, callSiteLine: Int) {
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(callerFile, line) }
        pendingCoroutineResumeCallSiteLine = callSiteLine
        pendingCoroutineResumeFile = callerFile
        val landing = policy.stepOutCallerResumeLandingLine(callSiteLine, lineTextAt)
        pendingCoroutineResumeLine = when {
            landing > callSiteLine -> landing
            else -> callSiteLine
        }
        coroutineStepLog(
            "step-out caller call=$callerFile:$callSiteLine caret→$pendingCoroutineResumeLine",
        )
    }

    /**
     * Parent continuation address breakpoints alone often miss when the parent has no user
     * breakpoint (KN resumes on the statement *after* the suspend call, e.g. 205 not 203).
     * Arm internal line breakpoints on the call site and the next user line — same as a user BP
     * on the landing line, but without requiring one.
     */
    private fun armStepOutCallerEditorBreakpoints() {
        val callerFile = pendingCoroutineResumeFile
        val callerLine = pendingCoroutineResumeCallSiteLine.takeIf { it > 0 }
            ?: pendingCoroutineResumeLine
        if (callerFile.isEmpty() || callerLine <= 0) return
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(callerFile, line) }
        if (!policy.isValidStepOutCallerLine(callerLine, pendingCoroutineResumeOriginLine, lineTextAt)) {
            coroutineStepLog(
                "step-out reject caller line $callerLine (must be < origin ${pendingCoroutineResumeOriginLine}, not launch header)",
            )
            return
        }
        val detector = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
        val callSiteText = lineTextAt(callerLine)
        val frameTriples = getLastSuspendedStackFrames().orEmpty().map { Triple(it.name, it.file, it.line) }
        val callerFrameSymbol = policy.selectCallerFrameSymbolForStepOut(frameTriples, callerFile, callerLine)
        val originLaunchHeader = policy.isCoroutineScopeLaunchHeaderLine(
            lineTextAt(pendingCoroutineResumeOriginLine),
        )
        val launchBlockCaller = policy.isLaunchBlockSuspendCallSite(
            frameTriples,
            callerFile,
            callerLine,
            lineTextAt,
        )
        val launchLambdaCaller = callerFrameSymbol.isNotEmpty() &&
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                .isCoroutineLambdaFrame(callerFrameSymbol)
        // Step Over must not arm 393 — re-enters callee. Step Out from epilogue 399 returns to 393.
        if (originLaunchHeader) {
            val machineryLine = policy.launchLambdaClosingBraceLine(pendingCoroutineResumeOriginLine, lineTextAt)
            if (machineryLine > 0) {
                pendingLaunchBlockMachineryLine = machineryLine
                val armedMachinery = coroutineSupport.armEditorLine(callerFile, machineryLine, replace = false)
                coroutineStepPipeline(
                    "step-out launch-header machinery bp armed=$armedMachinery " +
                        "$callerFile:$machineryLine (caret→$callerLine)",
                )
            }
        } else if ((launchBlockCaller || launchLambdaCaller) && detector.looksLikeSuspendCall(callSiteText)) {
            val useLaunchMachinery = policy.stepOutUsesLaunchBlockMachinery(
                pendingCoroutineResumeOriginLine,
                callerFile,
                callerLine,
                lineTextAt,
                frameTriples,
            )
            if (useLaunchMachinery) {
                val machineryLine = policy.launchBlockStepOutMachineryLine(callerLine, lineTextAt)
                if (machineryLine > 0) {
                    pendingLaunchBlockMachineryLine = machineryLine
                    val armedMachinery = coroutineSupport.armEditorLine(callerFile, machineryLine, replace = false)
                    coroutineStepPipeline(
                        "step-out launch-block machinery bp armed=$armedMachinery " +
                            "$callerFile:$machineryLine (caret→$callerLine)",
                    )
                } else {
                    val armedSite = coroutineSupport.armEditorLine(callerFile, callerLine, replace = false)
                    coroutineStepPipeline(
                        "step-out launch-block call-site bp armed=$armedSite $callerFile:$callerLine " +
                            "symbol=${callerFrameSymbol.ifEmpty { "inferred" }}",
                    )
                }
                return
            }
            coroutineStepPipeline(
                "step-out skip launch-block machinery arm — plain caller chase " +
                    "$callerFile:$callerLine from origin ${pendingCoroutineResumeOriginLine}",
            )
        }
        // Never arm the suspend *call* line for Step Over semantics (167 → re-enters 173).
        val next = if (detector.looksLikeSuspendCall(callSiteText)) {
            policy.resolveStepOverTarget(callerLine, lineTextAt)
        } else {
            coroutineSupport.armEditorLine(callerFile, callerLine, replace = false).also { armed ->
                coroutineStepLog("step-out caller line bp armed=$armed $callerFile:$callerLine")
            }
            policy.resolveStepOverTarget(callerLine, lineTextAt)
        }
        if (!next.lastStatement && next.nextLine1Based > callerLine &&
            policy.isValidParentResumeUserLine(next.nextLine1Based, lineTextAt(next.nextLine1Based))
        ) {
            val armedNext = coroutineSupport.armEditorLine(callerFile, next.nextLine1Based, replace = false)
            coroutineStepLog(
                "step-out caller next line bp armed=$armedNext $callerFile:${next.nextLine1Based}",
            )
        }
        armStepOutCallerResumePc(pendingCoroutineResumeThreadId)
    }

    /**
     * Step Out from the outermost suspend function: there is no caller to return to, so resume to the
     * next user breakpoint instead of arming internal caller breakpoints or a native Step Out.
     */
    private fun stepOutFromTopestSuspendFunction(threadId: Int) {
        coroutineStepPipeline(
            "step-out topest suspend function — no caller to arm; Continue to next user breakpoint",
        )
        clearPendingCoroutineResume()
        clearStickyUserSourcePause()
        leftoverNativeStepPlanLikely = false
        restoreBreakpointsAfterNativeCallSiteStepOut = false
        val ok = continueDiscardingStepPlans(threadId)
        if (!ok) {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
    }

    /**
     * After `delay` resumes (e.g. launch step-out 428), a parent-continuation breakpoint alone may
     * not stop before the caller's next statement. Bind the caller state machine's return PC at the
     * landing line (423 after call 422), same as Step Over out of a plain `return`.
     */
    /**
     * Step Out from a coroutine epilogue when the continuation chain could not be armed (428).
     * Relies on internal caller line breakpoints + [armStepOutCallerResumePc].
     */
    private fun armStepOutViaCallerBreakpointsOnly(
        threadId: Int,
        ctx: CoroutineStepEditorContext,
    ): Boolean {
        val originLine = ctx.currentLine1Based
        val target = resolveStepOutCallerTarget(
            threadId,
            ctx.filePath,
            originLine,
            ctx.topSymbol.orEmpty(),
        ) ?: return false
        pendingCoroutineResume = true
        coroutineResumeAutoContinueCount = 0
        assignStepOutCallerPending(target.first, target.second)
        pendingCoroutineResumeOriginLine = originLine
        pendingCoroutineResumeThreadId = threadId
        pendingCoroutineResumeLastStatement = false
        pendingCoroutineStepOut = true
        pendingCoroutineResumeOriginSymbol = ctx.topSymbol.orEmpty()
        pendingParentPrologueHopDone = false
        pendingCallerStopLine = 0
        pendingCallerResumeArmed = false
        leftoverNativeStepPlanLikely = false
        stickyUserSourcePause = false
        syncNativePendingResumeAutoContinue(enabled = true)
        coroutineStepPipeline(
            "step-out caller-only chase ${target.first}:${target.second}→$pendingCoroutineResumeLine from $originLine",
        )
        coroutineStepLog(
            "step-out caller-only chase ${target.first}:${target.second}→$pendingCoroutineResumeLine from $originLine",
        )
        val parentArmed = coroutineSupport.tryArmStepOutParentContinuation(
            threadId,
            ctx.topSymbol,
        )
        if (parentArmed) {
            coroutineStepPipeline("step-out caller-only also armed parent continuation")
        }
        plainCallerStepOutNativeUnwind = originLine > target.second
        armStepOutCallerEditorBreakpoints()
        return true
    }

    /**
     * `launch { launchStepInside() }` at 393 — no outer SM; Continue from 399 epilogue never hits caller.
     */
    private fun armLaunchBlockStepOutPending(
        threadId: Int,
        ctx: CoroutineStepEditorContext,
        target: Pair<String, Int>,
    ): Boolean {
        pendingCoroutineResume = true
        coroutineResumeAutoContinueCount = 0
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(target.first, line) }
        val callSite = target.second
        if (org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
                .looksLikeSuspendCall(lineTextAt(callSite))
        ) {
            assignStepOutCallerPending(target.first, callSite)
        } else {
            pendingCoroutineResumeCallSiteLine = 0
            pendingCoroutineResumeLine = callSite
            pendingCoroutineResumeFile = target.first
        }
        pendingCoroutineResumeOriginLine = ctx.currentLine1Based
        pendingCoroutineResumeThreadId = threadId
        pendingCoroutineResumeLastStatement = false
        pendingCoroutineStepOut = true
        pendingCoroutineResumeOriginSymbol = ctx.topSymbol.orEmpty()
        pendingParentPrologueHopDone = false
        pendingCallerStopLine = 0
        pendingCallerResumeArmed = false
        leftoverNativeStepPlanLikely = false
        stickyUserSourcePause = false
        pendingLaunchBlockStepOut = true
        launchBlockStepOutContinueChase = true
        syncNativePendingResumeAutoContinue(enabled = true)
        coroutineStepPipeline(
            "step-out launch-block pending ${target.first}:${target.second} from ${ctx.currentLine1Based}",
        )
        val parentArmed = coroutineSupport.tryArmStepOutParentContinuation(threadId, ctx.topSymbol)
        coroutineStepPipeline("step-out launch-block parent continuation armed=$parentArmed")
        armStepOutCallerEditorBreakpoints()
        return true
    }

    private fun armStepOutCallerResumePc(threadId: Int) {
        if (threadId <= 0) return
        val callerFile = pendingCoroutineResumeFile
        val callerLine = pendingCoroutineResumeLine
        if (callerFile.isEmpty() || callerLine <= 0) return
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val detector = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(callerFile, line) }
        val landingLine = if (pendingCoroutineStepOut) {
            policy.stepOutCallerResumeLandingLine(callerLine, lineTextAt)
        } else if (detector.looksLikeSuspendCall(lineTextAt(callerLine))) {
            policy.resolveStepOverTarget(callerLine, lineTextAt).nextLine1Based
        } else {
            val next = policy.resolveStepOverTarget(callerLine, lineTextAt)
            when {
                !next.lastStatement && next.nextLine1Based > callerLine -> next.nextLine1Based
                else -> callerLine
            }
        }
        if (landingLine <= 0) return
        val frames = getLastSuspendedStackFrames().orEmpty()
        val frameTriples = frames.map { Triple(it.name, it.file, it.line) }
        val launchBlockCaller = policy.isLaunchBlockSuspendCallSite(
            frameTriples,
            callerFile,
            callerLine,
            lineTextAt,
        )
        var callerSymbol = policy.selectCallerFrameSymbolForStepOut(
            frameTriples,
            callerFile,
            landingLine,
        ).ifEmpty {
            frames.drop(1).firstOrNull { frame ->
                val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                classifier.isStateMachineFrame(frame.name) &&
                    !classifier.isLibraryGlueFrame(frame.name) &&
                    !classifier.isCoroutineInternalFrame(frame.name)
            }?.name.orEmpty()
        }
        if (callerSymbol.isEmpty()) {
            callerSymbol = frames.drop(1).firstOrNull { frame ->
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                    .isCoroutineLambdaFrame(frame.name)
            }?.name.orEmpty()
        }
        val symbolForResume = when {
            callerSymbol.isNotEmpty() -> callerSymbol
            launchBlockCaller -> ""
            else -> ""
        }
        if (symbolForResume.isEmpty() && !launchBlockCaller) {
            coroutineStepLog("step-out caller-resume skipped — no parent state-machine symbol")
            return
        }
        val resumed = runCatching {
            coroutineSupport.armCallerResume(landingLine, symbolForResume, threadId)
        }.getOrNull()
        if (resumed == null) {
            coroutineStepPipeline(
                "step-out caller-resume missed landing=$landingLine symbol=${symbolForResume.ifEmpty { "stack" }}",
            )
            coroutineStepLog(
                "step-out caller-resume missed landing=$landingLine symbol=$symbolForResume",
            )
            return
        }
        pendingCallerStopLine = resumed.pcLine
        pendingCallerResumeArmed = true
        coroutineStepLog(
            "step-out caller-resume pc=0x${resumed.pc.toString(16)} " +
                "pcLine=${resumed.pcLine} landing=$landingLine",
        )
    }

    private data class CoroutineStepEditorContext(
        val sourceLine: String?,
        val atSuspendCall: Boolean,
        val filePath: String,
        val currentLine1Based: Int,
        val topSymbol: String?,
        val topFile: String?,
        val lambdaFrame: Boolean,
        val onStateMachine: Boolean,
        val nextLine: Int,
        val lastStatement: Boolean,
        val nextText: String?,
    )

    private fun gatherCoroutineStepEditorContext(): CoroutineStepEditorContext {
        val sourceLine = currentTopFrameSourceLineText()
        val atSuspendCall =
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector.looksLikeSuspendCall(
                sourceLine,
            )
        val sourcePosition = lastTopFrameIdeSourceOverride
        val filePath = sourcePosition?.file?.path.orEmpty()
        val currentLine1Based = sourcePosition?.let { it.line + 1 } ?: 0
        val topFrame = lastSuspendedStackFrames?.firstOrNull()
        val topSymbol = topFrame?.name
        val topFile = topFrame?.file
        val frames = lastSuspendedStackFrames.orEmpty()
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val onStateMachine = frames.any { f ->
            classifier.isStateMachineFrame(f.name) ||
                classifier.isDemangledSuspendContinuationFrame(f.name)
        } || classifier.isStateMachineFrame(topSymbol) ||
            classifier.isDemangledSuspendContinuationFrame(topSymbol)
        val lambdaFrame = classifier.isCoroutineLambdaFrame(topSymbol)
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(filePath, line) }
        val target = if (lambdaFrame) {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                .resolveLambdaStepOverPastSuspend(currentLine1Based, lineTextAt)
        } else {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.resolveStepOverTarget(
                currentLine1Based = currentLine1Based,
                lineTextAt = lineTextAt,
            )
        }
        val nextLine = target.nextLine1Based
        val lastStatement = target.lastStatement
        val nextText = if (nextLine > 0) lineTextAt(nextLine) else null
        return CoroutineStepEditorContext(
            sourceLine = sourceLine,
            atSuspendCall = atSuspendCall,
            filePath = filePath,
            currentLine1Based = currentLine1Based,
            topSymbol = topSymbol,
            topFile = topFile,
            lambdaFrame = lambdaFrame,
            onStateMachine = onStateMachine,
            nextLine = nextLine,
            lastStatement = lastStatement,
            nextText = nextText,
        )
    }

    /**
     * User Step Out while stopped on `[K][Suspend]Functions` / suspend trampoline — use a Kotlin
     * line one-shot instead of LLDB Step Out (which opens Disassembly).
     */
    private fun resumePastSuspendCallingConventionForStepOut(threadId: Int): Boolean {
        val top = lastSuspendedStackFrames?.firstOrNull() ?: return false
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        if (!policy.isKotlinSuspendCallingConventionFrame(top.name, top.file)) return false
        val origin = lastTopFrameIdeSourceOverride ?: lastKnownMappableIdeSource
        val file = origin?.file?.path.orEmpty()
        val originLine = (origin?.line ?: -1) + 1
        val target = if (file.isNotEmpty() && originLine > 0) {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                .resolveLambdaStepOverPastSuspend(originLine) { line -> sourceLineTextAt(file, line) }
        } else {
            null
        } ?: return false
        if (target.lastStatement || target.nextLine1Based <= originLine) return false
        pendingCoroutineResume = true
        pendingCoroutineResumeLine = target.nextLine1Based
        pendingCoroutineResumeFile = file
        pendingCoroutineResumeOriginLine = originLine
        pendingCoroutineResumeThreadId = threadId
        pendingCoroutineResumeLastStatement = false
        pendingCoroutineResumeOriginSymbol = top.name.orEmpty()
        pendingCallerStopLine = 0
        pendingCallerResumeArmed = false
        coroutineResumeAutoContinueCount = 0
        syncNativePendingResumeAutoContinue(enabled = true)
        if (!continueToCallerUserLine(threadId, file, target.nextLine1Based)) {
            clearPendingCoroutineResume()
            return false
        }
        coroutineStepLog(
            "[startStepOut] suspend bridge — one-shot $file:${target.nextLine1Based}, no native Step Out",
        )
        return true
    }

    private fun startStepOverLldb(context: XSuspendContext?) {
        stepPhaseMark("worker-start")
        stepPreviewAllowed = true
        val threadId = getCurrentThreadId()
        val suspended = threadId?.let { isThreadSuspended(it) } == true
        pluginDbgPrintln("[startStepOver] threadId=$threadId isThreadSuspended=$suspended sessionSuspended=${session.isSuspended}")
        if (threadId == null || !suspended) {
            pluginDbgPrintln("[startStepOver] SKIP — thread not suspended or id null (breakpoint/step JNI not sent)")
            return
        }
        if (!tryBeginUserContinueOrStep()) {
            stepPhaseMark("skip-not-stoppable")
            endStepPhaseTrace("abort")
            return
        }
        stepPhaseMark("session-check")
        clearStickyUserSourcePause()
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        if (useInstructionLevelStepping()) {
            printPluginDebugConsole("[startStepOver] Disassembly — instruction step over")
            resumeInstructionLevelStep(threadId, LLDBServiceWrapper.StepOption.InstructionOver)
            return
        }
        // A suspend call does not return to its caller, so an ordinary step-over runs away into
        // the dispatcher. When the coroutine step arms a resume breakpoint we plainly resume.
        val ctx = gatherCoroutineStepEditorContext()
        stepPhaseMark("editor-context")
        val sourceLine = ctx.sourceLine
        val atSuspendCall = ctx.atSuspendCall
        val filePath = ctx.filePath
        val currentLine1Based = ctx.currentLine1Based
        val topSymbol = ctx.topSymbol
        val lambdaFrame = ctx.lambdaFrame
        val onStateMachine = ctx.onStateMachine
        val nextLine = ctx.nextLine
        val lastStatement = ctx.lastStatement
        val nextText = ctx.nextText
        val lambdaStepOut = lambdaFrame &&
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                .coroutineLambdaShouldStepOut(nextText, lastStatement)
        pluginDbgPrintln(
            "[Coroutines][step] decision symbol=$topSymbol stateMachine=$onStateMachine " +
                "lambda=$lambdaFrame atSuspendCall=$atSuspendCall " +
                "current=$currentLine1Based next=$nextLine lastStatement=$lastStatement " +
                "lambdaStepOut=$lambdaStepOut nextText=${nextText?.trim()} file=$filePath " +
                "sourceLine=${sourceLine?.trim()}",
        )
        if (lambdaStepOut) {
            pluginDbgPrintln(
                "[startStepOver] next line is ${nextText?.trim()} — step out of lambda, do not Continue into suspend",
            )
            val ok = serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Out)
            if (!ok) {
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }
        printPluginDebugConsole(
            "[startStepOver] atSuspendCall=$atSuspendCall sourceLine=${sourceLine?.trim()} " +
                "filePath=$filePath currentLine=$currentLine1Based nextLine=$nextLine lastStatement=$lastStatement",
        )
        // Step Over a suspend call (user or library) is handled by the coroutine step below;
        // do not block it just because a user breakpoint sits on the call line.
        val followingText = if (nextLine > 0) sourceLineTextAt(filePath, nextLine + 1) else null
        val alsoArmFollowingLine = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
            .stepOverShouldArmFollowingLine(sourceLine, nextText, followingText)
        if (alsoArmFollowingLine) {
            printPluginDebugConsole(
                "[startStepOver] next line is a library suspend point — also arm line ${nextLine + 1}",
            )
        }
        stepPhaseMark("arm-begin")
        if (nextLine > 0 && filePath.isNotEmpty()) {
            // The next editor line is already known from the source text — before any USB
            // round trip. Paint the optimistic caret now; target-known confirms it later.
            executionGutterHighlighter.previewStepTarget(filePath, nextLine)
        }
        val armed = armCoroutineStep(
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER,
            threadId,
            atSuspendCall,
            nextLine,
            filePath,
            lastStatement,
            alsoArmFollowingLine,
            explicitOriginLine1Based = currentLine1Based,
        )
        stepPhaseMark("arm-done")
        if (armed) {
            printPluginDebugConsole("[startStepOver] option=resume(null) coroutine-armed")
            val userBreakpoint = currentLine1Based > 0 && filePath.isNotEmpty() &&
                breakpointHandler.findBreakpoint(filePath, currentLine1Based) != null
            if (userBreakpoint) {
                try {
                    breakpointHandler.suspendUserBreakpointForStep(filePath, currentLine1Based)
                } catch (t: Throwable) {
                    // Disabling the origin breakpoint must not swallow the Continue.
                    // A throw here used to leave the IDE in the running state while the
                    // process stayed stopped, so the next Step Over could not start.
                    printPluginDebugConsole(
                        "[startStepOver] disable user breakpoint failed: ${t.javaClass.simpleName}: ${t.message}",
                    )
                }
            }
            stepPhaseMark("origin-bp-off")
            printPluginDebugConsole(
                "[startStepOver] continue after arm origin=$currentLine1Based next=$nextLine",
            )
            try {
                pluginDbgPrintln(
                    "[Coroutines][step] armed resume userBreakpointDisabled=$userBreakpoint " +
                        "origin=$filePath:$currentLine1Based",
                )
            } catch (_: Throwable) {
            }
            stepPhaseMark("resume-issue")
            val ok = continueDiscardingStepPlans(threadId)
            stepPhaseMark("resume-returned")
            if (!ok) {
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }
        if (onStateMachine && atSuspendCall) {
            if (lastStatement && userSuspendCallBpStepOverAllowed && currentLine1Based > 0 && filePath.isNotEmpty()) {
                printPluginDebugConsole(
                    "[startStepOver] suspend call before lambda `}` — plain Continue (arm missed, origin=$currentLine1Based)",
                )
                val userBreakpoint = breakpointHandler.findBreakpoint(filePath, currentLine1Based) != null
                if (userBreakpoint) {
                    runCatching { breakpointHandler.suspendUserBreakpointForStep(filePath, currentLine1Based) }
                }
                leftoverNativeStepPlanLikely = true
                userSuspendCallBpStepOverAllowed = false
                val ok = continueDiscardingStepPlans(threadId)
                if (!ok) {
                    clearUserContinueOrStepPending()
                    restoreIdePauseAfterFailedContinueOrStep(threadId)
                }
                return
            }
            printPluginDebugConsole(
                "[startStepOver] state machine suspend-call at line $currentLine1Based " +
                    "did not bind line $nextLine — stay here",
            )
            val nextIsSuspend =
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
                    .looksLikeSuspendCall(nextText)
            if (onStateMachine && atSuspendCall && nextLine > currentLine1Based && !nextIsSuspend &&
                filePath.isNotEmpty() &&
                coroutineSupport.tryArmEditorLineBreakpoint(filePath, nextLine)
            ) {
                printPluginDebugConsole(
                    "[startStepOver] call-site suspend-call retry — armed line $nextLine (post step-out landing)",
                )
                val ok = continueDiscardingStepPlans(threadId)
                if (!ok) {
                    clearUserContinueOrStepPending()
                    restoreIdePauseAfterFailedContinueOrStep(threadId)
                }
                return
            }
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
            return
        }
        if (lastStatement && stepOverReturnIntoCoroutine(threadId)) return
        if (lambdaFrame && lastStatement && filePath.isNotEmpty() && currentLine1Based > 0) {
            val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            val afterLambda = policy.resolveCoroutineLambdaStepOverTarget(currentLine1Based) { line ->
                sourceLineTextAt(filePath, line)
            }
            val targetLine = when {
                afterLambda.nextLine1Based > currentLine1Based -> afterLambda.nextLine1Based
                else -> currentLine1Based
            }
            pendingCoroutineResume = true
            coroutineResumeAutoContinueCount = 0
            pendingCoroutineResumeLine = targetLine
            pendingCoroutineResumeFile = filePath
            pendingCoroutineResumeOriginLine = currentLine1Based
            pendingCoroutineResumeThreadId = threadId
            pendingCoroutineResumeLastStatement = afterLambda.lastStatement
            pendingCoroutineStepOut = false
            pendingCoroutineResumeOriginSymbol = topSymbol.orEmpty()
            syncNativePendingResumeAutoContinue(enabled = true)
            positionMapper.parseSourcePosition(filePath, targetLine.coerceAtLeast(currentLine1Based))?.let { pos ->
                lastKnownMappableIdeSource = pos
            }
            pluginDbgPrintln(
                "[startStepOver] lambda last line — continue past resume (target $filePath:$targetLine)",
            )
            val ok = continueDiscardingStepPlans(threadId)
            if (!ok) {
                clearPendingCoroutineResume()
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }
        if (lambdaFrame) {
            pluginDbgPrintln("[startStepOver] coroutine lambda one-shot did not bind — step out")
            val stepped = serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Out)
            if (!stepped) {
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }
        // Only after a suspend-ish Step Over from a state machine may a leftover native plan
        // complete at `}`. Plain F8 (and false-positive call-site heuristics like
        // `p0EvalTopLevelByte()`) must not pay USB discard on every stop.
        leftoverNativeStepPlanLikely = atSuspendCall && onStateMachine
        val instructionLevel = useInstructionLevelStepping()
        pluginDbgPrintln(
            "[Coroutines][step] native step-over option=${if (instructionLevel) "InstructionOver" else "Over"} " +
                "atSuspendCall=$atSuspendCall stateMachine=$onStateMachine",
        )
        val option = if (instructionLevel) {
            LLDBServiceWrapper.StepOption.InstructionOver
        } else {
            LLDBServiceWrapper.StepOption.Over
        }
        val ok = serviceWrapper.resumeThread(threadId, option)
        if (!ok) {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
    }

    override fun startStepInto(context: XSuspendContext?) {
        beginStepPhaseTrace("INTO")
        executionGutterHighlighter.clearStepPreview()
        pluginDbgPrintln(
            "[startStepInto] ENTRY arkTs=${crossLanguageCoordinator?.isArkTsSuspended()} " +
                "backend=${crossLanguageCoordinator?.activeBackend} sessionSuspended=${session.isSuspended}",
        )
        CrossLangTrace.log(
            session,
            "startStepInto ENTRY arkTsSuspended=${crossLanguageCoordinator?.isArkTsSuspended()} " +
                "activeBackend=${crossLanguageCoordinator?.activeBackend}",
        )
        if (delegateExecutionToCrossLanguage { stepInto(it) }) {
            CrossLangTrace.log(session, "startStepInto delegated to CrossLanguageCoordinator")
            return
        }
        CrossLangTrace.log(session, "startStepInto fallback to LLDB (ArkTS not active)")
        startStepIntoLldb(context)
    }

    internal fun stepIntoLldbOnly(context: XSuspendContext?) = startStepIntoLldb(context)

    private fun startStepIntoLldb(context: XSuspendContext?) {
        stepPhaseMark("worker-start")
        stepPreviewAllowed = true
        val threadId = getCurrentThreadId()
        val suspended = threadId?.let { isThreadSuspended(it) } == true
        pluginDbgPrintln("[startStepInto] threadId=$threadId isThreadSuspended=$suspended")
        if (threadId == null || !suspended) {
            pluginDbgPrintln("[startStepInto] SKIP")
            return
        }
        val sourceLine = currentTopFrameSourceLineText()
        if (
            !org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector.looksLikeStepIntoCallSite(
                sourceLine,
            )
        ) {
            pluginDbgPrintln(
                "[startStepInto] no call site — delegate to step over sourceLine=${sourceLine?.trim()?.take(120)}",
            )
            printPluginDebugConsole(
                "[startStepInto] no call site on current line — Step Over (Kotlin/Android Studio parity)",
            )
            startStepOverLldb(context)
            return
        }
        if (!tryBeginUserContinueOrStep()) {
            stepPhaseMark("skip-not-stoppable")
            endStepPhaseTrace("abort")
            return
        }
        stepPhaseMark("session-check")
        onUserOwnedBreakpointPause("startStepInto")
        skipCalleeHeaderOnNextStepInto = true
        val printCallName = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
            .primaryCallNameOnLine(sourceLine)
        if (printCallName == "println" || printCallName == "print") {
            // `println("... ${box.business}")` evaluates the property getter before calling the
            // print function. Arm a breakpoint on the print function and Continue so Step Into
            // lands inside println/print instead of the getter.
            coroutineStepPipeline("step-into print call $printCallName — arm symbol catch")
            executeStepIntoBoundaryTarget(
                org.cpf.kmp.tools.ohos.debugger.stepinto.StepIntoTarget.Symbol(printCallName),
                threadId,
            )
            return
        }
        if (useInstructionLevelStepping()) {
            val option = if (
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector.looksLikeStepIntoCallSite(
                    sourceLine,
                )
            ) {
                LLDBServiceWrapper.StepOption.InstructionInto
            } else {
                LLDBServiceWrapper.StepOption.InstructionOver
            }
            printPluginDebugConsole("[startStepInto] Disassembly — instruction step option=$option")
            resumeInstructionLevelStep(threadId, option)
            return
        }
        val stepCtx = gatherCoroutineStepEditorContext()
        stepPhaseMark("editor-context")
        stepIntoOriginLine1Based = stepCtx.currentLine1Based
        stepIntoOriginFile = stepCtx.filePath
        pendingNativeStepInto = true
        leftoverNativeStepPlanLikely = false
        skipCalleeHeaderOnNextStepInto = true
        val lineForLog = stepCtx.sourceLine ?: sourceLine
        val atLibrarySuspend =
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector.looksLikeKnownLibrarySuspendCall(
                lineForLog,
            )
        val nextLine = stepCtx.nextLine
        printPluginDebugConsole(
            "[startStepInto] atLibrarySuspend=$atLibrarySuspend sourceLine=${lineForLog?.trim()} nextLine=$nextLine",
        )
        if (!atLibrarySuspend) {
            val calleeName = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
                .primaryCallNameOnLine(stepCtx.sourceLine)
            val bodyLine = calleeName?.let { findSuspendFunctionBodyLine(it, stepCtx.filePath) } ?: 0
            if (bodyLine > 0) {
                // Body line known before the USB arm — show the optimistic caret immediately.
                executionGutterHighlighter.previewStepTarget(stepCtx.filePath, bodyLine)
                // Native Step Into of a user suspend call lands in runtime glue (UpdateHeapRef /
                // suspend trampoline). Arm the callee's first body line and Continue instead.
                coroutineStepPipeline("step-into user suspend $calleeName — arm body line $bodyLine")
                executeStepIntoBoundaryTarget(
                    org.cpf.kmp.tools.ohos.debugger.stepinto.StepIntoTarget.MappedSourceLine(
                        absolutePath = stepCtx.filePath,
                        lldbLine = bodyLine,
                        stem = "",
                        cSymbol = "",
                    ),
                    threadId,
                )
                return
            }
        }
        // Step Into always uses native LLDB step-in (or boundary router), including on `delay()` /
        // `println()` — coroutine resume arming is Step Over only.
        if (!useInstructionLevelStepping()) {
            val boundaryTarget = stepIntoBoundaryRouter.resolve(buildStepIntoContext())
            if (boundaryTarget != null) {
                executeStepIntoBoundaryTarget(boundaryTarget, threadId)
                return
            }
        }
        resumeNativeStepInto(threadId)
    }

    private fun buildStepIntoContext(): StepIntoContext {
        val frames = lastSuspendedStackFrames.orEmpty()
        return StepIntoContext(
            project = session.project,
            topFrame = frames.firstOrNull(),
            ideSourcePosition = lastTopFrameIdeSourceOverride,
            sourceMapRules = injectedSourceMapRules,
        )
    }

    private fun findSuspendFunctionBodyLine(calleeName: String, filePath: String): Int {
        if (calleeName.isEmpty() || filePath.isEmpty()) return 0
        val policy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(filePath, line) }
        var line = 1
        while (true) {
            val text = lineTextAt(line) ?: return 0
            if (policy.isCalleeFunctionDefinitionLine(text, calleeName)) {
                return policy.firstBodyLineAfterSuspendFunctionHeader(line, lineTextAt)
            }
            line++
        }
    }

    private fun executeStepIntoBoundaryTarget(target: StepIntoTarget, threadId: Int) {
        when (target) {
            is StepIntoTarget.MappedSourceLine -> {
                pluginDbgPrintln(
                    "[stepIntoBoundary] MappedSourceLine stem=${target.stem} cSymbol=${target.cSymbol} " +
                        "file=\"${target.absolutePath}\" line=${target.lldbLine}",
                )
                serviceWrapper.setBreakpoint(target.absolutePath, target.lldbLine) { success, breakpointId ->
                    ApplicationManager.getApplication().invokeLater {
                        if (stopped || session.project.isDisposed) {
                            clearStepIntoBoundaryBreakpoint()
                            clearUserContinueOrStepPending()
                            return@invokeLater
                        }
                        if (!success || breakpointId == null || breakpointId <= 0) {
                            pluginDbgPrintln("[stepIntoBoundary] breakpoint set failed — fallback native step-into")
                            clearStepIntoBoundaryBreakpoint()
                            resumeNativeStepInto(threadId)
                            return@invokeLater
                        }
                        onStepTargetKnown(target.absolutePath, target.lldbLine)
                        armStepIntoBoundaryBreakpoint(breakpointId)
                        stepPhaseMark("resume-issue")
                        val ok = serviceWrapper.resumeThread(threadId, null)
                        if (!ok) {
                            clearStepIntoBoundaryBreakpoint()
                            clearUserContinueOrStepPending()
                            restoreIdePauseAfterFailedContinueOrStep(threadId)
                        }
                    }
                }
            }
            is StepIntoTarget.Symbol -> {
                pluginDbgPrintln("[stepIntoBoundary] Symbol name=${target.name}")
                if (prepareNativeCatchBeforeStepInto(target.name)) {
                    val ok = serviceWrapper.resumeThread(threadId, null)
                    if (!ok) {
                        clearNativeCatchBreakpoint()
                        clearUserContinueOrStepPending()
                        restoreIdePauseAfterFailedContinueOrStep(threadId)
                    }
                } else {
                    resumeNativeStepInto(threadId)
                }
            }
            is StepIntoTarget.Address -> {
                pluginDbgPrintln("[stepIntoBoundary] Address va=0x${target.va.toString(16)}")
                serviceWrapper.ensureThreadSelectedForNativeCatch()
                val ok = serviceWrapper.runToAddress(target.va)
                if (!ok) {
                    clearUserContinueOrStepPending()
                    restoreIdePauseAfterFailedContinueOrStep(threadId)
                }
            }
        }
    }

    private fun resumeNativeStepInto(threadId: Int) {
        // Falling back to a native step: the predicted line may not be reached.
        executionGutterHighlighter.clearStepPreview()
        val option = if (useInstructionLevelStepping()) {
            LLDBServiceWrapper.StepOption.InstructionInto
        } else {
            LLDBServiceWrapper.StepOption.Into
        }
        resumeInstructionLevelStep(threadId, option)
    }

    private fun resumeInstructionLevelStep(threadId: Int, option: LLDBServiceWrapper.StepOption) {
        stepPhaseMark("resume-issue instruction")
        leftoverNativeStepPlanLikely = false
        pluginDbgPrintln("[instruction-step] option=$option thread=$threadId")
        val ok = serviceWrapper.resumeThread(threadId, option)
        if (!ok) {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
    }

    private fun armStepIntoBoundaryBreakpoint(breakpointId: Int) {
        clearStepIntoBoundaryBreakpoint()
        stepIntoBoundaryBreakpointId = breakpointId
        stepIntoBoundaryPending = true
    }

    private fun clearStepIntoBoundaryBreakpoint() {
        stepIntoBoundaryPending = false
        val id = stepIntoBoundaryBreakpointId
        stepIntoBoundaryBreakpointId = null
        if (id != null && id > 0) {
            serviceWrapper.removeBreakpoint(id) {}
        }
    }

    /** @return true when this stop completes a cross-module Step Into boundary capture. */
    private fun consumeStepIntoBoundaryStop(): Boolean {
        if (!stepIntoBoundaryPending) return false
        stepIntoBoundaryPending = false
        val id = stepIntoBoundaryBreakpointId
        stepIntoBoundaryBreakpointId = null
        if (id != null && id > 0) {
            serviceWrapper.removeBreakpoint(id) {}
        }
        pluginDbgPrintln("[handleStopped] ✓ step-into boundary target reached")
        return true
    }

    override fun startStepOut(context: XSuspendContext?) {
        // Step Out is acceptable without the optimistic caret; never preview it.
        stepPreviewAllowed = false
        executionGutterHighlighter.clearStepPreview()
        if (delegateExecutionToCrossLanguage { stepOut(it) }) return
        startStepOutLldb(context)
    }

    internal fun stepOutLldbOnly(context: XSuspendContext?) = startStepOutLldb(context)

    private fun startStepOutLldb(context: XSuspendContext?) {
        stepPreviewAllowed = false
        executionGutterHighlighter.clearStepPreview()
        val threadId = getCurrentThreadId()
        val suspended = threadId?.let { isThreadSuspended(it) } == true
        pluginDbgPrintln("[startStepOut] threadId=$threadId isThreadSuspended=$suspended")
        if (threadId == null || !suspended) {
            pluginDbgPrintln("[startStepOut] SKIP")
            return
        }
        if (!tryBeginUserContinueOrStep()) return
        clearStickyUserSourcePause()
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        if (useInstructionLevelStepping()) {
            val callerFrame = lastSuspendedStackFrames?.firstOrNull { frame ->
                frame.file.isNotEmpty() && frame.line > 0 &&
                    !org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isLibraryGlueFrame(frame.name)
            }
            val callerText = callerFrame?.let { sourceLineTextAt(it.file, it.line) }
            val callerIsContinuationResume = callerText?.contains(".resume(") == true
            val callerIsLastStatement = callerFrame != null &&
                org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.resolveStepOverTarget(
                    callerFrame.line,
                    { line -> sourceLineTextAt(callerFrame.file, line) },
                ).lastStatement
            if (callerIsLastStatement && callerIsContinuationResume) {
                // Step Out from glue with no next continuation: Step Out to the call line then
                // immediately Continue → Running (not paused), matching a continuation from 483.
                coroutineStepPipeline(
                    "step-out disassembly — caller last statement; Continue (running/not paused)",
                )
                leftoverNativeStepPlanLikely = false
                val ok = continueDiscardingStepPlans(threadId)
                if (!ok) {
                    clearUserContinueOrStepPending()
                    restoreIdePauseAfterFailedContinueOrStep(threadId)
                }
                return
            }
            printPluginDebugConsole("[startStepOut] Disassembly — native step out")
            leftoverNativeStepPlanLikely = false
            val ok = serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Out)
            if (!ok) {
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }

        val ctx = gatherCoroutineStepEditorContext()
        val stepOutFn = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
            .suspendBodyFunctionNameFromSymbol(ctx.topSymbol)
        coroutineStepPipeline(
            "step-out start fn=$stepOutFn sm=${ctx.onStateMachine} " +
                "atSuspend=${ctx.atSuspendCall} line=${ctx.currentLine1Based} " +
                "next=${ctx.nextLine} lastStmt=${ctx.lastStatement} file=${ctx.filePath}",
        )
        pluginDbgPrintln(
            "[Coroutines][step-out] symbol=${ctx.topSymbol} stateMachine=${ctx.onStateMachine} " +
                "lambda=${ctx.lambdaFrame} atSuspendCall=${ctx.atSuspendCall} " +
                "current=${ctx.currentLine1Based} next=${ctx.nextLine} file=${ctx.filePath} " +
                "sourceLine=${ctx.sourceLine?.trim()}",
        )

        if (resumePastSuspendCallingConventionForStepOut(threadId)) {
            // [continueToCallerUserLine] already issued Continue.
            return
        }
        val suspendBridge = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
            .isKotlinSuspendCallingConventionFrame(ctx.topSymbol, ctx.topFile)
        if (suspendBridge) {
            printPluginDebugConsole(
                "[startStepOut] stopped in suspend bridge frame — use Step Over (F8) or Continue",
            )
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
            return
        }

        val stepPolicy = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy
        val suspendDetector = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineSuspendCallDetector
        if (ctx.atSuspendCall && suspendDetector.looksLikeKnownLibrarySuspendCall(ctx.sourceLine)) {
            val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(ctx.filePath, line) }
            val next = stepPolicy.resolveStepOverTarget(ctx.currentLine1Based, lineTextAt)
            val targetLine = when {
                next.nextLine1Based > ctx.currentLine1Based -> next.nextLine1Based
                ctx.nextLine > ctx.currentLine1Based -> ctx.nextLine
                else -> 0
            }
            // Step Out on `delay`/`await` lands on the next line only when the user placed a
            // breakpoint there (211→212 with BP on 212). Otherwise fall through to parent OUT.
            val userBpOnNextLine = breakpointHandler.findBreakpoint(ctx.filePath, targetLine) != null
            if (targetLine > ctx.currentLine1Based && ctx.filePath.isNotEmpty() && userBpOnNextLine) {
                coroutineStepPipeline(
                    "step-out library suspend ${ctx.currentLine1Based} → next line $targetLine " +
                        "(user breakpoint; Step Out again from there for parent)",
                )
                pendingCoroutineResume = true
                coroutineResumeAutoContinueCount = 0
                pendingCoroutineResumeLine = targetLine
                pendingCoroutineResumeFile = ctx.filePath
                pendingCoroutineResumeOriginLine = ctx.currentLine1Based
                pendingCoroutineResumeThreadId = threadId
                pendingCoroutineResumeLastStatement = false
                pendingCoroutineStepOut = false
                pendingCoroutineResumeCallSiteLine = 0
                pendingCoroutineResumeOriginSymbol = ctx.topSymbol.orEmpty()
                syncNativePendingResumeAutoContinue(enabled = true)
                runCatching {
                    breakpointHandler.suspendUserBreakpointForStep(ctx.filePath, ctx.currentLine1Based)
                }
                restoreBreakpointsAfterNativeCallSiteStepOut = true
                val armedLib = armCoroutineStep(
                    org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OVER,
                    threadId,
                    atSuspendCall = true,
                    nextLine = targetLine,
                    ctx.filePath,
                    lastStatement = false,
                )
                if (armedLib) {
                    val ok = continueDiscardingStepPlans(threadId)
                    if (!ok) {
                        clearPendingCoroutineResume()
                        clearUserContinueOrStepPending()
                        restoreIdePauseAfterFailedContinueOrStep(threadId)
                    }
                    return
                }
                clearPendingCoroutineResume()
            } else if (targetLine > ctx.currentLine1Based && !userBpOnNextLine) {
                coroutineStepPipeline(
                    "step-out library suspend skip line $targetLine — no user breakpoint; parent OUT",
                )
            }
        }
        if (stepPolicy.isCoroutineScopeLaunchHeaderLine(ctx.sourceLine) &&
            (ctx.onStateMachine || ctx.lambdaFrame) &&
            ctx.atSuspendCall &&
            ctx.filePath.isNotEmpty()
        ) {
            val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(ctx.filePath, line) }
            val closing = stepPolicy.launchLambdaClosingBraceLine(ctx.currentLine1Based, lineTextAt)
            val landing = closing + 1
            if (closing > 0 && landing > ctx.currentLine1Based) {
                coroutineStepPipeline(
                    "step-out launch header ${ctx.currentLine1Based} → after lambda $landing (machinery $closing)",
                )
                val armedLaunchHeader = armLaunchBlockStepOutPending(
                    threadId,
                    ctx,
                    ctx.filePath to landing,
                )
                if (armedLaunchHeader) {
                    runCatching {
                        breakpointHandler.suspendUserBreakpointForStep(ctx.filePath, ctx.currentLine1Based)
                    }
                    restoreBreakpointsAfterNativeCallSiteStepOut = true
                    coroutineStepPipeline(
                        "step-out launch-header native Step Out thread=$threadId (chase $closing)",
                    )
                    val ok = serviceWrapper.resumeThread(threadId, LLDBServiceWrapper.StepOption.Out)
                    if (!ok) {
                        clearPendingCoroutineResume()
                        clearUserContinueOrStepPending()
                        restoreIdePauseAfterFailedContinueOrStep(threadId)
                    } else if (pendingCoroutineStepOut) {
                        scheduleStepOutPendingWatchdog(threadId)
                    }
                    return
                }
            }
        }
        if (stepPolicy.isStepOutFromSuspendCallSiteInCaller(ctx.sourceLine, ctx.topSymbol)) {
            pluginDbgPrintln(
                "[startStepOut] suspend call-site in caller (KMP Android stack Step Out) — " +
                    "suppress callee user breakpoints below ${ctx.currentLine1Based}",
            )
            breakpointHandler.suspendUserBreakpointsInFileWithLineAfter(
                ctx.filePath,
                ctx.currentLine1Based,
            )
            runCatching {
                breakpointHandler.suspendUserBreakpointForStep(ctx.filePath, ctx.currentLine1Based)
            }
            leftoverNativeStepPlanLikely = false
            restoreBreakpointsAfterNativeCallSiteStepOut = true
            val option = if (useInstructionLevelStepping()) {
                LLDBServiceWrapper.StepOption.InstructionOver
            } else {
                LLDBServiceWrapper.StepOption.Out
            }
            val ok = serviceWrapper.resumeThread(threadId, option)
            if (!ok) {
                restoreBreakpointsAfterNativeCallSiteStepOut = false
                breakpointHandler.restoreStepSuspendedBreakpoints()
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }

        // Stepping out of the outermost suspend frame has no physical caller to return to; the
        // coroutine step targets the parent continuation instead.
        // Do not pass editor `lastStatement` into OUT — it is Step Over closing-`}` semantics (428).
        var armed = armCoroutineStep(
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepKind.OUT,
            threadId,
            ctx.atSuspendCall,
            ctx.nextLine,
            ctx.filePath,
            lastStatement = false,
        )
        // Resolve the source-level caller BEFORE concluding there is no parent to return to: for
        // the first statement of a suspend function (before its first suspend point) the coroutine
        // dump can report parent=0 even though the LLDB stack still has the caller state machine.
        val callerTarget = resolveStepOutCallerTarget(
            threadId,
            ctx.filePath,
            ctx.currentLine1Based,
            ctx.topSymbol.orEmpty(),
        )
        coroutineStepPipeline(
            "step-out callerTarget resolved=${callerTarget?.first ?: "null"}:" +
                "${callerTarget?.second ?: 0} (origin=${ctx.currentLine1Based} topFn=$stepOutFn)",
        )
        // Once the coroutine is known to be the outermost suspend frame there is no physical caller
        // to step out to. Do not arm caller breakpoints / resume-PC / native Step Out — just Continue
        // so the next user breakpoint (if any, e.g. a breakpoint below the current line) still stops.
        if (!armed && ctx.onStateMachine && !ctx.atSuspendCall &&
            coroutineSupport.stepOutParentContinuationAddress(threadId) == 0L &&
            callerTarget == null
        ) {
            stepOutFromTopestSuspendFunction(threadId)
            return
        }
        // Epilogue after a suspend resume (428): parent link is sometimes missing from the dump;
        // caller PC + editor line breakpoints from [armStepOutCallerEditorBreakpoints] still chase OUT.
        // Mid-body (173 before `delay`): Continue would enter the next suspend — use native Step Out.
        val callerOnlyOk = stepPolicy.stepOutAllowsCallerOnlyChase(ctx.lastStatement, ctx.nextText)
        if (!armed && callerTarget != null && !ctx.atSuspendCall && callerOnlyOk &&
            (ctx.onStateMachine || ctx.lambdaFrame)
        ) {
            val frameTriples = getLastSuspendedStackFrames().orEmpty().map { Triple(it.name, it.file, it.line) }
            val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(callerTarget.first, line) }
            val useLaunchMachinery = stepPolicy.stepOutUsesLaunchBlockMachinery(
                ctx.currentLine1Based,
                callerTarget.first,
                callerTarget.second,
                lineTextAt,
                frameTriples,
            )
            armed = if (useLaunchMachinery) {
                armLaunchBlockStepOutPending(threadId, ctx, callerTarget)
            } else {
                armStepOutViaCallerBreakpointsOnly(threadId, ctx)
            }
        } else if (!armed && (ctx.onStateMachine || ctx.lambdaFrame) && !ctx.atSuspendCall && !callerOnlyOk) {
            coroutineStepPipeline(
                "step-out skip caller-only — next line is suspend (line=${ctx.currentLine1Based} next=${ctx.nextLine})",
            )
            if (callerTarget != null) {
                val frameTriples = getLastSuspendedStackFrames().orEmpty().map { Triple(it.name, it.file, it.line) }
                val lineTextAt: (Int) -> String? = { line -> sourceLineTextAt(callerTarget.first, line) }
                if (!stepPolicy.stepOutUsesLaunchBlockMachinery(
                        ctx.currentLine1Based,
                        callerTarget.first,
                        callerTarget.second,
                        lineTextAt,
                        frameTriples,
                    )
                ) {
                    armed = armStepOutViaCallerBreakpointsOnly(threadId, ctx)
                }
            }
        }
        if (armed) {
            coroutineStepPipeline(
                "step-out armed pending caller $pendingCoroutineResumeFile:$pendingCoroutineResumeLine " +
                    "from origin $pendingCoroutineResumeOriginLine callerPc=$pendingCallerResumeArmed",
            )
            val originLine = ctx.currentLine1Based
            if (originLine > 0 && ctx.filePath.isNotEmpty()) {
                runCatching {
                    breakpointHandler.suspendUserBreakpointForStep(ctx.filePath, originLine)
                }.onFailure { t ->
                    coroutineStepPipeline(
                        "step-out disable user bp failed: ${t.javaClass.simpleName}: ${t.message}",
                    )
                }
                restoreBreakpointsAfterNativeCallSiteStepOut = true
            }
            if (launchBlockStepOutContinueChase) {
                launchBlockStepOutContinueChase = false
                // Step Over from `println` / stdlib steps into another .so and never hits the launch `{ }`
                // machinery line (394). Native Step Out pops to the `launch` lambda / caller SM first.
                // JNI exposes step-out only as [StepOption.Out] (no instruction-level Out).
                val option = LLDBServiceWrapper.StepOption.Out
                restoreBreakpointsAfterNativeCallSiteStepOut = true
                coroutineStepPipeline(
                    "step-out launch-block native Step Out thread=$threadId (avoid Step Over into stdlib; " +
                        "chase ${pendingLaunchBlockMachineryLine.coerceAtLeast(pendingCoroutineResumeLine)})",
                )
                val ok = serviceWrapper.resumeThread(threadId, option)
                if (!ok) {
                    coroutineStepPipeline(
                        "step-out launch-block Step Out failed — Continue to machinery line",
                    )
                    leftoverNativeStepPlanLikely = false
                    if (!continueDiscardingStepPlans(threadId)) {
                        clearPendingCoroutineResume()
                        clearUserContinueOrStepPending()
                        restoreIdePauseAfterFailedContinueOrStep(threadId)
                    } else if (pendingCoroutineStepOut) {
                        scheduleStepOutPendingWatchdog(threadId)
                    }
                } else if (pendingCoroutineStepOut) {
                    scheduleStepOutPendingWatchdog(threadId)
                }
                return
            }
            if (plainCallerStepOutNativeUnwind) {
                plainCallerStepOutNativeUnwind = false
                leftoverNativeStepPlanLikely = true
                restoreBreakpointsAfterNativeCallSiteStepOut = true
                coroutineStepPipeline(
                    "step-out plain caller native Step Out thread=$threadId " +
                        "(origin $originLine > call ${pendingCoroutineResumeCallSiteLine}; " +
                        "chase $pendingCoroutineResumeFile:$pendingCoroutineResumeLine)",
                )
                val option = if (useInstructionLevelStepping()) {
                    LLDBServiceWrapper.StepOption.InstructionOver
                } else {
                    LLDBServiceWrapper.StepOption.Out
                }
                val ok = serviceWrapper.resumeThread(threadId, option)
                if (!ok) {
                    leftoverNativeStepPlanLikely = false
                    coroutineStepPipeline(
                        "step-out plain caller Step Out failed — fallback Continue to caller BPs",
                    )
                    if (!continueDiscardingStepPlans(threadId)) {
                        clearPendingCoroutineResume()
                        clearUserContinueOrStepPending()
                        restoreIdePauseAfterFailedContinueOrStep(threadId)
                    } else if (pendingCoroutineStepOut) {
                        scheduleStepOutPendingWatchdog(threadId)
                    }
                } else if (pendingCoroutineStepOut) {
                    scheduleStepOutPendingWatchdog(threadId)
                }
                return
            }
            val ok = continueDiscardingStepPlans(threadId)
            if (!ok) {
                coroutineStepPipeline("step-out Continue failed — restore pause at $originLine")
                clearPendingCoroutineResume()
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            } else if (pendingCoroutineStepOut) {
                scheduleStepOutPendingWatchdog(threadId)
            }
            return
        }
        if (ctx.onStateMachine || ctx.lambdaFrame) {
            if (ctx.onStateMachine) {
                coroutineStepPipeline(
                    "step-out parent not armed — native Step Out fallback (epilogue before `}`?)",
                )
            } else {
                coroutineStepPipeline("step-out lambda frame — native Step Out fallback")
            }
            resolveStepOutCallerTarget(
                threadId,
                ctx.filePath,
                ctx.currentLine1Based,
                ctx.topSymbol.orEmpty(),
            )?.let { (callerFile, callSiteLine) ->
                pendingCoroutineResume = true
                coroutineResumeAutoContinueCount = 0
                assignStepOutCallerPending(callerFile, callSiteLine)
                pendingCoroutineResumeOriginLine = ctx.currentLine1Based
                pendingCoroutineResumeThreadId = threadId
                pendingCoroutineResumeLastStatement = false
                pendingCoroutineStepOut = true
                pendingCoroutineResumeOriginSymbol = ctx.topSymbol.orEmpty()
                syncNativePendingResumeAutoContinue(enabled = true)
                coroutineStepPipeline(
                    "step-out native unwind pending caret $callerFile:$callSiteLine→$pendingCoroutineResumeLine",
                )
            }
            if (ctx.filePath.isNotEmpty() && ctx.currentLine1Based > 0) {
                runCatching {
                    breakpointHandler.suspendUserBreakpointForStep(ctx.filePath, ctx.currentLine1Based)
                }
            }
            leftoverNativeStepPlanLikely = false
            restoreBreakpointsAfterNativeCallSiteStepOut = true
            val option = if (useInstructionLevelStepping()) {
                LLDBServiceWrapper.StepOption.InstructionOver
            } else {
                LLDBServiceWrapper.StepOption.Out
            }
            val ok = serviceWrapper.resumeThread(threadId, option)
            if (!ok) {
                restoreBreakpointsAfterNativeCallSiteStepOut = false
                clearPendingCoroutineResume()
                breakpointHandler.restoreStepSuspendedBreakpoints()
                clearUserContinueOrStepPending()
                restoreIdePauseAfterFailedContinueOrStep(threadId)
            }
            return
        }
        coroutineStepPipeline(
            "step-out could not arm — stay here (try Step Over on delay/suspend lines first)",
        )
        clearUserContinueOrStepPending()
        restoreIdePauseAfterFailedContinueOrStep(threadId)
    }

    override fun resume(context: XSuspendContext?) {
        // Plain Continue: never show a provisional step caret.
        stepPreviewAllowed = false
        executionGutterHighlighter.clearStepPreview()
        pluginDbgPrintln(
            "[resume] ENTRY arkTs=${crossLanguageCoordinator?.isArkTsSuspended()} " +
                "sessionSuspended=${session.isSuspended}",
        )
        val coord = crossLanguageCoordinator
        if (coord != null && OhosKmpCrossLanguageDebug.isEnabled() &&
            (coord.isArkTsSuspended() || coord.activeBackend == CrossLanguageBackend.ARK_TS)
        ) {
            coord.resume(context)
            return
        }
        resumeLldbOnly(context)
    }

    /**
     * User Continue / Resume Program — same as a normal debugger Continue: run until the next
     * user breakpoint (or process exit). Must cancel coroutine Step Over chasing and leftover
     * native steppers; otherwise Continue lands on the one-shot next line / SM `}` instead.
     */
    internal fun resumeLldbOnly(context: XSuspendContext?) {
        if (!tryBeginUserContinueOrStep()) return
        clearStickyUserSourcePause()
        userSuspendCallBpStepOverAllowed = false
        skipCalleeHeaderOnNextStepInto = false
        clearPendingNativeStepInto()
        KotlinEvalResumeGate.request()
        // Cancel in-flight Step Over (pending + one-shot) before Continue.
        clearPendingCoroutineResume()
        // Always discard nested StepOver/Into plans on user Continue (not only when the
        // previous F8 was a suspend SM step). Leftover plans make Continue finish at `}`.
        leftoverNativeStepPlanLikely = true
        val threadId = getCurrentThreadId()
        if (threadId == null) {
            clearUserContinueOrStepPending()
            return
        }
        listener.resetBreakpointStopHandling()
        pluginDbgPrintln("[resume] Continue → next user breakpoint (cleared sticky/pending/step plans)")
        val ok = continueDiscardingStepPlans(threadId)
        if (ok) {
            clearSuspendedStackCache()
        } else {
            clearUserContinueOrStepPending()
            restoreIdePauseAfterFailedContinueOrStep(threadId)
        }
    }

    /**
     * Request an async interrupt (LLDB `SBProcess::Stop`) on the inferior. Runs JNI off the EDT.
     * The remote lldb-server process keeps running; only the debuggee is stopped.
     */
    override fun startPausing() {
        if (stopped) return
        clearStickyUserSourcePause()
        pluginDbgPrintln("[startPausing] LLDB SBProcess::Stop() — interrupt inferior; device lldb-server keeps serving")
        ApplicationManager.getApplication().executeOnPooledThread {
            if (stopped || session.project.isDisposed) return@executeOnPooledThread
            try {
                serviceWrapper.pauseThread(0)
            } catch (e: Exception) {
                LOG.warn("startPausing failed: ${e.message}", e)
            }
        }
    }

    override fun getBreakpointHandlers(): Array<XBreakpointHandler<*>> {
        return if (OhosKmpCrossLanguageDebug.isEnabled()) {
            arrayOf(kotlinBreakpointHandler, arkTsBreakpointHandler)
        } else {
            arrayOf(kotlinBreakpointHandler)
        }
    }

    override fun getEditorsProvider(): XDebuggerEditorsProvider {
        return LLDBEditorsProvider()
    }

    /**
     * Do not use the IDE built-in library-frame funnel; use the toggle in [registerAdditionalActions] and [hideLibraryExternalFrames] (per session).
     */
    override fun isLibraryFrameFilterSupported(): Boolean = false

    override fun registerAdditionalActions(
        leftToolbar: DefaultActionGroup,
        topToolbar: DefaultActionGroup,
        settings: DefaultActionGroup
    ) {
        // Coroutine actions live on the Debug tool window "Coroutines" tab, same place as
        // KMP Android. The left toolbar stays for stepping and threads.

        // Platform "Pause Program" is enabled only after attach + event loop (setPauseActionSupported(true)).
        topToolbar.addSeparator()
        topToolbar.add(object : ToggleAction(
            "Toggle External Frames",
            "",
            AllIcons.General.Filter
        ) {
            override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

            override fun isSelected(e: AnActionEvent): Boolean =
                this@LLDBDebugProcess.isHideLibraryExternalFrames()

            override fun setSelected(e: AnActionEvent, state: Boolean) {
                this@LLDBDebugProcess.setHideLibraryExternalFrames(state)
                val proc = this@LLDBDebugProcess
                ApplicationManager.getApplication().invokeLater {
                    proc.requestFramesListRefresh()
                }
            }

            override fun update(e: AnActionEvent) {
                val proc = this@LLDBDebugProcess
                val enabled = proc.isSuspendToolbarActionsEnabled()
                e.presentation.isEnabled = enabled
                val hiding = proc.isHideLibraryExternalFrames()
                e.presentation.description = if (hiding) {
                    "Show all stack frames in the list (library and external are hidden now). Display only; LLDB frame indices unchanged."
                } else {
                    "Hide library and external stack frames (all frames are shown now). Display only; LLDB frame indices unchanged."
                }
                if (enabled) {
                    super.update(e)
                } else {
                    Toggleable.setSelected(e.presentation, false)
                }
            }
        })
        topToolbar.add(object : ToggleAction(
            "Show All Threads",
            "",
            AllIcons.Actions.GroupBy
        ) {
            override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

            override fun isSelected(e: AnActionEvent): Boolean =
                this@LLDBDebugProcess.isShowAllThreadsInList()

            override fun setSelected(e: AnActionEvent, state: Boolean) {
                this@LLDBDebugProcess.setShowAllThreadsInList(state)
                this@LLDBDebugProcess.refreshThreadListDebugOutput(includeConsole = true)
            }

            override fun update(e: AnActionEvent) {
                val proc = this@LLDBDebugProcess
                val enabled = proc.isSuspendToolbarActionsEnabled()
                e.presentation.isEnabled = enabled
                val showAll = proc.isShowAllThreadsInList()
                e.presentation.description = if (showAll) {
                    "Thread list shows all threads (fetches full list from LLDB; toggle off for preview only)."
                } else {
                    "Thread list shows first ${THREAD_LIST_PREVIEW_COUNT} threads only (toggle on to fetch and list all)."
                }
                if (enabled) {
                    super.update(e)
                } else {
                    Toggleable.setSelected(e.presentation, false)
                }
            }
        })
        if (OhosKmpCrossLanguageDebug.isEnabled()) {
            settings.addSeparator()
            settings.add(org.cpf.kmp.tools.ohos.debugger.actions.ShowArkTsStackFrameAction())
        }
    }


    /**
     * 2026.1 split debugger 禁止在停住路径上调用 [XDebugSession.getUI]。
     * 协程页必须从这里注册，才会出现在用户看到的调试区里。
     */
    override fun createTabLayouter(): XDebugTabLayouter = object : XDebugTabLayouter() {
        override fun registerAdditionalContent(ui: RunnerLayoutUi) {
            installCoroutinesDebugTab(ui)
        }
    }

    /**
     * Debug tool-window tab next to Console / LLDB. Official KMP Android puts coroutine
     * inspection in a Coroutines tab; this is the same slot for Harmony LLDB sessions.
     */
    private fun installCoroutinesDebugTab(ui: RunnerLayoutUi) {
        if (stopped || session.project.isDisposed) return
        val app = ApplicationManager.getApplication()
        if (!app.isDispatchThread) {
            app.invokeLater { installCoroutinesDebugTab(ui) }
            return
        }
        coroutinesLayoutUi = ui
        if (coroutinesDebugTabRegistered && coroutinesDebugPanel != null && coroutinesDebugContent != null) {
            coroutinesDebugPanel?.bindDebugProcess(this)
            return
        }
        try {
            var panel = coroutinesDebugPanel
                ?: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel(
                    session.project,
                    showDebugActions = true,
                    boundProcess = this,
                )
            coroutinesDebugPanel = panel
            panel.bindDebugProcess(this)

            val existing = ui.contents.firstOrNull { it.displayName == "Coroutines" }
            val content = when {
                existing?.component is org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel -> {
                    panel = existing.component as org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel
                    coroutinesDebugPanel = panel
                    panel.bindDebugProcess(this)
                    existing
                }
                existing != null -> {
                    ui.removeContent(existing, false)
                    addCoroutinesDebugContent(ui, panel)
                }
                else -> addCoroutinesDebugContent(ui, panel)
            }
            coroutinesDebugContent = content
            coroutinesDebugTabRegistered = true
            attachCoroutinesTabSelectionListener(content, panel)
            pluginDbgPrintln("[Coroutines][tab] installed on debugger layout (reused=${existing != null})")
            applyCoroutinesTabStopState(panel)
        } catch (e: Exception) {
            LOG.warn("Failed to add Coroutines debug tab: ${e.message}")
        }
    }

    private fun addCoroutinesDebugContent(
        ui: RunnerLayoutUi,
        panel: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel,
    ): Content {
        val content = ui.createContent("CoroutinesDebug", panel, "Coroutines", null, panel)
        content.isCloseable = false
        ui.addContent(content)
        return content
    }

    private fun attachCoroutinesTabSelectionListener(
        content: Content,
        panel: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel,
    ) {
        if (coroutinesTabSelectionListenerAttached) return
        coroutinesTabSelectionListenerAttached = true
        content.addPropertyChangeListener { event ->
            if (event.propertyName == "isSelected" && content.isSelected) {
                panel.refresh()
            }
        }
    }

    private fun applyCoroutinesTabStopState(
        panel: org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutinesToolWindowPanel,
    ) {
        val frames = lastSuspendedStackFrames ?: return
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val frame = frames.firstOrNull { classifier.isCoroutineStopFrame(it.name) }
        val label = frame?.let { classifier.coroutineDebugLabel(it.name) }
        panel.noteCoroutineStop(label)
        if (coroutinesDebugContent?.isSelected == true) {
            panel.refresh()
        }
    }

    private fun notifyCoroutinesDebugTab(stackFrames: List<StackFrame>, reason: String?) {
        val classifier = org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
        val frame = stackFrames.firstOrNull { classifier.isCoroutineStopFrame(it.name) }
        val label = frame?.let { classifier.coroutineDebugLabel(it.name) }
        // A user breakpoint hit is a natural place to populate the Coroutines tab even when it is
        // not the currently focused tab; otherwise the first coroutine stop always looks empty until
        // the user clicks Refresh.
        val userBreakpoint = reason?.equals("breakpoint", ignoreCase = true) == true
        // 推迟到 positionReached 之后，避免 split debugger 在挂起上下文建立前切页。
        ApplicationManager.getApplication().invokeLater {
            if (stopped || session.project.isDisposed) return@invokeLater
            if (!coroutinesDebugTabRegistered) {
                coroutinesLayoutUi?.let { installCoroutinesDebugTab(it) }
            }
            val panel = coroutinesDebugPanel ?: run {
                pluginDbgPrintln("[Coroutines][tab] stop before layout ready label=$label")
                return@invokeLater
            }
            panel.noteCoroutineStop(label)
            val ui = coroutinesLayoutUi ?: return@invokeLater
            val content = coroutinesDebugContent
            val selected = content?.isSelected == true
            if (selected || (label != null && userBreakpoint)) {
                if (!selected) {
                    pluginDbgPrintln("[Coroutines][tab] auto-refresh on user breakpoint ($label)")
                }
                panel.refresh()
            } else {
                focusThreadsAndVariablesTab(ui)
                pluginDbgPrintln("[Coroutines][tab] skip device dump — tab not selected")
            }
        }
    }

    /**
     * Step Over 停住后留在 Threads & Variables。
     * Coroutines 页只刷新数据，不抢焦点。
     */
    private fun focusThreadsAndVariablesTab(ui: RunnerLayoutUi) {
        val threads = ui.contents.firstOrNull { content ->
            val name = content.displayName
            name.contains("Thread", ignoreCase = true) ||
                name.contains("Variable", ignoreCase = true) ||
                name == "Frames"
        } ?: return
        try {
            ui.selectAndFocus(threads, false, false)
            pluginDbgPrintln("[Coroutines][tab] keep ${threads.displayName}")
        } catch (e: Exception) {
            LOG.warn("Failed to select Threads & Variables: ${e.message}")
        }
    }

    // ========== LLDB Terminal ==========

    fun getLLDBTerminalView(): LLDBTerminalView? = lldbTerminalView

    private fun registerLLDBTerminalTab() {
        if (lldbTerminalTabRegistered || stopped || session.project.isDisposed) return
        val ui = session.ui
        if (ui == null) {
            scheduleLLDBTerminalTabRegistrationRetry()
            return
        }
        try {
            val existing = ui.contents.firstOrNull { it.displayName == "LLDB" }
            if (existing != null) {
                lldbTerminalContent = existing
                lldbTerminalTabRegistered = true
                return
            }
            lldbTerminalView = lldbTerminalView ?: LLDBTerminalView(session.project, this)
            val content = ui.createContent(
                "LLDBTerminal", lldbTerminalView!!.component, "LLDB",
                null, lldbTerminalView!!.preferredFocusableComponent
            )
            content.isCloseable = false
            lldbTerminalContent = content
            ui.addContent(content)
            lldbTerminalTabRegistered = true
            lldbTabRegisterRetryCount = 0
        } catch (e: Exception) {
            LOG.error("Failed to add LLDB Terminal tab", e)
        }
        if (!lldbTerminalTabRegistered) {
            scheduleLLDBTerminalTabRegistrationRetry()
        }
    }

    private fun scheduleLLDBTerminalTabRegistrationRetry() {
        if (lldbTerminalTabRegistered || stopped || session.project.isDisposed) return
        if (lldbTabRegisterRetryCount >= 20) {
            LOG.warn("LLDB terminal tab registration skipped: session UI not ready after retries")
            return
        }
        lldbTabRegisterRetryCount++
        lldbTabRegisterAlarm.cancelAllRequests()
        val delayMs = if (lldbTabRegisterRetryCount <= 10) 150 else 300
        lldbTabRegisterAlarm.addRequest({ registerLLDBTerminalTab() }, delayMs)
    }

    fun getLLDBDisassemblyView(): LLDBDisassemblyView? = lldbDisassemblyView

    private fun disassemblyTabFileName(): String =
        "Disassembly-${Integer.toHexString(session.hashCode())}.${DisassemblyFileType.DEFAULT_EXTENSION}"

    private fun isLightVirtualFile(file: VirtualFile): Boolean =
        LIGHT_VIRTUAL_FILE_CLASS?.isInstance(file) == true

    /**
     * Prefer an in-memory LightVirtualFile so the tab never depends on a `.idea` stub on disk.
     * Falls back to a tiny `.idea` stub only when LightVirtualFile is not loadable from this classpath.
     */
    private fun createDisassemblyTabVirtualFile(): VirtualFile {
        val name = disassemblyTabFileName()
        val lightClass = LIGHT_VIRTUAL_FILE_CLASS
        if (lightClass != null) {
            try {
                val ctor = lightClass.getConstructor(String::class.java, com.intellij.openapi.fileTypes.FileType::class.java, CharSequence::class.java)
                @Suppress("UNCHECKED_CAST")
                return ctor.newInstance(name, DisassemblyFileType, "") as VirtualFile
            } catch (e: Exception) {
                LOG.warn("LightVirtualFile construction failed (${e.message}); using .idea stub for Disassembly tab")
            }
        }
        return createOrRefreshDisassemblyDiskStubVirtualFile(name)
    }

    private fun createOrRefreshDisassemblyDiskStubVirtualFile(filename: String): VirtualFile {
        val base = session.project.basePath ?: throw IllegalStateException("project has no base path")
        val ideaDir = Paths.get(base, ".idea")
        Files.createDirectories(ideaDir)
        val path = ideaDir.resolve(filename)
        Files.writeString(path, "", StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)
        val lfs = LocalFileSystem.getInstance()
        return lfs.refreshAndFindFileByIoFile(path.toFile())
            ?: throw IllegalStateException("could not resolve VFS for $path")
    }

    private fun isDisassemblyDiskStub(file: VirtualFile): Boolean {
        if (isLightVirtualFile(file)) return false
        val base = session.project.basePath ?: return false
        return try {
            Paths.get(file.path).startsWith(Paths.get(base, ".idea"))
        } catch (_: Exception) {
            file.path.contains("/.idea/")
        }
    }

    /**
     * Step Out / focus can race workspace restore or prior session cleanup that deleted the `.idea` stub.
     * Recreate the stub and re-bind user data before [FileEditorManager.openFile].
     */
    private fun ensureDisassemblyEditorFileValid(): VirtualFile? {
        var vf = disassemblyEditorFile
        if (vf == null) {
            prepareDisassemblyEditorTab()
            vf = disassemblyEditorFile
        }
        vf ?: return null
        if (isLightVirtualFile(vf)) {
            if (vf.getUserData(DisassemblyEditorKeys.DEBUG_PROCESS) == null) {
                vf.putUserData(DisassemblyEditorKeys.DEBUG_PROCESS, this)
            }
            return vf
        }
        if (!isDisassemblyDiskStub(vf)) {
            return vf
        }
        val ioFile = java.io.File(vf.path)
        if (ioFile.isFile) {
            if (vf.getUserData(DisassemblyEditorKeys.DEBUG_PROCESS) == null) {
                vf.putUserData(DisassemblyEditorKeys.DEBUG_PROCESS, this)
            }
            return vf
        }
        LOG.warn("Disassembly stub missing on disk (${vf.path}); recreating")
        return try {
            val refreshed = createOrRefreshDisassemblyDiskStubVirtualFile(vf.name)
            refreshed.putUserData(DisassemblyEditorKeys.DEBUG_PROCESS, this)
            disassemblyEditorFile = refreshed
            refreshed
        } catch (e: Exception) {
            LOG.error("Failed to recreate Disassembly stub", e)
            null
        }
    }

    /**
     * Creates the disassembly view and virtual file for this session. Does not open an editor tab yet —
     * the tab is opened lazily in [focusDisassemblyEditor] when the debugger stops where IDE source is unavailable,
     * so starting / attaching does not show an empty Disassembly tab (and IDE reopen does not resurrect stray tabs
     * when paired with [org.cpf.kmp.tools.ohos.debugger.debugui.CloseStaleHarmonyDisassemblyEditorsActivity]).
     */
    private fun prepareDisassemblyEditorTab() {
        try {
            lldbDisassemblyView = LLDBDisassemblyView(this)
            val vf = createDisassemblyTabVirtualFile()
            vf.putUserData(DisassemblyEditorKeys.DEBUG_PROCESS, this)
            disassemblyEditorFile = vf
        } catch (e: Exception) {
            LOG.error("Failed to create Disassembly editor file", e)
        }
    }

    // ========== Stop event handling ==========

    /**
     * Kotlin/Native inserts runtime frames (e.g. [Kotlin_mm_safePointFunctionPrologue]) between user calls.
     * Strip them so recursion heuristics see repeated user functions.
     */
    private fun isLikelyKotlinRuntimeGlue(functionName: String): Boolean {
        if (functionName.isEmpty() || functionName == "unknown") return true
        if (functionName.startsWith("Kotlin_mm_")) return true
        if (functionName.startsWith("Kotlin_N2KStub")) return true
        if (functionName.startsWith("Kotlin_K2NStub")) return true
        if (functionName.startsWith("_konan_function")) return true
        if (functionName.startsWith("kfun:kotlin.")) return true
        if (functionName.startsWith("kfun:kotlinx.")) return true
        if (functionName.startsWith("kfun:androidx.")) return true
        if (functionName.startsWith("kfun:org.jetbrains.")) return true
        return org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier.isLibraryGlueFrame(functionName)
    }

    /**
     * Detect runaway recursion / stack blow-up so the IDE does not spin on huge identical stack traces.
     * Uses consecutive identical (name, file, line) only — not global name counts.
     */
    private fun looksLikeRunawayRecursion(frames: List<StackFrame>): Boolean {
        if (frames.size < RECURSION_MIN_NON_RUNTIME_FRAMES) return false
        val core = frames.filter { !isLikelyKotlinRuntimeGlue(it.name) }
        if (core.size < RECURSION_MIN_NON_RUNTIME_FRAMES) return false

        var bestRun = 1
        var run = 1
        for (i in 1 until core.size) {
            val a = core[i]
            val b = core[i - 1]
            val identical = a.name == b.name && a.file == b.file && a.line == b.line &&
                a.file.isNotEmpty() && a.line > 0
            if (identical) {
                run++
            } else {
                bestRun = maxOf(bestRun, run)
                run = 1
            }
        }
        bestRun = maxOf(bestRun, run)
        return bestRun >= RECURSION_CONSECUTIVE_IDENTICAL_FRAMES
    }

    /**
     * Whether this stop should surface in the IDE. Internal/linker stops typically have no file on any frame.
     * Kotlin/Native + LLDB often report **line==0** with a valid **file** after Step Over/Into — do not auto-continue.
     */
    private fun stackHasOpenableSourceForUi(stackFrames: List<StackFrame>): Boolean {
        if (stackFrames.isEmpty()) return false
        return stackFrames.any { frameHasMappableIdeSource(it) }
    }

    /**
     * User-driven stops (often [reason] `"signal"`), watchpoints, exceptions: do not auto-continue as
     * "internal" stops when no host-mappable source line exists on any frame.
     */
    private fun shouldKeepSuspendedDespiteNoOpenableSource(reason: String): Boolean {
        return when (reason.lowercase()) {
            "signal", "watchpoint", "exception", "breakpoint", "step" -> true
            else -> false
        }
    }

    private fun resolveStopThreadId(incomingThreadId: Int): Int {
        if (incomingThreadId > 0) return incomingThreadId
        serviceWrapper.getCurrentThreadId()?.takeIf { it > 0 }?.let { return it }
        return resolveEffectiveThreadIdForRecover(0)
    }

    private fun handleStopped(threadId: Int, reason: String, stackFrames: List<StackFrame>) {
        val t0 = if (OhosKmpStepTiming.isEnabled()) System.nanoTime() else 0L
        try {
            handleStoppedImpl(threadId, reason, stackFrames)
        } finally {
            if (OhosKmpStepTiming.isEnabled()) {
                val ms = (System.nanoTime() - t0) / 1_000_000.0
                stepDbgPrintln(
                    "[TIMING][step] handleStopped total ms=${String.format(java.util.Locale.US, "%.3f", ms)} " +
                        "reason=$reason frames=${stackFrames.size}",
                )
            }
        }
    }

    private fun handleStoppedImpl(threadId: Int, reason: String, stackFrames: List<StackFrame>) {
        // Elapsed Time subtracts the app execution span (continue JNI returned → STOPPED
        // received) ONLY for coroutine stops. Synchronous / multithread debugging keeps the
        // previous statistics (plain wall clock).
        val runStart = executionGutterHighlighter.runStartNanosOrNull()
        val issued = lastContinueIssuedNs
        val arrived = lastStopArrivedNs
        val coroutineStop = stackFrames.firstOrNull()?.name?.let {
            org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineFrameClassifier
                .isNativeCoroutineUserStackFrame(it)
        } == true
        lastStepAppRunMs =
            if (coroutineStop && runStart != null && issued > 0L && arrived > issued) {
                (arrived - issued) / 1_000_000L
            } else {
                0L
            }
        lastStepAppRunRunStartNs = runStart ?: -1L
        stepPhaseMark("stop-event")
        if (CrossLangJumpTiming.isActive()) {
            CrossLangJumpTiming.mark(
                session,
                "handleStopped EDT reason=$reason frames=${stackFrames.size} " +
                    "top=${stackFrames.firstOrNull()?.name ?: "?"}",
            )
        }
        pluginDbgPrintln("\n========== [LLDBDebugProcess.handleStopped] ==========")
        pluginDbgPrintln("[handleStopped] threadId=$threadId reason=$reason frames=${stackFrames.size}")
        pluginDbgPrintln(
            "[handleStopped] pre-state: stopped=$stopped sessionSuspended=${session.isSuspended} " +
                "cachedCurrentThreadId=${serviceWrapper.getCurrentThreadId()}",
        )
        val topPreview = stackFrames.firstOrNull()
        // Before positionReached. The Kotlin expression compile starts from that UI pass and
        // otherwise holds LLDB until k2native finishes (139 → 140 was ~11s of that wait).
        syncCoroutineExpressionCompile(stackFrames)
        coroutineStepLog(
            "stop thread=$threadId reason=$reason frames=${stackFrames.size} " +
                "top=${topPreview?.name} ${topPreview?.file}:${topPreview?.line} " +
                "pending=$pendingCoroutineResume target=$pendingCoroutineResumeFile:$pendingCoroutineResumeLine " +
                "origin=$pendingCoroutineResumeOriginLine skips=$coroutineResumeAutoContinueCount",
        )

        if (leaveSuspendCallingConvention(threadId, stackFrames, reason)) return

        var eventThreadId = threadId
        val topBeforePin = stackFrames.firstOrNull()
        if (stepOverSuspendCallPinLine > 0) {
            coroutineStepPipeline(
                "stop with suspend-call pin target $stepOverSuspendCallPinFile:$stepOverSuspendCallPinLine " +
                    "top=${topBeforePin?.file}:${topBeforePin?.line} pending=$pendingCoroutineResume " +
                    "reason=$reason",
            )
        }
        var eventFrames = applyStepOverSuspendCallTargetPin(stackFrames)

        // This STOPPED arrived from waitForEvent. JNI no longer Continues park/probe, so the
        // process is actually paused even when `lldbProcessRunning` is still true from the
        // previous Step/Continue.
        lldbProcessRunning = false
        pendingResumeContinueInFlight = false

        val afterSkip = applyProbeOrPendingResumeSkip(eventThreadId, eventFrames, reason) ?: return
        eventThreadId = afterSkip.first
        eventFrames = afterSkip.second

        clearUserContinueOrStepPending()
        val effectiveTid = resolveStopThreadId(eventThreadId)
        if (effectiveTid > 0 && effectiveTid != eventThreadId) {
            pluginDbgPrintln("[handleStopped] resolved effectiveTid=$effectiveTid (incoming=$eventThreadId)")
            serviceWrapper.setCurrentThreadId(effectiveTid)
        }
        val tidForUi = if (effectiveTid > 0) effectiveTid else eventThreadId

        if (crossLanguageCoordinator?.shouldSuppressIncidentalLldbStop() == true) {
            if (CrossLangJumpTiming.isActive()) {
                CrossLangJumpTiming.mark(
                    session,
                    "incidental stop suppressed reason=$reason top=${stackFrames.firstOrNull()?.name ?: "?"}",
                )
            }
            pluginDbgPrintln(
                "[handleStopped] suppress incidental LLDB stop while ArkTS paused " +
                    "(reason=$reason file=${stackFrames.firstOrNull()?.file ?: "?"})",
            )
            val resumeTid = if (tidForUi > 0) tidForUi else 0
            if (resumeTid > 0) {
                isolateResumed(resumeTid)
            }
            serviceWrapper.resumeThread(resumeTid, null)
            ApplicationManager.getApplication().invokeLater {
                if (!stopped && !session.project.isDisposed) {
                    crossLanguageCoordinator?.reassertArkTsSuspendUi()
                }
            }
            pluginDbgPrintln("==========================================================\n")
            return
        }

        // From here on, use the (possibly rewritten) frames after coroutine skip policy.
        @Suppress("NAME_SHADOWING")
        val stackFrames = eventFrames

        isolateSuspended(tidForUi)
        if (stackFrames.isEmpty()) {
            if (lldbProcessRunning) {
                pluginDbgPrintln("[handleStopped] ✗ empty stack while process already running — skip getStackTrace recover")
                return
            }
            if (nativeCatchHandoffPending || nativeLldbHandoffInFlight) {
                pluginDbgPrintln(
                    "[handleStopped] empty stack with native catch/handoff pending — recovering stack",
                )
            } else {
                pluginDbgPrintln("[handleStopped] ✗ empty stack from waitForEvent; trying getStackTrace fallback once")
            }
            recoverStoppedStackFromThread(tidForUi, reason)
            return
        }

        // Discard only after we know this is a real user-visible stop with frames.
        if (!isLibraryGlueStop(stackFrames)) {
            discardLeftoverNativeStepPlanIfNeeded()
        }

        if (looksLikeRunawayRecursion(stackFrames)) {
            LOG.warn("Runaway recursion detected (${stackFrames.size} frames); stopping debug session")
            pluginDbgPrintln("[handleStopped] runaway recursion detected, stopping session")
            try {
                session.consoleView?.print(
                    "Harmony LLDB: Debug session stopped — stack indicates deep recursion " +
                        "(same Kotlin function repeated many times). Fix the loop to continue debugging.\n",
                    ConsoleViewContentType.ERROR_OUTPUT
                )
            } catch (_: Throwable) {
            }
            session.stop()
            return
        }

        val firstFrame = stackFrames[0]
        pluginDbgPrintln("[handleStopped] top frame: file=\"${firstFrame.file}\" line=${firstFrame.line} name=${firstFrame.name}")

        consumeStepIntoBoundaryStop()

        val nativeStop = consumeNativeCatchOrHandoffStop(firstFrame)
        val nativeCatchStop = nativeStop.catchStop
        val nativeHandoff = nativeStop.addressHandoff

        // Filter system internal stops (e.g. _dl_debug_state, linker breakpoints)
        // Not user breakpoints: all frames have empty file and line 0 — auto-continue
        var hasUserFrame = stackHasOpenableSourceForUi(stackFrames)
        if (!hasUserFrame && pendingNativeStepInto && isLibraryGlueStop(stackFrames)) {
            hasUserFrame = true
            pluginDbgPrintln("[handleStopped] step-into glue — surface Disassembly (no quiet-continue)")
        }
        if (!hasUserFrame && launchBlockSyntheticCaretPendingUi && lastKnownMappableIdeSource != null) {
            hasUserFrame = true
            pluginDbgPrintln("[handleStopped] launch-block synthetic caret — force user stop UI")
        }
        if (!hasUserFrame) {
            if (stickyUserSourcePause && pendingCoroutineResume) {
                pluginDbgPrintln("[handleStopped] no openable source but sticky + pending resume — hold, no Continue")
                pluginDbgPrintln("==========================================================\n")
                return
            }
            if (stickyUserSourcePause) {
                clearStickyUserSourcePause()
                pluginDbgPrintln("[handleStopped] sticky epilogue at glue — release and auto-continue")
            }
            if (org.cpf.kmp.tools.ohos.debugger.coroutine.CoroutineStepStopPolicy.isUnspecifiedStopReason(reason)) {
                pluginDbgPrintln(
                    "[handleStopped] unspecified stop with no source — recover stack, do not Continue past user BP",
                )
                isolateSuspended(tidForUi)
                recoverStoppedStackFromThread(tidForUi, reason)
                pluginDbgPrintln("==========================================================\n")
                return
            }
            if ((isLibraryGlueStop(stackFrames) || leftoverNativeStepPlanLikely) && !pendingNativeStepInto) {
                pluginDbgPrintln(
                    "[handleStopped] glue/no-source ${firstFrame.name} reason=$reason — " +
                        "auto-continue, do not steal execution caret",
                )
                isolateResumed(tidForUi)
                if (lldbProcessRunning) {
                    pluginDbgPrintln("[handleStopped] glue already running — skip extra Continue")
                    pluginDbgPrintln("==========================================================\n")
                    return
                }
                continuePendingResumeQuietly(tidForUi)
                pluginDbgPrintln("==========================================================\n")
                return
            }
            if (!shouldKeepSuspendedDespiteNoOpenableSource(reason)) {
                if (lldbProcessRunning) {
                    pluginDbgPrintln("[handleStopped] ✗ no openable source but process already running — skip extra Continue")
                    pluginDbgPrintln("==========================================================\n")
                    return
                }
                pluginDbgPrintln("[handleStopped] ✗ no openable source on any frame (system internal: ${firstFrame.name}), auto-continue")
                isolateResumed(tidForUi)
                serviceWrapper.resumeThread(tidForUi, null)
                pluginDbgPrintln("==========================================================\n")
                return
            }
            pluginDbgPrintln("[handleStopped] no mappable source but keeping suspend (reason=$reason)")
        }

        val hitBreakpoint = if (reason == "breakpoint") {
            breakpointHandler.findBreakpoint(firstFrame.file, firstFrame.line)
        } else null
        if (reason == "breakpoint") {
            // TODO(eval-native-cache): Session 首次 breakpoint + continue 后再命中的 breakpoint
            // 调度 KotlinEvalNativeCache 补暖（不可用「整 session 只跑一次」挡死）。
            // 见 docs/EVAL_NATIVE_CACHE_WARMUP.md §4。本期仅 attach 后台暖。
            pluginDbgPrintln("[handleStopped] breakpoint stop, looking up matching breakpoint...")
            pluginDbgPrintln("[handleStopped] findBreakpoint: ${if (hitBreakpoint != null) "found" else "not found"}")
            if (hitBreakpoint != null && !hitBreakpoint.isEnabled) {
                pluginDbgPrintln("[handleStopped] IDE breakpoint disabled, auto-continue")
                isolateResumed(tidForUi)
                serviceWrapper.resumeThread(tidForUi, null)
                pluginDbgPrintln("==========================================================\n")
                return
            }
            if (hitBreakpoint != null) {
                val condition = breakpointHandler.getBreakpointCondition(hitBreakpoint)
                if (condition != null && condition.isNotBlank()) {
                    pluginDbgPrintln("[handleStopped] breakpoint condition: $condition")
                    evaluateCondition(condition, firstFrame.id) { result ->
                        if (result) {
                            showBreakpointHit(tidForUi, stackFrames, hitBreakpoint)
                        } else {
                            isolateResumed(tidForUi)
                            serviceWrapper.resumeThread(tidForUi, null)
                        }
                    }
                    return
                }
            }
        }

        if (reason == "breakpoint" && hitBreakpoint != null) {
            val cond = breakpointHandler.getBreakpointCondition(hitBreakpoint)
            if (cond.isNullOrBlank()) {
                val idePos = hitBreakpoint.sourcePosition
                val file = idePos?.file?.path.orEmpty()
                val line = (idePos?.line ?: -1) + 1
                if (file.isNotEmpty() && line > 0) {
                    onUserOwnedBreakpointPause("user breakpoint $file:$line")
                }
            }
        }

        try {
            try {
                session.setPauseActionSupported(true)
            } catch (_: Throwable) {
            }
            val (headerFrames, headerHighlight) = applyStepIntoCalleeHeaderPresentation(stackFrames, reason)
            val afterEpilogue = applyStepIntoEpilogueLineRewrite(headerFrames, reason)
            var uiFrames = rewriteTopFrameLineIfPendingTarget(afterEpilogue)
            uiFrames = applyStepOverSuspendCallTargetPin(uiFrames)
            if (pendingCoroutineStepOut) {
                val shown = uiFrames.firstOrNull()
                if (shown != null && shown.line == pendingCoroutineResumeLine &&
                    (frameFileMatchesPending(shown.file) || shown.file.isEmpty())
                ) {
                    onUserOwnedBreakpointPause("step-out caller shown")
                    pendingCoroutineStepOut = false
                }
            }
            val presentation = planStopPresentation(
                reason, uiFrames, hitBreakpoint, nativeHandoff, nativeCatchStop,
            )
            val executionHighlight = headerHighlight ?: presentation.executionHighlightSource
            glueDisassemblyInstructionStepping = presentation.glueInstructionStepping
            if (pendingNativeStepInto) {
                clearPendingNativeStepInto()
            }
            if (presentation.steppedAwayFromUserCode) {
                notifySteppedAwayFromUserCode(firstFrame, presentation.executionHighlightSource)
            }
            pluginDbgPrintln(
                "[handleStopped] calling session.positionReached()... preferDisassembly=${presentation.preferDisassembly} " +
                    "glueDisassemblyInstructionStepping=$glueDisassemblyInstructionStepping " +
                    "steppedAway=${presentation.steppedAwayFromUserCode} " +
                    "highlight=${executionHighlight?.file?.name}:" +
                    "${(executionHighlight?.line ?: -1) + 1} " +
                    "nativeHandoff=$nativeHandoff nativeCatchStop=$nativeCatchStop",
            )
            // Frames may have been rewritten after the early skip. Refresh the compile gate
            // before the UI evaluates anything.
            syncCoroutineExpressionCompile(uiFrames)
            // Invalidate the previous coroutine snapshot and (when enabled) start the new walk
            // before positionReached, so the prefetch overlaps with the UI building the thread stack.
            coroutineSupport.onStopped()
            crossLanguageCoordinator?.onLldbUserStopPresented()
            val suspendContext = crossLanguageCoordinator?.wrapLldbSuspendContext(
                tidForUi,
                uiFrames,
                executionHighlight,
            ) ?: LLDBSuspendContext(this, tidForUi, uiFrames, executionHighlight)
            rememberSuspendedStack(tidForUi, uiFrames, executionHighlight, reason)
            if (stepOverSuspendCallSpilledLocals) {
                try {
                    coroutineSupport.readCurrentSpilledLocals()
                    coroutineStepPipeline("prefetch current-spilled for step-over pin locals")
                } catch (t: Throwable) {
                    coroutineStepLog("prefetch current-spilled failed: ${t.message}")
                }
            }
            lldbDisassemblyView?.onDebuggerStopped(reason, forceRefresh = presentation.preferDisassembly)
            positionReachedNotifySuspend(
                suspendContext,
                attract = presentation.steppedAwayFromUserCode ||
                    !presentation.preferDisassembly || nativeHandoff || nativeCatchStop,
            )
            stepPhaseMark("caret")
            endStepPhaseTrace("surfaced")
            if (launchBlockSyntheticCaretPendingUi) {
                launchBlockSyntheticCaretPendingUi = false
                coroutineStepPipeline(
                    "step-out launch-block UI stop at ${presentation.executionHighlightSource?.file?.name}:" +
                        "${(presentation.executionHighlightSource?.line ?: -1) + 1}",
                )
            }
            try {
                session.rebuildViews()
            } catch (_: Throwable) {
            }
            if (presentation.preferDisassembly) {
                focusDisassemblyEditor(forceFocus = true)
            } else {
                dismissAutoDisassemblyEditorIfNeeded()
            }
            if (reason.equals("signal", ignoreCase = true)) {
                try {
                    session.consoleView?.print(
                        "Paused (interrupt). Top frame: ${firstFrame.name}\n",
                        ConsoleViewContentType.SYSTEM_OUTPUT,
                    )
                } catch (_: Throwable) {
                }
            }
            if (nativeHandoff || nativeCatchStop) {
                try {
                    session.showExecutionPoint()
                } catch (_: Throwable) {
                }
                CrossLangJumpTiming.end(
                    session,
                    "ui ready nativeHandoff=$nativeHandoff nativeCatch=$nativeCatchStop top=${firstFrame.name}",
                )
            }
            KotlinEvalResumeGate.clear()
            suspendCallingConventionHops = 0
            pluginDbgPrintln("[handleStopped] ✓ positionReached OK")
            if (reason == "breakpoint" && hitBreakpoint != null) {
                session.setBreakpointVerified(hitBreakpoint)
                pluginDbgPrintln("[handleStopped] ✓ setBreakpointVerified OK")
            }
            notifyConnectedOnce()
        } catch (e: Exception) {
            pluginDbgPrintln("[handleStopped] ✗ exception: ${e.message}")
            e.printStackTrace()
        }
        pluginDbgPrintln("==========================================================\n")
    }

    /**
     * Step/continue can occasionally deliver STOPPED with empty [stackFrames] (race/path filter/native transient).
     * Try one direct JNI stack query before giving up, then either refresh UI or continue if still no user source.
     */
    private fun recoverStoppedStackFromThread(threadId: Int, reason: String) {
        ApplicationManager.getApplication().executeOnPooledThread {
            if (stopped || session.project.isDisposed) return@executeOnPooledThread
            val effectiveThreadId = resolveEffectiveThreadIdForRecover(threadId)
            if (effectiveThreadId <= 0) {
                pluginDbgPrintln("[recoverStoppedStack] ✗ no valid thread id to recover stack (incoming=$threadId)")
                ApplicationManager.getApplication().invokeLater {
                    if (stopped || session.project.isDisposed) return@invokeLater
                    try {
                        session.rebuildViews()
                    } catch (_: Throwable) {
                    }
                }
                return@executeOnPooledThread
            }
            pluginDbgPrintln(
                "[recoverStoppedStack] start: incomingTid=$threadId effectiveTid=$effectiveThreadId reason=$reason",
            )
            val selected = serviceWrapper.selectThread(effectiveThreadId)
            pluginDbgPrintln("[recoverStoppedStack] selectThread($effectiveThreadId)=$selected")
            serviceWrapper.setCurrentThreadId(effectiveThreadId)
            // Force getStackTrace() to refetch selected thread handle instead of reusing stale cached one.
            serviceWrapper.updateCurrentThreadHandle(0L)
            serviceWrapper.getStackTrace(effectiveThreadId) { recovered ->
                ApplicationManager.getApplication().invokeLater {
                    if (stopped || session.project.isDisposed) return@invokeLater
                    try {
                        pluginDbgPrintln("[recoverStoppedStack] recoveredFrames=${recovered.size}")
                        if (recovered.isEmpty()) {
                            pluginDbgPrintln("[recoverStoppedStack] ✗ still empty after getStackTrace; rebuilding views only")
                            try {
                                session.rebuildViews()
                            } catch (_: Throwable) {
                            }
                            return@invokeLater
                        }

                        val first = recovered[0]
                        pluginDbgPrintln(
                            "[recoverStoppedStack] top frame: file=\"${first.file}\" line=${first.line} name=${first.name}",
                        )
                        val nativeStop = consumeNativeCatchOrHandoffStop(first)
                        val hasUserFrame = stackHasOpenableSourceForUi(recovered)
                        if (!hasUserFrame && !shouldKeepSuspendedDespiteNoOpenableSource(reason) &&
                            !nativeStop.catchStop && !nativeStop.addressHandoff
                        ) {
                            pluginDbgPrintln(
                                "[recoverStoppedStack] ✗ no openable source after fallback (reason=$reason), auto-continue",
                            )
                            isolateResumed(effectiveThreadId)
                            val ok = serviceWrapper.resumeThread(effectiveThreadId, null)
                            pluginDbgPrintln("[recoverStoppedStack] auto-continue sent: success=$ok")
                            return@invokeLater
                        }
                        if (!hasUserFrame && shouldKeepSuspendedDespiteNoOpenableSource(reason)) {
                            pluginDbgPrintln("[recoverStoppedStack] no mappable source but keeping suspend (reason=$reason)")
                        }

                        val presentation = planStopPresentation(
                            reason, recovered, null, nativeStop.addressHandoff, nativeStop.catchStop,
                        )
                        glueDisassemblyInstructionStepping = presentation.glueInstructionStepping
                        if (presentation.steppedAwayFromUserCode) {
                            notifySteppedAwayFromUserCode(first, presentation.executionHighlightSource)
                        }
                        pluginDbgPrintln(
                            "[recoverStoppedStack] ✓ recovered user stack, calling session.positionReached() " +
                                "preferDisassembly=${presentation.preferDisassembly} " +
                                "glueDisassemblyInstructionStepping=$glueDisassemblyInstructionStepping " +
                                "steppedAway=${presentation.steppedAwayFromUserCode} nativeStop=$nativeStop",
                        )
                        coroutineSupport.onStopped()
                        crossLanguageCoordinator?.onLldbUserStopPresented()
                        val suspendContext = crossLanguageCoordinator?.wrapLldbSuspendContext(
                            effectiveThreadId,
                            recovered,
                            presentation.executionHighlightSource,
                        ) ?: LLDBSuspendContext(this, effectiveThreadId, recovered, presentation.executionHighlightSource)
                        rememberSuspendedStack(effectiveThreadId, recovered, presentation.executionHighlightSource, reason)
                        lldbDisassemblyView?.onDebuggerStopped(reason, forceRefresh = presentation.preferDisassembly)
                        positionReachedNotifySuspend(
                            suspendContext,
                            attract = presentation.steppedAwayFromUserCode ||
                                !presentation.preferDisassembly ||
                                nativeStop.catchStop || nativeStop.addressHandoff,
                        )
                        if (presentation.preferDisassembly) {
                            focusDisassemblyEditor(forceFocus = true)
                        } else {
                            dismissAutoDisassemblyEditorIfNeeded()
                        }
                        if (reason.equals("signal", ignoreCase = true)) {
                            try {
                                session.consoleView?.print(
                                    "Paused (interrupt). Top frame: ${first.name}\n",
                                    ConsoleViewContentType.SYSTEM_OUTPUT,
                                )
                            } catch (_: Throwable) {
                            }
                        }
                        notifyConnectedOnce()
                    } catch (e: Exception) {
                        pluginDbgPrintln("[recoverStoppedStack] ✗ exception: ${e.message}")
                        LOG.warn("recoverStoppedStackFromThread failed: ${e.message}", e)
                    }
                }
            }
        }
    }

    private fun resolveEffectiveThreadIdForRecover(incomingThreadId: Int): Int {
        if (incomingThreadId > 0) return incomingThreadId
        return try {
            val threads = serviceWrapper.getThreadList()
            if (threads.isEmpty()) {
                pluginDbgPrintln("[recoverStoppedStack] thread list is empty")
                return 0
            }
            val current = threads.firstOrNull { it.isCurrent && it.threadId > 0 }?.threadId
            val suspended = threads.firstOrNull { it.isSuspended && it.threadId > 0 }?.threadId
            val any = threads.firstOrNull { it.threadId > 0 }?.threadId ?: 0
            val picked = current ?: suspended ?: any
            pluginDbgPrintln(
                "[recoverStoppedStack] resolve tid from thread list: current=${current ?: 0} " +
                    "suspended=${suspended ?: 0} any=${any} picked=$picked total=${threads.size}",
            )
            picked
        } catch (e: Exception) {
            pluginDbgPrintln("[recoverStoppedStack] resolveEffectiveThreadId failed: ${e.message}")
            0
        }
    }

    private fun showBreakpointHit(threadId: Int, stackFrames: List<StackFrame>, breakpoint: com.intellij.xdebugger.breakpoints.XLineBreakpoint<*>) {
        try {
            val idePos = breakpoint.sourcePosition
            val file = idePos?.file?.path.orEmpty()
            val line = (idePos?.line ?: -1) + 1
            if (file.isNotEmpty() && line > 0) {
                onUserOwnedBreakpointPause("user breakpoint $file:$line")
            }
            val preferDisassembly = shouldPreferDisassemblyOnStop("breakpoint", stackFrames)
            coroutineSupport.onStopped()
            crossLanguageCoordinator?.onLldbUserStopPresented()
            val suspendContext = crossLanguageCoordinator?.wrapLldbSuspendContext(
                threadId,
                stackFrames,
                idePos,
            ) ?: LLDBSuspendContext(this, threadId, stackFrames, idePos)
            rememberSuspendedStack(threadId, stackFrames, idePos, "breakpoint")
            lldbDisassemblyView?.onDebuggerStopped("breakpoint", forceRefresh = preferDisassembly)
            positionReachedNotifySuspend(suspendContext, attract = !preferDisassembly)
            if (preferDisassembly) {
                focusDisassemblyEditor(forceFocus = true)
            } else {
                dismissAutoDisassemblyEditorIfNeeded()
            }
            session.setBreakpointVerified(breakpoint)
            notifyConnectedOnce()
        } catch (e: Exception) {
            LOG.error("Failed to show breakpoint hit", e)
        }
    }

    private fun evaluateCondition(condition: String, frameIndex: Int, callback: (Boolean) -> Unit) {
        LOG.info("Evaluating condition: $condition, frameIndex=$frameIndex")

        val result = serviceWrapper.evaluateExpressionJni(condition, frameIndex)
        pluginDbgPrintln(
            "[evaluateCondition] success=${result.success} value=${result.value} error=${result.errorMessage}",
        )
        if (!result.success) {
            try {
                session.consoleView?.print(
                    "Harmony LLDB: failed to evaluate breakpoint condition '$condition': " +
                        "${result.errorMessage ?: "unknown error"}; continuing.\n",
                    ConsoleViewContentType.ERROR_OUTPUT,
                )
            } catch (_: Throwable) {
            }
            callback(false)
            return
        }

        val value = result.value?.lowercase() ?: ""
        val condResult = when {
            value.contains("true") -> true
            value.contains("false") -> false
            value == "1" -> true
            value == "0" -> false
            else -> true
        }
        callback(condResult)
    }

    private fun notifyConnectedOnce() {
        if (firstStopNotified) return
        firstStopNotified = true
        session.consoleView?.print(
            "Connected successfully\n",
            com.intellij.execution.ui.ConsoleViewContentType.SYSTEM_OUTPUT
        )
        registerLLDBTerminalTab()
        switchToFramesTab()
    }

    private fun switchToFramesTab() {
        ApplicationManager.getApplication().invokeLater {
            try {
                val ui = session.ui ?: return@invokeLater
                for (content in ui.contents) {
                    if (content.displayName == "Frames") {
                        ui.selectAndFocus(content, true, true)
                        return@invokeLater
                    }
                }
            } catch (e: Exception) {
                LOG.warn("Failed to switch to Frames tab: ${e.message}")
            }
        }
    }

    private fun focusDisassemblyEditor(forceFocus: Boolean = false) {
        val gen = ++disassemblyFocusGeneration
        val app = ApplicationManager.getApplication()
        // A no-source stop must actually bring the Disassembly tab to the front;
        // userPinned only governs "keep the tab sticky + instruction-level stepping".
        val requestFocus = forceFocus
        val focusRunnable = Runnable {
            if (session.project.isDisposed || gen != disassemblyFocusGeneration) return@Runnable
            val vf = ensureDisassemblyEditorFileValid() ?: return@Runnable
            try {
                closeStaleDisassemblyEditorTabs(except = vf)
                val fem = FileEditorManager.getInstance(session.project)
                val openWithFocus = requestFocus || (!fem.isFileOpen(vf) && disassemblyEditorUserPinned)
                disassemblyEditorProgrammaticFocus = true
                try {
                    fem.openFile(vf, openWithFocus)
                } finally {
                    disassemblyEditorProgrammaticFocus = false
                }
            } catch (e: Exception) {
                LOG.warn("Failed to focus Disassembly editor: ${e.message}")
            }
        }
        app.invokeLater(focusRunnable)
        if (requestFocus) {
            // Run again after positionReached / rebuildViews EDT work so Disassembly is not overwritten by source tab.
            app.invokeLater {
                if (gen == disassemblyFocusGeneration) {
                    focusRunnable.run()
                }
            }
        }
    }

    /** Closes auto-opened Disassembly tab when the stop maps back to project source (avoids tab ping-pong). */
    private fun dismissAutoDisassemblyEditorIfNeeded() {
        if (disassemblyEditorUserPinned) return
        ++disassemblyFocusGeneration
        val vf = disassemblyEditorFile ?: return
        ApplicationManager.getApplication().invokeLater {
            if (session.project.isDisposed || disassemblyEditorUserPinned) return@invokeLater
            try {
                val fem = FileEditorManager.getInstance(session.project)
                if (fem.isFileOpen(vf)) {
                    fem.closeFile(vf)
                }
            } catch (_: Throwable) {
            }
        }
    }

    /**
     * Instruction-level stepping when the stop landed on glue (see [glueDisassemblyInstructionStepping]), or when
     * the user pinned and focused the Disassembly editor tab.
     */
    private fun useInstructionLevelStepping(): Boolean =
        glueDisassemblyInstructionStepping ||
            (disassemblyEditorUserPinned && isDisassemblyEditorActive())

    internal fun onDisassemblyEditorUserSelected() {
        disassemblyEditorUserPinned = true
    }

    internal fun isDisassemblyEditorProgrammaticFocus(): Boolean = disassemblyEditorProgrammaticFocus

    /** True if currently focused editor tab is this debug session's disassembly file. */
    internal fun isDisassemblyEditorActive(): Boolean {
        val disasm = ensureDisassemblyEditorFileValid() ?: return false
        return try {
            val selected = FileEditorManager.getInstance(session.project).selectedEditor?.file
            selected != null && (selected == disasm || selected.name == disasm.name)
        } catch (_: Throwable) {
            false
        }
    }

    /** Closes orphan `.ohosdisasm` tabs (workspace restore / deleted stub) that would trigger NoSuchFileException. */
    private fun closeStaleDisassemblyEditorTabs(except: VirtualFile) {
        try {
            val fem = FileEditorManager.getInstance(session.project)
            for (file in fem.openFiles) {
                if (file == except) continue
                if (!DisassemblyFileType.DEFAULT_EXTENSION.equals(file.extension, ignoreCase = true)) continue
                val bound = file.getUserData(DisassemblyEditorKeys.DEBUG_PROCESS)
                val diskMissing = !isLightVirtualFile(file) && isDisassemblyDiskStub(file) &&
                    !java.io.File(file.path).isFile
                if (bound == null || diskMissing) {
                    try {
                        fem.closeFile(file)
                    } catch (_: Throwable) {
                    }
                }
            }
        } catch (_: Throwable) {
        }
    }

    /**
     * True when the top stack frame does not correspond to editable project source (dependency path on another machine,
     * unresolved VFS, or file outside project base).
     * Compose/Kotlin names like `<inlined-lambda>` still map to user `.kt` — check [frameHasMappableIdeSource] first.
     *
     * Do not use [ProjectFileIndex.isInContent] alone: KMP `commonMain` paths often resolve via [LLDBPositionMapper]
     * but are not marked "content", which used to open Disassembly while [XDebugSession.positionReached] opened the
     * same `.kt` tab (editor tab ping-pong).
     */
    private fun topFrameHasNoIdeSourceToStepIn(topFrame: StackFrame): Boolean {
        if (frameHasMappableIdeSource(topFrame)) return false
        if (topFrame.file.isBlank()) return true
        if (isInlinedSyntheticFrameName(topFrame.name)) return true

        val lldbLine = topFrame.line
        val pos = when {
            lldbLine > 0 -> positionMapper.parseSourcePosition(topFrame.file, lldbLine)
            else -> positionMapper.parseSourcePosition(topFrame.file, 1)
        }
        if (pos == null) {
            pluginDbgPrintln("[disassembly] top frame has no IDE VirtualFile: file=\"${topFrame.file}\" line=$lldbLine")
            return true
        }

        if (isLldbSourcePathUnderProjectBase(topFrame.file)) {
            return false
        }

        if (virtualFileIsProjectUserSource(pos.file)) {
            return false
        }

        pluginDbgPrintln("[disassembly] top frame outside project tree: lldb=\"${topFrame.file}\" ide=${pos.file.path}")
        return true
    }

    /** CI / dependency paths from the Kotlin/Native build machine — not editable project sources in the IDE. */
    private fun isRemoteOrDependencyBuildSourcePath(lldbPath: String): Boolean {
        val p = lldbPath.trim()
        if (p.startsWith("/opt/")) return true
        if (p.contains("/.gradle/")) return true
        return false
    }

    /** Project-owned `.kt` / sources the user can open in the editor (excludes library & Gradle cache). */
    private fun virtualFileIsProjectUserSource(vf: VirtualFile): Boolean {
        if (session.project.isDisposed || !vf.isValid) return false
        val fileIndex = ProjectRootManager.getInstance(session.project).fileIndex
        if (fileIndex.isInLibraryClasses(vf) || fileIndex.isInLibrarySource(vf)) return false
        if (fileIndex.isInContent(vf)) return true
        if (fileIndex.isInSourceContent(vf)) {
            val base = session.project.basePath ?: return false
            return vf.path.startsWith(base)
        }
        return false
    }

    /**
     * True when LLDB [frame] resolves to an IDE [VirtualFile] the user can step in (project tree or content roots).
     * Used so `<inlined-lambda>` at `App.kt:40` does not auto-open Disassembly.
     */
    private fun frameHasMappableIdeSource(frame: StackFrame): Boolean {
        if (frame.file.isBlank()) return false
        val lineForParse = if (frame.line > 0) frame.line else 1
        if (OhosSourceJarDebugPolicy.isUnderDebugSources(frame.file)) {
            return positionMapper.parseSourcePosition(frame.file, lineForParse) != null
        }
        if (isRemoteOrDependencyBuildSourcePath(frame.file)) return false
        val pos = positionMapper.parseSourcePosition(frame.file, lineForParse) ?: return false
        if (isLldbSourcePathUnderProjectBase(frame.file)) return true
        return virtualFileIsProjectUserSource(pos.file)
    }

    /**
     * Source position for the stack-top execution highlight — top frame only (same as Dart/Flutter VM Service).
     * No fallback to lower frames: that pins the yellow marker on stale project lines while stepping library code.
     */
    private fun resolveTopFrameIdeSource(
        stackFrames: List<StackFrame>,
        hitBreakpoint: com.intellij.xdebugger.breakpoints.XLineBreakpoint<*>?,
    ): XSourcePosition? {
        hitBreakpoint?.sourcePosition?.let { return it }
        if (stackFrames.isEmpty()) return null
        val top = stackFrames[0]
        if (!frameHasMappableIdeSource(top)) return null
        val topLine = if (top.line > 0) top.line else 1
        return positionMapper.parseSourcePosition(top.file, topLine)
    }

    private fun isInlinedSyntheticFrameName(name: String): Boolean {
        val n = name.trim()
        return n.contains("<inlined-out", ignoreCase = true) || n.contains("<inlined-lambda", ignoreCase = true)
    }

    private fun isLldbSourcePathUnderProjectBase(lldbFilePath: String): Boolean {
        val projectBase = session.project.basePath ?: return false
        return try {
            val framePath = Paths.get(lldbFilePath).toAbsolutePath().normalize()
            val basePath = Paths.get(projectBase).toAbsolutePath().normalize()
            framePath.startsWith(basePath)
        } catch (_: Exception) {
            lldbFilePath.startsWith(projectBase)
        }
    }

    /** Top frame has openable project source → source editor; otherwise → Disassembly (simple rule). */
    private fun shouldPreferDisassemblyOnStop(reason: String, stackFrames: List<StackFrame>): Boolean {
        val reasonOk = when (reason.lowercase()) {
            "step", "breakpoint", "signal", "watchpoint", "exception" -> true
            else -> false
        }
        if (!reasonOk || stackFrames.isEmpty()) return false
        val top = stackFrames[0]
        val prefer = !frameHasMappableIdeSource(top)
        if (prefer) {
            pluginDbgPrintln(
                "[disassembly] top frame has no project source → Disassembly " +
                    "(reason=$reason top=${top.name} file=\"${top.file}\")",
            )
        }
        return prefer
    }

    // ========== Thread state ==========

    fun isolateSuspended(threadId: Int) { suspendedThreads[threadId] = true }
    fun isolateResumed(threadId: Int) { suspendedThreads.remove(threadId) }

    /**
     * LLDB reported process running again (matches Dart [EventKind.Resume] → [DartVmServiceDebugProcess.isolateResumed]).
     * Clears [suspendedThreads] so Resume/Step guards do not see a stale suspended state after continue/step (rapid clicks).
     */
    internal fun markEngineRunning() {
        if (stopped) return
        if (holdStickyPauseOnRunning()) {
            pluginDbgPrintln("[markEngineRunning] skip — sticky/pending coroutine pause")
            return
        }
        if (crossLanguageCoordinator?.isArkTsSuspended() == true) {
            pluginDbgPrintln("[markEngineRunning] skip — ArkTS CDP is suspended")
            return
        }
        suspendedThreads.clear()
        resetCoroutinesDebugTabToIdle()
    }

    /**
     * When the inferior resumes (Running, not paused), the Coroutines tab must return to its
     * default idle state instead of keeping the previous snapshot / tree expansion.
     */
    private fun resetCoroutinesDebugTabToIdle() {
        val panel = coroutinesDebugPanel ?: return
        ApplicationManager.getApplication().invokeLater {
            if (stopped || session.project.isDisposed) return@invokeLater
            panel.resetIdle()
        }
    }
    fun isThreadSuspended(threadId: Int): Boolean {
        if (threadId == ARK_TS_VIRTUAL_THREAD_ID &&
            crossLanguageCoordinator?.isArkTsSuspended() == true
        ) {
            return true
        }
        return suspendedThreads.containsKey(threadId)
    }
    fun getCurrentThreadId(): Int? {
        if (crossLanguageCoordinator?.isArkTsSuspended() == true) {
            return ARK_TS_VIRTUAL_THREAD_ID
        }
        return serviceWrapper.getCurrentThreadId()
    }
    fun getServiceWrapper(): LLDBServiceWrapper = serviceWrapper
    fun getDebugSession(): XDebugSession = session
    fun getBreakpointHandler(): LLDBBreakpointHandler = breakpointHandler
    fun getPositionMapper(): LLDBPositionMapper = positionMapper

    internal fun getCrossLanguageCoordinator(): CrossLanguageDebugCoordinator? = crossLanguageCoordinator

    internal fun getArkTsDebugBackendOrNull(): ArkTsDebugBackend? = arkTsDebugBackend

    internal fun getMultiInstanceManagerOrNull(): ArkTsMultiInstanceManager? = multiInstanceManager

    internal fun debugModeContextOrNull(): DebugModeContext? = debugModeContext

    internal fun onWorkerDebugProcessStopped(instanceId: Int) {
        multiInstanceManager?.onWorkerProcessStopped(instanceId)
    }

    /**
     * hdc fport for ArkTS debugger server.
     * Main: `ark:{pid}@Debugger`; Worker: `ark:{pid}@{instanceId}@Debugger`.
     */
    internal fun setupDebuggerPortForwardingPublic(
        deviceId: String,
        pid: String,
        tcpPort: Int,
        instanceId: Int = 0,
    ): Boolean = setupPortForwarding(deviceId, pid, tcpPort, instanceId)

    internal fun removeDebuggerPortForwardingPublic(
        deviceId: String,
        tcpPort: Int,
        pid: String,
        instanceId: Int = 0,
    ) {
        val arkTarget = if (instanceId == 0) {
            "ark:$pid@Debugger"
        } else {
            "ark:$pid@$instanceId@Debugger"
        }
        try {
            val hdcPath = org.cpf.kmp.tools.ohos.sync.DevEcoPathService.getInstance(session.project).getHdcPath()
            executeHdcCommand(hdcPath, deviceId, "fport", "rm", "tcp:$tcpPort", arkTarget)
            pluginDbgPrintln("[MultiInstance] fport rm tcp:$tcpPort $arkTarget")
        } catch (e: Exception) {
            LOG.warn("removeDebuggerPortForwarding failed: ${e.message}")
        }
    }

    internal fun notifyArkTsPositionReached(
        context: XSuspendContext,
        stackTopSource: XSourcePosition? = null,
        editorHighlightSource: XSourcePosition? = stackTopSource,
    ) {
        val deliver = Runnable {
            if (stopped || session.project.isDisposed) return@Runnable
            isolateSuspended(ARK_TS_VIRTUAL_THREAD_ID)
            lastSuspendedThreadId = ARK_TS_VIRTUAL_THREAD_ID
            lastTopFrameIdeSourceOverride = stackTopSource
            try {
                session.setPauseActionSupported(true)
            } catch (_: Throwable) {
            }
            // attract=true already moves caret / execution point. Do NOT also call
            // showExecutionPoint() — that re-navigates the editor and flashes on every Step Over.
            positionReachedNotifySuspend(context, attract = editorHighlightSource != null)
            try {
                session.rebuildViews()
            } catch (_: Throwable) {
            }
            // Soft gutter update only (no openTextEditor focus steal). sessionPaused also refreshes.
            editorHighlightSource?.let { pos ->
                executionGutterHighlighter.onArkTsPausedAt(pos)
            }
            pluginDbgPrintln(
                "[ArkTs] positionReached stackTop=${stackTopSource?.file?.name}:${stackTopSource?.line} " +
                    "highlight=${editorHighlightSource?.file?.name}:${editorHighlightSource?.line} " +
                    "sessionSuspended=${session.isSuspended}",
            )
            notifyConnectedOnce()
        }
        if (ApplicationManager.getApplication().isDispatchThread) {
            deliver.run()
        } else {
            ApplicationManager.getApplication().invokeLater(deliver)
        }
    }

    /** Re-notify IDE after attach/CDP ready if ArkTS paused before "Connected successfully". */
    internal fun reassertArkTsSuspendUiIfNeeded() {
        crossLanguageCoordinator?.renotifySuspendIfActive()
    }

    /**
     * ArkTS [Debugger.nativeCalling]: arm LLDB at [nativeAddress], then replyNativeCalling so the VM
     * enters native where LLDB is already waiting (official order — reply before runTo races native).
     */
    internal fun handoffArkTsToNative(nativeAddress: Long, isStepInto: Boolean) {
        ApplicationManager.getApplication().executeOnPooledThread {
            if (isEngineStopped()) return@executeOnPooledThread
            val backend = arkTsDebugBackend ?: return@executeOnPooledThread
            nativeLldbHandoffInFlight = true
            pluginDbgPrintln(
                "[CrossLang] handoffArkTsToNative addr=0x${nativeAddress.toString(16)} stepInto=$isStepInto",
            )
            CrossLangJumpTiming.mark(session, "handoff thread addr=0x${nativeAddress.toString(16)}")
            val runOk = if (nativeAddress != 0L) {
                CrossLangTrace.log(
                    session,
                    "handoff arming runToAddress BEFORE replyNativeCalling addr=0x${nativeAddress.toString(16)}",
                )
                val armStartNs = System.nanoTime()
                val armed = serviceWrapper.runToAddress(nativeAddress)
                val armMs = (System.nanoTime() - armStartNs) / 1_000_000.0
                CrossLangJumpTiming.mark(
                    session,
                    "runToAddress end ok=$armed armMs=${String.format(java.util.Locale.US, "%.1f", armMs)}",
                )
                armed
            } else {
                CrossLangJumpTiming.mark(session, "runToAddress skipped addr=0")
                false
            }
            CrossLangTrace.log(
                session,
                "handoff runToAddress(0x${nativeAddress.toString(16)}) ok=$runOk (pre-reply)",
                error = !runOk,
            )
            if (!runOk && nativeAddress != 0L) {
                CrossLangBlocker.report(
                    session,
                    CrossLangBlockerCode.NATIVE_HANDOFF_FAILED,
                    "runToAddress(0x${nativeAddress.toString(16)}) 失败，无法在 replyNativeCalling 前设 LLDB 断点",
                    CrossLangBlocker.remediationFor(CrossLangBlockerCode.NATIVE_HANDOFF_FAILED),
                )
            }
            CrossLangJumpTiming.mark(session, "replyNativeCalling SEND")
            backend.replyNativeCalling(userCode = true) { replied ->
                CrossLangJumpTiming.mark(session, "replyNativeCalling RESP ok=$replied")
                crossLanguageCoordinator?.clearWaitingNativeReply()
                CrossLangTrace.log(session, "handoff replyNativeCalling ok=$replied")
                if (!replied) {
                    nativeLldbHandoffInFlight = false
                    pluginDbgPrintln("[CrossLang] handoff failed: replyNativeCalling rejected")
                    CrossLangBlocker.report(
                        session,
                        CrossLangBlockerCode.NATIVE_HANDOFF_FAILED,
                        "replyNativeCalling 被拒绝，无法从 ArkTS 交接到 LLDB",
                        CrossLangBlocker.remediationFor(CrossLangBlockerCode.NATIVE_HANDOFF_FAILED),
                    )
                    return@replyNativeCalling
                }
                if (!runOk && nativeAddress != 0L) {
                    CrossLangTrace.log(
                        session,
                        "handoff: native may have run without LLDB catch — waiting for event loop",
                        error = true,
                    )
                }
            }
        }
    }

    internal fun isNativeLldbHandoffInFlight(): Boolean = nativeLldbHandoffInFlight

    /**
     * Arm LLDB to catch [functionName] when ArkTS stepInto enters native (NAPI may not emit nativeCalling CDP).
     */
    internal fun prepareNativeCatchBeforeStepInto(functionName: String): Boolean {
        clearNativeCatchBreakpoint()
        return applyNativeCatchArm(serviceWrapper.armNativeCatchFunction(functionName))
    }

    private fun applyNativeCatchArm(arm: LLDBServiceWrapper.NativeCatchArmResult): Boolean {
        if (arm.breakpointId == 0) {
            lastNativeCatchDiagSummary = "no breakpoint id for ${arm.label}"
            CrossLangTrace.log(session, "LLDB native catch BP FAILED fn=${arm.label}", error = true)
            return false
        }
        nativeCatchBreakpointId = arm.breakpointId.takeIf { it > 0 }
        nativeCatchExtraBreakpointIds.clear()
        nativeCatchExtraBreakpointIds.addAll(arm.secondaryBreakpointIds.filter { it > 0 })
        nativeCatchHandoffPending = true
        nativeCatchEntryAddress = arm.preferredEntryAddress
        lastNativeCatchDiagSummary = buildString {
            append("symbol=${arm.label} resolved=${arm.locationCount}")
            if (arm.locationCount <= 0) append(" (UNRESOLVED — BP will not hit)")
            if (arm.preferredEntryAddress != 0L) {
                append(" entry=0x${arm.preferredEntryAddress.toString(16)}")
            }
        }
        val idHint = if (arm.breakpointId > 0) "id=${arm.breakpointId}" else "name-only (interpreter)"
        CrossLangTrace.log(
            session,
            "LLDB native catch BP armed $idHint $lastNativeCatchDiagSummary",
            error = arm.locationCount <= 0,
        )
        if (arm.breakpointId > 0 && arm.listOutput.isNotBlank()) {
            val snippet = arm.listOutput.replace('\n', ' ').take(280)
            CrossLangTrace.log(session, "LLDB BP list: $snippet", error = arm.locationCount <= 0)
        }
        if (arm.locationCount <= 0) {
            CrossLangBlocker.report(
                session,
                CrossLangBlockerCode.LLDB_NATIVE_CATCH_FAILED,
                lastNativeCatchDiagSummary,
                CrossLangBlocker.remediationFor(CrossLangBlockerCode.LLDB_NATIVE_CATCH_FAILED),
            )
        }
        return true
    }

    /**
     * Arms native symbol catch off the EDT with a timeout so CDP stepInto is not blocked indefinitely
     * when LLDB symbol resolution stalls.
     */
    internal fun prepareNativeCatchBeforeStepIntoWithTimeout(
        functionName: String,
        timeoutMs: Long = 4_000L,
    ): Boolean {
        val generation = nativeCatchArmGeneration.incrementAndGet()
        CrossLangTrace.log(
            session,
            "LLDB native catch arming gen=$generation fn=$functionName timeoutMs=$timeoutMs",
        )
        val future = CompletableFuture.supplyAsync {
            clearNativeCatchBreakpoint()
            val arm = serviceWrapper.armNativeCatchFunction(functionName)
            generation to arm
        }
        return try {
            val (completedGen, arm) = future.get(timeoutMs, TimeUnit.MILLISECONDS)
            if (completedGen != nativeCatchArmGeneration.get()) {
                if (arm.breakpointId > 0) {
                    serviceWrapper.removeBreakpoint(arm.breakpointId) {}
                } else if (arm.breakpointId < 0) {
                    serviceWrapper.removeArmedNameBreakpoint()
                }
                CrossLangTrace.log(
                    session,
                    "LLDB native catch stale gen=$completedGen (current=${nativeCatchArmGeneration.get()})",
                    error = true,
                )
                return false
            }
            applyNativeCatchArm(arm)
        } catch (e: TimeoutException) {
            nativeCatchArmGeneration.incrementAndGet()
            CrossLangTrace.log(
                session,
                "LLDB native catch TIMEOUT after ${timeoutMs}ms fn=$functionName — proceeding with CDP stepInto",
                error = true,
            )
            false
        } catch (e: Exception) {
            CrossLangTrace.log(
                session,
                "LLDB native catch ERROR fn=$functionName: ${e.message}",
                error = true,
            )
            false
        }
    }

    internal fun clearNativeCatchBreakpoint() {
        val id = nativeCatchBreakpointId
        val extras = nativeCatchExtraBreakpointIds.toList()
        nativeCatchBreakpointId = null
        nativeCatchExtraBreakpointIds.clear()
        nativeCatchHandoffPending = false
        nativeCatchEntryAddress = 0L
        if (id != null && id > 0) {
            serviceWrapper.removeBreakpoint(id) {}
        }
        for (extraId in extras) {
            serviceWrapper.removeBreakpoint(extraId) {}
        }
        serviceWrapper.removeArmedNameBreakpoint()
    }

    /**
     * Enter native from an ArkTS CDP pause without CDP resume or setSymbolicBreakpoints.
     * Uses LLDB runToAddress at the armed symbol entry (same idea as nativeCalling handoff).
     */
    internal fun enterNativeViaLldbCatchOnly(): Boolean {
        if (!nativeCatchHandoffPending && !hasNativeCatchBreakpointArmed()) {
            CrossLangTrace.log(session, "LLDB-only entry skipped — no catch armed", error = true)
            return false
        }
        val entryAddr = resolveNativeCatchEntryAddress()
        if (entryAddr == 0L) {
            CrossLangTrace.log(session, "LLDB-only entry skipped — no entry address", error = true)
            return false
        }
        CrossLangTrace.log(
            session,
            "LLDB-only native entry runToAddress=0x${entryAddr.toString(16)} " +
                "(skip CDP resume — setSymbolic/resume can break CDP socket)",
        )
        serviceWrapper.ensureThreadSelectedForNativeCatch()
        val ok = serviceWrapper.runToAddress(entryAddr)
        CrossLangTrace.log(
            session,
            "LLDB-only runToAddress ok=$ok addr=0x${entryAddr.toString(16)}",
            error = !ok,
        )
        return ok
    }

    /**
     * Arm LLDB to catch native entry before CDP step/resume.
     * @param aggressiveEntryCapture When true (assignment-line resume+catch), use runToAddress even if
     *   LLDB reports the inferior as running — symbol-only catch often misses without an address trap.
     */
    internal fun preKickNativeEntryCaptureBeforeCdpStep(aggressiveEntryCapture: Boolean = false): Boolean {
        if (!nativeCatchHandoffPending && !hasNativeCatchBreakpointArmed()) return false
        val entryAddr = resolveNativeCatchEntryAddress()
        if (entryAddr == 0L) {
            CrossLangTrace.log(session, "LLDB pre-step skipped — no entry address", error = true)
            return false
        }
        val running = serviceWrapper.isInferiorRunning()
        CrossLangTrace.log(
            session,
            "LLDB pre-step addr=0x${entryAddr.toString(16)} inferiorRunning=$running " +
                "aggressive=$aggressiveEntryCapture fn=${getLastNativeCatchDiagSummary()}",
        )
        serviceWrapper.ensureThreadSelectedForNativeCatch()
        if (!running) {
            val ok = serviceWrapper.ensureInferiorRunningForNativeCatch()
            CrossLangTrace.log(
                session,
                "LLDB ensureInferiorRunning before CDP step/resume ok=$ok inferiorWasStopped=true",
                error = !ok,
            )
            return ok
        }
        if (aggressiveEntryCapture) {
            val ok = serviceWrapper.runToAddress(entryAddr)
            CrossLangTrace.log(
                session,
                "LLDB runToAddress before resume-catch ok=$ok addr=0x${entryAddr.toString(16)}",
                error = !ok,
            )
            return ok
        }
        CrossLangTrace.log(session, "LLDB pre-step: inferior already running — symbol catch only")
        return true
    }

    internal fun queryNativeCatchBreakpointHitCount(): Int {
        var total = 0
        val primary = nativeCatchBreakpointId
        if (primary != null && primary > 0) {
            total += serviceWrapper.parseBreakpointHitCount(serviceWrapper.fetchBreakpointListOutput(primary))
        }
        for (extraId in nativeCatchExtraBreakpointIds) {
            if (extraId > 0) {
                total += serviceWrapper.parseBreakpointHitCount(serviceWrapper.fetchBreakpointListOutput(extraId))
            }
        }
        return total
    }

    private fun resolveNativeCatchEntryAddress(): Long {
        if (nativeCatchEntryAddress != 0L) return nativeCatchEntryAddress
        val id = nativeCatchBreakpointId ?: return 0L
        if (id <= 0) return 0L
        val listOut = serviceWrapper.fetchBreakpointListOutput(id)
        nativeCatchEntryAddress = serviceWrapper.parsePreferredEntryAddress(listOut)
        return nativeCatchEntryAddress
    }

    /**
     * While CDP stepInto runs, ensure inferior is running and optionally runToAddress at parsed NAPI entry.
     * Fallback when PRE-stepInto arm was missed.
     */
    internal fun kickNativeEntryCaptureAfterCdpStep(aggressivePoll: Boolean = false) {
        if (!nativeCatchHandoffPending) return
        ApplicationManager.getApplication().executeOnPooledThread {
            if (isEngineStopped() || !nativeCatchHandoffPending) return@executeOnPooledThread
            CrossLangTrace.log(session, "LLDB post-CDP catch kick aggressivePoll=$aggressivePoll")
            serviceWrapper.ensureThreadSelectedForNativeCatch()
            val running = serviceWrapper.isInferiorRunning()
            if (!running) {
                val ok = serviceWrapper.ensureInferiorRunningForNativeCatch()
                CrossLangTrace.log(
                    session,
                    "LLDB ensureInferiorRunning after CDP step/resume ok=$ok",
                    error = !ok,
                )
            }
            if (aggressivePoll) {
                val deadline = System.currentTimeMillis() + 3_000L
                while (System.currentTimeMillis() < deadline) {
                    if (isEngineStopped() || !nativeCatchHandoffPending) return@executeOnPooledThread
                    if (crossLanguageCoordinator?.activeBackend == CrossLanguageBackend.LLDB) {
                        CrossLangTrace.log(session, "LLDB post-resume poll — native handoff won")
                        return@executeOnPooledThread
                    }
                    val hits = queryNativeCatchBreakpointHitCount()
                    if (hits > 0) {
                        CrossLangTrace.log(
                            session,
                            "LLDB post-resume poll — symbol BP hit count=$hits (waiting for stop event)",
                        )
                    }
                    try {
                        Thread.sleep(80)
                    } catch (_: InterruptedException) {
                        Thread.currentThread().interrupt()
                        return@executeOnPooledThread
                    }
                }
                if (nativeCatchHandoffPending) {
                    CrossLangTrace.log(
                        session,
                        "LLDB post-resume poll timeout — hitCount=${queryNativeCatchBreakpointHitCount()} " +
                            "catchStillPending=true",
                        error = true,
                    )
                }
            } else if (running) {
                CrossLangTrace.log(session, "LLDB post-CDP: inferior already running (symbol catch armed)")
            }
        }
    }

    private data class NativeStopHandoff(val catchStop: Boolean, val addressHandoff: Boolean)

    /** @return handoff kind when this stop completes a cross-language native entry capture. */
    private fun consumeNativeCatchOrHandoffStop(firstFrame: StackFrame): NativeStopHandoff {
        if (nativeCatchHandoffPending) {
            nativeCatchHandoffPending = false
            clearNativeCatchBreakpoint()
            crossLanguageCoordinator?.onLldbNativeCatchStop()
            pluginDbgPrintln("[handleStopped] ✓ native LLDB catch stop at ${firstFrame.name}")
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Cross-lang: LLDB catch in native — ${firstFrame.file.ifEmpty { firstFrame.name }}",
            )
            session.consoleView?.print(
                "Cross-language: paused in native (${firstFrame.name}) via LLDB symbol breakpoint.\n",
                ConsoleViewContentType.SYSTEM_OUTPUT,
            )
            return NativeStopHandoff(catchStop = true, addressHandoff = false)
        }
        if (nativeLldbHandoffInFlight) {
            nativeLldbHandoffInFlight = false
            crossLanguageCoordinator?.onLldbNativeHandoffStop()
            pluginDbgPrintln("[handleStopped] ✓ native LLDB handoff stop at ${firstFrame.name}")
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Cross-lang: LLDB stopped in native — ${firstFrame.file.ifEmpty { firstFrame.name }}",
            )
            session.consoleView?.print(
                "Cross-language: paused in native code (${firstFrame.name}). " +
                    "Step/Resume uses LLDB; returning to ArkTS happens on native return.\n",
                ConsoleViewContentType.SYSTEM_OUTPUT,
            )
            return NativeStopHandoff(catchStop = false, addressHandoff = true)
        }
        return NativeStopHandoff(catchStop = false, addressHandoff = false)
    }

    internal fun hasNativeCatchBreakpointArmed(): Boolean =
        nativeCatchBreakpointId != null || serviceWrapper.hasArmedNameBreakpoint()

    internal fun isNativeCatchHandoffPending(): Boolean = nativeCatchHandoffPending

    internal fun tryBeginUserContinueOrStepCrossLang(): Boolean = tryBeginUserContinueOrStep()

    internal fun clearUserContinueOrStepPendingCrossLang() = clearUserContinueOrStepPending()

    internal fun isUserContinueOrStepPendingCrossLang(): Boolean =
        synchronized(userContinueOrStepLock) { userContinueOrStepPending }

    internal fun onArkTsEngineResumed() {
        isolateResumed(ARK_TS_VIRTUAL_THREAD_ID)
        clearUserContinueOrStepPending()
        ApplicationManager.getApplication().invokeLater {
            if (!session.project.isDisposed) {
                LLDBExecutionGutterHighlighter.clearAllMarkers(session.project)
            }
        }
    }

    private fun delegateExecutionToCrossLanguage(action: CrossLanguageDebugCoordinator.(XSuspendContext?) -> Unit): Boolean {
        val coord = crossLanguageCoordinator
        if (coord == null) {
            CrossLangTrace.log(session, "delegateCrossLang SKIP: no coordinator")
            return false
        }
        if (!OhosKmpCrossLanguageDebug.isEnabled()) {
            CrossLangTrace.log(session, "delegateCrossLang SKIP: cross-lang flag off")
            return false
        }
        if (!coord.isArkTsSuspended() && coord.activeBackend != CrossLanguageBackend.ARK_TS) {
            CrossLangTrace.log(
                session,
                "delegateCrossLang SKIP: arkTsSuspended=${coord.isArkTsSuspended()} backend=${coord.activeBackend}",
            )
            return false
        }
        action(coord, null)
        return true
    }

    private fun syncArkTsBreakpointIfNeeded(breakpoint: XLineBreakpoint<XBreakpointProperties<*>>) {
        if (!OhosKmpCrossLanguageDebug.isEnabled()) return
        val path = breakpoint.sourcePosition?.file?.path ?: return
        if (!ArkTsPositionMapper.isArkTsSourcePath(path)) return
        val backends = mutableListOf<ArkTsDebugBackend>()
        arkTsDebugBackend?.let { backends.add(it) }
        multiInstanceManager?.allWorkerBackends()?.let { backends.addAll(it) }
        if (backends.isEmpty()) return
        for (backend in backends) {
            if (breakpoint.isEnabled) {
                backend.registerBreakpoint(path, breakpoint.line)
            } else {
                backend.unregisterBreakpoint(path, breakpoint.line)
            }
        }
    }

    private fun syncAllArkTsBreakpointsFromProject(targetBackend: ArkTsDebugBackend? = null) {
        if (!OhosKmpCrossLanguageDebug.isEnabled()) return
        val backends = mutableListOf<ArkTsDebugBackend>()
        if (targetBackend != null) {
            backends.add(targetBackend)
        } else {
            arkTsDebugBackend?.let { backends.add(it) }
            multiInstanceManager?.allWorkerBackends()?.let { backends.addAll(it) }
        }
        if (backends.isEmpty()) return
        val bm = XDebuggerManager.getInstance(session.project).breakpointManager
        for (raw in bm.allBreakpoints) {
            if (raw !is XLineBreakpoint<*>) continue
            if (!raw.isEnabled) continue
            val path = raw.sourcePosition?.file?.path ?: continue
            if (!ArkTsPositionMapper.isArkTsSourcePath(path)) continue
            for (backend in backends) {
                backend.registerBreakpoint(path, raw.line, pushToDevice = false)
            }
        }
    }

    /** Push all IDE `.ets` / `.ts` breakpoints to the device CDP session(s). */
    internal fun resyncAllArkTsBreakpointsToDevice() {
        loadArkTsBreakpointsFromIde()
        arkTsDebugBackend?.resyncAllBreakpoints()
        multiInstanceManager?.fanOutResyncAll()
    }

    /** Register IDE breakpoints in backend memory only (no CDP push). */
    internal fun loadArkTsBreakpointsFromIde(targetBackend: ArkTsDebugBackend? = null) {
        syncAllArkTsBreakpointsFromProject(targetBackend)
    }

    // ========== Debug mode ==========

    fun setDebugModeContext(context: DebugModeContext) {
        this.debugModeContext = context
        connectServerArmedDuringPrewarm = false
        crossLangPrewarmFuture = null
        if (OhosKmpCrossLanguageDebug.isEnabled()) {
            startCrossLangBridgePrewarm()
        }
        beginAttachPipelinePreambleIfNeeded()
    }

    /**
     * Overlap hdc fport (+ connect-server registration) with LLDB JNI attach, Kotlin script load, and BP sync.
     * Does **not** open the main debugger CDP WebSocket (that remains after LLDB continue).
     */
    private fun startCrossLangBridgePrewarm() {
        val ctx = debugModeContext ?: return
        val startedAt = System.nanoTime()
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER IDE: cross-lang bridge prewarm (hdc fport, parallel with LLDB attach tail)",
        )
        val future = CompletableFuture<CrossLangBridgePrewarmSnapshot>()
        crossLangPrewarmFuture = future
        ApplicationManager.getApplication().executeOnPooledThread {
            if (attachStartupAborted()) {
                future.complete(CrossLangBridgePrewarmSnapshot.EMPTY)
                return@executeOnPooledThread
            }
            var connectPort: Int? = null
            var debuggerOk = false
            var connectArmed = false
            try {
                connectPort = setupConnectServerPortForwardingWithRetry(
                    ctx.deviceId,
                    ctx.pid,
                    ctx.bundleName,
                    ctx.connectServerPort,
                )
                if (connectPort != null) {
                    debugModeContext = ctx.copy(connectServerPort = connectPort)
                }
                debuggerOk = setupPortForwarding(ctx.deviceId, ctx.pid, ctx.debuggerPort)
                if (connectPort != null && !attachStartupAborted()) {
                    armMultiInstanceFromConnectServerDuringPrewarm(connectPort)
                    connectArmed = connectServerArmedDuringPrewarm
                }
            } catch (t: Throwable) {
                LOG.warn("Cross-lang bridge prewarm failed: ${t.message}", t)
            }
            val elapsedMs = (System.nanoTime() - startedAt) / 1_000_000L
            val snapshot = CrossLangBridgePrewarmSnapshot(
                connectServerPort = connectPort,
                debuggerPortForwardOk = debuggerOk,
                connectServerArmed = connectArmed,
                elapsedMs = elapsedMs,
            )
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER IDE: cross-lang bridge prewarm done " +
                    "(connectServer=${connectPort ?: "fail"}, debuggerFport=$debuggerOk, " +
                    "connectServerWs=$connectArmed, ${elapsedMs}ms)",
            )
            future.complete(snapshot)
        }
    }

    private fun armMultiInstanceFromConnectServerDuringPrewarm(connectPort: Int) {
        if (connectServerArmedDuringPrewarm) return
        val ctx = debugModeContext ?: return
        synchronized(this) {
            if (connectServerArmedDuringPrewarm) return
            connectServerClient = ArkTsConnectServerClient()
            connectServerClient?.connectAndHandshake(
                session = session,
                host = "127.0.0.1",
                port = connectPort,
                onComplete = { instanceId, tid, name ->
                    connectServerArmedDuringPrewarm = true
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server OK [prewarm] " +
                            "instanceId=$instanceId tid=$tid name=$name",
                    )
                    val mgr = multiInstanceManager
                        ?: ArkTsMultiInstanceManager(this@LLDBDebugProcess).also {
                            multiInstanceManager = it
                        }
                    mgr.bindConnectServer(connectServerClient!!, handshakeMainInstanceId = instanceId)
                },
                onError = { err ->
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server handshake FAILED [prewarm] — $err",
                        ConsoleViewContentType.ERROR_OUTPUT,
                    )
                },
            )
        }
    }

    private fun awaitCrossLangBridgePrewarm(): CrossLangBridgePrewarmSnapshot {
        val future = crossLangPrewarmFuture ?: return CrossLangBridgePrewarmSnapshot.EMPTY
        return try {
            future.get(120, TimeUnit.SECONDS)
        } catch (e: TimeoutException) {
            LOG.warn("Cross-lang bridge prewarm await timeout: ${e.message}")
            CrossLangBridgePrewarmSnapshot.EMPTY
        } catch (e: Exception) {
            LOG.warn("Cross-lang bridge prewarm await failed: ${e.message}")
            CrossLangBridgePrewarmSnapshot.EMPTY
        }
    }

    /** Menu Attach / attach-mode: app already past -D wait. */
    internal fun isAttachToRunningApp(): Boolean = debugModeContext?.attachToRunningApp == true

    /** `attach` = menu Attach (aa start, no -D). `debug` = Debug button (aa process -D). */
    internal fun launchModeLabel(): String = when {
        isAttachToRunningApp() -> "attach"
        debugModeContext != null -> "debug"
        else -> "lldb"
    }

    /** The continue that lets the app run after attach. Not used for later Step/Resume. */
    private fun markLaunchContinue(where: String) {
        launchContinueNs = System.nanoTime()
        launchContinueStopLogged = false
        CrossLangTrace.log(
            session,
            "[TIMING][jump] launch-continue where=$where mode=${launchModeLabel()} " +
                "warmup=$softwareBreakpointWarmupSummary",
        )
    }

    /**
     * First LLDB stop after [markLaunchContinue]. `msSinceLaunchContinue` includes however long
     * the app ran before this stop, so use the Step Into `[TIMING][jump]` lines for click latency.
     */
    internal fun onLldbStoppedForTiming(
        reason: String,
        breakpointId: Int,
        topName: String,
        topFile: String,
        topLine: Int,
    ) {
        val start = launchContinueNs
        if (start != 0L && !launchContinueStopLogged) {
            launchContinueStopLogged = true
            val sinceContinue = (System.nanoTime() - start) / 1_000_000.0
            CrossLangTrace.log(
                session,
                "[TIMING][jump] first STOP since launch-continue " +
                    "msSinceLaunchContinue=${String.format(java.util.Locale.US, "%.1f", sinceContinue)} " +
                    "reason=$reason bp=$breakpointId top=$topName @ $topFile:$topLine " +
                    "mode=${launchModeLabel()} warmup=$softwareBreakpointWarmupSummary",
            )
        }
        if (CrossLangJumpTiming.isActive()) {
            CrossLangJumpTiming.mark(
                session,
                "waitForEvent STOP reason=$reason bp=$breakpointId top=$topName @ $topFile:$topLine " +
                    "(includes native stack fill inside waitForEvent)",
            )
        }
    }

    private fun reportAttachCdpDegraded(reason: String) {
        LOG.warn("Attach: ArkTS CDP degraded (LLDB continues): $reason")
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER Attach: CDP degraded — LLDB/Kotlin eval active ($reason)",
            ConsoleViewContentType.SYSTEM_OUTPUT,
        )
        session.consoleView?.print(
            "Attach: ArkTS CDP unavailable ($reason). LLDB / Kotlin eval continue.\n",
            ConsoleViewContentType.SYSTEM_OUTPUT,
        )
    }

    /** Debug-ready / TTDR: LLDB is usable; ArkTS CDP may still be handshaking in the background. */
    private fun notifyCrossLangLldbStartupReady(onReady: () -> Unit) {
        if (!crossLangLldbStartupReadyOnce.compareAndSet(false, true)) return
        ApplicationManager.getApplication().invokeLater {
            if (stopped || session.project.isDisposed) return@invokeLater
            onReady()
        }
    }

    private fun reportCrossLangCdpAsyncFailure(reason: String) {
        if (!crossLangCdpFailureReportedOnce.compareAndSet(false, true)) return
        if (isAttachToRunningApp()) {
            reportAttachCdpDegraded(reason)
        } else {
            LOG.warn("ArkTS CDP async handshake failed (LLDB already ready): $reason")
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS CDP FAILED (background) — $reason",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            HarmonyDebugPipelineTiming.printDebugReady(session)
        }
    }

    /**
     * Pending one-shot on libentry's load-time constructor. Attach already has the .so
     * mapped and that constructor has run, so this is Debug-only.
     */
    private fun armLibentryLoadWarmupIfDebug() {
        if (isAttachToRunningApp() || debugModeContext == null) return
        if (!OhosKmpCrossLanguageDebug.isEnabled()) return
        if (attachStartupAborted()) return
        val id = try {
            serviceWrapper.armOneShotShlibBreakpoint("RegisterEntryModule", "libentry.so")
        } catch (t: Throwable) {
            LOG.warn("libentry load warmup breakpoint failed: ${t.message}", t)
            0
        }
        if (id <= 0) {
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: libentry load warmup not armed " +
                    "(RegisterEntryModule) — first Step Into may still pay the libentry stop",
            )
            return
        }
        libentryLoadWarmupBreakpointId = id
        libentryLoadWarmupPending = true
        softwareBreakpointWarmupSummary = "$softwareBreakpointWarmupSummary libentry-load-armed"
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: libentry load warmup armed on " +
                "RegisterEntryModule (id=$id, one-shot, swallowed)",
        )
    }

    internal fun shouldSwallowLibentryLoadWarmup(breakpointId: Int, topFunction: String?): Boolean {
        if (!libentryLoadWarmupPending) return false
        val armedId = libentryLoadWarmupBreakpointId
        if (armedId > 0 && breakpointId == armedId) return true
        // One-shot hits sometimes report bp id 0 after LLDB deletes the site.
        return topFunction?.contains("RegisterEntryModule") == true
    }

    internal fun completeLibentryLoadWarmupSwallow(topFunction: String?) {
        val id = libentryLoadWarmupBreakpointId
        libentryLoadWarmupPending = false
        libentryLoadWarmupBreakpointId = 0
        if (id > 0) {
            try {
                serviceWrapper.removeBreakpoint(id)
            } catch (t: Throwable) {
                LOG.warn("remove libentry load warmup bp $id: ${t.message}")
            }
        }
        val where = topFunction?.ifBlank { null } ?: "RegisterEntryModule"
        softwareBreakpointWarmupSummary = "$softwareBreakpointWarmupSummary libentry-load-hit $where"
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: first libentry software-breakpoint stop swallowed " +
                "($where, startup). Step Into should skip that cost.",
        )
    }

    /**
     * One-shot breakpoint on a userspace thread's next instruction. Waits for lldb-server
     * to actually stop, then removes the breakpoint and leaves the process stopped.
     * The later [resumeThread] continue is unchanged.
     * Caller must not have started the LLDB event loop.
     */
    private fun primeSoftwareBreakpointBeforeUserCanStep() {
        if (attachStartupAborted()) return
        val raw = try {
            serviceWrapper.primeFirstSoftwareBreakpoint()
        } catch (t: Throwable) {
            LOG.warn("software-breakpoint warmup failed: ${t.message}", t)
            "miss\t${t.message ?: t.javaClass.simpleName}\t0"
        }
        val parts = raw.split('\t')
        val kind = parts.getOrNull(0).orEmpty()
        val detail = parts.getOrNull(1).orEmpty().ifBlank { "unknown" }
        val ms = parts.getOrNull(2)?.toLongOrNull() ?: 0L
        softwareBreakpointWarmupSummary = "$kind $detail ${ms}ms mode=${launchModeLabel()}"
        if (kind == "hit") {
            val line = "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: software-breakpoint warmup stopped in $detail (${ms}ms)"
            session.printHarmonyDebugPipelineLine(line)
            pluginDbgPrintln("[Attach] warmup stopped in $detail, took ${ms}ms")
        } else {
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: software-breakpoint warmup missed " +
                    "($detail, ${ms}ms) — first Step Into may still be slow",
            )
            pluginDbgPrintln("[Attach] software-breakpoint warmup miss $detail ${ms}ms")
        }
    }

    /**
     * LLDB attach tail (non-cross-language): continue inferior, event loop, HiLog.
     */
    private fun finishLldbAttachContinueAndEventLoop(pid: Int) {
        if (attachStartupAborted()) {
            pluginDbgPrintln("[finishLldbAttachContinue] aborted before continue")
            return
        }
        session.printHarmonyDebugPipelineLine(
            "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: continue inferior (pid=$pid)",
        )
        markLaunchContinue("finishLldbAttachContinue")
        serviceWrapper.resumeThread(1, null)
        finishLldbAttachEventLoopAndHilog(pid)
    }

    /** Start LLDB event loop + HiLog after inferior is already running (cross-language CDP path). */
    private fun finishLldbAttachEventLoopAndHilog(pid: Int) {
        if (attachStartupAborted()) {
            pluginDbgPrintln("[finishLldbAttachEventLoop] aborted")
            return
        }
        if (attachEventLoopStarted) {
            pluginDbgPrintln("[finishLldbAttachEventLoop] already started — skip")
            return
        }
        attachEventLoopStarted = true
        serviceWrapper.startEventLoop()
        pluginDbgPrintln("[Attach] ✓ event loop started")

        ApplicationManager.getApplication().invokeLater {
            if (stopped || session.project.isDisposed) return@invokeLater
            try {
                session.setPauseActionSupported(true)
            } catch (_: Throwable) {
            }
        }

        startDebugConsoleHilogMirror(pid)
    }

    /**
     * Cross-language debug mode (DevEco / OHOS constraint):
     * 0. [startCrossLangBridgePrewarm] (parallel with LLDB attach tail): hdc fport + connect-server WS
     * 1. Await prewarm, fill any missing fport / connect-server registration
     * 2. LLDB continue — ArkTS debugger cannot answer main CDP WS while the inferior is frozen at attach
     * 3. WebSocket CDP immediately (async): enable → saveAll → setMixed → runIfWaiting
     * 4. LLDB event loop (started with continue); [onReady] / TTDR when LLDB is ready — does **not** block on CDP
     *
     * Do NOT wait for Index user BP before LLDB continue (deadlock).
     */
    private fun connectArkTsCdpAfterLldbContinue(
        pid: Int,
        onReady: () -> Unit,
        onFailed: (String) -> Unit,
    ) {
        val context = debugModeContext
        if (context == null) {
            onReady()
            return
        }
        ApplicationManager.getApplication().executeOnPooledThread {
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS bridge — apply prewarm, LLDB continue, async CDP",
            )
            if (attachStartupAborted()) {
                onFailed("session stopped")
                return@executeOnPooledThread
            }

            val prewarm = awaitCrossLangBridgePrewarm()
            var ctx = debugModeContext ?: context
            if (prewarm.connectServerPort != null) {
                ctx = ctx.copy(connectServerPort = prewarm.connectServerPort)
                debugModeContext = ctx
            }

            // Do NOT run [HdcPortForwardCleanup.cleanupOldPortForwarding] here on Attach.
            // Bulk rm of tcp:15036/15037 + all ark:* rules races an in-flight session (Stop cleanup
            // vs new connect-server WS) → code=1005, "Connection closed", spurious process EXITED.
            // Launch still cleans before first fport (HarmonyDebugRunner). Attach only adds/refreshes
            // rules for the current pid via setupConnectServerPortForwardingWithRetry / setupPortForwarding.
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER IDE: Attach — skip bulk hdc fport cleanup (keep live debugger forwards)",
            )
            if (attachStartupAborted()) {
                onFailed("session stopped")
                return@executeOnPooledThread
            }

            fun beginCdpHandshake() {
                crossLangCdpProtocolReady = false
                crossLangCdpFailureReportedOnce.set(false)
                startWebSocketDebugger(
                    onProtocolReady = {
                        crossLangCdpProtocolReady = true
                        session.printHarmonyDebugPipelineLine(
                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS CDP protocol ready (saveAll + runIfWaiting)",
                        )
                    },
                    onComplete = {
                        session.printHarmonyDebugPipelineLine(
                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS CDP ready",
                        )
                        HarmonyDebugPipelineTiming.printDebugReady(session)
                    },
                    onError = { err ->
                        reportCrossLangCdpAsyncFailure(err)
                    },
                )
                Thread({
                    try {
                        Thread.sleep(30_000L)
                    } catch (_: InterruptedException) {
                        Thread.currentThread().interrupt()
                        return@Thread
                    }
                    if (attachStartupAborted()) return@Thread
                    if (!crossLangCdpProtocolReady && !crossLangCdpFailureReportedOnce.get()) {
                        reportCrossLangCdpAsyncFailure("ArkTS CDP protocol handshake timeout (30s)")
                    }
                }, "ArkTs-CDP-Handshake-Watchdog").apply { isDaemon = true }.start()
            }

            fun armMultiInstanceFromConnectServer(stage: String) {
                if (connectServerArmedDuringPrewarm) {
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server already armed [$stage] (prewarm)",
                    )
                    return
                }
                val ctx = debugModeContext ?: return
                connectServerClient = ArkTsConnectServerClient()
                connectServerClient?.connectAndHandshake(
                    session = session,
                    host = "127.0.0.1",
                    port = ctx.connectServerPort,
                    onComplete = { instanceId, tid, name ->
                        connectServerArmedDuringPrewarm = true
                        session.printHarmonyDebugPipelineLine(
                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server OK [$stage] " +
                                "instanceId=$instanceId tid=$tid name=$name",
                        )
                        val mgr = multiInstanceManager
                            ?: ArkTsMultiInstanceManager(this@LLDBDebugProcess).also {
                                multiInstanceManager = it
                                pluginDbgPrintln(
                                    "[WorkerTabDiag] created MultiInstanceManager " +
                                        "@${Integer.toHexString(System.identityHashCode(it))}",
                                )
                            }
                        mgr.bindConnectServer(connectServerClient!!, handshakeMainInstanceId = instanceId)
                    },
                    onError = { err ->
                        CrossLangTrace.log(
                            session,
                            "connect-server handshake failed [$stage]: $err",
                            error = true,
                        )
                        session.printHarmonyDebugPipelineLine(
                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server handshake FAILED [$stage] — $err " +
                                "(Worker multi-instance unavailable)",
                            ConsoleViewContentType.ERROR_OUTPUT,
                        )
                    },
                )
            }

            fun proceedWithDebuggerBridge(stage: String, retryConnectServerAfterContinue: Boolean = false) {
                if (attachStartupAborted()) {
                    onFailed("session stopped")
                    return
                }
                val bridgeCtx = debugModeContext ?: ctx
                val debuggerFportOk = prewarm.debuggerPortForwardOk ||
                    setupPortForwarding(bridgeCtx.deviceId, bridgeCtx.pid, bridgeCtx.debuggerPort)
                if (!debuggerFportOk) {
                    onFailed("hdc fport failed (debugger)")
                    return
                }
                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: hdc fport OK (debugger tcp:${bridgeCtx.debuggerPort}) " +
                        "[$stage${if (prewarm.debuggerPortForwardOk) ", prewarm" else ""}]",
                )
                // Unlock ArkTS debugger threads first — CDP WS will hang while LLDB still has process stopped.
                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER LLDB: continue inferior (unlock ArkTS debugger, pid=$pid)",
                )
                markLaunchContinue("arkts-bridge-before-cdp")
                serviceWrapper.resumeThread(1, null)
                pluginDbgPrintln("[Attach] ✓ LLDB continue sent (before ArkTS CDP)")
                finishLldbAttachEventLoopAndHilog(pid)
                notifyCrossLangLldbStartupReady(onReady)

                if (retryConnectServerAfterContinue) {
                    // Connect-server often becomes available only after the inferior runs.
                    Thread({
                        try {
                            Thread.sleep(1200L)
                        } catch (_: InterruptedException) {
                            Thread.currentThread().interrupt()
                            return@Thread
                        }
                        if (attachStartupAborted()) return@Thread
                        session.printHarmonyDebugPipelineLine(
                            "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: retry connect-server fport after LLDB continue",
                        )
                        val port = setupConnectServerPortForwardingWithRetry(
                            context.deviceId,
                            context.pid,
                            context.bundleName,
                            context.connectServerPort,
                        )
                        if (port != null) {
                            debugModeContext = context.copy(connectServerPort = port)
                            armMultiInstanceFromConnectServer("after-continue-retry")
                        } else {
                            session.printHarmonyDebugPipelineLine(
                                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server still unavailable — " +
                                    "Worker tabs will not open (need ark:{pid}@{bundle} fport)",
                                ConsoleViewContentType.ERROR_OUTPUT,
                            )
                        }
                    }, "ArkTs-ConnectServer-Retry").apply { isDaemon = true }.start()
                }

                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: WebSocket CDP handshake (async, after LLDB continue)",
                )
                beginCdpHandshake()
            }

            val connectPort = prewarm.connectServerPort ?: setupConnectServerPortForwardingWithRetry(
                ctx.deviceId,
                ctx.pid,
                ctx.bundleName,
                ctx.connectServerPort,
            )
            if (connectPort != null) {
                debugModeContext = ctx.copy(connectServerPort = connectPort)
                if (prewarm.connectServerPort == null) {
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: hdc fport OK (connect-server tcp:$connectPort)",
                    )
                }
                if (!prewarm.connectServerArmed) {
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server handshake (DevEco mixed-debug)",
                    )
                    armMultiInstanceFromConnectServer("before-debugger")
                }
                proceedWithDebuggerBridge(
                    if (prewarm.connectServerPort != null) "after-prewarm" else "after-connect-server",
                )
            } else {
                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: connect-server fport FAILED — " +
                        "debugger-only CDP for now; will retry after LLDB continue " +
                        "(target was ark:${ctx.pid}@${ctx.bundleName})",
                    ConsoleViewContentType.ERROR_OUTPUT,
                )
                proceedWithDebuggerBridge("no-connect-server", retryConnectServerAfterContinue = true)
            }
        }
    }

    fun executeDebugModePostContinueSteps() {
        val context = debugModeContext ?: return

        ApplicationManager.getApplication().executeOnPooledThread {
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: ArkTS bridge — hdc fport tcp:15037 → ark:${context.pid}@Debugger, then WebSocket",
            )
            if (attachStartupAborted()) {
                pluginDbgPrintln("[executeDebugModePostContinueSteps] aborted before port forward")
                return@executeOnPooledThread
            }
            val portForwardSuccess = setupPortForwarding(context.deviceId, context.pid)
            if (!portForwardSuccess) {
                // 0411 added
                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: hdc fport FAILED (ArkTS / WebSocket debug may be unavailable)",
                    ConsoleViewContentType.ERROR_OUTPUT,
                )
                LOG.warn("Port forwarding failed; LLDB continues (ArkTS debug unavailable)")
                return@executeOnPooledThread
            }
            // 0411 added
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: hdc fport OK — starting WebSocket client (localhost:15037)",
            )

            if (attachStartupAborted()) {
                pluginDbgPrintln("[executeDebugModePostContinueSteps] aborted before WebSocket debug")
                return@executeOnPooledThread
            }

            startWebSocketDebugger(
                // 0411 added
                onComplete = {
                    LOG.info("Debug mode post-steps finished")
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: WebSocket debugger sequence finished",
                    )
                    HarmonyDebugPipelineTiming.printDebugReady(session)
                },
                onError = { error ->
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: WebSocket FAILED — $error",
                        ConsoleViewContentType.ERROR_OUTPUT,
                    )
                    LOG.warn("WebSocket debug start failed: $error")
                }
            )
        }
    }

    private fun setupPortForwarding(
        deviceId: String,
        pid: String,
        tcpPort: Int = 15037,
        instanceId: Int = 0,
    ): Boolean {
        try {
            val hdcPath = org.cpf.kmp.tools.ohos.sync.DevEcoPathService.getInstance(session.project).getHdcPath()
            val arkPort = if (instanceId == 0) {
                "ark:$pid@Debugger"
            } else {
                "ark:$pid@$instanceId@Debugger"
            }
            val tcp = "tcp:$tcpPort"
            var lastExit = -1
            var lastOut = ""
            // The ark debugger socket and a leftover tcp:15037 rule from the previous
            // session both make the first fport fail. Retry; do not wipe every ark rule.
            repeat(4) { attempt ->
                if (forwardRulePresent(hdcPath, deviceId, tcp, arkPort)) {
                    pluginDbgPrintln("[PortForward] already mapped $tcp → $arkPort")
                    return true
                }
                removeOtherForwardOnTcp(hdcPath, deviceId, tcp, arkPort)
                val command = listOf(hdcPath, "-t", deviceId, "fport", tcp, arkPort)
                pluginDbgPrintln("[PortForward] try#${attempt + 1} fport $tcp → $arkPort")
                val process = ProcessBuilder(command).redirectErrorStream(true).start()
                lastOut = process.inputStream.bufferedReader().readText().trim()
                lastExit = process.waitFor()
                pluginDbgPrintln("[PortForward] exit=$lastExit out=$lastOut")
                val failed = lastExit != 0 || lastOut.contains("[Fail]", ignoreCase = true)
                if (!failed || forwardRulePresent(hdcPath, deviceId, tcp, arkPort)) {
                    return true
                }
                if (attempt < 3) {
                    try {
                        Thread.sleep(350L)
                    } catch (_: InterruptedException) {
                        Thread.currentThread().interrupt()
                        return false
                    }
                }
            }
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug mode: hdc fport FAILED $tcp → $arkPort " +
                    "(exit=$lastExit out=${lastOut.ifBlank { "(empty)" }.take(200)})",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            return false
        } catch (e: Exception) {
            LOG.error("Port forwarding error", e)
            return false
        }
    }

    /** True when `fport ls` already has this exact tcp → ark rule. */
    private fun forwardRulePresent(
        hdcPath: String,
        deviceId: String,
        tcp: String,
        arkPort: String,
    ): Boolean {
        return try {
            val listProcess = ProcessBuilder(listOf(hdcPath, "-t", deviceId, "fport", "ls"))
                .redirectErrorStream(true).start()
            val listOutput = listProcess.inputStream.bufferedReader().readText().trim()
            listProcess.waitFor(5, TimeUnit.SECONDS)
            listOutput.lines().any { line ->
                line.contains(tcp) && line.contains(arkPort)
            }
        } catch (_: Exception) {
            false
        }
    }

    /** Drop a different target still bound to this local tcp port. Leave the desired rule alone. */
    private fun removeOtherForwardOnTcp(
        hdcPath: String,
        deviceId: String,
        tcp: String,
        arkPort: String,
    ) {
        try {
            val listProcess = ProcessBuilder(listOf(hdcPath, "-t", deviceId, "fport", "ls"))
                .redirectErrorStream(true).start()
            val listOutput = listProcess.inputStream.bufferedReader().readText().trim()
            listProcess.waitFor(5, TimeUnit.SECONDS)
            val rulePattern = Regex("""(tcp:\d+)\s+(\S+)""")
            for (line in listOutput.lines()) {
                val match = rulePattern.find(line) ?: continue
                val listedTcp = match.groupValues[1]
                val target = match.groupValues[2]
                if (listedTcp == tcp && target != arkPort) {
                    pluginDbgPrintln("[PortForward] rm stale $listedTcp $target")
                    executeHdcCommand(hdcPath, deviceId, "fport", "rm", listedTcp, target)
                }
            }
        } catch (e: Exception) {
            LOG.warn("removeOtherForwardOnTcp: ${e.message}")
        }
    }

    private fun setupConnectServerPortForwarding(
        deviceId: String,
        pid: String,
        bundleName: String,
        tcpPort: Int,
    ): Boolean = setupConnectServerPortForwardingWithRetry(deviceId, pid, bundleName, tcpPort) != null

    /**
     * hdc fport for connect-server: `tcp:{port} ark:{pid}@{bundleName}`.
     * Cleans stale rules and retries alternate host ports (arkcompiler uses +7) when occupied.
     * @return allocated local tcp port, or null on failure
     */
    private fun setupConnectServerPortForwardingWithRetry(
        deviceId: String,
        pid: String,
        bundleName: String,
        startPort: Int,
    ): Int? {
        if (bundleName.isBlank()) {
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Connect-server fport SKIP — blank bundleName",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            return null
        }
        try {
            val hdcPath = org.cpf.kmp.tools.ohos.sync.DevEcoPathService.getInstance(session.project).getHdcPath()
            val arkPort = "ark:$pid@$bundleName"
            removeStaleConnectServerForwards(hdcPath, deviceId, startPort, arkPort)

            var port = startPort
            repeat(5) { attempt ->
                val command = listOf(hdcPath, "-t", deviceId, "fport", "tcp:$port", arkPort)
                pluginDbgPrintln("[ConnectServer] fport try#${attempt + 1} tcp:$port → $arkPort")
                val processBuilder = ProcessBuilder(command)
                processBuilder.redirectErrorStream(true)
                val process = processBuilder.start()
                val output = process.inputStream.bufferedReader().readText().trim()
                val exitCode = process.waitFor()
                pluginDbgPrintln("[ConnectServer] fport exit=$exitCode out=$output")
                val failed = exitCode != 0 ||
                    output.contains("[Fail]", ignoreCase = true) ||
                    output.contains("error", ignoreCase = true)
                val mapped = !failed && (
                    output.contains("OK", ignoreCase = true) ||
                        connectServerForwardPresent(hdcPath, deviceId, port, arkPort)
                    )
                if (mapped) {
                    session.printHarmonyDebugPipelineLine(
                        "$HARMONY_DEBUG_PIPELINE_MARKER Connect-server fport OK tcp:$port → $arkPort",
                    )
                    return port
                }
                session.printHarmonyDebugPipelineLine(
                    "$HARMONY_DEBUG_PIPELINE_MARKER Connect-server fport try tcp:$port → $arkPort " +
                        "failed (exit=$exitCode out=${output.ifBlank { "(empty)" }})",
                    ConsoleViewContentType.ERROR_OUTPUT,
                )
                // Port may be occupied — bump like arkcompiler (+7).
                try {
                    executeHdcCommand(hdcPath, deviceId, "fport", "rm", "tcp:$port", arkPort)
                } catch (_: Exception) {
                }
                port += 7
            }
            return null
        } catch (e: Exception) {
            LOG.error("Connect-server port forwarding error", e)
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Connect-server fport exception — ${e.message}",
                ConsoleViewContentType.ERROR_OUTPUT,
            )
            return null
        }
    }

    private fun removeStaleConnectServerForwards(
        hdcPath: String,
        deviceId: String,
        preferredPort: Int,
        arkTarget: String,
    ) {
        try {
            val listProcess = ProcessBuilder(listOf(hdcPath, "-t", deviceId, "fport", "ls"))
                .redirectErrorStream(true).start()
            val listOutput = listProcess.inputStream.bufferedReader().readText().trim()
            listProcess.waitFor(5, TimeUnit.SECONDS)
            val rulePattern = Regex("""(tcp:\d+)\s+(\S+)""")
            for (line in listOutput.lines()) {
                val match = rulePattern.find(line) ?: continue
                val tcp = match.groupValues[1]
                val target = match.groupValues[2]
                val portNum = tcp.removePrefix("tcp:").toIntOrNull() ?: continue
                val stalePreferred = tcp == "tcp:$preferredPort"
                val staleConnectRange = portNum in 15036..15100 && !target.contains("@Debugger")
                val staleSameTarget = target == arkTarget || target.endsWith("@${arkTarget.substringAfter('@')}")
                if (stalePreferred || (staleConnectRange && staleSameTarget) || target == arkTarget) {
                    pluginDbgPrintln("[ConnectServer] rm stale fport $tcp $target")
                    executeHdcCommand(hdcPath, deviceId, "fport", "rm", tcp, target)
                }
            }
        } catch (e: Exception) {
            LOG.warn("removeStaleConnectServerForwards: ${e.message}")
        }
    }

    private fun connectServerForwardPresent(
        hdcPath: String,
        deviceId: String,
        port: Int,
        arkTarget: String,
    ): Boolean {
        return try {
            val listProcess = ProcessBuilder(listOf(hdcPath, "-t", deviceId, "fport", "ls"))
                .redirectErrorStream(true).start()
            val listOutput = listProcess.inputStream.bufferedReader().readText().trim()
            listProcess.waitFor(5, TimeUnit.SECONDS)
            listOutput.lines().any { line ->
                line.contains("tcp:$port") && line.contains(arkTarget)
            }
        } catch (_: Exception) {
            false
        }
    }

    private fun startWebSocketDebugger(
        onProtocolReady: () -> Unit = {},
        onComplete: () -> Unit,
        onError: (String) -> Unit,
    ) {
        if (!OhosKmpCrossLanguageDebug.isEnabled()) {
            startWebSocketDebuggerLegacy(onComplete, onError)
            return
        }
        try {
            val client = WebSocketDebugClient()
            webSocketClient = client
            val backend = ArkTsDebugBackend(this, client)
            arkTsDebugBackend = backend
            crossLanguageCoordinator = CrossLanguageDebugCoordinator(this, backend)
            client.connect(
                host = "127.0.0.1",
                port = 15037,
                onConnected = {
                    backend.start(
                        onProtocolReady = onProtocolReady,
                        onComplete = {
                            reassertArkTsSuspendUiIfNeeded()
                            onComplete()
                        },
                        onError = onError,
                    )
                },
                onError = { error -> onError(error) },
            )
        } catch (e: Exception) {
            onError("WebSocket client error: ${e.message}")
        }
    }

    /** Pre-cross-language handshake only (Runtime.enable → Debugger.enable → runIfWaitingForDebugger). */
    private fun startWebSocketDebuggerLegacy(onComplete: () -> Unit, onError: (String) -> Unit) {
        try {
            webSocketClient = WebSocketDebugClient()
            webSocketClient?.connect(
                host = "127.0.0.1",
                port = 15037,
                onConnected = {
                    webSocketClient?.executeDebugStartSequence(
                        onComplete = onComplete,
                        onError = onError,
                    )
                },
                onError = { error -> onError(error) },
            )
        } catch (e: Exception) {
            onError("WebSocket client error: ${e.message}")
        }
    }

    // ========== Cleanup ==========
    // 0410 added
    private fun startDebugConsoleHilogMirror(pid: Int) {
        stopDebugConsoleHilogMirror()
        if (!OhosKmpDebugConsoleHilogMirror.isEnabled()) {
            LOG.info(
                "Debug Console HiLog mirror disabled (OhosKmpDebugConsoleHilogMirror: compile flag or " +
                    "-Dohos.kmp.debug.console.hilog.mirror / OHOS_KMP_DEBUG_CONSOLE_HILOG_MIRROR)",
            )
            return
        }
        val deviceId = environmentVariables["HDC_UTID"]?.trim().orEmpty()
        if (deviceId.isEmpty() || deviceId == "unknown") {
            LOG.info("Debug hilog mirror skipped: no HDC_UTID")
            return
        }
        if (session.project.isDisposed) return
        try {
            val hdcPath = DevEcoPathService.getInstance(session.project).getHdcPath()
            val cmd = listOf(hdcPath, "-t", deviceId, "shell", "hilog", "-P", pid.toString())
            val proc = ProcessBuilder(cmd).redirectErrorStream(true).start()
            debugHilogMirrorProcess = proc
            val bundleForParser = environmentVariables["OHOS_BUNDLE_NAME"]?.trim().orEmpty()
            val mirrorScope = DebugHilogMirrorFilter.scopeFromEnv(environmentVariables)
            val readerThread = Thread({
                val pidMap = mutableMapOf<String, String>()
                if (bundleForParser.isNotEmpty()) {
                    pidMap[pid.toString()] = bundleForParser
                }
                try {
                    proc.inputStream.bufferedReader().use { reader ->
                        reader.lineSequence().forEach { line ->
                            if (Thread.currentThread().isInterrupted || stopped) return@forEach
                            val entry = HiLogParser.parse(line, pidMap)
                            if (!DebugHilogMirrorFilter.allow(
                                    line,
                                    entry,
                                    bundleForParser.ifBlank { null },
                                    environmentVariables,
                                )
                            ) {
                                return@forEach
                            }
                            val text = when {
                                entry.message.isNotBlank() -> entry.message
                                line.isNotBlank() -> line
                                else -> return@forEach
                            }
                            val lineOut = if (text.endsWith("\n")) text else "$text\n"
                            ApplicationManager.getApplication().invokeLater {
                                if (stopped || session.project.isDisposed) return@invokeLater
                                try {
                                    session.consoleView?.print(lineOut, ConsoleViewContentType.NORMAL_OUTPUT)
                                } catch (_: Throwable) {
                                }
                            }
                        }
                    }
                } catch (_: Throwable) {
                }
            }, "OHOS-Debug-HiLog-$pid")
            readerThread.isDaemon = true
            debugHilogMirrorThread = readerThread
            readerThread.start()
            session.printHarmonyDebugPipelineLine(
                "$HARMONY_DEBUG_PIPELINE_MARKER Debug Console: HiLog mirror active (hilog -P $pid, scope=$mirrorScope); device logs filtered per OHOS_DEBUG_HILOG_SCOPE",
            )
            LOG.info(
                "Debug Console hilog mirror started (pid=$pid, device=$deviceId, scope=$mirrorScope, bundle=${bundleForParser.ifBlank { "-" }})",
            )
            if (mirrorScope != DebugHilogMirrorFilter.Scope.FULL) {
                LOG.info(DebugHilogMirrorFilter.ideLogFilteredModeHint(mirrorScope))
            }
        } catch (e: Exception) {
            LOG.warn("Failed to start hilog mirror for Debug Console: ${e.message}")
        }
    }

    private fun stopDebugConsoleHilogMirror() {
        try {
            debugHilogMirrorProcess?.destroyForcibly()
        } catch (_: Throwable) {
        }
        debugHilogMirrorProcess = null
        try {
            debugHilogMirrorThread?.interrupt()
        } catch (_: Throwable) {
        }
        debugHilogMirrorThread = null
    }

    /** On Attach or Launch debug, when the user stops this session: print one line to the Debug **Console**. */
    private fun printConnectionClosedToConsole() {
        val cv = session.consoleView ?: return
        val line = "Connection has been closed\n"
        val app = ApplicationManager.getApplication()
        val print = Runnable {
            try {
                cv.print(line, ConsoleViewContentType.SYSTEM_OUTPUT)
            } catch (_: Throwable) {
            }
        }
        if (app.isDispatchThread) {
            print.run()
        } else {
            app.invokeLater(print)
        }
    }

    override fun stop() {
        if (stopped) {
            LOG.info("Debug process already stopped, skipping duplicate stop")
            return
        }
        stopPleaseWaitConsoleAnimation()
        lldbTabRegisterAlarm.cancelAllRequests()
        resetInferiorCoroutineStepState("debug process stop")
        // 0411 added
        stopDebugConsoleHilogMirror()
        coroutineSupport.dispose()
        stopped = true
        nativeLldbHandoffInFlight = false
        clearStepIntoBoundaryBreakpoint()
        ApplicationManager.getApplication().invokeLater {
            try {
                session.setPauseActionSupported(false)
            } catch (_: Throwable) {
            }
            coroutinesDebugPanel?.resetIdle()
        }
        clearUserContinueOrStepPending()
        LOG.info("=== Stopping debug process ===")
        lldbShutdownPrintln(
            "[LLDB-SHUTDOWN][tid=${Thread.currentThread().id}] LLDBDebugProcess.stop BEGIN " +
                "session=${session.project.name} thread=${Thread.currentThread().name}",
        )

        printConnectionClosedToConsole()

        val app = ApplicationManager.getApplication()
        val cleanup = Runnable {
            try {
                unregisterIdeBreakpointListeners()
            } catch (e: Throwable) {
                LOG.warn("unregisterIdeBreakpointListeners: ${e.message}")
            }

            clearSuspendedStackCache()

            try {
                multiInstanceManager?.stopAll()
                multiInstanceManager = null
                connectServerClient?.close()
                connectServerClient = null
                webSocketClient?.close()
                webSocketClient = null
                arkTsDebugBackend?.close()
                arkTsDebugBackend = null
                crossLanguageCoordinator = null
            } catch (e: Throwable) {
                LOG.warn("Error closing WebSocket: ${e.message}")
            }
            val disasmFile = disassemblyEditorFile
            disassemblyEditorFile = null
            disassemblyEditorUserPinned = false
            glueDisassemblyInstructionStepping = false
            disassemblyFocusGeneration++
            lldbDisassemblyView = null
            lldbTerminalView = null
            if (disasmFile != null && !session.project.isDisposed) {
                val diskPath = if (isDisassemblyDiskStub(disasmFile)) {
                    try {
                        Paths.get(disasmFile.path)
                    } catch (_: Throwable) {
                        null
                    }
                } else {
                    null
                }
                ApplicationManager.getApplication().invokeLater {
                    try {
                        FileEditorManager.getInstance(session.project).closeFile(disasmFile)
                    } catch (_: Throwable) {
                    }
                    if (diskPath != null) {
                        try {
                            Files.deleteIfExists(diskPath)
                        } catch (_: Throwable) {
                        }
                    }
                }
            }

            try {
                lldbShutdownPrintln("[LLDB-SHUTDOWN] cleanup: stopEventLoop (via dispose next)")
            } catch (_: Throwable) {
            }

            try {
                cleanupDeviceProcesses()
            } catch (e: Throwable) {
                LOG.warn("Device cleanup error: ${e.message}")
            }

            try {
                lldbShutdownPrintln("[LLDB-SHUTDOWN] cleanup: serviceWrapper.dispose()")
                serviceWrapper.dispose()
            } catch (e: Throwable) {
                lldbShutdownPrintln("[LLDB-SHUTDOWN] cleanup: dispose exception ${e.message}")
                LOG.warn("Error disposing ServiceWrapper: ${e.message}")
            }

            lldbShutdownPrintln("[LLDB-SHUTDOWN] LLDBDebugProcess.stop END")
            LOG.info("=== Debug process fully stopped ===")
        }

        // dispose() -> stopEventLoop() joins up to ~45s — never run that on the EDT.
        if (app.isDispatchThread) {
            app.executeOnPooledThread(cleanup)
        } else {
            cleanup.run()
        }
    }

    private fun cleanupDeviceProcesses() {
        val deviceId = environmentVariables["HDC_UTID"]
        if (deviceId.isNullOrEmpty()) {
            LOG.info("No device id, skipping device-side cleanup")
            return
        }

        try {
            val hdcPath = org.cpf.kmp.tools.ohos.sync.DevEcoPathService.getInstance(session.project).getHdcPath()

            val bundleName = debugModeContext?.bundleName
            if (!bundleName.isNullOrEmpty()) {
                LOG.info("Force-stopping app: $bundleName")
                executeHdcCommand(hdcPath, deviceId, "shell", "aa", "force-stop", bundleName)
            }

            LOG.info("Cleaning up port forwarding rules...")
            cleanupPortForwardingOnStop(hdcPath, deviceId)

        } catch (e: Exception) {
            LOG.warn("Device-side cleanup error (non-fatal)", e)
        }
    }

    private fun cleanupPortForwardingOnStop(hdcPath: String, deviceId: String) {
        try {
            val listProcess = ProcessBuilder(listOf(hdcPath, "-t", deviceId, "fport", "ls"))
                .redirectErrorStream(true).start()
            val listOutput = listProcess.inputStream.bufferedReader().readText().trim()
            listProcess.waitFor(5, java.util.concurrent.TimeUnit.SECONDS)

            for (line in listOutput.lines()) {
                if (line.contains("tcp:15037") || line.contains("tcp:15036") ||
                    line.contains("@Debugger") || Regex("""ark:\d+@[^@\s]+$""").containsMatchIn(line)
                ) {
                    val rulePattern = Regex("""(tcp:\d+)\s+(\S+)""")
                    val match = rulePattern.find(line) ?: continue
                    val tcpPort = match.groupValues[1]
                    val arkTarget = match.groupValues[2]
                    // Keep unrelated forwards; remove debugger / connect-server ark rules.
                    if (arkTarget.contains("@Debugger") ||
                        tcpPort == "tcp:15037" ||
                        tcpPort == "tcp:15036" ||
                        (arkTarget.startsWith("ark:") && !arkTarget.contains("lldb"))
                    ) {
                        LOG.info("Removing port forward: $tcpPort $arkTarget")
                        executeHdcCommand(hdcPath, deviceId, "fport", "rm", tcpPort, arkTarget)
                    }
                }
            }
        } catch (e: Exception) {
            LOG.warn("Port forward cleanup error", e)
        }
    }

    private fun executeHdcCommand(hdcPath: String, deviceId: String, vararg args: String) {
        try {
            val command = listOf(hdcPath, "-t", deviceId) + args.toList()
            val process = ProcessBuilder(command).redirectErrorStream(true).start()
            process.waitFor(10, java.util.concurrent.TimeUnit.SECONDS)
            if (process.isAlive) {
                process.destroyForcibly()
            }
        } catch (e: Exception) {
            LOG.warn("hdc command error: ${args.joinToString(" ")}", e)
        }
    }
}
