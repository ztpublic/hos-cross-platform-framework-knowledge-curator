/***************************************************************************
    File                 : ohos_bridge.cpp
    Project              : SciDAVis
    Description          : Qt → ArkTS event channel + dialog symbol
                           interposition for the OpenHarmony single-window
                           QPA (see ohos_bridge.h for rationale).

    HOW THE INTERPOSITION WORKS
    ---------------------------
    libscidavis.a is statically linked into libentry.so.  At static link
    time the linker prefers symbols *defined inside the link unit* over
    dynamic symbols exported by libQt5Widgets.so, so every call site in
    SciDAVis code binds to the definitions below instead of the real Qt
    implementations.  Vtable slots emitted in libentry.so (all SciDAVis
    dialog subclasses) bind the same way, and vtable slots inside
    libQt5Widgets.so are data relocations resolved through the dlopen
    lookup order (libentry.so first), so virtual dispatch is covered too.

    Every blocked popup is reported to ArkTS via scidavis_emit so the
    shell can render a native ArkUI replacement instead of failing
    silently.
 ***************************************************************************/
#include "ohos_bridge.h"

#ifdef SCIDAVIS_OHOS

#include <QApplication>
#include <QCursor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileDialog>
#include <QLineEdit>
#include <QFontDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRegion>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QWindow>
#include <QTimer>
#include <QToolButton>

#include <atomic>
#include <climits>
#include <cstdio>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hilog/log.h>

