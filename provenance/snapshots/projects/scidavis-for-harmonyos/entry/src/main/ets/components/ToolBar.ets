// OHPlot toolbar — original SciDAVis desktop icons (converted from
// scidavis/icons *.xpm and icons/light/24 *.svg), replicating the Windows
// toolbar categories (ApplicationWindow::initToolBars): File / Edit /
// Graph / Plot / Plot3D groups.  Ids reuse the menu dispatch.
//
// Tablet layout (task: toolbar regroup + occlusion fix):
// - Groups sit in a Flex(wrap) container, so rows wrap instead of
//   overflowing the screen width (the old single-row Scroll cut off the
//   Plot3D tail on the tablet).
// - Context groups follow the desktop enable logic but HIDE instead of
//   grey to save width: Plot + Plot3D groups only with an active Table.
//   Graph group is ALWAYS visible (Windows parity); its buttons grey out
//   (opacity) when no 2D plot is active.  File/Edit are always shown.
// - The container height is dynamic (1..n rows); MainLayout measures it
//   via onAreaChange and forwards the chrome inset to Qt
//   (setChromeInsets) so the MDI area is never occluded.
// - Buttons carry component ids (tb_* / tbdrop_*) so verify scripts can
//   locate them through `uitest dumpLayout` instead of fixed coordinates.
//
// Buttons are emitted directly (no ForEach) so @Prop changes always
// refresh the greyed state (past ForEach-key defect on device).
//
// The 2D-plot (plot_tools) and 3D-surface groups now live in a separate
// bottom bar (BottomToolBar.ets); this bar keeps File / Edit / Graph.
import * as Theme from './Theme';

// @Builder by-reference parameter shapes: passing one object literal keeps
// the enabled flags reactive (see the Builders note inside ToolBar).
interface TbButtonOpts {
  id: string;
  icon: Resource;
  enabled: boolean;
  tip: string;   // resource name of the hover tooltip (Windows-style hint)
}

interface TbDropOpts {
  id: string;
  icon: Resource;
  enabled: boolean;
  menu: MenuElement[];
  tip: string;
}

@Component
export struct ToolBar {
  @Prop undoEnabled: boolean = false;
  @Prop redoEnabled: boolean = false;
  @Prop hasWin: boolean = false;      // an MDI window is active
  @Prop isTable: boolean = false;     // active window is a Table
  @Prop isGraph: boolean = false;     // active window is a 2D plot (MultiLayer)
  // Visibility toggles (driven by MainLayout's blank-area context menu):
  // each group can be hidden/shown independently; default true keeps the
  // original layout when no flags are passed.
  @Prop showFile: boolean = true;
  @Prop showEdit: boolean = true;
  @Prop showGraph: boolean = true;
  onItemClick?: (itemId: string) => void;
  // Right-click callback (问题4b): toolbar buttons only carried
  // onClick/onHover, so a right press had no handler anywhere.  The root
  // Flex now forwards Right+Press here in WINDOW coordinates (MouseEvent
  // windowX/windowY = app-window top-left, the same space MainLayout's
  // Stack-relative contextMenuX/Y live in), so MainLayout can pop the
  // existing ContextMenu at the real click position.
  onContextMenu?: (x: number, y: number) => void;
  // Callback to show/hide a custom tooltip overlay in MainLayout (rendered
  // at Stack level with zIndex 300, same style as BottomToolBar).  tipKey
  // is the string resource name; x/y is the global position for the
  // overlay; show=true → display, show=false → hide.
  onHoverTip?: (tipKey: string, x: number, y: number, show: boolean) => void;
  // Id of the button currently hovered by the mouse; used internally.
  @State hoverTipId: string = '';
  // Id of the button currently hovered (mouse over) — drives the light-blue
  // background feedback (Theme.SELECTED_BG) while the pointer rests on it.
  @State hoveredId: string = '';
  // Id of the button currently pressed (touch down) — keeps the light-blue
  // background while the finger is held on the button.
  @State pressedId: string = '';
  // Per-button global position [x, y], captured via onAreaChange.
  @State btnPos: Record<string, [number, number]> = {};

