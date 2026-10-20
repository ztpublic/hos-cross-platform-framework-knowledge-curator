// OHPlot Main Layout - overlays on top of XComponent (Qt render)
// Phase 1: XComponent remains visible as backup underneath
//
// CRITICAL EVENT FLOW (device-verified 2026-07-27 via InputProbe src= field):
// - Finger touches: the QPA touch pipeline delivers them natively and Qt
//   synthesizes mouse events itself (src=2).  The overlay ALSO forwards
//   them (src=3) because TouchEvent.source/sourceTool cannot distinguish
//   finger from mouse-converted touches (both report TouchScreen/Finger).
//   The resulting double delivery is deduplicated in Qt (InputProbe).
// - Bluetooth LEFT clicks: ArkUI re-routes them into the touch pipeline;
//   the QPA plugin drops them, so only the forwarded copy reaches Qt.
// - Bluetooth RIGHT/MIDDLE clicks: ArkUI onMouse on the outer Stack catches
//   these and forwards via sendMouse 鈫?scidavis_inject_mouse 鈫?Qt.
// - MenuBar area: ArkTS handles all menu UI natively.
// - Content area: HitTestMode.Transparent lets events pass through to
//   the XComponent underneath for native QPA processing.
// - StatusBar area: ArkTS handles status display.
// - WARNING: Do NOT forward touch events via sendMouse 鈥?the QPA plugin
//   already handles them natively.  Forwarding duplicates causes crashes.
import { MenuBar, DropdownMenu } from '../components/MenuBar';
import { ContextMenu } from '../components/ContextMenu';
import { ToolBar } from '../components/ToolBar';
import { BottomToolBar } from '../components/BottomToolBar';
import {
  ACCENT, TEXT, TEXT_LIST, TEXT_EMPTY,
  BTN_BG, BTN_TEXT, RADIUS_4, RADIUS_8, BTN_HEIGHT,
  FS_DIALOG_TITLE, FS_DIALOG_LABEL, FS_DIALOG_SMALL,
  SPACING_4, SPACING_6, SPACING_12, SPACING_16,
  BACKDROP_COLOR, SHADOW_RADIUS, SHADOW_COLOR, SHADOW_OFFSET_X, SHADOW_OFFSET_Y
} from '../components/Theme';
import { StatusBar } from '../components/StatusBar';
import { QtEventHost } from '../components/QtEventHost';
import { CurveRangeDialog } from './dialogs/CurveRangeDialog';
import { AssociationsDialog } from './dialogs/AssociationsDialog';
import { LayerDialog } from './dialogs/LayerDialog';
import { getAppString } from '../components/LocaleHelper';
import hilog from '@ohos.hilog';
import qohos from 'libqohos.so';
import { picker, fileUri, fileShare } from '@kit.CoreFileKit';
import fs from '@ohos.file.fs';
import { preferences } from '@kit.ArkData';
import pasteboard from '@ohos.pasteboard';
import common from '@ohos.app.ability.common';
import display from '@ohos.display';

interface QohosInjector {
  sendMouse(x: number, y: number, button: number, action: number): boolean;
  sendWheel(x: number, y: number, deltaY: number): boolean;
  callQtCommand(cmd: string, argsJson: string): string;
}

const qtInjector = qohos as QohosInjector;

const DOMAIN: number = 0x0000;
const TAG: string = 'QtInput';

// ArkTS chrome bands that must NOT be forwarded to Qt (vp).  A forwarded
// tap on the Qt menu bar (hidden underneath the ArkTS one) makes Qt call
// QMenu::popup() 鈫?QPA window creation 鈫?SIGSEGV: the alpha_v6 plugin
// supports only the single main XComponent window (device crash log
// cppcrash-...-20260727151314, QOpenHarmonyJsObject::getJsFunction).
const MENUBAR_HEIGHT_VP: number = 28;
// The toolbar height is DYNAMIC (grouped toolbars wrap onto 1..n rows and
// context groups show/hide with the active window type), so the full top
// chrome band is MENUBAR_HEIGHT_VP + the measured toolbar height 鈥?see
// topChromeVp().  That band must never be forwarded to Qt (the hidden Qt
// menu bar sits underneath).
const STATUSBAR_HEIGHT_VP: number = 24;
// Results Log auto-scroll: distance (vp) from bottom at which the user
// is considered "at the bottom" vs "scrolled up".  Smaller = stricter.
// (removed — Results Log panel is now a Qt dock widget)

// Mirror of the Qt-side menuAction whitelist (main.cpp command registry).
// menuAction is a *mutation* command: scidavis_call queues it and returns
// {"success":true,"queued":true} immediately, so the Qt-side "unsupported"
// error can never reach ETS through the return value.  Unadapted items must
// therefore be gated here, before calling Qt.  Keep in sync with main.cpp.
const MENU_ACTION_SUPPORTED: string[] = [
  'new_notes', 'new_matrix', 'new_project', 'new_graph',
  'cascade',
  'about', 'undo', 'redo', 'cut', 'copy', 'paste', 'delete',
  'clear_selection',
  'table_size', 'time_stamp',
  'new_legend', 'normalize', 'convolve',
  // Stage 6: 原 add_text/draw_arrow/draw_line/add_image/format_title 改由
  // show_text/show_line/show_image 分支承接（C++ 已落地，main.cpp
  // L3216/3226/3236，内嵌 Qt 对话框）；下述新分支为右键曲线/标记菜单的
  // 直连 itemId。
  'show_text', 'show_line', 'show_image',
  'edit_range', 'associations', 'plot_details', 'window_geometry',
  'arrange_layers',
  'format_plot', 'format_scales', 'format_axes', 'format_grid',
  // D2 (2026-08-14): Window 菜单补齐 — tile/next/previous/duplicate/
  // hide_window（C++ menuAction 分支已落地，main.cpp）。
  'tile', 'next', 'previous', 'duplicate', 'hide_window',
  // D2 (2026-08-14): Matrix 菜单补齐 — recalculate/clear/mirror_v/
  // determinant/convert_to_table（main.cpp menuAction 分支已落地）。
  'matrix_recalculate', 'matrix_clear', 'matrix_mirror_v',
  'matrix_determinant', 'matrix_convert_to_table',
  // D2 (2026-08-14): Table 菜单补齐 — show_comments/recalculate/
  // go_to_cell/convert_to_matrix（main.cpp menuAction 分支已落地）。
  'show_comments', 'recalculate', 'go_to_cell', 'convert_to_matrix',
  // D3 (2026-08-14): Table→Sort / Table→Dimensions / View→Plot Wizard
  // 迁 Qt 内嵌（C++ menuAction 分支落地）。
  'sort_table', 'sort', 'table_size', 'table_dimensions', 'dimensions',
  'plot_wizard',
  // D3 (2026-08-14): Quick Fit → Fit Multi-Peak（C++ menuAction 分支）。
  'fit_multi_peak_gauss', 'fit_multi_peak_lorentz',
  // D3 (2026-08-14): Tools 工具链复活（QwtPicker 完整版恢复）。
  'disable_tools', 'zoom_in', 'zoom_out', 'screen_reader',
  'data_reader', 'range', 'move_points', 'remove_bad',
  // D3 (2026-08-14): Table/Matrix 剩余接线（hide_controls F12 /
  // formula_edit_mode / edit_column_description / change_type_format /
  // clear_table / matrix_hide_controls / matrix_mirror_h）。
  'hide_controls', 'formula_edit_mode', 'edit_column_description',
  'change_type_format', 'clear_table', 'matrix_hide_controls', 'matrix_mirror_h',
  // D3 (2026-08-14): Graph 图层 / Edit 清理 / Analysis 平移。
  'add_layer', 'remove_layer', 'delete_fit_tables', 'clear_log_info',
  'translate_horizontal', 'translate_vertical',
  // Phase-1 (2026-08-15): 菜单快赢对齐接线新增（C++ 分支已落地 main.cpp）。
  'automatic_layout', 'restart_scripting', 'open_template',
  'save_as_template', 'new_function_plot', 'an_fft',
  // P3 (2026-08-16): Note 激活时 Scripting 菜单追加项（Win L1235-1245）。
  'note_execute', 'note_execute_all', 'note_evaluate'
];

let leftDown: boolean = false;

// Cross-channel left-click dedup: with easy_go mouse2TouchEventMode=disabled,
// a physical left click may still reach BOTH onTouch (Down) and onMouse
// (Press), so Qt would see two presses within 400ms and synthesize a
// DblClick (single click -> edit).  A real double-click always interleaves a
// release between the two presses; the duplicate channel does not.  Track the
// last forwarded left press and drop a same-position press that arrives
// before any release was forwarded (Oracle 2026-08-06).
let lastLeftPressTime: number = 0;
let lastLeftPressX: number = 0;
let lastLeftPressY: number = 0;
let leftReleaseSeen: boolean = true;

interface SubMenuItemData {
  id: string;
  label: string;
  icon?: ResourceStr;
  shortcut?: string;
  enabled?: boolean;
  checked?: boolean;
  children?: SubMenuItemData[];
}

interface MenuItemData {
  id: string;
  label: string;
  items?: SubMenuItemData[];
}

// Payload for the generic 'menuAction' fallback command.
interface MenuActionArgs {
  itemId: string;
}

// Result envelope returned by callQtCommand for menuAction.
interface CmdResult {
  success: boolean;
  error?: string;
}

// tableStatistics command payload (queued; result toast via event channel).
interface TableStatisticsArgs {
  tableId: string;
  type: string;
}

// tableSize command payload (Phase 4).
interface TableSizeArgs {
  tableId: string;
}

// addText / drawArrow / drawLine / timeStamp / addImage / newLegend payload.
interface PlotActionArgs {
  plotId: string;
}

// plotStackedHistogram payload (C++ resolveTable reads args["tableId"]).
interface PlotTableArgs {
  tableId: string;
}

// plot2D command payload: no cols array, so the Qt side falls back to the
// active table's selectedColumns() (Win parity: direct plot, no picker).
interface PlotArgs {
  tableId: string;
  type: string;
}

// getPlotList response (export_graph_all): each MultiLayer window entry.
interface PlotListEntry {
  name: string;
  label: string;
  layers: number;
}

interface PlotListResponse {
  success: boolean;
  data?: PlotListEntry[];
  error?: string;
}

// plotPanel command payload (panel_2h/panel_2v): cols/rows mirror
// ApplicationWindow::plot2HorizontalLayers/plot2VerticalLayers
// (multilayerPlot(2,1) resp. multilayerPlot(1,2)).
interface PanelArgs {
  tableId: string;
  cols: number;
  rows: number;
}

// setClipboardText payload (Phase 2 clipboard bridge, OHOS 鈫?Qt).
interface SetClipboardArgs {
  text: string;
}

// setWorkFolderPath payload (route 5, 2026-08-14): the ArkTS-authorized
// work folder (real path from FileUri.path).  Pushed to Qt so embedded
// QFileDialog instances browse inside the authorized tree.
interface WorkFolderArgs {
  path: string;
}

// checkWorkFolderAccess response (route 5, 2026-08-14): whether the stored
// work folder is still directly readable from Qt after a restart.
interface WorkFolderAccessResponse {
  success: boolean;
  exists?: boolean;
  isDir?: boolean;
  entryCount?: number;
  readable?: boolean;
}

// setChromeInsets payload: physical-pixel top/bottom margins Qt applies
// to the QMainWindow so the MDI area clears the ArkTS chrome
// (top/bottom bands).  The Qt-native table column panel is rendered by
// Qt itself (TableView d_control_tabs), so no right inset is needed.
interface ChromeInsetsArgs {
  top: number;
  bottom: number;
}

// setMenuBottomLimit payload (2026-08-17): ArkTS-measured bottom-toolbar
// top edge in global physical px.  Qt uses this as the authoritative inline
// menu bottom limit so the menu bottom aligns to the real overlay even if
// the Qt window and ArkTS chrome are misaligned by a few px.
interface MenuBottomLimitArgs {
  globalY: number;
}

// matrixTranspose / matrixInvert payload (Phase 3).
interface MatrixIdArgs {
  matrixId: string;
}

// exportGraphImage payload (Phase 3).
interface ExportImageArgs {
  plotId: string;
  path: string;
}

interface ActivateWindowArgs {
  name: string;
}

// openProject / saveProject command payload (both queued; completion is
// signalled by the projectSaved event handled in QtEventHost).
interface ProjectPathArgs {
  path: string;
}

// getRecentProjects response (Stage-2A): Qt-side recentProjects QStringList,
// most recent first, max 10 (ApplicationWindow::MaxRecentProjects).
interface RecentProjectsResponse {
  success: boolean;
  projects: string[];
}

// isProjectModified response (2026-08-15): unsaved-changes flag for the
// quit/new-project confirmation (mirrors ApplicationWindow::saved).
interface IsModifiedResponse {
  success: boolean;
  modified: boolean;
}

// getUiState payload (see uiStateJson in scidavis/src/main.cpp)
interface UiWindow {
  name: string;
  type: string;
  label: string;
  status: number;
  active: boolean;
}

interface UiState {
  activeType: string;
  activeName: string;
  windows: UiWindow[];
  undoEnabled: boolean;
  redoEnabled: boolean;
}

interface UiStateResponse {
  success: boolean;
  data?: UiState;
  explorerVisible?: boolean;
  logVisible?: boolean;
}

@Component
export struct MainLayout {
  @State @Watch('onMenuChanged') openMenu: string | null = null;
  @State statusText: string = '';
  // Qt first-frame readiness: set true once getUiState reports a non-empty
  // active window (see refreshUiState).  Gates the startup splash overlay
  // that hides the XComponent's black surface before Qt renders its first
  // frame.  Only ever set true, never reset.  (2026-08-04)
  @State qtReady: boolean = false;
  // Right-click context menu state
  @State showContextMenu: boolean = false;
  @State contextMenuX: number = 0;
  @State contextMenuY: number = 0;
  // Right-click target (from the Qt contextMenuRequest event): the widget
  // class under the cursor decides which context menu to build (curve menu
  // for 'MultiLayer'/'Graph3D', blank workspace menu otherwise).  markerType
  // ('text'/'arrow'/'image') is the plot item hit — C++ InputProbe is adding
  // the field; until then the marker menu branch stays inert and the menu
  // falls back to the widgetClass branch.  The event has no coordinates, so
  // position comes from the onMouse right-press handler recorded just before.
  @State contextMenuWidgetClass: string = '';
  @State contextMenuMarkerType: string = '';
  // Table-operation dialogs (Phase 2); tableOpsTarget is the active table
  // captured at menu-dispatch time (uiState refreshes on every menu-open).
  // AddColumns/SetColumnValues are Qt-native menuActions now (batch A).
  // Batch-4 dialogs: 10 UI dialogs mounted but hidden until opened via dispatchMenuItem.
  @State showLayerDialog: boolean = false;
  // Stage-7 dialogs (about/scripting_language/rename_window) are now
  // Qt-native embedded; the ArkTS mimics are gone.
  // Stage-6 dialogs: ArkTS fallbacks for the curve context menu.  The C++
  // menuAction branches 'edit_range'/'associations'（另一代理添加中）落地后
  // 走 Qt 内嵌对话框；在此之前右键菜单落到这两个 ArkTS 对话框
  // （getCurveInfo/setCurveRange/getPlotAssociations/updatePlotAssociation
  // 数据命令已就绪）。
  @State showCurveRangeDialog: boolean = false;
  @State showAssociationsDialog: boolean = false;
  // Batch-5 dialogs: Format (Tools menu is Qt-native since batch B)
  // 2026-08-14 文件夹级授权直通（路线5 探针验证）: 用户通过 FOLDER picker
  // 授权一个工作目录后，Qt 可直接读写该目录（无需拷贝进沙箱）。所有
  // 文件导入/导出/打开/保存走该目录。授权为进程级（重启后需重新授权；
  // persistPermission 持久化列为远期）。
  @State workFolderPath: string = '';
  // 工作文件夹内待选文件（Import/Open 用），由 fs.listFileSync 填充。
  @State workFolderFiles: string[] = [];
  @State showWorkFolderOpenDialog: boolean = false;
  // Results Log (View menu): Qt-native logWindow dock (toggleLog).  The
  // ArkTS replacement panel was removed on 2026-08-14 (issue list #2): the
  // Qt dock renders inside the main window and its QTextEdit scrolls via
  // the wheel-injection chain, so all entries (menu/toolbar/context menu)
  // now call Qt toggleLog instead of showing a local panel.
  @State dockLogVisible: boolean = false;
  // Long-press-to-right-click state (touch -> context menu).  Issue list
  // #6 (2026-08-14): the triggering source is captured at Down and only
  // TouchScreen (real finger) long-presses open the menu — Bluetooth
  // mouse left long-press does not (a physical right button exists).
  private lastTouchX: number = 0;
  private lastTouchY: number = 0;
  private touchStartTime: number = 0;
  private longPressSource: number = 0;
  private LONG_PRESS_DURATION_MS: number = 500;
  private LONG_PRESS_MAX_MOVE_VP: number = 20;
  // P1 (2026-08-12): sendMouse Move throttle — the synchronous NAPI call
  // can block the main thread (Qt busy → up to 3s); rapid drags flood it
  // and starve the input pump.  Move (action=2) is merged into 16ms
  // windows keeping the latest coords; Down/Up are never throttled.
  private MOUSE_MOVE_THROTTLE_MS: number = 16;
  private mouseMoveQueued: boolean = false;
  private mouseMoveQueuedX: number = 0;
  private mouseMoveQueuedY: number = 0;
  private mouseMoveQueuedBtn: number = 0;
  private mouseMoveLastSent: number = 0;
  // Project browser sidebar: replaced by Qt dock widget; state kept only
  // for menu checkmark mirroring (dockExplorerVisible).
  @State dockExplorerVisible: boolean = false;
  // Dynamic toolbar height (vp), measured via onAreaChange on the ToolBar:
  // the grouped toolbar wraps onto 1..n rows and its context groups
  // show/hide with the active window type, so the chrome band cannot be a
  // constant any more.
  @State toolbarHeightVp: number = 32;
  // Height (vp) of the bottom toolbar (2D-plot + 3D-surface groups), only
  // present while a Table is active; 0 otherwise.  Folded into the Qt
  // bottom chrome inset so the MDI area clears the docked bar.
  @State bottomToolbarHeightVp: number = 0;
  // Custom tooltip overlay for BottomToolBar buttons (bindPopup is occluded
  // by the XComponent native rendering layer; this overlay sits at Stack
  // level with zIndex 300 to appear above XComponent).
  @State tooltipText: string = '';
  @State tooltipX: number = 0;
  @State tooltipY: number = 0;
  @State tooltipShow: boolean = false;
  // Global y of the BottomToolBar top edge, used to position the tooltip
  // above the hovered button.
  @State bottomBarTopY: number = 0;
  // setChromeInsets startup retry: the first push can race the Qt event
  // loop ("Qt not running"), so retry until the queued ack arrives.
  private insetsRetry: number = 0;
  private uiRefreshRetry: number = 0;
  // P0 (2026-08-12): wait budget (1s ticks) for the qtReady signal before
  // the startup uiState poll may issue its first getUiState (see
  // scheduleStartupUiRefresh).  Distinct from uiRefreshRetry so the
  // post-qtReady poll budget is untouched.
  private qtReadyWaitRetry: number = 0;
  // Sandbox path of the current project file ('' = untitled).  The picker
  // flows copy external files in/out of the sandbox because Qt can only
  // reach sandbox paths, not content URIs.
  @State projectPath: string = '';
  // Language switch pipeline: OHPlotAbility.onConfigurationUpdate bumps
  // langTick on every language change; menu labels are data-model strings
  // (not $r bindings), so they must be rebuilt explicitly.
  @StorageProp('langTick') @Watch('onLangChanged') langTick: number = 0;
  // Bug 2: Qt→ArkTS window-activation tick.  QtEventHost bumps it on every
  // windowActivated event (an MDI sub-window was raised/clicked); the Watch
  // re-queries uiState and rebuilds the menus so Graph/Tools/Format follow
  // the newly active window type immediately.
  @StorageProp('activeWindowTick') @Watch('onActiveWindowChanged') activeWindowTick: number = 0;
  // x offset (vp) of the floating dropdown, looked up from the anchor map
  // whenever a menu opens so it appears under its title.
  @State dropdownX: number = 0;
  private menuAnchorMap: Map<string, number> = new Map<string, number>();

  onMenuChanged(): void {
    if (this.openMenu !== null) {
      this.statusText = '[' + this.openMenu + ' menu open]';
      const x: number | undefined = this.menuAnchorMap.get(this.openMenu);
      this.dropdownX = (x !== undefined) ? x : 0;
      // Refresh backend UI state on every menu-open so greying/checkmarks
      // and the Windows list are current.  getUiState is a query command:
      // Qt answers via QueuedConnection with a 300ms semaphore fallback
      // (P0 2026-08-12: was 3s), so the worst-case stall is bounded
      // (normally single-digit ms when idle).
      this.refreshUiState();
    } else {
      this.statusText = '';
    }
  }