// stderr is a black hole on OHOS — route bridge diagnostics to hilog so
// interposer hits show up next to the ArkTS logs (tag SciDAVisBridge).
#define BRIDGE_LOG(fmt, ...)                                                                       \
    do {                                                                                           \
        OH_LOG_Print(LOG_APP, LOG_INFO, 0x0000, "SciDAVisBridge", fmt, ##__VA_ARGS__);             \
    } while (0)

// ── Event channel ────────────────────────────────────────────────────────

namespace {
std::mutex g_sinkMutex;
scidavis_event_sink_t g_sink = nullptr;
// Events emitted before the ArkTS sink registers (e.g. during Qt startup)
// are buffered here and flushed on registration.
std::deque<std::string> g_pending;
constexpr size_t kMaxPending = 64;
}

// ── Embedded-dialog modality state (see ohos_bridge.h) ───────────────────
// Stack of embedded dialogs currently running their manual modal loops.
// Nested dialogs (e.g. a color picker opened from an embedded dialog) push
// further entries; InputProbe (main.cpp) isolates against the TOP entry.
bool g_modalDialogActive = false;
std::vector<QWidget *> g_modalStack;
// P1 (2026-08-14): modalStarted emitted but modalEnded not yet — the ArkTS
// side pauses its getUiState polling on modalStarted and only resumes on
// modalEnded.  modalEnded is emitted on EVERY exit path, but the event
// channel is a nonblocking TSFN: if the queue is full the event is DROPPED
// (qohos.cpp EventSinkTrampoline frees the copy on napi_tsfn_nonblocking
// failure) and the shell would stay paused forever.  Every command handler
// in main.cpp flushes a pending modalEnded when the modal stack is empty,
// so a lost event self-heals on the next user interaction.
static bool g_modalStartedSent = false;

void scidavis_mark_modal_started()
{
    g_modalStartedSent = true;
}

void scidavis_mark_modal_ended()
{
    g_modalStartedSent = false;
}

// Called from main.cpp before running any queued command on the Qt thread:
// if a modal loop has fully exited (stack empty) but the modalEnded event
// may have been dropped by the TSFN queue, re-emit it so the ArkTS polling
// resumes.  No-op when no modal is pending or a modal is genuinely open.
void scidavis_flush_modal_ended()
{
    if (!g_modalStartedSent || g_modalDialogActive || !g_modalStack.empty())
        return;
    g_modalStartedSent = false;
    scidavisEmitEvent(QStringLiteral("modalEnded"));
}

// ── Embedded-dialog title bar ────────────────────────────────────────────
// Every embedded dialog reserves a 44px title bar across its top: the
// window title (left-aligned) + the manual close button (right-aligned),
// with the dialog content starting below it via contents margins.  44px
// fits the 32px-tall close button (placed at y=8) plus breathing room.
// Shared by the drag filter below (the title bar is the drag strip) and
// embedDialog (title-bar construction + height compensation).
constexpr int kTitleBarHeight = 44;

// ── Embedded-dialog drag ─────────────────────────────────────────────────
// The embedded ConfigDialog is a plain child widget (Qt::Widget): no OS
// chrome and no windowHandle(), so startSystemMove() is unavailable.  An
// event filter implements dragging manually in client coordinates via the
// title bar.  The filter is installed on BOTH the dialog and its title
// bar: the title bar is a QWidget child covering the top 44px, and as a
// widget it swallows every mouse event in that strip (events on child
// widgets never propagate to the parent) — a dialog-only filter would
// never see presses on the title-bar blank area, which made dragging dead.
// Events whose target is the dialog OR the title bar are handled; all
// other child widgets (buttons, list, tabs, the close button) pass through
// untouched.  Title-bar events are delivered in title-bar coordinates and
// are mapped into dialog coordinates before the region test below.
class DragResizeFilter : public QObject
{
public:
    explicit DragResizeFilter(QDialog *dlg, QWidget *titleBar = nullptr)
        : QObject(dlg), m_dlg(dlg), m_titleBar(titleBar)
    {
        // The title bar child widget intercepts all mouse events in the top
        // 44px strip, so dragging must be observed on it as well as on the
        // dialog; the filter forwards the events after coordinate mapping.
        if (m_titleBar)
            m_titleBar->installEventFilter(this);
    }

    // Wire the embedded title-bar chrome so the filter can keep it in sync
    // when the dialog re-grows / retitles on a language switch (2026-08-14).
    void setChrome(QLabel *titleLabel, QPushButton *closeBtn)
    {
        m_titleLabel = titleLabel;
        m_closeBtn = closeBtn;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj != m_dlg && obj != m_titleBar)
            return QObject::eventFilter(obj, ev);
        switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(ev);
            if (me->button() != Qt::LeftButton)
                break;
            // Title-bar events are delivered in title-bar coordinates; map
            // them into dialog coordinates so the region test below is the
            // single source of truth for both event sources.
            const QPoint p = (obj == m_titleBar)
                    ? m_titleBar->mapTo(m_dlg, me->pos())
                    : me->pos();
            // The title bar is the drag handle (its right edge is left of
            // the close button; events targeting the close button itself
            // never reach this filter — it is a child of the title bar, so
            // the filter never sees it as obj).
            if (p.y() < kTitleBarHeight && p.x() < m_dlg->width() - 44) {
                m_dragging = true;
                m_startGlobal = me->globalPos();
                m_startPos = m_dlg->pos();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(ev);
            if (m_dragging) {
                const QPoint d = me->globalPos() - m_startGlobal;
                m_dlg->move(m_startPos + d);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
            m_dragging = false;
            break;
        case QEvent::Resize:
            // The 14px rounded mask (set once in embedDialog) does not
            // follow widget resizes — a language switch that re-grows the
            // dialog (ConfigDialog::switchToLanguage) would clip the new
            // area to the old mask shape.  Re-apply it on every resize
            // (2026-08-14).
            if (m_dlg && m_dlg->isVisible()) {
                QPainterPath mask;
                mask.addRoundedRect(QRectF(m_dlg->rect()), 14, 14);
                m_dlg->setMask(QRegion(mask.toFillPolygon().toPolygon()));
            }
            // The title bar / label were sized for the OLD dialog width;
            // stretch them with the dialog (language-switch re-grow).
            if (m_titleBar)
                m_titleBar->setGeometry(0, 0, m_dlg->width(), kTitleBarHeight);
            if (m_titleLabel) {
                m_titleLabel->setGeometry(12, 0, m_dlg->width() - 100, kTitleBarHeight);
                m_titleLabel->setText(m_dlg->windowTitle());
            }
            if (m_closeBtn)
                m_closeBtn->move(m_dlg->width() - 44, 8);
            break;
        case QEvent::WindowTitleChange:
            // Language switch changes the dialog title (tr()); keep the
            // embedded title bar in sync (2026-08-14).
            if (m_titleLabel && obj == m_dlg)
                m_titleLabel->setText(m_dlg->windowTitle());
            break;
        default:
            break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QDialog *m_dlg;
    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_closeBtn = nullptr;
    bool m_dragging = false;
    QPoint m_startGlobal;
    QPoint m_startPos;
};

// ── Embedded-dialog border ───────────────────────────────────────────────
// The embedded dialogs have no OS chrome, so paint a thin dark border in
// the dialog's own paint event via an event filter.  A dialog-scoped
// stylesheet is deliberately NOT used — any stylesheet on an ancestor
// switches the whole subtree to stylesheet rendering and alters child
// widgets (QListWidget alternatingRowColors, QTabWidget tabs), the same
// reason the spec 13 §5.1 background fix avoids it.  The border follows the
// 14px rounded mask so the corners stay clean.  Child widgets are never
// touched (their paint events do not pass through this filter).
class DialogBorderFilter : public QObject
{
public:
    explicit DialogBorderFilter(QWidget *dlg)
        : QObject(dlg), m_dlg(dlg)
    {
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj != m_dlg)
            return QObject::eventFilter(obj, ev);
        if (ev->type() == QEvent::Paint) {
            // Paint the dialog content first, then draw the border on top;
            // returning true skips the duplicate dispatch in notify().
            // QWidget::event() is protected, so call through the public
            // QObject::event() — still virtual, so it dispatches to the
            // dialog's own event() → paintEvent().  Event filters are only
            // invoked by QApplication::notify(), so this direct call does
            // NOT re-enter this filter.
            static_cast<QObject *>(m_dlg)->event(ev);
            QPainter p(m_dlg);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(QPen(QColor(0x66, 0x66, 0x66), 1.5));   // 1.5px dark gray
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(QRectF(m_dlg->rect()).adjusted(0.75, 0.75, -0.75, -0.75),
                              14, 14);
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QWidget *m_dlg;
};

// ── Embedded-dialog whitelist ────────────────────────────────────────────
// Only these class names are let through the embedded rendering path
// (QDialog::exec / QDialog::setVisible dispatch); everything else keeps the
// dialogBlocked + Rejected behavior.  QFileDialog is deliberately absent:
// it stays gated behind the Phase-2 file probe (planning doc §4, spec 13
// §9.1) — its sandbox/picker flow is not ready for embedding.
static const char *const kEmbedDialogWhitelist[] = {
    "ConfigDialog",
    "QMessageBox",
    "QInputDialog",
    "QColorDialog",
    "QFontDialog",
    // Business dialogs (stage 4): CurvesDialog/FunctionDialog open with a
    // non-modal show() (handled by the QDialog::setVisible interposer),
    // ErrDialog/FitDialog go through QDialog::exec(), and AxesDialog runs its
    // own exec() override routed via ohosEmbedDialog() (see below).
    "CurvesDialog",
    "ErrDialog",
    "FunctionDialog",
    "AxesDialog",
    "FitDialog",
    // Phase 5 analysis dialogs (2026-08-13): IntDialog/FFTDialog/
    // SmoothCurveDialog/FilterDialog/InterpolationDialog/PolynomFitDialog/
    // ExpDecayDialog — the Analysis-menu family.  All are exec()/show()-
    // opened; the generic QDialog::exec / QDialog::setVisible interposer
    // handles both paths, so no per-class hooks are needed.
    "IntDialog",
    "FFTDialog",
    "SmoothCurveDialog",
    "FilterDialog",
    "InterpolationDialog",
    "PolynomFitDialog",
    "ExpDecayDialog",
    // Phase 6 format dialogs (2026-08-13): TextDialog/LineDialog/
    // ImageDialog/LayerDialog/CurveRangeDialog/AssociationsDialog/
    // PlotDialog — the Format-menu family (title/line/image/layer
    // geometry/curve range/plot associations/plot details; ImageDialog is
    // also reused by resizeActiveWindow → "Window Geometry").  All are
    // exec()/show()-opened; the generic QDialog::exec / QDialog::setVisible
    // interposer handles both paths (none overrides exec), so no per-class
    // hooks are needed.  PlotDialog internals (tree combos/popups) get
    // tuned after real-device testing.
    "TextDialog",
    "LineDialog",
    "ImageDialog",
    "LayerDialog",
    "CurveRangeDialog",
    "AssociationsDialog",
    "PlotDialog",
    // Phase 7 direct-embed dialogs (2026-08-13): ScriptingLangDialog /
    // RenameWindowDialog are opened via embedDialog() directly from the
    // main.cpp menuAction branches.  Their named-subclass vtables (emitted
    // in libentry.so) route show() through the interposed QDialog::
    // setVisible below, whose whitelist gate would otherwise swallow the
    // first show() (dialogBlocked, dialog never appears).  Plain-QDialog
    // siblings (undo_redo_history / about) need no entry: their vtables
    // come from libQt5Widgets.so and dispatch to the real QWidget::
    // setVisible.  The modal-stack guard in QDialog::setVisible still
    // keeps these two blocked outside embedDialog (e.g. a stray desktop
    // showNormal() would SIGSEGV the single-window QPA).
    "ScriptingLangDialog",
    "RenameWindowDialog",
    // QFileDialog (2026-08-14, route 5): embedded file dialogs.  The
    // internal QComboBox popups are neutralized by the InputProbe
    // press-swallow → ArkTS picker, and the initial directory is the
    // authorized work folder (Qt reads/writes it directly).  Probe (a)
    // failure modes (popup window layer / qfiledialog_p showPopup rewrite)
    // no longer trigger because the popups never open.
    "QFileDialog",
    // ImportASCIIDialog / ExportDialog (2026-08-14, route 5): the native
    // File→Import/Export ASCII dialogs.  Both are QDialog subclasses whose
    // exec() goes through the generic interposer; their QComboBoxes
    // (lookInCombo/fileTypeCombo/separator/table/import-mode) are swallowed
    // by the InputProbe → ArkTS picker, so the popup hazards are gone.
    "ImportASCIIDialog",
    "ExportDialog",
    // OpenProjectDialog / ImageExportDialog (2026-08-14, route 5): the
    // remaining file-picker dialogs (File→Open Project, File→Export Graph).
    // Same ExtensibleFileDialog family as ImportASCIIDialog — embedded
    // rendering verified on-device (ImportASCIIDialog 1500×825).
    "OpenProjectDialog",
    "ImageExportDialog",
    // D3 (2026-08-14): Table→Sort (future::SortDialog) / View→Plot Wizard
    // (PlotWizard) — both exec()-opened named subclasses; the setVisible
    // whitelist gate would otherwise swallow their first show().
    "SortDialog",
    "PlotWizard",
    // D3 (2026-08-14): Table→Dimensions 用裸 QDialog（Ui::DimensionsDialog
    // setupUi 到 QDialog 实例，future_Table.cpp L1887）——exec() 走本拦截器
    // 必须白名单（className 恰为 "QDialog"，不影响任何子类）。
    "QDialog",
};

static bool isEmbedDialog(const char *className)
{
    for (const char *name : kEmbedDialogWhitelist) {
        if (strcmp(className, name) == 0)
            return true;
    }
    return false;
}

// ── Per-class size/anchor policies (spec 13 §4) ──────────────────────────
// Embedded dialogs keep their DESKTOP aspect ratios — the on-device
// sizeHint is NEVER used as the ratio basis because the 15pt OHOS font
// inflates it (ConfigDialog: ~1901x697 = 2.73:1 instead of the desktop
// 1132:704, spec 13 §4.4).  Since 2026-08-13 the sizeHint WIDTH is used
// as a floor (max with the table width) so no form row truncates; the
// height still follows the table aspect.  Width is clamped to screen-60,
// height to screen-120; every dialog anchors screen-centered with the
// mapToGlobal origin compensation (spec 13 §4.3).
struct DialogSizePolicy {
    const char *className;   // QMetaObject className to match
    int width;               // design width in logical px (before clamping)
    double aspect;           // desktop width / height
    int closeInsetX;         // retained for table stability (spec 14 §4);
                             // unused under the title-bar scheme — the
                             // close button lives in the 44px title bar,
                             // clear of all content (kTitleBarHeight)
    int closeInsetY;         // see closeInsetX
    int maxContentH;         // 2026-08-14: hard cap on the CONTENT height
                             // (before title bar); 0 = no cap.  File-dialog
                             // family uses this — their sizeHint height
                             // balloons at the 15pt device font, and the
                             // user wants them as compact as the image
                             // pickers.
};

static const DialogSizePolicy kDialogSizePolicies[] = {
    // ConfigDialog: user-approved 1473 + 5% = 1547, native 1132:704 (1.608:1).
    { "ConfigDialog", 1547, 1132.0 / 704.0, 0, 0, 0 },
    // QMessageBox: icon + text + button row — compact and wide.
    { "QMessageBox", 540, 2.10, 0, 0, 0 },
    // QInputDialog: label + input field + button row.
    { "QInputDialog", 480, 2.35, 0, 0, 0 },
    // QColorDialog: the color grid is roughly square; slightly wider.
    { "QColorDialog", 560, 1.20, 0, 0, 0 },
    // QFontDialog: family/style/size lists need the most width (device
    // verdict 2026-08-12: 680 too small → 1024, h≈706).
    { "QFontDialog", 1024, 1.45, 12, 8, 0 },
    // Business dialogs (stage 4), anchor-scaled to ConfigDialog.
    { "CurvesDialog", 1150, 1.75, 0, 0, 0 },
    { "ErrDialog", 900, 1.71, 0, 0, 0 },
    { "FunctionDialog", 593, 1.08, 0, 0, 0 },
    { "AxesDialog", 1200, 1.49, 0, 0, 0 },
    { "FitDialog", 1144, 1.40, 0, 0, 0 },
    // Phase 5 analysis dialogs.
    { "IntDialog", 720, 1.30, 0, 0, 0 },
    { "FFTDialog", 660, 1.35, 0, 0, 0 },
    { "SmoothCurveDialog", 560, 1.30, 0, 0, 0 },
    { "FilterDialog", 720, 1.30, 0, 0, 0 },
    { "InterpolationDialog", 520, 1.30, 0, 0, 0 },
    { "PolynomFitDialog", 740, 1.30, 0, 0, 0 },
    { "ExpDecayDialog", 720, 1.30, 0, 0, 0 },
    // Phase 6 format dialogs.
    { "TextDialog", 560, 1.30, 0, 0, 0 },
    { "LineDialog", 560, 1.30, 0, 0, 0 },
    { "ImageDialog", 560, 1.25, 0, 0, 0 },
    { "LayerDialog", 700, 1.25, 0, 0, 0 },
    { "CurveRangeDialog", 560, 1.35, 0, 0, 0 },
    { "AssociationsDialog", 700, 1.30, 0, 0, 0 },
    { "PlotDialog", 900, 1.30, 0, 0, 0 },
    // File-dialog family (2026-08-14, route 5): generous width for the
    // sidebar+list+preview panes.  maxContentH caps the CONTENT height —
    // their sizeHint height balloons at the 15pt device font; the user
    // wants them as compact as the image pickers (which render fine).
    { "QFileDialog", 1500, 1.60, 0, 0, 620 },
    { "ImportASCIIDialog", 1500, 1.55, 0, 0, 620 },
    { "ExportDialog", 620, 1.45, 0, 0, 0 },
    { "OpenProjectDialog", 1500, 1.55, 0, 0, 620 },
    { "ImageExportDialog", 1500, 1.55, 0, 0, 620 },
};

static const DialogSizePolicy *findSizePolicy(const char *className)
{
    for (const DialogSizePolicy &p : kDialogSizePolicies) {
        if (strcmp(p.className, className) == 0)
            return &p;
    }
    return nullptr;
}

// Locate the SciDAVis main window — the only window the QPA renders to the
// XComponent surface (spec 13 §1.1).  Null during startup; dialog statics
// then fall back to their event-reporting paths.
static QWidget *findMainWindow()
{
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (w->inherits("ApplicationWindow"))
            return w;
    }
    return nullptr;
}

static bool inModalStack(const QWidget *w)
{
    for (QWidget *m : g_modalStack) {
        if (m == w)
            return true;
    }
    return false;
}

// ── Generic embedded-dialog renderer (spec 13 §2.2/§9.1) ─────────────────
// Renders a whitelisted QDialog as a floating child of the main window:
// the single-window QPA attaches only the FIRST window to an XComponent
// surface (window_component_1 attach root node success); a second native
// window's surface is destroyed by a 3s attach timeout (verified on-device
// 2026-08-11).  This is the shared template for ConfigDialog, the four
// standard dialogs, and the five business dialogs (stage 4) — 100% the real
// Qt widget, composited onto the main window's proven XComponent surface.
// Steps (identical to the original ConfigDialog-only path): setParent +
// Qt::Widget → background fill → class size policy → screen-centered anchor →
// show/raise/setFocus → rounded mask → title bar (dialog title + close
// button) → drag filter → manual QEventLoop with InputProbe modal isolation.
// Returns the QDialog
// result code (QDialog::Rejected if no main window — callers then fall back).
int embedDialog(QDialog *dlg)
{
    // Reentry guard (device crash 2026-08-12): the setVisible interposer can
    // re-enter embedDialog for the same dialog (pre-whitelist-fix L990), which
    // recursed until the stack overflowed (cppcrash SIGSEGV 16:13:22).  A
    // dialog already being embedded must never embed again; DIFFERENT nested
    // dialogs (ConfigDialog → CurvesDialog) are unaffected.
    if (inModalStack(dlg))
        return QDialog::Rejected;
    QWidget *mainWindow = findMainWindow();
    if (!mainWindow)
        return QDialog::Rejected;

    dlg->setParent(mainWindow);
    // 2026-08-16 (crash fix, cppcrash 14:32): QFileDialog's file-name
    // QLineEdit carries a QCompleter whose popup is a Qt::Popup QListView
    // window ("QListViewClassWindow").  Typing the first character in the
    // PDF save dialog opened the completer popup; the single-window QPA
    // cannot create it (JsWindowManager refuses sub-windows), the plugin's
    // "create open harmony platform window failed" path then died with
    // SIGSEGV — the "typing one character crashes the app" bug.  Disable
    // the completer on every line edit of every embedded dialog so no
    // completer popup can ever open (the ArkTS shell owns all popups).
    {
        const QList<QLineEdit *> lineEdits = dlg->findChildren<QLineEdit *>();
        for (QLineEdit *le : lineEdits) {
            if (le->completer()) {
                qWarning("[SciDAVis] embedDialog %s: completer disabled on QLineEdit (%s)",
                         dlg->metaObject()->className(), qPrintable(le->objectName()));
                le->setCompleter(nullptr);
            }
        }
    }
    dlg->setWindowFlags(Qt::Widget);
    // Child-widget QDialogs paint no background by default, so the main
    // window's table content showed through (verified on-device 2026-08-11).
    // autoFillBackground fills with the palette Window color;
    // WA_StyledBackground enables the stylesheet paint path.  A
    // dialog-scoped stylesheet is deliberately NOT used — it would switch
    // the whole subtree to stylesheet rendering and alter child widgets
    // (QListWidget with alternating rows, QTabWidget) look.
    dlg->setAutoFillBackground(true);
    dlg->setAttribute(Qt::WA_StyledBackground, true);
    // Size policy from the class table (spec 13 §4): keep the DESKTOP
    // aspect ratio — the on-device sizeHint is distorted by the 15pt OHOS
    // font, so it is NEVER the ratio basis.  Width = max(table policy
    // width, Qt sizeHint width floor): Qt's layout engine computes the
    // sizeHint against the ACTUAL device font, so it is the true minimum
    // the widest form row needs.  Static ASCII-pixel estimates (8.5px×1.4
    // calibration, 2026-08-13) failed twice on device ("Non rounded Akima"
    // still truncated after 520→720), so the table alone cannot cover every
    // row — the sizeHint floor fixes all embedded dialogs at once (stage
    // 4/5/6) without touching the per-class policy values.  Width is capped
    // to the screen (physical geometry on this QPA); height is independent
    // of aspect (2026-08-13: decoupled to avoid vertical whitespace —
    // height = max(sizeHint H + titleH, 260+titleH floor), capped to
    // screen-120).
    // Risk note (2026-08-13): if some dialog's sizeHint is itself distorted
    // (a6 QPA known risk — e.g. a hint computed before the layout ran, or a
    // bogus cached hint), the diagnostic log below exposes it as an
    // anomalously small/large value.  If a specific dialog proves
    // distorted, add a coefficient or a whitelist exception for it there;
    // do NOT reintroduce static width estimates.
    const DialogSizePolicy *policy = findSizePolicy(dlg->metaObject()->className());
    const QRect screenRect = qApp->primaryScreen()->availableGeometry();
    const int policyW = policy ? policy->width : 800;
    // Hint = max(sizeHint, minimumSizeHint, minimumSize): explicit
    // minimums (setMinimumSize/setMinimumHeight) live in minimumSize(),
    // NOT in minimumSizeHint() (a layout "suggested" minimum that ignores
    // them) — About credits_box 400px floor was lost otherwise
    // (2026-08-13 real-device).
    const int hintW = qMax(qMax(dlg->sizeHint().width(), dlg->minimumSizeHint().width()),
                           dlg->minimumSize().width());
    const int hintH = qMax(qMax(dlg->sizeHint().height(), dlg->minimumSizeHint().height()),
                           dlg->minimumSize().height());
    int dlgW = policyW;
    if (hintW > 100)             // ignore empty/abnormal (<100) hints
        dlgW = qMax(policyW, hintW);
    dlgW = qMin(dlgW, screenRect.width() - 60);
    // Height compensation for the title bar: the 44px title bar (dialog
    // title + close button) sits ON TOP of the dialog, so the content area
    // keeps its design height — the dialog grows by titleH (user
    // preference: slightly larger is fine over squashed content).  Still
    // capped to screen-120 below.  2026-08-13: height is now driven by
    // sizeHint H (+ titleH), NOT by aspect — aspect field stays in the
    // size table for reference only (decoupling avoids vertical
    // whitespace when width is enlarged by the sizeHint floor).
    const int titleH = kTitleBarHeight;
    // 2026-08-13: height decoupled from width/aspect.  Old formula
    // (dlgW/aspect) scaled height proportionally to width, so when the
    // sizeHint widened the dialog the height ballooned and left large
    // vertical whitespace (user-reported).  Now height tracks the
    // sizeHint height directly (+ title bar), with a floor so small/
    // abnormal hints still yield a usable dialog.
    int dlgH = (hintH > 100) ? (hintH + titleH)
                             : (qRound(policyW / (policy ? policy->aspect : 1.6)) + titleH);
    dlgH = qMax(dlgH, 260 + titleH);
    // 2026-08-14: per-class content-height cap (maxContentH) — the file
    // dialog family's sizeHint height balloons at the 15pt device font
    // (ImportASCIIDialog hintH=781); cap the CONTENT height so these stay
    // as compact as the image pickers.  Applied before the screen cap.
    if (policy && policy->maxContentH > 0)
        dlgH = qMin(dlgH, policy->maxContentH + titleH);
    dlgH = qMin(dlgH, screenRect.height() - 120);
    dlg->resize(dlgW, dlgH);
    qWarning("[SciDAVis] embedDialog %s: sizeHint=%dx%d policy=%d aspect=%.2f final=%dx%d",
             dlg->metaObject()->className(), hintW, hintH, policyW,
             policy ? policy->aspect : 1.6, dlgW, dlgH);
    // The title bar occupies the top titleH rows of the dialog; the
    // contents margins push every embedded dialog's content below it, so
    // nothing can ever overlap the close button (the whole top strip is
    // reserved for title + ×).
    dlg->setContentsMargins(0, titleH, 0, 0);
    // The QPA places the main-window content at a screen offset; compensate
    // so the dialog is truly screen-centered.  Falls back to the
    // un-compensated position if mapToGlobal is a no-op.
    const QPoint contentOrigin = mainWindow->mapToGlobal(QPoint(0, 0));
    const QPoint desired = screenRect.center()
            - QRect(QPoint(0, 0), QSize(dlgW, dlgH)).center();
    dlg->move(qMax(0, desired.x() - contentOrigin.x()),
              qMax(0, desired.y() - contentOrigin.y()));
    // ConfigDialog-specific: the left category list must never be squeezed
    // below readable width (layout compresses it when the dialog is
    // resized).  The 15pt font makes "2D Plots"/"3D Plots" wider than the
    // default 180 logical px — the trailing "s" was clipped (spec 13 §4.5).
    if (strcmp(dlg->metaObject()->className(), "ConfigDialog") == 0) {
        if (QListWidget *list = dlg->findChild<QListWidget *>())
            list->setMinimumWidth(300);
    }
    // Save the currently active MDI subwindow BEFORE the dialog takes
    // focus: after the dialog hides, QMdiArea's focus fallback activates
    // the WRONG subwindow (typically the first-created table1).  The
    // saved one is restored after hide() below.  QPointer guards against
    // the subwindow being destroyed while the modal loop runs.
    QMdiArea *mdi = mainWindow->findChild<QMdiArea *>();
    QPointer<QMdiSubWindow> savedActive = mdi ? mdi->activeSubWindow() : nullptr;
    // Push onto the modal stack BEFORE show(): it doubles as the
    // "currently being embedded" marker that the QColorDialog/QFontDialog
    // setVisible passthrough consults, and InputProbe uses the top entry
    // for modal isolation.  No events are dispatched until the loop below
    // runs, so the earlier set only tightens the isolation window.
    g_modalStack.push_back(dlg);
    g_modalDialogActive = true;
    // QColorDialog crash guard (device crash, 2026-08-12): the color wells'
    // "drag the color out" feature starts QDrag::exec() once the drag
    // threshold is exceeded (rapid taps with a slight drift).  On this QPA
    // the drag-window creation SEGVs in QOpenHarmonyJsObject::getJsFunction
    // (addr2line: QColorWell::mouseMoveEvent → QDrag::exec →
    // QBasicDrag::recreateShapedPixmapWindow).  The QPA plugin cannot be
    // modified (AGENTS.md), so raise the drag threshold to INT_MAX for the
    // duration of the embedded loop: the drag never starts, while
    // in-well color adjustment (which does not use startDragDistance())
    // keeps working.  Restored right after the loop, so global dragging
    // (table columns, ...) is unaffected.
    const bool colorDialog = strcmp(dlg->metaObject()->className(), "QColorDialog") == 0;
    const int savedDragDistance = colorDialog ? QApplication::startDragDistance() : 0;
    if (colorDialog)
        QApplication::setStartDragDistance(INT_MAX);
    // Notify ArkTS that a manual modal loop is about to block the Qt
    // thread: the shell pauses its getUiState polling (each sync NAPI call
    // can hold the JS thread up to 3s and starve input delivery — the
    // device watchdog killed the app with APP_INPUT_BLOCK, hilog 2026-08-12).
    // Emitted before show() so it precedes the first poll that could block.
    // Self-check (2026-08-12): queued scidavis_call lambdas are plain
    // posted events, so the nested QEventLoop below DOES process them and
    // short queries still resolve quickly; the polling pause removes the
    // remaining input-starvation window entirely.
    scidavisEmitEvent(QStringLiteral("modalStarted"));
    scidavis_mark_modal_started();
    dlg->show();
    dlg->raise();
    dlg->setFocus();
    // Rounded corners: the embedded dialog has no OS chrome, so mask the
    // widget region with a rounded rect — a soft 14px corner (spec 13
    // §5.2).  QWidget masks apply to child widgets too (paint-clipped),
    // and setMask keeps the close button inside the rounded area (it is
    // raised above, but its top-right corner is within the mask).
    {
        QPainterPath mask;
        mask.addRoundedRect(QRectF(dlg->rect()), 14, 14);
        dlg->setMask(QRegion(mask.toFillPolygon().toPolygon()));
    }
    // ── Title bar: dialog title (left) + close button (right) ──
    // Embedded dialogs have no OS window chrome, so the top titleH rows
    // become a title bar: the window title (left-aligned) plus the close
    // button (right-aligned).  The dialog content starts BELOW it thanks to
    // the contents margins set above, so no content can ever overlap the ×
    // (the old hideCloseButton escape hatch is gone — every dialog gets a
    // top-right ×, per the title-bar requirement; empty title bars simply
    // keep the reserved strip).
    // reject() drives the finished signal which exits the manual QEventLoop
    // below — the same verified close path as OK/Cancel.  Parented to the
    // dialog, so it is destroyed together with it (WA_DeleteOnClose).
    // Created after show() so the widget tree is live; the dialog's rounded
    // 14px mask clips the title bar's corners too (paint-clipped, as
    // before).  show()/update() are mandatory — a6 QPA does not composite
    // widgets created after the dialog is shown otherwise (spec 13 §5.4).
    QWidget *titleBar = new QWidget(dlg);
    titleBar->setGeometry(0, 0, dlg->width(), titleH);
    // Transparent on purpose (root cause of the "frameless title bar" bug):
    // the dialog paints its own background (autoFillBackground +
    // WA_StyledBackground, set in embedDialog) and DialogBorderFilter draws
    // the 1.5px #666 border at the END of the dialog's paint phase — i.e.
    // before any child widget paints.  An opaque title bar background would
    // overwrite that border along y=0 and along the x=0 / x=w-1 edges in the
    // top titleH strip, leaving the title area with no frame while the area
    // below kept it.  With autoFillBackground(false) the title bar writes no
    // pixels: the dialog background (same palette Window color) and the
    // border show through untouched, so the frame is continuous.  titleLabel
    // (color:#333) and closeBtn (own stylesheet) draw themselves and are
    // unaffected.
    titleBar->setAutoFillBackground(false);
    QLabel *titleLabel = new QLabel(titleBar);
    titleLabel->setText(dlg->windowTitle());
    titleLabel->setGeometry(12, 0, dlg->width() - 100, titleH);
    titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    // Ordinary text color on the dialog background; the stylesheet is
    // label-scoped and does not affect the dialog's subtree rendering.
    titleLabel->setStyleSheet(QStringLiteral("color: #333;"));
    QPushButton *closeBtn = new QPushButton(titleBar);
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setFixedSize(36, 32);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setToolTip(QObject::tr("Close"));
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { border: 1px solid #888; border-radius: 4px;"
        "  background: #f0f0f0; color: #222; font-size: 16px; }"
        "QPushButton:pressed { background: #e0e0e0; }"));
    closeBtn->move(dlg->width() - 44, 8);
    closeBtn->raise();
    // a6 QPA: children created after show() must be shown/updated
    // explicitly or they never composite (spec 13 §5.4).
    titleBar->show();
    titleBar->update();
    titleLabel->show();
    titleLabel->update();
    closeBtn->show();
    closeBtn->update();
    // The dialog size is fixed for its lifetime (no stretching), so one
    // geometry pass right after the first show() suffices; had the dialog
    // resized later, an event filter on the dialog would be needed.
    titleBar->setGeometry(0, 0, dlg->width(), titleH);
    closeBtn->move(dlg->width() - 44, 8);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    dlg->update();   // force repaint so the new children are composited
    // Drag (title bar) for the embedded dialog; owned by the dialog,
    // destroyed with it.  The filter installs itself on the title bar as
    // well — the title bar widget swallows every mouse event in the top
    // strip, so a dialog-only filter would never see presses there.
    // The title label + close button are handed over so the filter can
    // stretch/retitle them when the dialog re-grows on a language switch
    // (2026-08-14).
    DragResizeFilter *drf = new DragResizeFilter(dlg, titleBar);
    drf->setChrome(titleLabel, closeBtn);
    dlg->installEventFilter(drf);
    // Visible border (no OS chrome on embedded dialogs); painted on top of
    // the dialog's own paint event, child widgets untouched.
    DialogBorderFilter *dbf = new DialogBorderFilter(dlg);
    dlg->installEventFilter(dbf);
    // WA_DeleteOnClose guard (device cppcrash 20260813200516, 2026-08-13):
    // FFTDialog/CurvesDialog run with WA_DeleteOnClose, and their accept()
    // chains (accept → close → deleteLater) can destroy the dialog while the
    // nested QEventLoop below is still draining events — the DeferredDelete
    // is processed before loop.exec() returns.  Without the guard, the
    // cleanup section after the loop dereferenced a freed dialog (jemalloc
    // 0x6b6b Use-After-Free pattern: read of the destroyed vtable in
    // dlg->hide()).  QPointer auto-nulls on destruction, so every post-loop
    // dlg access runs only inside `if (dlgGuard)`.
    QPointer<QDialog> dlgGuard(dlg);
    int dialogResult = QDialog::Rejected;
    QEventLoop loop;
    QObject::connect(dlg, &QDialog::finished, &loop,
                     [&](int r) { dialogResult = r; loop.quit(); });
    // Modal isolation: InputProbe swallows all input outside the top
    // dialog's subtree while the manual loop runs.
    loop.exec();
    g_modalStack.pop_back();
    g_modalDialogActive = !g_modalStack.empty();
    if (colorDialog)
        QApplication::setStartDragDistance(savedDragDistance);
    // The dialog may already be destroyed (WA_DeleteOnClose accept→close→
    // deleteLater inside the modal loop, see dlgGuard above): every dialog
    // access below is guarded — a destroyed dialog needs no hide/repaint.
    if (dlgGuard) {
        dlg->hide();
        // a6 QPA backingstore fix (2026-08-13): hide() does not refresh the
        // backingstore on this plugin — the dialog's pixels stay on screen
        // after it is hidden ("Hide needs a second click to disappear").
        // repaint() updates the backingstore but does NOT flush it to the
        // screen on this plugin (a later input event happens to trigger the
        // flush).  Request an update on the QWindow: the QPA's updateRequested
        // path performs the flush that clears the stale pixels.
        if (mainWindow) {
            const QRect dlgRect = dlg->geometry();
            mainWindow->repaint(dlgRect);
            if (QWindow *win = mainWindow->windowHandle())
                win->requestUpdate();
            qWarning("[SciDAVis] embedDialog cleanup: repainted+flushed %d,%d,%dx%d",
                     dlgRect.x(), dlgRect.y(), dlgRect.width(), dlgRect.height());
        }
        // 2026-08-13 hide-confirm diag: after "Hide" the table is hidden via
        // hiddenWindow while the dialog should already be hidden — log the
        // dialog's actual visibility state and the saved active sub-window
        // (a hidden savedActive restored by the deferred callback below could
        // re-show the table AND leave stale dialog pixels on screen).
        qWarning("[SciDAVis] embedDialog cleanup: dlg=%s visible=%d result=%d savedActive=%p",
                 dlg->metaObject()->className(), int(dlg->isVisible()), dialogResult,
                 static_cast<void *>(savedActive.data()));
    }
    // Restore the subwindow that was active before the dialog opened:
    // QMdiArea's focus fallback after the dialog hides activates the wrong
    // subwindow (typically the first-created table1).  Deferred by one
    // event-loop tick so the intermediate focus state drains before the
    // restore takes effect.  savedActive is a QPointer: the subwindow could
    // in theory be destroyed while the modal loop runs, and QPointer
    // auto-nulls on destruction so the callback never dereferences a
    // dangling pointer.
    if (mdi && savedActive) {
        // modalEnded is emitted only AFTER the focus restore, inside the
        // same deferred callback: it is the gate that re-enables the ArkTS
        // getUiState polling (paused by modalStarted), and emitting it
        // before setActiveSubWindow would let the poller sample the
        // intermediate fallback frame (table1 active, graph deactivated)
        // and only then be corrected by windowActivated(graph) — a spurious
        // "table1" frame in the shell.  Deferring past the restore keeps
        // the sampled window clean.  modalEnded fires on EVERY path — also
        // when the dialog was destroyed inside the modal loop (dlgGuard
        // null) — so the ArkTS getUiState polling never stays paused.
        QTimer::singleShot(0, mdi, [mdi, savedActive]() {
            // 2026-08-13 hide-confirm diag: restoring a window that was
            // hidden by the confirm dialog's "Hide" branch would re-show it
            // (and defeat the user's choice) — log what is being restored.
            if (savedActive)
                qWarning("[SciDAVis] embedDialog focus restore: %s visible=%d",
                         qPrintable(savedActive->objectName()), int(savedActive->isVisible()));
            if (savedActive)
                mdi->setActiveSubWindow(savedActive);
            scidavisEmitEvent(QStringLiteral("modalEnded"));
            scidavis_mark_modal_ended();
        });
    } else {
        // Nothing to restore: resume the ArkTS polling immediately, at the
        // original point in the close sequence.
        scidavisEmitEvent(QStringLiteral("modalEnded"));
        scidavis_mark_modal_ended();
    }
    return dialogResult;
}

// Exported hook for AxesDialog::exec() (libscidavis, SCIDAVIS_OHOS branch):
// AxesDialog overrides exec() with a self-contained implementation that never
// reaches QDialog::exec(), so the exec() interposer below cannot intercept it.
// Desktop AxesDialog::exec() shows the dialog NON-blockingly and returns
// immediately — callers (showAxisDialog/showGridDialog/
// showScalePageFromAxisDialog) then switch the page / select the axis AFTER
// exec() returns.  A synchronous embed would block until the dialog closes
// and leave those configuration calls on a hidden dialog (Format→Axes would
// always open on the Scales page).  Defer the embed by one event-loop tick:
// exec() returns immediately, the caller's post-exec configuration runs
// BEFORE the dialog becomes visible, and only then does embedDialog() render
// it.  The context overload of singleShot cancels the call if the dialog is
// destroyed meanwhile (WA_DeleteOnClose is not set on the addChild-owned
// AxesDialog, but the guard costs nothing).
int ohosEmbedDialog(QDialog *dlg)
{
    QTimer::singleShot(0, dlg, [dlg]() { embedDialog(dlg); });
    return 0;
}

extern "C" void scidavis_set_event_sink(scidavis_event_sink_t sink)
{
    std::deque<std::string> flush;
    {
        std::lock_guard<std::mutex> lock(g_sinkMutex);
        g_sink = sink;
        if (sink)
            std::swap(flush, g_pending);
    }
    BRIDGE_LOG("event sink %s (%zu buffered flushed)", sink ? "registered" : "cleared",
               flush.size());
    if (sink)
        for (const std::string &e : flush)
            sink(e.c_str());
}

extern "C" void scidavis_emit(const char *json)
{
    if (!json)
        return;
    scidavis_event_sink_t sink;
    {
        std::lock_guard<std::mutex> lock(g_sinkMutex);
        if (!g_sink) {
            if (g_pending.size() >= kMaxPending)
                g_pending.pop_front();
            g_pending.emplace_back(json);
            return;
        }
        sink = g_sink;
    }
    sink(json);
}

void scidavisEmitEvent(const QString &kind, QJsonObject payload)
{
    payload.insert(QStringLiteral("kind"), kind);
    const QByteArray doc = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    scidavis_emit(doc.constData());
}

// ── Shared helpers for the interposed entry points ───────────────────────

namespace {

QJsonObject widgetInfo(const QWidget *w)
{
    QJsonObject o;
    if (w) {
        o[QStringLiteral("className")] = QString::fromLatin1(w->metaObject()->className());
        o[QStringLiteral("objectName")] = w->objectName();
        o[QStringLiteral("title")] = w->windowTitle();
    }
    return o;
}

// Safe default answer for a blocked QMessageBox: prefer the non-destructive
// choice (Ok to acknowledge, No to decline) over a potentially destructive
// Yes, then the caller-supplied default, then Cancel.
QMessageBox::StandardButton pickSafeButton(QMessageBox::StandardButtons buttons,
                                           QMessageBox::StandardButton defaultButton)
{
    if (buttons & QMessageBox::Ok)
        return QMessageBox::Ok;
    if (buttons & QMessageBox::No)
        return QMessageBox::No;
    if (defaultButton != QMessageBox::NoButton)
        return defaultButton;
    if (buttons & QMessageBox::Cancel)
        return QMessageBox::Cancel;
    if (buttons & QMessageBox::Close)
        return QMessageBox::Close;
    return QMessageBox::Cancel;
}

QMessageBox::StandardButton emitMessage(const char *severity, QWidget *parent,
                                        const QString &title, const QString &text,
                                        QMessageBox::StandardButtons buttons,
                                        QMessageBox::StandardButton defaultButton)
{
    const QMessageBox::StandardButton chosen = pickSafeButton(buttons, defaultButton);
    QJsonObject p = widgetInfo(parent);
    p[QStringLiteral("severity")] = QString::fromLatin1(severity);
    p[QStringLiteral("title")] = title;
    p[QStringLiteral("text")] = text;
    p[QStringLiteral("buttons")] = int(buttons);
    p[QStringLiteral("defaultButton")] = int(defaultButton);
    p[QStringLiteral("chosen")] = int(chosen);
    scidavisEmitEvent(QStringLiteral("message"), p);
    BRIDGE_LOG("QMessageBox::%s blocked: %s", severity, title.toUtf8().constData());
    return chosen;
}

void emitFileDialog(const char *mode, QWidget *parent, const QString &caption, const QString &dir,
                    const QString &filter)
{
    QJsonObject p = widgetInfo(parent);
    p[QStringLiteral("mode")] = QString::fromLatin1(mode);
    p[QStringLiteral("caption")] = caption;
    p[QStringLiteral("dir")] = dir;
    p[QStringLiteral("filter")] = filter;
    scidavisEmitEvent(QStringLiteral("fileDialog"), p);
    BRIDGE_LOG("QFileDialog::%s blocked: %s", mode, caption.toUtf8().constData());
}

void emitInputDialog(const char *mode, const QString &title, const QString &label)
{
    QJsonObject p;
    p[QStringLiteral("mode")] = QString::fromLatin1(mode);
    p[QStringLiteral("title")] = title;
    p[QStringLiteral("label")] = label;
    scidavisEmitEvent(QStringLiteral("inputDialog"), p);
    BRIDGE_LOG("QInputDialog::%s blocked: %s", mode, title.toUtf8().constData());
}

// ── Menu registry (context menus) ────────────────────────────────────────
// Blocked QMenus are registered so ArkTS can render the entries and, for
// heap-allocated menus that stay alive (e.g. windowsMenu), trigger the
// chosen action via ohosBridgeTriggerMenu.  Stack-allocated menus die as
// soon as the caller returns; triggering those fails gracefully.
std::mutex g_menuMutex;
std::map<int, QPointer<QMenu>> g_menus;
int g_menuSeq = 0;

QJsonArray describeActions(const QList<QAction *> &actions, int depth)
{
    QJsonArray arr;
    for (QAction *a : actions) {
        QJsonObject item;
        if (a->isSeparator()) {
            item[QStringLiteral("separator")] = true;
        } else {
            item[QStringLiteral("text")] = a->text();
            item[QStringLiteral("enabled")] = a->isEnabled();
            item[QStringLiteral("checkable")] = a->isCheckable();
            item[QStringLiteral("checked")] = a->isChecked();
            if (a->menu() && depth < 3)
                item[QStringLiteral("children")] = describeActions(a->menu()->actions(), depth + 1);
        }
        arr.append(item);
    }
    return arr;
}

void emitBlockedMenu(QMenu *menu)
{
    if (!menu)
        return;
    int id;
    {
        std::lock_guard<std::mutex> lock(g_menuMutex);
        // prune dead entries
        for (auto it = g_menus.begin(); it != g_menus.end();)
            it = it->second.isNull() ? g_menus.erase(it) : std::next(it);
        id = ++g_menuSeq;
        g_menus[id] = menu;
    }
    QJsonObject p = widgetInfo(menu);
    p[QStringLiteral("menuId")] = id;
    p[QStringLiteral("items")] = describeActions(menu->actions(), 0);
    scidavisEmitEvent(QStringLiteral("contextMenu"), p);
    BRIDGE_LOG("QMenu blocked: id=%d '%s' (%d actions)", id,
               menu->objectName().toUtf8().constData(), menu->actions().size());
}

// ── Combo popup registry ─────────────────────────────────────────────────
std::mutex g_comboMutex;
std::map<int, QPointer<QComboBox>> g_combos;
int g_comboSeq = 0;

// Combo-picker modality (2026-08-14): while the ArkTS picker is up, Qt must
// not react to input underneath it (a press on another control inside the
// embedded dialog would re-open a picker / activate the control although the
// user only meant to dismiss the current one).  InputProbe consults
// ohosBridgeComboPickerActive(); requestCombo opens the modal state,
// selectCombo/dismissCombo end it.  All access happens on the Qt GUI thread.
// (These must stay OUTSIDE the anonymous namespace — main.cpp links them.)
static bool g_comboPickerOpen = false;
// Debounce: the native press belonging to the same tap that closed the
// picker on the ArkTS side arrives just after dismiss — keep swallowing for
// this window so the tap cannot activate a control underneath.
static QElapsedTimer g_comboPickerCloseTimer;
static bool g_comboPickerCloseTimerValid = false;
static const qint64 kComboDismissDebounceMs = 250;

} // namespace

bool ohosBridgeComboPickerActive()
{
    if (g_comboPickerOpen)
        return true;
    return g_comboPickerCloseTimerValid
            && !g_comboPickerCloseTimer.hasExpired(kComboDismissDebounceMs);
}

void ohosBridgeDismissCombo()
{
    g_comboPickerOpen = false;
    g_comboPickerCloseTimer.start();
    g_comboPickerCloseTimerValid = true;
    BRIDGE_LOG("combo picker dismissed (debounce %lld ms)", qint64(kComboDismissDebounceMs));
}

// Qt-side language change → ArkTS shell mirror (three-mode language switch,
// 2026-08-14).  ApplicationWindow::switchToLanguage() calls this so the
// shell can call setAppPreferredLanguage and rebuild its own UI strings.
void ohosNotifyLanguage(const char *mode)
{
    if (!mode)
        return;
    QJsonObject p;
    p[QStringLiteral("mode")] = QString::fromUtf8(mode);
    scidavisEmitEvent(QStringLiteral("languageChanged"), p);
    BRIDGE_LOG("languageChanged: mode=%s", mode);
}

bool ohosBridgeTriggerMenu(int menuId, const QString &path)
{
    QPointer<QMenu> menu;
    {
        std::lock_guard<std::mutex> lock(g_menuMutex);
        auto it = g_menus.find(menuId);
        if (it == g_menus.end())
            return false;
        menu = it->second;
        g_menus.erase(it);
    }
    if (menu.isNull())
        return false;
    // path is a '/'-separated index chain, e.g. "2/0" = 3rd action's
    // submenu, 1st action.
    QList<QAction *> actions = menu->actions();
    QAction *target = nullptr;
    for (const QString &seg : path.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int idx = seg.toInt(&ok);
        if (!ok || idx < 0 || idx >= actions.size())
            return false;
        target = actions.at(idx);
        actions = target->menu() ? target->menu()->actions() : QList<QAction *>();
    }
    if (!target || target->isSeparator() || target->menu() || !target->isEnabled())
        return false;
    target->trigger();
    return true;
}

bool ohosBridgeSelectCombo(int comboId, int index)
{
    // Selection ends the picker modal state (debounced) — see
    // ohosBridgeComboPickerActive().
    ohosBridgeDismissCombo();
    QPointer<QComboBox> box;
    {
        std::lock_guard<std::mutex> lock(g_comboMutex);
        auto it = g_combos.find(comboId);
        if (it == g_combos.end())
            return false;
        box = it->second;
        g_combos.erase(it);
    }
    if (box.isNull())
        return false;
    if (index >= 0 && index < box->count()) {
        box->setCurrentIndex(index);
        // The ArkTS picker replaced the user's dropdown interaction, so the
        // combo's own click path never runs — emit activated() manually to
        // fire the business logic connected to it (e.g. ConfigDialog
        // boxLanguage → switchToLanguage).  Without this, selecting a new
        // item changed the display but the app behaviour did not follow
        // (issue "embedded dialog dropdown misbehaves", 2026-08-14).
        // currentIndexChanged is already emitted by setCurrentIndex when the
        // index actually changes; activated is the user-interaction signal.
        emit box->activated(index);
    }
    return true;
}

// Shared popup-request emitter: registers the combo, builds the payload and
// pushes a comboRequest event.  Used both by the QComboBox::showPopup
// interposition (below) and — decisively — by InputProbe (main.cpp) which
// swallows the mouse press on a QComboBox BEFORE Qt's own mousePressEvent
// can reach the real popup path (see header comment).
bool ohosBridgeRequestCombo(QComboBox *box)
{
    if (!box)
        return false;
    // 2026-08-14 (user-reported): tapping a combo twice (or more) pushed a
    // SECOND comboRequest while the ArkTS picker was already up — the shell
    // stacked pickers / re-rendered and the layout collapsed into a broken
    // state.  Hard-gate re-entry: while the picker is open (or within the
    // dismiss debounce window) ANY further request is refused and the press
    // is swallowed, so only the FIRST tap opens the picker; extra taps do
    // nothing (and can never reach the real QComboBox popup path).
    if (ohosBridgeComboPickerActive()) {
        BRIDGE_LOG("comboRequest REFUSED: picker already active (re-entry guard)");
        return false;
    }
    int id;
    {
        std::lock_guard<std::mutex> lock(g_comboMutex);
        for (auto it = g_combos.begin(); it != g_combos.end();)
            it = it->second.isNull() ? g_combos.erase(it) : std::next(it);
        id = ++g_comboSeq;
        g_combos[id] = box;
    }
    // Opening the picker starts the modal state: InputProbe swallows every
    // press while it is active so nothing underneath the ArkTS picker can
    // be activated (see ohosBridgeComboPickerActive()).
    g_comboPickerOpen = true;
    g_comboPickerCloseTimerValid = false;
    QJsonArray items;
    for (int i = 0; i < box->count(); ++i)
        items.append(box->itemText(i));
    QJsonObject p = widgetInfo(box);
    p[QStringLiteral("comboId")] = id;
    p[QStringLiteral("items")] = items;
    p[QStringLiteral("current")] = box->currentIndex();
    scidavisEmitEvent(QStringLiteral("comboRequest"), p);
    BRIDGE_LOG("comboRequest: id=%d (%d items)", id, box->count());
    return true;
}

// ═════════════════════════════════════════════════════════════════════════
//  Symbol interposition — these definitions preempt the libQt5Widgets.so
//  implementations at static link time (signatures must match the Qt 5.15
//  OHOS headers exactly; default arguments live in the declarations).
// ═════════════════════════════════════════════════════════════════════════

// ── QDialog ──────────────────────────────────────────────────────────────

// libQt5Widgets.so is a DT_NEEDED dependency of libentry.so loaded with
// LOCAL scope, so dlsym(RTLD_NEXT, ...) — which only searches the global
// scope — returns nullptr for the real QDialog symbols on this device
// (verified on-device; same reason qohos.cpp resolves through dlopen
// handles).  Resolve them via an explicit handle instead: RTLD_NOLOAD
// reuses the already-loaded instance, with a lazy dlopen as fallback.
// LESSON: verify mangled names against the SDK library with llvm-nm -D —
// "_ZN7QDialog4execEv" is 7-QDialog (not 6), and QDialog::setVisible has
// no exported symbol at all (only subclass overrides are exported), so it
// is routed to QWidget::setVisible below instead of dlsym.
static void *qtWidgetsHandle = nullptr;
static void *resolveQtDialogSym(const char *sym)
{
    if (!qtWidgetsHandle) {
        qtWidgetsHandle = dlopen("libQt5Widgets.so", RTLD_LAZY | RTLD_NOLOAD);
        if (!qtWidgetsHandle)
            qtWidgetsHandle = dlopen("libQt5Widgets.so", RTLD_LAZY);  // 兜底：需要时加载
        if (!qtWidgetsHandle)
            qWarning("[SciDAVis] dlopen libQt5Widgets.so failed: %s", dlerror());
    }
    return qtWidgetsHandle ? dlsym(qtWidgetsHandle, sym) : nullptr;
}

int QDialog::exec()
{
    // Whitelisted dialogs render embedded in the main window via the
    // generic embedDialog template above (see kEmbedDialogWhitelist).
    // QInputDialog/QColorDialog/QFontDialog never override exec() in real
    // Qt, so direct call sites land here; QMessageBox overrides exec() and
    // its statics embed directly (never reaching this point) — its real
    // exec() would run a modal loop whose internal setVisible(true) is
    // blocked by the whitelist guard outside the modal stack, leaving an
    // invisible dialog.  Stage-4 business dialogs: ErrDialog/FitDialog are
    // exec()-opened without overrides (addErrorBars()/showFitDialog()
    // configure them BEFORE exec()) and land here; CurvesDialog/
    // FunctionDialog are show()-opened and route via the setVisible
    // interposer instead; AxesDialog overrides exec() and is handled by
    // ohosEmbedDialog() (AxesDialog.cpp, SCIDAVIS_OHOS branch).  Every
    // other dialog stays blocked — the single-window QPA SIGSEGVs on a real
    // top-level dialog.
    if (isEmbedDialog(metaObject()->className())) {
        if (findMainWindow())
            return embedDialog(this);
        // No main window (startup / defensive): the standard dialogs keep
        // their historical blocked behavior here (their statics own the
        // event-path fallback and never call exec() without a main window);
        // ConfigDialog falls through to the symbol-based fallback below.
        if (strcmp(metaObject()->className(), "ConfigDialog") != 0) {
            scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
            BRIDGE_LOG("blocked %s::exec()", metaObject()->className());
            return QDialog::Rejected;
        }
        // No main window (defensive): fall through to the symbol-based path.

        // The fallback: forward exec() to the real libQt5Widgets
        // implementation, resolved through the dlopen handle above
        // (RTLD_NEXT misses the LOCAL-scope Qt lib).  Called as a member
        // function pointer with this (Itanium ABI: this in x0), not a
        // plain function pointer.
        typedef int (QDialog::*QDialogExecFn)();
        static QDialogExecFn realExec = nullptr;
        if (!realExec) {
            void *sym = resolveQtDialogSym("_ZN7QDialog4execEv");
            memcpy(&realExec, &sym, sizeof(realExec));   // dlsym→成员函数指针的标准做法
        }
        if (realExec) {
            // ConfigDialog arrives here with Qt's default 200x60 geometry —
            // QPA builds the native window from this rect, which then never
            // reaches the screen.  Force a real size (sizeHint-based) before
            // the modal loop so the QPA creates a proper window.
            if (width() < 400 || height() < 300)
                adjustSize();
            if (width() < 400 || height() < 300)
                resize(qMax(640, sizeHint().width()), qMax(480, sizeHint().height()));
            // QPA: Qt::Dialog windows take the QOhJsWindowManager
            // createSubWindow path (content never reaches the screen on this
            // firmware); the main window (Qt::Window) renders through the
            // EWNode/XComponent path.  Promote ConfigDialog to Qt::Window so
            // it uses the working embedded path (2026-08-11 experiment).
            Qt::WindowFlags f = windowFlags();
            if (f & Qt::Dialog) {
                setWindowFlags((f & ~Qt::Dialog) | Qt::Window);
            }
            // QPA view-type decision (qohosview.cpp determineViewTypeAndLogicalParent):
            //   parent != nullptr  -> EmbeddedWindow (XComponent attach path, works)
            //   no parent + Dialog/Tool types + visible main window -> SubWindow
            //     (JS system window; firmware rejects: create js window timeout
            //      / WMSSub device not support)
            // A parentless ConfigDialog lands in the SubWindow path (verified
            // on-device 2026-08-11).  Restore the QWidget parent (the visible
            // main window) AND keep the Qt::Window type promotion (removes
            // Qt::Dialog from the SubWindow fallback list) so the dialog takes
            // the EmbeddedWindow path and renders into the main window's
            // XComponent surface.
            QWidget *mainW = nullptr;
            const auto topLevels = QApplication::topLevelWidgets();
            for (QWidget *w : topLevels) {
                if (w->isVisible() && w->windowType() == Qt::Window && w != this) {
                    mainW = w;
                    break;
                }
            }
            if (mainW)
                setParent(mainW);
            // The setVisible whitelist guard (above) requires embedded
            // dialogs to be on the modal stack; push for the duration of
            // the real modal loop so its internal show() still passes
            // through (InputProbe also isolates the main window here, same
            // as embedDialog).  modalStarted/modalEnded pause the ArkTS
            // getUiState polling, as in embedDialog.
            g_modalStack.push_back(this);
            g_modalDialogActive = true;
            scidavisEmitEvent(QStringLiteral("modalStarted"));
            scidavis_mark_modal_started();
            const int result = (this->*realExec)();
            g_modalStack.pop_back();
            g_modalDialogActive = !g_modalStack.empty();
            scidavisEmitEvent(QStringLiteral("modalEnded"));
            scidavis_mark_modal_ended();
            return result;
        }
        qWarning("ohos_bridge: dlsym(_ZN7QDialog4execEv) failed — keeping exec() "
                 "blocked for %s", metaObject()->className());
    }
    scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
    BRIDGE_LOG("blocked %s::exec()", metaObject()->className());
    return QDialog::Rejected;
}

void QDialog::setVisible(bool visible)
{
    // Whitelist passthrough (same rationale as exec() above): the
    // visibility must reach a real implementation so the dialog actually
    // shows/hides.  Guarded by modal-stack membership — embedDialog pushes
    // the dialog before show() — so only dialogs CURRENTLY being embedded
    // pass through (the QColorDialog/QFontDialog overrides below use the
    // same guard); hiding is always safe via the fall-through branch.
    // QDialog::setVisible has no exported symbol in libQt5Widgets.so (only
    // subclasses' overrides are), so route whitelisted dialogs straight to
    // the base QWidget implementation — show/hide both work and the real
    // QDialog::exec's internal setVisible(true) passes through here without
    // recursion (the symbol fallback keeps the dialog on the stack for its
    // whole modal loop).
    if (isEmbedDialog(metaObject()->className()) && inModalStack(this)) {
        this->QWidget::setVisible(visible);
        return;
    }
    // Stage-4 show()-style dialogs: CurvesDialog/FunctionDialog are opened
    // with a NON-modal show() on desktop (WA_DeleteOnClose, no exec), so
    // their first setVisible(true) never passes through the exec()
    // interposer.  Route them into embedDialog() directly instead of
    // reporting them as blocked (AxesDialog never lands here — its exec()
    // override embeds via ohosEmbedDialog(), and embedDialog's internal
    // show() hits the modal-stack passthrough above).
    if (visible && findMainWindow() && isEmbedDialog(metaObject()->className())) {
        // FunctionDialog: its callers configure the dialog AFTER show()
        // returns — functionDialog() (the factory) is followed by
        // setGraph()/setCurveToModify() in addFunctionCurve()/showFunction-
        // Dialog().  A synchronous embed would block until the dialog
        // closes, leaving setGraph() on a hidden dialog (the OK handler
        // then fails with "no graph").  Defer the embed by one event-loop
        // tick: the caller's configuration runs before the dialog appears.
        if (strcmp(metaObject()->className(), "FunctionDialog") == 0) {
            // The reentry guard (modal-stack flag) is not yet set during
            // this ~1-tick window before the callback runs; acceptable,
            // since menu-interaction round-trips are far larger than one
            // tick.
            QTimer::singleShot(0, this, [this]() { embedDialog(this); });
            return;
        }
        // CurvesDialog: showCurvesDialog() does nothing after show(), so a
        // synchronous embed (blocking until the dialog closes) is fine.
        embedDialog(this);
        return;
    }
    if (visible) {
        scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
        BRIDGE_LOG("blocked %s show()", metaObject()->className());
        return;
    }
    // Hiding is safe; bypass QDialog's own logic (never shown anyway).
    this->QWidget::setVisible(visible);
}

// QFileDialog / QColorDialog / QFontDialog override setVisible, so their
// vtable slots would otherwise still point at the real (crashing) code.

void QFileDialog::setVisible(bool visible)
{
    // 2026-08-14: embedded file dialog (route 5).  Pass through while the
    // dialog is being embedded (embedDialog pushes it onto the modal stack
    // before show()); any other QFileDialog stays blocked.  The internal
    // QComboBox popups (lookInCombo/fileTypeCombo) are neutralized by the
    // InputProbe press-swallow (main.cpp) → ArkTS picker, so the probe (a)
    // failure modes (popup window layer / qfiledialog_p showPopup rewrite)
    // no longer trigger.
    if (visible && !inModalStack(this)) {
        scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
        BRIDGE_LOG("blocked QFileDialog show()");
        return;
    }
    this->QWidget::setVisible(visible);
}

void QColorDialog::setVisible(bool visible)
{
    // Pass through while the dialog is being embedded (embedDialog pushes it
    // onto the modal stack before show()); any other QColorDialog stays
    // blocked.
    if (visible && !inModalStack(this)) {
        scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
        return;
    }
    this->QWidget::setVisible(visible);
}

void QFontDialog::setVisible(bool visible)
{
    // Same as QColorDialog above: only embedded dialogs (present on the
    // modal stack) may reach a real setVisible.
    if (visible && !inModalStack(this)) {
        scidavisEmitEvent(QStringLiteral("dialogBlocked"), widgetInfo(this));
        return;
    }
    this->QWidget::setVisible(visible);
}

// ── QMenu ────────────────────────────────────────────────────────────────
// Route A (2026-08-16, plan 2026-08-16-1854): REAL inline QMenu rendering.
// The single-window QPA cannot create the native popup window a real
// QMenu::exec/popup needs (same SIGSEGV family as tooltips), and a
// child-shown QMenu never enters popup state, so its own mouse handling
// never activates actions (Stage-3A probe, 2026-08-15).  Solution: interpose
// QMenu::exec/popup into embedMenuCore() — re-parent the REAL menu to the
// main window with Qt::Widget flags, position it at the click point (screen
// clamped, no finger offset — aligned with Win desktop), show it (pixels
// exactly Qt's), and run a manual QEventLoop that keeps stack-allocated
// menus alive.  InputProbe (main.cpp) routes every mouse event over the
// menus: actionAt() + deferred QAction::trigger() for clicks, outside
// press / Esc close.  Cascades (actions with submenus) embed the submenu
// next to the parent and nest loops; a click outside the top menu closes
// the whole chain.  Qt's own popup state machine is never entered —
// rendering only.
static std::vector<QPointer<QMenu>> g_menuStack;       // parent first, submenus on top
static std::vector<QPointer<QEventLoop>> g_menuLoops;  // parallel, same order

// Anchor override for context-menu deliveries: QCursor::pos() on this QPA
// returns a stale fixed value, so menus that anchor on QCursor::pos()
// (showWindowMenu → exec(QCursor::pos())) render far from the right-click.
// InputProbe records the real context-event global pos here; embedMenuCore()
// consumes it once (the cascade's own computed anchor is untouched).
static bool g_ohosCtxAnchorValid = false;
static QPoint g_ohosCtxAnchor;

// Defined below; HandlePress (InputProbe) opens cascades through it while
// the parent menu's loop is already running, and the standard-text-menu
// hook embeds through it too.
static QAction *embedMenuCore(QMenu *menu, const QPoint &globalPos);

void ohosBridgeSetContextPos(const QPoint &globalPos)
{
    g_ohosCtxAnchorValid = true;
    g_ohosCtxAnchor = globalPos;
}

void ohosBridgeShowStandardMenu(QMenu *menu)
{
    if (!menu)
        return;
    // The context anchor (set by InputProbe right before the call) anchors
    // the menu at the right-click point; the pos argument is a fallback.
    embedMenuCore(menu, menu->mapToGlobal(QPoint(0, 0)));
    // Standard menus are WA_DeleteOnClose — they delete themselves.
}

bool ohosBridgeMenuActive()
{
    return !g_menuStack.empty();
}

// Hide every open menu and quit their loops (innermost first).  Each
// embedMenuCore() pops its own entries when its loop returns, so the stack
// entries are intentionally NOT cleared here.
static void closeAllMenus()
{
    if (g_menuStack.empty())
        return;
    qWarning("[SciDAVis] menu: closing %zu menu(s)", g_menuStack.size());
    for (auto it = g_menuStack.rbegin(); it != g_menuStack.rend(); ++it) {
        if (*it)
            (*it)->hide();
    }
    for (auto it = g_menuLoops.rbegin(); it != g_menuLoops.rend(); ++it) {
        if (*it)
            (*it)->quit();
    }
}

void ohosBridgeCloseMenu()
{
    closeAllMenus();
}

// Deferred trigger: hides+quits already unwound the menu stack; firing from
// the outer event loop lets the press/release pair fully drain and the slot
// run after the menu chain is gone.  QPointer guards actions owned by
// disappearing menus (the table menus use table-level actions, safe either
// way).
static void triggerMenuAction(QAction *a)
{
    QPointer<QAction> act(a);
    QTimer::singleShot(0, [act]() {
        if (act)
            act->trigger();
        else
            qWarning("[SciDAVis] menu action died before deferred trigger");
    });
}

// True when the given menu is already part of the open chain (idempotence
// guard: hover floods must not re-embed the same submenu).
static bool menuInStack(const QMenu *menu)
{
    for (const QPointer<QMenu> &m : g_menuStack) {
        if (m.data() == menu)
            return true;
    }
    return false;
}

// Close every submenu ABOVE `keep` (keep stays open).  Used when a press or
// hover lands on the parent menu: all children close, then the new cascade
// opens — Win "switch" semantics regardless of the chain depth.
static void closeSubmenusAbove(QMenu *keep)
{
    for (int i = int(g_menuStack.size()) - 1; i >= 1; --i) {
        if (g_menuStack[size_t(i)].data() == keep)
            break;
        if (g_menuStack[size_t(i)])
            g_menuStack[size_t(i)]->hide();
        if (g_menuLoops[size_t(i)])
            g_menuLoops[size_t(i)]->quit();
    }
}

// Open a cascade submenu beside its parent, aligned with the triggering
// ACTION ROW's top edge (Win parity — NOT the press point).  Right side of
// the parent by default; flips to the left when the right side is too
// narrow.  Vertically clamped/flipped so the submenu stays on screen.
static void openCascade(QMenu *parent, QAction *a);

// Cascade SWITCH (press/hover on a submenu action): close every submenu
// above `parent`, then open the new cascade ONE EVENT-LOOP TICK later.
// The deferred open is mandatory: quitting the old submenu's loop only
// takes effect after the current press/hover handler returns, and a
// SYNCHRONOUS openCascade would nest the new loop inside that handler —
// the old loop then never unwinds ("zombie" stack entries; repeated
// switches re-embed the same QMenu and corrupt the chain — the round-3
// "clicking a sibling submenu closes the menu" report).  By the time the
// 0ms timer fires, the old loop has unwound (match-erase) and the new
// cascade nests cleanly on the parent.
static void switchCascade(QMenu *parent, QAction *a)
{
    if (!parent || !a)
        return;
    QMenu *sub = a->menu();
    if (!sub)
        return;
    // Already open (or being opened): hover floods / pointer jitter must not
    // close+reopen the SAME cascade — that oscillates (round-3 storm: a
    // jittering device pointer on the 面板 row re-triggered switchCascade
    // every ~15ms and each call closed the just-opened submenu again).
    if (menuInStack(sub))
        return;
    closeSubmenusAbove(parent);
    QPointer<QMenu> pp(parent);
    QPointer<QAction> pa(a);
    QTimer::singleShot(0, [pp, pa]() {
        if (!pp || !pa)
            return;
        QMenu *sub2 = pa->menu();
        if (!sub2 || menuInStack(sub2))
            return; // idempotent / already open
        if (g_menuStack.size() >= 1 && g_menuStack.back().data() == pp.data())
            openCascade(pp.data(), pa.data());
    });
}

// Highlight the action under the cursor on `menu` (Win hover feedback).
// For CASCADE actions the submenu is temporarily detached while calling
// setActiveAction: QMenuPrivate::setCurrentAction runs the popup-state
// branch (openMenu → REAL QMenu::popup → native window) when the action
// has a menu and popupState is Showing/Shown — which our inline menus reach
// after show(); without the detach that crashes the single-window QPA
// (round-3 regression).  The bridge opens cascades manually, so the menu
// pointer is restored right after (before paint — the cascade arrow needs
// it).  This gives cascade rows the same gray hover highlight (user
// round-4 #2).
static void setHoverAction(QMenu *menu, QAction *a)
{
    if (!menu)
        return;
    if (a && a->menu()) {
        QMenu *sub = a->menu();
        a->setMenu(nullptr);
        menu->setActiveAction(a);
        a->setMenu(sub);
    } else {
        menu->setActiveAction(a);
    }
    menu->update();
}

// Bottom limit for inline menus (user round-5 #3): a menu may never extend
// past the Qt content-area bottom, which sits exactly at the ArkTS
// bottom-toolbar top (setChromeInsets pushes the bottom chrome inset into
// the QMainWindow).  Clamping against availableGeometry() alone let menus
// slide UNDER the opaque toolbar when the right-click point was near the
// screen bottom; Win flips the menu upward instead, so the hidden part
// must move above the limit rather than stay cut off.
// 2026-08-17 (user round-8 #2): the Qt content bottom can still disagree
// with the visible ArkTS bottom-toolbar top by a few px.  ArkTS therefore
// also sends the MEASURED toolbar top (setMenuBottomLimit) and that explicit
// value wins when available — the menu bottom then aligns to the real
// overlay even when Qt-window vs ArkTS-window alignment is off.
static int g_explicitMenuBottomGlobalLimit = -1;

void scidavisSetMenuBottomLimit(int globalY)
{
    g_explicitMenuBottomGlobalLimit = globalY;
}

static int menuBottomGlobalLimit()
{
    const QRect screen = qApp->primaryScreen()->availableGeometry();
    if (g_explicitMenuBottomGlobalLimit >= screen.top()
        && g_explicitMenuBottomGlobalLimit <= screen.bottom()) {
        return qMax(screen.top() + 1, g_explicitMenuBottomGlobalLimit - 1);
    }
    QWidget *mw = findMainWindow();
    if (mw) {
        // The Qt window is FULL-SCREEN on this QPA; the ArkTS bottom
        // toolbar lives in the bottom chrome inset pushed by
        // setChromeInsets → setContentsMargins(0, top, 0, bottom) — so the
        // Qt CONTENT bottom = window bottom − contentsMargins().bottom().
        const int gb = mw->mapToGlobal(QPoint(0, mw->height())).y()
                       - mw->contentsMargins().bottom();
        if (gb > screen.top() && gb <= screen.bottom())
            return qMax(screen.top() + 1, gb - 1);
        qWarning("[SciDAVis] menu: window bottom %d outside (top=%d bottom=%d) — "
                 "screen limit", gb, screen.top(), screen.bottom());
    }
    return screen.bottom();
}

static void openCascade(QMenu *parent, QAction *a)
{
    QMenu *sub = a ? a->menu() : nullptr;
    if (!sub || !parent)
        return;
    const QRect ag = parent->actionGeometry(a);
    const QRect prect = parent->geometry();
    const QRect screen = qApp->primaryScreen()->availableGeometry();
    const int subW = qMax(sub->sizeHint().width(), 80);
    const int subH = qMax(sub->sizeHint().height(), 32);
    // x: parent's right edge, action-row aligned (flip left if no room)
    QPoint anchor = parent->mapToGlobal(QPoint(prect.width(), ag.top()));
    if (anchor.x() + subW > screen.right())
        anchor = parent->mapToGlobal(QPoint(0, ag.top()));
    if (anchor.x() + subW > screen.right())
        anchor.setX(screen.right() - subW);
    // y: action row top; shift up when the submenu would leave the Qt
    // content bottom (the ArkTS toolbar top — user round-5 #3), then the
    // screen bottom.
    const int bottomLimit = menuBottomGlobalLimit();
    if (anchor.y() + subH > bottomLimit)
        anchor.setY(bottomLimit - subH);
    anchor.setX(qMax(screen.left(), anchor.x()));
    anchor.setY(qMax(screen.top(), anchor.y()));
    qWarning("[SciDAVis] menu: cascade open '%s' (→ %d menus)",
             qPrintable(a->text()), int(g_menuStack.size() + 1));
    embedMenuCore(sub, anchor);
}

// True when the app UI language resolves to Chinese (three-mode /Language
// setting).  Used to translate the few strings the shipped zh catalogue
// lacks — wording follows the ArkTS menu strings (edit_undo=撤消 etc.).
static QString g_langMode; // mirrored from ApplicationWindow::appLanguage

void ohosSetLanguageMode(const char *mode)
{
    g_langMode = QString::fromUtf8(mode ? mode : "");
}

bool ohosBridgeChineseActive()
{
    // In-memory mirror first (authoritative, synced by readSettings/
    // switchToLanguage); QSettings + device locale are only the cold-start
    // fallback (QSettings persistence is unreliable on this platform).
    if (g_langMode == QLatin1String("en"))
        return false;
    if (g_langMode == QLatin1String("zh-cn"))
        return true;
    const QString mode =
        QSettings().value(QStringLiteral("/Language"), QStringLiteral("system")).toString();
    if (mode == QLatin1String("en"))
        return false;
    if (mode == QLatin1String("zh-cn"))
        return true;
    // "system": follow the device locale
    return QLocale::system().name().startsWith(QLatin1String("zh"), Qt::CaseInsensitive);
}

// Hover routing (user round-3, Win parity): highlight the row under the
// cursor; hovering a submenu action auto-opens its cascade; leaving the
// cascade (onto the parent or outside) closes it.  Touch devices have no
// hover — the press path keeps working unchanged.  The coordinates are
// de-duplicated by InputProbe's move handling, so this runs on real moves.
void ohosBridgeMenuHover(const QPoint &globalPos)
{
    if (g_menuStack.empty())
        return;
    // Same-position de-dup: pointer jitter floods the same coordinates over
    // and over (device input errors resend the last pointer position); the
    // first event already ran the state machine for this position.
    static QPoint s_lastHoverPos;
    if (globalPos == s_lastHoverPos)
        return;
    s_lastHoverPos = globalPos;
    // User round-4 #3/#4: hover ONLY highlights and auto-opens cascades —
    // it NEVER closes submenus.  A cascade closes only when the user CLICKS
    // another row (trigger/switch), clicks outside, or closes the menu
    // (Esc/trigger).  This removes the flicker (no hover-driven close+reopen
    // oscillation) and the "only special positions expand" feel (the expand
    // condition is simply "the cursor is on a cascade row of any open menu").
    // Walk the chain from the TOP down: the cursor is in at most one menu.
    for (int i = int(g_menuStack.size()) - 1; i >= 0; --i) {
        QMenu *m = g_menuStack[size_t(i)].data();
        if (!m)
            continue;
        const QPoint local = m->mapFromGlobal(globalPos);
        if (!m->rect().contains(local))
            continue;
        QAction *a = m->actionAt(local);
        if (a && !a->isSeparator() && a->isEnabled()) {
            setHoverAction(m, a);
            // Auto-open the cascade when hovering its row (idempotent via
            // switchCascade's deferred open + menuInStack guard).  If the
            // cursor moves to a DIFFERENT cascade row, switchCascade closes
            // the old one and opens the new — clicking is NOT required for
            // the switch (hover follows the cursor, round-3 #3).
            if (a->menu() && !menuInStack(a->menu()))
                switchCascade(m, a);
        } else {
            setHoverAction(m, nullptr); // blank / separator: no highlight
        }
        return;
    }
    // Cursor outside every open menu: clear the top highlight only —
    // submenus stay open until an explicit close (round-4 #4).
    if (QMenu *top = g_menuStack.back().data())
        setHoverAction(top, nullptr);
}

bool ohosBridgeMenuHandlePress(const QPoint &globalPos)
{
    if (g_menuStack.empty()) {
        qWarning("[SciDAVis] menu press while inactive — swallowed");
        return true;
    }
    QMenu *menu = g_menuStack.back().data();
    if (!menu) {
        closeAllMenus();
        return true;
    }
    const QPoint local = menu->mapFromGlobal(globalPos);
    if (!menu->rect().contains(local)) {
        // Outside the TOP menu.  With a submenu open, a press INSIDE the
        // PARENT menu follows Win semantics: another submenu action switches
        // the cascade, a plain action triggers and closes everything, a
        // blank row closes the child only.  Outside both → close all.
        if (g_menuStack.size() > 1) {
            QMenu *parent = g_menuStack[g_menuStack.size() - 2].data();
            if (parent) {
                const QPoint plocal = parent->mapFromGlobal(globalPos);
                if (parent->rect().contains(plocal)) {
                    QAction *pa = parent->actionAt(plocal);
                    qWarning("[SciDAVis] menu: press on parent row '%s' while submenu open",
                             pa ? qPrintable(pa->text()) : "blank");
                    closeSubmenusAbove(parent); // any depth → all children close
                    if (pa && !pa->isSeparator() && pa->isEnabled()) {
                        setHoverAction(parent, pa);
                        if (pa->menu()) {
                            // switch: close children, open the new cascade
                            // one tick later (zombie-loop fix, round-3)
                            switchCascade(parent, pa);
                        } else {
                            qWarning("[SciDAVis] menu: triggering '%s' enabled=%d checkable=%d checked=%d",
                                     qPrintable(pa->text()), int(pa->isEnabled()),
                                     int(pa->isCheckable()), int(pa->isChecked()));
                            closeAllMenus();
                            triggerMenuAction(pa);
                        }
                    }
                    return true;
                }
            }
        }
        qWarning("[SciDAVis] menu: outside press (%d,%d) closes %zu menu(s)",
                 globalPos.x(), globalPos.y(), g_menuStack.size());
        closeAllMenus();
        return true;
    }
    QAction *a = menu->actionAt(local);
    if (!a || a->isSeparator() || !a->isEnabled()) {
        qWarning("[SciDAVis] menu: press @local(%d,%d) on %s — no-op",
                 local.x(), local.y(), a ? qPrintable(a->text()) : "blank");
        setHoverAction(menu, nullptr);
        return true; // blank / separator / disabled: keep the menu open
    }
    setHoverAction(menu, a);
    if (a->menu()) {
        // Cascade: embed the submenu beside the parent, aligned with the
        // action row (touch: press opens it; mouse: hover opens it earlier).
        switchCascade(menu, a);
        return true;
    }
    qWarning("[SciDAVis] menu: triggering '%s' enabled=%d checkable=%d checked=%d",
             qPrintable(a->text()), int(a->isEnabled()),
             int(a->isCheckable()), int(a->isChecked()));
    closeAllMenus();
    triggerMenuAction(a);
    return true;
}

static QAction *embedMenuCore(QMenu *menu, const QPoint &globalPos)
{
    QWidget *mainWindow = findMainWindow();
    if (!mainWindow)
        return nullptr;

    // Restore ownership after the loop: member menus (d_plot_menu etc.)
    // must not drift to the main window's ownership when their window dies.
    QWidget *prevParent = menu->parentWidget();
    menu->setParent(mainWindow);
    menu->setWindowFlags(Qt::Widget); // never Qt::Popup / Qt::Window
    menu->setAutoFillBackground(true);
    menu->setAttribute(Qt::WA_StyledBackground, true);

    // Dynamic action names (Win parity, user round-2): desktop menus refresh
    // their item texts on QMenu::aboutToShow (e.g. Table::adjustActionNames
    // → tr("Show Comments")/tr("Hide Controls")...), which Qt only emits
    // from the popup state machine we never enter — so menus built from
    // literals ("Show/Hide comments") or stale tr() text would render
    // wrong/mixed-language.  Emit the signal ourselves before sizing/show.
    // aboutToShow is invokable via the meta-object (signals are methods).
    if (!QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection))
        qWarning("[SciDAVis] embedMenu '%s': aboutToShow emit failed",
                 qPrintable(menu->objectName()));

    // Hover highlight color (user round-3): the Fusion selected-item color
    // is a saturated blue; the user asked for a GRAY highlight ("背景变灰").
    // A light-gray Highlight with dark HighlightedText keeps the row
    // readable on the white menu background.
    {
        QPalette pal = menu->palette();
        pal.setColor(QPalette::Highlight, QColor(0xe0, 0xe0, 0xe0));
        pal.setColor(QPalette::HighlightedText, QColor(0x20, 0x20, 0x20));
        menu->setPalette(pal);
    }

    // zh display-level fallback translation (user round-3/4): the shipped zh
    // catalogue lacks a few strings ("Paste transposed") and Qt's standard
    // text-menu words (Undo/Redo/Cut/Copy/Paste — no qt_zh catalogue;
    // Delete/Select All translate via QPlatformTheme entries in the app qm,
    // hence a mixed menu).  In Chinese mode fill ALL of them with the ArkTS
    // menu wording (edit_undo=撤消, table_paste_transposed=粘贴转置 …).
    // The changes are DISPLAY-LEVEL: originals are restored after the menu
    // closes, so long-lived actions (Table's Paste transposed) never keep a
    // stale language — switching to English shows the English source again
    // (user round-4 #5).  Shortcut text embedded as "Undo\tCtrl+Z" is kept
    // (round-4 #1); items without one get no suffix.
    std::vector<std::pair<QPointer<QAction>, QString>> zhRestore;
    if (ohosBridgeChineseActive()) {
        for (QAction *a2 : menu->actions()) {
            if (a2->isSeparator())
                continue;
            const QKeySequence sc = a2->shortcut();
            const QString suf = [&]() {
                const int ti = a2->text().indexOf(QLatin1Char('\t'));
                return ti >= 0 ? a2->text().mid(ti) : QString();
            }();
            QString t = a2->text();
            const int tab = t.indexOf(QLatin1Char('\t'));
            if (tab >= 0)
                t.truncate(tab);
            t.remove(QLatin1Char('&'));
            QString zhText;
            if (t == QLatin1String("Paste transposed"))
                zhText = QStringLiteral("粘贴转置");
            else if (sc == QKeySequence::Undo || t == QLatin1String("Undo"))
                zhText = QStringLiteral("撤消");
            else if (sc == QKeySequence::Redo || t == QLatin1String("Redo"))
                zhText = QStringLiteral("重做");
            else if (sc == QKeySequence::Cut || t == QLatin1String("Cut"))
                zhText = QStringLiteral("剪切");
            else if (sc == QKeySequence::Copy || t == QLatin1String("Copy"))
                zhText = QStringLiteral("复制");
            else if (sc == QKeySequence::Paste || t == QLatin1String("Paste"))
                zhText = QStringLiteral("粘贴");
            else if (sc == QKeySequence::Delete || t == QLatin1String("Delete")
                     || t == QStringLiteral("删除"))
                zhText = QStringLiteral("删除");
            else if (sc == QKeySequence::SelectAll || t == QLatin1String("Select All")
                     || t == QStringLiteral("全选"))
                zhText = QStringLiteral("全选");
            // Graph layer/button context menus (showLayerContextMenu /
            // showLayerButtonContextMenu — Copy/Export/Print submenus with
            // Layer/Window leaves): the shipped zh catalogue HAS these
            // entries but on-device reports (user round-5 #2) show them
            // English after a zh switch, so cover them display-level too —
            // wording identical to the TS catalogue so a fixed qm renders
            // the same text.  Exact full-text match only (a bare "Window"
            // action inside "Moving Window &Average..." keeps its own
            // translation).
            else if (t == QLatin1String("Export"))
                zhText = QStringLiteral("导出");
            else if (t == QLatin1String("Print"))
                zhText = QStringLiteral("打印");
            else if (t == QLatin1String("Print All Plots"))
                zhText = QStringLiteral("打印全部绘图");
            else if (t == QLatin1String("Layer"))
                zhText = QStringLiteral("图层");
            else if (t == QLatin1String("Window"))
                zhText = QStringLiteral("窗口");
            else if (t == QLatin1String("Geometry..."))
                zhText = QStringLiteral("几何...");
            else if (t == QLatin1String("Properties..."))
                zhText = QStringLiteral("属性...");
            else if (t == QLatin1String("Delete Layer"))
                zhText = QStringLiteral("删除图层");
            else if (t == QLatin1String("Paste Layer"))
                zhText = QStringLiteral("粘贴图层");
            else if (t == QLatin1String("Paste Text"))
                zhText = QStringLiteral("粘贴文本");
            else if (t == QLatin1String("Paste Line/Arrow"))
                zhText = QStringLiteral("粘贴线/箭头");
            else if (t == QLatin1String("Paste Image"))
                zhText = QStringLiteral("粘贴图形");
            if (!zhText.isEmpty()) {
                zhRestore.emplace_back(QPointer<QAction>(a2), a2->text());
                qWarning("[zh-fix] '%s' -> '%s' (sc=%s)", qPrintable(a2->text()),
                         qPrintable(zhText + suf), qPrintable(sc.toString()));
                a2->setText(zhText + suf);
            }
        }
    }

    // Icon normalization (user round-2): single-xpm icons (cut.xpm 18x18…)
    // and dual-pixmap icons (16x16+32x32) render at their NATIVE sizes in
    // the menu (QPixmapIconEngine returns the best-fit pixmap and the menu
    // draws it unscaled), so differently-sized sources look misaligned
    // (全选 ≈ 32px vs 剪切 ≈ 18px on device).  Re-wrap every icon to ONE
    // pixmap of the SAME size — the LARGEST native size in this menu (the
    // 32x32 class, i.e. the 全选 reference) — so all items match the
    // reference while small sources scale UP cleanly.
    {
        int iconTarget = 0;
        for (QAction *a2 : menu->actions()) {
            if (a2->isSeparator() || a2->icon().isNull())
                continue;
            const QList<QSize> szs = a2->icon().availableSizes();
            for (const QSize &s : szs)
                iconTarget = qMax(iconTarget, qMax(s.width(), s.height()));
        }
        if (iconTarget <= 0)
            iconTarget = menu->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, menu);
        qWarning("[SciDAVis] embedMenu '%s': iconTarget=%d PM_SmallIconSize=%d",
                 qPrintable(menu->objectName()), iconTarget,
                 menu->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, menu));
        if (iconTarget > 8) {
            const QSize target(iconTarget, iconTarget);
            for (QAction *a2 : menu->actions()) {
                if (a2->isSeparator() || a2->icon().isNull())
                    continue;
                QPixmap pm = a2->icon().pixmap(target);
                if (pm.isNull())
                    continue;
                if (pm.size() != target)
                    pm = pm.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                a2->setIcon(QIcon(pm));
            }
        }
    }

    // Size: sizeHint covers the widest action row; a freshly built stack
    // menu (or one with submenus) is occasionally missed → manual floor.
    QSize sh = menu->sizeHint();
    if (sh.width() < 40 || sh.height() < 16) {
        int w = 0;
        const QFontMetrics fm(menu->font());
        for (QAction *a2 : menu->actions()) {
            if (a2->isSeparator())
                continue;
            int aw = fm.horizontalAdvance(a2->text());
            if (a2->menu())
                aw += 28; // cascade arrow
            if (!a2->shortcut().isEmpty())
                aw += fm.horizontalAdvance(QStringLiteral("    Ctrl+X"));
            w = qMax(w, aw);
        }
        sh = QSize(qMax(w + 24, 120), qMax(menu->actions().size() * 30 + 8, 32));
        qWarning("[SciDAVis] embedMenu '%s': sizeHint floor → %dx%d",
                 qPrintable(menu->objectName()), sh.width(), sh.height());
    }

    // Anchor: prefer the context-menu event's real global pos when this
    // menu was opened from a right-click delivery (QCursor::pos() is stale
    // on this QPA); otherwise the caller-provided point (submenu anchors).
    const QRect screen = qApp->primaryScreen()->availableGeometry();
    QPoint anchor = g_ohosCtxAnchorValid ? g_ohosCtxAnchor : globalPos;
    g_ohosCtxAnchorValid = false; // consumed — cascade submenus compute their own
    if (anchor.x() + sh.width() > screen.right())
        anchor.setX(screen.right() - sh.width());
    // y: menu bottom must clear the Qt content bottom (ArkTS toolbar top,
    // user round-5 #3) — flip the menu upward instead of sliding under the
    // toolbar band; the screen bottom remains the outer fallback.
    const int bottomLimit = menuBottomGlobalLimit();
    if (anchor.y() + sh.height() > bottomLimit)
        anchor.setY(bottomLimit - sh.height());
    anchor.setX(qMax(screen.left(), anchor.x()));
    anchor.setY(qMax(screen.top(), anchor.y()));
    const QPoint contentOrigin = mainWindow->mapToGlobal(QPoint(0, 0));
    const QPoint localPos = anchor - contentOrigin;
    menu->setGeometry(localPos.x(), localPos.y(), sh.width(), sh.height());

    // Register BEFORE show: InputProbe needs the modal state to route
    // clicks (and the generic dialog-modal isolation yields to the menu
    // route — main.cpp consults ohosBridgeMenuActive()).
    const bool firstModal = g_modalStack.empty();
    g_modalStack.push_back(menu);
    g_modalDialogActive = true;
    g_menuStack.push_back(menu);
    if (firstModal) {
        // Same input-starvation guard as embedDialog: pause shell polling
        // while the nested loop blocks the Qt thread.
        scidavisEmitEvent(QStringLiteral("modalStarted"));
        scidavis_mark_modal_started();
    }

    menu->show();
    menu->raise();

    QEventLoop loop;
    g_menuLoops.push_back(&loop);
    QAction *result = nullptr;
    QObject::connect(menu, &QObject::destroyed, &loop, [&]() { loop.quit(); });
    QObject::connect(menu, &QMenu::triggered, &loop, [&](QAction *a2) {
        result = a2;
        loop.quit();
    });
    loop.exec();
    // Match-erase (NOT plain pop_back): a cascade SWITCH on the parent can
    // push a new submenu while this (old) submenu's loop is still unwinding;
    // popping by pointer removes exactly this menu's entries, so the new
    // submenu's stack entries stay intact.
    for (auto it = g_menuLoops.begin(); it != g_menuLoops.end(); ++it) {
        if (it->data() == &loop) {
            g_menuLoops.erase(it);
            break;
        }
    }
    for (auto it = g_menuStack.begin(); it != g_menuStack.end(); ++it) {
        if (it->data() == menu) {
            g_menuStack.erase(it);
            break;
        }
    }
    for (auto it = g_modalStack.begin(); it != g_modalStack.end(); ++it) {
        if (*it == menu) {
            g_modalStack.erase(it);
            break;
        }
    }
    g_modalDialogActive = !g_modalStack.empty();
    const bool lastModal = g_modalStack.empty();
    if (menu->isVisible())
        menu->hide();
    // Restore display-level zh translations (round-4 #5): long-lived actions
    // keep their source-language text once the menu is gone, so a language
    // switch shows the correct language next time the menu opens.
    for (auto &pr : zhRestore) {
        if (pr.first) {
            qWarning("[zh-restore] '%s' -> '%s'", qPrintable(pr.first->text()),
                     qPrintable(pr.second));
            pr.first->setText(pr.second);
        }
    }
    // a6 QPA backingstore: hide() does not refresh — repaint the menu rect
    // and request an update flush (same as the embedDialog cleanup path).
    {
        const QRect menuRect = menu->geometry();
        mainWindow->repaint(menuRect);
        if (QWindow *win = mainWindow->windowHandle())
            win->requestUpdate();
        qWarning("[SciDAVis] embedMenu cleanup: repainted %d,%d,%dx%d result=%p",
                 menuRect.x(), menuRect.y(), menuRect.width(), menuRect.height(),
                 static_cast<void *>(result));
    }
    if (lastModal) {
        scidavisEmitEvent(QStringLiteral("modalEnded"));
        scidavis_mark_modal_ended();
    }
    // Restore the pre-embed parent so long-lived member menus keep their
    // original ownership (a hidden reparent to nullptr is harmless — no
    // window is created until show()).
    if (prevParent && menu->parentWidget() == mainWindow)
        menu->setParent(prevParent);
    return result;
}
QAction *QMenu::exec()
{
    return embedMenuCore(this, QCursor::pos());
}