  private fire(itemId: string): void {
    if (this.onItemClick) {
      this.onItemClick(itemId);
    }
  }



  // ── Dropdown menus (Windows QToolButton InstantPopup equivalents) ──

  // File ▸ New Aspect (new_aspect.xpm button).  Icons per the authoritative
  // Win menu (newF.xpm / newFxy.xpm); ids renamed to the File ▸ New menu ids
  // (issue list #8) so toolbar clicks share the menu dispatch entry.
  private newAspectMenu(): MenuElement[] {
    const items: MenuElement[] = [
      { value: $r('app.string.tbmenu_new_table'), icon: $r('app.media.tb_table'), action: (): void => this.fire('new_table') },
      { value: $r('app.string.tbmenu_new_matrix'), icon: $r('app.media.tb_new_matrix'), action: (): void => this.fire('new_matrix') },
      { value: $r('app.string.tbmenu_new_note'), icon: $r('app.media.tb_new_note'), action: (): void => this.fire('new_notes') },
      { value: $r('app.string.tbmenu_new_graph'), icon: $r('app.media.tb_new_graph'), action: (): void => this.fire('new_graph') },
      { value: $r('app.string.tbmenu_new_function_plot'), icon: $r('app.media.mn_newF'), action: (): void => this.fire('new_function_plot') },
      { value: $r('app.string.tbmenu_new_3d_surface'), icon: $r('app.media.mn_newFxy'), action: (): void => this.fire('new_3d_surface') }
    ];
    return items;
  }

  // Graph ▸ Manage layers (arrangeLayers.xpm button).  Icons per the Win
  // toolbar (auto_layout/newLayer/erase/arrangeLayers.xpm); 'delete_layer'
  // renamed to the Graph ▸ Remove Layer menu id 'remove_layer' (issue #8).
  private layersMenu(): MenuElement[] {
    const items: MenuElement[] = [
      { value: $r('app.string.tbmenu_auto_layout'), icon: $r('app.media.mn_auto_layout'), action: (): void => this.fire('automatic_layout') },
      { value: $r('app.string.tbmenu_add_layer'), icon: $r('app.media.mn_layer_enabled'), action: (): void => this.fire('add_layer') },
      { value: $r('app.string.tbmenu_remove_layer'), icon: $r('app.media.mn_layer_disabled'), action: (): void => this.fire('remove_layer') },
      { value: $r('app.string.tbmenu_arrange_layers'), icon: $r('app.media.tb_arrangelayers'), action: (): void => this.fire('arrange_layers') }
    ];
    return items;
  }

  // Graph ▸ Add curves / error bars (curves.xpm button).  Icons per the Win
  // menu (curves/yerror/fx.xpm).
  private curvesMenu(): MenuElement[] {
    const items: MenuElement[] = [
      { value: $r('app.string.tbmenu_add_remove_curve'), icon: $r('app.media.tb_curves'), action: (): void => this.fire('add_curve') },
      { value: $r('app.string.tbmenu_add_error_bars'), icon: $r('app.media.mn_yerror'), action: (): void => this.fire('add_error_bars') },
      { value: $r('app.string.tbmenu_add_function'), icon: $r('app.media.mn_fx'), action: (): void => this.fire('add_function') }
    ];
    return items;
  }

  // Graph ▸ Enrichments (text.xpm button).  Icons per the Win menu
  // (text/arrow/lPlot/clock/monalisa/legend.xpm).
  private enrichmentsMenu(): MenuElement[] {
    const items: MenuElement[] = [
      { value: $r('app.string.tbmenu_add_text'), icon: $r('app.media.tb_text'), action: (): void => this.fire('add_text') },
      { value: $r('app.string.tbmenu_draw_arrow'), icon: $r('app.media.mn_arrow'), action: (): void => this.fire('draw_arrow') },
      { value: $r('app.string.tbmenu_draw_line'), icon: $r('app.media.tb_lplot'), action: (): void => this.fire('draw_line') },
      { value: $r('app.string.tbmenu_add_timestamp'), icon: $r('app.media.mn_clock'), action: (): void => this.fire('add_time_stamp') },
      { value: $r('app.string.tbmenu_add_image'), icon: $r('app.media.mn_monalisa'), action: (): void => this.fire('add_image') },
      { value: $r('app.string.tbmenu_new_legend'), icon: $r('app.media.mn_legend'), action: (): void => this.fire('new_legend') }
    ];
    return items;
  }