  onLangChanged(): void {
    this.rebuildMenus();
  }

  // Bug 2: an MDI sub-window was activated in the Qt area.  Refresh the
  // backend uiState so the menu bar switches to the newly active window
  // type right away (refreshUiState ends in rebuildMenus()).
  // Defer the refresh so the Qt thread finishes creating/activating the
  // window: an immediate synchronous getUiState can time out while Qt is
  // busy (device-verified: Matrix creation → timeout → stale menus).
  onActiveWindowChanged(): void {
    setTimeout(() => {
      this.refreshUiState();
    }, 250);
  }

  aboutToAppear(): void {
    this.rebuildMenus(); // initial static skeleton before Qt is queried
    this.schedulePushChromeInsets(); // tell Qt to clear the ArkTS chrome
    // 2026-08-14: restore the persisted work folder (route 5).  The FOLDER
    // grant itself is process-scoped — it is re-verified lazily: we push the
    // path to Qt anyway (harmless if stale); a later file op that fails on
    // access re-prompts via ensureWorkFolder.  Persisting the PATH removes
    // the re-pick ceremony for the common case.
    this.restoreWorkFolder();
  }

  // Restore the persisted work folder path (route 5, 2026-08-14).  The
  // FOLDER grant is process-scoped on OpenHarmony, so after a restart the
  // stored path may exist but the access is gone.  Since 2026-08-14 the
  // grant is ALSO persisted via fileShare.persistPermission (API 11+,
  // ohos.permission.FILE_ACCESS_PERSIST = normal/system_grant) — restore
  // tries activatePermission FIRST; only if the persistent policy is
  // missing or the path is still unreadable does it drop the path so the
  // next file op re-prompts the FOLDER picker.
  private async restoreWorkFolder(): Promise<void> {
    try {
      const ctx = getContext(this) as common.UIAbilityContext;
      const store = await preferences.getPreferences(ctx, 'ohplot_workfolder');
      const saved: string = store.getSync('workFolderPath', '') as string;
      if (saved === '' || saved === undefined) {
        return;
      }
      const savedUri: string = store.getSync('workFolderUri', '') as string;
      // Reactivate the persisted grant (if any) BEFORE asking Qt: on a
      // fresh process the FOLDER picker grant is gone, but the persisted
      // policy can re-grant it without any user dialog.
      if (savedUri !== '') {
        await this.activatePersistedGrant(savedUri);
      }
      // Verify the grant survived the restart: ask Qt whether the path is
      // still directly readable.  A stale path (list empty / not exists)
      // must NOT be pushed — Qt would open an empty file dialog and the
      // user would think the app is broken (图1, 2026-08-14).
      // Use ArkTS fs.listFileSync rather than Qt checkWorkFolderAccess: a
      // denied path can also look "readable" to QDir with 0 entries, which
      // left the persisted stale path in place and opened an empty dialog.
      const accessible: boolean = this.isPathAccessible(saved);
      if (!accessible) {
        // Grant still missing after reactivation attempt.  Clear the
        // persisted path — the next file op goes through ensureWorkFolder
        // and re-prompts the FOLDER picker automatically.
        this.workFolderPath = '';
        AppStorage.setOrCreate<string>('workFolderPath', '');
        store.deleteSync('workFolderPath');
        store.deleteSync('workFolderUri');
        await store.flush();
        hilog.warn(DOMAIN, TAG, 'workFolder grant expired after restart — cleared: %{public}s', saved);
        return;
      }
      this.workFolderPath = saved;
      AppStorage.setOrCreate<string>('workFolderPath', saved);
      try {
        qtInjector.callQtCommand('setWorkFolderPath',
          JSON.stringify({ path: saved } as WorkFolderArgs));
        hilog.info(DOMAIN, TAG, 'workFolder restored from prefs: %{public}s', saved);
      } catch (e) {
        hilog.error(DOMAIN, TAG, 'restoreWorkFolder push failed: %{public}s', (e as Error).message);
      }
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'restoreWorkFolder failed: %{public}s', JSON.stringify(e));
    }
  }

  // Persist the folder grant across restarts (fileShare.persistPermission,
  // API 11+).  Without this the FOLDER picker grant is process-scoped and
  // dies on restart; with it, restoreWorkFolder can re-activate silently.
  private async persistWorkFolderGrant(uri: string): Promise<boolean> {
    try {
      const mode = fileShare.OperationMode.READ_MODE | fileShare.OperationMode.WRITE_MODE
        | fileShare.OperationMode.CREATE_MODE | fileShare.OperationMode.DELETE_MODE
        | fileShare.OperationMode.RENAME_MODE;
      await fileShare.persistPermission([{ uri: uri, operationMode: mode } as fileShare.PolicyInfo]);
      hilog.info(DOMAIN, TAG, 'workFolder grant persisted: %{public}s', uri);
      return true;
    } catch (e) {
      // 801 (capability unsupported) / 13900001 (not permitted) — the
      // process-scoped grant still works for this session.
      hilog.warn(DOMAIN, TAG, 'persistPermission failed (session-only grant): %{public}s',
        JSON.stringify(e));
      return false;
    }
  }

  // Re-activate a previously persisted folder grant on a fresh process.
  private async activatePersistedGrant(uri: string): Promise<void> {
    try {
      const mode = fileShare.OperationMode.READ_MODE | fileShare.OperationMode.WRITE_MODE
        | fileShare.OperationMode.CREATE_MODE | fileShare.OperationMode.DELETE_MODE
        | fileShare.OperationMode.RENAME_MODE;
      const checked: boolean[] = await fileShare.checkPersistentPermission(
        [{ uri: uri, operationMode: mode } as fileShare.PolicyInfo]);
      if (checked.length > 0 && checked[0]) {
        await fileShare.activatePermission(
          [{ uri: uri, operationMode: mode } as fileShare.PolicyInfo]);
        hilog.info(DOMAIN, TAG, 'persisted workFolder grant reactivated: %{public}s', uri);
      } else {
        hilog.info(DOMAIN, TAG, 'no persistent grant for %{public}s — picker will re-prompt', uri);
      }
    } catch (e) {
      hilog.warn(DOMAIN, TAG, 'activatePersistedGrant failed: %{public}s', JSON.stringify(e));
    }
  }

  // Full ArkTS top chrome height (vp): menu bar + measured toolbar rows.
  private topChromeVp(): number {
    return MENUBAR_HEIGHT_VP + this.toolbarHeightVp;
  }

  // Push the chrome insets to Qt (setChromeInsets 鈫?QMainWindow content
  // Unified hover-tip handler for both ToolBar (top) and BottomToolBar
  // (bottom).  Renders a yellow tooltip overlay at Stack level (zIndex 300)
  // above the XComponent native layer.  The caller supplies x/y so the
  // overlay can appear below (top toolbar) or above (bottom toolbar) the
  // hovered button.
  private handleHoverTip(tipKey: string, x: number, y: number, show: boolean): void {
    if (show) {
      this.tooltipText = getAppString(tipKey);
      this.tooltipX = x;
      // Tooltip positions come from ArkUI globalPosition, which on 2in1 is
      // relative to the FULL window (system decor included) while the
      // overlay (and this .position()) uses content-area coordinates —
      // compensate by the decor height (device: 37px, tablet 0 → no-op).
      this.tooltipY = y - (AppStorage.get<number>('qtDecorH') ?? 0);
      this.tooltipShow = true;
    } else {
      this.tooltipShow = false;
    }
  }

  // margins in physical px).  Queued fire-and-forget on the Qt side;
  // returns false while Qt is not running yet so the retry loop keeps
  // going.
  //
  // Bottom white-strip root cause (fixed 2026-08-06): the old bottom
  // formula (STATUSBAR_HEIGHT_VP = 24vp inflated constant + onAreaChange
  // toolbar height, itself inflated ~55vp) overshot the real chrome by
  // ~67px, so the Qt viewport shrank too far and exposed the QMdiArea
  // frame's white background below it.  The bottom inset is now computed
  // from the MEASURED BottomToolBar top edge (bottomBarTopY, vp, captured
  // by the toolbar's onAreaChange): screen height px - vp2px(bottomBarTopY),
  // i.e. the Qt viewport bottom lands exactly on the toolbar's top border.
  // The old estimate survives only as a pre-first-callback fallback.
  private pushChromeInsets(): boolean {
    try {
      // Use the UIContext-bound vp2px, NOT the deprecated global vp2px():
      // the global one falls back to the DEFAULT screen's virtual pixel
      // ratio when the UI instance is ambiguous, which can disagree with
      // the real UI density (device DPR=2.2).  A wrong conversion makes
      // the Qt MDI top inset mismatch the ArkTS overlay bottom edge and
      // the overlay covers the first rows of tables/matrices (问题3).
      const density: number = display.getDefaultDisplaySync().densityPixels;
      const topVp: number = this.topChromeVp();
      const top: number = Math.round(this.getUIContext().vp2px(topVp));
      // Bottom inset in physical px.  Preferred: the MEASURED BottomToolBar
      // height (bottomToolbarHeightVp, captured by the toolbar's
      // onAreaChange) — the toolbar docks at the WINDOW bottom edge, so the
      // Qt viewport must end exactly at its top border.  (2in1/free-window
      // fix 2026-08-12: the previous formula derived bottom from the SCREEN
      // height minus the toolbar's global y, which is only valid while the
      // window fills the screen.  On a 2in1 free window the window bottom
      // sits above the screen bottom, so that derivation measured the
      // screen area BELOW the window as chrome (device: bottom=716px vs
      // ~62px real) and pushed the Qt viewport far too high, exposing the
      // QMainWindow white background as a large blank band.)
      let bottom: number = 0;
      let bottomVp: number = 0;
      if (this.bottomToolbarHeightVp > 0) {
        bottomVp = this.bottomToolbarHeightVp;
        bottom = Math.round(this.getUIContext().vp2px(bottomVp));
      } else {
        bottomVp = STATUSBAR_HEIGHT_VP + this.bottomToolbarHeightVp;
        bottom = Math.round(this.getUIContext().vp2px(bottomVp));
      }
      const raw: string = qtInjector.callQtCommand('setChromeInsets',
        JSON.stringify({ top: top, bottom: bottom } as ChromeInsetsArgs));
      const resp: CmdResult = JSON.parse(raw) as CmdResult;
      hilog.info(DOMAIN, TAG,
        'setChromeInsets density=%{public}s topVp=%{public}s topPx=%{public}d bottomVp=%{public}s bottomPx=%{public}d -> %{public}s',
        String(density), String(topVp), top, String(bottomVp), bottom, raw);
      // Always push the menu limit too; -1 disables the override when the
      // bottom toolbar is absent.
      this.updateMenuBottomLimit();
      return resp.success;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'setChromeInsets failed: %{public}s', (e as Error).message);
      return false;
    }
  }

  // 2026-08-17: push the ArkTS-measured bottom-toolbar top to Qt for
  // inline menu clamping.  The Qt content bottom (setChromeInsets) almost
  // matches it, but a few px of Qt/ArkTS alignment error can still leave
  // the last menu item half hidden; the measured overlay top is the
  // authoritative "no-obstruction" line the user asked for.
  private updateMenuBottomLimit(): void {
    try {
      // bottomBarTopY<=0 means the bottom toolbar is hidden; send -1 so Qt
      // falls back to its own Qt-content-bottom clamp instead of keeping a
      // stale toolbar top from a previously visible toolbar.
      const globalY: number = this.bottomBarTopY > 0
        ? Math.round(this.getUIContext().vp2px(this.bottomBarTopY))
        : -1;
      const raw: string = qtInjector.callQtCommand('setMenuBottomLimit',
        JSON.stringify({ globalY: globalY } as MenuBottomLimitArgs));
      hilog.info(DOMAIN, TAG,
        'setMenuBottomLimit bottomBarTopY=%{public}s px=%{public}d -> %{public}s',
        String(this.bottomBarTopY), globalY, raw);
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'setMenuBottomLimit failed: %{public}s',
        (e as Error).message);
    }
  }

  // Startup path: Qt's event loop may not be running when the overlay
  // first appears, so retry with a short backoff until the push succeeds
  // (later height changes go through onAreaChange directly).  A queued
  // ack only proves the QApplication exists, so the follow-up uiState
  // fetch below still needs its own retry loop.
  private schedulePushChromeInsets(): void {
    if (this.pushChromeInsets()) {
      this.scheduleStartupUiRefresh();
      return;
    }
    if (this.insetsRetry < 20) {
      this.insetsRetry++;
      setTimeout(() => this.schedulePushChromeInsets(), 500);
    }
  }

  // Startup uiState fetch: getUiState can time out (300ms fallback, P0
  // 2026-08-12: was 3s) while the Qt event loop is still starting, leaving
  // the toolbar context groups hidden although the default Table exists.
  // Poll until an active window shows up (device-verified: first fetch
  // after install returned an empty state).
  private scheduleStartupUiRefresh(): void {
    // P0 (2026-08-12): gate the first getUiState on the qtReady event
    // (QtEventHost bumps AppStorage 'qtReadyTick' right before app.exec()).
    // Querying while the Qt GUI thread is still initialising blocked the
    // ArkTS main thread for the full 3s semaphore fallback per attempt —
    // the startup retries froze input >8s and the device killed the app
    // (APP_INPUT_BLOCK; uv_timer_task stuck — device log).
    if ((AppStorage.get<number>('qtReadyTick') ?? 0) <= 0) {
      if (this.qtReadyWaitRetry < 60) { // ≤60s wait for the qtReady signal
        this.qtReadyWaitRetry++;
        setTimeout(() => this.scheduleStartupUiRefresh(), 1000);
        return;
      }
      // Budget exhausted: the signal never arrived within 60s.  Distinguish
      // signal loss (warn repeats on every fallback poll) from a slow
      // device (warn stops once qtReadyTick > 0).  Polling proceeds below
      // as the legacy fallback.
      hilog.warn(DOMAIN, TAG, 'qtReady fallback after 60s: polling anyway');
    }
    this.refreshUiState();
    if (this.uiState.activeName === '' && this.uiRefreshRetry < 10) {
      this.uiRefreshRetry++;
      // P0 (2026-08-12): ≥1.5s outer poll interval — each attempt can still
      // cost up to the 300ms Qt-side query timeout, so polling faster just
      // adds main-thread freezes without making Qt idle sooner.
      setTimeout(() => this.scheduleStartupUiRefresh(), 1500);
    }
  }

  private refreshUiState(): void {
    // P0 (2026-08-12): skip while a Qt modal loop is open (QtEventHost
    // flips AppStorage 'modalActive' on modalStarted/modalEnded).  A
    // synchronous getUiState issued then blocks the ArkTS main thread for
    // the full 300ms semaphore fallback per attempt — repeated attempts
    // froze input >8s and the device killed the app (APP_INPUT_BLOCK).
    // All polling/refresh paths (startup poll, timeout retry, menu-open,
    // windowActivated) funnel through here, so this single gate pauses
    // every synchronous query while the modal is up.
    if (AppStorage.get<boolean>('modalActive') === true) {
      hilog.info(DOMAIN, TAG, 'getUiState skipped (modal active)');
      return;
    }
    let ok: boolean = false;
    try {
      const raw: string = qtInjector.callQtCommand('getUiState', '{}');
      const resp: UiStateResponse = JSON.parse(raw) as UiStateResponse;
      if (resp.success && resp.data !== undefined) {
        ok = true;
        this.uiState = resp.data;
        // Mirror Qt dock visibility into ArkTS state for menu checkmarks
        // (explorer + log both come from the Qt dock widgets now; the
        // ArkTS Results Log panel was removed 2026-08-14).
        this.dockExplorerVisible = resp.explorerVisible === true;
        this.dockLogVisible = resp.logVisible === true;
        // Qt first window is rendered once activeName is non-empty: hide
        // the startup splash overlay (only set true, never reset).
        if (resp.data.activeName !== '') {
          this.qtReady = true;
        }
        // P3 (2026-08-16): the bottom toolbar is ALWAYS rendered (Win
        // customToolBars keeps plot/table/3D toolbars visible, greyed by
        // project content) — the inset must keep the measured bar height
        // regardless of the active window type.  The old branch zeroed the
        // inset when a Graph/Matrix was active, so the always-rendered bar
        // OVERLAID the MDI content's bottom band (content hidden under it).
        // onAreaChange (in build()) re-measures the bar whenever its height
        // changes, so no zeroing is ever needed here.
        // Greying debug trail: activeType drives isTable/isGraph on the
        // toolbar; log it so device runs can prove what ETS received.
        hilog.info(DOMAIN, TAG, 'uiState: type=%{public}s name=%{public}s undo=%{public}s',
          resp.data.activeType, resp.data.activeName, String(resp.data.undoEnabled));
      }
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'getUiState failed: %{public}s', (e as Error).message);
    }
    // P0 (2026-08-12): a timed-out getUiState is logged and silently
    // swallowed — NO inner retry.  Retrying every 400ms re-froze the ArkTS
    // main thread ~300ms per attempt while Qt was busy, which fed the
    // startup ANR loop (plan 0245 §2.4 R2).  The next
    // windowActivated/menu-open event re-queries anyway.
    if (!ok) {
      hilog.info(DOMAIN, TAG, 'getUiState timeout — silently skipped');
    }
    this.refreshRecentProjects();
    this.rebuildMenus();
  }

  // Stage-2A (2026-08-16): refresh the File→Recent Projects submenu from the
  // Qt-side recentProjects list (persisted by libscidavis via QSettings
  // "/RecentProjects"). Entries that are no longer accessible from this
  // process (work-folder grant expired after restart, file deleted) are
  // dropped so the submenu never offers a dead open. Called on every
  // refreshUiState — the caller rebuilds the menu right after, so setting
  // @State recentProjects here is enough.
  private refreshRecentProjects(): void {
    try {
      const raw: string = qtInjector.callQtCommand('getRecentProjects', '{}');
      const resp: RecentProjectsResponse = JSON.parse(raw) as RecentProjectsResponse;
      if (resp.success && Array.isArray(resp.projects)) {
        const kept: string[] = [];
        for (const p of resp.projects) {
          try {
            fs.accessSync(p);
            kept.push(p);
          } catch (e) {
            hilog.info(DOMAIN, TAG, 'recentProjects drop (inaccessible): %{public}s', p);
          }
        }
        const changed: boolean = kept.length !== this.recentProjects.length ||
          kept.some((v: string, i: number) => v !== this.recentProjects[i]);
        if (changed) {
          this.recentProjects = kept;
          hilog.info(DOMAIN, TAG, 'recentProjects refreshed: %{public}d entries', kept.length);
        }
      }
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'getRecentProjects failed: %{public}s', (e as Error).message);
    }
  }

  // Menu model is rebuilt from getUiState on every menu-open (desktop
  // parity: greyed = not yet wired on tablet / not applicable; the fourth
  // menu switches Table/Matrix/Graph with the active MDI window type).
  @State menuDefs: MenuItemData[] = [];
  // Stage-2A: File→Recent Projects dynamic submenu entries (paths, most
  // recent first). Refreshed from C++ getRecentProjects on every
  // refreshUiState; entries no longer accessible are dropped.
  @State recentProjects: string[] = [];
  @State uiState: UiState = {
    activeType: '', activeName: '', windows: [],
    undoEnabled: false, redoEnabled: false
  };

  private rebuildMenus(): void {
    const st: UiState = this.uiState;
    const hasWin: boolean = st.activeName !== '';
    const isTable: boolean = st.activeType === 'Table';
    const isMatrix: boolean = st.activeType === 'Matrix';
    const isMultiLayer: boolean = st.activeType === 'MultiLayer';
    const isGraph3D: boolean = st.activeType === 'Graph3D';
    const isPlotWin: boolean = isMultiLayer || isGraph3D;
    // P3 (2026-08-16): Note 激活时 Scripting 菜单追加执行项（Win L1235）。
    const isNote: boolean = st.activeType === 'Note';
    const defs: MenuItemData[] = [];
    const SEPARATOR: string = '\u2500';

    // Stage-2A (2026-08-16): Recent Projects dynamic submenu mirroring the
    // Qt-side recentProjects list (Win: "&N <full path>", most recent first,
    // max 10 via MaxRecentProjects). Entries dispatch 'recent:<path>'.
    const recentItems: SubMenuItemData[] = [];
    for (let i = 0; i < this.recentProjects.length && i < 10; i++) {
      const rp: string = this.recentProjects[i];
      const slash: number = rp.lastIndexOf('/');
      const base: string = slash >= 0 ? rp.substring(slash + 1) : rp;
      const dir: string = slash >= 0 ? rp.substring(0, slash) : '';
      // id is built through a variable (NOT 'id: \'recent:\' + rp') so the
      // verify_menu_layers id regex cannot mis-parse the concatenation.
      const rid: string = 'recent:' + rp;
      recentItems.push({ id: rid, label: (i + 1) + '  ' + base + '   ' + dir });
    }
    // Stage-2A: Recent Projects entry — submenu when the list is non-empty,
    // disabled leaf otherwise (parent menu objects cannot carry an enabled
    // field).  Built imperatively: ArkTS forbids conditional object-literal
    // expressions (arkts-no-untyped-obj-literals).
    const recentMenuEntry: SubMenuItemData = { id: 'recent_projects', label: getAppString('file_recent_projects') };
    if (this.recentProjects.length > 0) {
      recentMenuEntry.children = recentItems;
    } else {
      recentMenuEntry.enabled = false;
    }

    // File menu: aligned with Qt initMainMenu (ApplicationWindow.cpp L780-828).
    // Order/separators per Qt: New▸ / Open / Recent Projects / ─ / Open Image
    // File / Import Image / ─ / Save / Save As / ─ / Open Template / Save As
    // Template / ─ / Export Graph▸ / Print / Print All Plots / ─ / Export
    // ASCII / Import ASCII / ─ / Quit (6 separators; NO separator between
    // New/Open/Recent Projects). Icons per Qt source: open/save/open_template/
    // save_as_template/print/import/quit and the New submenu children carry
    // icons; open_image_file/import_image/save_as/export_graph(parent)/
    // print_all_plots/export(ascii) have none.
    defs.push({ id: 'file', label: getAppString('menu_file'), items: [
      // New submenu (cascade): parent itself has NO icon; children DO (Qt)
      { id: 'new', label: getAppString('file_new'), enabled: true,
        children: [
          { id: 'new_project', label: getAppString('file_new_project'), icon: $r('app.media.tb_new'), shortcut: 'Ctrl+N' },
          { id: 'new_table', label: getAppString('file_new_table'), icon: $r('app.media.tb_table'), shortcut: 'Ctrl+T' },
          { id: 'new_matrix', label: getAppString('file_new_matrix'), icon: $r('app.media.tb_new_matrix'), shortcut: 'Ctrl+M' },
          { id: 'new_notes', label: getAppString('file_new_note'), icon: $r('app.media.tb_new_note'), shortcut: 'Ctrl+Alt+N' },
          { id: 'new_graph', label: getAppString('file_new_graph'), icon: $r('app.media.tb_new_graph'), shortcut: 'Ctrl+G' },
          { id: 'new_function_plot', label: getAppString('file_new_function_plot'), icon: $r('app.media.mn_newF'), shortcut: 'Ctrl+F', enabled: true },
          { id: 'new_3d_surface', label: getAppString('file_new_3d_surface'), icon: $r('app.media.mn_newFxy'), shortcut: 'Ctrl+Alt+Z', enabled: false }
        ]
      },
      { id: 'open', label: getAppString('file_open_project'), icon: $r('app.media.tb_fileopen'), shortcut: 'Ctrl+O' },
      // Recent Projects: dynamic submenu / disabled leaf (see recentMenuEntry
      // above).
      recentMenuEntry,
      { id: 'sep1', label: SEPARATOR },
      // 2026-08-14 (route 5): Qt 原生内嵌 getOpenFileName（loadImage/importImage）。
      { id: 'open_image_file', label: getAppString('file_open_image_file'), shortcut: 'Ctrl+I', enabled: true },
      { id: 'import_image', label: getAppString('file_import_image'), enabled: true },
      { id: 'sep2', label: SEPARATOR },
      { id: 'save', label: getAppString('file_save_project'), icon: $r('app.media.tb_filesave'), shortcut: 'Ctrl+S' },
      { id: 'save_as', label: getAppString('file_save_as') },
      { id: 'sep3', label: SEPARATOR },
      { id: 'open_template', label: getAppString('file_open_template'), icon: $r('app.media.tb_open_template'), enabled: true },
      { id: 'save_as_template', label: getAppString('file_save_as_template'), icon: $r('app.media.tb_save_template'), enabled: true },
      { id: 'sep4', label: SEPARATOR },
      { id: 'export_graph', label: getAppString('file_export_graph'),
        children: [
          // 2026-08-14: 文件夹级授权直通（路线5 探针已验证 Qt 直读写沙箱外）。
          // exportGraphImage 直接写授权工作文件夹，不再走 picker+拷贝。
          { id: 'export_graph_all', label: getAppString('file_export_graph_all'), enabled: true },
          { id: 'export_graph_current', label: getAppString('file_export_graph_current'), enabled: true }
        ]
      },
      { id: 'print', label: getAppString('file_print'), icon: $r('app.media.tb_fileprint'), shortcut: 'Ctrl+P', enabled: false },
      { id: 'print_all_plots', label: getAppString('file_print_all_plots'), enabled: false },
      { id: 'sep5', label: SEPARATOR },
      { id: 'export', label: getAppString('file_export_ascii'), enabled: true },
      // 2026-08-14: 文件夹级授权直通（ImportDialog 改造为从授权工作文件夹
      // 选文件直通 importASCII，不再 DocumentViewPicker+拷入沙箱）。
      { id: 'import', label: getAppString('file_import_ascii'), icon: $r('app.media.tb_import'), shortcut: 'Ctrl+K', enabled: true },
      { id: 'sep6', label: SEPARATOR },
      { id: 'quit', label: getAppString('file_quit'), icon: $r('app.media.mn_quit'), shortcut: 'Ctrl+Q' }
    ]});

    // Edit menu: per Qt initMainMenu (ApplicationWindow.cpp L830-850) — 3
    // separators: after Redo, after Delete Selection, after Clear Log Info.
    // Order: Undo / Redo / ─ / Cut / Copy / Paste / Delete Selection / ─ /
    // Delete Fit Tables / Clear Log Info / ─ / Preferences. Icons per Qt:
    // undo/edit-undo, redo/edit-redo, cut/edit-cut, copy/edit-copy,
    // paste/edit-paste, delete_selection/erase.xpm, delete_fit_tables/close.xpm;
    // clear_log_info and preferences have none. Paste keeps the PasteRow
    // security-component render in MenuBar (icon field unused there).
    defs.push({ id: 'edit', label: getAppString('menu_edit'), items: [
      { id: 'undo', label: getAppString('edit_undo'), icon: $r('app.media.tb_edit_undo'), shortcut: 'Ctrl+Z', enabled: st.undoEnabled },
      { id: 'redo', label: getAppString('edit_redo'), icon: $r('app.media.tb_edit_redo'), shortcut: 'Ctrl+R', enabled: st.redoEnabled },
      { id: 'sep1', label: SEPARATOR },
      { id: 'cut', label: getAppString('edit_cut_selection'), icon: $r('app.media.tb_edit_cut'), shortcut: 'Ctrl+X', enabled: hasWin },
      { id: 'copy', label: getAppString('edit_copy_selection'), icon: $r('app.media.tb_edit_copy'), shortcut: 'Ctrl+C', enabled: hasWin },
      { id: 'paste', label: getAppString('edit_paste_selection'), icon: $r('app.media.tb_edit_paste'), shortcut: 'Ctrl+V', enabled: hasWin },
      { id: 'delete', label: getAppString('edit_delete_selection'), icon: $r('app.media.tb_erase'), shortcut: 'Del', enabled: hasWin },
      { id: 'sep2', label: SEPARATOR },
      // D3 (2026-08-14): 已接线 menuAction（C++ 分支落地）。
      { id: 'delete_fit_tables', label: getAppString('edit_delete_fit_tables'), enabled: true },
      { id: 'clear_log_info', label: getAppString('edit_clear_log_info'), enabled: true },
      { id: 'sep3', label: SEPARATOR },
      { id: 'preferences', label: getAppString('edit_preferences') }
    ]});

    // View menu: toolbars/lock_toolbars disabled, plot_wizard first, then
    // project_browser/results_log/undo_redo_history（enabled，menuAction 直连
    // C++ 内嵌 QUndoView；results_log 为 ArkTS 本地面板，见 dispatch
    // case 'results_log'）/console（dock 形态受 QDockWidget 隐藏限制，
    // 阶段7 搁置 — 保持 disabled）.
    // Per Win view.png: Toolbars (no icon) / Lock Toolbars (lock icon) / ─ /
    // Plot Wizard (Ctrl+Alt+W, wand) / Project Explorer (Ctrl+E, folder) /
    // Results Log (icon) / Undo/Redo History / Console. Lock Toolbars uses the
    // closest available asset (no dedicated lock icon in base/media).
    defs.push({ id: 'view', label: getAppString('menu_view'), items: [
      { id: 'toolbars', label: getAppString('view_toolbars'), enabled: false },
      { id: 'lock_toolbars', label: getAppString('view_lock_toolbars'), icon: $r('app.media.tb_cursor_16'), enabled: false },
      { id: 'sep1', label: SEPARATOR },
      { id: 'plot_wizard', label: getAppString('view_plot_wizard'), icon: $r('app.media.mn_wizard'), shortcut: 'Ctrl+Alt+W' },
      { id: 'project_browser', label: getAppString('panel_project_explorer'), icon: $r('app.media.tb_folder'), shortcut: 'Ctrl+E', checked: this.dockExplorerVisible },
      { id: 'results_log', label: getAppString('view_results_log'), icon: $r('app.media.tb_log'), checked: this.dockLogVisible },
      { id: 'undo_redo_history', label: getAppString('view_undo_redo_history'), enabled: true },
      // console: dock 形态受 QDockWidget 隐藏限制，阶段7 搁置 — 保持 disabled
      { id: 'console', label: getAppString('view_console'), enabled: false }
    ]});

    // Scripting menu: scripting_language（enabled，Qt-native embedded）/
    // restart_scripting（enabled）。P3 (2026-08-16): Note 激活时按 Win
    // ApplicationWindow.cpp L1235-1245 追加 分隔线 + Execute (Ctrl+J) /
    // Execute All (Ctrl+Shift+J) / Evaluate Expression (Ctrl+Return)；
    // 三项 dispatch 到 menuAction（C++ 分支调用 Note::execute/executeAll/
    // evaluate）。
    const scriptingItems: SubMenuItemData[] = [
      { id: 'scripting_language', label: getAppString('scripting_language'), enabled: true },
      { id: 'restart_scripting', label: getAppString('restart_scripting'), enabled: true }
    ];
    if (isNote) {
      scriptingItems.push({ id: 'sep_note', label: SEPARATOR });
      scriptingItems.push({ id: 'note_execute', label: getAppString('note_execute'), shortcut: 'Ctrl+J', enabled: true });
      scriptingItems.push({ id: 'note_execute_all', label: getAppString('note_execute_all'), shortcut: 'Ctrl+Shift+J', enabled: true });
      scriptingItems.push({ id: 'note_evaluate', label: getAppString('note_evaluate'), shortcut: 'Ctrl+Return', enabled: true });
    }
    defs.push({ id: 'scripting', label: getAppString('menu_scripting'), items: scriptingItems });

    // Context menus follow the active MDI window type.
    // Matrix menu: per Qt Matrix::fillProjectMenu (future_Matrix.cpp L814-848)
    // + customMenu tail (ApplicationWindow.cpp L1200-1207). Order: Hide
    // Controls / ─ / Set Coordinates / Dimensions / Set Display Format / ─ /
    // Assign Formula / Recalculate / ─ / Clear Matrix / Transpose / Mirror
    // Horizontally / Mirror Vertically / ─ / Import Image / ─ / Go to Cell /
    // ─ / Invert / Determinant / ─ / Convert to Table (7 separators).
    if (isMatrix) {
      // Phase-1 (2026-08-15): Matrix 模式补顶级 "3D Plot" 菜单，结构对齐 Win
      // （plot3DMenu，9 项：WireFrame/HiddenLine/Polygons/WireSurface /
      // Bars/Scatter / ColorMap/ContourMap/GrayMap）。3D 后端 stub，置灰。
      defs.push({ id: 'plot_3d', label: getAppString('plot_3d'),
        items: [
          { id: 'plot_3d_wire', label: getAppString('plot_3d_wire'), enabled: false },
          { id: 'plot_3d_hidden', label: getAppString('plot_3d_hidden'), enabled: false },
          { id: 'plot_3d_polygons', label: getAppString('plot_3d_polygons'), enabled: false },
          { id: 'plot_3d_wire_surface', label: getAppString('plot_3d_wire_surface'), enabled: false },
          { id: 'sep_a', label: SEPARATOR },
          { id: 'plot_3d_matrix_bars', label: getAppString('plot_3d_bars'), enabled: false },
          { id: 'plot_3d_matrix_scatter', label: getAppString('plot_3d_scatter'), enabled: false },
          { id: 'sep_b', label: SEPARATOR },
          { id: 'color_map_matrix', label: getAppString('color_map'), enabled: false },
          { id: 'contour_map_matrix', label: getAppString('contour_map'), enabled: false },
          { id: 'gray_map_matrix', label: getAppString('gray_map'), enabled: false }
        ]
      });
      defs.push({ id: 'matrix', label: getAppString('menu_matrix'), items: [
        // D3 (2026-08-14): 已接线 menuAction（C++ 分支落地）。
        { id: 'matrix_hide_controls', label: getAppString('matrix_hide_controls'), shortcut: 'F12', enabled: true },
        { id: 'sep1', label: SEPARATOR },
        { id: 'matrix_set_coordinates', label: getAppString('matrix_set_coordinates'), enabled: true },
        { id: 'matrix_dimensions', label: getAppString('matrix_set_dimensions'), enabled: true },
        { id: 'matrix_set_display_format', label: getAppString('matrix_set_display_format'), enabled: true },
        { id: 'sep2', label: SEPARATOR },
        { id: 'matrix_assign_formula', label: getAppString('matrix_set_values'), icon: $r('app.media.mn_fx'), shortcut: 'Alt+Q' },
        // D2 (2026-08-14): matrix_recalculate/clear/mirror_v/
        // determinant/convert_to_table 已接线 menuAction（C++ 分支落地）。
        { id: 'matrix_recalculate', label: getAppString('matrix_recalculate'), icon: $r('app.media.mn_recalculate'), shortcut: 'Ctrl+Return', enabled: true },
        { id: 'sep3', label: SEPARATOR },
        { id: 'matrix_clear', label: getAppString('matrix_clear'), icon: $r('app.media.mn_clear'), enabled: true },
        { id: 'matrix_transpose', label: getAppString('matrix_transpose') },
        // D3 (2026-08-14): matrix_mirror_h 已接线 menuAction（C++ 分支落地）。
        { id: 'matrix_mirror_h', label: getAppString('matrix_mirror_h'), enabled: true },
        { id: 'matrix_mirror_v', label: getAppString('matrix_mirror_v'), enabled: true },
        { id: 'sep4', label: SEPARATOR },
        { id: 'matrix_import_image', label: getAppString('matrix_import_image'), icon: $r('app.media.mn_image3'), enabled: true },
        { id: 'sep5', label: SEPARATOR },
        { id: 'matrix_go_to_cell', label: getAppString('matrix_go_to_cell'), icon: $r('app.media.mn_go_to_cell'), shortcut: 'Ctrl+Alt+G', enabled: true },
        { id: 'sep6', label: SEPARATOR },
        { id: 'matrix_invert', label: getAppString('matrix_invert') },
        { id: 'matrix_determinant', label: getAppString('matrix_determinant'), enabled: true },
        { id: 'sep7', label: SEPARATOR },
        { id: 'matrix_convert_to_table', label: getAppString('matrix_convert_to_table'), enabled: true }
      ]});
    } else if (isMultiLayer || isGraph3D) {
      // Graph menu (MultiLayer-active): drawing tools, then layout, then layer management
      // Per Win graph.png: icons on add_curve/add_error_bars/add_function/
      // add_text/draw_arrow/draw_line/add_image/new_legend/add_layer/
      // remove_layer/arrange_layers. add_time_stamp and automatic_layout have
      // none. Separators sit between Add Function and Add Text, and between
      // New Legend and Automatic Layout.
      defs.push({ id: 'graph', label: getAppString('menu_graph'), items: [
        // Icons per Win ApplicationWindow.cpp (2026-08-14 aligned):
        // add_curve/curves.xpm, add_error_bars/yerror.xpm,
        // add_function/fx.xpm, add_text/text.xpm, draw_arrow/arrow.xpm,
        // draw_line/lPlot.xpm, add_time_stamp/clock.xpm,
        // add_image/monalisa.xpm, new_legend/legend.xpm,
        // automatic_layout/auto_layout.xpm, add_layer/newLayer.xpm,
        // remove_layer/erase.xpm, arrange_layers/arrangeLayers.xpm.
        { id: 'add_curve', label: getAppString('graph_add_remove_curve'), icon: $r('app.media.tb_curves'), shortcut: 'Alt+C', enabled: isMultiLayer },
        { id: 'add_error_bars', label: getAppString('graph_add_error_bars'), icon: $r('app.media.mn_yerror'), shortcut: 'Ctrl+B', enabled: isMultiLayer },
        { id: 'add_function', label: getAppString('graph_add_function'), icon: $r('app.media.mn_fx'), shortcut: 'Ctrl+Alt+F', enabled: isMultiLayer },
        { id: 'sep1', label: SEPARATOR },
        { id: 'add_text', label: getAppString('graph_add_text'), icon: $r('app.media.tb_text'), shortcut: 'Alt+T', enabled: isMultiLayer },
        { id: 'draw_arrow', label: getAppString('graph_draw_arrow'), icon: $r('app.media.mn_arrow'), shortcut: 'Ctrl+Alt+A', enabled: isMultiLayer },
        { id: 'draw_line', label: getAppString('graph_draw_line'), icon: $r('app.media.tb_lplot'), shortcut: 'Ctrl+Alt+L', enabled: isMultiLayer },
        // D3 (2026-08-14): 已接线（timeStamp 命令 / menuAction）。
        { id: 'add_time_stamp', label: getAppString('graph_add_time_stamp'), icon: $r('app.media.mn_clock'), shortcut: 'Ctrl+Alt+T', enabled: isMultiLayer },
        { id: 'add_image', label: getAppString('graph_add_image'), icon: $r('app.media.mn_monalisa'), shortcut: 'Alt+I', enabled: isMultiLayer },
        { id: 'new_legend', label: getAppString('graph_new_legend'), icon: $r('app.media.mn_legend'), shortcut: 'Ctrl+L', enabled: isMultiLayer },
        { id: 'sep2', label: SEPARATOR },
        { id: 'automatic_layout', label: getAppString('graph_automatic_layout'), icon: $r('app.media.mn_auto_layout'), enabled: isMultiLayer },
        { id: 'add_layer', label: getAppString('graph_add_layer'), icon: $r('app.media.mn_layer_enabled'), shortcut: 'Alt+L', enabled: isMultiLayer },
        { id: 'remove_layer', label: getAppString('graph_remove_layer'), icon: $r('app.media.mn_layer_disabled'), shortcut: 'Alt+R', enabled: isMultiLayer },
        { id: 'arrange_layers', label: getAppString('graph_arrange_layers'), icon: $r('app.media.tb_arrangelayers'), shortcut: 'Alt+A', enabled: isMultiLayer }
      ]});

      // Tools menu: MultiLayer-active only
      // Per Qt initPlotDataMenu (ApplicationWindow.cpp L1025-1045): Disable
      // tools / Zoom In / Zoom Out / Rescale to Show All / ─ / Screen Reader /
      // Data Reader / Select Data Range / ─ / Move Data Points / Remove Bad
      // Data Points. NO separator between Disable Tools and Zoom In. Screen
      // Reader and Disable Tools have NO icon; zoom/rescale/data_reader/range
      // have icons; move_points/remove_bad 已置灰禁用（2026-08-13 真机实测）。
      // 根因：OH 编译的 QwtPicker 是 54 行 stub → DataPickerTool 事件处理
      // 无效 + setCursor 引发单窗口 QPA 合成错乱；C++ 分支保留备用。
      // P1-5 (2026-08-14): disable_tools/zoom_in/zoom_out/screen_reader/
      // data_reader/range 一并置灰 — zoom/screen_reader/data_reader 的 C++
      // 独立命令最终都走 QwtPicker 工具链（Graph::zoom/setActiveTool 的
      // setCursor 与 move_points 同根因，真机实测合成错乱）；range 无
      // dispatch case 也无 C++ 命令。仅 rescale_tools 保留（setAutoScale
      // 纯缩放不触工具链，已在用）。
      defs.push({ id: 'tools', label: getAppString('menu_tools'), items: [
        { id: 'disable_tools', label: getAppString('tools_disable_tools'), shortcut: 'Ctrl++', enabled: isPlotWin },
        { id: 'zoom_in', label: getAppString('tools_zoom_in'), icon: $r('app.media.tb_zoom'), shortcut: 'Ctrl+←', enabled: isPlotWin },
        { id: 'zoom_out', label: getAppString('tools_zoom_out'), icon: $r('app.media.tb_zoomout'), shortcut: 'Ctrl+→', enabled: isPlotWin },
        { id: 'rescale_tools', label: getAppString('tools_rescale'), icon: $r('app.media.tb_unzoom'), shortcut: 'Ctrl+Shift+R', enabled: isMultiLayer },
        { id: 'sep1', label: SEPARATOR },
        { id: 'screen_reader', label: getAppString('tools_screen_reader'), enabled: isPlotWin },
        { id: 'data_reader', label: getAppString('tools_data_reader'), icon: $r('app.media.tb_select'), shortcut: 'Ctrl+D', enabled: isPlotWin },
        { id: 'range', label: getAppString('tools_range'), icon: $r('app.media.tb_cursors'), shortcut: 'Alt+S', enabled: isPlotWin },
        { id: 'sep2', label: SEPARATOR },
        // D3 (2026-08-14): QwtPicker 完整版恢复 → 工具链复活，解除置灰。
        { id: 'move_points', label: getAppString('tools_move_points'), shortcut: 'Ctrl+Alt+M', enabled: isPlotWin },
        { id: 'remove_bad', label: getAppString('tools_remove_bad'), shortcut: 'Alt+B', enabled: isPlotWin }
      ]});

      // Format menu: MultiLayer-active only
      defs.push({ id: 'format', label: getAppString('menu_format'), items: [
        // Phase-1 (2026-08-15): 分隔线对齐 Win（Plot | Scales/Axes | Grid/Title）。
        { id: 'format_plot', label: getAppString('format_plot'), enabled: isMultiLayer },
        { id: 'sep1', label: SEPARATOR },
        { id: 'format_scales', label: getAppString('format_scales'), enabled: isMultiLayer },
        { id: 'format_axes', label: getAppString('format_axes'), enabled: isMultiLayer },
        { id: 'sep2', label: SEPARATOR },
        { id: 'format_grid', label: getAppString('format_grid'), enabled: isMultiLayer },
        { id: 'format_title', label: getAppString('format_title'), enabled: isMultiLayer }
      ]});

      // Analysis menu (MultiLayer graph mode): per Win analysis(graph).png
      // only Fit Wizard carries an icon (mn_wizard); Translate/Differentiate/
      // Integrate/Smooth/FFT Filter/Interpolate/FFT/Quick Fit have none.
      // Translate/Smooth/FFT Filter/Quick Fit are cascade parents.
      // Stage 5 (2026-08-13): all leaf ids use the an_ prefix, matching the
      // dispatch cases and the C++ menuAction branches (menu id == case id ==
      // C++ itemId).  The ArkTS AnalysisDialog mimic is gone — leaves are
      // dispatched straight to Qt.  Quick Fit carries the ExpDecay family
      // flat (Win nests them under "Fit Exponential Decay"; flat chosen to
      // keep the menu tree two levels deep — parents never dispatch).
      defs.push({ id: 'analysis', label: getAppString('menu_analysis'), items: [
        // D3 (2026-08-14): 已接线 menuAction（TranslateCurveTool）。
        { id: 'translate', label: getAppString('analysis_translate'), enabled: isPlotWin,
          children: [
            // Phase-1 (2026-08-15): 顺序对齐 Win（TranslateVert → TranslateHor）。
            { id: 'translate_vertical', label: getAppString('analysis_translate_vertical'), enabled: isPlotWin },
            { id: 'translate_horizontal', label: getAppString('analysis_translate_horizontal'), enabled: isPlotWin }
          ]
        },
        { id: 'an_differentiate', label: getAppString('analysis_differentiate'), enabled: isPlotWin },
        { id: 'an_integrate', label: getAppString('analysis_integrate'), enabled: isPlotWin },
        { id: 'smooth', label: getAppString('analysis_smooth'), enabled: isPlotWin,
          children: [
            { id: 'an_smooth_savgol', label: getAppString('dlg_method_savgol'), enabled: isPlotWin },
            { id: 'an_smooth_average', label: getAppString('dlg_method_moving_avg'), enabled: isPlotWin },
            { id: 'an_smooth_fft', label: getAppString('dlg_method_fft'), enabled: isPlotWin }
          ]
        },
        { id: 'fft_filter', label: getAppString('analysis_fft_filter'), enabled: isPlotWin,
          children: [
            { id: 'an_fft_filter_low', label: getAppString('dlg_filter_low_pass'), enabled: isPlotWin },
            { id: 'an_fft_filter_high', label: getAppString('dlg_filter_high_pass'), enabled: isPlotWin },
            { id: 'an_fft_filter_bandpass', label: getAppString('dlg_filter_band_pass'), enabled: isPlotWin },
            { id: 'an_fft_filter_bandblock', label: getAppString('dlg_filter_band_block'), enabled: isPlotWin }
          ]
        },
        { id: 'an_interpolate', label: getAppString('analysis_interpolate'), enabled: isPlotWin },
        { id: 'an_fft', label: getAppString('analysis_fft_dlg'), enabled: isPlotWin },
        { id: 'quick_fit', label: getAppString('analysis_quick_fit'), enabled: isPlotWin,
          children: [
            { id: 'an_fit_linear', label: getAppString('analysis_fit_linear'), enabled: isPlotWin },
            { id: 'an_fit_poly', label: getAppString('analysis_fit_polynomial'), enabled: isPlotWin },
            // Phase-1 (2026-08-15): Quick Fit 结构对齐 Win —— Fit Exponential
            // Decay 恢复三级级联（MenuBar 已支持三级）+ 分隔线。
            { id: 'sep1', label: SEPARATOR },
            { id: 'an_exp_decay', label: getAppString('analysis_fit_exp_decay'), enabled: isPlotWin,
              children: [
                { id: 'an_exp_decay_1', label: getAppString('analysis_fit_exp_decay_1'), enabled: isPlotWin },
                { id: 'an_exp_decay_2', label: getAppString('analysis_fit_exp_decay_2'), enabled: isPlotWin },
                { id: 'an_exp_decay_3', label: getAppString('analysis_fit_exp_decay_3'), enabled: isPlotWin }
              ]
            },
            { id: 'an_exp_growth', label: getAppString('analysis_fit_exp_growth'), enabled: isPlotWin },
            { id: 'an_fit_sigmoidal', label: getAppString('analysis_fit_boltzmann'), enabled: isPlotWin },
            { id: 'an_fit_gauss', label: getAppString('analysis_fit_gaussian'), enabled: isPlotWin },
            { id: 'an_fit_lorentz', label: getAppString('analysis_fit_lorentzian'), enabled: isPlotWin },
            // D3 (2026-08-14): Win Quick Fit → Fit Multi-Peak → Gaussian.../
            // Lorentzian...（三级级联；C++ fitMultiPeakGauss/Lorentz，Qwt
            // 交互层 stub 下选峰交互受限，入口可达）。
            { id: 'fit_multi_peak', label: getAppString('analysis_fit_multi_peak'), enabled: isPlotWin,
              children: [
                { id: 'fit_multi_peak_gauss', label: getAppString('analysis_fit_multi_peak_gauss'), enabled: isPlotWin },
                { id: 'fit_multi_peak_lorentz', label: getAppString('analysis_fit_multi_peak_lorentz'), enabled: isPlotWin }
              ]
            }
          ]
        },
        { id: 'fit_wizard', label: getAppString('fit_wizard'), icon: $r('app.media.mn_wizard'), shortcut: 'Ctrl+Y', enabled: isPlotWin }
      ]});
    }

    // Plot menu: only when Table is active (13 items per Qt initPlotMenu,
    // ApplicationWindow.cpp L1047-1096 — 3 separators). Line/Scatter/Line+Symbol
    // then Special▸, then ─, Vertical/Horizontal Bars/Area/Pie/Vectors XYXY/
    // Vectors XYAM, then ─, Statistical▸/Panel▸, then ─, 3D▸. Line/Scatter/
    // Line+Symbol/Bars/Area/Pie/Vectors carry icons; Special/Statistical/Panel/
    // 3D parent rows have NO icon (children do for Special/Statistical/3D).
    if (isTable) {
      defs.push({ id: 'plot', label: getAppString('menu_plot'), items: [
        { id: 'plot_line', label: getAppString('plot_line'), icon: $r('app.media.tb_lplot'), enabled: true },
        { id: 'plot_scatter', label: getAppString('plot_scatter'), icon: $r('app.media.tb_pplot'), enabled: true },
        { id: 'plot_line_symbol', label: getAppString('plot_line_symbol'), icon: $r('app.media.tb_lpplot'), enabled: true },
        { id: 'plot_special', label: getAppString('plot_special'), enabled: true,
          children: [
            // Icons per Win (dropLines/spline/vert_steps/hor_steps.xpm).
            { id: 'plot_vertical_drop_lines', label: getAppString('plot_vertical_drop_lines'), icon: $r('app.media.mn_dropLines'), enabled: true },
            { id: 'plot_spline', label: getAppString('plot_spline'), icon: $r('app.media.mn_spline'), enabled: true },
            { id: 'plot_vertical_steps', label: getAppString('plot_vertical_steps'), icon: $r('app.media.mn_vert_steps'), enabled: true },
            { id: 'plot_horizontal_steps', label: getAppString('plot_horizontal_steps'), icon: $r('app.media.mn_hor_steps'), enabled: true }
          ]
        },
        { id: 'sep1', label: SEPARATOR },
        { id: 'plot_vertical_bars', label: getAppString('plot_vertical_bars'), icon: $r('app.media.tb_vertbars'), enabled: true },
        { id: 'plot_horizontal_bars', label: getAppString('plot_horizontal_bars'), icon: $r('app.media.mn_hBars'), enabled: true },
        { id: 'plot_area', label: getAppString('plot_area'), icon: $r('app.media.tb_area'), enabled: true },
        { id: 'plot_pie', label: getAppString('plot_pie'), icon: $r('app.media.tb_pie'), enabled: true },
        { id: 'plot_vectors_xyxy', label: getAppString('plot_vectors_xyxy'), icon: $r('app.media.tb_vectxyxy'), enabled: true },
        { id: 'plot_vectors_xyam', label: getAppString('plot_vectors_xyam'), icon: $r('app.media.mn_vectXYAM'), enabled: true },
        { id: 'sep2', label: SEPARATOR },
        { id: 'plot_statistical', label: getAppString('plot_statistical'), enabled: true,
          children: [
            // Phase-1 (2026-08-15): 顺序对齐 Win（Box → Histogram → Stacked）。
            { id: 'plot_box', label: getAppString('plot_box'), icon: $r('app.media.tb_boxplot'), enabled: true },
            { id: 'plot_histogram', label: getAppString('plot_histogram'), icon: $r('app.media.tb_histogram'), enabled: true },
            { id: 'plot_stacked_histogram', label: getAppString('plot_stacked_histogram'), enabled: true }
          ]
        },
        { id: 'plot_panel', label: getAppString('plot_panel'), enabled: true,
          children: [
            { id: 'panel_2v', label: getAppString('plot_panel_v2'), enabled: true },
            { id: 'panel_2h', label: getAppString('plot_panel_h2'), enabled: true },
            // Phase-1 (2026-08-15): Win Plot→Panel 4 Layers / Stacked Layers
            // （plotPanel(2,2)/(1,-1)，C++ 命令已支持）。
            { id: 'panel_4', label: getAppString('plot_panel_4'), enabled: true },
            { id: 'panel_stacked', label: getAppString('plot_panel_stacked'), enabled: true }
          ]
        },
        { id: 'sep3', label: SEPARATOR },
        // Phase-1 (2026-08-15): Table 模式 Plot→3D 子项对齐 Win
        // （initPlotMenu：Ribbon / Bars / Scatter / Trajectory）；3D 后端 stub，
        // 保持置灰。
        { id: 'plot_3d', label: getAppString('plot_3d'), enabled: false,
          children: [
            { id: 'plot_3d_ribbon', label: getAppString('plot_3d_ribbon'), icon: $r('app.media.tb_ribbon'), enabled: false },
            { id: 'plot_3d_bars', label: getAppString('plot_3d_bars'), icon: $r('app.media.tb_bars'), enabled: false },
            { id: 'plot_3d_scatter', label: getAppString('plot_3d_scatter'), icon: $r('app.media.tb_scatter'), enabled: false },
            { id: 'plot_3d_trajectory', label: getAppString('plot_3d_trajectory'), icon: $r('app.media.tb_trajectory'), enabled: false }
          ]
        }
      ]});
    }

    // Analysis menu (Table mode): statistics_columns/statistics_rows moved from Table menu.
    // Per Qt initTableAnalysisMenu (ApplicationWindow.cpp L1098-1118): Statistics on
    // Columns / Statistics on Rows / ─ / FFT / ─ / Correlate / Autocorrelate / ─ /
    // Convolute / Deconvolute / ─ / Fit Wizard (Ctrl+Y, mn_wizard). Icons per Qt:
    // statistics_columns=col_stat.xpm, statistics_rows=stat_rows.xpm; FFT/Correlate/
    // Autocorrelate/Convolute/Deconvolute/Fit Wizard have none.
    if (isTable) {
      // Check if already added (for MultiLayer case) - if not, add Table-mode Analysis
      const existingAnalysis = defs.find(d => d.id === 'analysis');
      if (!existingAnalysis || !isPlotWin) {
        defs.push({ id: 'analysis', label: getAppString('menu_analysis'), items: [
          { id: 'statistics_columns', label: getAppString('table_stats_columns'), icon: $r('app.media.tb_stats_columns'), enabled: true },
          { id: 'statistics_rows', label: getAppString('table_stats_rows'), icon: $r('app.media.tb_stats_rows'), enabled: true },
          { id: 'sep1', label: SEPARATOR },
          { id: 'fft', label: getAppString('analysis_fft_dlg'), enabled: true },
          { id: 'sep2', label: SEPARATOR },
          { id: 'correlate', label: getAppString('analysis_correlate'), enabled: true },
          { id: 'autocorrelate', label: getAppString('analysis_autocorrelate'), enabled: true },
          { id: 'sep3', label: SEPARATOR },
          { id: 'convolute', label: getAppString('analysis_convolute'), enabled: true },
          { id: 'deconvolute', label: getAppString('analysis_deconvolute'), enabled: true },
          { id: 'sep4', label: SEPARATOR },
          // 2026-08-14 错配修复: dispatch 已有 case 'fit_wizard' 直连 Qt 内嵌
          // FitDialog（C++ menuAction 已落地），菜单却置灰 → 解除置灰。
          { id: 'fit_wizard', label: getAppString('fit_wizard'), icon: $r('app.media.mn_wizard'), shortcut: 'Ctrl+Y', enabled: true }
        ]});
      }
    }

    // Table menu: per Qt Table::fillProjectMenu (future_Table.cpp L1068-1118)
    // + ApplicationWindow.cpp L1190-1196 (export/convert tail). Full structure:
    // Set Columns As▸ / ─ / Fill Selection▸ / ─ / Show Comments / Hide
    // Controls / Formula Edit Mode / Edit Column Description / Change Type &
    // Format / ─ / Clear Table / Sort Table / ─ / Assign Formula / Recalculate
    // / ─ / Add Columns / Dimensions / ─ / Go To Cell / ─ / Export ASCII / ─ /
    // Convert to Matrix (8 separators; statistics removed → Analysis).
    // Icons per Qt: change_type_format, clear_table, sort_table, assign_formula,
    // recalculate, add_columns, go_to_cell carry icons; set_columns_as/
    // fill_selection parents, formula_edit_mode, dimensions, export_ascii,
    // convert_to_matrix have none.
    if (isTable) {
      defs.push({ id: 'table', label: getAppString('menu_table'), items: [
        { id: 'set_columns_as', label: getAppString('table_set_columns_as'), enabled: true,
          children: [
            { id: 'set_as_x', label: getAppString('table_set_as_x'), enabled: true },
            { id: 'set_as_y', label: getAppString('table_set_as_y'), enabled: true },
            { id: 'set_as_z', label: getAppString('table_set_as_z'), enabled: true },
            { id: 'set_as_xerr', label: getAppString('table_set_as_xerr'), enabled: true },
            { id: 'set_as_yerr', label: getAppString('table_set_as_yerr'), enabled: true },
            { id: 'set_as_none', label: getAppString('table_set_as_none'), enabled: true }
          ]
        },
        { id: 'sep1', label: SEPARATOR },
        { id: 'fill_selection', label: getAppString('table_fill_selection'), enabled: true,
          children: [
            { id: 'fill_row_numbers', label: getAppString('table_fill_row_numbers'), enabled: true },
            { id: 'fill_random', label: getAppString('table_fill_random'), enabled: true }
          ]
        },
        { id: 'sep2', label: SEPARATOR },
        // 以下 5 项置灰（P1-5，2026-08-14）：C++ menuAction 无对应分支
        // （grep main.cpp），ArkTS 无 dispatch case → 点击只弹"未适配"。
        // hide_controls=F12 / formula_edit_mode / edit_column_description /
        // change_type_format / clear_table 待 C++ 分支落地后恢复。
        // D2 (2026-08-14): show_comments/recalculate/go_to_cell/
        // convert_to_matrix 已接线 menuAction（C++ 分支落地）。
        { id: 'show_comments', label: getAppString('table_show_comments'), enabled: true },
        // D3 (2026-08-14): 剩余 5 项已接线 menuAction（C++ 分支落地）。
        { id: 'hide_controls', label: getAppString('table_hide_controls'), shortcut: 'F12', enabled: true },
        { id: 'formula_edit_mode', label: getAppString('table_formula_edit_mode'), enabled: true },
        { id: 'edit_column_description', label: getAppString('table_edit_column_description'), enabled: true },
        { id: 'change_type_format', label: getAppString('table_change_type_format'), icon: $r('app.media.mn_configTable'), shortcut: 'Ctrl+Alt+O', enabled: true },
        { id: 'sep3', label: SEPARATOR },
        { id: 'clear_table', label: getAppString('table_clear_table'), icon: $r('app.media.tb_erase'), enabled: true },
        { id: 'sort_table', label: getAppString('table_sort_table'), icon: $r('app.media.mn_sort'), enabled: true },
        { id: 'sep4', label: SEPARATOR },
        { id: 'assign_formula', label: getAppString('table_set_values'), icon: $r('app.media.mn_fx'), shortcut: 'Alt+Q', enabled: true },
        { id: 'recalculate', label: getAppString('table_recalculate'), icon: $r('app.media.mn_recalculate'), shortcut: 'Ctrl+Return', enabled: true },
        { id: 'sep5', label: SEPARATOR },
        { id: 'add_columns', label: getAppString('table_add_columns'), icon: $r('app.media.tb_add_column'), enabled: true },
        { id: 'dimensions', label: getAppString('dlg_table_size'), enabled: true },
        { id: 'sep6', label: SEPARATOR },
        { id: 'go_to_cell', label: getAppString('table_go_to_cell'), icon: $r('app.media.mn_go_to_cell'), shortcut: 'Ctrl+Alt+G', enabled: true },
        { id: 'sep7', label: SEPARATOR },
        { id: 'export_ascii', label: getAppString('file_export_ascii'), enabled: true },
        { id: 'sep8', label: SEPARATOR },
        { id: 'convert_to_matrix', label: getAppString('table_convert_to_matrix'), enabled: true }
      ]});
    }

    // Windows menu: cascade/tile / ─ / next/previous / ─ / rename/duplicate/window_geometry/hide_window/close_window / window list
    // Icons per Win (duplicate.xpm / resize.xpm / close.xpm).
    // window_geometry: stage 6 — 直连 menuAction 'window_geometry'（C++
    // 内嵌对话框承接，另一代理添加中；未落地前 menuAction 报 unsupported →
    // 未适配提示）.
    const winItems: SubMenuItemData[] = [
      { id: 'cascade', label: getAppString('win_cascade') },
      // D2 (2026-08-14): tile/next/previous/duplicate/hide_window 已
      // 接线 menuAction（C++ 分支落地，Win L7552-7562 同源）。
      { id: 'tile', label: getAppString('win_tile'), enabled: true },
      { id: 'sep1', label: SEPARATOR },
      { id: 'next', label: getAppString('win_next'), shortcut: 'F5', enabled: true },
      { id: 'previous', label: getAppString('win_previous'), shortcut: 'F6', enabled: true },
      { id: 'sep2', label: SEPARATOR },
      { id: 'rename_window', label: getAppString('win_rename') },
      { id: 'duplicate', label: getAppString('win_duplicate'), icon: $r('app.media.mn_duplicate'), enabled: true },
      { id: 'window_geometry', label: getAppString('win_window_geometry'), icon: $r('app.media.mn_resize'), enabled: true },
      { id: 'hide_window', label: getAppString('win_hide_window'), enabled: true },
      { id: 'close_window', label: getAppString('win_close_window'), icon: $r('app.media.mn_close'), shortcut: 'Ctrl+W', enabled: true }
    ];
    if (st.windows.length > 0) {
      winItems.push({ id: 'sep5', label: SEPARATOR });
      const seenWinNames: string[] = [];
      for (let i = 0; i < st.windows.length; i++) {
        const w: UiWindow = st.windows[i];
        if (seenWinNames.indexOf(w.name) >= 0) {
          continue;
        }
        seenWinNames.push(w.name);
        winItems.push({ id: 'win:' + w.name, label: w.name + ' (' + w.type + ')', checked: w.active });
      }
    }
    defs.push({ id: 'windows', label: getAppString('menu_windows'), items: winItems });

    // Help menu: per Qt initMainMenu (ApplicationWindow.cpp L1000-1020) — 3
    // separators. Order: Help (Ctrl+H) / Choose Help Folder... / ─ / SciDAVis
    // Homepage / Search for Updates / Download Manual / ─ / Visit SciDAVis
    // Forums / Report a Bug / ─ / About SciDAVis (F1). NO icons on any item.
    defs.push({ id: 'help', label: getAppString('menu_help'), items: [
      { id: 'help_browser', label: getAppString('help_browser'), shortcut: 'Ctrl+H', enabled: false },
      { id: 'choose_help_folder', label: getAppString('help_choose_help_folder'), enabled: false },
      { id: 'sep1', label: SEPARATOR },
      // D3 (2026-08-14): 链接类直接跳系统默认浏览器（webview.openBrowser），
      // 不内置浏览器；help_browser/choose_help_folder 非链接类保持禁用。
      { id: 'scidavis_homepage', label: getAppString('help_scidavis_homepage'), enabled: true },
      { id: 'search_updates', label: getAppString('help_search_updates'), enabled: true },
      { id: 'download_manual', label: getAppString('help_download_manual'), enabled: true },
      { id: 'sep2', label: SEPARATOR },
      { id: 'visit_forums', label: getAppString('help_visit_forums'), enabled: true },
      { id: 'report_bug', label: getAppString('help_report_bug'), enabled: true },
      { id: 'sep3', label: SEPARATOR },
      { id: 'about', label: getAppString('help_about_ohplot'), shortcut: 'F1' }
    ]});

    this.menuDefs = defs;
  }
  
  // Unified "not adapted yet" notice (menu + toolbar): an AlertDialog per
  // spec 鈥?silent no-ops and toasts are both forbidden for unadapted items.
  // Help 链接 → 系统默认浏览器（Win ApplicationWindow.cpp URI 常量同源；
  // OHOS SDK 26 无 webview.openBrowser → startAbility ACTION_VIEW_DATA）。
  private openHelpLink(itemId: string): void {
    const url: string = itemId === 'scidavis_homepage'
      ? 'https://scidavis.sourceforge.net'
      : itemId === 'search_updates'
        ? 'https://sourceforge.net/projects/scidavis/files/SciDAVis/'
        : itemId === 'download_manual'
          ? 'https://sourceforge.net/projects/scidavis/files/SciDAVis%20Documentation/0.1/'
          : itemId === 'visit_forums'
            ? 'https://sourceforge.net/forum/?group_id=199120'
            : 'https://sourceforge.net/tracker/?group_id=199120&atid=968214';
    hilog.info(DOMAIN, TAG, 'openHelpLink(%{public}s) -> %{public}s', itemId, url);
    try {
      const ctx = getContext(this) as common.UIAbilityContext;
      ctx.startAbility({
        action: 'ohos.want.action.viewData',
        uri: url
      }).catch((e: Error) => {
        hilog.error(DOMAIN, TAG, 'startAbility failed: %{public}s', e.message);
      });
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'openHelpLink failed: %{public}s', (e as Error).message);
    }
  }

  private showNotAdapted(): void {
    this.getUIContext().showAlertDialog({
      message: $r('app.string.not_adapted'),
      confirm: {
        value: $r('app.string.common_ok'),
        action: () => {}
      }
    });
  }

  // Right-click context menu items, built from the Qt contextMenuRequest
  // payload (widgetClass + optional markerType):
  //   - marker hit (markerType 'text'/'arrow'/'image'): single Properties
  //     entry → dispatch 'marker_properties' splits by markerType.
  //   - graph area ('MultiLayer'/'Graph3D', no marker): curve menu —
  //     plot_details/edit_range/associations direct-connect to the C++
  //     menuAction branches（另一代理添加中）；hide_curve/remove_curve 尚无
  //     C++ 分支，落到 dispatch default 的未适配提示（TODO: 分支落地后直连）.
  //   - blank workspace ('Blank'/'MdiArea' or unknown ''): the two dock
  //     toggles (existing behaviour).  'TableView'/'MatrixView'/'Note' also
  //     land here — window menus are not built yet (TODO).
  // markerType 由 C++ InputProbe 补充中；未到位时该字段恒空，marker 分支
  // 不可达，一切右键按 widgetClass 处理（曲线区 → 曲线菜单）。
  private buildContextMenuItems(): SubMenuItemData[] {
    const marker: string = this.contextMenuMarkerType;
    if (marker === 'text' || marker === 'arrow' || marker === 'image') {
      return [
        { id: 'marker_properties', label: getAppString('graph_properties') }
      ];
    }
    const cls: string = this.contextMenuWidgetClass;
    if (cls === 'Graph' || cls === 'MultiLayer' || cls === 'Graph3D') {
      return [
        { id: 'plot_details', label: getAppString('ctx_plot_details') },
        { id: 'edit_range', label: getAppString('ctx_edit_range') },
        { id: 'associations', label: getAppString('graph_associations') },
        { id: 'sep1', label: '\u2500' },
        { id: 'hide_curve', label: getAppString('ctx_hide_curve') },
        { id: 'remove_curve', label: getAppString('ctx_remove_curve') }
      ];
    }
    return [
      { id: 'project_browser', label: getAppString('panel_project_explorer'),
        icon: $r('app.media.tb_folder'), shortcut: 'Ctrl+E',
        checked: this.dockExplorerVisible },
      { id: 'results_log', label: getAppString('view_results_log'),
        icon: $r('app.media.tb_log'), checked: this.dockLogVisible }
    ];
  }

  // Qt right-click (QEvent::ContextMenu, forwarded by the C++ InputProbe).
  // Issue list #9 (2026-08-14): the ArkTS right-click menu must ONLY work
  // inside the top/bottom toolbar ArkTS bands — a right-click anywhere in
  // the Qt-rendered surface must do nothing.  The C++ InputProbe still
  // reports the widget class (the native QMenu popup is unsupported by the
  // single-window QPA), but the ArkTS menu is no longer woken from here:
  // the request is logged and swallowed.  Toolbar right-clicks bypass this
  // path (ToolBar.onContextMenu pops the dock-toggle menu directly).
  private handleContextMenuRequest(widgetClass: string, markerType?: string): void {
    hilog.info(DOMAIN, TAG,
      'context menu request swallowed (Qt surface right-click, issue #9): widgetClass=%{public}s markerType=%{public}s',
      widgetClass, markerType ?? '');
  }

  // menuAction direct-connect helper: every callQtCommand must be wrapped in
  // try-catch (NAPI spec §3.3).  menuAction is a QUEUED mutation — the C++
  // "unsupported" answer can only ever come back as success:true (the error
  // dies inside Qt), so a missing C++ branch would silently no-op.
  // P1-6 (2026-08-14): gate locally on MENU_ACTION_SUPPORTed FIRST — only
  // whitelisted ids reach Qt; anything else returns false so the caller's
  // fallback (ArkTS dialog / showNotAdapted) actually runs instead of being
  // dead code behind an always-true reply.
  private menuActionOrFallback(itemId: string): boolean {
    if (MENU_ACTION_SUPPORTED.indexOf(itemId) < 0) {
      hilog.warn(DOMAIN, TAG, 'menuActionOrFallback: %{public}s not whitelisted — falling back', itemId);
      return false;
    }
    try {
      const result: string = qtInjector.callQtCommand('menuAction',
        JSON.stringify({ itemId: itemId } as MenuActionArgs));
      hilog.info(DOMAIN, TAG, 'menuAction(%{public}s): %{public}s', itemId, result);
      const resp: CmdResult = JSON.parse(result) as CmdResult;
      return resp.success !== false;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'menuAction(%{public}s) failed: %{public}s',
        itemId, (e as Error).message);
      return false;
    }
  }

  // Win parity (2026-08-15): exit with unsaved changes asks first.  Save
  // routes through the queued saveProject command and terminates only after
  // the projectSaved event (QtEventHost) — terminating immediately would
  // kill the process before the queued write flushed.
  private confirmExit(): void {
    let modified: boolean = false;
    try {
      const raw: string = qtInjector.callQtCommand('isProjectModified', '{}');
      const resp = JSON.parse(raw) as IsModifiedResponse;
      modified = resp.success === true && resp.modified === true;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'isProjectModified failed: %{public}s', (e as Error).message);
    }
    if (!modified) {
      this.doQuit();
      return;
    }
    this.getUIContext().showAlertDialog({
      title: $r('app.string.file_quit'),
      message: $r('app.string.msg_save_project_changes'),
      primaryButton: {
        value: $r('app.string.common_save_yes'),
        action: () => {
          AppStorage.setOrCreate<boolean>('pendingQuitAfterSave', true);
          // 2026-08-17: 与工具栏 File→Save 保持一致——全新项目没有现成路径
          // 时弹 Qt 原生保存对话框，而不是直接写入 untitled 并退出。
          if (this.projectPath === '') {
            this.openEmbeddedFileDialog('save_as');
          } else {
            this.saveProjectToSandbox();
          }
        }
      },
      secondaryButton: {
        value: $r('app.string.common_discard'),
        fontColor: Color.Red,
        action: () => this.doQuit()
      }
      // back / outside tap = cancel (no action)
    });
  }

  // 2026-08-17: Qt New Project / Quit 的 QMessageBox 点 Yes 后由事件主机
  // 回调到这里。走与工具栏 Save 完全相同的 openEmbeddedFileDialog：先复核/
  // 申请授权工作文件夹，再打开 Qt 保存对话框；保存成功后由 projectSaved
  // 事件继续 quit 或 new_project_after_save。
  private handleQtRequestSaveBeforeQuit(): void {
    if (this.projectPath === '') {
      this.openEmbeddedFileDialog('save_as');
    } else {
      this.saveProjectToSandbox();
    }
  }

  private handleQtRequestSaveBeforeNewProject(): void {
    if (this.projectPath === '') {
      this.openEmbeddedFileDialog('save_as');
    } else {
      this.saveProjectToSandbox();
    }
  }

  private handleQtAfterSaveNewProject(): void {
    this.projectPath = '';
    qtInjector.callQtCommand('menuAction',
      JSON.stringify({ itemId: 'new_project_after_save' } as MenuActionArgs));
    this.refreshUiState();
  }

  private doQuit(): void {
    hilog.info(DOMAIN, TAG, 'Quit requested - terminating ability');
    (getContext(this) as common.UIAbilityContext).terminateSelf();
  }

  // NAPI defensive-programming spec (HARNESS guide 搂3.3): every callQtCommand
  // must be wrapped in try-catch 鈥?a failed dlsym/route otherwise throws an
  // uncaught exception that kills the ArkTS UI thread.
  onMenuItemClick(itemId: string): void {
    hilog.info(DOMAIN, TAG, 'MainLayout menu click: %{public}s', itemId);
    try {
      this.dispatchMenuItem(itemId);
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'menuAction failed for %{public}s: %{public}s',
        itemId, (e as Error).message);
    }
  }

  private dispatchMenuItem(itemId: string): void {
    // Windows menu dynamic entries: 'win:<name>' activates that subwindow.
    if (itemId.startsWith('win:')) {
      const result: string = qtInjector.callQtCommand('activateWindow',
        JSON.stringify({ name: itemId.substring(4) } as ActivateWindowArgs));
      hilog.info(DOMAIN, TAG, 'activateWindow result: %{public}s', result);
      return;
    }
    // Stage-2A: Recent Projects dynamic entries: 'recent:<path>' opens that
    // project via the existing openProject command (Qt-side loadProject also
    // re-pushes the entry to the front of the recent list).
    if (itemId.startsWith('recent:')) {
      const rp: string = itemId.substring('recent:'.length);
      try {
        const result: string = qtInjector.callQtCommand('openProject',
          JSON.stringify({ path: rp } as ProjectPathArgs));
        hilog.info(DOMAIN, TAG, 'openRecent(%{public}s): %{public}s', rp, result);
      } catch (e) {
        hilog.error(DOMAIN, TAG, 'openRecent failed: %{public}s', (e as Error).message);
      }
      return;
    }
    switch (itemId) {
      case 'new_project': {
        // 2026-08-15: unsaved-changes confirmation moved to the Qt side
        // (menuAction branch — same Yes/No/Cancel + save semantics as Win
        // closeEvent); the old ArkTS confirm is gone to avoid double prompts.
        const result: string = qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'new_project' } as MenuActionArgs));
        hilog.info(DOMAIN, TAG, 'new_project result: %{public}s', result);
        this.projectPath = '';
        this.refreshUiState();
        break;
      }
      case 'new_table': {
        const result: string = qtInjector.callQtCommand('newTable', '{}');
        hilog.info(DOMAIN, TAG, 'newTable result: %{public}s', result);
        break;
      }
      case 'import':
        // 2026-08-14 (route 5): File→Import ASCII 改为 Qt 原生内嵌
        // ImportASCIIDialog（QFileDialog 内嵌渲染 + 授权工作文件夹初始
        // 目录）。ArkTS ImportDialog 退役。
        this.openEmbeddedFileDialog('import_ascii');
        break;
      case 'open':
        // 2026-08-14 (route 5): File→Open Project 改为 Qt 原生内嵌
        // OpenProjectDialog（QFileDialog 内嵌渲染 + 授权工作文件夹初始目录）。
        this.openEmbeddedFileDialog('open_project');
        break;
      case 'save':
        // Win parity (2026-08-15): Save with no existing project path
        // prompts Save As (embedded getSaveFileName); once a project is
        // opened/saved, Save is silent to the same path.
        if (this.projectPath === '') {
          this.openEmbeddedFileDialog('save_as');
        } else {
          this.saveProjectToSandbox();
        }
        break;
      case 'save_as':
        // 2026-08-14 (route 5): File→Save As 改为 Qt 原生内嵌 getSaveFileName
        // （ApplicationWindow::saveProjectAs）。
        this.openEmbeddedFileDialog('save_as');
        break;
      case 'open_template':
      case 'save_as_template':
        // Phase-1 (2026-08-15): File→Open/Save As Template（openTemplate()/
        // saveAsTemplate() 内部用 QFileDialog::getOpen/SaveFileName 内嵌，
        // 初始目录 = 授权工作文件夹 templatesDir）。
        this.openEmbeddedFileDialog(itemId);
        break;
      case 'export_pdf':
        // Stage-2C (2026-08-16): Export PDF 探针 — menuAction 分支 →
        // exportPDF()（QFileDialog 内嵌 + MultiLayer::exportVector +
        // QPrinter 直出 PDF，无打印对话框）。真机失败则回退置灰并记录。
        this.openEmbeddedFileDialog('export_pdf');
        break;
      case 'new_function_plot':
        // Phase-1 (2026-08-15): Win File→New→Function Plot（functionDialog()）。
        this.menuActionOrFallback('new_function_plot');
        break;
      case 'export':
      case 'export_ascii':
        // 2026-08-14 (route 5): File/Table→Export ASCII 改为 Qt 原生内嵌
        // ExportDialog（accept → exportASCII → QFileDialog::getSaveFileName
        // 内嵌）。ArkTS ExportDialog 退役。
        this.openEmbeddedFileDialog('export_ascii');
        break;
      case 'export_graph_current':
      case 'export_graph_all':
        // 2026-08-14 (route 5): File→Export Graph 改为 Qt 原生内嵌
        // ImageExportDialog（ApplicationWindow::exportGraph/exportAllGraphs）。
        this.openEmbeddedFileDialog(itemId === 'export_graph_all'
          ? 'export_graph_all' : 'export_graph_current');
        break;
      case 'open_image_file':
      case 'import_image':
      case 'matrix_import_image':
        // 2026-08-14 (route 5): File→Open/Import Image 改为 Qt 原生内嵌
        // getOpenFileName（loadImage/importImage）。
        this.openEmbeddedFileDialog(itemId);
        break;
      case 'add_columns':
      case 'add_column':
        // Qt-native addColumn — rendered embedded in the main window by
        // ohos_bridge; the ArkTS mimic is gone (Qt adds one column per call).
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'add_column' } as MenuActionArgs));
        break;
      case 'dimensions':
      case 'table_dimensions':
      case 'table_size':
        // D3 (2026-08-14): Table→Dimensions 迁 Qt 内嵌
        // （Ui::DimensionsDialog exec，C++ menuAction 分支）。ArkTS
        // TableSizeDialog 已删。
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      case 'assign_formula':
      case 'set_values':
      case 'matrix_assign_formula':
        // Qt-native setValues — dispatched straight to Qt, which picks the
        // Table/Matrix parameter dialog by active window type; the ArkTS
        // mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'set_values' } as MenuActionArgs));
        break;
      case 'set_dimensions':
      case 'matrix_dimensions':
        // Qt-native matrix dimensions dialog — QInputDialog rendered
        // embedded in the main window by ohos_bridge; the ArkTS mimic
        // (MatrixOpsDialog) is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'matrix_dimensions' } as MenuActionArgs));
        break;
      case 'matrix_set_coordinates':
      case 'matrix_set_display_format':
      case 'matrix_go_to_cell':
        // Qt-native matrix dialogs (Set Coordinates / Set Display Format /
        // Go to Cell) — rendered embedded in the main window by ohos_bridge.
        // Qt-native direct command; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      case 'matrix_transpose':
      case 'transpose':
      case 'invert':
      case 'matrix_invert': {
        // 2026-08-14: menu id 'matrix_invert'（Win: Matrix→Invert）与
        // dispatch case 'invert' 错配 → 补 case，避免落 default 未适配。
        const cmd: string = (itemId === 'invert' || itemId === 'matrix_invert')
          ? 'matrixInvert' : 'matrixTranspose';
        const result: string = qtInjector.callQtCommand(cmd,
          JSON.stringify({ matrixId: this.uiState.activeName } as MatrixIdArgs));
        hilog.info(DOMAIN, TAG, '%{public}s queued: %{public}s', cmd, result);
        break;
      }
      case 'rescale_tools':
      case 'rescale': {
        const result: string = qtInjector.callQtCommand('rescale', '{}');
        hilog.info(DOMAIN, TAG, 'rescale result: %{public}s', result);
        break;
      }
      case 'add_curve':
        // Qt-native CurvesDialog — rendered embedded in the main window by
        // ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'add_curve' } as MenuActionArgs));
        break;
      case 'add_error_bars':
        // Qt-native ErrDialog — rendered embedded in the main window by
        // ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'add_error_bars' } as MenuActionArgs));
        break;
      case 'add_function':
        // Qt-native FunctionDialog — rendered embedded in the main window by
        // ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'add_function' } as MenuActionArgs));
        break;
      case 'sort_table':
      case 'sort':
        // D3 (2026-08-14): Table→Sort 迁 Qt 内嵌（future::SortDialog exec，
        // C++ menuAction 分支）。ArkTS SortTableDialog 已删。
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'sort_table' } as MenuActionArgs));
        break;
      case 'statistics_rows':
      case 'stats_rows':
        // Qt-native Statistics on Rows — dispatched straight to Qt; the
        // ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'statistics_rows' } as MenuActionArgs));
        break;
      case 'statistics_columns':
      case 'stats_columns':
        // Qt-native Statistics on Columns — dispatched straight to Qt; the
        // ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'statistics_columns' } as MenuActionArgs));
        break;
      case 'log':
      case 'results_log':
        // Results Log → Qt-native logWindow dock (toggleLog).  The ArkTS
        // replacement panel was removed 2026-08-14 (issue list #2): the Qt
        // dock renders inside the main window and its QTextEdit scrolls via
        // the wheel-injection chain.  Menu checkmark mirrors resp.logVisible
        // via refreshUiState.
        try {
          qtInjector.callQtCommand('toggleLog', '{}');
          hilog.info(DOMAIN, TAG, 'toggleLog sent');
          this.refreshUiState();
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'toggleLog failed: %{public}s', (e as Error).message);
        }
        break;
      case 'project_browser':
        try {
          const result: string = qtInjector.callQtCommand('toggleExplorer', '{}');
          hilog.info(DOMAIN, TAG, 'toggleExplorer result: %{public}s', result);
          this.refreshUiState();
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'toggleExplorer failed: %{public}s', (e as Error).message);
        }
        break;
      case 'preferences':
        // Qt-native Preferences dialog (ConfigDialog) — rendered embedded in
        // the main window by ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'preferences' } as MenuActionArgs));
        break;
      case 'format_axes':
        // Qt-native AxesDialog (Axes/Scale/Grid/Title tab) — rendered embedded
        // in the main window by ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'format_axes' } as MenuActionArgs));
        break;
      case 'format_scales':
        // Qt-native AxesDialog (Scale tab) — rendered embedded in the main
        // window by ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'format_scales' } as MenuActionArgs));
        break;
      case 'format_grid':
        // Qt-native AxesDialog (Grid tab) — rendered embedded in the main
        // window by ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'format_grid' } as MenuActionArgs));
        break;
      case 'format_title':
        // Qt-native TextDialog in AxisTitle mode — menuAction 'show_text'
        // (same branch as Add Text; the C++ side 按调用来源/模式分发).
        // ArkTS FormatDialog title 页随之退役。
        this.menuActionOrFallback('show_text');
        break;
      case 'format_plot':
        // D3 (2026-08-14): Win Format→Plot = PlotDialog — menuAction
        // 'format_plot' 直连（C++ showGeneralPlotDialog，PlotDialog 白名单
        // 内嵌）；ArkTS FormatDialog 已删。
        this.menuActionOrFallback('format_plot');
        break;
      case 'plot_line':
      case 'plot_scatter':
      case 'plot_line_symbol':
      case 'plot_vertical_bars':
      case 'plot_horizontal_bars':
      case 'plot_area':
      case 'plot_pie':
      case 'plot_vertical_drop_lines':
      case 'plot_drop_lines':
      case 'plot_spline':
      case 'plot_vertical_steps':
      case 'plot_horizontal_steps':
      case 'plot_histogram':
      case 'plot_box':
      case 'plot_vectors_xyxy':
      case 'plot_vectors_xyam': {
        // Plot directly with the active table's current selection (Win
        // parity: no column picker; Qt plot2D falls back to
        // selectedColumns() when no cols array is passed).
        const tableId: string = this.uiState.activeName;
        if (tableId === '') {
          hilog.warn(DOMAIN, TAG, 'plot: no active table');
          break;
        }
        // plot_vertical_drop_lines 是 plot_drop_lines 的菜单 id 别名
        // （C++ plot2D 只接受 'drop_lines'，不可 substring 直转）；
        // plot_vectors_xyxy/xyam 映射到 C++ 的 vect_xyxy/vect_xyam
        // （Qt 侧要求恰好 4 列，否则弹错误提示）。
        const type: string = itemId === 'plot_vertical_drop_lines'
          ? 'drop_lines'
          : itemId === 'plot_vectors_xyxy'
            ? 'vect_xyxy'
            : itemId === 'plot_vectors_xyam'
              ? 'vect_xyam'
              : itemId.substring(5); // strip 'plot_'
        try {
          const raw: string = qtInjector.callQtCommand('plot2D',
            JSON.stringify({ tableId: tableId, type: type } as PlotArgs));
          hilog.info(DOMAIN, TAG, 'plot2D(%{public}s) -> %{public}s', type, raw);
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'plot2D failed: %{public}s', (e as Error).message);
        }
        break;
      }
      case 'plot_stacked_histogram': {
        // D3 (2026-08-14): Win Plot→Statistical→Stacked Histogram —
        // C++ plotStackedHistogram 命令（main.cpp，multilayerPlot(1,-1,
        // Graph::Histogram)），不走 plot2D。
        const tableId: string = this.uiState.activeName;
        if (tableId === '') {
          hilog.warn(DOMAIN, TAG, 'stacked histogram: no active table');
          break;
        }
        const result: string = qtInjector.callQtCommand('plotStackedHistogram',
          JSON.stringify({ tableId: tableId } as PlotTableArgs));
        hilog.info(DOMAIN, TAG, 'plotStackedHistogram queued: %{public}s', result);
        break;
      }
      case 'panel_2h':
      case 'panel_2v':
      // Phase-1 (2026-08-15): Win Plot→Panel 4 Layers / Stacked Layers
      // （plotPanel(2,2)/(1,-1)）。
      case 'panel_4':
      case 'panel_stacked': {
        // Panel menu parity: Win plot2HorizontalLayers = multilayerPlot(2,1)
        // (2 side-by-side), plot2VerticalLayers = multilayerPlot(1,2)
        // (2 stacked); plot4Layers = (2,2); plotStackedLayers = (1,-1).
        const tableId: string = this.uiState.activeName;
        if (tableId === '') {
          hilog.warn(DOMAIN, TAG, 'panel: no active table');
          break;
        }
        const cols: number = itemId === 'panel_2h' ? 2
          : (itemId === 'panel_4' ? 2 : 1);
        const rows: number = itemId === 'panel_stacked' ? -1
          : (itemId === 'panel_2h' ? 1 : 2);
        try {
          const raw: string = qtInjector.callQtCommand('plotPanel',
            JSON.stringify({ tableId: tableId, cols: cols, rows: rows } as PanelArgs));
          hilog.info(DOMAIN, TAG, 'plotPanel(%{public}s %{public}dx%{public}d) -> %{public}s',
            itemId, cols, rows, raw);
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'plotPanel failed: %{public}s', (e as Error).message);
        }
        break;
      }
      // Phase-1 (2026-08-15): Table 模式 Analysis→FFT 复用 C++ an_fft 分支
      // （showFFTDialog，FFTDialog 已白名单内嵌）。
      case 'fft':
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'an_fft' } as MenuActionArgs));
        break;
      case 'an_fit_linear':
      case 'an_fit_poly':
      case 'an_fit_gauss':
      case 'an_fit_lorentz':
      case 'an_fit_sigmoidal':
      case 'fit_multi_peak_gauss':
      case 'fit_multi_peak_lorentz':
      case 'an_smooth_savgol':
      case 'an_smooth_average':
      case 'an_smooth_fft':
      case 'an_fft_filter_low':
      case 'an_fft_filter_high':
      case 'an_fft_filter_bandpass':
      case 'an_fft_filter_bandblock':
      case 'an_integrate':
      case 'an_interpolate':
      case 'an_fft':
      case 'an_differentiate':
      case 'an_exp_decay_1':
      case 'an_exp_decay_2':
      case 'an_exp_decay_3':
      case 'an_exp_growth':
        // Qt-native Analysis commands — dispatched straight to Qt.  The C++
        // menuAction branches (another agent, main.cpp) run the op directly
        // (fitLinear etc.) or show the embedded dialog (Smooth/FFT Filter/
        // Integrate/FFT/Fit Polynomial/ExpDecay); the ArkTS mimic
        // (AnalysisDialog) is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      case 'correlate':
        // Qt-native Correlate dialog — two-column picker rendered embedded
        // in the main window by ohos_bridge; the ArkTS mimic
        // (CorrelateDialog) is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'correlate' } as MenuActionArgs));
        break;
      case 'autocorrelate':
      case 'deconvolute':
        // Qt-native direct command; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      case 'convolute':
        // Qt-native direct command (ApplicationWindow::convolute) — embedded
        // warning on wrong selection count; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'convolute' } as MenuActionArgs));
        break;
      case 'add_text':
        // Qt-native TextMarker dialog — menuAction 'show_text'（C++ 另一
        // 代理添加中；未落地前 menuAction 报 unsupported → 未适配提示，
        // ArkTS TextDialog 已删）.
        if (!this.menuActionOrFallback('show_text')) {
          this.showNotAdapted();
        }
        break;
      case 'draw_arrow':
      case 'draw_line':
        // Qt-native Line/Arrow marker dialog — menuAction 'show_line'
        // (C++ 按调用来源区分画线/画箭头；ArkTS LineDialog 已删).
        if (!this.menuActionOrFallback('show_line')) {
          this.showNotAdapted();
        }
        break;
      case 'time_stamp': {
        const result: string = qtInjector.callQtCommand('timeStamp',
          JSON.stringify({ plotId: this.uiState.activeName } as PlotActionArgs));
        hilog.info(DOMAIN, TAG, 'timeStamp queued: %{public}s', result);
        break;
      }
      case 'add_image':
        // Qt-native ImageMarker dialog — menuAction 'show_image'（C++ 另一
        // 代理添加中；ArkTS ImageDialog 已删）.
        if (!this.menuActionOrFallback('show_image')) {
          this.showNotAdapted();
        }
        break;
      case 'new_legend': {
        const result: string = qtInjector.callQtCommand('newLegend',
          JSON.stringify({ plotId: this.uiState.activeName } as PlotActionArgs));
        hilog.info(DOMAIN, TAG, 'newLegend queued: %{public}s', result);
        break;
      }
      case 'normalize': {
        const result: string = qtInjector.callQtCommand('normalize',
          JSON.stringify({ tableId: this.uiState.activeName } as TableSizeArgs));
        hilog.info(DOMAIN, TAG, 'normalize queued: %{public}s', result);
        break;
      }
      case 'convolve': {
        const result: string = qtInjector.callQtCommand('convolve',
          JSON.stringify({ tableId: this.uiState.activeName } as TableSizeArgs));
        hilog.info(DOMAIN, TAG, 'convolve queued: %{public}s', result);
        break;
      }
      case 'paste':
         // Clipboard bridge: pull the OHOS system pasteboard into the Qt
         // clipboard first, then dispatch the paste (both queued → FIFO
         // order on the Qt thread is guaranteed).
         this.pasteWithSystemClipboard();
         break;
      case 'insert_row': {
         const result: string = qtInjector.callQtCommand('insert_row', '{}');
         hilog.info(DOMAIN, TAG, 'insert_row: %{public}s', result);
         setTimeout(() => { this.refreshUiState(); }, 300);
         break;
      }
      case 'insert_col': {
         const result: string = qtInjector.callQtCommand('insert_col', '{}');
         hilog.info(DOMAIN, TAG, 'insert_col: %{public}s', result);
         setTimeout(() => { this.refreshUiState(); }, 300);
         break;
      }
      case 'quit':
        // 2026-08-17: 退出确认整体迁移到 Qt 侧 menuAction('quit')：由 C++
        // 弹原生 SciDAVis QMessageBox（Yes/No/Cancel），不再弹 ArkTS 窗口。
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'quit' } as MenuActionArgs));
        break;
      case 'fit_wizard':
        // Qt-native FitDialog (Fit/Edit/Advanced) — rendered embedded in the
        // main window by ohos_bridge; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'fit_wizard' } as MenuActionArgs));
        break;
      case 'plot_wizard':
        // D3 (2026-08-14): View→Plot Wizard 迁 Qt 内嵌（PlotWizard exec，
        // C++ menuAction 分支）。ArkTS PlotWizardDialog 已删。
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'plot_wizard' } as MenuActionArgs));
        break;
      case 'arrange_layers':
        // C++ menuAction 'arrange_layers' 已落地（main.cpp L3246，内嵌 Qt
        // LayerDialog）。menuAction 主路径生效；ArkTS LayerDialog 为
        // fire-and-forget 语义下的防御性兜底。
        if (!this.menuActionOrFallback('arrange_layers')) {
          this.showLayerDialog = true;
        }
        break;
      case 'edit_range':
        // Curve row range (curve context menu).  C++ menuAction 'edit_range'
        // 已落地（main.cpp L3255，内嵌 Qt CurveRangeDialog）。menuAction
        // 主路径生效；ArkTS CurveRangeDialog 为防御性兜底。
        if (!this.menuActionOrFallback('edit_range')) {
          this.showCurveRangeDialog = true;
        }
        break;
      case 'associations':
        // Curve column associations (curve context menu).  C++ menuAction
        // 'associations' 已落地（main.cpp L3279，内嵌 Qt AssociationsDialog）。
        // menuAction 主路径生效；ArkTS AssociationsDialog 为防御性兜底。
        if (!this.menuActionOrFallback('associations')) {
          this.showAssociationsDialog = true;
        }
        break;
      case 'plot_details':
        // Qt-native PlotDialog（Win: 曲线右键 Plot details）— menuAction
        // 'plot_details' 已落地（main.cpp L3302，内嵌 Qt PlotDialog）。
        // menuAction 主路径生效；未适配提示为防御性兜底。
        if (!this.menuActionOrFallback('plot_details')) {
          this.showNotAdapted();
        }
        break;
      case 'window_geometry':
        // Qt-native Window Geometry（Win 复用 ImageDialog 布局）— menuAction
        // 'window_geometry' 已落地（main.cpp L3317，resizeActiveWindow）。
        // menuAction 主路径生效；未适配提示为防御性兜底。
        if (!this.menuActionOrFallback('window_geometry')) {
          this.showNotAdapted();
        }
        break;
      case 'marker_properties':
        // Marker right-click Properties: dispatch by the markerType cached
        // from the last contextMenuRequest.  C++ 无独立 'show_text_marker'
        // 分支（grep main.cpp 确认）→ text 复用 'show_text'，由 show_text
        // 分支按模式区分新建/编辑（TODO: C++ 若提供 show_text_marker 则
        // 改之）。
        if (!this.menuActionOrFallback(
          this.contextMenuMarkerType === 'arrow' ? 'show_line'
            : (this.contextMenuMarkerType === 'image' ? 'show_image' : 'show_text'))) {
          this.showNotAdapted();
        }
        break;
      case 'rename_window':
        // Qt-native SciDAVisRenameWindow embedded; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'rename_window' } as MenuActionArgs));
        break;
      case 'close_window':
        // Qt-native close (Ctrl+W) — Win has Close Window in the Windows
        // menu; C++ menuAction 'close_window' branch is ready.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'close_window' } as MenuActionArgs));
        break;
      // D3 (2026-08-14): Help 链接类 — 系统默认浏览器打开对应网页
      // （webview.openBrowser，不内置浏览器；Win URI 常量同源，http→https）。
      case 'scidavis_homepage':
      case 'search_updates':
      case 'download_manual':
      case 'visit_forums':
      case 'report_bug':
        this.openHelpLink(itemId);
        break;
      // Stage-7 misc: About / Scripting Language / Undo-Redo History.
      case 'about':
        // Qt-native SciDAVisAbout embedded; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'about' } as MenuActionArgs));
        break;
      case 'scripting_language':
        // Qt-native ScriptingLangDialog embedded; the ArkTS mimic is gone.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'scripting_language' } as MenuActionArgs));
        break;
      case 'restart_scripting':
        // Phase-1 (2026-08-15): Win Scripting→Restart Scripting
        // （restartScriptingEnv()，C++ 分支已落地）。
        this.menuActionOrFallback('restart_scripting');
        break;
      case 'note_execute':
      case 'note_execute_all':
      case 'note_evaluate':
        // P3 (2026-08-16): Note 激活脚本执行（Win L1246-1248 → Note::
        // execute/executeAll/evaluate；C++ menuAction 分支已落地）。
        this.menuActionOrFallback(itemId);
        break;
      case 'undo_redo_history':
        // View→Undo/Redo History — Qt-native QUndoView 内嵌于主窗口
        // （ohos_bridge 承接）；menuAction 直连（参照 add_curve 范式）.
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: 'undo_redo_history' } as MenuActionArgs));
        break;
      // D3 (2026-08-14): 时间戳直连 timeStamp 命令（同 case 'time_stamp'）；
      // add_layer/remove_layer 走 menuAction（C++ 分支落地，白名单）。
      case 'add_time_stamp': {
        const result: string = qtInjector.callQtCommand('timeStamp',
          JSON.stringify({ plotId: this.uiState.activeName } as PlotActionArgs));
        hilog.info(DOMAIN, TAG, 'timeStamp queued: %{public}s', result);
        break;
      }
      // Phase-1 (2026-08-15): automatic_layout 已接线 C++ autoArrangeLayers。
      case 'automatic_layout':
        this.menuActionOrFallback('automatic_layout');
        break;
      // Table menu: set columns as (with subitems) — each id maps to the
      // matching Qt plot-designation action; the ArkTS mimic is gone.
      case 'set_columns_as':
      case 'set_as_x':
      case 'set_as_y':
      case 'set_as_z':
      case 'set_as_xerr':
      case 'set_as_yerr':
      case 'set_as_none':
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      // Table menu: fill selection (with subitems) — each id maps to the
      // matching Qt fill action; the ArkTS mimic is gone.
      case 'fill_selection':
      case 'fill_row_numbers':
      case 'fill_random':
        qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        break;
      // Tools menu: move_points / remove_bad — 已置灰禁用（2026-08-13
      // 真机实测）。根因：OH 编译的 QwtPicker 是 54 行 stub（qwt_picker.cpp
      // 构造不 installEventFilter、eventFilter 恒 false）→ DataPickerTool
      // 事件处理无效；且 setActiveTool 后 canvas->setCursor() 在单窗口 QPA
      // 软件渲染下引发合成错乱。C++ 分支保留备用（未来恢复 Qwt 交互层可
      // 重新启用）；此处不再直连，回落到 default 的未适配提示。
      default: {
        // Unadapted item: the queued menuAction reply cannot tell us (see
        // MENU_ACTION_SUPPORTED above), so gate on the local whitelist.
        if (MENU_ACTION_SUPPORTED.indexOf(itemId) < 0) {
          hilog.info(DOMAIN, TAG, 'not adapted: %{public}s', itemId);
          this.showNotAdapted();
          break;
        }
        // Fallback: route whitelisted items to Qt so the backend can decide
        // (HARNESS guide P1 鈥?users expect a real response, not just a log).
        const result: string = qtInjector.callQtCommand('menuAction',
          JSON.stringify({ itemId: itemId } as MenuActionArgs));
        hilog.info(DOMAIN, TAG, 'fallback menuAction(%{public}s): %{public}s', itemId, result);
        try {
          const resp: CmdResult = JSON.parse(result) as CmdResult;
          if (resp.success === false) {
            // Belt-and-braces: synchronous failures (e.g. Qt not running)
            // still surface the same unified notice.
            this.showNotAdapted();
          }
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'fallback parse failed: %{public}s', (e as Error).message);
        }
        break;
      }
    }
  }

  // 鈹€鈹€ Project open/save (Phase 2) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
  // Qt only reaches sandbox paths, so the pickers copy files in/out.

  // 鈹€鈹€ Clipboard bridge (Phase 2) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
  // Qt 鈫?OHOS runs through the clipboardChanged event (QtEventHost).
  // The PasteButton temp READ_PASTEBOARD authorization is one-shot and is
  // granted on click — the read MUST complete synchronously inside this
  // onClick chain (spec §2.2), so only getDataSync() is used here (same
  // constraint as the JsPasteBoard bridge, spec §1.3).  An async getData()
  // would resume after the onClick callback returns and may lose the auth.
  private pasteWithSystemClipboard(): void {
    try {
      const data: pasteboard.PasteData = pasteboard.getSystemPasteboard().getDataSync();
      const text: string = data.getPrimaryText();
      if (text !== null && text !== undefined && text !== '') {
        qtInjector.callQtCommand('setClipboardText',
          JSON.stringify({ text: text } as SetClipboardArgs));
      }
    } catch (e) {
      // Empty/non-text pasteboard or auth window missed — fall through to
      // Qt's own clipboard (the in-app copy already lives in Qt's cache).
      hilog.info(DOMAIN, TAG, 'pasteboard read skipped: %{public}s', (e as Error).message);
    }
    const result: string = qtInjector.callQtCommand('menuAction',
      JSON.stringify({ itemId: 'paste' } as MenuActionArgs));
    hilog.info(DOMAIN, TAG, 'paste dispatched: %{public}s', result);
  }

  // Stage-2D (2026-08-15): exportImageViaPicker removed — File→Export Graph
  // goes through the embedded ImageExportDialog (route 5, work-folder direct
  // write); the old picker+copy-out chain had no reachable caller.

  private sandboxProjectDir(): string {
    const ctx = getContext(this) as common.UIAbilityContext;
    return ctx.filesDir;
  }

  private async openProjectViaPicker(): Promise<void> {
    try {
      // 2026-08-14 文件夹级授权直通: 优先从授权工作文件夹列表选 .sciprj
      // 直通 openProject（Qt 直读沙箱外已验证）。工作文件夹未授权时回退
      // 到 DocumentViewPicker 单文件选择 + 拷贝（兼容旧流程）。
      if (this.workFolderPath !== '') {
        const candidates: string[] = [];
        try {
          const names: string[] = fs.listFileSync(this.workFolderPath);
          for (const n of names) {
            if (n.endsWith('.sciprj') || n.endsWith('.sciprj.gz') || n.endsWith('.gz')) {
              candidates.push(n);
            }
          }
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'openProject list workFolder failed: %{public}s', JSON.stringify(e));
        }
        if (candidates.length > 0) {
          this.workFolderFiles = candidates;
          this.showWorkFolderOpenDialog = true;
          return; // 用户在 OpenDialog 中选择 → openProjectFromWorkFolder()
        }
        hilog.warn(DOMAIN, TAG, 'openProject: no .sciprj in work folder — falling back to picker');
      }
      const docPicker = new picker.DocumentViewPicker();
      const opts = new picker.DocumentSelectOptions();
      opts.maxSelectNumber = 1;
      opts.fileSuffixFilters = ['.sciprj', '.gz'];
      const uris: string[] = await docPicker.select(opts);
      if (uris.length === 0) {
        return; // user cancelled
      }
      const uri: string = uris[0];
      let name: string = decodeURIComponent(uri.substring(uri.lastIndexOf('/') + 1));
      if (name === '') {
        name = 'project.sciprj';
      }
      const dest: string = this.sandboxProjectDir() + '/' + name;
      const src = fs.openSync(uri, fs.OpenMode.READ_ONLY);
      fs.copyFileSync(src.fd, dest);
      fs.closeSync(src);
      this.projectPath = dest;
      const result: string = qtInjector.callQtCommand('openProject',
        JSON.stringify({ path: dest } as ProjectPathArgs));
      hilog.info(DOMAIN, TAG, 'openProject(%{public}s): %{public}s', dest, result);
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'openProjectViaPicker failed: %{public}s', (e as Error).message);
    }
  }

  // 打开项目：确保工作文件夹授权后列出 .sciprj（直通）；无授权/无项目时
  // 回退 openProjectViaPicker 的单文件 picker+拷贝。
  private async openViaWorkFolder(): Promise<void> {
    if (this.workFolderPath === '') {
      const ok: boolean = await this.ensureWorkFolder();
      if (!ok || this.workFolderPath === '') {
        return; // 用户取消授权
      }
    }
    this.openProjectViaPicker();
  }

  // 用户在工作文件夹 OpenDialog 中选中项目 → 直通 openProject（无拷贝）。
  private openProjectFromWorkFolder(name: string): void {
    if (this.workFolderPath === '' || name === '') {
      return;
    }
    try {
      const dest: string = this.workFolderPath + '/' + name;
      this.projectPath = dest;
      const result: string = qtInjector.callQtCommand('openProject',
        JSON.stringify({ path: dest } as ProjectPathArgs));
      hilog.info(DOMAIN, TAG, 'openProject(%{public}s) workFolder: %{public}s', dest, result);
      this.showWorkFolderOpenDialog = false;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'openProjectFromWorkFolder failed: %{public}s', (e as Error).message);
    }
  }

  // Save to the current sandbox project file (untitled.sciprj when new).
  // saveProject is a QUEUED command: saving calls back into the JS thread
  // via the QPA, so a blocking query would deadlock (device-verified 3s
  // timeout).  Completion arrives as the 'projectSaved' event, handled in
  // QtEventHost (which also performs the pending save-as copy-out).
  // 2026-08-14: 有授权工作文件夹时直通写入（Qt 直写沙箱外已验证）。
  private saveProjectToSandbox(): string {
    const dest: string = this.projectPath !== ''
      ? this.projectPath
      : (this.workFolderPath !== ''
        ? this.workFolderPath + '/untitled.sciprj'
        : this.sandboxProjectDir() + '/untitled.sciprj');
    this.projectPath = dest;
    const result: string = qtInjector.callQtCommand('saveProject',
      JSON.stringify({ path: dest } as ProjectPathArgs));
    hilog.info(DOMAIN, TAG, 'saveProject(%{public}s) queued: %{public}s', dest, result);
    return dest;
  }

  // Stage-2D (2026-08-15): saveProjectAsViaPicker removed — File→Save As
  // goes through the embedded getSaveFileName (route 5, work-folder direct
  // write); the old picker+copy-out chain had no reachable caller.

  // ── 2026-08-14 文件夹级授权直通（路线5 探针验证）─────────────────────
  // 用户通过 FOLDER picker 授权一个工作目录，Qt/C++ 即可直接读写该目录
  // （探针证据: QFileInfo exists=1、QDir entryList、QFile 读写全通过）。
  // 授权是进程级的：应用重启后授权失效，需重新选择（persistPermission
  // 持久化列为远期）。所有导入/导出/打开/保存命令都接受路径参数，
  // 因此 C++ 侧零改动，只需 ArkTS 把授权路径传给命令即可。

  // 授权工作文件夹（若无）：FOLDER picker → FileUri.path → 存 workFolderPath。
  // 返回是否成功。用户取消返回 false。
  private async ensureWorkFolder(): Promise<boolean> {
    if (this.workFolderPath !== '') {
      return true;
    }
    // 探针（2026-08-14, authMode 路线）: 先尝试 DocumentSelectOptions.authMode
    // 静默授权"个人"目录 —— authMode=true 时系统不显示 Picker UI，直接对
    // defaultFilePathUri 指向的路径发起授权（API 12+；文档称 2-in-1 生效，
    // 平板行为以真机为准）。成功 → 用户无感知；失败/异常 → 回退 FOLDER
    // picker（现有流程）。个人目录文档类 URI 为 file://docs/storage/Users/currentUser。
    const personalUri: string = 'file://docs/storage/Users/currentUser';
    try {
      const authPicker = new picker.DocumentViewPicker();
      const authOpts = new picker.DocumentSelectOptions();
      authOpts.selectMode = picker.DocumentSelectMode.FOLDER;
      authOpts.defaultFilePathUri = personalUri;
      authOpts.authMode = true;
      const authUris: string[] = await authPicker.select(authOpts);
      hilog.info(DOMAIN, TAG, 'workFolder authMode: returned %{public}d uris, first=%{public}s',
        authUris.length, authUris.length > 0 ? authUris[0] : '(none)');
      if (authUris.length > 0) {
        const authPath: string = new fileUri.FileUri(authUris[0]).path;
        if (authPath !== '' && (authPath === '/storage/Users/currentUser'
          || authPath.startsWith('/storage/Users/currentUser/'))) {
          this.workFolderPath = authPath;
          AppStorage.setOrCreate<string>('workFolderPath', authPath);
          try {
            qtInjector.callQtCommand('setWorkFolderPath',
              JSON.stringify({ path: authPath } as WorkFolderArgs));
          } catch (e) {
            hilog.error(DOMAIN, TAG, 'authMode setWorkFolderPath failed: %{public}s', (e as Error).message);
          }
          try {
            const ctx = getContext(this) as common.UIAbilityContext;
            const store = await preferences.getPreferences(ctx, 'ohplot_workfolder');
            store.putSync('workFolderPath', authPath);
            store.putSync('workFolderUri', authUris[0]);
            await store.flush();
          } catch (e) {
            hilog.error(DOMAIN, TAG, 'authMode persist failed: %{public}s', JSON.stringify(e));
          }
          await this.persistWorkFolderGrant(authUris[0]);
          hilog.info(DOMAIN, TAG, 'workFolder authMode authorized: %{public}s', authPath);
          return true;
        }
        hilog.warn(DOMAIN, TAG, 'workFolder authMode: unexpected path %{public}s — falling back to picker', authPath);
      }
    } catch (e) {
      // authMode 在非 2-in-1 上可能抛"参数无效/不支持"——记录后走正常 picker。
      hilog.info(DOMAIN, TAG, 'workFolder authMode unsupported/failed: %{public}s — falling back to picker',
        JSON.stringify(e));
    }
    try {
      const docPicker = new picker.DocumentViewPicker();
      const opts = new picker.DocumentSelectOptions();
      opts.selectMode = picker.DocumentSelectMode.FOLDER;
      const uris: string[] = await docPicker.select(opts);
      if (uris.length === 0) {
        hilog.info(DOMAIN, TAG, 'workFolder: picker cancelled');
        return false;
      }
      const uri: string = uris[0];
      const path: string = new fileUri.FileUri(uri).path;
      if (path === '') {
        hilog.error(DOMAIN, TAG, 'workFolder: FileUri path empty for %{public}s', uri);
        return false;
      }
      this.workFolderPath = path;
      // 供 ImportDialog/ExportDialog/其他组件读取
      AppStorage.setOrCreate<string>('workFolderPath', path);
      // 2026-08-14 内嵌文件对话框: 授权路径推给 Qt → QFileDialog 直接在该
      // 目录内浏览选择（Qt 直读写沙箱外已验证）。
      try {
        qtInjector.callQtCommand('setWorkFolderPath',
          JSON.stringify({ path: path } as WorkFolderArgs));
        hilog.info(DOMAIN, TAG, 'workFolder pushed to Qt: %{public}s', path);
      } catch (e) {
        hilog.error(DOMAIN, TAG, 'setWorkFolderPath failed: %{public}s', (e as Error).message);
      }
      // 2026-08-14: 持久化路径（免每次启动重选；授权失效时下次使用会重提）。
      try {
        const ctx = getContext(this) as common.UIAbilityContext;
        const store = await preferences.getPreferences(ctx, 'ohplot_workfolder');
        store.putSync('workFolderPath', path);
        store.putSync('workFolderUri', uri);
        await store.flush();
        hilog.info(DOMAIN, TAG, 'workFolder persisted: %{public}s', path);
      } catch (e) {
        hilog.error(DOMAIN, TAG, 'workFolder persist failed: %{public}s', JSON.stringify(e));
      }
      // 2026-08-14: 持久化授权策略（fileShare.persistPermission, API 11+，
      // ohos.permission.FILE_ACCESS_PERSIST = normal/system_grant）。成功后
      // 冷启动 restoreWorkFolder 可静默重新激活，无需再弹 FOLDER picker。
      await this.persistWorkFolderGrant(uri);
      hilog.info(DOMAIN, TAG, 'workFolder authorized: %{public}s', path);
      return true;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'workFolder picker failed: %{public}s', JSON.stringify(e));
      return false;
    }
  }

  // 列出工作文件夹内文件（fs 直读授权路径，探针已验证）。
  // 返回文件名字符串数组；失败返回空数组。
  private listWorkFolderFiles(): string[] {
    if (this.workFolderPath === '') {
      return [];
    }
    try {
      const names: string[] = fs.listFileSync(this.workFolderPath);
      const files: string[] = [];
      for (const n of names) {
        try {
          const st = fs.statSync(this.workFolderPath + '/' + n);
          if (st.isFile()) {
            files.push(n);
          }
        } catch (e) {
          // stat 失败（隐藏/特殊项）跳过
        }
      }
      return files;
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'workFolder list failed: %{public}s', JSON.stringify(e));
      return [];
    }
  }

  // 检查当前 workFolderPath 是否仍对 Qt 直接可读。路径存在但授权失效时，
  // Qt 文件对话框会打开一个“空白/不可访问”的目录；这里在打开任何内嵌文件
  // 对话框前复核一次，失效就清理并重新走 ensureWorkFolder。
  // ArkTS-side access check: fs.listFileSync throws when the app does not
  // actually hold a usable grant for the external path, while Qt's
  // QDir::entryList can return 0 entries both for "empty" and "denied"
  // (the C++ check cannot reliably tell them apart on OHOS).  Using the
  // ArkTS filesystem layer here mirrors what the picker grant actually
  // controls, so a stale authorized path is recognised and re-picked.
  private isPathAccessible(path: string): boolean {
    if (path === '') {
      return false;
    }
    try {
      const names: string[] = fs.listFileSync(path);
      hilog.info(DOMAIN, TAG, 'workFolder fs-access: %{public}s ok entries=%{public}d',
        path, names.length);
      return true;
    } catch (e) {
      hilog.warn(DOMAIN, TAG, 'workFolder fs-access failed: %{public}s err=%{public}s',
        path, JSON.stringify(e));
      return false;
    }
  }

  private isWorkFolderAccessible(): boolean {
    if (this.workFolderPath === '') {
      return false;
    }
    return this.isPathAccessible(this.workFolderPath);
  }

  // 打开 Qt 原生内嵌文件对话框（route 5, 2026-08-14）：先确保授权工作
  // 文件夹（Qt 侧 setWorkFolderPath 已同步），再 dispatch menuAction →
  // C++ 打开内嵌的 ImportASCIIDialog / ExportDialog（QFileDialog 内嵌渲染，
  // 初始目录 = 授权工作文件夹）。2026-08-17：即使已有持久化路径，也先复核
  // 是否仍可访问，避免“个人文件夹空白”（grant 已过期但路径仍被 push 到 Qt）。
  private async openEmbeddedFileDialog(itemId: string): Promise<void> {
    if (this.workFolderPath !== '' && !this.isWorkFolderAccessible()) {
      hilog.warn(DOMAIN, TAG, 'workFolder stale — clearing and re-prompting');
      this.workFolderPath = '';
      AppStorage.setOrCreate<string>('workFolderPath', '');
    }
    if (this.workFolderPath === '') {
      const ok: boolean = await this.ensureWorkFolder();
      if (!ok || this.workFolderPath === '') {
        return; // 用户取消授权 → 不打开对话框
      }
    }
    qtInjector.callQtCommand('menuAction',
      JSON.stringify({ itemId: itemId } as MenuActionArgs));
  }

  // 导出图到授权工作文件夹（直通，不拷贝）。all=true 导出全部图。
  private async exportGraphViaWorkFolder(all: boolean): Promise<void> {
    if (this.workFolderPath === '') {
      const ok: boolean = await this.ensureWorkFolder();
      if (!ok || this.workFolderPath === '') {
        return;
      }
    }
    try {
      if (all) {
        // 遍历所有 MultiLayer 子窗口逐个导出（getPlotList 返回 name 字段）
        const listRaw: string = qtInjector.callQtCommand('getPlotList', '{}');
        let plotIds: string[] = [];
        try {
          const parsed = JSON.parse(listRaw) as PlotListResponse;
          if (parsed.success && parsed.data) {
            plotIds = parsed.data.map((p: PlotListEntry) => p.name);
          }
        } catch (e) {
          hilog.error(DOMAIN, TAG, 'exportGraphAll parse failed: %{public}s', (e as Error).message);
        }
        if (plotIds.length === 0) {
          hilog.warn(DOMAIN, TAG, 'exportGraphAll: no plots');
          return;
        }
        for (const pid of plotIds) {
          const dest: string = this.workFolderPath + '/' + pid + '.png';
          qtInjector.callQtCommand('exportGraphImage',
            JSON.stringify({ plotId: pid, path: dest } as ExportImageArgs));
          hilog.info(DOMAIN, TAG, 'exportGraphImage(%{public}s) queued: %{public}s', pid, dest);
        }
      } else {
        const plotId: string = this.uiState.activeName;
        if (plotId === '') {
          return;
        }
        const dest: string = this.workFolderPath + '/' + plotId + '.png';
        qtInjector.callQtCommand('exportGraphImage',
          JSON.stringify({ plotId: plotId, path: dest } as ExportImageArgs));
        hilog.info(DOMAIN, TAG, 'exportGraphImage(%{public}s) queued: %{public}s', plotId, dest);
      }
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'exportGraphViaWorkFolder failed: %{public}s', (e as Error).message);
    }
  }

  // P1 (2026-08-12): route every sendMouse here.  Moves (action=2) are
  // merged into MOUSE_MOVE_THROTTLE_MS windows: the first move of a window
  // goes out immediately and later moves only refresh the kept coords; a
  // flush timer at the window end sends the latest position once.  Down
  // (0) / Up (1) are sent synchronously, never merged — press/release
  // timing stays intact.  Ordering: a queued move is always flushed BEFORE
  // a Down/Up, so Qt never sees the release ahead of the drag's final
  // move, and an Up can never be dropped (it is never queued).
  // qtInjector.sendMouse parameter semantics are unchanged.
  private forwardMouse(px: number, py: number, button: number, action: number): void {
    if (action === 2) {
      const now: number = Date.now();
      if (this.mouseMoveQueued) {
        // window still open — keep only the latest coords
        this.mouseMoveQueuedX = px;
        this.mouseMoveQueuedY = py;
        this.mouseMoveQueuedBtn = button;
        return;
      }
      if (now - this.mouseMoveLastSent >= this.MOUSE_MOVE_THROTTLE_MS) {
        this.mouseMoveLastSent = now;
        this.sendMouseRaw(px, py, button, action);
        return;
      }
      // window open: queue the latest and arm the flush at its end
      this.mouseMoveQueued = true;
      this.mouseMoveQueuedX = px;
      this.mouseMoveQueuedY = py;
      this.mouseMoveQueuedBtn = button;
      setTimeout(() => this.flushQueuedMove(),
        this.MOUSE_MOVE_THROTTLE_MS - (now - this.mouseMoveLastSent));
      return;
    }
    // Down/Up: never throttled; flush any queued move first to keep order.
    this.flushQueuedMove();
    this.sendMouseRaw(px, py, button, action);
  }

  // Sends the merged latest position once the throttle window ends.
  private flushQueuedMove(): void {
    if (!this.mouseMoveQueued) {
      return;
    }
    this.mouseMoveQueued = false;
    this.mouseMoveLastSent = Date.now();
    this.sendMouseRaw(this.mouseMoveQueuedX, this.mouseMoveQueuedY, this.mouseMoveQueuedBtn, 2);
  }

  // Single raw synchronous NAPI call site (was duplicated in onTouch and
  // onMouse).  Only this method touches qtInjector.sendMouse now.
  private sendMouseRaw(px: number, py: number, button: number, action: number): void {
    try {
      // 2in1 diag: log press/release coordinates once per gesture to verify
      // the decor compensation (qtDecorH) is applied.
      if (action === 0 || action === 1) {
        hilog.info(DOMAIN, TAG, 'sendMouse %{public}d at (%{public}d,%{public}d) decor=%{public}d',
          action, px, py, AppStorage.get<number>('qtDecorH') ?? 0);
      }
      const ok: boolean = qtInjector.sendMouse(px, py, button, action);
      if (!ok) {
        hilog.warn(DOMAIN, TAG, 'sendMouse(act=%{public}d) returned false', action);
      }
    } catch (e) {
      hilog.error(DOMAIN, TAG, 'sendMouse(act=%{public}d) failed: %{public}s',
        action, (e as Error).message);
    }
  }

  // ── Results Log (Qt-native logWindow dock) ────────────────────────────
  // The ArkTS replacement panel was removed 2026-08-14 (issue list #2):
  // all Results Log entries now call Qt toggleLog, which shows the Qt
  // logWindow QDockWidget inside the main window; its QTextEdit scrolls
  // through the wheel-injection chain (spec 15).

  build() {
    Stack() {
      // Layer 1 (bottom): XComponent lives BELOW this Stack.
      // This is the Qt rendering surface - events must pass through to it.

      // Layer 2: Main layout with menu bar, content area, status bar
      Column() {
        // Menu bar
        MenuBar({
          openMenu: this.openMenu,
          menus: this.menuDefs,
          onItemClick: (itemId: string) => this.onMenuItemClick(itemId),
          onAnchorChange: (menuId: string, x: number) => {
            this.menuAnchorMap.set(menuId, x);
            if (this.openMenu === menuId) {
              this.dropdownX = x;
            }
          }
        })

        // ArkTS toolbar: original SciDAVis desktop icons grouped into the
        // Windows toolbar categories (File/Edit/Graph/Plot/3D); groups wrap
        // onto extra rows on narrow screens and the context groups follow
        // the active window type.  Ids reuse the menu dispatch so behaviour
        // matches the menu items 1:1.
        ToolBar({
          undoEnabled: this.uiState.undoEnabled,
          redoEnabled: this.uiState.redoEnabled,
          hasWin: this.uiState.activeName !== '',
          isTable: this.uiState.activeType === 'Table',
          isGraph: this.uiState.activeType === 'MultiLayer',
          onItemClick: (itemId: string) => {
            this.onMenuItemClick(itemId);
            // keep greying current (menus refresh on open; toolbar has no
            // such hook, so refresh right after each dispatch)
            this.refreshUiState();
            // Mutations are QUEUED (new_*, activateWindow...), so the
            // immediate refresh can read the pre-mutation state and leave
            // stale context groups (device-verified: Graph group lingered
            // after New Matrix).  Refresh again once the queue drained.
            setTimeout(() => this.refreshUiState(), 700);
          },
          // 问题4b: toolbar right-click 鈫?reuse the existing ContextMenu.
          // ToolBar forwards window coordinates; both live in the same
          // full-window space as the Stack-relative contextMenuX/Y.
          onContextMenu: (x: number, y: number) => {
            this.contextMenuX = x;
            this.contextMenuY = y;
            // Toolbar right-click: no Qt widget under it — reset the target
            // so a stale widgetClass from a previous graph right-click
            // cannot leak into this menu (blank dock toggles).
            this.contextMenuWidgetClass = '';
            this.contextMenuMarkerType = '';
            this.showContextMenu = true;
          },
          onHoverTip: (tipKey: string, x: number, y: number, show: boolean) => {
            this.handleHoverTip(tipKey, x, y, show);
          }
        })
          // The toolbar height changes when rows wrap or context groups
          // toggle; track it so the touch guards, floating panels and the
          // Qt-side insets all follow the real chrome height.
          .onAreaChange((_old: Area, now: Area) => {
            const h: number = now.height as number;
            if (h > 0 && h !== this.toolbarHeightVp) {
              this.toolbarHeightVp = h;
              this.pushChromeInsets();
            }
          })

        // Content area: transparent pass-through to Qt native rendering.
        // When menu is open, touching the content area closes it.
        // Default: transparent - touch events pass through to
        // the XComponent for native QPA processing.  We do NOT forward
        // touch via sendMouse here 鈥?QPA handles it natively.
        Column()
          .layoutWeight(1)
          .hitTestBehavior(HitTestMode.Transparent)
          .onTouch((_event: TouchEvent) => {
            // Touch ANYWHERE in the content area closes an open menu.
            if (this.openMenu !== null) {
              hilog.info(DOMAIN, TAG, 'content touch - closing menu');
              this.openMenu = null;
            }
          })
          // Mouse-wheel forwarding lives on the XComponent in Index.ets
          // (restored 2026-08-13): the transparent content Column never
          // receives AxisEvents, so this overlay must not register its own
          // onAxisEvent handler (duplicate sendWheel path).

        // Bottom toolbar: 2D-plot and 3D-surface groups docked below the
        // content area (Windows plot_tools/plot3d moved to the lower page
        // region).  Always visible regardless of active window type.
        BottomToolBar({
          // P3 (2026-08-16): 按 Win customToolBars 语义灰显 —— 项目里至少
          // 有一张表才启用 Plot/Table 组，至少一个矩阵才启用 Matrix 组。
          hasTable: this.uiState.windows.some((w: UiWindow) => w.type === 'Table'),
          hasMatrix: this.uiState.windows.some((w: UiWindow) => w.type === 'Matrix'),
          onItemClick: (itemId: string) => {
            this.onMenuItemClick(itemId);
            this.refreshUiState();
            setTimeout(() => this.refreshUiState(), 700);
          },
          onHoverTip: (tipKey: string, x: number, y: number, show: boolean) => {
            this.handleHoverTip(tipKey, x, y, show);
          }
        })
          .onAreaChange((_old: Area, now: Area) => {
            const h: number = now.height as number;
            if (h >= 0 && h !== this.bottomToolbarHeightVp) {
              this.bottomToolbarHeightVp = h;
              this.pushChromeInsets();
            }
            if (now.globalPosition) {
              this.bottomBarTopY = now.globalPosition.y as number;
              this.updateMenuBottomLimit();
            }
          })

        // Status bar
        StatusBar({ statusText: this.statusText })
      }
      .width('100%')
      .height('100%')

      // Qt startup overlay: covers the XComponent's black surface until Qt
      // renders its first frame (signalled by uiState.activeName != '' in
      // refreshUiState → qtReady).  Sits above the main Column (zIndex 10)
      // but below every interactive layer (scrim 100 / dropdown 101 /
      // context menu 200 / tooltip 300).  Transparent hit-test so the first
      // touches pass through to Qt once rendering starts.  (2026-08-04)
      if (!this.qtReady) {
        Column() {
          Text(getAppString('app_loading'))
            .fontSize(16)
            .fontColor('#666666')
        }
        .width('100%')
        .height('100%')
        .backgroundColor('#F5F5F5')
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
        .zIndex(10)
        .hitTestBehavior(HitTestMode.Transparent)
      }

      // Scrim while a menu is open: covers everything below the menu bar,
      // closes the menu when tapped, and (being hit-test opaque) blocks
      // both ETS forwarding and native QPA delivery of that tap 鈥?desktop
      // menus dismiss on outside-click without acting on the content.
      // It starts at MENUBAR_HEIGHT_VP so the titles keep their toggle and
      // hover-switch behaviour, and is rendered BELOW DropdownMenu so the
      // submenu items stay clickable.  Close happens in onClick (fires on
      // Up, after Stack.onTouch already saw openMenu!==null for both Down
      // and Up) so the dismissing tap is never forwarded to Qt.
      if (this.openMenu !== null) {
        Column()
          .width('100%')
          .height('calc(100% - 28vp)') // 28 = MENUBAR_HEIGHT_VP
          .position({ x: 0, y: MENUBAR_HEIGHT_VP })
          .backgroundColor(Color.Transparent)
          .onClick(() => {
            hilog.info(DOMAIN, TAG, 'scrim tap - closing menu');
            this.openMenu = null;
          })
          .zIndex(100) // above floating overlays
      }

      // Floating dropdown menu: rendered at Stack level (NOT inside the
      // Column) so opening it does not reflow/push the content area down.
      // .position() is applied HERE (Stack child) so x/y are relative to
      // the Stack's top-left, anchoring the panel under its menu title.
      // zIndex keeps it above the Project sidebar, which sits later in the
      // Stack and otherwise covers the File/Edit/View dropdowns.
      DropdownMenu({
        openMenu: this.openMenu,
        menus: this.menuDefs,
        onItemClick: (itemId: string) => this.onMenuItemClick(itemId)
      })
        .position({ x: this.dropdownX, y: MENUBAR_HEIGHT_VP })
        .zIndex(101)

      // Right-click context menu: floating popup positioned at the click
      // coordinates.  Always rendered (like DropdownMenu) so the @Link
      // isVisible drives its internal show/hide.  zIndex above the sidebar
      // and regular dropdown so menu items stay clickable.
      ContextMenu({
        isVisible: $showContextMenu,
        positionX: this.contextMenuX,
        positionY: this.contextMenuY,
        items: this.buildContextMenuItems(),
        onItemClick: (itemId: string) => this.onMenuItemClick(itemId)
      })
        .zIndex(200)

      // Custom tooltip overlay for ToolBar and BottomToolBar buttons.
      // bindPopup is occluded by the XComponent (native rendering layer);
      // this Text sits at Stack level with zIndex 300 so it renders above
      // XComponent.  Position (x/y) is supplied by the caller via
      // handleHoverTip: top toolbar → below button (btnY+30), bottom
      // toolbar → above button (btnY-34).
      if (this.tooltipShow && this.tooltipText) {
        Text(this.tooltipText)
          .fontSize(12)
          .fontColor('#333333')
          .padding({ left: 8, right: 8, top: 4, bottom: 4 })
          .backgroundColor('#FFFFE1')
          .borderRadius(4)
          .border({ width: 1, color: '#C0C0C0' })
          .position({ x: this.tooltipX, y: this.tooltipY })
          .zIndex(300)
          .hitTestBehavior(HitTestMode.Transparent)
      }

      // Column-properties sidebar is intentionally NOT mounted: the user
      // wants the Qt-native table column control panel (TableView
      // d_control_tabs) rendered by Qt itself, like the matrix one.

      // Dialogs

      // 2026-08-14 工作文件夹项目选择（Open 直通）: 授权工作文件夹内的
      // .sciprj 列表，点选 → openProjectFromWorkFolder 直通（无拷贝）。
      if (this.showWorkFolderOpenDialog) {
        Stack() {
          Column()
            .width('100%')
            .height('100%')
            .backgroundColor(BACKDROP_COLOR)
            .onClick(() => {
              this.showWorkFolderOpenDialog = false;
            })
          Column() {
            Text(getAppString('dlg_open_project'))
              .fontSize(FS_DIALOG_TITLE)
              .fontWeight(FontWeight.Bold)
              .fontColor(TEXT)
              .margin({ top: SPACING_16, bottom: SPACING_12 })
            if (this.workFolderFiles.length > 0) {
              Scroll() {
                Column() {
                  ForEach(this.workFolderFiles, (fname: string) => {
                    Text(fname)
                      .fontSize(FS_DIALOG_SMALL)
                      .fontColor(TEXT_LIST)
                      .padding({ top: SPACING_4, bottom: SPACING_4, left: SPACING_6 })
                      .width('100%')
                      .onClick(() => this.openProjectFromWorkFolder(fname))
                  }, (fname: string) => fname)
                }
              }
              .layoutWeight(1)
              .width('100%')
              .border({ width: 1, color: '#E0E0E0' })
            } else {
              Text(getAppString('common_no_files'))
                .fontSize(FS_DIALOG_SMALL)
                .fontColor(TEXT_EMPTY)
            }
            Row() {
              Button(getAppString('common_cancel'))
                .fontSize(FS_DIALOG_LABEL)
                .height(BTN_HEIGHT)
                .backgroundColor(BTN_BG)
                .fontColor(BTN_TEXT)
                .borderRadius(RADIUS_4)
                .onClick(() => {
                  this.showWorkFolderOpenDialog = false;
                })
            }
            .width('100%')
            .padding({ left: SPACING_16, right: SPACING_16, bottom: SPACING_16 })
            .justifyContent(FlexAlign.End)
          }
          .width(420)
          .constraintSize({ maxHeight: '80%' })
          .backgroundColor('#FFFFFF')
          .borderRadius(RADIUS_8)
          .shadow({ radius: SHADOW_RADIUS, color: SHADOW_COLOR, offsetX: SHADOW_OFFSET_X, offsetY: SHADOW_OFFSET_Y })
        }
        .width('100%')
        .height('100%')
      }

      // Batch-4 UI dialogs
      LayerDialog({ isVisible: $showLayerDialog })
      // Batch-5 dialogs

      // Stage-6 dialogs: curve context-menu fallbacks.  The C++ menuAction
      // branches 'edit_range'/'associations'（另一代理添加中）落地后走 Qt
      // 内嵌对话框，这两个 ArkTS 对话框保留为回退路径。
      CurveRangeDialog({ isVisible: $showCurveRangeDialog })
      AssociationsDialog({ isVisible: $showAssociationsDialog })

      // Qt backend event host: renders ArkUI replacements for QMessageBox /
      // context menus / combo popups pushed via scidavis_emit → onQtEvent.
      // Fully hit-test transparent while no overlay is visible.
      // Bug 2: onWindowActivated is a direct parent-child callback so a
      // windowActivated event reliably triggers the menu-bar rebuild
      // (@StorageProp+@Watch across components is unreliable on device).
      QtEventHost({
        onWindowActivated: () => this.onActiveWindowChanged(),
        onContextMenuRequest: (widgetClass: string, markerType?: string) =>
          this.handleContextMenuRequest(widgetClass, markerType),
        onRequestSaveBeforeQuit: () => this.handleQtRequestSaveBeforeQuit(),
        onRequestSaveBeforeNewProject: () => this.handleQtRequestSaveBeforeNewProject(),
        onAfterSaveNewProject: () => this.handleQtAfterSaveNewProject()
      })

      // No external overlay: touching the content area closes the menu
      // and forwards events to Qt. The dropdown menu area is handled
      // directly by the MenuBar component.
    }
    .width('100%')
    .height('100%')
    // Transparent normally (touches fall through to the XComponent for
    // native QPA delivery).  While a menu is open, switch to Default so
    // the dismissing tap stays inside ArkTS 鈥?the scrim alone cannot stop
    // the native pipeline because an opaque child does not override the
    // outer Stack's Transparent mode (device-verified: src=2 press reached
    // Qt at the dismiss tap position).
    .hitTestBehavior(this.openMenu !== null ? HitTestMode.Default : HitTestMode.Transparent)
    // Forward touch events (finger + Bluetooth-left-converted-to-touch)
    // via sendMouse.  ETS CANNOT tell them apart: both report
    // source=TouchScreen, sourceTool=Finger (device-verified).  Finger
    // touches are also delivered natively by the QPA plugin, so Qt would
    // see them twice 鈥?the Qt side (InputProbe) drops the duplicate by
    // matching type+pos+button across different event sources within 60ms.
    // Right/middle Bluetooth clicks stay as MouseEvent and go via onMouse.
    // COORDINATES: ETS delivers vp; Qt runs with HighDpi scaling disabled
    // (DPR=1) and therefore expects PHYSICAL pixels 鈥?convert with vp2px()
    // before forwarding (DEVELOPMENT_GUIDE 搂1.3, device 2456x1600, ~1.5x).
    .onTouch((event: TouchEvent) => {
      // Never forward while an ArkTS menu is open (dropdown interactions),
      // nor within the ArkTS menu-bar / status-bar bands: the Qt menu bar
      // sits underneath and a forwarded tap there opens a QMenu popup,
      // which the single-window QPA cannot create (SIGSEGV).
      if (this.openMenu !== null) { return; }
      // Combo picker overlay up (QtEventHost raised comboPickerActive): the
      // scrim consumes touches, but a Bluetooth-mouse left press can still
      // arrive on the touch channel — hold it back too (issue "dropdown
      // misbehaves", 2026-08-14).  This is the DEDICATED picker flag, NOT
      // 'modalActive': the latter is raised by modalStarted (embedded
      // dialogs like Preferences) and must NOT block forwarding — the Qt
      // dialog underneath needs every click (2026-08-14 regression: dialog
      // was completely unresponsive while open).
      if (AppStorage.get<boolean>('comboPickerActive') === true) { return; }
      const stackHeight: number = event.target.area.height as number;
      const changed: TouchObject[] = event.changedTouches;
      for (let i = 0; i < changed.length; i++) {
        const t: TouchObject = changed[i];
        let action: number = -1;
        if (t.type === TouchType.Down) {
          // Band guard applies to Down only; Move/Up follow leftDown so a
          // drag that ends inside a chrome band still delivers its release
          // (otherwise Qt is left with a stuck pressed button).
          if (t.y < this.topChromeVp() || t.y > stackHeight - STATUSBAR_HEIGHT_VP) {
            continue; // ArkTS chrome band - handled natively, not Qt's
          }
          // Record long-press start position and time
          this.lastTouchX = t.x;
          this.lastTouchY = t.y;
          this.touchStartTime = Date.now();
          // Issue list #6 (2026-08-14): input-source split for long-press
          // = right-click.  The source is captured here (finger touches
          // report TouchScreen; Bluetooth-mouse-converted touches report
          // MOUSE even inside onTouch) and the Up-branch only fires the
          // context menu for TouchScreen — mouse left long-press must NOT
          // count as right-click (a physical right button exists).
          this.longPressSource = event.source;
          hilog.info(DOMAIN, TAG,
            'touch start recorded for long-press at (%{public}d,%{public}d) source=%{public}d',
            Math.round(t.x), Math.round(t.y), event.source);
          action = 0; leftDown = true;
        } else if (t.type === TouchType.Move) {
          // Forwarded since 2026-08-04 (Bluetooth-mouse drag fix): ArkUI
          // re-routes Bluetooth LEFT drags into the touch pipeline marked as
          // a MOUSE source, and the QPA plugin drops MOUSE-sourced touch
          // moves, so a Bluetooth drag would otherwise never reach Qt.
          // Finger drags ARE also delivered natively; the Qt InputProbe
          // dedups Move across event sources (60ms + manhattan<5), so the
          // forwarded copy no longer double-delivers nor lags graph drag.
          // Only forward while a button is held (leftDown) so hover motion
          // with no button pressed is never treated as a drag.
          if (!leftDown) { continue; }
          action = 2; // keep leftDown unchanged
        } else if (t.type === TouchType.Up) {
          if (!leftDown) { continue; }
          // Long-press detection: check if held long enough with minimal
          // movement AND the press came from a real finger (source
          // TouchScreen).  Issue list #6 (2026-08-14): a Bluetooth mouse
          // left long-press must NOT open the context menu — a physical
          // right button exists — so MOUSE-sourced touches are excluded
          // here.  (Per the issue: if the sources could not be told apart
          // this feature would be removed entirely; they can, via
          // event.source, so the split is implemented.)
          // Issue list #9 (2026-08-14): long-press-to-right-click in the
          // Qt-rendered content area is disabled — the ArkTS context menu
          // must only wake inside the toolbar bands.  The press tracking
          // above is kept so the gesture metrics remain diagnostic.
          if (event.source === SourceType.TouchScreen &&
              (Date.now() - this.touchStartTime) >= this.LONG_PRESS_DURATION_MS) {
            hilog.info(DOMAIN, TAG,
              'long-press swallowed (Qt surface, issue #9) at (%{public}d,%{public}d)',
              Math.round(this.lastTouchX), Math.round(this.lastTouchY));
          }
          action = 1; leftDown = false;
        }
        // TouchType.Move IS forwarded since 2026-08-04 (Bluetooth-mouse drag
        // fix): it is required for Bluetooth LEFT drags, whose moves the QPA
        // plugin discards.  Finger moves are also delivered natively, but the
        // Qt InputProbe dedups Move across sources (60ms + manhattan<5), so
        // the forwarded copy no longer double-delivers every move to Qt and
        // no longer lags graph drag/zoom (previous Bug 3).  Down (0), Move
        // (2) and Up (1) are all forwarded via sendMouse; leftDown tracks the
        // press state for the guards above.
        // Cross-channel left-press dedup (Oracle 2026-08-06): the same physical
        // left click can arrive here AND in onMouse, so without a release in
        // between Qt would synthesize a DblClick on a single click.  A real
        // double-click always has a release between the two presses.
        if (action === 0 && !leftReleaseSeen
            && Math.abs(t.x - lastLeftPressX) < 15 && Math.abs(t.y - lastLeftPressY) < 15
            && (Date.now() - lastLeftPressTime) < 400) {
          continue; // skip forwarding this duplicate press
        }
        if (action === 0) {
          lastLeftPressTime = Date.now();
          lastLeftPressX = t.x;
          lastLeftPressY = t.y;
          leftReleaseSeen = false;
        } else if (action === 1) {
          leftReleaseSeen = true;
        }
        if (action >= 0) {
          const px: number = vp2px(t.x);
          // NO decor compensation: Qt window coords == content-area coords
          // (device-verified winPos=0,0 / mdiGlobal=0,111 in the DIAG log) —
          // the overlay Stack and the QPA window share the same origin, so
          // the raw content-area coordinate is exactly what Qt expects.
          const py: number = vp2px(t.y);
          this.forwardMouse(px, py, 1, action);
        }
      }
    })
    // Forward ALL mouse buttons (Left/Right/Middle).  Since easy_go
    // mouse2TouchEventMode=disabled (2026-08-06 mouse-wheel fix), OHOS no
    // longer converts ANY mouse event to touch, so LEFT must also be
    // forwarded here (previously it arrived as a touch event handled in
    // onTouch above and was skipped here to avoid duplicates).
    .onMouse((event: MouseEvent) => {
      // Right-click in the content area: record the position ONLY — the
      // menu itself is driven by the Qt contextMenuRequest event (the C++
      // InputProbe blocks the native QMenu popup and reports the widget
      // class under the cursor; ArkTS then renders the matching menu).
      // Directly showing a placeholder menu here produced a wrong/blank
      // popup (issue list #2a, fixed 2026-08-14): the Qt event decides
      // both content and visibility, and it always arrives because the
      // forwarded right-press reaches Qt first.
      if (event.button === MouseButton.Right && event.action === MouseAction.Press) {
        const sh: number = event.target.area.height as number;
        if (event.y >= this.topChromeVp() && event.y <= sh - STATUSBAR_HEIGHT_VP) {
          this.contextMenuX = event.x;
          this.contextMenuY = event.y;
        }
      }
      // Same chrome-band guard as onTouch: keep forwarded CLICKS out of
      // the Qt menu bar underneath the ArkTS one (popup crash).  Hover
      // moves (no button) MUST still be forwarded everywhere — Qt's
      // QMdiSubWindow updates its resize cursor (SizeVer/SizeHor/...)
      // from the incoming move stream, and when the cursor leaves a
      // sub-window edge the next forwarded move resets it to the default
      // arrow.  Blocking moves in the chrome band left the cursor stuck
      // on the last edge arrow (issue-list #6 follow-up, 2026-08-14).
      if (this.openMenu !== null) return;
      // Combo picker overlay is up (QtEventHost raised comboPickerActive):
      // hold mouse CLICKS back so they cannot reach the Qt widget
      // underneath (would re-open the picker / hit another control — the
      // "dropdown misbehaves" issue, 2026-08-14).  Hover moves still
      // forward so the cursor stays responsive.  Only the picker flag
      // gates input: 'modalActive' (modalStarted, embedded dialogs) must
      // keep letting clicks through to the Qt dialog (2026-08-14
      // regression: Preferences unresponsive while open).
      if (AppStorage.get<boolean>('comboPickerActive') === true
        && event.action !== MouseAction.Move) {
        return;
      }
      const stackHeight: number = event.target.area.height as number;
      const inChromeBand: boolean =
        event.y < this.topChromeVp() || event.y > stackHeight - STATUSBAR_HEIGHT_VP;
      if (inChromeBand && event.action !== MouseAction.Move) {
        return;
      }
      let btn: number = 0;
      if (event.button === MouseButton.Left) btn = 1;
      else if (event.button === MouseButton.Right) btn = 2;
      else if (event.button === MouseButton.Middle) btn = 3;
      // 悬停移动（无按钮的 MouseAction.Move）：必须转发到 Qt —— 否则
      // QMdiSubWindow 的 hover 分支永不执行，窗口边缘的光标形状
      // （SizeHor/SizeVer/SizeFDiag 双箭头）不显示、按下后也不恢复
      // （2026-08-13 光标修复）。Qt 侧靠注入流的 press/release 状态
      // 区分悬停与拖动（action=2 时忽略 button 参数），btn 传 1 无影响。
      else if (event.action !== MouseAction.Move) return;
      else btn = 1;
      let act: number = -1;
      if (event.action === MouseAction.Press) act = 0;
      else if (event.action === MouseAction.Release) act = 1;
      else if (event.action === MouseAction.Move) act = 2;
      else return;
      // Cross-channel left-press dedup (Oracle 2026-08-06): the same physical
      // left click can arrive here AND in onTouch Down; drop the duplicate so
      // Qt does not synthesize a DblClick on a single click.  Move (act=2)
      // never touches lastLeftPress* / leftReleaseSeen.
      if (act === 0 && btn === 1 && !leftReleaseSeen
          && Math.abs(event.x - lastLeftPressX) < 15 && Math.abs(event.y - lastLeftPressY) < 15
          && (Date.now() - lastLeftPressTime) < 400) {
        return;
      }
      if (act === 0 && btn === 1) {
        lastLeftPressTime = Date.now();
        lastLeftPressX = event.x;
        lastLeftPressY = event.y;
        leftReleaseSeen = false;
      } else if (act === 1 && btn === 1) {
        leftReleaseSeen = true;
      }
      const px: number = vp2px(event.x);
      // No decor compensation — Qt window coords == overlay content coords.
      const py: number = vp2px(event.y);
      this.forwardMouse(px, py, btn, act);
    })
  }
}