QAction *QMenu::exec(const QPoint &pos, QAction *at)
{
    Q_UNUSED(at);
    return embedMenuCore(this, pos);
}

void QMenu::popup(const QPoint &pos, QAction *at)
{
    Q_UNUSED(at);
    embedMenuCore(this, pos);
}

QAction *QMenu::exec(QList<QAction *> actions, const QPoint &pos, QAction *at, QWidget *parent)
{
    Q_UNUSED(pos);
    Q_UNUSED(at);
    QJsonObject p = widgetInfo(parent);
    p[QStringLiteral("items")] = describeActions(actions, 0);
    scidavisEmitEvent(QStringLiteral("contextMenu"), p);
    BRIDGE_LOG("static QMenu::exec blocked (%d actions)", actions.size());
    return nullptr;
}

// ── QMessageBox ──────────────────────────────────────────────────────────
// Phase 1 (2026-08-12): construct the REAL QMessageBox and run it through
// the generic embedDialog template (spec 13).  Real Qt's QMessageBox::exec()
// override is NOT called: the single-window QPA cannot render a top-level
// dialog, and the dialog must go through embedDialog's setup — its internal
// setVisible(true) dispatch lands in the interposed QDialog::setVisible,
// which the whitelist guard blocks outside the modal stack, so exec() would
// run a modal loop over an invisible dialog.  embedDialog runs the manual
// loop instead; the clicked button is read back via
// clickedButton()/standardButton() (the same pattern Qt's own statics use
// after exec()).  Fallback: no main window (e.g. during startup) →
// historical event-reporting path with the safe default button.