  // ── Builders ──
  // Device-verified defect: @Builder value-parameters are frozen at first
  // render (ArkTS by-value passing is not reactive), which left Edit/Plot
  // buttons permanently greyed after uiState refreshes.  Parameters are
  // therefore passed as a single object literal — the documented
  // by-reference form that keeps @Prop-driven greying live.

  @Builder
  ToolButton($$: TbButtonOpts) {
    Column() {
      Image($$.icon)
        .width(22)
        .height(22)
        .objectFit(ImageFit.Contain)
        .draggable(false)
    }
    .width(30)
    .height(28)
    .justifyContent(FlexAlign.End)
    .borderRadius(4)
    .backgroundColor($$.id === this.hoveredId || $$.id === this.pressedId ? Theme.SELECTED_BG : Color.Transparent)
    .opacity($$.enabled ? 1.0 : Theme.OPACITY_DISABLED)
    .enabled($$.enabled)
    .id('tb_' + $$.id)
    .onAreaChange((_o: Area, n: Area) => {
      if (n.globalPosition) {
        this.btnPos[$$.id] = [n.globalPosition.x as number, n.globalPosition.y as number];
      }
    })
    .onHover((isHover: boolean) => {
      this.hoveredId = isHover ? $$.id : (this.hoveredId === $$.id ? '' : this.hoveredId);
      if (isHover) {
        this.hoverTipId = $$.id;
        const p = this.btnPos[$$.id];
        if (this.onHoverTip && p) { this.onHoverTip($$.tip, p[0], p[1] + 30, true); }
      } else if (this.hoverTipId === $$.id) {
        this.hoverTipId = '';
        if (this.onHoverTip) { this.onHoverTip('', 0, 0, false); }
      }
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedId = $$.id;
      } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        if (this.pressedId === $$.id) {
          this.pressedId = '';
        }
      }
    })
    .onClick(() => {
      if (!$$.enabled) {
        return;
      }
      this.fire($$.id);
    })
  }

  @Builder
  DropButton($$: TbDropOpts) {
    // 2026-08-15 follow-up: the bottom-right down-arrow overlay (issue #7)
    // was reverted — the 8px glyph was too small and shifted the row layout
    // (toolbar rows no longer aligned).  Keep the plain Column button so the
    // two toolbar rows stay aligned on the same baseline.
    Column() {
      Image($$.icon)
        .width(22)
        .height(22)
        .objectFit(ImageFit.Contain)
        .draggable(false)
    }
    .width(30)
    .height(28)
    .justifyContent(FlexAlign.End)
    .borderRadius(4)
    .backgroundColor($$.id === this.hoveredId || $$.id === this.pressedId ? Theme.SELECTED_BG : Color.Transparent)
    .opacity($$.enabled ? 1.0 : Theme.OPACITY_DISABLED)
    .enabled($$.enabled)
    .id($$.id)
    .onAreaChange((_o: Area, n: Area) => {
      if (n.globalPosition) {
        this.btnPos[$$.id] = [n.globalPosition.x as number, n.globalPosition.y as number];
      }
    })
    .onHover((isHover: boolean) => {
      this.hoveredId = isHover ? $$.id : (this.hoveredId === $$.id ? '' : this.hoveredId);
      if (isHover) {
        this.hoverTipId = $$.id;
        const p = this.btnPos[$$.id];
        if (this.onHoverTip && p) { this.onHoverTip($$.tip, p[0], p[1] + 30, true); }
      } else if (this.hoverTipId === $$.id) {
        this.hoverTipId = '';
        if (this.onHoverTip) { this.onHoverTip('', 0, 0, false); }
      }
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedId = $$.id;
      } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        if (this.pressedId === $$.id) {
          this.pressedId = '';
        }
      }
    })
    .bindMenu($$.menu)
  }

  @Builder
  Separator() {
    Row()
      .width(1)
      .height(20)
      .backgroundColor(Theme.LINE)
      .margin({ left: 4, right: 4 })
  }

  // QToolBar-style grip drawn at the left edge of every group: marks the
  // groups as independent toolbars (visual parity with the Windows build).
  @Builder
  Grip() {
    Row() {
      Row().width(1).height(16).backgroundColor(Theme.GRIP)
      Row().width(1).height(16).backgroundColor(Theme.GRIP).margin({ left: 2 })
    }
    .margin({ left: 3, right: 5 })
  }

  // ── Toolbar groups (one per desktop QToolBar) ──
  // Each group is a bordered rounded Row = one Windows toolbar segment.
  // The shared container attributes are repeated per builder because
  // @Builder functions cannot take content closures.

  // file_tools (always visible)
  @Builder
  FileGroup() {
    Row() {
      this.Grip()
      this.ToolButton({ id: 'new_project', icon: $r('app.media.tb_new'), enabled: true, tip: 'tip_new_project' })
      this.DropButton({ id: 'tbdrop_new_aspect', icon: $r('app.media.tb_new_aspect'), enabled: true, menu: this.newAspectMenu(), tip: 'tip_new_aspect' })
      this.ToolButton({ id: 'open', icon: $r('app.media.tb_fileopen'), enabled: true, tip: 'tip_open' })
      this.ToolButton({ id: 'open_template', icon: $r('app.media.tb_open_template'), enabled: true, tip: 'tip_open_template' })
      this.ToolButton({ id: 'import', icon: $r('app.media.tb_import'), enabled: true, tip: 'tip_import' })
      this.ToolButton({ id: 'save', icon: $r('app.media.tb_filesave'), enabled: true, tip: 'tip_save' })
      // id 'save_as_template' matches the File ▸ Save As Template menu id
      // (issue list #8).
      this.ToolButton({ id: 'save_as_template', icon: $r('app.media.tb_save_template'), enabled: true, tip: 'tip_save_template' })
      this.Separator()
      this.ToolButton({ id: 'print', icon: $r('app.media.tb_fileprint'), enabled: false, tip: 'tip_print' })
      // Stage-2C (2026-08-16): export_pdf 临时启用（PDF 探针）；真机失败回退置灰。
      this.ToolButton({ id: 'export_pdf', icon: $r('app.media.tb_pdf'), enabled: true, tip: 'tip_export_pdf' })
      this.Separator()
      this.ToolButton({ id: 'project_browser', icon: $r('app.media.tb_folder'), enabled: true, tip: 'tip_project_browser' })
      this.ToolButton({ id: 'log', icon: $r('app.media.tb_log'), enabled: true, tip: 'tip_log' })
    }
    .height(30)
    .padding({ right: 3 })
    .margin(2)
    .borderRadius(4)
    .border({ width: 1, color: Theme.GROUP_BORDER })
    .backgroundColor(Theme.GROUP_BG)
  }

  // edit_tools (always visible; greying follows uiState)
  @Builder
  EditGroup() {
    Row() {
      this.Grip()
      this.ToolButton({ id: 'undo', icon: $r('app.media.tb_edit_undo'), enabled: this.undoEnabled, tip: 'tip_undo' })
      this.ToolButton({ id: 'redo', icon: $r('app.media.tb_edit_redo'), enabled: this.redoEnabled, tip: 'tip_redo' })
      this.ToolButton({ id: 'cut', icon: $r('app.media.tb_edit_cut'), enabled: this.hasWin, tip: 'tip_cut' })
      this.ToolButton({ id: 'copy', icon: $r('app.media.tb_edit_copy'), enabled: this.hasWin, tip: 'tip_copy' })
      this.ToolButton({ id: 'paste', icon: $r('app.media.tb_edit_paste'), enabled: this.hasWin, tip: 'tip_paste' })
      this.ToolButton({ id: 'delete', icon: $r('app.media.tb_erase'), enabled: this.hasWin, tip: 'tip_delete' })
    }
    .height(30)
    .padding({ right: 3 })
    .margin(2)
    .borderRadius(4)
    .border({ width: 1, color: Theme.GROUP_BORDER })
    .backgroundColor(Theme.GROUP_BG)
  }

  // graph_tools (always visible, Windows parity; per-button greying
  // follows the active-window state — no group-level show/hide)
  @Builder
  GraphGroup() {
    Row() {
      this.Grip()
      this.ToolButton({ id: 'disable_tools', icon: $r('app.media.tb_pointer'), enabled: this.isGraph, tip: 'tip_graph_pointer' })
      this.Separator()
      this.DropButton({ id: 'tbdrop_layers', icon: $r('app.media.tb_arrangelayers'), enabled: this.isGraph, menu: this.layersMenu(), tip: 'tip_layers' })
      this.DropButton({ id: 'tbdrop_curves', icon: $r('app.media.tb_curves'), enabled: this.isGraph, menu: this.curvesMenu(), tip: 'tip_curves' })
      this.DropButton({ id: 'tbdrop_enrichments', icon: $r('app.media.tb_text'), enabled: this.isGraph, menu: this.enrichmentsMenu(), tip: 'tip_enrichments' })
      this.Separator()
      this.ToolButton({ id: 'zoom_in', icon: $r('app.media.tb_zoom'), enabled: this.isGraph, tip: 'tip_zoom_in' })
      this.ToolButton({ id: 'zoom_out', icon: $r('app.media.tb_zoomout'), enabled: this.isGraph, tip: 'tip_zoom_out' })
      this.ToolButton({ id: 'rescale', icon: $r('app.media.tb_unzoom'), enabled: this.isGraph, tip: 'tip_rescale' })
      this.Separator()
      this.ToolButton({ id: 'screen_reader', icon: $r('app.media.tb_cursor_16'), enabled: this.isGraph, tip: 'tip_screen_reader' })
      this.ToolButton({ id: 'data_reader', icon: $r('app.media.tb_select'), enabled: this.isGraph, tip: 'tip_data_reader' })
      // id 'range' matches the Tools ▸ Select Data Range menu id (issue #8).
      this.ToolButton({ id: 'range', icon: $r('app.media.tb_cursors'), enabled: this.isGraph, tip: 'tip_select_range' })
    }
    .height(30)
    .padding({ right: 3 })
    .margin(2)
    .borderRadius(4)
    .border({ width: 1, color: Theme.GROUP_BORDER })
    .backgroundColor(Theme.GROUP_BG)
  }

  build() {
    // Flex(wrap) = OHOS stand-in for QMainWindow's toolbar rows: groups
    // that do not fit the tablet width wrap onto the next row instead of
    // being clipped (the old single-row Scroll hid the Plot3D tail).
    // Height is content-driven; MainLayout measures it via onAreaChange.
    // Each group is gated by its show* @Prop (context-menu visibility);
    // hidden groups are not rendered at all, so the measured height drops.
    Flex({ wrap: FlexWrap.Wrap, alignItems: ItemAlign.Center }) {
      if (this.showFile) {
        this.FileGroup()
      }
      if (this.showEdit) {
        this.EditGroup()
      }
      if (this.showGraph) {
        this.GraphGroup()
      }
    }
    .width('100%')
    .padding({ left: 2, top: 1, bottom: 1 })
    .backgroundColor(Theme.CHROME_BG)
    .border({ width: { bottom: 1 }, color: Theme.LINE })
    // 问题4b: the toolbar buttons have no .onMouse, so right-click had no
    // handler.  Mouse events bubble child → root, so ONE handler here
    // covers every button (left clicks still go through the per-button
    // onClick).  windowX/windowY are app-window-relative, matching the
    // MainLayout context-menu coordinate space.
    .onMouse((event: MouseEvent) => {
      if (event.button === MouseButton.Right && event.action === MouseAction.Press) {
        if (this.onContextMenu) {
          this.onContextMenu(event.windowX, event.windowY);
        }
      }
    })
  }
}