QMessageBox::StandardButton QMessageBox::information(QWidget *parent, const QString &title,
                                                     const QString &text, StandardButtons buttons,
                                                     StandardButton defaultButton)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(title);
        box.setText(text);
        box.setStandardButtons(buttons);
        box.setDefaultButton(defaultButton);
        embedDialog(&box);
        // This Qt fork predates clickedStandardButton() (Qt 5.15): map the
        // clicked button through standardButton() instead.
        return box.clickedButton() ? box.standardButton(box.clickedButton())
                                   : QMessageBox::NoButton;
    }
    return emitMessage("information", parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton QMessageBox::question(QWidget *parent, const QString &title,
                                                  const QString &text, StandardButtons buttons,
                                                  StandardButton defaultButton)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(title);
        box.setText(text);
        box.setStandardButtons(buttons);
        box.setDefaultButton(defaultButton);
        embedDialog(&box);
        // This Qt fork predates clickedStandardButton() (Qt 5.15): map the
        // clicked button through standardButton() instead.
        return box.clickedButton() ? box.standardButton(box.clickedButton())
                                   : QMessageBox::NoButton;
    }
    return emitMessage("question", parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton QMessageBox::warning(QWidget *parent, const QString &title,
                                                 const QString &text, StandardButtons buttons,
                                                 StandardButton defaultButton)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(title);
        box.setText(text);
        box.setStandardButtons(buttons);
        box.setDefaultButton(defaultButton);
        embedDialog(&box);
        // This Qt fork predates clickedStandardButton() (Qt 5.15): map the
        // clicked button through standardButton() instead.
        return box.clickedButton() ? box.standardButton(box.clickedButton())
                                   : QMessageBox::NoButton;
    }
    return emitMessage("warning", parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton QMessageBox::critical(QWidget *parent, const QString &title,
                                                  const QString &text, StandardButtons buttons,
                                                  StandardButton defaultButton)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Critical);
        box.setWindowTitle(title);
        box.setText(text);
        box.setStandardButtons(buttons);
        box.setDefaultButton(defaultButton);
        embedDialog(&box);
        // This Qt fork predates clickedStandardButton() (Qt 5.15): map the
        // clicked button through standardButton() instead.
        return box.clickedButton() ? box.standardButton(box.clickedButton())
                                   : QMessageBox::NoButton;
    }
    return emitMessage("critical", parent, title, text, buttons, defaultButton);
}

// ── Obsolete (Qt4-compat) QMessageBox statics ─────────────────────────────
// 2026-08-14 crash fix (cppcrash 17:10:40): ApplicationWindow::saveFolder
// calls the OBSOLETE int-returning overloads
//   int QMessageBox::critical(parent, title, text, int button0, int button1,
//                             int button2 = 0)
// (button params carry the obsolete Default/Escape flag bits, see
// qmessagebox.h).  These were NOT interposed — the call resolved to the real
// libQt5Widgets.so implementation, which ran the REAL QDialog::exec →
// QWidget::create → second top-level window → QPA SIGSEGV
// (getJsFunction NULL).  Interpose the int overloads (and the obsolete
// text-button overloads) and route them through the same embedDialog path.
// Returns the clicked standard button (int) — the same contract as Qt's
// showOldMessageBox.

static QMessageBox::StandardButton interposeOldMessageBox(
        QMessageBox::Icon icon, QWidget *parent, const QString &title,
        const QString &text, int button0, int button1, int button2)
{
    // Decode the obsolete flag bits (Default=0x100, Escape=0x200,
    // ButtonMask=~0x300).  Qt's showOldMessageBox treats each non-zero int
    // as one button with optional Default/Escape flag; button0's Default
    // flag selects the default button, any Escape flag selects the escape
    // button.  Map onto the modern API and embed.
    const int kDefault = 0x00000100;
    const int kEscape = 0x00000200;
    const int kButtonMask = ~0x00000300;
    QMessageBox::StandardButtons buttons = QMessageBox::NoButton;
    QMessageBox::StandardButton defaultBtn = QMessageBox::NoButton;
    QMessageBox::StandardButton escapeBtn = QMessageBox::NoButton;
    const int ints[3] = { button0, button1, button2 };
    for (int i = 0; i < 3; ++i) {
        const int raw = ints[i];
        if (!raw)
            continue;
        const QMessageBox::StandardButton b =
                QMessageBox::StandardButton(raw & kButtonMask);
        if (b != QMessageBox::NoButton)
            buttons |= b;
        if (raw & kDefault)
            defaultBtn = b;
        if (raw & kEscape)
            escapeBtn = b;
    }
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(icon);
        box.setWindowTitle(title);
        box.setText(text);
        box.setStandardButtons(buttons);
        if (defaultBtn != QMessageBox::NoButton)
            box.setDefaultButton(defaultBtn);
        if (escapeBtn != QMessageBox::NoButton)
            box.setEscapeButton(escapeBtn);
        embedDialog(&box);
        return box.clickedButton() ? box.standardButton(box.clickedButton())
                                   : QMessageBox::NoButton;
    }
    // No main window (startup): historical event path with the safe button.
    return emitMessage(icon == QMessageBox::Critical ? "critical"
                       : icon == QMessageBox::Warning ? "warning"
                       : icon == QMessageBox::Question ? "question"
                                                       : "information",
                       parent, title, text, buttons, defaultBtn);
}

int QMessageBox::information(QWidget *parent, const QString &title, const QString &text,
                             int button0, int button1, int button2)
{
    return int(interposeOldMessageBox(QMessageBox::Information, parent, title, text,
                                      button0, button1, button2));
}

int QMessageBox::question(QWidget *parent, const QString &title, const QString &text,
                          int button0, int button1, int button2)
{
    return int(interposeOldMessageBox(QMessageBox::Question, parent, title, text,
                                      button0, button1, button2));
}

int QMessageBox::warning(QWidget *parent, const QString &title, const QString &text,
                         int button0, int button1, int button2)
{
    return int(interposeOldMessageBox(QMessageBox::Warning, parent, title, text,
                                      button0, button1, button2));
}

int QMessageBox::critical(QWidget *parent, const QString &title, const QString &text,
                          int button0, int button1, int button2)
{
    return int(interposeOldMessageBox(QMessageBox::Critical, parent, title, text,
                                      button0, button1, button2));
}

void QMessageBox::about(QWidget *parent, const QString &title, const QString &text)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setWindowTitle(title);
        box.setText(text);
        // Real Qt's about() shows the application window icon instead of
        // the Information glyph; cheap here (windowIcon is already loaded,
        // no resource fetch) — fall back to the glyph if no icon is set.
        const QPixmap pm = qApp->windowIcon().pixmap(64, 64);
        if (!pm.isNull())
            box.setIconPixmap(pm);
        else
            box.setIcon(QMessageBox::Information);
        box.setStandardButtons(QMessageBox::Ok);
        embedDialog(&box);
        return;
    }
    emitMessage("about", parent, title, text, QMessageBox::Ok, QMessageBox::Ok);
}

void QMessageBox::aboutQt(QWidget *parent, const QString &title)
{
    if (findMainWindow()) {
        QMessageBox box(parent);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(title.isEmpty() ? QStringLiteral("About Qt") : title);
        box.setText(QStringLiteral("Qt %1").arg(QLatin1String(qVersion())));
        box.setStandardButtons(QMessageBox::Ok);
        embedDialog(&box);
        return;
    }
    emitMessage("about", parent, title.isEmpty() ? QStringLiteral("About Qt") : title,
                QStringLiteral("Qt %1").arg(QLatin1String(qVersion())), QMessageBox::Ok,
                QMessageBox::Ok);
}

// Obsolete 8-arg overloads (2026-08-13/14): SciDAVis calls these text-button
// forms (window-close confirm "Do you want to hide or delete 'Table1' ?",
// saveFolder errors, ...), which return the CLICKED BUTTON INDEX (0/1/2),
// not a StandardButton.  The library implementations would run a real
// QMessageBox::exec() whose top-level-window path the single-window QPA
// cannot render (cppcrash 17:10:40 — saveFolder's int-critical resolved to
// libQt5Widgets.so's real exec → QWidget::create → getJsFunction NULL), so
// embed them like the 5-arg statics (spec 13/14; QMessageBox size policy
// 540×2.10 already in kDialogSizePolicies).  Button roles mirror
// QMessageBoxPrivate::showOldMessageBox: AcceptRole / RejectRole /
// DestructiveRole; default/escape indices are honored so Enter/Esc map to
// the right button.
static int interposeOldTextMessageBox(QMessageBox::Icon icon, QWidget *parent,
                                      const QString &title, const QString &text,
                                      const QString &button0Text, const QString &button1Text,
                                      const QString &button2Text, int defaultButtonNumber,
                                      int escapeButtonNumber)
{
    if (!findMainWindow()) {
        // Defensive fallback (startup): report + safest answer (Cancel).
        QJsonObject p = widgetInfo(parent);
        p[QStringLiteral("severity")] =
                icon == QMessageBox::Critical ? QStringLiteral("critical")
                : icon == QMessageBox::Warning ? QStringLiteral("warning")
                : icon == QMessageBox::Question ? QStringLiteral("question")
                                                : QStringLiteral("information");
        p[QStringLiteral("title")] = title;
        p[QStringLiteral("text")] = text;
        p[QStringLiteral("chosen")] = 2;
        scidavisEmitEvent(QStringLiteral("message"), p);
        BRIDGE_LOG("QMessageBox (8-arg) blocked: %s", title.toUtf8().constData());
        return escapeButtonNumber >= 0 ? escapeButtonNumber : 2;
    }
    QMessageBox box(parent);
    box.setIcon(icon);
    box.setWindowTitle(title);
    box.setText(text);
    box.setStandardButtons(QMessageBox::NoButton);
    QPushButton *btn[3] = { nullptr, nullptr, nullptr };
    if (!button0Text.isEmpty())
        btn[0] = box.addButton(button0Text, QMessageBox::AcceptRole);
    if (!button1Text.isEmpty())
        btn[1] = box.addButton(button1Text, QMessageBox::RejectRole);
    if (!button2Text.isEmpty())
        btn[2] = box.addButton(button2Text, QMessageBox::DestructiveRole);
    if (defaultButtonNumber >= 0 && defaultButtonNumber < 3 && btn[defaultButtonNumber])
        box.setDefaultButton(btn[defaultButtonNumber]);
    if (escapeButtonNumber >= 0 && escapeButtonNumber < 3 && btn[escapeButtonNumber])
        box.setEscapeButton(btn[escapeButtonNumber]);
    embedDialog(&box);
    const QPushButton *clicked = static_cast<const QPushButton *>(box.clickedButton());
    for (int i = 0; i < 3; ++i) {
        if (clicked && clicked == btn[i])
            return i;
    }
    // Closed via the title-bar × (or Esc with no escape button): safest
    // reading is Cancel — the window stays open.
    return escapeButtonNumber >= 0 ? escapeButtonNumber : 2;
}

int QMessageBox::information(QWidget *parent, const QString &title, const QString &text,
                             const QString &button0Text, const QString &button1Text,
                             const QString &button2Text, int defaultButtonNumber,
                             int escapeButtonNumber)
{
    return interposeOldTextMessageBox(QMessageBox::Information, parent, title, text,
                                      button0Text, button1Text, button2Text,
                                      defaultButtonNumber, escapeButtonNumber);
}

int QMessageBox::question(QWidget *parent, const QString &title, const QString &text,
                          const QString &button0Text, const QString &button1Text,
                          const QString &button2Text, int defaultButtonNumber,
                          int escapeButtonNumber)
{
    return interposeOldTextMessageBox(QMessageBox::Question, parent, title, text,
                                      button0Text, button1Text, button2Text,
                                      defaultButtonNumber, escapeButtonNumber);
}

int QMessageBox::warning(QWidget *parent, const QString &title, const QString &text,
                         const QString &button0Text, const QString &button1Text,
                         const QString &button2Text, int defaultButtonNumber,
                         int escapeButtonNumber)
{
    return interposeOldTextMessageBox(QMessageBox::Warning, parent, title, text,
                                      button0Text, button1Text, button2Text,
                                      defaultButtonNumber, escapeButtonNumber);
}

int QMessageBox::critical(QWidget *parent, const QString &title, const QString &text,
                          const QString &button0Text, const QString &button1Text,
                          const QString &button2Text, int defaultButtonNumber,
                          int escapeButtonNumber)
{
    return interposeOldTextMessageBox(QMessageBox::Critical, parent, title, text,
                                      button0Text, button1Text, button2Text,
                                      defaultButtonNumber, escapeButtonNumber);
}

// ── QFileDialog statics ──────────────────────────────────────────────────
// 2026-08-14 (route 5, embedded file dialogs): construct a REAL QFileDialog
// with DontUseNativeDialog and run it through the generic embedDialog
// template (rendered inside the main window).  Probe (a) failure modes are
// neutralized: the internal QComboBox popups (lookInCombo/fileTypeCombo)
// are swallowed by the InputProbe press-interception (main.cpp) and shown
// as ArkTS pickers instead, so no popup window layer is created.  The
// initial directory is the ArkTS-authorized work folder (set via
// setWorkFolderPath command; Qt can read/write it directly — route 5
// probe), so the user browses/selects INSIDE the authorized tree.
// Fallback (no main window, e.g. startup): historical fileDialog event.
// Empty return = "user cancelled".

// The authorized work folder (set by the ArkTS shell before any file
// dialog opens).  Stays empty until the user grants one via FOLDER picker.
static QString g_workFolderPath;

void scidavisSetWorkFolderPath(const char *path)
{
    g_workFolderPath = QString::fromUtf8(path ? path : "");
    BRIDGE_LOG("work folder set: %s", qPrintable(g_workFolderPath));
}

const QString &scidavisWorkFolderPath()
{
    return g_workFolderPath;
}

static QString resolveDialogDir(const QString &dir)
{
    // 2026-08-17 (user round-8 #3): ALWAYS prefer the authorized work
    // folder over SciDAVis's internal workingDir when one is set.  The
    // internal workingDir (default: application dir or last save path) is
    // frequently outside the ArkTS FOLDER grant on OpenHarmony, so the Qt
    // dialog starts in a blank/unreadable directory even though the app just
    // re-requested permission.  Every embedded file dialog must start inside
    // the authorized tree; only when no grant exists fall back to the
    // caller-provided dir.
    if (!g_workFolderPath.isEmpty())
        return g_workFolderPath;
    if (!dir.isEmpty())
        return dir;
    return QString();
}

QString QFileDialog::getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
                                     const QString &filter, QString *selectedFilter,
                                     Options options)
{
    if (findMainWindow()) {
        QFileDialog dialog(parent, caption, resolveDialogDir(dir), filter);
        dialog.setOption(DontUseNativeDialog, true);
        dialog.setFileMode(ExistingFile);
        if (options & DontResolveSymlinks) dialog.setOption(DontResolveSymlinks, true);
        const int r = embedDialog(&dialog);
        if (r == QDialog::Accepted) {
            const QStringList sel = dialog.selectedFiles();
            if (selectedFilter) *selectedFilter = dialog.selectedNameFilter();
            return sel.isEmpty() ? QString() : sel.first();
        }
        return QString();
    }
    Q_UNUSED(selectedFilter);
    Q_UNUSED(options);
    emitFileDialog("open", parent, caption, dir, filter);
    return QString();
}

QStringList QFileDialog::getOpenFileNames(QWidget *parent, const QString &caption,
                                          const QString &dir, const QString &filter,
                                          QString *selectedFilter, Options options)
{
    if (findMainWindow()) {
        QFileDialog dialog(parent, caption, resolveDialogDir(dir), filter);
        dialog.setOption(DontUseNativeDialog, true);
        dialog.setFileMode(ExistingFiles);
        const int r = embedDialog(&dialog);
        if (r == QDialog::Accepted) {
            if (selectedFilter) *selectedFilter = dialog.selectedNameFilter();
            return dialog.selectedFiles();
        }
        return QStringList();
    }
    Q_UNUSED(selectedFilter);
    Q_UNUSED(options);
    emitFileDialog("openMulti", parent, caption, dir, filter);
    return QStringList();
}

QString QFileDialog::getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
                                     const QString &filter, QString *selectedFilter,
                                     Options options)
{
    if (findMainWindow()) {
        QFileDialog dialog(parent, caption, resolveDialogDir(dir), filter);
        dialog.setOption(DontUseNativeDialog, true);
        dialog.setFileMode(AnyFile);
        dialog.setAcceptMode(AcceptSave);
        if (options & DontConfirmOverwrite) dialog.setOption(DontConfirmOverwrite, true);
        const int r = embedDialog(&dialog);
        if (r == QDialog::Accepted) {
            const QStringList sel = dialog.selectedFiles();
            if (selectedFilter) *selectedFilter = dialog.selectedNameFilter();
            return sel.isEmpty() ? QString() : sel.first();
        }
        return QString();
    }
    Q_UNUSED(selectedFilter);
    Q_UNUSED(options);
    emitFileDialog("save", parent, caption, dir, filter);
    return QString();
}

QString QFileDialog::getExistingDirectory(QWidget *parent, const QString &caption,
                                          const QString &dir, Options options)
{
    if (findMainWindow()) {
        QFileDialog dialog(parent, caption, resolveDialogDir(dir));
        dialog.setOption(DontUseNativeDialog, true);
        dialog.setFileMode(Directory);
        dialog.setOption(ShowDirsOnly, true);
        const int r = embedDialog(&dialog);
        if (r == QDialog::Accepted) {
            const QStringList sel = dialog.selectedFiles();
            return sel.isEmpty() ? QString() : sel.first();
        }
        return QString();
    }
    Q_UNUSED(options);
    emitFileDialog("directory", parent, caption, dir, QString());
    return QString();
}

// ── QInputDialog statics ─────────────────────────────────────────────────
// Phase 1 (2026-08-12): construct the REAL QInputDialog and run it through
// the generic embedDialog template (QInputDialog does not override exec(),
// so the manual modal loop is driven directly).  Fallback: no main window
// (e.g. during startup) → historical event-reporting path.
// *ok = false and the default value = "user cancelled".

QString QInputDialog::getText(QWidget *parent, const QString &title, const QString &label,
                              QLineEdit::EchoMode echo, const QString &text, bool *ok,
                              Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints)
{
    if (findMainWindow()) {
        QInputDialog dialog(parent, flags);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setTextValue(text);
        // This Qt fork predates QInputDialog::setEchoMode (Qt 5.15).
        dialog.setTextEchoMode(echo);
        dialog.setInputMethodHints(inputMethodHints);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.textValue();
        return text;
    }
    emitInputDialog("text", title, label);
    if (ok)
        *ok = false;
    return text;
}

QString QInputDialog::getMultiLineText(QWidget *parent, const QString &title, const QString &label,
                                       const QString &text, bool *ok, Qt::WindowFlags flags,
                                       Qt::InputMethodHints inputMethodHints)
{
    if (findMainWindow()) {
        QInputDialog dialog(parent, flags);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setTextValue(text);
        dialog.setInputMethodHints(inputMethodHints);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.textValue();
        return text;
    }
    emitInputDialog("multiLineText", title, label);
    if (ok)
        *ok = false;
    return text;
}

QString QInputDialog::getItem(QWidget *parent, const QString &title, const QString &label,
                              const QStringList &items, int current, bool editable, bool *ok,
                              Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints)
{
    if (findMainWindow()) {
        QInputDialog dialog(parent, flags);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setComboBoxItems(items);
        dialog.setComboBoxEditable(editable);
        dialog.setInputMethodHints(inputMethodHints);
        // No setComboBoxCurrentIndex in this Qt fork; setTextValue
        // preselects the matching entry (list mode selects by exact match,
        // editable combo mode sets the edit text).
        if (current >= 0 && current < items.size())
            dialog.setTextValue(items.at(current));
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.textValue();
        return (current >= 0 && current < items.size()) ? items.at(current) : QString();
    }
    emitInputDialog("item", title, label);
    if (ok)
        *ok = false;
    return (current >= 0 && current < items.size()) ? items.at(current) : QString();
}

int QInputDialog::getInt(QWidget *parent, const QString &title, const QString &label, int value,
                         int minValue, int maxValue, int step, bool *ok, Qt::WindowFlags flags)
{
    if (findMainWindow()) {
        QInputDialog dialog(parent, flags);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setIntRange(minValue, maxValue);
        dialog.setIntValue(value);
        dialog.setIntStep(step);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.intValue();
        return value;
    }
    emitInputDialog("int", title, label);
    if (ok)
        *ok = false;
    return value;
}

double QInputDialog::getDouble(QWidget *parent, const QString &title, const QString &label,
                               double value, double minValue, double maxValue, int decimals,
                               bool *ok, Qt::WindowFlags flags)
{
    if (findMainWindow()) {
        QInputDialog dialog(parent, flags);
        dialog.setWindowTitle(title);
        dialog.setLabelText(label);
        dialog.setDoubleRange(minValue, maxValue);
        dialog.setDoubleValue(value);
        dialog.setDoubleDecimals(decimals);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.doubleValue();
        return value;
    }
    emitInputDialog("double", title, label);
    if (ok)
        *ok = false;
    return value;
}

double QInputDialog::getDouble(QWidget *parent, const QString &title, const QString &label,
                               double value, double minValue, double maxValue, int decimals,
                               bool *ok, Qt::WindowFlags flags, double step)
{
    Q_UNUSED(step);
    return getDouble(parent, title, label, value, minValue, maxValue, decimals, ok, flags);
}

// ── QColorDialog / QFontDialog statics ───────────────────────────────────
// Phase 1 (2026-08-12): construct the REAL dialog and run it through the
// generic embedDialog template.  Fallback: no main window (e.g. during
// startup) → historical event-reporting path.

QColor QColorDialog::getColor(const QColor &initial, QWidget *parent, const QString &title,
                              ColorDialogOptions options)
{
    if (findMainWindow()) {
        QColorDialog dialog(initial, parent);
        if (!title.isEmpty())
            dialog.setWindowTitle(title);
        dialog.setOptions(options);
        const int ret = embedDialog(&dialog);
        if (ret == QDialog::Accepted)
            return dialog.selectedColor();
        // Cancelled: keep the initial color.  SciDAVis call sites
        // short-circuit on `c == initial`, so this is a safe no-op — the
        // same convention as QFontDialog::getFont below.
        return initial;
    }
    Q_UNUSED(parent);
    Q_UNUSED(options);
    QJsonObject p;
    p[QStringLiteral("title")] = title;
    p[QStringLiteral("initial")] = initial.name();
    scidavisEmitEvent(QStringLiteral("colorDialog"), p);
    BRIDGE_LOG("QColorDialog::getColor fallback (no main window): returning initial");
    return initial; // cancelled: keep the initial color
}

QFont QFontDialog::getFont(bool *ok, QWidget *parent)
{
    if (findMainWindow()) {
        QFontDialog dialog(parent);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.selectedFont();
        return QFont();
    }
    Q_UNUSED(parent);
    scidavisEmitEvent(QStringLiteral("fontDialog"), QJsonObject());
    if (ok)
        *ok = false;
    return QFont();
}

QFont QFontDialog::getFont(bool *ok, const QFont &initial, QWidget *parent, const QString &title,
                           FontDialogOptions options)
{
    if (findMainWindow()) {
        QFontDialog dialog(initial, parent);
        if (!title.isEmpty())
            dialog.setWindowTitle(title);
        dialog.setOptions(options);
        const int ret = embedDialog(&dialog);
        if (ok)
            *ok = (ret == QDialog::Accepted);
        if (ret == QDialog::Accepted)
            return dialog.selectedFont();
        return initial;
    }
    Q_UNUSED(parent);
    Q_UNUSED(options);
    QJsonObject p;
    p[QStringLiteral("title")] = title;
    p[QStringLiteral("initial")] = initial.toString();
    scidavisEmitEvent(QStringLiteral("fontDialog"), p);
    BRIDGE_LOG("QFontDialog::getFont blocked");
    if (ok)
        *ok = false;
    return initial;
}

// ── QComboBox popup ──────────────────────────────────────────────────────
// The dropdown is a Qt::Popup top-level window → crash.  Emit the items so
// ArkTS can render a picker; the selection comes back through the
// selectCombo command (ohosBridgeSelectCombo).

void QComboBox::showPopup()
{
    // NOTE (2026-08-14): this interposition is only reachable from code
    // inside libentry.so — Qt's own vtable dispatch (mousePressEvent →
    // showPopupFromMouseEvent → q->showPopup()) binds to the versioned
    // @@Qt_5 symbol in libQt5Widgets.so, so real dropdowns from user
    // clicks NEVER land here.  The decisive interception is InputProbe
    // swallowing the press and calling ohosBridgeRequestCombo().  This
    // interposition remains as a defensive net for direct callers.
    ohosBridgeRequestCombo(this);
}

// OHOS fix (2026-08-14, issue "embedded dialog dropdown makes the app
// misbehave"): QComboBox::showPopup is interposed (above), but hidePopup
// was NOT — the first click after the ArkTS picker closed (or the
// outside-tap dismiss) called the REAL hidePopup on a combo whose popup
// state the interposer never opened.  In Qt 5.15, QComboBoxPrivate's
// popupOpen flag was set by the click path that called showPopup(), so a
// subsequent click calls hidePopup() → QComboBoxPrivateContainer ops on a
// never-created popup → the combo's event handling goes sideways and the
// dialog UI appears "broken" until reopen.  Interpose hidePopup() to
// clear the registry entry (matching the real implementation's
// popup-closed state) and swallow the call — the ArkTS picker owns
// dismissal.
void QComboBox::hidePopup()
{
    {
        std::lock_guard<std::mutex> lock(g_comboMutex);
        for (auto it = g_combos.begin(); it != g_combos.end();) {
            if (it->second == this)
                it = g_combos.erase(it);
            else
                ++it;
        }
    }
    BRIDGE_LOG("QComboBox hidePopup swallowed (registry cleared)");
}

#endif // SCIDAVIS_OHOS
