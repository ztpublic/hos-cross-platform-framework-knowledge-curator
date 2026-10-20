/***************************************************************************
    File                 : main.cpp
    Project              : SciDAVis
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Benkert
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : SciDAVis main function

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include <QMenuBar>
#include "ApplicationWindow.h"
#include "globals.h"
#include "Note.h"
#include "ohos_bridge.h"
#include "OpenProjectDialog.h"
#include "TableStatistics.h"
#include "Table.h"
#include "Matrix.h"
#include "future/matrix/MatrixView.h"
#include "future/table/TableView.h"
#include "future/core/column/Column.h"
#include "Correlation.h"
#include "Differentiation.h"
#include "FFT.h"
#include "FFTFilter.h"
#include "Integration.h"
#include "Interpolation.h"
#include "PolynomialFit.h"
#include "SmoothFilter.h"
#include "Convolution.h"
#include "ArrowMarker.h"
// Phase-7 fix 2026-08-13: Qt-native embedded dialogs for the
// scripting_language / rename_window menuAction branches.  (The about branch
// builds its dialog via SciDAVis::aboutDialog() in libscidavis instead —
// ui_SciDAVisAbout.h stays inside the libscidavis target, since cross-target
// AUTOUIC is not supported.)
#include "MyWidget.h"
#include "RenameWindowDialog.h"
#include "ScriptingLangDialog.h"

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QClipboard>
#include <QComboBox>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QMdiSubWindow>
#include <QScrollBar>
#include <QSplashScreen>
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QWindow>
#include <QPointer>
#include <QSemaphore>
#include <QSet>
#include <QProxyStyle>
// Phase 7: QUndoView for the embedded Undo/Redo History dialog
// (menuAction "undo_redo_history"; desktop counterpart
// ApplicationWindow::showHistory()).
#include <QUndoStack>
#include <QUndoView>
#include <QVBoxLayout>
#include <QChildEvent>
#include <atomic>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include "Folder.h"
#include "MultiLayer.h"
#include "Graph.h"
#include "Legend.h"
#include "PlotCurve.h"
#include "ImageMarker.h"
#include "Grid.h"
#include <qwt_data.h>
#include <qwt_scale_div.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_map.h>

#include <typeinfo>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <chrono>
// Bounded logging helper: the format string is always a compile-time literal
// at the call site (never user-controlled); vsnprintf caps the output and the
// final fprintf uses a fixed "%s" so nothing dynamic reaches the format arg.
static inline void scidavis_ohos_log(int line, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
#ifdef SCIDAVIS_OHOS
    qWarning("[SciDAVis:%d] %s", line, buf);
#else
    fprintf(stderr, "[SciDAVis:%d] %s\n", line, buf);
    fflush(stderr);
#endif
}
#define OHOS_LOG(fmt, ...) scidavis_ohos_log(__LINE__, fmt, ##__VA_ARGS__)



struct Application : public QApplication
{
    Application(int &argc, char **argv) : QApplication(argc, argv) { }

    // catch exception, and display their contents as modal dialogue
    bool notify(QObject *receiver, QEvent *event)
    {
        try {
            return QApplication::notify(receiver, event);
        } catch (const std::exception &e) {
            QMessageBox::critical(0, tr("Error!"),
                                  tr("Error ") + e.what() + tr(" sending event ")
                                          + typeid(*event).name() + tr(" to object ")
                                          + qPrintable(receiver->objectName()) + " \""
                                          + typeid(*receiver).name() + "\"");
        } catch (...) // shouldn't happen...
        {
            QMessageBox::critical(0, tr("Error!"),
                                  tr("Error <unknown> sending event") + typeid(*event).name()
                                          + tr(" to object ") + qPrintable(receiver->objectName())
                                          + " \"" + typeid(*receiver).name() + "\"");
        }
        return false;
    }
};

// The OpenHarmony QPA delivers physical-keyboard key events with an empty
// text() (verified via InputProbe: digit '1' arrives as key=0x31 text='').
// Editors insert nothing without text, so synthesize it for printable keys.
static QString printableTextFor(int key, Qt::KeyboardModifiers mods)
{
    if (mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return QString(); // shortcuts don't need text
    if (key < 0x20 || key > 0x7e)
        return QString(); // non-printable / special keys untouched
    QChar c = QChar(key);
    if (mods & Qt::ShiftModifier) {
        static const char digitShift[] = ")!@#$%^&*("; // US layout
        if (key >= Qt::Key_0 && key <= Qt::Key_9)
            c = QLatin1Char(digitShift[key - Qt::Key_0]);
        static const struct { int key; char shifted; } symbolShift[] = {
            { Qt::Key_Minus, '_' },
            { Qt::Key_Equal, '+' },
            { Qt::Key_BracketLeft, '{' },
            { Qt::Key_BracketRight, '}' },
            { Qt::Key_Backslash, '|' },
            { Qt::Key_Semicolon, ':' },
            { Qt::Key_Apostrophe, '"' },
            { Qt::Key_Comma, '<' },
            { Qt::Key_Period, '>' },
            { Qt::Key_Slash, '?' },
            { Qt::Key_AsciiTilde, '~' },
        };
        for (const auto &s : symbolShift) {
            if (key == s.key) {
                c = QLatin1Char(s.shifted);
                break;
            }
        }
    }
    if (c.isLetter()) {
        bool shift = mods & Qt::ShiftModifier;
#ifdef Q_OS_OHOS
        bool caps = false; // CapsLockModifier not available on OHOS Qt
#else
        bool caps = mods & Qt::CapsLockModifier;
#endif
        c = (shift ^ caps) ? c.toUpper() : c.toLower();
    }
    return QString(c);
}

// Native (OHOS) keycode -> Qt::Key fallback map for full-size (notebook /
// Bluetooth) keyboards.  The QPA plugin (alpha_v6) resolves letters, digits
// and space correctly, but leaves many special keys as Qt::Key_unknown (or a
// bogus 0x1000061).  Consulted ONLY for unresolved keys, so correctly-mapped
// keys and system keys (Back(2) etc.) are never disturbed.
//
// Two key-code layouts must be supported, because a Bluetooth / physical
// keyboard may deliver either one depending on how the system driver
// translates the scancodes:
//   * legacy AOSP layout (0-999): classic Android keycodes
//     (KEYCODE_DEL=67 == Backspace, KEYCODE_FORWARD_DEL=112 == Delete, ...);
//   * OHOS extended layout (2000+, oh_key_code.h): the modern OpenHarmony
//     keycodes used by newer devices / the uinput keymap
//     (KEYCODE_DEL=2055 == Backspace, KEYCODE_FORWARD_DEL=2071 == Delete,
//      KEYCODE_A=2017, KEYCODE_TAB=2049, KEYCODE_SPACE=2050, ...).
//
// NOTE: in BOTH layouts KEYCODE_DEL means BACKSPACE and KEYCODE_FORWARD_DEL
// means DELETE.  Mapping 67/2055 to Qt::Key_Delete (the earlier bug) made the
// Backspace key a no-op, and 2071 had no entry at all, so the Delete key was
// dead on extended-layout keyboards.
static Qt::Key keyForNativeVk(quint32 nativeVk)
{
    switch (nativeVk) {
    // ── legacy AOSP layout ────────────────────────────────────────────────
    case 67:   return Qt::Key_Backspace; // KEYCODE_DEL (AOSP "DEL" == Backspace)
    case 112:  return Qt::Key_Delete;    // KEYCODE_FORWARD_DEL
    case 3:    return Qt::Key_Home;      // KEYCODE_HOME (AOSP 系统级回桌面键, 应用层一般收不到; 保留映射以防万一)
    case 122:  return Qt::Key_Home;      // KEYCODE_MOVE_HOME
    case 123:  return Qt::Key_End;       // KEYCODE_MOVE_END
    case 19:   return Qt::Key_Up;        // KEYCODE_DPAD_UP
    case 20:   return Qt::Key_Down;      // KEYCODE_DPAD_DOWN
    case 21:   return Qt::Key_Left;      // KEYCODE_DPAD_LEFT
    case 22:   return Qt::Key_Right;     // KEYCODE_DPAD_RIGHT
    case 92:   return Qt::Key_PageUp;    // KEYCODE_PAGE_UP
    case 93:   return Qt::Key_PageDown;  // KEYCODE_PAGE_DOWN
    case 111:  return Qt::Key_Escape;    // KEYCODE_ESCAPE
    case 61:   return Qt::Key_Tab;       // KEYCODE_TAB
    case 66:   return Qt::Key_Return;    // KEYCODE_ENTER
    case 62:   return Qt::Key_Space;     // KEYCODE_SPACE
    case 131: return Qt::Key_F1;   case 132: return Qt::Key_F2;
    case 133: return Qt::Key_F3;   case 134: return Qt::Key_F4;
    case 135: return Qt::Key_F5;   case 136: return Qt::Key_F6;
    case 137: return Qt::Key_F7;   case 138: return Qt::Key_F8;
    case 139: return Qt::Key_F9;   case 140: return Qt::Key_F10;
    case 141: return Qt::Key_F11;  case 142: return Qt::Key_F12;

    // legacy AOSP modifier keys (KEYCODE_ALT_LEFT=57 / ALT_RIGHT=58,
    // SHIFT_LEFT=59 / SHIFT_RIGHT=60, CTRL_LEFT=113 / CTRL_RIGHT=114,
    // META_LEFT=117 / META_RIGHT=118): same rationale as the extended-layout
    // modifiers below — consulted only when the plugin leaves them
    // unresolved, so Qt modifier tracking (Ctrl+click multi-selection etc.)
    // works on legacy-layout keyboards too.
    case 57:  return Qt::Key_Alt;
    case 58:  return Qt::Key_Alt;
    case 59:  return Qt::Key_Shift;
    case 60:  return Qt::Key_Shift;
    case 113: return Qt::Key_Control;
    case 114: return Qt::Key_Control;
    case 115: return Qt::Key_CapsLock;    // KEYCODE_CAPS_LOCK
    case 117: return Qt::Key_Meta;
    case 118: return Qt::Key_Meta;
    case 124: return Qt::Key_Insert;      // KEYCODE_INSERT

    // legacy AOSP punctuation / symbol keys (issue list #5): Bluetooth
    // keyboards may deliver the 0-999 layout for these too - previously only
    // digits/letters/Backspace had legacy coverage, which left every legacy
    // punctuation key dead ("only digits and Backspace can be typed").
    case 68:  return Qt::Key_AsciiTilde; // KEYCODE_GRAVE
    case 69:  return Qt::Key_Minus;      // KEYCODE_MINUS
    case 70:  return Qt::Key_Equal;      // KEYCODE_EQUALS
    case 71:  return Qt::Key_BracketLeft;
    case 72:  return Qt::Key_BracketRight;
    case 73:  return Qt::Key_Backslash;
    case 74:  return Qt::Key_Semicolon;
    case 75:  return Qt::Key_Apostrophe;
    case 76:  return Qt::Key_Slash;
    case 77:  return Qt::Key_At;
    case 81:  return Qt::Key_Plus;

    // ── OHOS extended layout (oh_key_code.h, 2000+) ───────────────────────
    case 2012: return Qt::Key_Up;        // KEYCODE_DPAD_UP
    case 2013: return Qt::Key_Down;      // KEYCODE_DPAD_DOWN
    case 2014: return Qt::Key_Left;      // KEYCODE_DPAD_LEFT
    case 2015: return Qt::Key_Right;     // KEYCODE_DPAD_RIGHT
    case 2016: return Qt::Key_Select;    // KEYCODE_DPAD_CENTER (导航确认键 ≈ Enter/Select)
    case 2049: return Qt::Key_Tab;       // KEYCODE_TAB
    case 2050: return Qt::Key_Space;     // KEYCODE_SPACE
    case 2054: return Qt::Key_Return;    // KEYCODE_ENTER
    case 2055: return Qt::Key_Backspace; // KEYCODE_DEL (退格键 == Backspace!)
    case 2068: return Qt::Key_PageUp;    // KEYCODE_PAGE_UP
    case 2069: return Qt::Key_PageDown;  // KEYCODE_PAGE_DOWN
    case 2070: return Qt::Key_Escape;    // KEYCODE_ESCAPE
    case 2071: return Qt::Key_Delete;    // KEYCODE_FORWARD_DEL (删除键 == Delete)
    case 2081: return Qt::Key_Home;      // KEYCODE_MOVE_HOME
    case 2082: return Qt::Key_End;       // KEYCODE_MOVE_END
    case 2083: return Qt::Key_Insert;    // KEYCODE_INSERT
    case 2090: return Qt::Key_F1;   case 2091: return Qt::Key_F2;
    case 2092: return Qt::Key_F3;   case 2093: return Qt::Key_F4;
    case 2094: return Qt::Key_F5;   case 2095: return Qt::Key_F6;
    case 2096: return Qt::Key_F7;   case 2097: return Qt::Key_F8;
    case 2098: return Qt::Key_F9;   case 2099: return Qt::Key_F10;
    case 2100: return Qt::Key_F11;  case 2101: return Qt::Key_F12;

    // printable punctuation / symbol keys (plugin leaves them Key_unknown;
    // printableTextFor() supplies the text for the remapped event)
    case 2010: return Qt::Key_Asterisk;     // KEYCODE_STAR   '*'
    case 2011: return Qt::Key_NumberSign;   // KEYCODE_POUND  '#'
    case 2043: return Qt::Key_Comma;        // KEYCODE_COMMA
    case 2044: return Qt::Key_Period;       // KEYCODE_PERIOD
    case 2056: return Qt::Key_AsciiTilde;   // KEYCODE_GRAVE '`'
    case 2057: return Qt::Key_Minus;        // KEYCODE_MINUS
    case 2058: return Qt::Key_Equal;        // KEYCODE_EQUALS
    case 2059: return Qt::Key_BracketLeft;  // KEYCODE_LEFT_BRACKET
    case 2060: return Qt::Key_BracketRight; // KEYCODE_RIGHT_BRACKET
    case 2061: return Qt::Key_Backslash;    // KEYCODE_BACKSLASH
    case 2062: return Qt::Key_Semicolon;    // KEYCODE_SEMICOLON
    case 2063: return Qt::Key_Apostrophe;   // KEYCODE_APOSTROPHE
    case 2064: return Qt::Key_Slash;        // KEYCODE_SLASH
    case 2065: return Qt::Key_At;           // KEYCODE_AT
    case 2066: return Qt::Key_Plus;         // KEYCODE_PLUS

    // modifier / menu / system keys: consulted only when the plugin left the
    // key unresolved, so mapping them to the Qt modifier keys keeps
    // shortcuts working on extended-layout keyboards
    case 2045: return Qt::Key_Alt;          // KEYCODE_ALT_LEFT
    case 2046: return Qt::Key_AltGr;        // KEYCODE_ALT_RIGHT
    case 2047: return Qt::Key_Shift;        // KEYCODE_SHIFT_LEFT
    case 2048: return Qt::Key_Shift;        // KEYCODE_SHIFT_RIGHT
    case 2051: return Qt::Key_Mode_switch;  // KEYCODE_SYM (符号修改器)
    case 2052: return Qt::Key_Explorer;     // KEYCODE_EXPLORER (浏览器键)
    case 2053: return Qt::Key_LaunchMail;   // KEYCODE_ENVELOPE (邮件键)
    case 2067: return Qt::Key_Menu;         // KEYCODE_MENU
    case 2072: return Qt::Key_Control;      // KEYCODE_CTRL_LEFT
    case 2073: return Qt::Key_Control;      // KEYCODE_CTRL_RIGHT
    case 2074: return Qt::Key_CapsLock;     // KEYCODE_CAPS_LOCK
    case 2075: return Qt::Key_ScrollLock;   // KEYCODE_SCROLL_LOCK
    case 2076: return Qt::Key_Meta;         // KEYCODE_META_LEFT
    case 2077: return Qt::Key_Meta;         // KEYCODE_META_RIGHT
    case 2079: return Qt::Key_SysReq;       // KEYCODE_SYSRQ (PrtScn)
    case 2080: return Qt::Key_Pause;        // KEYCODE_BREAK
    case 2084: return Qt::Key_Forward;      // KEYCODE_FORWARD (浏览器前进)

    // numpad keys
    case 2102: return Qt::Key_NumLock;      // KEYCODE_NUM_LOCK
    case 2103: return Qt::Key_0;   case 2104: return Qt::Key_1;
    case 2105: return Qt::Key_2;   case 2106: return Qt::Key_3;
    case 2107: return Qt::Key_4;   case 2108: return Qt::Key_5;
    case 2109: return Qt::Key_6;   case 2110: return Qt::Key_7;
    case 2111: return Qt::Key_8;   case 2112: return Qt::Key_9;
    case 2113: return Qt::Key_division;     // KEYCODE_NUMPAD_DIVIDE
    case 2114: return Qt::Key_multiply;     // KEYCODE_NUMPAD_MULTIPLY
    case 2115: return Qt::Key_Minus;        // KEYCODE_NUMPAD_SUBTRACT
    case 2116: return Qt::Key_Plus;         // KEYCODE_NUMPAD_ADD
    case 2117: return Qt::Key_Period;       // KEYCODE_NUMPAD_DOT
    case 2118: return Qt::Key_Comma;        // KEYCODE_NUMPAD_COMMA
    case 2119: return Qt::Key_Enter;        // KEYCODE_NUMPAD_ENTER
    case 2120: return Qt::Key_Equal;        // KEYCODE_NUMPAD_EQUALS
    case 2121: return Qt::Key_ParenLeft;    // KEYCODE_NUMPAD_LEFT_PAREN
    case 2122: return Qt::Key_ParenRight;   // KEYCODE_NUMPAD_RIGHT_PAREN
    // KEYCODE_FUNCTION(2078) Fn 键、KEYCODE_MEDIA_*(2085-2089) 多媒体键、以及
    // 2210/2300+/2600+ (虚拟多任务/游戏手柄/日文布局等) 在 Qt 中无对应输入语义,
    // 保持 Key_unknown (QPA 若已正确映射则不受影响).
    default:
        break;
    }
    // Letters A-Z (KEYCODE_A=29 .. KEYCODE_Z=54; OHOS KEYCODE_A=2017 .. 2042)
    // and digits 0-9 (KEYCODE_0=7 .. KEYCODE_9=16; OHOS KEYCODE_0=2000 .. 2009):
    // Qt::Key_A..Key_Z and Qt::Key_0..Key_9 are contiguous ranges.
    if (nativeVk >= 29 && nativeVk <= 54)
        return Qt::Key(int(Qt::Key_A) + int(nativeVk - 29));
    if (nativeVk >= 7 && nativeVk <= 16)
        return Qt::Key(int(Qt::Key_0) + int(nativeVk - 7));
    if (nativeVk >= 2017 && nativeVk <= 2042)
        return Qt::Key(int(Qt::Key_A) + int(nativeVk - 2017));
    if (nativeVk >= 2000 && nativeVk <= 2009)
        return Qt::Key(int(Qt::Key_0) + int(nativeVk - 2000));
    return Qt::Key_unknown;
}

// Bluetooth/物理键盘修饰键跟踪 (2026-08-13): the alpha_v6 QPA plugin leaves
// many modifier keycodes unresolved, so its key-event modifiers field cannot
// be relied on to feed Qt's own modifier state.  Track Ctrl/Shift/Alt/Meta
// from the InputProbe key stream (the same keyForNativeVk remap the keys
// themselves use) and OR the result into every injected mouse event —
// scidavis_inject_mouse previously hardcoded Qt::NoModifier, so Ctrl+click
// (multi-column selection, Ctrl-drag) arrived at Qt without the Control bit.
// GUI-thread only: InputProbe and the inject lambda both run on the Qt GUI
// thread, never concurrently.
static Qt::KeyboardModifiers g_ohosModifierState = Qt::NoModifier;

// Shortcut fix (2026-08-16): fan-out dedup for the mods-fix re-send — the
// plugin delivers one physical key event to every widget in the focus chain,
// so the fixed copy must be sent once per physical event (keyed on
// nativeVk + event timestamp; ts==0 disables the guard).  GUI thread only.
static quint32 s_lastFixedVk = 0;
static qint64 s_lastFixedTs = 0;

// Shortcut-map fix (2026-08-16): the alpha_v6 QPA plugin delivers key events
// DIRECTLY to each widget in the focus chain (7 fan-out deliveries observed)
// instead of going through QWindowSystemInterface::handleKeyEvent, so
// QGuiApplicationPrivate::processKeyEvent → handleShortcutEvent (the QAction
// shortcut map) NEVER runs — QAction shortcuts (Ctrl+Z/C/V/X …) stayed dead
// even though the events carry correct modifiers (device-verified 2026-08-16:
// toolbar undo triggers the same action fine, Ctrl+Z does nothing).  We
// consult the shortcut map ourselves, once per physical key event.
//
// qwindowsysteminterface.h is a PRIVATE QtGui header not installed in the
// OHOS Qt SDK, so the call is hand-declared here; the symbol is exported
// from libQt5Gui (verified in the bundled .so).
namespace QWindowSystemInterface {
bool handleShortcutEvent(QWindow *window, unsigned long timestamp, int keyCode,
                         Qt::KeyboardModifiers modifiers, quint32 nativeScanCode,
                         quint32 nativeVirtualKey, quint32 nativeModifiers,
                         const QString &text, bool autorepeat, unsigned short count);
}
static quint32 s_lastShortcutVk = 0;
static qint64 s_lastShortcutTs = 0;
static qint64 s_lastShortcutMs = 0;
static bool s_lastShortcutMatched = false;
static QElapsedTimer s_shortcutClock;
// Route-A embedded menu (2026-08-16, plan 2026-08-16-1854): a press consumed
// by the menu route must have its matching release swallowed too, so the
// lift never falls through to a widget underneath the (already closed) menu.
static bool s_menuPressConsumed = false;

// Shortcut fix (2026-08-16): the alpha_v6 plugin never delivers a
// window-activation event, so QApplication::activeWindow() stays null and the
// QAction shortcut context matcher rejects every match — the shortcut map
// (via handleShortcutEvent) can therefore never fire QAction shortcuts
// (device-verified: Ctrl+Z/C/V/X dead while the same actions work from the
// toolbars).  Instead of fixing the missing activation, match the key against
// the window's QActions directly, with the standard ShortcutOverride
// semantics: the focused widget may claim the key first (QLineEdit built-in
// Ctrl+V/Z etc.), and only when nothing claims it do we trigger the action.
// This is fully self-contained (no shortcut map, no context matcher, no
// activeWindow dependency) and covers every menu/window action shortcut.
class ApplicationWindow;
extern QPointer<ApplicationWindow> g_mainWindow;
static bool triggerActionShortcut(const QKeyEvent *ke)
{
    if (!g_mainWindow)
        return false;
    // ShortcutOverride semantics: walk the focus chain; if any widget
    // accepts the override it wants the key itself — do not trigger.
    for (QWidget *w = QApplication::focusWidget(); w; w = w->parentWidget()) {
        if (!w->isEnabled())
            continue;
        QKeyEvent overrideEv(QEvent::ShortcutOverride, ke->key(), ke->modifiers(),
                             ke->text(), ke->isAutoRepeat(), ke->count());
        overrideEv.setTimestamp(ke->timestamp());
        QApplication::sendEvent(w, &overrideEv);
        if (overrideEv.isAccepted())
            return false;
    }
    const QKeySequence seq(int(ke->key())
        | int(ke->modifiers()
              & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier
                 | Qt::MetaModifier | Qt::KeypadModifier)));
    if (seq.isEmpty())
        return false;
    bool hit = false;
    const QList<QAction *> actions = g_mainWindow->findChildren<QAction *>();
    for (QAction *a : actions) {
        if (!a->isEnabled() || a->isSeparator() || a->shortcut().isEmpty())
            continue;
        if (a->shortcut() == seq) {
            qWarning("[InputProbe] action shortcut: key=0x%x seq='%s' action='%s'",
                     int(ke->key()), qPrintable(seq.toString()),
                     qPrintable(a->text().remove('&')));
            a->trigger();
            // Diagnostic: did the slot actually move data?  (copy/paste
            // depend on the MDI selection state on the Qt side.)
            const QString clip = QApplication::clipboard()->text();
            QAction *u = g_mainWindow->ohosActionUndo();
            qWarning("[InputProbe] after trigger '%s': clipboard='%s' (%d chars) undoEnabled=%d",
                     qPrintable(a->text().remove('&')), qPrintable(clip),
                     clip.size(), int(u && u->isEnabled()));
            hit = true;
            // Trigger only the first matching action (same as the map).
            break;
        }
    }
    return hit;
}

// qtKey is the EFFECTIVE Qt key (after the keyForNativeVk remap decision);
// press/release events only — an auto-repeat press just re-sets the bit.
static void trackOhosModifierKey(QEvent::Type type, int qtKey)
{
    Qt::KeyboardModifiers bit = Qt::NoModifier;
    switch (qtKey) {
    case Qt::Key_Control: bit = Qt::ControlModifier; break;
    case Qt::Key_Shift:   bit = Qt::ShiftModifier;   break;
    case Qt::Key_Alt:     bit = Qt::AltModifier;     break;
    case Qt::Key_Meta:    bit = Qt::MetaModifier;    break;
    default:
        return;
    }
    if (type == QEvent::KeyRelease)
        g_ohosModifierState &= ~bit;
    else
        g_ohosModifierState |= bit;
}

// QMdiSubWindow title-bar fix (2026-08-13): on the tablet the
// PM_TitleBarHeight metric can collapse to 0 (QStyle font-metrics
// dependency), which makes QMdiSubWindow's internal layout put the table
// content at y=0 — the title bar (and its × close button) is then covered
// by the table widget and unreachable, so clicking × never opens the
// hide-or-delete confirm dialog.  A proxy style guarantees a usable MDI
// title-bar height without touching any other metric (no-op when the
// metric is already sane).
class MdiTitleBarStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if (metric == PM_TitleBarHeight) {
            const int v = QProxyStyle::pixelMetric(metric, option, widget);
            return v > 0 ? v : 30;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

// Input diagnostics for the OpenHarmony port: logs mouse press / release,
// key presses and focus changes via qWarning (Qt5Core routes messages to
// hilog on OpenHarmony).  Also fixes text-less printable key events (see
// printableTextFor above).
class InputProbe : public QObject
{
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        // Embedded-dialog modality: while an embedded dialog runs its
        // manual QEventLoop, swallow every input event whose target lies
        // OUTSIDE the TOP dialog's widget subtree — the main window (tables,
        // MDI area, menu overlays) must not react underneath the modal
        // dialog.  Nested dialogs (e.g. a color picker opened from an
        // embedded dialog) push further entries onto g_modalStack, so the
        // top of the stack is the isolation baseline.  Events inside the
        // dialog (buttons, list, tabs) pass through untouched.  Both the
        // mouse and the touch channels are covered, and the check runs
        // BEFORE the per-type handling below (dedup, key remap, ...).
        // Route A (2026-08-16): while an embedded MENU runs, this generic
        // isolation yields to the menu route below — menus need clicks OUTSIDE
        // to close them, not to be swallowed (inside-press handling is fully
        // manual in the mouse branch via ohosBridgeMenuHandlePress).
        if (!ohosBridgeMenuActive() && g_modalDialogActive && !g_modalStack.empty()
            && obj->isWidgetType()) {
            switch (ev->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseMove:
            case QEvent::Wheel:
                // 2026-08-13 wheel-path diagnostics: distinguishes Qt-native
                // wheel events (this log) from ArkTS sendWheel injection
                // (see scidavis_inject_wheel qWarning).  Logged BEFORE the
                // modal isolation check so swallowed events are recorded too.
                qWarning("[SciDAVis] nativeWheel: target=%s modalStack=%zu",
                         obj ? obj->metaObject()->className() : "null",
                         g_modalStack.size());
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchEnd:
            case QEvent::TouchCancel:
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            case QEvent::ContextMenu: {
                QWidget *w = qobject_cast<QWidget *>(obj);
                QWidget *top = g_modalStack.back();
                if (w != top && !top->isAncestorOf(w)) {
                    qWarning("[InputProbe] modal: swallowed %d on %s",
                             int(ev->type()), obj->metaObject()->className());
                    return true;
                }
                break;
            }
            default:
                break;
            }
        }
        switch (ev->type()) {
        case QEvent::ChildAdded: {
            // QMdiSubWindow title-bar margin fallback (2026-08-13): when the
            // style metric collapses to 0 the sub-window's internal layout
            // never reserves the title-bar strip, so the embedded table
            // covers the × close button (click → no confirm dialog).  On
            // every QMdiArea child addition, force a sane top margin (no-op
            // when updateWindowDecorations already set one) and re-place the
            // content strictly BELOW the title-bar strip (a stale child
            // geometry — content starting at y=0 — makes the × unreachable
            // even when the margins are correct).
            auto *ce = static_cast<QChildEvent *>(ev);
            if (ce->child() && ce->child()->isWidgetType()) {
                QWidget *child = static_cast<QWidget *>(ce->child());
                if (qobject_cast<QMdiSubWindow *>(child)) {
                    const int tbH = child->style()->pixelMetric(QStyle::PM_TitleBarHeight,
                                                                nullptr, child);
                    QMargins m = child->contentsMargins();
                    if (tbH > 0 && m.top() < 1) {
                        child->setContentsMargins(2, tbH, 2, 2);
                        qWarning("[SciDAVis] fixed QMdiSubWindow title-bar margin tbH=%d", tbH);
                    }
                    if (child->layout())
                        child->layout()->activate();
                    QWidget *inner = static_cast<QMdiSubWindow *>(child)->widget();
                    if (inner && inner->y() < tbH) {
                        inner->setGeometry(2, tbH, inner->width(), child->height() - tbH - 2);
                        qWarning("[SciDAVis] re-placed sub-window content below title bar: y=%d tbH=%d",
                                 inner->y(), tbH);
                    }
                }
            }
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick: {
            auto *me = static_cast<QMouseEvent *>(ev);
            // Route-A embedded menu (2026-08-16): while the inline QMenu's
            // nested loop runs, EVERY mouse event over the screen is routed
            // by the bridge (actionAt + deferred trigger / outside close).
            // Qt never sees these events, so QMenu's (dormant) popup state
            // machine cannot double-fire.  The release belonging to a
            // consumed press is swallowed even after the menu closed, so the
            // lift never lands on a widget underneath.
            if (ohosBridgeMenuActive()) {
                if (ev->type() == QEvent::MouseButtonPress) {
                    if (ohosBridgeMenuHandlePress(me->globalPos()))
                        s_menuPressConsumed = true;
                    return true;
                }
                if (ev->type() == QEvent::MouseButtonRelease && s_menuPressConsumed) {
                    s_menuPressConsumed = false;
                    return true;
                }
                // dblclick / stray release / anything else while a menu is
                // open: never reach the underlying widgets.
                return true;
            }
            // Combo-picker modality (2026-08-14): while the ArkTS picker is
            // up (or within the dismiss debounce), swallow EVERY press —
            // a tap on the dialog underneath would otherwise re-open the
            // picker (another QComboBox) or activate another control,
            // exactly the "dropdown misbehaves" report.  The ArkTS scrim
            // handles the dismiss tap itself; Qt must not see it at all.
            // Runs BEFORE the QComboBox check so the same modal state also
            // suppresses a second comboRequest.
            if (ohosBridgeComboPickerActive()) {
                qWarning("[InputProbe] combo modal: swallowed %s at (%d,%d)",
                         ev->type() == QEvent::MouseButtonPress ? "press"
                                 : ev->type() == QEvent::MouseButtonRelease ? "release"
                                                                           : "dblclick",
                         me->globalX(), me->globalY());
                return true;
            }
            // QComboBox dropdown is a popup-window hazard: the interposed
            // showPopup() is unreachable from Qt's vtable dispatch
            // (versioned @@Qt_5 symbols bind to the Qt library), so a press
            // here would run the REAL popup and create a second native
            // window → main surface broken ("dropdown makes the app
            // misbehave", 2026-08-14).  Swallow the press and push a
            // comboRequest so ArkTS renders the picker instead.  This MUST
            // run BEFORE the cross-source dedup below: the dedup drops
            // NotSynthesized/SynthesizedByQt events (stylus, native touch
            // synthesis), and the combo check must see every source.
            // Only the PRESS is consumed; the release is harmless.
            if (ev->type() == QEvent::MouseButtonPress && obj->isWidgetType()
                && obj->inherits("QComboBox")) {
                QComboBox *cb = static_cast<QComboBox *>(obj);
                // 2026-08-14 (user-reported): extra taps while the ArkTS
                // picker is already up MUST be swallowed silently — the
                // request-side guard (ohosBridgeRequestCombo) refuses them,
                // and here we consume them so they can never reach the real
                // popup path either.  Multi-tap = one picker, no layout
                // breakage.
                if (ohosBridgeComboPickerActive()) {
                    qWarning("[InputProbe] swallowed QComboBox press while picker active");
                    return true;
                }
                // Editable combos (boxSeparator in Preferences): a press
                // inside the line-edit area must keep focus/editing — only
                // block when the press hits the arrow button (right side).
                QStyleOptionComboBox opt;
                opt.rect = cb->rect();
                const QStyle::SubControl sc =
                    cb->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt,
                                                       cb->mapFromGlobal(me->globalPos()), cb);
                const bool onArrow = (sc == QStyle::SC_ComboBoxArrow);
                if (!cb->isEditable() || onArrow) {
                    qWarning("[InputProbe] blocked QComboBox press (editable=%d arrow=%d) on %s",
                             int(cb->isEditable()), int(onArrow),
                             obj->metaObject()->className());
                    ohosBridgeRequestCombo(cb);
                    return true;
                }
            }
            // 2in1 fix (2026-08-13): the QPA native mouse channel reports
            // coordinates offset by the window decor (device: +65px) while
            // the ArkTS overlay forwarding (scidavis_inject_mouse, source
            // = SynthesizedByApplication) is exact.  The two channels then
            // disagree by >5px, the cross-source dedup never fires, and the
            // WRONG native event wins the row-selection race (click row 1
            // → row 3 selected).  The overlay forwards every left/middle/
            // right click and drag (onTouch + onMouse), so drop native
            // (NotSynthesized / SynthesizedByQt) mouse-button events —
            // tablet is unaffected (overlay coords are exact there too).
            if (me->source() == Qt::MouseEventNotSynthesized
                || me->source() == Qt::MouseEventSynthesizedByQt) {
                return true;
            }
            // 2in1 row-selection diag (2026-08-13): print every press's
            // target class, local pos and (for QTableView) the row Qt
            // computes; then, after the default handler runs, the selected
            // row — one build, full truth about click → row mapping.
            if (ev->type() == QEvent::MouseButtonPress) {
                // 2026-08-13 close-button diag: also log the widget's
                // objectName and geometry — the ×-button hit can land on a
                // different sub-window than the DIAG's activeSubWindow, and
                // a stale geometry (content covering the title bar) is only
                // visible via the actual widget rect at press time.
                QWidget *pw = obj->isWidgetType() ? static_cast<QWidget *>(obj) : nullptr;
                qWarning("[SciDAVis] PRESS obj=%s name='%s' pos=(%d,%d) global=(%d,%d) geo=%d,%d,%dx%d",
                         obj->metaObject()->className(),
                         pw ? qPrintable(pw->objectName()) : "",
                         me->pos().x(), me->pos().y(),
                         me->globalPos().x(), me->globalPos().y(),
                         pw ? pw->x() : 0, pw ? pw->y() : 0,
                         pw ? pw->width() : 0, pw ? pw->height() : 0);
                if (obj->isWidgetType() && obj->inherits("QTableView")) {
                    QTableView *tv = static_cast<QTableView *>(obj);
                    const QPoint vp = tv->viewport()->mapFromGlobal(me->globalPos());
                    qWarning("[SciDAVis] PRESS-TV vpGeo=%d,%d,%dx%d vp=(%d,%d) rowAt(vpY)=%d rowAt(posY)=%d",
                             tv->viewport()->x(), tv->viewport()->y(),
                             tv->viewport()->width(), tv->viewport()->height(),
                             vp.x(), vp.y(), tv->rowAt(vp.y()), tv->rowAt(me->pos().y()));
                    // P1-11 (2026-08-14): capture a QPointer instead of a
                    // raw QTableView* — the deferred lambda ran after the
                    // table could be closed/destroyed (use-after-free).
                    QPointer<QTableView> keep(tv);
                    QTimer::singleShot(0, [keep]() {
                        if (!keep)
                            return;
                        qWarning("[SciDAVis] AFTER-TV currentRow=%d currentCol=%d",
                                 keep->currentIndex().row(), keep->currentIndex().column());
                    });
                }
            }
            // The single-window QPA cannot create popup windows: a press on
            // the native QMenuBar would run QMenu::popup() -> new top-level
            // window -> SIGSEGV inside the plugin.  The ArkTS overlay is the
            // only menu UI, so block menu-bar presses outright.
            if (ev->type() != QEvent::MouseButtonRelease && obj->isWidgetType()
                && (obj->inherits("QMenuBar") || obj->inherits("QMenu"))) {
                qWarning("[InputProbe] blocked menu-bar mouse %d on %s (popup unsupported)",
                         int(ev->type()), obj->metaObject()->className());
                return true;
            }
            // QComboBox dropdown is the same popup-window hazard: the
            // interposed showPopup() is unreachable from Qt's vtable
            // dispatch (versioned @@Qt_5 symbols bind to the Qt library),
            // so a press here would run the REAL popup and create a second
            // native window → main surface broken ("dropdown makes the app
            // misbehave", 2026-08-14).  Swallow the press and push a
            // comboRequest so ArkTS renders the picker instead.  Only the
            // PRESS is consumed; the corresponding release is harmless.
            if (ev->type() == QEvent::MouseButtonPress && obj->isWidgetType()
                && obj->inherits("QComboBox")) {
                QComboBox *cb = static_cast<QComboBox *>(obj);
                // 2026-08-14 (user-reported): while the ArkTS picker is up,
                // extra combo taps must be swallowed silently — never reach
                // the real popup path and never push a second comboRequest
                // (the request-side guard refuses it anyway).
                if (ohosBridgeComboPickerActive()) {
                    qWarning("[InputProbe] swallowed QComboBox press while picker active");
                    return true;
                }
                // Editable combos (boxSeparator in Preferences): a press
                // inside the line-edit area must keep focus/editing — only
                // block when the press hits the arrow button (right side).
                // QStyleOptionComboBox default-constructs with current
                // subRect data; hitTestComplexControl only needs the rect
                // (computed below from the widget's own geometry).
                const auto *me = static_cast<QMouseEvent *>(ev);
                QStyleOptionComboBox opt;
                opt.rect = cb->rect();
                const QStyle::SubControl sc =
                    cb->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt,
                                                       cb->mapFromGlobal(me->globalPos()), cb);
                const bool onArrow = (sc == QStyle::SC_ComboBoxArrow);
                if (!cb->isEditable() || onArrow) {
                    qWarning("[InputProbe] blocked QComboBox press (editable=%d arrow=%d) on %s",
                             int(cb->isEditable()), int(onArrow),
                             obj->metaObject()->className());
                    ohosBridgeRequestCombo(cb);
                    return true;
                }
            }
            // Cross-source dedup at the QWindow delivery level: a finger tap
            // reaches Qt twice -- natively via the QPA touch pipeline
            // (MouseEventSynthesizedByQt) and again via the ETS overlay
            // (MouseEventSynthesizedByApplication), because ArkTS cannot
            // distinguish finger touches from mouse-converted ones (both
            // report TouchScreen/Finger).  An identical type+button+pos
            // event from a DIFFERENT source within 60ms is that duplicate:
            // swallow it before the widget dispatch runs.
            if (obj->isWindowType()) {
                // Match by down/up KIND, not exact type: the injected path
                // synthesizes DblClick REPLACING the second press, while the
                // native path delivers Press AND DblClick, so exact-type
                // matching would let the native pair of a double-tap leak.
                const int kind = (ev->type() == QEvent::MouseButtonRelease) ? 1 : 0;
                static int sDupKind = -1;
                static QPoint sDupPos;
                static Qt::MouseButton sDupBtn = Qt::NoButton;
                static int sDupSrc = -1;
                static QElapsedTimer sDupTimer;
                const bool dup = sDupTimer.isValid() && !sDupTimer.hasExpired(60)
                        && kind == sDupKind && me->button() == sDupBtn
                        && int(me->source()) != sDupSrc
                        && (me->globalPos() - sDupPos).manhattanLength() < 5;
                if (dup) {
                    qWarning("[InputProbe] dropped cross-source duplicate type=%d src=%d",
                             int(ev->type()), int(me->source()));
                    return true;
                }
                sDupKind = kind;
                sDupPos = me->globalPos();
                sDupBtn = me->button();
                sDupSrc = int(me->source());
                sDupTimer.restart();
            }
            qWarning("[InputProbe] mouse %s recv=%s pos=(%d,%d) global=(%d,%d) btn=0x%x src=%d mods=0x%x",
                     ev->type() == QEvent::MouseButtonPress         ? "press"
                             : ev->type() == QEvent::MouseButtonRelease ? "release"
                                                                        : "dblclick",
                     obj->metaObject()->className(), me->pos().x(), me->pos().y(), me->globalX(),
                     me->globalY(), int(me->button()), int(me->source()),
                     unsigned(me->modifiers()));
            // Title-bar close-button fallback (2026-08-13): on this device
            // the QMdiSubWindow control buttons (ControlLabel ×) are never
            // hit — childAt returns the sub-window itself, so the release
            // lands on the sub-window's own title-bar handler, which needs
            // a hoveredSubControl that is never set.  A release in the
            // title-bar's rightmost ~30px (the close-button strip) is
            // therefore treated as a close: it goes through
            // QMdiSubWindow::close() → the embedded widget's closeEvent →
            // the hide-or-delete confirm dialog (MyWidget::closeEvent),
            // exactly like a real × click on the desktop.  Mirrors desktop
            // semantics: releasing over the close strip closes the window.
            if (ev->type() == QEvent::MouseButtonRelease
                && me->source() == Qt::MouseEventSynthesizedByApplication
                && obj->isWidgetType()) {
                QWidget *w = static_cast<QWidget *>(obj);
                if (QMdiSubWindow *sub = qobject_cast<QMdiSubWindow *>(w)) {
                    const QPoint p = me->pos();
                    const int tbH = sub->contentsMargins().top();
                    if (tbH > 0 && p.y() >= 0 && p.y() < tbH
                        && p.x() >= sub->width() - 30) {
                        qWarning("[SciDAVis] title-bar close fallback: close '%s' pos=(%d,%d) tbH=%d",
                                 qPrintable(sub->objectName()), p.x(), p.y(), tbH);
                        sub->close();
                    }
                }
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(ev);
            // Route-A embedded menu (2026-08-16): swallow every move while
            // the inline menu is open, but FIRST route it to the bridge's
            // hover state machine — row highlight + auto-open/close of
            // cascades (Win parity, user round-3).  Qt must never see a
            // hover over an action with a submenu — its internal openMenu
            // would call the REAL QMenu::popup (binding inside the Qt
            // library) and create a native popup window (crash family).
            if (ohosBridgeMenuActive()) {
                if (obj->isWindowType()) {
                    static int sMenuMoveSrc = -1;
                    static QPoint sMenuMovePos;
                    static QElapsedTimer sMenuMoveTimer;
                    sMenuMovePos = me->globalPos();
                    sMenuMoveSrc = int(me->source());
                    sMenuMoveTimer.restart();
                }
                ohosBridgeMenuHover(me->globalPos());
                return true;
            }
            // Finger drags reach Qt twice -- natively via the QPA touch
            // pipeline (MouseEventSynthesizedByQt) and again via the ETS
            // overlay (MouseEventSynthesizedByApplication) -- so every move
            // would otherwise be processed twice by the widgets (drag lag).
            // Coalesce the cross-source duplicate at the QWindow delivery
            // level: a near-identical move from the OTHER source within 60ms
            // is the same finger position re-delivered, so swallow it before
            // the widget dispatch runs.  Moves from the SAME source always
            // pass (the QPA native stream is singular, and the injected
            // stream is a single source too).  Separate state from the
            // press/release dedup above so the two never interfere.
            // 2in1 fix (2026-08-13): like the button events, the native
            // move channel carries the decor offset (+65px) so its moves
            // never match the overlay ones — drop it, keep the exact
            // overlay stream (covers finger + mouse drags on both devices).
            if (me->source() == Qt::MouseEventNotSynthesized
                || me->source() == Qt::MouseEventSynthesizedByQt) {
                return true;
            }
            if (obj->isWindowType()) {
                static int sMoveSrc = -1;
                static QPoint sMovePos;
                static QElapsedTimer sMoveTimer;
                const bool dup = sMoveTimer.isValid() && !sMoveTimer.hasExpired(60)
                        && int(me->source()) != sMoveSrc
                        && (me->globalPos() - sMovePos).manhattanLength() < 5;
                if (dup) {
                    qWarning("[InputProbe] dropped cross-source duplicate move src=%d pos=(%d,%d)",
                             int(me->source()), me->globalPos().x(), me->globalPos().y());
                    return true;
                }
                sMovePos = me->globalPos();
                sMoveSrc = int(me->source());
                sMoveTimer.restart();
            }
            break;
        }
        case QEvent::ContextMenu: {
            // Route A (2026-08-16, plan 2026-08-16-1854): pass the event
            // THROUGH so the widget's ORIGINAL contextMenuEvent /
            // TableView::eventFilter runs and its QMenu::exec lands in the
            // interposed embedMenu — real inline QMenu rendering (pixels
            // exactly Qt's), items = the same app-level QActions as the menu
            // bar, zero extra wiring.  The widget-class walk + report below
            // is kept as DIAGNOSTICS only (ArkTS keeps ignoring #9, so no
            // double menu).  While a menu is already open a new context
            // event is swallowed (no stacking).
            if (ohosBridgeMenuActive()) {
                qWarning("[InputProbe] context-menu event while menu open — swallowed (reentry guard)");
                return true;
            }
            // Standard-text-menu whitelist (T3/T4/T5): Qt's internal
            // QWidgetLineControl/QTextEdit context-menu exec is bound inside
            // the Qt library and CANNOT be captured by symbol interposition
            // (same reason as QComboBox::showPopup), so a pass-through runs
            // the REAL popup path — QMenuClassWindow creation crashes the
            // single-window QPA (verified 2026-08-16: process restarted).
            // Swallow, rebuild the standard menu via the PUBLIC
            // createStandardContextMenu() and embed it inline.  The target
            // is usually the edit widget's VIEWPORT (plain QWidget child),
            // so walk the parent chain to find the edit widget.  Subclasses
            // with their OWN contextMenuEvent (ScriptEdit etc.) call exec
            // directly from app code → interposed → pass them through.
            {
                QWidget *edit = nullptr;
                for (QWidget *cur = qobject_cast<QWidget *>(obj); cur && cur->parentWidget();
                     cur = cur->parentWidget()) {
                    if ((cur->inherits("QLineEdit"))
                        || ((cur->inherits("QTextEdit") || cur->inherits("QPlainTextEdit"))
                            && !cur->inherits("ScriptEdit"))
                        || cur->inherits("QAbstractSpinBox")) {
                        edit = cur;
                        break;
                    }
                }
                if (edit) {
                    QMenu *m = nullptr;
                    if (auto *le = qobject_cast<QLineEdit *>(edit))
                        m = le->createStandardContextMenu();
                    else if (auto *te = qobject_cast<QTextEdit *>(edit))
                        m = te->createStandardContextMenu();
                    else if (auto *pt = qobject_cast<QPlainTextEdit *>(edit))
                        m = pt->createStandardContextMenu();
                    else if (auto *sb = qobject_cast<QAbstractSpinBox *>(edit)) {
                        // Spinboxes own their edit as a CHILD — rebuild the
                        // standard menu from the spinbox's internal QLineEdit
                        // (QAbstractSpinBox::lineEdit() is protected).
                        if (QLineEdit *sle = sb->findChild<QLineEdit *>())
                            m = sle->createStandardContextMenu();
                    }
                    if (m) {
                        auto *cm = static_cast<QContextMenuEvent *>(ev);
                        qWarning("[InputProbe] standard text menu on %s (target %s) — inline embed",
                                 edit->metaObject()->className(),
                                 obj->metaObject()->className());
                        // zh wording is applied centrally in
                        // embedMenuCore (covers every menu path).
                        ohosBridgeSetContextPos(cm->globalPos());
                        ohosBridgeShowStandardMenu(m);
                        return true;
                    }
                }
            }
            // Phase 6 (2026-08-13): walk the parent chain Graph-FIRST — a
            // right-click inside a plot layer reports "Graph" (Graph is a
            // plain QWidget child of the MultiLayer canvas, so the old
            // MyWidget-only walk lumped it with the layer's blank area),
            // while a click on the MultiLayer's own blank area stays
            // "MultiLayer".  markerType reports the ACTIVE graph's selected
            // marker (arrow/image/text, or none) so ArkTS can enable the
            // marker format items on the graph menu; the desktop computes
            // this in Graph::contextMenuEvent, which now runs here.
            QString cls = QStringLiteral("Blank");
            QWidget *graphHost = nullptr; // Graph* (graph area) or MultiLayer*
            if (QWidget *w = qobject_cast<QWidget *>(obj)) {
                for (QWidget *p = w; p; p = p->parentWidget()) {
                    if (qobject_cast<Graph *>(p)) {
                        cls = QStringLiteral("Graph");
                        graphHost = p;
                        break;
                    }
                    if (qobject_cast<MyWidget *>(p)) {
                        cls = QString::fromLatin1(p->metaObject()->className());
                        graphHost = p;
                        break;
                    }
                    if (qobject_cast<QMdiArea *>(p)) {
                        cls = QStringLiteral("MdiArea");
                        break;
                    }
                }
            }
            QString markerType = QStringLiteral("none");
            if (cls == QStringLiteral("Graph") || cls == QStringLiteral("MultiLayer")) {
                Graph *g = nullptr;
                if (auto *gx = qobject_cast<Graph *>(graphHost))
                    g = gx;
                else if (auto *ml = qobject_cast<MultiLayer *>(graphHost))
                    g = ml->activeGraph();
                if (g && g->markerSelected()) {
                    if (g->arrowMarkerSelected())
                        markerType = QStringLiteral("arrow");
                    else if (g->imageMarkerSelected())
                        markerType = QStringLiteral("image");
                    else
                        markerType = QStringLiteral("text"); // text markers + legends
                }
            }
            QJsonObject p;
            p[QStringLiteral("widgetClass")] = cls;
            p[QStringLiteral("markerType")] = markerType;
            scidavisEmitEvent(QStringLiteral("contextMenuRequest"), p);
            qWarning("[InputProbe] context-menu event on %s (widgetClass=%s, markerType=%s) — passing through (Route A)",
                     obj->metaObject()->className(), qPrintable(cls), qPrintable(markerType));
            // Route A anchor: record the event's global position for
            // embedMenuCore() — menus that anchor on QCursor::pos() (title
            // bar / workspace / graph) would otherwise render at the QPA's
            // stale cursor position.  Blank/MdiArea targets run no menu.
            if (cls != QStringLiteral("Blank") && cls != QStringLiteral("MdiArea")) {
                auto *cm = static_cast<QContextMenuEvent *>(ev);
                ohosBridgeSetContextPos(cm->globalPos());
            }
            return false;
        }
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
            // Route A (2026-08-16): while an inline menu is open no touch
            // gesture may reach the widgets underneath (tap = cell select,
            // drag = table scroll).  The mouse side is already routed by the
            // menu branch; swallow the raw touch channel too.
            if (ohosBridgeMenuActive())
                return true;
            break;
        case QEvent::ToolTip:
            // QToolTip::showText creates a top-level tooltip window -- the
            // single-window QPA SIGSEGVs in QWidgetPrivate::create (device
            // faultlog 20260727155912, same family as QMenu popups).
            return true;
        case QEvent::FocusIn:
            if (obj->isWidgetType())
                qWarning("[InputProbe] focus-in recv=%s", obj->metaObject()->className());
            break;
        case QEvent::KeyPress: {
            auto *ke = static_cast<QKeyEvent *>(ev);
            // Route-A embedded menu (2026-08-16): Esc closes the inline menu
            // (desktop semantics); everything else while a menu is open falls
            // through to the widget (the menu needs no keyboard input yet).
            if (ohosBridgeMenuActive() && ke->key() == Qt::Key_Escape) {
                qWarning("[InputProbe] Esc closes embedded menu");
                ohosBridgeCloseMenu();
                return true;
            }
            qWarning("[InputProbe] key press recv=%s key=0x%x nativeVk=%u text='%s' mods=0x%x ts=%lld",
                     obj->metaObject()->className(), ke->key(), unsigned(ke->nativeVirtualKey()), qPrintable(ke->text()),
                     int(ke->modifiers()), ke->timestamp());
            // Diagnostic markers for the Bluetooth-keyboard Del problem: flag
            // Del/Backspace explicitly so the device log shows whether the key
            // reaches Qt and which Qt key code it carries.
            if (ke->key() == Qt::Key_Delete)
                qWarning("[InputProbe] DEL key reached Qt key=0x%x text='%s'", ke->key(), qPrintable(ke->text()));
            else if (ke->key() == Qt::Key_Backspace)
                qWarning("[InputProbe] BACKSPACE key reached Qt key=0x%x text='%s'", ke->key(), qPrintable(ke->text()));
            // Shortcut-map consultation (2026-08-16, see the declaration
            // above): the plugin bypasses the QPA input queue, so the QAction
            // shortcut map never runs — replicate its once-per-physical-key
            // lookup here.  handleShortcutEvent itself first delivers a
            // ShortcutOverride synchronously (focus widgets like QLineEdit
            // accept it and handle the key themselves — e.g. built-in
            // Ctrl+V/Ctrl+Z in editors), then consults the shortcut map; a
            // match fires the QAction and we swallow every fan-out delivery.
            if (ev->spontaneous()) {
                const quint32 vk = ke->nativeVirtualKey();
                const qint64 ts = ke->timestamp();
                const bool sameKey = (vk == s_lastShortcutVk)
                    && ((ts != 0 && ts == s_lastShortcutTs)
                        || (ts == 0 && qAbs(s_shortcutClock.elapsed() - s_lastShortcutMs) < 100));
                if (!sameKey) {
                    s_lastShortcutVk = vk;
                    s_lastShortcutTs = ts;
                    s_lastShortcutMs = s_shortcutClock.elapsed();
                    // Direct QAction lookup (see triggerActionShortcut): the
                    // QShortcutMap path cannot match (no window activation on
                    // this QPA), so match the actions ourselves — once per
                    // physical key, with ShortcutOverride semantics.
                    QKeyEvent lookupEv(ev->type(), ke->key(),
                                       ke->modifiers() | g_ohosModifierState,
                                       ke->text(), ke->isAutoRepeat(), ke->count());
                    const bool matched = triggerActionShortcut(&lookupEv);
                    if (matched) {
                        s_lastShortcutMatched = true;
                        return true; // consume this delivery
                    }
                    s_lastShortcutMatched = false;
                } else if (s_lastShortcutMatched) {
                    return true; // later fan-out deliveries of a consumed shortcut
                }
            }
            // Full-keyboard fix (2026-08-05): the deployed QPA plugin (alpha_v6) fails
            // to map many OHOS keycodes -- notably KEYCODE_DEL(67; AOSP "DEL" is
            // BACKSPACE, which is why Backspace was dead), KEYCODE_FORWARD_DEL(112)
            // and the uinput Del (2055) -- leaving them as Qt::Key_unknown.  Remap via
            // the nativeVirtualKey the plugin forwards (device-verified: uinput DEL ->
            // key=0x1ffffff).  Safety: map ONLY when Qt's own resolution is unknown or
            // bogus (0x1000061), never for correctly-mapped keys or system keys
            // (Back(2) keeps its own semantics).
            const quint32 nativeVk = ke->nativeVirtualKey();
            const bool isUnresolved = (ke->key() == Qt::Key_unknown || ke->key() == 0x1000061);
            if (isUnresolved) {
                const Qt::Key mapped = keyForNativeVk(nativeVk);
                if (mapped != Qt::Key_unknown) {
                    qWarning("[InputProbe] remap nativeVk=%u key=0x%x -> 0x%x", nativeVk, ke->key(), int(mapped));
                    // Track modifiers from the REMAPPED key: the plugin
                    // leaves modifier keycodes unresolved on many keyboards,
                    // so this is the only way Ctrl/Shift/Alt state reaches
                    // the injected mouse events (Ctrl+click multi-select).
                    trackOhosModifierKey(ev->type(), int(mapped));
                    // Issue list #5 (2026-08-15): OR the InputProbe-tracked
                    // modifier state into every remapped key event - the
                    // plugin's own modifiers field is unreliable for keys
                    // that follow an unresolved modifier press (Bluetooth
                    // keyboards), so Ctrl/Shift/Alt combos otherwise never
                    // match Qt shortcuts and Shift-shifted text is lost.
                    const Qt::KeyboardModifiers remapMods = ke->modifiers() | g_ohosModifierState;
                    QKeyEvent remapEv(ev->type(), int(mapped), remapMods,
                                      printableTextFor(int(mapped), remapMods),
                                      ke->isAutoRepeat(), ke->count());
                    QCoreApplication::sendEvent(obj, &remapEv);
                    return true; // consume original
                }
            }
            // Resolved keys (plugin mapped them): track modifiers directly.
            trackOhosModifierKey(ev->type(), ke->key());
            // Shortcut fix (2026-08-16): QPA delivers spontaneous key events
            // with empty text.  Swallow the original and re-send a copy
            // carrying (a) the synthesized character for plain printable
            // keys, and (b) the OR-ed InputProbe modifier state for shortcut
            // keys — the plugin's modifiers field is unreliable for keys that
            // follow an unresolved modifier press (Bluetooth keyboards), and
            // Ctrl/Alt/Cmd combos produce NO printable text, so the old
            // `!fixed.isEmpty()` gate silently skipped them and Ctrl+C/V/Z
            // never matched Qt shortcuts (device-verified 2026-08-16: Ctrl
            // press + C press both arrive resolved with empty text).
            // Dedup: the plugin fans ONE physical key event out to every
            // widget in the focus chain (7 deliveries observed for BT keys),
            // so re-send at most once per physical event and swallow every
            // fan-out delivery (keyed on nativeVk + event timestamp; with no
            // plugin timestamps the guard degrades to no dedup, which is safe
            // — each delivery is then a separate re-send, same as before).
            if (ev->spontaneous() && ke->text().isEmpty()) {
                const Qt::KeyboardModifiers fixedMods = ke->modifiers() | g_ohosModifierState;
                const QString fixed = printableTextFor(ke->key(), fixedMods);
                const bool modsFixed = (fixedMods != ke->modifiers());
                if (modsFixed || !fixed.isEmpty()) {
                    const quint32 vk = ke->nativeVirtualKey();
                    const qint64 ts = ke->timestamp();
                    const bool alreadySent =
                        (vk == s_lastFixedVk && ts != 0 && ts == s_lastFixedTs);
                    if (!alreadySent) {
                        s_lastFixedVk = vk;
                        s_lastFixedTs = ts;
                        qWarning("[InputProbe] key fixed: vk=%u key=0x%x mods=0x%x->0x%x text='%s' ts=%lld",
                                 vk, int(ke->key()), int(ke->modifiers()),
                                 int(fixedMods), qPrintable(fixed), ts);
                        QKeyEvent fixedEv(ev->type(), ke->key(), fixedMods,
                                          fixed, ke->isAutoRepeat(), ke->count());
                        QCoreApplication::sendEvent(obj, &fixedEv);
                    }
                    return true; // swallow every fan-out delivery
                }
            }
            break;
        }
        case QEvent::KeyRelease: {
            auto *ke = static_cast<QKeyEvent *>(ev);
            qWarning("[InputProbe] key release recv=%s key=0x%x nativeVk=%u text='%s' mods=0x%x ts=%lld",
                     obj->metaObject()->className(), ke->key(), unsigned(ke->nativeVirtualKey()), qPrintable(ke->text()),
                     int(ke->modifiers()), ke->timestamp());
            // Diagnostic markers for the Bluetooth-keyboard Del problem (same
            // tags as KeyPress, so the press→release pair is greppable in the
            // device log).
            if (ke->key() == Qt::Key_Delete)
                qWarning("[InputProbe] DEL key reached Qt key=0x%x text='%s'", ke->key(), qPrintable(ke->text()));
            else if (ke->key() == Qt::Key_Backspace)
                qWarning("[InputProbe] BACKSPACE key reached Qt key=0x%x text='%s'", ke->key(), qPrintable(ke->text()));
            // Full-keyboard fix (2026-08-05): mirror of the KeyPress remap -- the QPA
            // plugin (alpha_v6) fails to map these keycodes on release too, so remap
            // identically to keep press/release paired for widgets that consume
            // release events.
            const quint32 nativeVk = ke->nativeVirtualKey();
            const bool isUnresolved = (ke->key() == Qt::Key_unknown || ke->key() == 0x1000061);
            if (isUnresolved) {
                const Qt::Key mapped = keyForNativeVk(nativeVk);
                if (mapped != Qt::Key_unknown) {
                    qWarning("[InputProbe] remap nativeVk=%u key=0x%x -> 0x%x", nativeVk, ke->key(), int(mapped));
                    // Clear tracked modifier state on release (mirror of the
                    // KeyPress tracking above).
                    trackOhosModifierKey(ev->type(), int(mapped));
                    // Issue list #5 (2026-08-15): OR the InputProbe-tracked
                    // modifier state into every remapped key event - the
                    // plugin's own modifiers field is unreliable for keys
                    // that follow an unresolved modifier press (Bluetooth
                    // keyboards), so Ctrl/Shift/Alt combos otherwise never
                    // match Qt shortcuts and Shift-shifted text is lost.
                    const Qt::KeyboardModifiers remapMods = ke->modifiers() | g_ohosModifierState;
                    QKeyEvent remapEv(ev->type(), int(mapped), remapMods,
                                      printableTextFor(int(mapped), remapMods),
                                      ke->isAutoRepeat(), ke->count());
                    QCoreApplication::sendEvent(obj, &remapEv);
                    return true; // consume original
                }
            }
            // Resolved keys (plugin mapped them): track modifiers directly.
            trackOhosModifierKey(ev->type(), ke->key());
            // Same shortcut fix as KeyPress (2026-08-16): the mods-OR re-send
            // must fire for empty-text events too (Ctrl/Alt combos on
            // release), with the same fan-out dedup.
            if (ev->spontaneous() && ke->text().isEmpty()) {
                const Qt::KeyboardModifiers fixedMods = ke->modifiers() | g_ohosModifierState;
                const QString fixed = printableTextFor(ke->key(), fixedMods);
                const bool modsFixed = (fixedMods != ke->modifiers());
                if (modsFixed || !fixed.isEmpty()) {
                    const quint32 vk = ke->nativeVirtualKey();
                    const qint64 ts = ke->timestamp();
                    const bool alreadySent =
                        (vk == s_lastFixedVk && ts != 0 && ts == s_lastFixedTs);
                    if (!alreadySent) {
                        s_lastFixedVk = vk;
                        s_lastFixedTs = ts;
                        qWarning("[InputProbe] key fixed (release): vk=%u key=0x%x mods=0x%x->0x%x text='%s' ts=%lld",
                                 vk, int(ke->key()), int(ke->modifiers()),
                                 int(fixedMods), qPrintable(fixed), ts);
                        QKeyEvent fixedEv(ev->type(), ke->key(), fixedMods,
                                          fixed, ke->isAutoRepeat(), ke->count());
                        QCoreApplication::sendEvent(obj, &fixedEv);
                    }
                    return true; // swallow every fan-out delivery
                }
            }
            break;
        }
        default:
            break;
        }
        return false; // observe only, never consume
    }
};

// ── ETS → Qt mouse injection channel ────────────────────────────────────
// The QPA plugin drops LEFT-button mouse events entirely (ArkUI re-routes
// them into the touch pipeline which the plugin filters out; only RIGHT
// button reaches dispatchMouseEvent — verified via InputProbe).  The ETS
// layer forwards left-button events through libqohos.so, which dlsym's this
// export.  Coordinates are physical pixels (HighDpi scaling disabled).
// action: 0 = press, 1 = release, 2 = move (left button held)
// Forward decl: g_mainWindow is defined below (L698); the injection diag
// below logs subwindow state, so declare it here.
class ApplicationWindow;
// UAF fix (2026-08-16): QPointer auto-nulls when the ApplicationWindow is
// destroyed, so every handler's `if (!g_mainWindow)` guard also covers the
// destroyed-window case (crash 2026-08-15 22:39:33: a queued scidavis_call
// ran windowsList() on a destroyed ApplicationWindow -> SIGSEGV in
// projectFolder() -> folders.topLevelItem(0)).  A raw pointer can never
// distinguish "null" from "dangling"; the queued-command shutdown gate is
// g_qtShuttingDown below.
extern QPointer<ApplicationWindow> g_mainWindow;
// Shutdown gate (UAF fix 2026-08-16): set when the application begins
// teardown (aboutToQuit / ApplicationWindow destroyed).  scidavis_call
// refuses to queue and the queued lambdas skip execution once set, so no
// command can run against a half-destroyed Qt object graph while the QPA
// plugin still pumps events during teardown.
static std::atomic<bool> g_qtShuttingDown{false};
// Phase 7: embedDialog() is defined in ohos_bridge.cpp (same library) and is
// deliberately NOT declared in ohos_bridge.h — it is the embedder underneath
// the whitelisted dialog path, called from the interposers inside that file.
// menuAction "undo_redo_history" calls it directly so the QUndoView dialog
// embeds without a whitelist entry (QDialog is forward-declared in
// ohos_bridge.h, included above).
int embedDialog(QDialog *dlg);
extern "C" __attribute__((visibility("default")))
void scidavis_inject_mouse(float x, float y, int button, int action)
{
    QCoreApplication *core = QCoreApplication::instance();
    if (!core)
        return;
    // Hop onto the Qt GUI thread; NAPI calls arrive on the ArkTS thread.
    QMetaObject::invokeMethod(core, [x, y, button, action]() {
        // NOTE: the previous 30ms global debounce was removed.  It did not
        // distinguish button/action, so a fast tap lost its release event
        // (<30ms after press -> stuck button) and drag moves arriving every
        // 8-16ms were mostly dropped.  Duplicate delivery is prevented at
        // the source instead: the ETS overlay skips LEFT clicks in onMouse
        // (left arrives via onTouch only), so no event reaches here twice.

        QPointer<QWindow> win = QGuiApplication::focusWindow();
        if (!win) {
            const auto tls = QGuiApplication::topLevelWindows();
            for (QWindow *w : tls)
                if (w->isVisible()) { win = w; break; }
        }
        if (!win || !win->isExposed())
            return;
        // Double-click tracking: static locals are safe here -- this lambda
        // always runs on the Qt GUI thread (QueuedConnection), never
        // concurrently.  (The previous make_shared-per-event version reset
        // the timer on every call, so DblClick was never synthesized.)
        static QElapsedTimer sPressTimer;
        static QPoint sLastPress;
        const QPoint localP{ static_cast<int>(x), static_cast<int>(y) };
        const QPoint globalP = win->mapToGlobal(localP);
        // 2in1 coord diag (2026-08-13): log injected presses plus the
        // current main-window contents margins and active subwindow rect
        // so the click-misalignment can be pinned down exactly.
        if (action == 0 || action == 1) {
            ApplicationWindow *mw = g_mainWindow;
            const QMargins m = mw ? mw->contentsMargins() : QMargins();
            QWidget *sub = mw ? mw->d_workspace.activeSubWindow() : nullptr;
            const QRect sr = sub ? sub->geometry() : QRect();
            const QPoint mdiGlobal = mw ? mw->d_workspace.mapToGlobal(QPoint(0, 0)) : QPoint();
            const QRect mdiGeo = mw ? mw->d_workspace.geometry() : QRect();
            const QRect mwGeo = mw ? mw->geometry() : QRect();
            const QPoint winPos = win->position();
            const QPoint subGlobal = sub ? sub->mapToGlobal(QPoint(0, 0)) : QPoint();
            // 2026-08-13 close-button diag: the sub-window's own margins
            // (top = title-bar strip), the embedded widget geometry and the
            // style title-bar metric decide whether the × close button is
            // reachable (covered by the table → click lands on the table).
            const QMargins sm = sub ? sub->contentsMargins() : QMargins();
            QMdiSubWindow *ms = sub ? qobject_cast<QMdiSubWindow *>(sub) : nullptr;
            QWidget *subChild = ms ? ms->widget() : nullptr;
            const QRect cg = subChild ? subChild->geometry() : QRect();
            const int tbH = sub ? sub->style()->pixelMetric(QStyle::PM_TitleBarHeight,
                                                            nullptr, sub) : -1;
            qWarning("[SciDAVis] DIAG act=%d btn=%d at (%d,%d) winPos=%d,%d winGeo=%dx%d mwGeo=%d,%d,%dx%d margins=%d,%d,%d,%d mdi=%d,%d,%dx%d mdiGlobal=%d,%d sub=%d,%d,%dx%d subGlobal=%d,%d subM=%d,%d,%d,%d child=%d,%d,%dx%d tbH=%d",
                     action, button, localP.x(), localP.y(),
                     winPos.x(), winPos.y(), win->width(), win->height(),
                     mwGeo.x(), mwGeo.y(), mwGeo.width(), mwGeo.height(),
                     m.left(), m.top(), m.right(), m.bottom(),
                     mdiGeo.x(), mdiGeo.y(), mdiGeo.width(), mdiGeo.height(),
                     mdiGlobal.x(), mdiGlobal.y(),
                     sr.x(), sr.y(), sr.width(), sr.height(),
                     subGlobal.x(), subGlobal.y(),
                     sm.left(), sm.top(), sm.right(), sm.bottom(),
                     cg.x(), cg.y(), cg.width(), cg.height(), tbH);
        }
        const Qt::MouseButton srcBtn = (button == 2) ? Qt::RightButton
                                     : (button == 3) ? Qt::MiddleButton
                                                     : Qt::LeftButton;
        // Track the injected press/release stream so hover moves arrive at
        // Qt WITHOUT the held-button bit.  QMdiSubWindow::mouseMoveEvent
        // branches on buttons()==0 to refresh the move/resize operation and
        // cursor shape (SizeAll/SizeVer/SizeHor/SizeFDiag/SizeBDiag) and on
        // the drag branch otherwise — previously every action=2 move carried
        // srcBtn, so Qt always took the drag branch: the resize cursors never
        // appeared while hovering a sub-window frame, and an operation set by
        // a press stayed stuck (special cursor persisted after the pointer
        // left the frame).  Static locals are safe here — the lambda always
        // runs on the Qt GUI thread (QueuedConnection).
        static Qt::MouseButtons s_pressedButtons = Qt::NoButton;
        Qt::MouseButton btn = srcBtn;
        Qt::MouseButtons buttons = srcBtn;
        QEvent::Type type = QEvent::MouseButtonPress;
        switch (action) {
        case 0: { // press — synthesize double click like QGuiApplication would
            s_pressedButtons |= srcBtn;
            const bool dbl = sPressTimer.isValid() && !sPressTimer.hasExpired(400)
                    && (localP - sLastPress).manhattanLength() < 10;
            type = dbl ? QEvent::MouseButtonDblClick : QEvent::MouseButtonPress;
            sPressTimer.restart();
            sLastPress = localP;
            break;
        }
        case 1:
            s_pressedButtons &= ~srcBtn;
            type = QEvent::MouseButtonRelease;
            buttons = Qt::NoButton;
            break;
        case 2: // move — hover (no buttons held) or drag (buttons held)
            type = QEvent::MouseMove;
            btn = Qt::NoButton;
            buttons = s_pressedButtons;
            break;
        default:
            return;
        }
        // Bounds sanity check: reject coordinates far outside the window
        // to prevent Qt widget code from asserting on bogus positions.
        if (localP.x() < -100 || localP.y() < -100 ||
            localP.x() > win->width() + 100 || localP.y() > win->height() + 100)
            return;

        // Modifier state (2026-08-13): QGuiApplication::keyboardModifiers()
        // covers whatever the QPA reports; g_ohosModifierState covers the
        // remapped Bluetooth-keyboard modifiers.  OR both — the injected
        // click must carry Ctrl/Shift/Alt so Ctrl+click multi-selection
        // works (this previously hardcoded Qt::NoModifier).
        const Qt::KeyboardModifiers mods =
                QGuiApplication::keyboardModifiers() | g_ohosModifierState;
        // Hover move BEFORE the press (2026-08-13): the dropped QPA native
        // channel used to deliver a no-button move at the touch point ahead
        // of every press (touch pipeline Move→Press).  QMdiSubWindow only
        // arms move/resize when its currentOperation was set by exactly such
        // a move, so a bare injected press left the operation None and MDI
        // sub-window drag/resize died on the tablet after the dual-channel
        // filter.  Replicate the native sequence through the surviving
        // channel (harmless for tables/graphs — hover state only).
        if (action == 0) {
            QMouseEvent hover(QEvent::MouseMove, localP, localP, globalP,
                              Qt::NoButton, Qt::NoButton, mods,
                              Qt::MouseEventSynthesizedByApplication);
            try {
                QCoreApplication::sendEvent(win, &hover);
            } catch (...) {
                qWarning("[SciDAVis] scidavis_inject_mouse: hover sendEvent crashed, caught");
            }
        }
        QMouseEvent ev(type, localP, localP, globalP, btn, buttons, mods,
                       Qt::MouseEventSynthesizedByApplication);
        // Deliver to the QWidgetWindow: its event() runs the full widget-level
        // dispatch (popup handling, implicit grab, child lookup).
        // Wrap in try-catch as defensive measure against widget-tree transitions.
        try {
            QCoreApplication::sendEvent(win, &ev);
        } catch (...) {
            qWarning("[SciDAVis] scidavis_inject_mouse: sendEvent crashed, caught");
        }
    }, Qt::QueuedConnection);
}


// ── ETS → Qt wheel injection channel ────────────────────────────────────
// Phase-7 wheel restore 2026-08-13: ArkUI delivers mouse-wheel as AxisEvent
// (onAxisEvent); the ETS layer forwards it via libqohos.so sendWheel →
// dlsym("scidavis_inject_wheel").  This Qt build COMPILES OUT QWheelEvent
// (QT_CONFIG(wheelevent)==0 in qt-ohos 5.15.12 — qevent.h L171), so a
// QWheelEvent cannot be constructed; instead we scroll the
// QAbstractScrollArea under the cursor directly — the same end result as
// QAbstractScrollArea::wheelEvent, which scrolls the vertical scrollbar by
// (angleDelta.y()/120) * singleStep() (angleDelta = deltaY*120).
//
// Contract (matches qohos.cpp SendWheel): x/y are window-local physical
// pixels, same convention as scidavis_inject_mouse (HighDpi disabled);
// deltaY > 0 = ArkUI verticalAxisValue positive = wheel up → content scrolls
// down → value must INCREASE (2026-08-13 真机实测方向反了: old `value -`
// made content scroll up, opposite to desktop Qt).  i.e. `value + rows*step`.
// deltaY is the ArkUI AxisEvent vertical axis value in
// degrees, one notch = 15°; qRound(deltaY/15.0) converts to notch count
// × scrollbar singleStep (2026-08-06 scaling fix: raw `deltaY*step` mixed
// deg×px and scrolled 15 rows/notch).
extern "C" __attribute__((visibility("default")))
void scidavis_inject_wheel(float x, float y, int deltaY)
{
    QCoreApplication *core = QCoreApplication::instance();
    if (!core)
        return;
    QMetaObject::invokeMethod(core, [x, y, deltaY]() {
        QPointer<QWindow> win = QGuiApplication::focusWindow();
        if (!win) {
            const auto tls = QGuiApplication::topLevelWindows();
            for (QWindow *w : tls)
                if (w->isVisible()) { win = w; break; }
        }
        if (!win || !win->isExposed())
            return;
        const QPoint localP{ static_cast<int>(x), static_cast<int>(y) };
        if (localP.x() < -100 || localP.y() < -100 ||
            localP.x() > win->width() + 100 || localP.y() > win->height() + 100)
            return;
        if (deltaY == 0)
            return;
        // Hit-test the widget under the cursor (fall back to the focused
        // widget — covers embedded modal dialogs such as About where the
        // credits QTextEdit holds focus) and scroll the nearest
        // QAbstractScrollArea's vertical scrollbar.  TableViewWidget /
        // MatrixViewWidget (QTableView subclasses) are QAbstractScrollArea,
        // so this reaches them; About's credits_box is a QTextEdit inside
        // the embedded QDialog (embedDialog) — also reached, its widgets
        // are real children of the main window hierarchy.
        QWidget *w = QApplication::widgetAt(win->mapToGlobal(localP));
        if (!w)
            w = QApplication::focusWidget();
        QAbstractScrollArea *sa = nullptr;
        for (QWidget *p = w; p; p = p->parentWidget()) {
            sa = qobject_cast<QAbstractScrollArea *>(p);
            if (sa)
                break;
        }
        if (!sa)
            return;
        QScrollBar *sb = sa->verticalScrollBar();
        if (!sb || !sb->isVisible())
            return;
        int step = sb->singleStep();
        if (step <= 0)
            step = 20;
        int rows = qRound(deltaY / 15.0);
        if (rows == 0)
            return;
        try {
            sb->setValue(sb->value() + rows * step);
            sb->update(); // embedded modal dialogs (embedDialog): force repaint
        } catch (...) {
            qWarning("[SciDAVis] scidavis_inject_wheel: scroll failed, caught");
        }
    }, Qt::QueuedConnection);
}


// UAF fix (2026-08-16): g_mainWindow is a QPointer — it auto-nulls when the
// ApplicationWindow is destroyed, so the `if (!g_mainWindow)` guards in every
// handler also cover the destroyed-window case (crash 2026-08-15 22:39:33:
// a queued scidavis_call ran windowsList() on a destroyed ApplicationWindow
// -> SIGSEGV in projectFolder() -> folders.topLevelItem(0)).  A raw pointer
// can never distinguish "null" from "dangling".
QPointer<ApplicationWindow> g_mainWindow;

// --- Plot helper functions ------------------------------------------------
static std::string plotListJson()
{
    if (!g_mainWindow) return "[]";
    QJsonArray plots;
    QList<MyWidget *> windows = g_mainWindow->windowsList();
    for (MyWidget *w : windows) {
        if (!w->inherits("MultiLayer")) continue;
        MultiLayer *ml = static_cast<MultiLayer *>(w);
        QJsonObject plotObj;
        plotObj["name"] = ml->name();
        plotObj["label"] = ml->windowLabel();
        plotObj["layers"] = ml->layers();
        QJsonArray graphs;
        QWidgetList gList = ml->graphPtrs();
        for (QWidget *gw : gList) {
            Graph *g = qobject_cast<Graph *>(gw);
            if (!g) continue;
            QJsonObject gObj;
            gObj["curves"] = g->curves();
            QJsonArray curveNames;
            for (int i = 0; i < g->curves(); i++) {
                QwtPlotCurve *pc = g->curve(i);
                if (pc) curveNames.append(pc->title().text());
            }
            gObj["curveNames"] = curveNames;
            graphs.append(gObj);
        }
        plotObj["graphs"] = graphs;
        plots.append(plotObj);
    }
    return QJsonDocument(plots).toJson(QJsonDocument::Compact).toStdString();
}

static std::string plotDataJson(const QString &plotName, int graphIdx, int curveIdx)
{
    if (!g_mainWindow) return "[]";
    QList<MyWidget *> windows = g_mainWindow->windowsList();
    for (MyWidget *w : windows) {
        if (!w->inherits("MultiLayer")) continue;
        MultiLayer *ml = static_cast<MultiLayer *>(w);
        if (ml->name() != plotName) continue;
        QWidgetList gList = ml->graphPtrs();
        if (graphIdx < 0 || graphIdx >= gList.size()) return "[]";
        Graph *g = qobject_cast<Graph *>(gList[graphIdx]);
        if (!g || curveIdx < 0 || curveIdx >= g->curves()) return "[]";
        QwtPlotCurve *pc = g->curve(curveIdx);
        if (!pc) return "[]";
        const QwtData &d = pc->data();
        size_t n = d.size();
        QJsonObject root;
        root["title"] = pc->title().text();
        root["points"] = static_cast<int>(n);
        QJsonArray xArr, yArr;
        for (size_t i = 0; i < n; i++) {
            xArr.append(d.x(i));
            yArr.append(d.y(i));
        }
        root["x"] = xArr;
        root["y"] = yArr;
        DataCurve *dc = dynamic_cast<DataCurve *>(pc);
        if (dc) {
            root["xColumn"] = dc->xColumnName();
            root["yColumn"] = dc->yColumnName();
        }
        return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
    }
    return "[]";
}

// --- NAPI -> Qt command dispatch ------------------------------------------

static std::string tableListJson()
{
    if (!g_mainWindow) return "[]";
    QList<MyWidget *> *tables = g_mainWindow->tableList();
    if (!tables) return "[]";
    QJsonArray arr;
    for (MyWidget *w : *tables) {
        Table *t = qobject_cast<Table *>(w);
        if (!t) continue;
        QJsonObject obj;
        obj["name"] = t->name();
        obj["label"] = t->windowLabel();
        obj["rows"] = t->rowCount();
        obj["cols"] = t->columnCount();
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact).toStdString();
}

static std::string tableDataJson(const QString &tableName)
{
    if (!g_mainWindow) return "[]";
    Table *t = g_mainWindow->table(tableName);
    if (!t) return "[]";
    int rows = t->rowCount();
    int cols = t->columnCount();
    QJsonObject root;
    root["name"] = t->name();
    root["rows"] = rows;
    root["cols"] = cols;
    QJsonArray colNames;
    for (int c = 0; c < cols; c++)
        colNames.append(t->colName(c));
    root["colNames"] = colNames;
    QJsonArray data;
    for (int r = 0; r < rows; r++) {
        QJsonArray row;
        for (int c = 0; c < cols; c++)
            row.append(t->text(r, c));
        data.append(row);
    }
    root["data"] = data;
    return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

// Resolve the table a command targets: explicit tableId, else the active
// MDI subwindow if it is a Table.
static Table *resolveTable(const QJsonObject &args)
{
    if (!g_mainWindow) return nullptr;
    QString id = args["tableId"].toString();
    if (!id.isEmpty())
        return g_mainWindow->table(id);
    return qobject_cast<Table *>(g_mainWindow->d_workspace.activeSubWindow());
}

// Resolve the matrix a command targets (same pattern as resolveTable).
static Matrix *resolveMatrix(const QJsonObject &args)
{
    if (!g_mainWindow) return nullptr;
    QString id = args["matrixId"].toString();
    if (!id.isEmpty())
        return g_mainWindow->matrix(id);
    return qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
}

// Resolve the MultiLayer a command targets (explicit plotId, else the
// active MDI subwindow if it is a plot).
static MultiLayer *resolvePlot(const QJsonObject &args)
{
    if (!g_mainWindow) return nullptr;
    QString id = args["plotId"].toString();
    if (!id.isEmpty()) {
        for (QMdiSubWindow *w : g_mainWindow->d_workspace.subWindowList()) {
            auto *ml = qobject_cast<MultiLayer *>(w);
            if (ml && ml->name() == id) return ml;
        }
        return nullptr;
    }
    return qobject_cast<MultiLayer *>(g_mainWindow->d_workspace.activeSubWindow());
}

// Resolve the active graph's "current" curve KEY for the Phase-6 format
// dialogs (edit_range/associations/plot_details).  Desktop semantics: the
// curve the user right-clicked, carried in the action data.  On device the
// InputProbe blocks context menus before Graph::contextMenuEvent can record
// it, and the QwtPicker-based range selector is a stub (same root cause as
// the disabled move_points/remove_bad menu items), so Graph::
// selectedCurveID() is usually -1 → fall back to the most recently added
// analysable curve (the analyzeCurve command's default).  An explicit
// "curve" arg (curve name) from ArkTS wins when present.  Returns -1 when
// nothing resolvable.
static int menuCurveKey(Graph *g, const QJsonObject &args)
{
    if (!g)
        return -1;
    const QString name = args["curve"].toString();
    if (!name.isEmpty()) {
        const int idx = g->curveIndex(name);
        if (idx >= 0)
            return int(g->curveKey(idx));
    }
    const int sel = g->selectedCurveID();
    if (sel >= 0)
        return sel;
    const QStringList names = g->analysableCurvesList();
    if (names.isEmpty())
        return -1;
    const int idx = g->curveIndex(names.last());
    return idx >= 0 ? int(g->curveKey(idx)) : -1;
}

// UI state for the ArkTS shell: active window, window list, undo/redo
// availability.  Drives menu enabling/greying and context-menu switching.
static std::string uiStateJson()
{
    if (!g_mainWindow) return "{}";
    QJsonObject root;
    QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
    MyWidget *aw = qobject_cast<MyWidget *>(sub);
    root["activeType"] = aw ? QString::fromLatin1(aw->metaObject()->className()) : QString();
    root["activeName"] = aw ? aw->name() : QString();
    QJsonArray wins;
    // windowsList() concatenates the folder-tree windows with the
    // hiddenWindows/outWindows lists; analysis result tables created via
    // newHiddenTable() sit in BOTH (initTable adds them to the folder,
    // hideWindow appends to hiddenWindows), so dedupe by pointer here or
    // the Windows menu shows Smoothed1/Derivative1/... twice.
    QSet<MyWidget *> seen;
    for (MyWidget *w : g_mainWindow->windowsList()) {
        if (seen.contains(w)) continue;
        seen.insert(w);
        QJsonObject o;
        o["name"] = w->name();
        o["type"] = QString::fromLatin1(w->metaObject()->className());
        o["label"] = w->windowLabel();
        o["status"] = int(w->status());
        o["active"] = (w == aw);
        wins.append(o);
    }
    // Additional dedup by name (pointer dedup above may miss duplicates
    // from windowsList() concatenating folder + hiddenWindows + outWindows
    // where the same window appears as different pointer entries)
    QSet<QString> seenNames;
    QJsonArray dedupedWins;
    for (int i = 0; i < wins.size(); i++) {
        const QJsonObject &o = wins[i].toObject();
        const QString name = o["name"].toString();
        if (seenNames.contains(name)) continue;
        seenNames.insert(name);
        dedupedWins.append(o);
    }
    root["windows"] = dedupedWins;
    QAction *undo = g_mainWindow->ohosActionUndo();
    QAction *redo = g_mainWindow->ohosActionRedo();
    root["undoEnabled"] = undo && undo->isEnabled();
    root["redoEnabled"] = redo && redo->isEnabled();
    root["explorerVisible"] = g_mainWindow->explorerWindow.isVisible();
    root["logVisible"] = g_mainWindow->logWindow.isVisible();
    return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

// Qt → ArkTS push for MDI subwindow activation: the ArkTS menu bar rebuilds
// on windowActivated, mirroring the activeType/activeName getUiState reports
// (className() of the active MyWidget, e.g. "Table"/"Matrix"/"MultiLayer").
// Connected to QMdiArea::subWindowActivated in main() so BOTH activation by
// clicking a window in the workspace AND activation via the Windows→win:*
// menu (activateSubWindow → setActiveSubWindow) fire the event.
static void emitWindowActivated(QMdiSubWindow *sub)
{
    MyWidget *aw = qobject_cast<MyWidget *>(sub);
    if (!aw)
        return;
    QJsonObject p;
    p[QStringLiteral("activeType")] = QString::fromLatin1(aw->metaObject()->className());
    p[QStringLiteral("activeName")] = aw->name();
    scidavisEmitEvent(QStringLiteral("windowActivated"), p);
}

static QJsonObject projectTreeJson(Folder *f)
{
    QJsonObject node;
    node["name"] = f->name();
    node["type"] = QStringLiteral("Folder");
    QJsonArray children;
    for (MyWidget *w : f->windowsList()) {
        QJsonObject wn;
        wn["name"] = w->name();
        wn["type"] = QString::fromLatin1(w->metaObject()->className());
        wn["children"] = QJsonArray();
        children.append(wn);
    }
    for (Folder *sub : f->folders())
        children.append(projectTreeJson(sub));
    node["children"] = children;
    return node;
}

// --- Command registry -------------------------------------------------------
// Every scidavis_call command is one entry here; handlers always run on the
// Qt GUI thread (queued from scidavis_call).  Adding a command = adding an
// entry (and, for read-only commands, listing it in queryCommands()).

using CommandHandler = std::function<std::string(const QJsonObject &)>;

static std::string jsonError(const std::string &msg)
{
    return "{\"success\":false,\"error\":\"" + msg + "\"}";
}

// Read-only commands: scidavis_call waits (3s timeout) and returns the
// payload; everything else is queued fire-and-forget (deadlock guard).
static const std::set<std::string> &queryCommands()
{
    static const std::set<std::string> q = { "ping",        "getTableList", "getTableData",
                                             "getColumnInfo", "getPlotList", "getPlotData",
                                              "getUiState", "getClipboardText", "getGraphCurves",
                                              "get_curve_contents",
                                              "getProjectTree", "getPreference", "getNoteData",
                                              "getMatrixData", "tableSize", "getAxisConfig",
                                              "getPlotAssociations", "getCurveInfo",
                                              "getActiveWindowInfo", "getStartPath",
                                              "checkWorkFolderAccess",
                                              "getRecentProjects", "isProjectModified" };
    return q;
}

static const std::map<std::string, CommandHandler> &commandRegistry()
{
    static const std::map<std::string, CommandHandler> reg = {
        { "ping",
          [](const QJsonObject &) -> std::string { return "{\"success\":true,\"pong\":true}"; } },

        { "getTableList",
          [](const QJsonObject &) -> std::string {
              return "{\"success\":true,\"data\":" + tableListJson() + "}";
          } },

        { "getTableData",
          [](const QJsonObject &args) -> std::string {
              // Empty tableId falls back to the active table (the ArkTS
              // @Prop may not have propagated yet when a dialog opens).
              QString id = args["tableId"].toString();
              if (id.isEmpty()) {
                  Table *t = resolveTable(args);
                  if (t) id = t->name();
              }
              return "{\"success\":true,\"data\":" + tableDataJson(id) + "}";
          } },

        { "getColumnInfo",
          [](const QJsonObject &args) -> std::string {
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              int cols = t->columnCount();
              QJsonArray arr;
              for (int c = 0; c < cols; c++) {
                  Column *col = t->column(c);
                  if (!col) continue;
                  QJsonObject obj;
                  obj["col"] = c;
                  obj["name"] = col->name();
                  // Column mode / type
                  QString type;
                  switch (col->columnMode()) {
                  case SciDAVis::ColumnMode::Numeric:  type = QStringLiteral("Numeric");  break;
                  case SciDAVis::ColumnMode::Text:     type = QStringLiteral("Text");     break;
                  case SciDAVis::ColumnMode::DateTime: type = QStringLiteral("DateTime"); break;
                  case SciDAVis::ColumnMode::Month:    type = QStringLiteral("Month");    break;
                  case SciDAVis::ColumnMode::Day:      type = QStringLiteral("Day");      break;
                  default: type = QStringLiteral("Numeric"); break;
                  }
                  obj["type"] = type;
                  // Description — use comment(), or fall back to name
                  QString desc = col->comment();
                  if (desc.isEmpty()) desc = col->name();
                  obj["description"] = desc;
                  // Formula for row 0
                  obj["formula"] = col->formula(0);
                  // Plot designation
                  QString pd;
                  switch (col->plotDesignation()) {
                  case SciDAVis::X:             pd = QStringLiteral("X");      break;
                  case SciDAVis::Y:             pd = QStringLiteral("Y");      break;
                  case SciDAVis::Z:             pd = QStringLiteral("Z");      break;
                  case SciDAVis::xErr:          pd = QStringLiteral("XError"); break;
                  case SciDAVis::yErr:          pd = QStringLiteral("YError"); break;
                  case SciDAVis::noDesignation: pd = QStringLiteral("None");   break;
                  default:                      pd = QStringLiteral("None");   break;
                  }
                  obj["plotDesignation"] = pd;
                  arr.append(obj);
              }
              QJsonObject root;
              root["success"] = true;
              root["data"] = arr;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "getUiState",
          [](const QJsonObject &) -> std::string {
              return "{\"success\":true,\"data\":" + uiStateJson() + "}";
          } },

        { "getProjectTree",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QJsonObject root;
              root["success"] = true;
              root["data"] = projectTreeJson(g_mainWindow->projectFolder());
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "newTable",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              Table *t = g_mainWindow->newTable();
              if (!t) return jsonError("failed");
              // Notify ArkTS that the table list changed
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              return "{\"success\":true,\"data\":" + tableListJson() + "}";
          } },

        { "getPlotList",
          [](const QJsonObject &) -> std::string {
              return "{\"success\":true,\"data\":" + plotListJson() + "}";
          } },

        { "getPlotData",
          [](const QJsonObject &args) -> std::string {
              std::string data = plotDataJson(args["plotId"].toString(),
                                              args["graphIdx"].toInt(), args["curveIdx"].toInt());
              return "{\"success\":true,\"data\":" + data + "}";
          } },

        { "setCellValue",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              Table *t = g_mainWindow->table(args["tableId"].toString());
              int row = args["row"].toInt();
              int col = args["col"].toInt();
              if (!t || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
                  return jsonError("invalid cell");
              t->setText(row, col, args["value"].toString());
              // Notify ArkTS to refresh table data
              {
                  QJsonObject ep;
                  ep["tableId"] = args["tableId"].toString();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              return "{\"success\":true}";
          } },

        // ── Embedded file dialogs (route 5, 2026-08-14) ──────────────
        // The ArkTS shell grants a work folder via DocumentSelectMode.FOLDER
        // and pushes the REAL path here; embedded QFileDialog instances
        // (getOpenFileName/getExistingDirectory/... interposers) start
        // browsing in it, so the user selects files/folders directly inside
        // the authorized tree rendered by Qt.  Qt can read/write that tree
        // without any sandbox copy (route 5 probe verified QFile access).
        { "setWorkFolderPath",
          [](const QJsonObject &args) -> std::string {
              scidavisSetWorkFolderPath(args["path"].toString().toUtf8().constData());
              return "{\"success\":true}";
          } },

        // Checks whether the (restored) work folder is still directly
        // readable/writable from Qt — the FOLDER grant is process-scoped on
        // OpenHarmony (no persistPermission API), so after a restart the
        // path may exist in prefs but the access is gone.  ArkTS calls this
        // on startup to decide whether to re-run the FOLDER picker.
        { "checkWorkFolderAccess",
          [](const QJsonObject &args) -> std::string {
              const QString path = args["path"].toString();
              QJsonObject r;
              const QFileInfo fi(path);
              r["exists"] = fi.exists();
              r["isDir"] = fi.isDir();
              bool readable = false;
              if (fi.exists() && fi.isDir()) {
                  QDir d(path);
                  // entryList on an unauthorized path yields nothing or an
                  // error; on an authorized path it lists real entries.
                  const int n = d.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name).count();
                  readable = n >= 0;
                  r["entryCount"] = n;
              } else {
                  r["entryCount"] = -1;
              }
              r["readable"] = readable;
              r["success"] = true;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "importASCII",
          [](const QJsonObject &args) -> std::string {
              // Queued command; ArkTS copies the picked file into the sandbox
              // first.  Completion is announced through the message event.
              if (!g_mainWindow) return jsonError("no mw");
              QString fn = args["filePath"].toString();
              if (fn.isEmpty() || !QFile::exists(fn)) return jsonError("file not found");
              QString sep = args["separator"].toString("\t");
              QStringList files;
              files << fn;
              // Only set locale if explicitly provided
              QLocale locale = QLocale::c();
              if (args.contains("locale"))
                  locale = QLocale(args["locale"].toString());
              g_mainWindow->importASCII(files, args["importMode"].toInt(0), sep,
                                        args["ignoredLines"].toInt(0),
                                        args["renameColumns"].toBool(true),
                                        args["stripSpaces"].toBool(true),
                                        args["simplifySpaces"].toBool(false),
                                        args["convertToNumeric"].toBool(true), locale);
              QJsonObject p;
              p["title"] = QObject::tr("Import ASCII");
              p["text"] = QObject::tr("Imported: ") + QFileInfo(fn).fileName();
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS that table list changed after import
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              return "{\"success\":true,\"data\":" + tableListJson() + "}";
          } },

        { "exportASCII",
          [](const QJsonObject &args) -> std::string {
              // Direct-to-path export (Table::exportASCII); the desktop
              // ApplicationWindow::exportASCII pops a QFileDialog which the
              // single-window QPA cannot show.  Queued; the asciiExported
              // event triggers the ArkTS picker copy-out.
              if (!g_mainWindow) return jsonError("no mw");
              Table *t = g_mainWindow->table(args["tableId"].toString());
              if (!t) return jsonError("table not found");
              QString fn = args["path"].toString();
              if (fn.isEmpty()) return jsonError("no path");
              bool ok = t->exportASCII(fn, args["separator"].toString("\t"),
                                       args["colNames"].toBool(true), false);
              QJsonObject done; done["ok"] = ok; done["path"] = fn;
              scidavisEmitEvent(QStringLiteral("asciiExported"), done);
              if (!ok) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Export ASCII");
                  p["text"] = QObject::tr("Failed to export: ") + QFileInfo(fn).fileName();
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              }
              return ok ? std::string("{\"success\":true}") : jsonError("export failed");
          } },

        // ── Table operations (Phase 2) ─────────────────────────────────
        // All queued (mutations); results are announced via the message
        // event so the ArkTS shell can toast them.  The desktop dialogs
        // (SetColValuesDialog/SortDialog/...) cannot be shown on the
        // single-window QPA, so ArkTS dialogs supply the parameters.
        { "addTableColumns",
          [](const QJsonObject &args) -> std::string {
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              int count = qBound(1, args["count"].toInt(1), 100);
              for (int i = 0; i < count; i++)
                  t->addCol();
              // Notify ArkTS to refresh table data + list
              {
                  QJsonObject ep;
                  ep["tableId"] = args["tableId"].toString();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              QJsonObject p;
              p["title"] = QObject::tr("Add Columns");
              p["text"] = QObject::tr("Added %1 column(s) to %2").arg(count).arg(t->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

        { "setColumnValues",
          [](const QJsonObject &args) -> std::string {
              // muParser formula, whole column (desktop SetColValuesDialog).
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              int col = args["col"].toInt(-1);
              if (col < 0 || col >= t->numCols()) return jsonError("invalid column");
              QString formula = args["formula"].toString();
              if (formula.isEmpty()) return jsonError("empty formula");
              t->setCommand(col, formula);
              bool ok = t->recalculate(col, false);
              QJsonObject p;
              p["title"] = QObject::tr("Set Column Values");
              p["text"] = ok ? t->colName(col) + " = " + formula
                             : QObject::tr("Formula error: ") + formula;
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh table data after column recalculation
              {
                  QJsonObject ep;
                  ep["tableId"] = args["tableId"].toString();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              return ok ? std::string("{\"success\":true}") : jsonError("recalculate failed");
          } },

        { "sortTable",
          [](const QJsonObject &args) -> std::string {
              // future::Table::sortColumns — no-dialog core of SortDialog.
              // leading < 0 sorts every column separately.
              Table *t = resolveTable(args);
              if (!t || !t->d_future_table) return jsonError("table not found");
              bool asc = args["ascending"].toBool(true);
              int leadIdx = args["leading"].toInt(-1);
              QList<Column *> cols;
              for (int i = 0; i < t->numCols(); i++)
                  cols << t->column(i);
              Column *leading =
                      (leadIdx >= 0 && leadIdx < t->numCols()) ? t->column(leadIdx) : nullptr;
              t->d_future_table->sortColumns(leading, cols, asc);
              QJsonObject p;
              p["title"] = QObject::tr("Sort Table");
              p["text"] = QObject::tr("Sorted %1 (%2)").arg(t->name())
                      .arg(asc ? QObject::tr("ascending") : QObject::tr("descending"));
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh table data after sort
              {
                  QJsonObject ep;
                  ep["tableId"] = args["tableId"].toString();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              return "{\"success\":true}";
          } },

        { "tableStatistics",
          [](const QJsonObject &args) -> std::string {
              // Column/row statistics table (desktop acts on the selection;
              // ArkTS has no selection info, so default to all).
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              bool onRows = args["type"].toString() == "rows";
              QList<int> targets;
              if (args["targets"].isArray()) {
                  for (const QJsonValue &v : args["targets"].toArray())
                      targets << v.toInt();
              } else {
                  int n = onRows ? t->numRows() : t->numCols();
                  for (int i = 0; i < n; i++)
                      targets << i;
              }
              if (targets.isEmpty()) return jsonError("no targets");
              TableStatistics *s = g_mainWindow->newTableStatistics(
                      t, onRows ? TableStatistics::StatRow : TableStatistics::StatColumn,
                      targets);
              if (!s) return jsonError("failed");
              s->showNormal();
              // Notify ArkTS that the table list changed (new statistics table created)
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              QJsonObject p;
              p["title"] = QObject::tr("Statistics");
              p["text"] = QObject::tr("Created %1").arg(s->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

        // ── 2D plotting (Phase 2) ──────────────────────────────────────
        // No-dialog core of the desktop Plot menu: multilayerPlot(table,
        // colList, style, ...) never pops a dialog.  Validation that the
        // desktop does via QMessageBox is re-done here with message events.
        { "plot2D",
          [](const QJsonObject &args) -> std::string {
              static const std::map<std::string, int> styles = {
                  { "line", Graph::Line },
                  { "scatter", Graph::Scatter },
                  { "line_symbol", Graph::LineSymbols },
                  { "vertical_bars", Graph::VerticalBars },
                  { "horizontal_bars", Graph::HorizontalBars },
                  { "area", Graph::Area },
                  { "pie", Graph::Pie },
                  { "drop_lines", Graph::VerticalDropLines },
                  { "spline", Graph::Spline },
                  { "vertical_steps", Graph::VerticalSteps },
                  { "horizontal_steps", Graph::HorizontalSteps },
                  { "histogram", Graph::Histogram },
                  { "box", Graph::Box },
                  { "vect_xyxy", Graph::VectXYXY },
                  { "vect_xyam", Graph::VectXYAM },
              };
              auto emitPlotError = [](const QString &text) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Plot error");
                  p["text"] = text;
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              };
              Table *t = resolveTable(args);
              if (!t) {
                  emitPlotError(QObject::tr("No table to plot from"));
                  return jsonError("table not found");
              }
              auto it = styles.find(args["type"].toString().toStdString());
              if (it == styles.end()) return jsonError("unknown plot type");
              const int style = it->second;

              // Column list: explicit indices from the ArkTS dialog, else
              // the current Qt selection, else every Y column.
              QStringList colList;
              if (args["cols"].isArray()) {
                  for (const QJsonValue &v : args["cols"].toArray()) {
                      int idx = v.toInt(-1);
                      if (idx >= 0 && idx < t->numCols())
                          colList << t->colName(idx);
                  }
              }
              if (colList.isEmpty())
                  colList = t->selectedColumns();
              if (colList.isEmpty()) {
                  for (int i = 0; i < t->numCols(); i++)
                      if (t->colPlotDesignation(i) == SciDAVis::Y)
                          colList << t->colName(i);
              }

              // Win parity: the desktop plot path feeds insertCurvesList with
              // drawableColumnSelection() (Y / xErr / yErr columns only).  An X
              // column in colList can never be plotted as a curve — colX() returns
              // -1 for it (a column cannot be its own X), loadData gets an empty X
              // name and the whole plot fails (device hilog 2026-08-12).  Filter
              // every source the same way; error columns stay so insertCurvesList
              // can attach them as error bars.
              // Vector plots (VectXYXY/VectXYAM) are the exception: they need
              // exactly four columns (X,Y,X2,Y2 resp. X,Y,A,M) and the X column
              // has PlotDesignation X — the drawable filter would strip it.  Keep
              // the raw selection for vector styles and enforce the count like
              // ApplicationWindow::plotVectXYXY/plotVectXYAM.
              const bool isVectorStyle = (style == Graph::VectXYXY || style == Graph::VectXYAM);
              QStringList drawable;
              if (!isVectorStyle) {
                  for (const QString &n : colList) {
                      const int idx = t->colIndex(n);
                      if (idx < 0)
                          continue;
                      const SciDAVis::PlotDesignation d = t->colPlotDesignation(idx);
                      if (d == SciDAVis::Y || d == SciDAVis::xErr || d == SciDAVis::yErr)
                          drawable << n;
                  }
                  colList = drawable;
              }

              // Diagnostic (device bring-up): column resolution outcome, also
              // logged when colList is empty so the fallback path is visible
              // in hilog. Column names only; no cell data is printed.
              OHOS_LOG("plot2D type=%s style=%d colList=%s",
                       qPrintable(args["type"].toString()), style,
                       qPrintable(colList.join(',')));
              if (colList.isEmpty())
                  OHOS_LOG("plot2D: no columns for table %s", qPrintable(t->name()));
              else
                  OHOS_LOG("plot2D: plotting %d curves from table %s",
                           colList.count(), qPrintable(t->name()));

              // Diagnostic (2026-08-12): column designations vs colX pairing.  The Y
              // fallback and colX/noXColumn read designation sources that may disagree
              // (legacy wrapper vs future table); print both so the mismatch is visible.
              QStringList desigs;
              for (int i = 0; i < t->numCols(); i++)
                  desigs << QString("%1:%2").arg(t->colName(i)).arg(int(t->colPlotDesignation(i)));
              QStringList xcols;
              for (int i = 0; i < t->numCols(); i++)
                  xcols << QString::number(t->colX(i));
              OHOS_LOG("plot2D desigs=[%s] noX=%d colX=[%s]",
                       qPrintable(desigs.join(',')), t->noXColumn() ? 1 : 0,
                       qPrintable(xcols.join(',')));

              // Desktop validFor2DPlot()/plotPie() minus the QMessageBox popups.
              if (style == Graph::Pie) {
                  if (colList.count() != 1) {
                      emitPlotError(QObject::tr("Select exactly one column for a pie plot"));
                      return jsonError("pie needs 1 column");
                  }
                  if (t->noXColumn()) {
                      emitPlotError(QObject::tr("Please set a default X column for this table, first!"));
                      return jsonError("no X column");
                  }
              } else if (style != Graph::Histogram && style != Graph::Box) {
                  if (t->numCols() < 2) {
                      emitPlotError(QObject::tr("You need at least two columns for this operation!"));
                      return jsonError("need 2 columns");
                  }
                  if (t->noXColumn()) {
                      emitPlotError(QObject::tr("Please set a default X column for this table, first!"));
                      return jsonError("no X column");
                  }
              }
              if (isVectorStyle && colList.count() != 4) {
                  emitPlotError(QObject::tr("Please select four columns for this operation!"));
                  return jsonError("vect needs 4 columns");
              }
              if (colList.isEmpty()) {
                  emitPlotError(QObject::tr("Please select a column to plot!"));
                  return jsonError("no columns");
              }

              int startRow = args["startRow"].toInt(0);
              int endRow = args["endRow"].toInt(t->numRows() - 1);
              auto *ml = g_mainWindow->multilayerPlot(t, colList, style, startRow, endRow);
              if (!ml) return jsonError("plot failed");
              QJsonObject p;
              p["title"] = QObject::tr("Plot");
              p["text"] = QObject::tr("Created plot from %1").arg(colList.join(", "));
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh plot list
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        // ── Panel-menu plotting (stacked histogram / panels) ─────────
        // plotStackedHistogram mirrors ApplicationWindow::plotStackedHistograms()
        // (multilayerPlot(1, -1, Graph::Histogram)); plotPanel mirrors the panel
        // menu slots plot2VerticalLayers/plot2HorizontalLayers/plot4Layers/
        // plotStackedLayers, which map to multilayerPlot(1,2)/(2,1)/(2,2)/(1,-1)
        // with the desktop defaultCurveStyle (= Graph::Line).  Unlike the plot2D
        // overload, multilayerPlot(int,int,int) operates on the ACTIVE MDI
        // subwindow and the table's current selection, so the resolved table is
        // activated first (no-op when it is already the active subwindow).
        { "plotStackedHistogram",
          [](const QJsonObject &args) -> std::string {
              auto emitPlotError = [](const QString &text) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Plot error");
                  p["text"] = text;
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              };
              Table *t = resolveTable(args);
              if (!t) {
                  emitPlotError(QObject::tr("No table to plot from"));
                  return jsonError("table not found");
              }
              // With r = -1 the overload stacks the table's selected columns
              // (selectedColumns() for the Histogram style), so ensure the
              // table has at least one column before handing it over.
              if (t->numCols() < 1) {
                  emitPlotError(QObject::tr("Table %1 has no columns to plot").arg(t->name()));
                  return jsonError("no columns");
              }
              g_mainWindow->activateSubWindow(t);
              MultiLayer *ml = g_mainWindow->multilayerPlot(1, -1, Graph::Histogram);
              if (!ml) return jsonError("plot failed");
              QJsonObject p;
              p["title"] = QObject::tr("Plot");
              p["text"] = QObject::tr("Created stacked histogram from %1").arg(t->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh plot list
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "plotPanel",
          [](const QJsonObject &args) -> std::string {
              auto emitPlotError = [](const QString &text) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Plot error");
                  p["text"] = text;
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              };
              Table *t = resolveTable(args);
              if (!t) {
                  emitPlotError(QObject::tr("No table to plot from"));
                  return jsonError("table not found");
              }
              const int cols = args["cols"].toInt(1);
              const int rows = args["rows"].toInt(1);
              if (cols < 1 || (rows < 1 && rows != -1)) {
                  emitPlotError(QObject::tr("Invalid panel size"));
                  return jsonError("invalid panel size");
              }
              if (t->numCols() < 1) {
                  emitPlotError(QObject::tr("Table %1 has no columns to plot").arg(t->name()));
                  return jsonError("no columns");
              }
              g_mainWindow->activateSubWindow(t);
              // Graph::Line is the desktop defaultCurveStyle the panel menu uses.
              MultiLayer *ml = g_mainWindow->multilayerPlot(cols, rows, Graph::Line);
              if (!ml) return jsonError("plot failed");
              QJsonObject p;
              p["title"] = QObject::tr("Plot");
              p["text"] = QObject::tr("Arranged panel from %1").arg(t->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh plot list
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        // ── Clipboard bridge (Phase 2) ──────────────────────────────
        // OHOS pasteboard ↔ QClipboard.  ArkTS pushes the system pasteboard
        // text in before dispatching a paste; Qt-side changes are emitted
        // through the clipboardChanged event (connected in main()).
        { "setClipboardText",
          [](const QJsonObject &args) -> std::string {
              QClipboard *cb = QApplication::clipboard();
              if (!cb) return jsonError("no clipboard");
              cb->setText(args["text"].toString());
              return "{\"success\":true}";
          } },

        { "getClipboardText",
          [](const QJsonObject &) -> std::string {
              QClipboard *cb = QApplication::clipboard();
              QJsonObject r;
              r["success"] = cb != nullptr;
              r["text"] = cb ? cb->text() : QString();
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        // ── Matrix operations (Phase 3) ────────────────────────────
        // All Matrix slots are dialog-free; invert()'s not-square warning
        // goes through the QMessageBox interposer (ohos_bridge).
        { "matrixSetDimensions",
          [](const QJsonObject &args) -> std::string {
              Matrix *m = resolveMatrix(args);
              if (!m) return jsonError("matrix not found");
              int rows = qBound(1, args["rows"].toInt(m->numRows()), 100000);
              int cols = qBound(1, args["cols"].toInt(m->numCols()), 10000);
              m->setDimensions(rows, cols);
              QJsonObject p;
              p["title"] = QObject::tr("Matrix");
              p["text"] = QObject::tr("%1 resized to %2 x %3").arg(m->name()).arg(rows).arg(cols);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

        { "matrixSetFormula",
          [](const QJsonObject &args) -> std::string {
              // Matrix::recalculate only fills selected cells, so select
              // everything first (ArkTS has no cell selection to offer).
              Matrix *m = resolveMatrix(args);
              if (!m) return jsonError("matrix not found");
              QString formula = args["formula"].toString();
              if (formula.isEmpty()) return jsonError("empty formula");
              m->setFormula(formula);
              if (auto *view = qobject_cast<MatrixView *>(m->view()))
                  view->selectAll();
              bool ok = m->recalculate();
              QJsonObject p;
              p["title"] = QObject::tr("Set Matrix Values");
              p["text"] = ok ? m->name() + " = " + formula
                             : QObject::tr("Formula error: ") + formula;
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return ok ? std::string("{\"success\":true}") : jsonError("recalculate failed");
          } },

        { "matrixTranspose",
          [](const QJsonObject &args) -> std::string {
              Matrix *m = resolveMatrix(args);
              if (!m) return jsonError("matrix not found");
              m->transpose();
              QJsonObject p;
              p["title"] = QObject::tr("Matrix");
              p["text"] = QObject::tr("Transposed %1").arg(m->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

        { "matrixInvert",
          [](const QJsonObject &args) -> std::string {
              Matrix *m = resolveMatrix(args);
              if (!m) return jsonError("matrix not found");
              if (m->numRows() != m->numCols()) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Error");
                  p["text"] = QObject::tr("Inversion failed, the matrix is not square!");
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
                  return jsonError("not square");
              }
              m->invert();
              QJsonObject p;
              p["title"] = QObject::tr("Matrix");
              p["text"] = QObject::tr("Inverted %1").arg(m->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

        // ── Graph image export (Phase 3) ──────────────────────────
        // Renders the MultiLayer to a sandbox PNG; the imageExported event
        // triggers the ArkTS picker copy-out (same flow as exportASCII).
        { "exportGraphImage",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QString id = args["plotId"].toString();
              MultiLayer *ml = nullptr;
              if (!id.isEmpty()) {
                  for (QMdiSubWindow *w : g_mainWindow->d_workspace.subWindowList()) {
                      auto *cand = qobject_cast<MultiLayer *>(w);
                      if (cand && cand->name() == id) { ml = cand; break; }
                  }
              } else {
                  ml = qobject_cast<MultiLayer *>(g_mainWindow->d_workspace.activeSubWindow());
              }
              if (!ml) return jsonError("plot not found");
              QString fn = args["path"].toString();
              if (fn.isEmpty()) return jsonError("no path");
              ml->exportImage(fn, args["quality"].toInt(-1));
              bool ok = QFile::exists(fn);
              QJsonObject done; done["ok"] = ok; done["path"] = fn;
              scidavisEmitEvent(QStringLiteral("imageExported"), done);
              if (!ok) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Export Image");
                  p["text"] = QObject::tr("Failed to export: ") + QFileInfo(fn).fileName();
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              }
              return ok ? std::string("{\"success\":true}") : jsonError("export failed");
          } },

        // ── Analysis suite (Phase 3) ───────────────────────────────
        // One command drives every curve analysis of the desktop Analysis
        // menu; parameters come from ArkTS dialogs.  Whatever the filter
        // appends to the results log is re-emitted as a resultsLog event.
        { "getGraphCurves",
          [](const QJsonObject &args) -> std::string {
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              QJsonArray arr;
              for (const QString &c : g->analysableCurvesList())
                  arr.append(c);
              QJsonObject r;
              r["success"] = true;
              r["curves"] = arr;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "get_curve_contents",
          [](const QJsonObject &args) -> std::string {
              // Read-only: list every plot item on the active graph (curves,
              // error bars, function curves, spectrograms) together with the
              // SciDAVis style id that add_curve consumes.  plotItemsList()
              // is index-aligned with Graph::curveType() (both walk the
              // c_keys/c_type vectors in insertion order, and removeCurve()
              // shifts both together), so each name gets its type directly.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QStringList names = g->plotItemsList();
              static const std::map<int, QString> typeNames = {
                  { Graph::Line, QStringLiteral("line") },
                  { Graph::Scatter, QStringLiteral("scatter") },
                  { Graph::LineSymbols, QStringLiteral("line_symbol") },
                  { Graph::VerticalBars, QStringLiteral("vertical_bars") },
                  { Graph::HorizontalBars, QStringLiteral("horizontal_bars") },
                  { Graph::Area, QStringLiteral("area") },
                  { Graph::Pie, QStringLiteral("pie") },
                  { Graph::VerticalDropLines, QStringLiteral("drop_lines") },
                  { Graph::Spline, QStringLiteral("spline") },
                  { Graph::VerticalSteps, QStringLiteral("vertical_steps") },
                  { Graph::HorizontalSteps, QStringLiteral("horizontal_steps") },
                  { Graph::Histogram, QStringLiteral("histogram") },
                  { Graph::Box, QStringLiteral("box") },
                  { Graph::VectXYXY, QStringLiteral("vector_xyxy") },
                  { Graph::VectXYAM, QStringLiteral("vector_xyam") },
                  { Graph::ErrorBars, QStringLiteral("errorBars") },
                  { Graph::Function, QStringLiteral("function") },
                  { Graph::ColorMap, QStringLiteral("spectrogram") },
                  { Graph::GrayMap, QStringLiteral("spectrogram") },
                  { Graph::ContourMap, QStringLiteral("spectrogram") },
              };
              QJsonArray arr;
              for (int i = 0; i < names.count(); i++) {
                  QJsonObject o;
                  o["name"] = names[i];
                  int t = g->curveType(i);
                  auto it = typeNames.find(t);
                  o["type"] = it != typeNames.end() ? it->second : QStringLiteral("unknown");
                  arr.append(o);
              }
              QJsonObject root;
              root["success"] = true;
              root["curves"] = arr;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "analyzeCurve",
          [](const QJsonObject &args) -> std::string {
              auto emitErr = [](const QString &text) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Analysis");
                  p["text"] = text;
                  p["icon"] = QStringLiteral("critical");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              };
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g || !g->validCurvesDataSize()) {
                  emitErr(QObject::tr("No plot with analysable curves is active"));
                  return jsonError("no graph");
              }
              QStringList curves = g->analysableCurvesList();
              if (curves.isEmpty()) {
                  emitErr(QObject::tr("The active plot has no analysable curves"));
                  return jsonError("no curves");
              }
              // Default to the most recently added curve (the desktop pops
              // a DataSetDialog instead, which the QPA cannot show).
              QString curve = args["curve"].toString();
              if (curve.isEmpty())
                  curve = curves.last();
              else if (!curves.contains(curve)) {
                  emitErr(QObject::tr("Unknown curve: ") + curve);
                  return jsonError("curve not found");
              }

              const QString op = args["op"].toString();
              const int logBefore = g_mainWindow->logInfo.length();
              bool ok = true;
              if (op == "fit_linear" || op == "fit_sigmoidal" || op == "fit_gauss"
                  || op == "fit_lorentz") {
                  static const std::map<std::string, QString> whichFit = {
                      { "fit_linear", QStringLiteral("fitLinear") },
                      { "fit_sigmoidal", QStringLiteral("fitSigmoidal") },
                      { "fit_gauss", QStringLiteral("fitGauss") },
                      { "fit_lorentz", QStringLiteral("fitLorentz") },
                  };
                  g_mainWindow->analyzeCurve(g, whichFit.at(op.toStdString()), curve);
              } else if (op == "fit_poly") {
                  int order = qBound(2, args["order"].toInt(2), 9);
                  auto *fit = new PolynomialFit(g_mainWindow, g, order, true);
                  if ((ok = fit->setDataFromCurve(curve))) {
                      fit->scaleErrors(g_mainWindow->fit_scale_errors);
                      fit->setOutputPrecision(g_mainWindow->fit_output_precision);
                      fit->generateFunction(g_mainWindow->generateUniformFitPoints,
                                            g_mainWindow->fitPoints);
                      fit->fit();
                  }
                  delete fit;
              } else if (op == "differentiate") {
                  auto *d = new Differentiation(g_mainWindow, g, curve);
                  d->run();
                  delete d;
              } else if (op == "integrate") {
                  auto *i = new Integration(g_mainWindow, g, curve);
                  i->run();
                  delete i;
              } else if (op == "fft") {
                  auto *f = new FFT(g_mainWindow, g, curve);
                  f->run();
                  delete f;
              } else if (op == "smooth") {
                  // method: 1 = Savitzky-Golay, 2 = FFT, 3 = moving average
                  int method = qBound(1, args["method"].toInt(3), 3);
                  auto *s = new SmoothFilter(g_mainWindow, g, curve, method);
                  s->setSmoothPoints(qBound(2, args["points"].toInt(5), 1000));
                  if (method == SmoothFilter::SavitzkyGolay)
                      s->setPolynomOrder(qBound(0, args["order"].toInt(2), 9));
                  ok = s->run();
                  delete s;
              } else if (op == "interpolate") {
                  // method: 0 = linear, 1 = cubic, 2 = Akima
                  int method = qBound(0, args["method"].toInt(0), 2);
                  auto *i = new Interpolation(g_mainWindow, g, curve, method);
                  i->setOutputPoints(qBound(3, args["points"].toInt(1000), 100000));
                  ok = i->run();
                  delete i;
              } else if (op == "fft_filter") {
                  // filterType: 1 low pass, 2 high pass, 3 band pass, 4 band block
                  int type = qBound(1, args["filterType"].toInt(1), 4);
                  auto *f = new FFTFilter(g_mainWindow, g, curve, type);
                  if (type <= FFTFilter::HighPass)
                      f->setCutoff(args["freq"].toDouble(0.0));
                  else
                      f->setBand(args["freq"].toDouble(0.0), args["freq2"].toDouble(0.0));
                  ok = f->run();
                  delete f;
              } else {
                  return jsonError("unknown op");
              }

              QString logDelta = g_mainWindow->logInfo.mid(logBefore).trimmed();
              QJsonObject p;
              p["title"] = QObject::tr("Analysis");
              p["text"] = ok ? op + QObject::tr(" done on ") + curve
                             : QObject::tr("%1 failed on %2").arg(op, curve);
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              if (!logDelta.isEmpty()) {
                  QJsonObject l;
                  l["text"] = logDelta;
                  scidavisEmitEvent(QStringLiteral("resultsLog"), l);
              }
              return ok ? std::string("{\"success\":true}") : jsonError("analysis failed");
          } },

        { "correlate",
          [](const QJsonObject &args) -> std::string {
              // (Auto)correlation of two table columns; the desktop takes
              // the two selected columns, ArkTS passes indices instead.
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              int c1 = args["col1"].toInt(-1);
              int c2 = args["col2"].toInt(c1);
              if (c1 < 0 || c1 >= t->numCols() || c2 < 0 || c2 >= t->numCols())
                  return jsonError("invalid columns");
              const int logBefore = g_mainWindow->logInfo.length();
              auto *cor = new Correlation(g_mainWindow, t, t->colName(c1), t->colName(c2));
              cor->run();
              delete cor;
              QString logDelta = g_mainWindow->logInfo.mid(logBefore).trimmed();
              QJsonObject p;
              p["title"] = QObject::tr("Correlation");
              p["text"] = QObject::tr("Correlated %1 and %2").arg(t->colName(c1), t->colName(c2));
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              if (!logDelta.isEmpty()) {
                  QJsonObject l;
                  l["text"] = logDelta;
                  scidavisEmitEvent(QStringLiteral("resultsLog"), l);
              }
              return "{\"success\":true}";
          } },

        // ArkTS picker result for a blocked QComboBox popup (ohos_bridge).
        { "selectCombo",
          [](const QJsonObject &args) -> std::string {
              if (!ohosBridgeSelectCombo(args["comboId"].toInt(-1), args["index"].toInt(-1)))
                  return jsonError("combo gone");
              return "{\"success\":true}";
          } },

        // ArkTS picker dismissed via scrim tap (no selection made): end the
        // combo-picker modal state so Qt input resumes (debounced).
        { "comboDismiss",
          [](const QJsonObject &) -> std::string {
              ohosBridgeDismissCombo();
              return "{\"success\":true}";
          } },

        // ArkTS selection for a blocked context QMenu (ohos_bridge).
        { "triggerMenu",
          [](const QJsonObject &args) -> std::string {
              if (!ohosBridgeTriggerMenu(args["menuId"].toInt(-1), args["path"].toString()))
                  return jsonError("menu gone");
              return "{\"success\":true}";
          } },

        { "activateWindow",
          [](const QJsonObject &args) -> std::string {
              // Windows menu: raise the MDI subwindow with this name.
              if (!g_mainWindow) return jsonError("no mw");
              QString name = args["name"].toString();
              for (MyWidget *w : g_mainWindow->windowsList()) {
                  if (w && w->name() == name) {
                      g_mainWindow->activateSubWindow(w);
                      return "{\"success\":true}";
                  }
              }
              return jsonError("window not found");
          } },

        { "setGraphTitle",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              g->setTitle(args["title"].toString());
              g->replot();
              return "{\"success\":true}";
          } },

        { "setAxisTitle",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int axis = args["axis"].toInt(0);
              if (axis < 0 || axis > 3) return jsonError("bad axis");
              g->setAxisTitle(axis, args["text"].toString());
              g->replot();
              return "{\"success\":true}";
          } },

        { "setAxisScale",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int axis = args["axis"].toInt(0);
              if (axis < 0 || axis > 3) return jsonError("bad axis");
              const QwtScaleDiv *scDiv = g->plotWidget()->axisScaleDiv(axis);
              double start = std::min(scDiv->lowerBound(), scDiv->upperBound());
              double end = std::max(scDiv->lowerBound(), scDiv->upperBound());
              // Honor explicit from/to bounds (AxesDialog sends {axis, scale,
              // from, to}); fall back to the current axis bounds when omitted
              // (GraphPropsDialog toggles the scale type only).
              if (args.contains("from"))
                  start = args["from"].toDouble(start);
              if (args.contains("to"))
                  end = args["to"].toDouble(end);
              if (start > end)
                  std::swap(start, end);
              int type = (args["scale"].toString() == "log") ? 1 : 0;
              if (type == 1 && start <= 0)
                  start = 1e-3;
              g->setScale(axis, start, end, 0.0, 5, 5, type, false);
              g->replot();
              return "{\"success\":true}";
          } },

        { "setAxisGrid",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int axis = args["axis"].toInt(0);
              if (axis < 0 || axis > 3) return jsonError("bad axis");
              Grid *grid = g->grid();
              if (!grid) return jsonError("no grid");
              // Same axis->grid mapping as getAxisConfig: yLeft/yRight drive
              // the Y grid lines, xBottom/xTop the X lines.
              const bool major = args.contains("majorGrid")
                      ? args["majorGrid"].toBool()
                      : (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
                              ? grid->yEnabled()
                              : grid->xEnabled();
              const bool minor = args.contains("minorGrid")
                      ? args["minorGrid"].toBool()
                      : (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
                              ? grid->yMinEnabled()
                              : grid->xMinEnabled();
              if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight) {
                  grid->enableY(major);
                  grid->enableYMin(minor);
              } else {
                  grid->enableX(major);
                  grid->enableXMin(minor);
              }
              // Optional grid pen styling (AxesDialog Grid tab): when the
              // dialog sends gridColor / gridLineStyle / gridLineWidth, rebuild
              // the major+minor pens of the X (or Y) grid lines targeted above.
              // gridLineStyle is a Qt::PenStyle int (0=NoPen..6=CustomDashLine).
              const int gridStyle = args.contains("gridLineStyle")
                      ? qBound(0, args["gridLineStyle"].toInt(1), 6) : -1;
              const int gridWidth = args["gridLineWidth"].toInt(-1);
              QColor gridColor;
              if (args.contains("gridColor") && !args["gridColor"].toString().isEmpty()) {
                  gridColor = QColor(args["gridColor"].toString());
                  if (!gridColor.isValid())
                      gridColor = QColor();
              }
              if (gridStyle >= 0 || gridWidth >= 0 || gridColor.isValid()) {
                  auto stylePens = [&](QPen &maj, QPen &min) {
                      if (gridStyle >= 0) {
                          maj.setStyle((Qt::PenStyle)gridStyle);
                          min.setStyle((Qt::PenStyle)gridStyle);
                      }
                      if (gridWidth >= 0) {
                          maj.setWidth(gridWidth);
                          min.setWidth(gridWidth);
                      }
                      if (gridColor.isValid()) {
                          maj.setColor(gridColor);
                          min.setColor(gridColor);
                      }
                  };
                  if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight) {
                      QPen majPen = grid->majPenY();
                      QPen minPen = grid->minPenY();
                      stylePens(majPen, minPen);
                      grid->setMajPenY(majPen);
                      grid->setMinPenY(minPen);
                  } else {
                      QPen majPen = grid->majPenX();
                      QPen minPen = grid->minPenX();
                      stylePens(majPen, minPen);
                      grid->setMajPenX(majPen);
                      grid->setMinPenX(minPen);
                  }
              }
              g->replot();
              QJsonObject p;
              p["title"] = QObject::tr("Axes");
              p["text"] = QObject::tr("Grid updated for axis %1").arg(axis);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "setAxisFrame",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const bool showAxis = args.contains("showAxis") ? args["showAxis"].toBool() : true;
              const bool backbones = args.contains("backbones") ? args["backbones"].toBool() : true;
              const int lineWidth = args["lineWidth"].toInt(1);
              // Major/minor tick lengths (AxesDialog Frame tab): applied to all
              // axes via Graph::changeTicksLength() — the same API the desktop
              // FrameWidget (AxesDialog::changeMajorTicksLength) uses.  Omitted
              // fields keep the current lengths (read via the Graph getters).
              if (args.contains("majorTicksLength") || args.contains("minorTicksLength")) {
                  const int minLen = qBound(0, args["minorTicksLength"].toInt(g->minorTickLength()),
                                            1000);
                  const int majLen = qBound(0, args["majorTicksLength"].toInt(g->majorTickLength()),
                                            1000);
                  g->changeTicksLength(minLen, majLen);
              }
              g->drawAxesBackbones(backbones);
              g->setAxesLinewidth(lineWidth);
              // AxesDialog's frame tab sends no per-axis key; apply the axis
              // on/off state to every axis in that case, otherwise only to the
              // requested one.
              if (args.contains("axis")) {
                  int axis = qBound(0, args["axis"].toInt(0), 3);
                  g->enableAxis(axis, showAxis);
              } else {
                  for (int a = QwtPlot::yLeft; a <= QwtPlot::xTop; a++)
                      g->enableAxis(a, showAxis);
              }
              g->replot();
              QJsonObject p;
              p["title"] = QObject::tr("Axes");
              p["text"] = QObject::tr("Frame updated");
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "toggleLegend",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              if (g->hasLegend())
                  g->removeLegend();
              else
                  (void)g->newLegend();
              g->replot();
              return "{\"success\":true}";
          } },

        { "toggleColumnEditor",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              auto *tableView = g_mainWindow->findChild<TableView *>();
              if (!tableView) return jsonError("not a table");
              if (tableView->isControlTabBarVisible())
                  tableView->toggleControlTabBar();
              return "{\"success\":true}";
          } },

        { "openProject",
          [](const QJsonObject &args) -> std::string {
              // ArkTS DocumentViewPicker copies the picked .sciprj into the
              // app sandbox and hands the path over (queued; result toast
              // arrives through the event channel).
              if (!g_mainWindow) return jsonError("no mw");
              QString fn = args["path"].toString();
              if (fn.isEmpty() || !QFile::exists(fn)) return jsonError("file not found");
              bool ok = g_mainWindow->loadProject(fn);
              QJsonObject p;
              p["title"] = QObject::tr("Open Project");
              p["text"] = ok ? QObject::tr("Opened: ") + QFileInfo(fn).fileName()
                             : QObject::tr("Failed to open: ") + QFileInfo(fn).fileName();
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              QJsonObject r;
              r["success"] = ok;
              r["path"] = fn;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "saveProject",
          [](const QJsonObject &args) -> std::string {
              // No-dialog replacement for saveProjectAs(): fix projectname
              // to a sandbox path first so saveProject() never falls back to
              // the QFileDialog branch (single-window QPA would SIGSEGV).
              // Queued command (saving calls back into the JS thread via the
              // QPA — a blocking query would deadlock); completion is
              // signalled by the projectSaved event, which also triggers the
              // ArkTS save-as copy-out to the picker target.
              if (!g_mainWindow) return jsonError("no mw");
              QString fn = args["path"].toString();
              if (fn.isEmpty()) return jsonError("no path");
              if (!fn.endsWith(".sciprj") && !fn.endsWith(".sciprj.gz"))
                  fn += ".sciprj";
              g_mainWindow->projectname = fn;
              g_mainWindow->saveProject();
              bool ok = QFile::exists(fn);
              QJsonObject done;
              done["ok"] = ok;
              done["path"] = fn;
              scidavisEmitEvent(QStringLiteral("projectSaved"), done);
              QJsonObject p;
              p["title"] = QObject::tr("Save Project");
              p["text"] = ok ? QObject::tr("Saved: ") + QFileInfo(fn).fileName()
                             : QObject::tr("Failed to save: ") + QFileInfo(fn).fileName();
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              QJsonObject r;
              r["success"] = ok;
              r["path"] = fn;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "isProjectModified",
          [](const QJsonObject &) -> std::string {
              // 2026-08-15: unsaved-changes flag for the ArkTS quit/close
              // confirmation (mirrors ApplicationWindow::saved).
              if (!g_mainWindow) return jsonError("no mw");
              QJsonObject r;
              r["success"] = true;
              r["modified"] = !g_mainWindow->saved;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "getRecentProjects",
          [](const QJsonObject &) -> std::string {
              // Stage-2A (2026-08-16): recent-projects list for the ArkTS
              // File→Recent Projects submenu.  The list itself is maintained
              // by libscidavis ApplicationWindow (loadProject/openProject
              // push_front + QSettings "/RecentProjects" persistence, max 10
              // via MaxRecentProjects); here we only expose it.  ArkTS drops
              // entries that are no longer accessible (grant expired/deleted).
              if (!g_mainWindow) return jsonError("no mw");
              QJsonArray arr;
              const QStringList &recent = g_mainWindow->recentProjects;
              for (const QString &p : recent)
                  arr.append(p);
              QJsonObject r;
              r["success"] = true;
              r["projects"] = arr;
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "menuAction",
          [](const QJsonObject &args) -> std::string {
              // Fallback route for ArkTS menu items without a dedicated
              // command.  Runs queued on the Qt thread, so ApplicationWindow
              // methods are called directly on the right thread.  Dialog
              // actions are fine for whitelisted classes: the ohos_bridge
              // interposer embeds them in the main window (see
              // kEmbedDialogWhitelist); every non-whitelisted popup stays
              // blocked (dialogBlocked event).
              if (!g_mainWindow) return jsonError("no mw");
              QString itemId = args["itemId"].toString();
              bool ok = true;

              // Phase-5 fix 2026-08-13: the direct analysis branches
              // (an_fit_linear/an_fit_sigmoidal/an_fit_gauss/an_fit_lorentz/
              // an_differentiate) run ApplicationWindow analysis slots that
              // append their results into logInfo, whose QDockWidget
              // (logWindow) is hidden on OH at startup, so the results never
              // surface.  Relay the logInfo delta to ArkTS as a resultsLog
              // event — same payload shape ({text}) as the analyzeCurve and
              // correlate commands.
              auto emitResultsLogDelta = [](const int before) {
                  const QString logDelta = g_mainWindow->logInfo.mid(before).trimmed();
                  if (logDelta.isEmpty())
                      return;
                  QJsonObject l;
                  l["text"] = logDelta;
                  scidavisEmitEvent(QStringLiteral("resultsLog"), l);
              };
              // Phase-5 instrumentation 2026-08-13 (diagnosis aid, kept):
              // curve-resolution chain probe shared by the direct analysis
              // branches — hilog tag [ANALYSIS].  Returns logInfo length
              // before the call so the caller can relay/measure the delta.
              auto analysisProbe = [](const char *id) -> int {
                  QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
                  MultiLayer *ml = qobject_cast<MultiLayer *>(sub);
                  Graph *g = ml ? ml->activeGraph() : nullptr;
                  const QStringList curves = g ? g->analysableCurvesList() : QStringList();
                  qWarning("[ANALYSIS] %s before: activeSubWindow=%s multiLayer=%d "
                           "activeGraph=%p validData=%d curves=%d selectedCurveID=%d "
                           "selectedCurveTitle='%s' lastAnalysable='%s' logInfoLen=%d",
                           id,
                           sub ? (sub->widget() ? sub->widget()->metaObject()->className()
                                                : "(no widget)")
                               : "(none)",
                           ml ? 1 : 0,
                           static_cast<void *>(g),
                           g ? g->validCurvesDataSize() : 0,
                           curves.size(),
                           g ? g->selectedCurveID() : -1,
                           g ? qPrintable(g->selectedCurveTitle()) : "(n/a)",
                           curves.isEmpty() ? "(none)" : qPrintable(curves.last()),
                           g_mainWindow->logInfo.length());
                  return g_mainWindow->logInfo.length();
              };
               if (itemId == "new_notes") {
                   Note *note = g_mainWindow->newNote();
                   if (note) {
                       // Defer the display to the next event-loop tick: a
                       // synchronous showNormal()+setFocus() blocks the Qt GUI
                       // thread (QMdiArea activation chain + first-frame
                       // render) until the whole menuAction command finishes,
                       // which the UI perceives as a freeze before the window
                       // appears.  QTimer::singleShot(0,...) lets the loop
                       // flush other events first.  Note* is owned by
                       // ApplicationWindow, so the bare pointer stays valid
                       // across one event-loop turn (null-checked anyway).
                       QTimer::singleShot(0, [note]() {
                           if (note) {
                               note->showNormal();
                               note->setFocus();
                           }
                       });
                   }
               }
              else if (itemId == "new_matrix") {
                  // newMatrix() only registers the matrix in the project tree
                  // (d_project->addChild) — it never adds the widget to the MDI
                  // workspace nor shows it, so the matrix is created but stays
                  // invisible.  Mirror the display steps of initMatrix() here.
                  Matrix *m = g_mainWindow->newMatrix(32, 32);
                  if (m) {
                      // Registration steps stay synchronous (must complete in
                      // this command)…
                      // OHOS: give the matrix an explicit 1080x650 geometry the
                      // same way initTable/initMatrix do.  An explicit
                      // setGeometry (sets Qt::WA_Resized) makes
                      // QMdiArea::appendChild skip its min(sizeHint, viewport)
                      // auto-size, so the matrix opens shrunk instead of
                      // filling the viewport.  (Spec: matrix 1080x650,
                      // 2026-08-13 issue list #5.)
                      QMdiSubWindow *sw = g_mainWindow->d_workspace.addSubWindow(m);
                      const QPoint placedPos = sw->pos();
                      sw->setWindowState(Qt::WindowNoState);
                      sw->setGeometry(placedPos.x(), placedPos.y(), 960, 650);
                      // newMatrix() already generates and sets a unique name.
                      g_mainWindow->currentFolder()->addWindow(m);
                      g_mainWindow->addListViewItem(m);
                      g_mainWindow->modifiedProject(m);
                      // …but the display is deferred to the next event-loop
                      // tick: synchronous showNormal()/setFocus() blocks the
                      // GUI thread (MDI activation + first-frame render) until
                      // the command finishes.  Deferring lets the loop flush
                      // first, so the window appears without the perceived
                      // freeze.  Matrix* is owned by ApplicationWindow, and the
                      // QMdiSubWindow is owned by the MDI area, so the bare
                      // pointers stay valid (null-checked anyway).
                      QTimer::singleShot(0, [sw, placedPos, m]() {
                          if (m && sw) {
                              // showNormal() triggers the MDI activation chain,
                              // which can re-maximize the fresh subwindow when
                              // the previously active window is maximized, so
                              // force the Normal geometry once more after
                              // showNormal() — as the last writer it wins — and
                              // the final state is the shrunk 960x650 matrix
                              // (spec: matrix 960x650, 2026-08-15 user
                              // request: default restored size 960x650).
                              sw->setWindowState(Qt::WindowNoState);
                              sw->setGeometry(placedPos.x(), placedPos.y(), 960, 650);
                              sw->showNormal();
                              sw->setWindowState(Qt::WindowNoState);
                              sw->setGeometry(placedPos.x(), placedPos.y(), 960, 650);
                              m->setFocus();
                          }
                      });
                  }
              }
              else if (itemId == "new_project") {
                  // Cannot call newProject() — it creates a second
                  // ApplicationWindow which SIGSEGVs on the single-window QPA.
                  // Instead, clear the existing project in-place.
                  // 2026-08-15: Win-parity unsaved-changes confirmation
                  // (same Yes/No/Cancel + save semantics as closeEvent).
                  // 2026-08-17: Yes 不再在这里直接 saveProject —— C++ 不能弹
                  // ArkTS 授权，改为先发 requestSaveBeforeNewProject，由
                  // ArkTS 打开带权限申请的 Qt 保存对话框；保存成功后由
                  // new_project_after_save 执行真正清空。
                  if (!g_mainWindow->saved) {
                      const QString s = QObject::tr("Save changes to project: <p><b> %1 </b> ?")
                                            .arg(g_mainWindow->projectname);
                      const int r = QMessageBox::information(
                          g_mainWindow, QObject::tr("SciDAVis"), s,
                          QObject::tr("Yes"), QObject::tr("No"), QObject::tr("Cancel"), 0, 2);
                      if (r == 0) {
                          QJsonObject req;
                          req["reason"] = QStringLiteral("new_project");
                          scidavisEmitEvent(QStringLiteral("requestSaveBeforeNewProject"), req);
                          return "{\"success\":true,\"pendingSave\":true}";
                      } else if (r != 1) {
                          return "{\"success\":false,\"error\":\"cancelled\"}"; // Cancel
                      }
                  }
                  for (MyWidget *w : g_mainWindow->windowsList()) {
                      w->askOnCloseEvent(false);
                      w->close();
                  }
                  g_mainWindow->projectname = "untitled";
                  g_mainWindow->newTable();
                  g_mainWindow->savedProject();
                  QJsonObject p;
                  p["title"] = QObject::tr("New Project");
                  p["text"] = QObject::tr("Project cleared");
                  p["icon"] = QStringLiteral("information");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              }
              else if (itemId == "new_project_after_save") {
                  // Continuation after ArkTS save-as (with permission) from
                  // requestSaveBeforeNewProject.  No prompt: the user already
                  // answered Yes and saved.
                  for (MyWidget *w : g_mainWindow->windowsList()) {
                      w->askOnCloseEvent(false);
                      w->close();
                  }
                  g_mainWindow->projectname = "untitled";
                  g_mainWindow->newTable();
                  g_mainWindow->savedProject();
                  QJsonObject p;
                  p["title"] = QObject::tr("New Project");
                  p["text"] = QObject::tr("Project cleared");
                  p["icon"] = QStringLiteral("information");
                  scidavisEmitEvent(QStringLiteral("message"), p);
              }
              else if (itemId == "quit") {
                  // 2026-08-17: 退出确认也走 Qt 原生 QMessageBox（与
                  // New Project 同款标题/Yes/No/Cancel），不再弹 ArkTS 窗口。
                  // Yes 且未命名时同样先通知 ArkTS 申请权限再打开保存对话框。
                  if (!g_mainWindow->saved) {
                      const QString s = QObject::tr("Save changes to project: <p><b> %1 </b> ?")
                                            .arg(g_mainWindow->projectname);
                      const int r = QMessageBox::information(
                          g_mainWindow, QObject::tr("SciDAVis"), s,
                          QObject::tr("Yes"), QObject::tr("No"), QObject::tr("Cancel"), 0, 2);
                      if (r == 0) {
                          const QString pn = g_mainWindow->projectname;
                          const bool needsSaveAs = pn.isEmpty()
                              || pn == QStringLiteral("untitled")
                              || pn.endsWith(".opj", Qt::CaseInsensitive)
                              || pn.endsWith(".ogm", Qt::CaseInsensitive)
                              || pn.endsWith(".ogw", Qt::CaseInsensitive)
                              || pn.endsWith(".ogg", Qt::CaseInsensitive)
                              || pn.endsWith(".org", Qt::CaseInsensitive);
                          if (needsSaveAs) {
                              QJsonObject req;
                              req["reason"] = QStringLiteral("quit");
                              scidavisEmitEvent(QStringLiteral("requestSaveBeforeQuit"), req);
                              return "{\"success\":true,\"pendingSave\":true}";
                          }
                          if (!g_mainWindow->saveProject())
                              return "{\"success\":false,\"error\":\"save failed\"}";
                          scidavisEmitEvent(QStringLiteral("quitRequested"));
                          return "{\"success\":true}";
                      } else if (r == 1) {
                          scidavisEmitEvent(QStringLiteral("quitRequested"));
                          return "{\"success\":true}";
                      }
                      return "{\"success\":false,\"error\":\"cancelled\"}"; // Cancel
                  }
                  scidavisEmitEvent(QStringLiteral("quitRequested"));
                  return "{\"success\":true}";
              }
               else if (itemId == "new_graph") {
                   // newGraph() → multilayerPlot() → initMultilayerPlot() now
                   // shows the subwindow in Normal state (initMultilayerPlot
                   // uses showNormal(), not show()) so a new graph opens
                   // already small — no "fullscreen then shrink" flash.
                   // MultiLayer IS the QMdiSubWindow (MultiLayer → MyWidget →
                   // QMdiSubWindow), so its pos()/setGeometry()/setWindowState()
                   // act on the real subwindow managed by QMdiArea.
                   // OHOS-FIX: all placement (capture the cascade position,
                   // force the Normal 900x650 geometry) now happens inside
                   // newGraph() around its single showNormal().  No deferred
                   // re-show is needed here — a second showNormal() would
                   // re-trigger the MDI activation chain, which can
                   // move/resize the fresh subwindow (the "new graph flashes
                   // then jumps" bug).
                   MultiLayer *ml = g_mainWindow->newGraph();
                   if (ml) {
                       // newGraph() already displayed the graph at the cascade
                       // position; nothing further to do.
                   }
               }
              else if (itemId == "cascade")
                  g_mainWindow->cascade();
              else if (itemId == "maximize_window")
                  g_mainWindow->maximizeWindow();
              else if (itemId == "minimize_window")
                  g_mainWindow->minimizeWindow();
              else if (itemId == "restore_window")
                  g_mainWindow->restoreWindow();
              else if (itemId == "close_window") {
                  // OHOS (2026-08-13): the close-confirm dialog is embedded
                  // and rendered by the bridge now, so the Windows→Close
                  // Window menu path runs the normal closeEvent confirm
                  // flow (desktop parity) — no suppression needed.
                  g_mainWindow->closeActiveWindow();
              }
              // D2 (2026-08-14): Windows menu补齐 — tile/next/previous/
              // duplicate/hide_window, per Win ApplicationWindow.cpp
              // L7552-7562 (windowsMenuAboutToShow).
              else if (itemId == "tile")
                  g_mainWindow->d_workspace.tileSubWindows();
              else if (itemId == "next")
                  g_mainWindow->d_workspace.activateNextSubWindow();
              else if (itemId == "previous")
                  g_mainWindow->d_workspace.activatePreviousSubWindow();
              else if (itemId == "duplicate")
                  // Win: actionCopyWindow → ApplicationWindow::clone()
                  // (L10536-10537, "Duplicate window").
                  g_mainWindow->clone();
              else if (itemId == "hide_window")
                  // Win: "&Hide Window" → hideActiveWindow() (L7562).
                  g_mainWindow->hideActiveWindow();
              else if (itemId == "about") {
                  // Phase-7 fix 2026-08-13 (OHOS embedded About dialog): the
                  // dialog is constructed inside libscidavis
                  // (SciDAVis::aboutDialog, globals.cpp L132ff) so that the
                  // AUTOUIC-generated ui_SciDAVisAbout.h stays in the
                  // libscidavis target — main.cpp no longer includes it
                  // (cross-target AUTOUIC is not supported).  aboutDialog
                  // returns a new dialog (caller owns it): embed it and
                  // delete it here.
                  static bool aboutOpen = false;
                  if (!aboutOpen) {
                      aboutOpen = true;
                      QDialog *dlg = SciDAVis::aboutDialog(g_mainWindow);
                      if (dlg) {
                          embedDialog(dlg);
                          delete dlg;
                      }
                      aboutOpen = false;
                  }
              }
              else if (itemId == "undo")
                  g_mainWindow->undo();
              else if (itemId == "redo")
                  g_mainWindow->redo();
              else if (itemId == "undo_redo_history") {
                  // Phase 7 (2026-08-13): Undo/Redo History.  Win's
                  // showHistory() (ApplicationWindow.cpp L13844) blocks in
                  // QDialog::exec(); on the single-window QPA the dialog must
                  // instead be embedded in the main window.  QUndoView is not
                  // whitelisted, so embedDialog() is called directly here
                  // (declared above; same library as ohos_bridge.cpp) — no
                  // whitelist entry needed.  d_project is private, so the
                  // stack comes through the new SCIDAVIS_OHOS accessor
                  // ApplicationWindow::ohosUndoStack().
                  static bool historyOpen = false;
                  if (!historyOpen) {
                      historyOpen = true;
                      QUndoStack *stack = g_mainWindow->ohosUndoStack();
                      if (stack) {
                          QDialog dlg(g_mainWindow, Qt::Widget);
                          dlg.setWindowTitle(QObject::tr("Undo/Redo History"));
                          QUndoView *view = new QUndoView(stack, &dlg);
                          QVBoxLayout *lay = new QVBoxLayout(&dlg);
                          lay->addWidget(view);
                          embedDialog(&dlg);
                      }
                      historyOpen = false;
                  }
              }
              else if (itemId == "scripting_language") {
                  // Phase-7 fix 2026-08-13 (ArkTS 界面 → Qt 原生内嵌对齐 Win):
                  // embed the real Qt ScriptingLangDialog (language list +
                  // OK/Cancel) instead of the ArkTS mimic — same pattern as
                  // undo_redo_history: static guard + heap dialog with
                  // WA_DeleteOnClose + embedDialog() direct embed (no
                  // whitelist needed).  The scripting env comes through the
                  // new SCIDAVIS_OHOS accessor ApplicationWindow::ohosScriptEnv()
                  // (wraps the protected scripted::scriptEnv member; on OHOS
                  // it is the MuParser engine).  The dialog's updateLangList()
                  // dereferences scriptEnv, so a null env falls back to a
                  // message instead.  ScriptingLangDialog::accept() ends in
                  // close() and WA_DeleteOnClose destroys the dialog inside
                  // the modal loop — covered by embedDialog()'s QPointer guard
                  // (ohos_bridge.cpp, 2026-08-13 fix).
                  static bool sldOpen = false;
                  if (!sldOpen) {
                      sldOpen = true;
                      ScriptingEnv *env = g_mainWindow->ohosScriptEnv();
                      // Phase-7 diag (2026-08-13): env null → ArkTS message
                      // fallback; env non-null → embed.  On-device hilog tag
                      // [P7DIALOG] shows whether the branch is reached and
                      // which path runs.
                      qWarning("[P7DIALOG] scripting_language: env=%p",
                               static_cast<void *>(env));
                      if (env) {
                          ScriptingLangDialog *d =
                                  new ScriptingLangDialog(env, g_mainWindow);
                          d->setAttribute(Qt::WA_DeleteOnClose);
                          embedDialog(d);
                      } else {
                          QJsonObject p;
                          p["title"] = QObject::tr("Scripting Language");
                          p["text"] = QObject::tr("Scripting engine unavailable");
                          p["icon"] = QStringLiteral("critical");
                          scidavisEmitEvent(QStringLiteral("message"), p);
                      }
                      sldOpen = false;
                  }
              }
              else if (itemId == "restart_scripting") {
                  // Win Scripting → Restart Scripting (actionRestartScripting →
                  // restartScriptingEnv(), ApplicationWindow.cpp L4114).
                  // Restarts the muParser scripting env; safe, no dialog.
                  static bool rse = false;
                  if (!rse) {
                      rse = true;
                      g_mainWindow->restartScriptingEnv();
                      rse = false;
                  }
              }
              else if (itemId == "note_execute" || itemId == "note_execute_all"
                       || itemId == "note_evaluate") {
                  // P3 (2026-08-16): Note 激活时 Scripting 菜单执行项
                  // （Win L1246-1248: connect(..., w, SLOT(execute()/
                  // executeAll()/evaluate()))）。Note::execute/executeAll/
                  // evaluate 均为 inline（Note.h L70-72，走 te-> 脚本执行）。
                  QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
                  Note *note = qobject_cast<Note *>(sub);
                  if (!note) {
                      OHOS_LOG("note scripting: no active Note (itemId=%s)",
                               qPrintable(itemId));
                  } else if (itemId == "note_execute") {
                      note->execute();
                  } else if (itemId == "note_execute_all") {
                      note->executeAll();
                  } else {
                      note->evaluate();
                  }
              }
              else if (itemId == "rename_window") {
                  // Phase-7 fix 2026-08-13 (ArkTS 界面 → Qt 原生内嵌对齐 Win):
                  // embed the real Qt RenameWindowDialog (Name/Label/Both
                  // radio group + the current window pre-filled) instead of
                  // the ArkTS RenameWindowDialog.  Mirrors
                  // ApplicationWindow::renameActiveWindow()
                  // (ApplicationWindow.cpp L5226-5236) with qobject_cast on
                  // the active subwindow; the static guard is taken only
                  // around the embedding (no modal loop runs when there is no
                  // active window, so the flag would otherwise stay stuck —
                  // the no-window case returns jsonError like the other
                  // guard branches).  RenameWindowDialog::accept() ends in
                  // close() and WA_DeleteOnClose destroys the dialog inside
                  // the modal loop — covered by embedDialog()'s QPointer
                  // guard (ohos_bridge.cpp, 2026-08-13 fix).
                  static bool rwdOpen = false;
                  if (!rwdOpen) {
                      QMdiSubWindow *activeSub = g_mainWindow->d_workspace.activeSubWindow();
                      MyWidget *m = qobject_cast<MyWidget *>(activeSub);
                      // Phase-7 diag (2026-08-13): logs the active subwindow
                      // class (MyWidget IS-A QMdiSubWindow, so the cast works
                      // for Table/Matrix/MultiLayer/Note/Graph3D) and the
                      // cast outcome.  cast=0 → jsonError below (the dialog
                      // is not opened by design, desktop parity).
                      qWarning("[P7DIALOG] rename_window: active=%s cast=%d",
                               activeSub ? qPrintable(activeSub->metaObject()->className())
                                         : "(none)",
                               m ? 1 : 0);
                      if (!m)
                          return jsonError("no active window");
                      rwdOpen = true;
                      RenameWindowDialog *rwd = new RenameWindowDialog(g_mainWindow);
                      rwd->setAttribute(Qt::WA_DeleteOnClose);
                      rwd->setWidget(m);
                      embedDialog(rwd);
                      rwdOpen = false;
                  }
              }
              else if (itemId == "cut")
                  g_mainWindow->cutSelection();
              else if (itemId == "copy")
                  g_mainWindow->copySelection();
              else if (itemId == "paste")
                  g_mainWindow->pasteSelection();
               else if (itemId == "delete")
                   g_mainWindow->clearSelection();
               else if (itemId == "clear_selection")
                   g_mainWindow->clearSelection();
               else if (itemId == "insert_row" || itemId == "insert_col") {
                   // 2026-08-14: these menu branches used to set ok=false
                   // (silent "unsupported"), while the scidavis_call
                   // registry had real implementations — the menu always
                   // reported "未适配".  Route to the same logic the
                   // insert_row/insert_col commands use.
                   if (g_mainWindow) {
                       QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
                       MyWidget *w = qobject_cast<MyWidget *>(sub);
                       if (w && w->inherits("Table")) {
                           Table *tbl = static_cast<Table *>(w);
                           if (itemId == "insert_row")
                               tbl->insertRow();
                           else
                               tbl->insertCol();
                           QJsonObject ep;
                           ep["tableId"] = w->name();
                           scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
                           scidavisEmitEvent(QStringLiteral("tableListChanged"));
                       } else {
                           ok = false;
                       }
                   } else {
                       ok = false;
                   }
               }
               else if (itemId == "preferences") {
                   // Qt-native Preferences dialog (ConfigDialog, a Qt::Dialog
                   // window) — the one dialog the ohos_bridge interposer now
                   // forwards to the real Qt implementation; not the ArkTS
                   // settings mimic.  exec() blocks until the dialog closes,
                   // but its nested modal loop still drains queued commands,
                   // so a static flag guards against opening a second copy.
                   static bool prefOpen = false;
                   if (!prefOpen) {
                       prefOpen = true;
                       g_mainWindow->showPreferencesDialog();
                       prefOpen = false;
                   }
               }
               else if (itemId == "add_curve") {
                   // Qt-native Curves dialog (CurvesDialog): non-modal show()
                   // on desktop, routed into embedDialog by the ohos_bridge
                   // setVisible interposer.  As with preferences, the embedded
                   // modal loop blocks until the dialog closes, so a static
                   // flag guards against opening a second copy (the nested
                   // loop would otherwise drain a re-triggered command).
                   static bool curvesOpen = false;
                   if (!curvesOpen) {
                       curvesOpen = true;
                       g_mainWindow->showCurvesDialog();
                       curvesOpen = false;
                   }
               }
               else if (itemId == "add_error_bars") {
                   // ErrDialog: exec()-opened, embedded via QDialog::exec()
                   // interposer.
                   static bool errBarsOpen = false;
                   if (!errBarsOpen) {
                       errBarsOpen = true;
                       g_mainWindow->addErrorBars();
                       errBarsOpen = false;
                   }
               }
               else if (itemId == "add_function") {
                   // Graph → Add Function: the full addFunctionCurve() flow
                   // (functionDialog() factory + setGraph on the returned
                   // dialog).  The FunctionDialog embed is deferred one
                   // event-loop tick so setGraph() runs before the dialog
                   // appears (see the QDialog::setVisible interposer).
                   static bool functionOpen = false;
                   if (!functionOpen) {
                       functionOpen = true;
                       g_mainWindow->addFunctionCurve();
                       functionOpen = false;
                   }
               }
               else if (itemId == "new_function_plot") {
                   // Win File → New → Function Plot = ApplicationWindow::
                   // functionDialog() (opens a bare FunctionDialog for a NEW
                   // function plot; no active graph required).  FunctionDialog
                   // is whitelisted → rendered embedded (add_function parity).
                   static bool nfpOpen = false;
                   if (!nfpOpen) {
                       nfpOpen = true;
                       g_mainWindow->functionDialog();
                       nfpOpen = false;
                   }
               }
               else if (itemId == "format_scales") {
                   // AxesDialog (Scales page by default): exec() override
                   // embeds via ohosEmbedDialog().
                   static bool scalesOpen = false;
                   if (!scalesOpen) {
                       scalesOpen = true;
                       g_mainWindow->showScaleDialog();
                       scalesOpen = false;
                   }
               }
               else if (itemId == "format_axes") {
                   // AxesDialog on the Axes page: showAxisDialog() switches
                   // the page AFTER showScaleDialog()'s exec() returns; the
                   // deferred embed in ohosEmbedDialog() keeps that order
                   // working on-device.
                   static bool axesOpen = false;
                   if (!axesOpen) {
                       axesOpen = true;
                       g_mainWindow->showAxisDialog();
                       axesOpen = false;
                   }
               }
               else if (itemId == "format_plot") {
                   // D3 (2026-08-14): Win Format→Plot = PlotDialog
                   // （showGeneralPlotDialog，ApplicationWindow.cpp L5792；
                   // PlotDialog 已在 kEmbedDialogWhitelist，show() 走
                   // setVisible 拦截内嵌）。ArkTS FormatDialog 随之退役。
                   static bool plotFmtOpen = false;
                   if (!plotFmtOpen) {
                       plotFmtOpen = true;
                       g_mainWindow->showGeneralPlotDialog();
                       plotFmtOpen = false;
                   }
               }
               else if (itemId == "format_grid") {
                   // AxesDialog on the Grid page (same deferred-embed order
                   // as format_axes).
                   static bool gridOpen = false;
                   if (!gridOpen) {
                       gridOpen = true;
                       g_mainWindow->showGridDialog();
                       gridOpen = false;
                   }
               }
               else if (itemId == "fit_wizard") {
                   // FitDialog: exec()-opened, embedded via QDialog::exec()
                   // interposer.
                   static bool fitOpen = false;
                   if (!fitOpen) {
                       fitOpen = true;
                       g_mainWindow->showFitDialog();
                       fitOpen = false;
                   }
               }
               // ── Table direct commands (batch A, 2026-08-12) ──────────
               // Table API confirmation (grep 2026-08-12): Table* resolves
               // to the legacy wrapper class in libscidavis/src/Table.h
               // ("class Table : public TableView"), which declares only
               // addColumns(int)/addCol(pd); none of the six methods below
               // exist there.  They all live on future::Table in
               // libscidavis/src/future/table/future_Table.h (addColumn
               // :257, addColumns :259, setSelectionAs :222,
               // setFormulaForSelection :272, fillSelectedCellsWithRow-
               // Numbers :274, fillSelectedCellsWithRandomNumbers :275),
               // reachable through Table::d_future_table; the wrapper's
               // future-table signal connections (columnsInserted etc.)
               // keep the MDI view in sync.  Win semantics: one column per
               // add_column, no dialogs; no selection → silent no-op
               // (set_as_*) or interposed QMessageBox::warning (statistics).
               // QMessageBox::warning here is embedded via ohos_bridge
               // embedDialog (blocking modal loop; QMessageBox is in
               // kEmbedDialogWhitelist); the emitMessage event is only the
               // fallback when no main window exists (startup).  The static
               // guard prevents re-entry while the embedded modal loop
               // drains queued commands.
               else if (itemId == "statistics_rows") {
                   // Table → Row Statistics: legacy ApplicationWindow path
                   // (no selection → QMessageBox::warning "Please select a
                   // row first!").
                   static bool statsRowsOpen = false;
                   if (!statsRowsOpen) {
                       statsRowsOpen = true;
                       g_mainWindow->showRowStatistics();
                       statsRowsOpen = false;
                   }
               }
               else if (itemId == "statistics_columns") {
                   // Table → Column Statistics (no selection → "Please
                   // select a column first!").
                   static bool statsColsOpen = false;
                   if (!statsColsOpen) {
                       statsColsOpen = true;
                       g_mainWindow->showColStatistics();
                       statsColsOpen = false;
                   }
               }
               else if (itemId == "add_column") {
                   // Table → Add Column: one column, no dialog (Win
                   // semantics; future::Table::addColumn() appends via
                   // setColumnCount under an undo macro, future_Table.cpp:
                   // 328).  Mirrors addTableColumns' refresh events.
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->addColumn();
                   {
                       QJsonObject ep;
                       ep["tableId"] = args["tableId"].toString();
                       scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
                   }
                   scidavisEmitEvent(QStringLiteral("tableListChanged"));
               }
               // D2 (2026-08-14): Table菜单补齐 — show_comments/recalculate/
               // go_to_cell/convert_to_matrix（Win future_Table.cpp 菜单动作
               // 同源）。
               else if (itemId == "show_comments") {
                   // Table → Show/Hide Comments: action_toggle_comments →
                   // TableView::toggleComments (future_Table.cpp L1471).
                   Table *t = resolveTable(args);
                   if (!t) return jsonError("table not found");
                   t->toggleComments();
               }
               else if (itemId == "recalculate") {
                   // Table → Recalculate: action_recalculate →
                   // Table::recalculateSelectedCells (future_Table.cpp
                   // L1425/L613; Ctrl+Return).
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->recalculateSelectedCells();
                   QJsonObject ep;
                   ep["tableId"] = t->name();
                   scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
               }
               else if (itemId == "go_to_cell") {
                   // Table → Go to Cell: two QInputDialog::getInt prompts
                   // (future_Table.cpp L1869).  Guarded (banner).
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   static bool cellOpen = false;
                   if (!cellOpen) {
                       cellOpen = true;
                       t->d_future_table->goToCell();
                       cellOpen = false;
                   }
               }
               else if (itemId == "convert_to_matrix") {
                   // Table → Convert to Matrix: ApplicationWindow::
                   // convertTableToMatrix (L2871; WaitCursor, no dialog).
                   g_mainWindow->convertTableToMatrix();
               }
               // D3 (2026-08-14): Table 剩余 5 项 — hide_controls(F12)/
               // formula_edit_mode/edit_column_description/change_type_format/
               // clear_table（Win future_Table.cpp 菜单动作同源；legacy Table
               // 继承 TableView，toggleControlTabBar/activateFormulaMode 直接可用）。
               else if (itemId == "hide_controls") {
                   Table *t = resolveTable(args);
                   if (!t) return jsonError("table not found");
                   t->toggleControlTabBar();
               }
               else if (itemId == "formula_edit_mode") {
                   // Win action_formula_mode checkable toggled(bool) →
                   // TableView::activateFormulaMode；ArkTS 无持久 checked →
                   // 按当前状态取反。
                   Table *t = resolveTable(args);
                   if (!t) return jsonError("table not found");
                   t->activateFormulaMode(!t->formulaModeActive());
               }
               else if (itemId == "edit_column_description") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->editDescriptionOfCurrentColumn();
               }
               else if (itemId == "change_type_format") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->editTypeAndFormatOfSelectedColumns();
               }
               else if (itemId == "clear_table") {
                   // Win action_clear_table → Table::clear()（破坏性，无确认）。
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->clear();
                   QJsonObject ep;
                   ep["tableId"] = t->name();
                   scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
               }
               // D3 (2026-08-14): Sort/TableSize/PlotWizard 三暂代对话框迁
               // Qt 内嵌（future::SortDialog / Ui::DimensionsDialog /
               // PlotWizard 均 exec()，由 ohos_bridge 内嵌；白名单见
               // ohos_bridge.cpp kEmbedDialogWhitelist）。
               else if (itemId == "sort_table" || itemId == "sort") {
                   // Table → Sort: future::SortDialog exec（Win
                   // future_Table.cpp L944 sortSelectedColumns；无选中列
                   // 时对话框不弹出，与 Win 一致）。guarded。
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   static bool sortOpen = false;
                   if (!sortOpen) {
                       sortOpen = true;
                       t->d_future_table->sortSelectedColumns();
                       sortOpen = false;
                   }
               }
               else if (itemId == "table_size" || itemId == "dimensions"
                        || itemId == "table_dimensions") {
                   // Table → Dimensions: QDialog(Ui::DimensionsDialog) exec
                   // 内嵌（future_Table.cpp L1887）。guarded。
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   static bool dimsOpen = false;
                   if (!dimsOpen) {
                       dimsOpen = true;
                       t->d_future_table->dimensionsDialog();
                       dimsOpen = false;
                   }
               }
               else if (itemId == "plot_wizard") {
                   // View → Plot Wizard: PlotWizard exec 内嵌
                   // （ApplicationWindow.cpp L8330；无表格 → 内嵌
                   // QMessageBox warning）。guarded。
                   static bool wizardOpen = false;
                   if (!wizardOpen) {
                       wizardOpen = true;
                       g_mainWindow->showPlotWizard();
                       wizardOpen = false;
                   }
               }
               else if (itemId == "import_ascii" || itemId == "export_ascii"
                        || itemId == "open_project" || itemId == "save_as"
                        || itemId == "export_graph_current" || itemId == "export_graph_all"
                        || itemId == "open_image_file" || itemId == "import_image"
                        || itemId == "matrix_import_image"
                         || itemId == "open_template" || itemId == "save_as_template"
                         || itemId == "export_pdf") {
                   // 2026-08-14 (route 5): native embedded file dialogs —
                   // one shared re-entry-guarded branch for the whole file
                   // family.  All target ApplicationWindow slots that open a
                   // Qt file dialog (ImportASCIIDialog / ExportDialog /
                   // OpenProjectDialog / ImageExportDialog / getSaveFileName
                   // / getOpenFileName); every dialog is whitelisted and
                   // embedded, its QComboBox popups are swallowed by
                   // InputProbe → ArkTS picker, and the initial directory is
                   // the authorized work folder (scidavisSetWorkFolderPath).
                   static bool fileDlgOpen = false;
                   if (!fileDlgOpen) {
                       fileDlgOpen = true;
                       const QString wf = scidavisWorkFolderPath();
                       if (!wf.isEmpty()) {
                            // 2026-08-14: point EVERY file dialog at the
                            // authorized work folder (Look-in shows
                            // computer/storage/Users/currentUser, not the
                            // sandbox workingDir / Computer root):
                            // asciiDirPath feeds ImportASCIIDialog, while
                            // OpenProjectDialog / ImageExportDialog /
                            // QFileDialog statics (open/saveAs/loadImage/
                            // importImage) all seed from workingDir.
                            g_mainWindow->asciiDirPath = wf;
                            g_mainWindow->workingDir = wf;
                            g_mainWindow->templatesDir = wf;  // template dialogs seed here
                        }
                       if (itemId == "import_ascii") {
                           g_mainWindow->importASCII();
                       } else if (itemId == "export_ascii") {
                           g_mainWindow->showExportASCIIDialog();
                       } else if (itemId == "open_project") {
                                                       {
                                // OHOS-safe open (2026-08-15): ApplicationWindow::open()
                                // → openProject() creates a SECOND ApplicationWindow and
                                // then closes THIS one (WA_DeleteOnClose in the ctor) —
                                // g_mainWindow dangles and the next getUiState SIGSEGVs
                                // on the freed folder tree (device cppcrash 22:39, 0x6b6b
                                // freed-object poison).  Run the embedded OpenProjectDialog
                                // and loadProject() in-place instead.
                                OpenProjectDialog opd(g_mainWindow, false);
                                opd.setDirectory(g_mainWindow->workingDir);
                                if (opd.exec() == QDialog::Accepted
                                    && !opd.selectedFiles().isEmpty()) {
                                    const QString fn = opd.selectedFiles().first();
                                    if (!QFile::exists(fn)) {
                                        QJsonObject p;
                                        p["title"] = QObject::tr("Open Project");
                                        p["text"] = QObject::tr("File not found: ") + fn;
                                        p["icon"] = QStringLiteral("critical");
                                        scidavisEmitEvent(QStringLiteral("message"), p);
                                    } else {
                                        g_mainWindow->workingDir = opd.directory().path();
                                        const bool ok = g_mainWindow->loadProject(fn);
                                        QJsonObject p;
                                        p["title"] = QObject::tr("Open Project");
                                        p["text"] = ok
                                            ? QObject::tr("Opened: ") + QFileInfo(fn).fileName()
                                            : QObject::tr("Failed to open: ") + QFileInfo(fn).fileName();
                                        p["icon"] = ok ? QStringLiteral("information")
                                                       : QStringLiteral("critical");
                                        scidavisEmitEvent(QStringLiteral("message"), p);
                                    }
                                }
                            };
                       } else if (itemId == "save_as") {
                           const QString beforeProjectName = g_mainWindow->projectname;
                           g_mainWindow->saveProjectAs();
                           // 2026-08-17: 保存/取消事件给 ArkTS quit-save 流程：
                           // 只有在 Qt 保存对话框真正成功后发 projectSaved，取消则发
                           // fileDialogCancelled，避免“点保存后直接退出/不退出”错乱。
                           if (!g_mainWindow->projectname.isEmpty()
                               && g_mainWindow->projectname != beforeProjectName) {
                               QJsonObject done;
                               done["ok"] = true;
                               done["path"] = g_mainWindow->projectname;
                               scidavisEmitEvent(QStringLiteral("projectSaved"), done);
                           } else {
                               QJsonObject cancelled;
                               cancelled["dialog"] = QStringLiteral("save_as");
                               scidavisEmitEvent(QStringLiteral("fileDialogCancelled"), cancelled);
                           }
                       } else if (itemId == "export_graph_current") {
                           g_mainWindow->exportGraph();
                       } else if (itemId == "export_graph_all") {
                           g_mainWindow->exportAllGraphs();
                       } else if (itemId == "open_image_file") {
                           g_mainWindow->loadImage();
                       } else if (itemId == "import_image") {
                           g_mainWindow->importImage();
                       } else if (itemId == "matrix_import_image") {
                            g_mainWindow->importImage();
                        } else if (itemId == "open_template") {
                            g_mainWindow->openTemplate();
                        } else if (itemId == "save_as_template") {
                            g_mainWindow->saveAsTemplate();
                        } else if (itemId == "export_pdf") {
                            // Stage-2C (2026-08-16): Export PDF 探针 —
                            // exportPDF() → QFileDialog::getSaveFileName
                            // (embedded) + MultiLayer::exportVector →
                            // QPrinter + setOutputFileName (no print dialog).
                            // 真机验证 QPrinter 能否直出 PDF；失败则回退置灰。
                            g_mainWindow->exportPDF();
                        }
                       fileDlgOpen = false;
                   }
               }
               else if (itemId == "set_values") {
                   // Table → Set Column Values / Matrix → Set Formula (Win
                   // dialogs).  Dispatch by active window type — Applica-
                   // tionWindow has no activeWindow(), so use the same
                   // d_workspace.activeSubWindow() pattern as resolveTable.
                   QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
                   Table *t = qobject_cast<Table *>(sub);
                   if (t) {
                       // Table: show the formula editor as the Control Tabs
                       // Formula page — future::Table::setFormulaForSelec-
                       // tion(), the no-dialog core of the Win
                       // SetValuesDialog (future_Table.cpp:606).
                       if (!t->d_future_table) return jsonError("table not found");
                       t->d_future_table->setFormulaForSelection();
                   } else {
                       Matrix *m = qobject_cast<Matrix *>(sub);
                       if (m) {
                           // Matrix → Set Formula: Win parity — show the
                           // Control Tabs Formula page (future_Matrix.cpp
                           // editFormula :1038 → MatrixView::showControl-
                           // FormulaTab()).  d_future_matrix is the public
                           // future::Matrix wrapper (Matrix.h:56; raw
                           // pointer, not QPointer like Table.h:55 — owned
                           // by the legacy Matrix wrapper, valid for its
                           // lifetime).  Pure tab switch, no dialog → no
                           // reentry guard.
                           if (!m->d_future_matrix) return jsonError("matrix not found");
                           m->d_future_matrix->editFormula();
                           QJsonObject p;
                           p["title"] = QObject::tr("Set Values");
                           p["text"] = QObject::tr("Matrix formula editor shown.");
                           p["icon"] = QStringLiteral("information");
                           scidavisEmitEvent(QStringLiteral("message"), p);
                       } else {
                           // Neither Table nor Matrix active — surface the
                           // Win-parity "activate first" notice.
                           QJsonObject p;
                           p["title"] = QObject::tr("Set Values");
                           p["text"] = QObject::tr("Activate a table or matrix first.");
                           p["icon"] = QStringLiteral("information");
                           scidavisEmitEvent(QStringLiteral("message"), p);
                       }
                   }
               }
               else if (itemId == "set_as_x") {
                   // Table → Set As X: setSelectionAs() iterates the
                   // selected columns; nothing selected → silent no-op
                   // (Win behavior).  Enum (globals.h): noDesignation=0,
                   // X=1, Y=2, Z=3, xErr=4, yErr=5.
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::X);
               }
               else if (itemId == "set_as_y") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::Y);
               }
               else if (itemId == "set_as_z") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::Z);
               }
               else if (itemId == "set_as_xerr") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::xErr);
               }
               else if (itemId == "set_as_yerr") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::yErr);
               }
               else if (itemId == "set_as_none") {
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->setSelectionAs(SciDAVis::noDesignation);
               }
               else if (itemId == "fill_row_numbers") {
                   // Table → Fill Selection With Row Numbers (future_Table
                   // .cpp:626; refresh event mirrors setColumnValues).
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->fillSelectedCellsWithRowNumbers();
                   {
                       QJsonObject ep;
                       ep["tableId"] = args["tableId"].toString();
                       scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
                   }
               }
               else if (itemId == "fill_random") {
                   // Table → Fill Selection With Random Values, uniform
                   // [0,1) (future_Table.cpp:670).
                   Table *t = resolveTable(args);
                   if (!t || !t->d_future_table) return jsonError("table not found");
                   t->d_future_table->fillSelectedCellsWithRandomNumbers();
                   {
                       QJsonObject ep;
                       ep["tableId"] = args["tableId"].toString();
                       scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
                   }
               }
               // ── Matrix direct commands + Analysis/Tools slots (batch B,
               // 2026-08-12) ─────────────────────────────────────────────
               // Access confirmations (grep 2026-08-12): Matrix.h:56 exposes
               // future::Matrix *d_future_matrix as a PUBLIC raw pointer
               // (Table.h:55 uses QPointer<future::Table>; here a raw
               // pointer owned by the legacy wrapper — valid for its
               // lifetime).  editCoordinates()/editFormat()/editFormula()
               // (future_Matrix.h:265-267) are plain tab switches
               // (showControlCoordinatesTab/FormatTab/FormulaTab on the
               // MatrixView, MatrixView.h:169-171); goToCell()
               // (future_Matrix.h:240) and dimensionsDialog()
               // (future_Matrix.h:239) each run two QInputDialog::getInt()
               // calls — QInputDialog is in kEmbedDialogWhitelist
               // (ohos_bridge.cpp:237), so the QDialog::exec interposer
               // embeds each prompt and its nested modal loop drains queued
               // commands → static reentry guard (preferences pattern).
               // correlate()/autoCorrelate()/convolute()/deconvolute()/
               // movePoints()/removePoints() are ApplicationWindow public
               // slots (ApplicationWindow.h:570-573 / 621-622) that pop
               // QMessageBox::warning on bad selection / empty plot — same
               // embedded modal loop → same static guard (statistics_rows
               // pattern).
               else if (itemId == "matrix_set_coordinates") {
                   // Matrix → Set Coordinates: editCoordinates() →
                   // showControlCoordinatesTab() (future_Matrix.cpp:1031).
                   // No dialog → no reentry guard.
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->editCoordinates();
               }
               else if (itemId == "matrix_set_display_format") {
                   // Matrix → Set Display Format: editFormat() →
                   // showControlFormatTab() (future_Matrix.cpp:1024).  No
                   // dialog → no reentry guard.
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->editFormat();
               }
               else if (itemId == "matrix_go_to_cell") {
                   // Matrix → Go to Cell: goToCell() (future_Matrix.cpp:882)
                   // prompts "Enter column" then "Enter row" via two
                   // QInputDialog::getInt(0, ...) — a null parent is fine,
                   // the interposer embeds by class name.  Guarded (banner).
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   static bool cellOpen = false;
                   if (!cellOpen) {
                       cellOpen = true;
                       m->d_future_matrix->goToCell();
                       cellOpen = false;
                   }
               }
               else if (itemId == "matrix_dimensions") {
                   // Matrix → Dimensions: dimensionsDialog() (future_Matrix
                   // .cpp:967) prompts "Enter number of columns" / "rows"
                   // via two QInputDialog::getInt() calls, then
                   // setDimensions(rows, cols).  Guarded (banner).
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   static bool dimsOpen = false;
                   if (!dimsOpen) {
                       dimsOpen = true;
                       m->d_future_matrix->dimensionsDialog();
                       dimsOpen = false;
                   }
               }
               // D2 (2026-08-14): Matrix菜单补齐 — recalculate/clear/
               // mirror_v/determinant/convert_to_table（Win future_Matrix.cpp
               // connectActions 同源）。
               else if (itemId == "matrix_recalculate") {
                   // Matrix → Recalculate: action_recalculate →
                   // recalculateSelectedCells (future_Matrix.cpp L728/L1519).
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->recalculateSelectedCells();
               }
               else if (itemId == "matrix_clear") {
                   // Matrix → Clear Matrix: action_clear_matrix → clear()
                   // (future_Matrix.cpp L731/L191).  Destructive, no confirm
                   // (Win semantics).
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->clear();
               }
               else if (itemId == "matrix_mirror_v") {
                   // Matrix → Mirror Vertically (future_Matrix.cpp L734/L1512).
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->mirrorVertically();
               }
               else if (itemId == "matrix_determinant") {
                   // Matrix → Determinant: ApplicationWindow::matrixDeterminant
                   // appends to the Results Log dock (L2770; no dialog).
                   g_mainWindow->matrixDeterminant();
               }
               else if (itemId == "matrix_convert_to_table") {
                   // Matrix → Convert to Table: ApplicationWindow::
                   // convertMatrixToTable (L2796; WaitCursor, no dialog).
                   g_mainWindow->convertMatrixToTable();
               }
               // D3 (2026-08-14): Matrix 剩余 2 项 — hide_controls(F12)/
               // mirror_h（Win future_Matrix.cpp connectActions 同源）。
               else if (itemId == "matrix_hide_controls") {
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->toggleControlTabBar();
               }
               else if (itemId == "matrix_mirror_h") {
                   // Matrix → Mirror Horizontally（future_Matrix.cpp L733）。
                   Matrix *m = qobject_cast<Matrix *>(g_mainWindow->d_workspace.activeSubWindow());
                   if (!m || !m->d_future_matrix) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Matrix");
                       p["text"] = QObject::tr("Activate a matrix first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("matrix not found");
                   }
                   m->d_future_matrix->mirrorHorizontally();
               }
               else if (itemId == "correlate") {
                   // Analysis → Correlate (Win ApplicationWindow.cpp:5609):
                   // exactly 2 selected columns, else QMessageBox::warning
                   // "Please select two columns for this operation!".
                   static bool correlateOpen = false;
                   if (!correlateOpen) {
                       correlateOpen = true;
                       g_mainWindow->correlate();
                       correlateOpen = false;
                   }
               }
               else if (itemId == "autocorrelate") {
                   // Analysis → Autocorrelate (:5627): exactly 1 column,
                   // else "Please select exactly one columns...".
                   static bool autocorrelateOpen = false;
                   if (!autocorrelateOpen) {
                       autocorrelateOpen = true;
                       g_mainWindow->autoCorrelate();
                       autocorrelateOpen = false;
                   }
               }
               else if (itemId == "convolute") {
                   // Analysis → Convolute (:5645): exactly 2 selected
                   // columns (signal + response), else warning.  The
                   // dedicated "convolve" command (L3043) instead takes
                   // signalCol/responseCol JSON args with no selection
                   // check/warning, so it is NOT equivalent to the Win menu
                   // action; keep it for the column context menu and route
                   // the menu here for parity.
                   static bool convoluteOpen = false;
                   if (!convoluteOpen) {
                       convoluteOpen = true;
                       g_mainWindow->convolute();
                       convoluteOpen = false;
                   }
               }
               else if (itemId == "deconvolute") {
                   // Analysis → Deconvolute (:5664): same 2-column check.
                   static bool deconvoluteOpen = false;
                   if (!deconvoluteOpen) {
                       deconvoluteOpen = true;
                       g_mainWindow->deconvolute();
                       deconvoluteOpen = false;
                   }
               }
               else if (itemId == "move_points") {
                   // ⚠ 当前 ArkTS 菜单已置灰禁用（2026-08-13 真机实测），此分支保留备用。
                   // 根因：OH 编译的 QwtPicker 是 54 行 stub（qwt_picker.cpp 构造不
                   // installEventFilter、eventFilter 恒 false）→ DataPickerTool 事件处理无效；
                   // 且 setActiveTool 后 canvas->setCursor() 在单窗口 QPA 软件渲染下引发合成错乱。
                   // 启用条件：未来恢复完整 Qwt 交互层后可重新启用。
                   static bool movePointsOpen = false;
                   if (!movePointsOpen) {
                       movePointsOpen = true;
                       g_mainWindow->movePoints();
                       movePointsOpen = false;
                   }
               }
               else if (itemId == "remove_bad") {
                   // D3 (2026-08-14): QwtPicker 完整版恢复 → 工具链复活。
                   static bool removePointsOpen = false;
                   if (!removePointsOpen) {
                       removePointsOpen = true;
                       g_mainWindow->removePoints();
                       removePointsOpen = false;
                   }
               }
               // D3 (2026-08-14): Tools 菜单其余工具 — QwtPicker 完整版恢复后
               // 全部启用（Win pickDataTool 同源：btnZoomIn/btnZoomOut/
               // btnCursor/btnPicker/btnSelect → zoomIn/zoomOut/showCursor/
               // showScreenReader/showRangeSelectors；disable_tools →
               // pickPointerCursor）。
               else if (itemId == "zoom_in") {
                   static bool ziOpen = false;
                   if (!ziOpen) {
                       ziOpen = true;
                       g_mainWindow->zoomIn();
                       ziOpen = false;
                   }
               }
               else if (itemId == "zoom_out") {
                   static bool zoOpen = false;
                   if (!zoOpen) {
                       zoOpen = true;
                       g_mainWindow->zoomOut();
                       zoOpen = false;
                   }
               }
               else if (itemId == "data_reader") {
                   // Data Reader = btnCursor → showCursor()（Display 模式
                   // DataPickerTool）。
                   static bool curOpen = false;
                   if (!curOpen) {
                       curOpen = true;
                       g_mainWindow->showCursor();
                       curOpen = false;
                   }
               }
               else if (itemId == "screen_reader") {
                   static bool srOpen = false;
                   if (!srOpen) {
                       srOpen = true;
                       g_mainWindow->showScreenReader();
                       srOpen = false;
                   }
               }
               else if (itemId == "range") {
                   static bool rngOpen = false;
                   if (!rngOpen) {
                       rngOpen = true;
                       g_mainWindow->showRangeSelectors();
                       rngOpen = false;
                   }
               }
               else if (itemId == "disable_tools") {
                   // Tools → Disable Tools: pointer cursor = 禁用全部工具
                   // （Win btnPointer/pickPointerCursor）。
                   g_mainWindow->pickPointerCursor();
               }
               // D3 (2026-08-14): Graph 图层 / Edit 清理 / Analysis 平移 —
               // add_layer（三选 QMessageBox 内嵌）/ remove_layer
               // （confirmRemoveLayer 内嵌确认）/ delete_fit_tables /
               // clear_log_info / translate_horizontal/vertical
               // （TranslateCurveTool，QwtPicker 恢复后可用）。
               else if (itemId == "add_layer") {
                   static bool addLayerOpen = false;
                   if (!addLayerOpen) {
                       addLayerOpen = true;
                       g_mainWindow->addLayer();
                       addLayerOpen = false;
                   }
               }
               else if (itemId == "automatic_layout") {
    // Win Graph → Automatic Layout (autoArrangeLayers()).
    static bool aal = false;
    if (!aal) {
        aal = true;
        g_mainWindow->autoArrangeLayers();
        aal = false;
    }
}
else if (itemId == "remove_layer") {
                   static bool delLayerOpen = false;
                   if (!delLayerOpen) {
                       delLayerOpen = true;
                       g_mainWindow->deleteLayer();
                       delLayerOpen = false;
                   }
               }
               else if (itemId == "delete_fit_tables")
                   g_mainWindow->deleteFitTables();
               else if (itemId == "clear_log_info")
                   g_mainWindow->clearLogInfo();
               else if (itemId == "translate_horizontal")
                   g_mainWindow->translateCurveHor();
               else if (itemId == "translate_vertical")
                   g_mainWindow->translateCurveVert();
               // ── Analysis menu (Phase 5, 2026-08-13) ──────────────
               // Five direct fits run straight through the ApplicationWindow
               // analysis() slot (no dialog).  The dialog branches below
               // (IntDialog/FFTDialog/SmoothCurveDialog/FilterDialog/
               // InterpolationDialog/PolynomFitDialog/ExpDecayDialog) are all
               // whitelisted in ohos_bridge kEmbedDialogWhitelist, so their
               // exec()/show() opens embedded; each keeps a static reentry
               // guard like the stage-4 dialog branches (the embedded modal
               // loop drains queued commands).
               else if (itemId == "an_fit_linear") {
                   const int logBefore = analysisProbe("an_fit_linear");
                   g_mainWindow->fitLinear(); // 直连，无对话框
                   // Phase-5 fix 2026-08-13: direct commands write results
                   // into Qt logInfo; relay the delta to ArkTS results log
                   // (logWindow dock is hidden on OH).
                   emitResultsLogDelta(logBefore);
                   qWarning("[ANALYSIS] an_fit_linear after: logDelta=%d logWindowVisible=%d",
                            g_mainWindow->logInfo.length() - logBefore,
                            g_mainWindow->logWindow.isVisible() ? 1 : 0);
               }
               else if (itemId == "an_fit_sigmoidal") {
                   const int logBefore = analysisProbe("an_fit_sigmoidal");
                   g_mainWindow->fitSigmoidal(); // 直连，无对话框
                   // Phase-5 fix 2026-08-13: see an_fit_linear.
                   emitResultsLogDelta(logBefore);
                   qWarning("[ANALYSIS] an_fit_sigmoidal after: logDelta=%d logWindowVisible=%d",
                            g_mainWindow->logInfo.length() - logBefore,
                            g_mainWindow->logWindow.isVisible() ? 1 : 0);
               }
               else if (itemId == "an_fit_gauss") {
                   const int logBefore = analysisProbe("an_fit_gauss");
                   g_mainWindow->fitGauss(); // 直连，无对话框
                   // Phase-5 fix 2026-08-13: see an_fit_linear.
                   emitResultsLogDelta(logBefore);
                   qWarning("[ANALYSIS] an_fit_gauss after: logDelta=%d logWindowVisible=%d",
                            g_mainWindow->logInfo.length() - logBefore,
                            g_mainWindow->logWindow.isVisible() ? 1 : 0);
               }
               else if (itemId == "an_fit_lorentz") {
                   const int logBefore = analysisProbe("an_fit_lorentz");
                   g_mainWindow->fitLorentz(); // 直连，无对话框
                   // Phase-5 fix 2026-08-13: see an_fit_linear.
                   emitResultsLogDelta(logBefore);
                   qWarning("[ANALYSIS] an_fit_lorentz after: logDelta=%d logWindowVisible=%d",
                            g_mainWindow->logInfo.length() - logBefore,
                            g_mainWindow->logWindow.isVisible() ? 1 : 0);
               }
               // D3 (2026-08-14): Quick Fit → Fit Multi-Peak（Win L11831
               // fitMultiPeakGauss/Lorentz）：内部 QInputDialog（内嵌）+
               // MultiPeakFitTool（依赖 DataPickerTool → QwtPicker stub 下
               // 选峰交互受限，2026-08-14 评估记录；入口可达，不崩溃）。
               else if (itemId == "fit_multi_peak_gauss") {
                   static bool mpOpen = false;
                   if (!mpOpen) {
                       mpOpen = true;
                       g_mainWindow->fitMultiPeakGauss();
                       mpOpen = false;
                   }
               }
               else if (itemId == "fit_multi_peak_lorentz") {
                   static bool mpOpen = false;
                   if (!mpOpen) {
                       mpOpen = true;
                       g_mainWindow->fitMultiPeakLorentz();
                       mpOpen = false;
                   }
               }
               else if (itemId == "an_differentiate") {
                   const int logBefore = analysisProbe("an_differentiate");
                   g_mainWindow->differentiate(); // 直连，无对话框
                   // Phase-5 fix 2026-08-13: see an_fit_linear.  differentiate()
                   // also spawns a new graph window (DerivativeN) into the MDI
                   // workspace, so log the sub-window count after the call.
                   emitResultsLogDelta(logBefore);
                   qWarning("[ANALYSIS] an_differentiate after: logDelta=%d logWindowVisible=%d subWindows=%d",
                            g_mainWindow->logInfo.length() - logBefore,
                            g_mainWindow->logWindow.isVisible() ? 1 : 0,
                            g_mainWindow->d_workspace.subWindowList().size());
               }
               else if (itemId == "an_integrate") {
                   // Analysis → Integrate (IntDialog, non-modal show()).
                   static bool integrateOpen = false;
                   if (!integrateOpen) {
                       integrateOpen = true;
                       g_mainWindow->showIntegrationDialog();
                       integrateOpen = false;
                   }
               }
               else if (itemId == "an_fft") {
                   // Analysis → FFT (FFTDialog, exec()).
                   static bool fftOpen = false;
                   if (!fftOpen) {
                       fftOpen = true;
                       g_mainWindow->showFFTDialog();
                       fftOpen = false;
                   }
               }
               else if (itemId == "an_fit_poly") {
                   // Analysis → Fit Polynomial (PolynomFitDialog, show()).
                   static bool polyFitOpen = false;
                   if (!polyFitOpen) {
                       polyFitOpen = true;
                       g_mainWindow->showFitPolynomDialog();
                       polyFitOpen = false;
                   }
               }
               else if (itemId == "an_smooth_savgol") {
                   // Analysis → Smooth (Savitzky-Golay; SmoothCurveDialog,
                   // exec()).
                   static bool smoothSGOpen = false;
                   if (!smoothSGOpen) {
                       smoothSGOpen = true;
                       g_mainWindow->showSmoothSavGolDialog();
                       smoothSGOpen = false;
                   }
               }
               else if (itemId == "an_smooth_average") {
                   // Analysis → Smooth (Average).
                   static bool smoothAvgOpen = false;
                   if (!smoothAvgOpen) {
                       smoothAvgOpen = true;
                       g_mainWindow->showSmoothAverageDialog();
                       smoothAvgOpen = false;
                   }
               }
               else if (itemId == "an_smooth_fft") {
                   // Analysis → Smooth (FFT).
                   static bool smoothFFTOpen = false;
                   if (!smoothFFTOpen) {
                       smoothFFTOpen = true;
                       g_mainWindow->showSmoothFFTDialog();
                       smoothFFTOpen = false;
                   }
               }
               else if (itemId == "an_fft_filter_low") {
                   // Analysis → FFT Filter (Low Pass; FilterDialog, exec()).
                   static bool filterLowOpen = false;
                   if (!filterLowOpen) {
                       filterLowOpen = true;
                       g_mainWindow->lowPassFilterDialog();
                       filterLowOpen = false;
                   }
               }
               else if (itemId == "an_fft_filter_high") {
                   // Analysis → FFT Filter (High Pass).
                   static bool filterHighOpen = false;
                   if (!filterHighOpen) {
                       filterHighOpen = true;
                       g_mainWindow->highPassFilterDialog();
                       filterHighOpen = false;
                   }
               }
               else if (itemId == "an_fft_filter_bandpass") {
                   // Analysis → FFT Filter (Band Pass).
                   static bool filterBandOpen = false;
                   if (!filterBandOpen) {
                       filterBandOpen = true;
                       g_mainWindow->bandPassFilterDialog();
                       filterBandOpen = false;
                   }
               }
               else if (itemId == "an_fft_filter_bandblock") {
                   // Analysis → FFT Filter (Band Block).
                   static bool filterBlockOpen = false;
                   if (!filterBlockOpen) {
                       filterBlockOpen = true;
                       g_mainWindow->bandBlockFilterDialog();
                       filterBlockOpen = false;
                   }
               }
               else if (itemId == "an_exp_decay_1") {
                   // Analysis → Fit Exponential Decay (ExpDecayDialog type 1,
                   // show()).
                   static bool expDecay1Open = false;
                   if (!expDecay1Open) {
                       expDecay1Open = true;
                       g_mainWindow->showExpDecayDialog(1);
                       expDecay1Open = false;
                   }
               }
               else if (itemId == "an_exp_decay_2") {
                   // Analysis → Fit Two-Exponential Decay (type 2).
                   static bool expDecay2Open = false;
                   if (!expDecay2Open) {
                       expDecay2Open = true;
                       g_mainWindow->showTwoExpDecayDialog();
                       expDecay2Open = false;
                   }
               }
               else if (itemId == "an_exp_decay_3") {
                   // Analysis → Fit Three-Exponential Decay (type 3).
                   static bool expDecay3Open = false;
                   if (!expDecay3Open) {
                       expDecay3Open = true;
                       g_mainWindow->showExpDecay3Dialog();
                       expDecay3Open = false;
                   }
               }
               else if (itemId == "an_exp_growth") {
                   // Analysis → Fit Exponential Growth (type -1).
                   static bool expGrowthOpen = false;
                   if (!expGrowthOpen) {
                       expGrowthOpen = true;
                       g_mainWindow->showExpGrowthDialog();
                       expGrowthOpen = false;
                   }
               }
               else if (itemId == "an_interpolate") {
                   // Analysis → Interpolate (InterpolationDialog, show()).
                   // NOTE: this branch was missing from the phase-5 task
                   // list but IS in the ArkTS an_* dispatch table — it must
                   // be implemented (task context §6).
                   static bool interpolateOpen = false;
                   if (!interpolateOpen) {
                       interpolateOpen = true;
                       g_mainWindow->showInterpolationDialog();
                       interpolateOpen = false;
                   }
               }
               // ── Format dialogs (Phase 6, 2026-08-13) ────────────────
               // TextDialog/LineDialog/ImageDialog/LayerDialog/
               // CurveRangeDialog/AssociationsDialog/PlotDialog are all
               // whitelisted in ohos_bridge kEmbedDialogWhitelist with
               // size-table entries, so their exec()/show() opens embedded
               // (same static-reentry-guard pattern as the stage-4/5
               // dialog branches; the embedded modal loop drains queued
               // commands).  The marker-targeted dialogs (show_text /
               // show_line / show_image) resolve the active graph + selected
               // marker inside the ApplicationWindow slots, exactly as on
               // desktop.  The curve-targeted ones (edit_range / associ-
               // ations / plot_details) resolve the active graph's current
               // curve HERE via menuCurveKey(): the desktop reads it from
               // the right-clicked action data, which the blocked context
               // menus never populate on device.
               else if (itemId == "show_text") {
                   // Format → Title: TextDialog in AxisTitle mode
                   // (showTitleDialog(); TextMarker mode is show_text_marker).
                   static bool titleOpen = false;
                   if (!titleOpen) {
                       titleOpen = true;
                       g_mainWindow->showTitleDialog();
                       titleOpen = false;
                   }
               }
               else if (itemId == "show_line") {
                   // Format → Line: LineDialog for the selected arrow
                   // marker (desktop slot returns silently without one).
                   static bool lineOpen = false;
                   if (!lineOpen) {
                       lineOpen = true;
                       g_mainWindow->showLineDialog();
                       lineOpen = false;
                   }
               }
               else if (itemId == "show_image") {
                   // Format → Image: ImageDialog for the selected image
                   // marker.
                   static bool imageOpen = false;
                   if (!imageOpen) {
                       imageOpen = true;
                       g_mainWindow->showImageDialog();
                       imageOpen = false;
                   }
               }
               else if (itemId == "arrange_layers") {
                   // Format → Arrange Layers: LayerDialog (exec()).
                   static bool layersOpen = false;
                   if (!layersOpen) {
                       layersOpen = true;
                       g_mainWindow->showLayerDialog();
                       layersOpen = false;
                   }
               }
               else if (itemId == "edit_range") {
                   // Format → Curve Range (CurveRangeDialog, show()): the
                   // no-arg desktop slot reads the curve key from the
                   // right-clicked action data, so resolve the active
                   // graph's current curve here and use the 2-arg overload
                   // (which takes the curve INDEX).
                   MultiLayer *ml = qobject_cast<MultiLayer *>(g_mainWindow->d_workspace.activeSubWindow());
                   Graph *g = ml ? ml->activeGraph() : nullptr;
                   int key = menuCurveKey(g, args);
                   if (key < 0) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Curve Range");
                       p["text"] = QObject::tr("Select a curve on the active graph first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("no curve");
                   }
                   static bool rangeOpen = false;
                   if (!rangeOpen) {
                       rangeOpen = true;
                       g_mainWindow->showCurveRangeDialog(g, g->curveIndex(key));
                       rangeOpen = false;
                   }
               }
               else if (itemId == "associations") {
                   // Format → Plot Associations (AssociationsDialog, exec()).
                   // The desktop slot takes the curve index into the
                   // dialog's data-curve list; pass the plot-item index
                   // (the dialog clamps an out-of-range value to row 0).
                   MultiLayer *ml = qobject_cast<MultiLayer *>(g_mainWindow->d_workspace.activeSubWindow());
                   Graph *g = ml ? ml->activeGraph() : nullptr;
                   int key = menuCurveKey(g, args);
                   if (key < 0) {
                       QJsonObject p;
                       p["title"] = QObject::tr("Plot Associations");
                       p["text"] = QObject::tr("Select a curve on the active graph first.");
                       p["icon"] = QStringLiteral("information");
                       scidavisEmitEvent(QStringLiteral("message"), p);
                       return jsonError("no curve");
                   }
                   static bool assocOpen = false;
                   if (!assocOpen) {
                       assocOpen = true;
                       g_mainWindow->showPlotAssociations(g->curveIndex(key));
                       assocOpen = false;
                   }
               }
               else if (itemId == "plot_details") {
                   // Format → Plot Details (PlotDialog, show()).  The slot
                   // takes the curve KEY; with none resolvable the -1
                   // default still opens the dialog for the active graph
                   // (desktop parity).
                   MultiLayer *ml = qobject_cast<MultiLayer *>(g_mainWindow->d_workspace.activeSubWindow());
                   Graph *g = ml ? ml->activeGraph() : nullptr;
                   int key = menuCurveKey(g, args);
                   static bool plotDetailsOpen = false;
                   if (!plotDetailsOpen) {
                       plotDetailsOpen = true;
                       g_mainWindow->showPlotDialog(key);
                       plotDetailsOpen = false;
                   }
               }
               else if (itemId == "window_geometry") {
                   // Format → Window Geometry (ImageDialog reused, exec()).
                   static bool winGeomOpen = false;
                   if (!winGeomOpen) {
                       winGeomOpen = true;
                       g_mainWindow->resizeActiveWindow();
                       winGeomOpen = false;
                   }
               }
               else
                   ok = false;
              if (!ok)
                  return jsonError("unsupported on device: " + itemId.toStdString());
              return "{\"success\":true}";
          } },

        // ── Graph interaction (Phase 4) ──────────────────────────────
        // Direct ApplicationWindow method calls for zoom, rescale, and
        // pointer tools.  QMessageBox warnings inside these methods are
        // caught by the ohos_bridge interposer.
        { "rescale",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->setAutoScale();
              return "{\"success\":true}";
          } },

        { "graph_pointer",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->pickPointerCursor();
              return "{\"success\":true}";
          } },

        { "zoom_in",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->zoomIn();
              return "{\"success\":true}";
          } },

        { "zoom_out",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->zoomOut();
              return "{\"success\":true}";
          } },

        { "screen_reader",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->showScreenReader();
              return "{\"success\":true}";
          } },

        { "data_reader",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              g_mainWindow->showCursor();
              return "{\"success\":true}";
          } },

        // ── Function / curve / error-bar commands (Phase 4) ───────────
        // add_curve / add_error_bars / add_function receive JSON
        // parameters from the ArkTS dialogs and call the Qt engine API
        // directly.  They never open QDialogs (the OHOS single-window
        // QPA blocks them), so all errors are reported via jsonError.
        { "new_function",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              // newFunctionPlot requires formula parameters normally
              // obtained from a FunctionDialog.  Without a dialog,
              // create a default y=x function plot (type=0 normal).
              int type = 0;
              QStringList formulas{ QStringLiteral("x") };
              QString var = QStringLiteral("x");
              QList<double> ranges{ 0.0, 1.0 };
              int points = 1000;
              bool ok = g_mainWindow->newFunctionPlot(type, formulas, var, ranges, points);
              if (!ok)
                  return jsonError("newFunctionPlot failed");
              return "{\"success\":true}";
          } },

        { "add_curve",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              static const std::map<std::string, int> styles = {
                  { "line", Graph::Line },
                  { "scatter", Graph::Scatter },
                  { "line_symbol", Graph::LineSymbols },
                  { "vertical_bars", Graph::VerticalBars },
                  { "horizontal_bars", Graph::HorizontalBars },
                  { "area", Graph::Area },
                  { "pie", Graph::Pie },
                  { "drop_lines", Graph::VerticalDropLines },
                  { "spline", Graph::Spline },
                  { "vertical_steps", Graph::VerticalSteps },
                  { "horizontal_steps", Graph::HorizontalSteps },
                  { "histogram", Graph::Histogram },
                  { "box", Graph::Box },
              };
              // No QDialog: minimal replica of CurvesDialog::addCurve --
              // insert each requested column as a curve on the active
              // graph with the requested style, then replot.  Column
              // colours/symbols are left at Graph defaults.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QString tableId = args["tableId"].toString();
              if (tableId.isEmpty()) return jsonError("missing tableId");
              Table *t = g_mainWindow->table(tableId);
              if (!t) return jsonError("table not found");
              auto it = styles.find(args["style"].toString(QStringLiteral("line")).toStdString());
              if (it == styles.end()) return jsonError("unknown style");
              const int style = it->second;
              if (!args["cols"].isArray()) return jsonError("missing cols");
              int added = 0;
              for (const QJsonValue &v : args["cols"].toArray()) {
                  int idx = v.toInt(-1);
                  if (idx >= 0 && idx < t->numCols()) {
                      if (g->insertCurve(t, t->colName(idx), style))
                          added++;
                  }
              }
              if (added == 0) return jsonError("no valid columns");
              g->updatePlot();
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              QJsonObject r;
              r["success"] = true;
              r["added"] = added;
               return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "remove_curve",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              // Two forms:
              //   1) curveName (string) — remove the curve with that title.
              //   2) tableId + cols (array) — remove all DataCurves whose
              //      source table is tableId and whose yColumnName is one of
              //      the named columns.
              const QString curveName = args["curveName"].toString();
              int removed = 0;
              if (!curveName.isEmpty()) {
                  if (g->curve(curveName))
                      g->removeCurve(curveName);
                  else
                      return jsonError("curve not found");
                  removed = 1;
              } else {
                  const QString tableId = args["tableId"].toString();
                  if (tableId.isEmpty()) return jsonError("missing curveName or tableId");
                  Table *t = g_mainWindow->table(tableId);
                  if (!t) return jsonError("table not found");
                  if (!args["cols"].isArray()) return jsonError("missing cols");
                  // Collect the y-column names to match.
                  QStringList colNames;
                  for (const QJsonValue &v : args["cols"].toArray()) {
                      int idx = v.toInt(-1);
                      if (idx >= 0 && idx < t->numCols())
                          colNames.append(t->colName(idx));
                  }
                  if (colNames.isEmpty()) return jsonError("no valid columns");
                  // Walk curves; remove any DataCurve sourced from t with a
                  // matching yColumnName.  removeCurve(QString) re-searches
                  // the plot item list each call, so it is safe to invoke
                  // repeatedly without index bookkeeping.
                  for (int i = 0; i < g->curves(); i++) {
                      QwtPlotCurve *pc = g->curve(i);
                      if (!pc) continue;
                      DataCurve *dc = dynamic_cast<DataCurve *>(pc);
                      if (!dc) continue;
                      if (dc->table() == t && colNames.contains(dc->yColumnName())) {
                          g->removeCurve(dc->yColumnName());
                          removed++;
                      }
                  }
              }
              if (removed == 0) return jsonError("no matching curves found");
              g->updatePlot();
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              QJsonObject r;
              r["success"] = true;
              r["removed"] = removed;
               return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
           } },

        { "toggleExplorer",
           [](const QJsonObject &) -> std::string {
               if (!g_mainWindow) return jsonError("no mw");
               g_mainWindow->explorerWindow.toggleViewAction()->trigger();
               return "{\"success\":true}";
           } },

        { "toggleLog",
           [](const QJsonObject &) -> std::string {
               if (!g_mainWindow) return jsonError("no mw");
               g_mainWindow->logWindow.toggleViewAction()->trigger();
               return "{\"success\":true}";
           } },

        { "setDockVisible",
           [](const QJsonObject &args) -> std::string {
               if (!g_mainWindow) return jsonError("no mw");
               const QString dock = args["dock"].toString();
               const bool vis = args["visible"].toBool();
               if (dock == QStringLiteral("explorer"))
                   g_mainWindow->explorerWindow.setVisible(vis);
               else if (dock == QStringLiteral("log"))
                   g_mainWindow->logWindow.setVisible(vis);
               else
                   return jsonError("unknown dock");
               return "{\"success\":true}";
           } },

        { "add_error_bars",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              // No QDialog: replicate both
              // ApplicationWindow::defineErrorBars() overloads with the
              // graph resolved from plotId instead of the active MDI
              // subwindow.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QString curveName = args["curveName"].toString();
              if (curveName.isEmpty()) return jsonError("missing curveName");
              DataCurve *master = static_cast<DataCurve *>(g->curve(curveName));
              if (!master) return jsonError("curve not found");
              const QString xColName = master->xColumnName();
              if (xColName.isEmpty()) return jsonError("no x column");
              // QwtErrorPlotCurve::Horizontal == 0, Vertical == 1
              const int direction =
                  args["direction"].toString(QStringLiteral("y")) == QStringLiteral("x") ? 0 : 1;
              const QString mode = args["mode"].toString();
              if (mode == QStringLiteral("compute")) {
                  // Curve source table = curveName with the "_<col>"
                  // suffix stripped (ApplicationWindow::table() does the
                  // same truncation).
                  const int pos = curveName.indexOf(QStringLiteral("_"));
                  const QString tableName = pos >= 0 ? curveName.left(pos) : curveName;
                  Table *w = g_mainWindow->table(tableName);
                  if (!w) return jsonError("table not found");
                  Column *errors = new Column(QStringLiteral("1"), SciDAVis::ColumnMode::Numeric);
                  errors->setPlotDesignation(direction == 0 ? SciDAVis::xErr : SciDAVis::yErr);
                  Column *data = w->d_future_table->column(direction == 0 ? xColName : curveName);
                  if (!data) {
                      delete errors;
                      return jsonError("data column not found");
                  }
                  const int rows = data->rowCount();
                  if (args["type"].toInt(0) == 0) {
                      // percent of the data values
                      const double fraction = args["percent"].toDouble(5.0) / 100.0;
                      for (int i = 0; i < rows; i++)
                          errors->setValueAt(i, data->valueAt(i) * fraction);
                  } else {
                      // standard deviation of the data column
                      double average = 0.0;
                      for (int i = 0; i < rows; i++)
                          average += data->valueAt(i);
                      average /= rows;
                      double dev = 0.0;
                      for (int i = 0; i < rows; i++)
                          dev += (data->valueAt(i) - average) * (data->valueAt(i) - average);
                      dev = std::sqrt(dev / rows);
                      for (int i = 0; i < rows; i++)
                          errors->setValueAt(i, dev);
                  }
                  w->d_future_table->addChild(errors);
                  if (!g->addErrorBars(xColName, curveName, w, errors->name(), direction))
                      return jsonError("addErrorBars failed");
              } else if (mode == QStringLiteral("column")) {
                  const QString errTableId = args["errTableId"].toString();
                  const QString errColumnName = args["errColumnName"].toString();
                  if (errTableId.isEmpty()) return jsonError("missing errTableId");
                  if (errColumnName.isEmpty()) return jsonError("missing errColumnName");
                  Table *errTable = g_mainWindow->table(errTableId);
                  if (!errTable) return jsonError("err table not found");
                  if (!g->addErrorBars(curveName, errTable, errColumnName, direction))
                      return jsonError("addErrorBars failed");
              } else {
                  return jsonError("unknown mode");
              }
              emit g_mainWindow->modified();
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "add_function",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              // No QDialog: replicate the dialog-free path of
              // addFunctionCurve(); without an active graph fall back to
              // newFunctionPlot(), which creates a new plot window.
              const int type = args["type"].toInt(0);
              QStringList formulas;
              if (args["formulas"].isArray()) {
                  for (const QJsonValue &v : args["formulas"].toArray())
                      formulas << v.toString();
              }
              if (formulas.isEmpty()) return jsonError("missing formulas");
              // normal needs 1 formula, parametric / polar need 2
              if (formulas.count() != (type == 0 ? 1 : 2)) return jsonError("wrong formula count");
              const QString var = args["var"].toString(QStringLiteral("x"));
              const QJsonArray rangeArr = args["ranges"].toArray();
              if (rangeArr.count() != 2) return jsonError("missing ranges");
              QList<double> ranges;
              for (const QJsonValue &v : rangeArr)
                  ranges << v.toDouble();
              if (!(ranges[0] < ranges[1])) return jsonError("invalid range");
              const int points = args["points"].toInt(1000);
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) {
                  if (!g_mainWindow->newFunctionPlot(type, formulas, var, ranges, points))
                      return jsonError("newFunctionPlot failed");
                  scidavisEmitEvent(QStringLiteral("plotListChanged"));
                  QJsonObject r;
                  r["success"] = true;
                  r["newPlot"] = true;
                  return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
              }
              if (!g->addFunctionCurve(g_mainWindow, type, formulas, var, ranges, points))
                  return jsonError("function parse failed");
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "getPreference",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QSettings &s = ApplicationWindow::getSettings();
              QString key = args["key"].toString();
              if (key.isEmpty()) return jsonError("missing key");
              QVariant v = s.value(key);
              QJsonObject r;
              r["success"] = true;
              r["value"] = v.toString();
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "setPreference",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QSettings &s = ApplicationWindow::getSettings();
              QString key = args["key"].toString();
              if (key.isEmpty()) return jsonError("missing key");
              s.setValue(key, args["value"].toString());
              return "{\"success\":true}";
          } },

        { "setMenuBottomLimit",
          [](const QJsonObject &args) -> std::string {
              // ArkTS-measured bottom toolbar top (global Y, physical px).
              // 2026-08-17: overrides Qt content-bottom clamping when the
              // Qt window and ArkTS toolbar top are misaligned by a few px.
              const int globalY = args["globalY"].toInt(-1);
              scidavisSetMenuBottomLimit(globalY);
              return "{\"success\":true}";
          } },

        { "setChromeInsets",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              int top = args["top"].toInt(0);
              int bottom = args["bottom"].toInt(0);
              // Inset the WHOLE QMainWindow so docks (Top/BottomDockWidgetArea)
              // and the central widget all live inside the ArkTS chrome band.
              // setViewportMargins only shrank the central MdiArea, leaving
              // docks overlapped by the ArkTS overlay and a blank MdiArea-margin
              // band above the bottom dock (the white strip on device).
              g_mainWindow->setContentsMargins(0, top, 0, bottom);
              if (auto *cw = g_mainWindow->centralWidget())
                  cw->update();
              return "{\"success\":true}";
          } },

        { "getNoteData",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QString name = args["noteId"].toString();
              Note *note = nullptr;
              if (name.isEmpty()) {
                  note = qobject_cast<Note *>(g_mainWindow->d_workspace.activeSubWindow());
              } else {
                  for (MyWidget *w : g_mainWindow->windowsList()) {
                      if (w->inherits("Note") && w->name() == name) {
                          note = static_cast<Note *>(w);
                          break;
                      }
                  }
              }
              if (!note) return jsonError("note not found");
              QJsonObject r;
              r["success"] = true;
              r["text"] = note->text();
              r["name"] = note->name();
              r["label"] = note->windowLabel();
              return QJsonDocument(r).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "setNoteData",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QString name = args["noteId"].toString();
              Note *note = nullptr;
              if (name.isEmpty()) {
                  note = qobject_cast<Note *>(g_mainWindow->d_workspace.activeSubWindow());
              } else {
                  for (MyWidget *w : g_mainWindow->windowsList()) {
                      if (w->inherits("Note") && w->name() == name) {
                          note = static_cast<Note *>(w);
                          break;
                      }
                  }
              }
              if (!note) return jsonError("note not found");
              note->setText(args["text"].toString());
return "{\"success\":true}";
           } },

        { "insert_row",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
              MyWidget *w = qobject_cast<MyWidget *>(sub);
              if (!w || !w->inherits("Table"))
                  return jsonError("no active table");
              static_cast<Table *>(w)->insertRow();
              {
                  QJsonObject ep;
                  ep["tableId"] = w->name();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              return "{\"success\":true}";
          } },

        { "insert_col",
          [](const QJsonObject &) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
              MyWidget *w = qobject_cast<MyWidget *>(sub);
              if (!w || !w->inherits("Table"))
                  return jsonError("no active table");
              static_cast<Table *>(w)->insertCol();
              {
                  QJsonObject ep;
                  ep["tableId"] = w->name();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              scidavisEmitEvent(QStringLiteral("tableListChanged"));
              return "{\"success\":true}";
          } },

        { "normalize",
          [](const QJsonObject &args) -> std::string {
              // future::Table::normalizeColumns — no-dialog core of
              // NormalizeDialog.  columns (optional) filters which columns
              // are normalised; default = every column in the table.
              Table *t = resolveTable(args);
              if (!t || !t->d_future_table) return jsonError("table not found");
              QList<Column *> cols;
              if (args["columns"].isArray()) {
                  QSet<QString> wanted;
                  for (const QJsonValue &v : args["columns"].toArray())
                      wanted.insert(v.toString());
                  for (int i = 0; i < t->numCols(); i++) {
                      Column *c = t->column(i);
                      if (c && wanted.contains(c->name()))
                          cols << c;
                  }
              } else {
                  for (int i = 0; i < t->numCols(); i++)
                      cols << t->column(i);
              }
              if (cols.isEmpty()) return jsonError("no matching columns");
              t->d_future_table->normalizeColumns(cols);
              QJsonObject p;
              p["title"] = QObject::tr("Normalize");
              p["text"] = QObject::tr("Normalized %1").arg(t->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // Notify ArkTS to refresh table data after normalize
              {
                  QJsonObject ep;
                  ep["tableId"] = args["tableId"].toString();
                  scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              }
              return "{\"success\":true}";
          } },

        { "convolve",
          [](const QJsonObject &args) -> std::string {
              // Convolution filter (libscidavis Convolution.h) — convolves
              // signalCol with responseCol and appends a result curve.
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              QString signalCol = args["signalCol"].toString();
              QString responseCol = args["responseCol"].toString();
              if (signalCol.isEmpty() || responseCol.isEmpty())
                  return jsonError("missing signalCol/responseCol");
              bool ok = false;
              try {
                  Convolution *conv =
                          new Convolution(g_mainWindow, t, signalCol, responseCol);
                  ok = conv->run();
                  delete conv;
              } catch (...) {
                  ok = false;
              }
              QJsonObject p;
              p["title"] = QObject::tr("Convolution");
              p["text"] = ok ? QObject::tr("Convolved %1").arg(t->name())
                             : QObject::tr("Convolution failed on %1").arg(t->name());
              p["icon"] = ok ? QStringLiteral("information") : QStringLiteral("critical");
              scidavisEmitEvent(QStringLiteral("message"), p);
              if (ok) scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return ok ? std::string("{\"success\":true}") : jsonError("convolution failed");
          } },

        { "getMatrixData",
          [](const QJsonObject &args) -> std::string {
              // Read-only: return the full matrix cell contents as a 2-D
              // JSON array (rows of cols).  Mirrors getTableData.
              Matrix *m = resolveMatrix(args);
              if (!m) return jsonError("matrix not found");
              QJsonArray rows;
              for (int r = 0; r < m->numRows(); r++) {
                  QJsonArray rowArr;
                  for (int c = 0; c < m->numCols(); c++)
                      rowArr.append(m->text(r, c));
                  rows.append(rowArr);
              }
              QJsonObject data;
              data["rows"] = m->numRows();
              data["cols"] = m->numCols();
              data["cells"] = rows;
              QJsonObject root;
              root["success"] = true;
              root["data"] = data;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "addText",
          [](const QJsonObject &args) -> std::string {
              // Add a legend/text marker to the active graph layer and
              // consume the 11 styling keys sent by TextDialog
              // (text/fontFamily/fontSize/bold/italic/textColor/bgFrame/
              // bgOpacity/bgColor/alignment/angle).
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QString text = args["text"].toString();
              Legend *l;
              if (text.isEmpty())
                  l = g->newLegend();
              else
                  l = g->newLegend(text);
              if (!l) return jsonError("failed to create text marker");
              QFont f = l->font();
              const QString family = args["fontFamily"].toString();
              if (!family.isEmpty())
                  f.setFamily(family);
              if (args.contains("fontSize"))
                  f.setPointSizeF(args["fontSize"].toDouble(f.pointSizeF()));
              f.setBold(args["bold"].toBool(false));
              f.setItalic(args["italic"].toBool(false));
              l->setFont(f);
              const QString textColor = args["textColor"].toString();
              if (!textColor.isEmpty())
                  l->setTextColor(QColor(textColor));
              const QString bgColor = args["bgColor"].toString();
              if (!bgColor.isEmpty()) {
                  QColor bg(bgColor);
                  // bgOpacity is sent as an alpha in the 0-255 range.
                  if (args.contains("bgOpacity"))
                      bg.setAlpha(qBound(0, args["bgOpacity"].toInt(255), 255));
                  l->setBackgroundColor(bg);
              }
              // TextDialog's bgFrameIdx maps directly onto the Legend frame
              // styles (None=0/Line=1/Shadow=2).
              if (args.contains("bgFrame"))
                  l->setFrameStyle(args["bgFrame"].toInt(int(Legend::None)));
              if (args.contains("angle"))
                  l->setAngle(args["angle"].toInt(0));
              // "alignment" has no Qt Legend equivalent: the desktop build
              // only uses it for axis titles, so it is intentionally
              // ignored here.
              g->updatePlot();
              QJsonObject p;
              p["title"] = QObject::tr("Add Text");
              p["text"] = QObject::tr("Added text to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "drawArrow",
          [](const QJsonObject &args) -> std::string {
              // Draw an arrow marker (axes-value coordinates) on the graph.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              // LineDialog sends xStart/yStart/xEnd/yEnd; keep x1/y1/x2/y2
              // as a legacy fallback.
              double x1 = args["xStart"].toDouble(args["x1"].toDouble(0.0));
              double y1 = args["yStart"].toDouble(args["y1"].toDouble(0.0));
              double x2 = args["xEnd"].toDouble(args["x2"].toDouble(0.0));
              double y2 = args["yEnd"].toDouble(args["y2"].toDouble(0.0));
              auto *mrk = new ArrowMarker();
              mrk->setStartPoint(x1, y1);
              mrk->setEndPoint(x2, y2);
              mrk->setWidth(args["width"].toInt(1));
              mrk->setColor(QColor(args["color"].toString(QStringLiteral("black"))));
              // drawArrow defaults to arrow heads at both ends when the
              // flags are absent.
              const bool startArrow = args.contains("arrowStart")
                      ? args["arrowStart"].toBool() : true;
              const bool endArrow = args.contains("arrowEnd")
                      ? args["arrowEnd"].toBool() : true;
              mrk->drawStartArrow(startArrow);
              mrk->drawEndArrow(endArrow);
              if (args.contains("headLength"))
                  mrk->setHeadLength(args["headLength"].toInt(mrk->headLength()));
              if (args.contains("headAngle"))
                  mrk->setHeadAngle(args["headAngle"].toInt(mrk->headAngle()));
              if (args.contains("headFilled"))
                  mrk->fillArrowHead(args["headFilled"].toBool(true));
              g->addArrow(mrk);
              QJsonObject p;
              p["title"] = QObject::tr("Draw Arrow");
              p["text"] = QObject::tr("Arrow added to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "drawLine",
          [](const QJsonObject &args) -> std::string {
              // Draw a plain line marker (no arrow heads) on the graph.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              // LineDialog sends xStart/yStart/xEnd/yEnd; keep x1/y1/x2/y2
              // as a legacy fallback.
              double x1 = args["xStart"].toDouble(args["x1"].toDouble(0.0));
              double y1 = args["yStart"].toDouble(args["y1"].toDouble(0.0));
              double x2 = args["xEnd"].toDouble(args["x2"].toDouble(0.0));
              double y2 = args["yEnd"].toDouble(args["y2"].toDouble(0.0));
              auto *mrk = new ArrowMarker();
              mrk->setStartPoint(x1, y1);
              mrk->setEndPoint(x2, y2);
              mrk->setWidth(args["width"].toInt(1));
              mrk->setColor(QColor(args["color"].toString(QStringLiteral("black"))));
              // drawLine defaults to no arrow heads when the flags are
              // absent.
              const bool startArrow = args.contains("arrowStart")
                      ? args["arrowStart"].toBool() : false;
              const bool endArrow = args.contains("arrowEnd")
                      ? args["arrowEnd"].toBool() : false;
              mrk->drawStartArrow(startArrow);
              mrk->drawEndArrow(endArrow);
              if (args.contains("headLength"))
                  mrk->setHeadLength(args["headLength"].toInt(mrk->headLength()));
              if (args.contains("headAngle"))
                  mrk->setHeadAngle(args["headAngle"].toInt(mrk->headAngle()));
              if (args.contains("headFilled"))
                  mrk->fillArrowHead(args["headFilled"].toBool(true));
              g->addArrow(mrk);
              QJsonObject p;
              p["title"] = QObject::tr("Draw Line");
              p["text"] = QObject::tr("Line added to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "timeStamp",
          [](const QJsonObject &args) -> std::string {
              // One-liner: add a timestamp text marker to the graph.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              g->addTimeStamp();
              QJsonObject p;
              p["title"] = QObject::tr("Time Stamp");
              p["text"] = QObject::tr("Timestamp added to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "addImage",
          [](const QJsonObject &args) -> std::string {
              // Insert an image file as an image marker on the graph and
              // consume the 5 keys sent by ImageDialog (originX/originY/
              // width/height/keepAspect), all in paint/pixel coordinates.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QString filePath = args["filePath"].toString();
              if (filePath.isEmpty()) return jsonError("missing filePath");
              if (!QFile::exists(filePath)) return jsonError("file not found");
              ImageMarker *m = g->addImage(filePath);
              if (!m) return jsonError("failed to add image");
              int x = args["originX"].toInt(0);
              int y = args["originY"].toInt(0);
              int w = args["width"].toInt(0);
              int h = args["height"].toInt(0);
              const bool keepAspect = args["keepAspect"].toBool(false);
              if (w <= 0 && h <= 0) {
                  // No explicit size: keep the canvas-clamped default sizing
                  // that addImage() already applied.
              } else {
                  const QSize src = m->pixmap().size();
                  if (w <= 0) w = src.width();
                  if (h <= 0) h = src.height();
                  // Preserve the source pixmap aspect ratio when requested
                  // (same ratio source as Graph::addImage).
                  if (keepAspect && src.width() > 0 && src.height() > 0) {
                      const double ratio = double(src.width()) / double(src.height());
                      if (w / double(h) > ratio)
                          w = qRound(h * ratio);
                      else
                          h = qRound(w / ratio);
                  }
                  m->setRect(x, y, w, h);
              }
              g->updatePlot();
              QJsonObject p;
              p["title"] = QObject::tr("Add Image");
              p["text"] = QObject::tr("Image added to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "newLegend",
          [](const QJsonObject &args) -> std::string {
              // Add a legend to the graph; auto-generated from curve titles
              // when no explicit text is supplied.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              QString text = args["text"].toString();
              if (text.isEmpty())
                  g->newLegend();
              else
                  g->newLegend(text);
              QJsonObject p;
              p["title"] = QObject::tr("New Legend");
              p["text"] = QObject::tr("Legend added to %1").arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "tableSize",
          [](const QJsonObject &args) -> std::string {
              // Read-only: return the row/column count of a table.
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              QJsonObject root;
              root["success"] = true;
              root["rows"] = t->rowCount();
              root["cols"] = t->columnCount();
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "resizeTable",
          [](const QJsonObject &args) -> std::string {
              // Mutation: resize a table to the given dimensions
              // (TableSizeDialog "Table Size").  Existing cell data is kept;
              // rows/cols below 1 are rejected.
              Table *t = resolveTable(args);
              if (!t) return jsonError("table not found");
              int rows = args["rows"].toInt(0);
              int cols = args["cols"].toInt(0);
              if (rows < 1 || cols < 1) return jsonError("bad dimensions");
              t->setNumRows(rows);
              t->setNumCols(cols);
              QJsonObject p;
              p["title"] = QObject::tr("Table Size");
              p["text"] = QObject::tr("Resized %1 to %2 × %3")
                      .arg(t->name()).arg(rows).arg(cols);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              QJsonObject ep;
              ep["tableId"] = t->name();
              scidavisEmitEvent(QStringLiteral("tableDataChanged"), ep);
              return "{\"success\":true}";
          } },

        // ── Dialog suite (batch 4) ─────────────────────────────────────
        // One command per ArkTS dialog: PlotWizard, Axes, Associations,
        // Curve Range, Arrange Layers, Rename Window, Find.  Each dialog
        // was read to match its exact arg keys and response shape.
        { "plotWizard",
          [](const QJsonObject &args) -> std::string {
              if (!g_mainWindow) return jsonError("no mw");
              if (!args["curves"].isArray()) return jsonError("missing curves");
              // Reuse the desktop PlotWizard path: build the same
              // "Table: XCol(X), YCol(Y)" spec strings that
              // ApplicationWindow::multilayerPlot(const QStringList&)
              // parses (it handles the X/Y curves and xErr/yErr error
              // bars).  Master curves first, error bars appended last,
              // mirroring PlotWizard::accept().
              QStringList colList, errList;
              int plotted = 0;
              bool zSkipped = false;
              for (const QJsonValue &v : args["curves"].toArray()) {
                  const QJsonObject cv = v.toObject();
                  const QString tableId = cv["tableId"].toString();
                  const QString xCol = cv["xCol"].toString();
                  const QString yCol = cv["yCol"].toString();
                  if (tableId.isEmpty() || xCol.isEmpty() || yCol.isEmpty())
                      continue;
                  if (!cv["zCol"].toString().isEmpty()) {
                      zSkipped = true; // 3-D curves have no 2-D wizard path
                      continue;
                  }
                  const QString master = tableId + ": " + xCol + "(X), " + yCol + "(Y)";
                  colList << master;
                  if (!cv["xErrCol"].toString().isEmpty())
                      errList << master + ", " + cv["xErrCol"].toString() + "(xErr)";
                  if (!cv["yErrCol"].toString().isEmpty())
                      errList << master + ", " + cv["yErrCol"].toString() + "(yErr)";
                  plotted++;
              }
              if (colList.isEmpty() && errList.isEmpty())
                  return jsonError("no valid curves");
              colList += errList;
              MultiLayer *ml = g_mainWindow->multilayerPlot(colList);
              if (!ml) return jsonError("plot failed");
              QJsonObject p;
              p["title"] = QObject::tr("Plot Wizard");
              p["text"] = QObject::tr("Plotted %1 curve(s)").arg(plotted);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              if (zSkipped) {
                  QJsonObject pz;
                  pz["title"] = QObject::tr("Plot Wizard");
                  pz["text"] = QObject::tr("3-D (Z) curves are not supported and were skipped");
                  pz["icon"] = QStringLiteral("warning");
                  scidavisEmitEvent(QStringLiteral("message"), pz);
              }
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "getAxisConfig",
          [](const QJsonObject &args) -> std::string {
              // Read-only: one axis at a time (Qwt axis id: Bottom=2,
              // Left=0, Top=3, Right=1).  Shape matches AxesDialog's
              // AxisConfigData.  Fields with no clean getter fall back to
              // safe defaults rather than failing.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int axis = qBound(0, args["axis"].toInt(0), 3);
              const QwtScaleDiv *scDiv = g->plotWidget()->axisScaleDiv(axis);
              const QwtScaleEngine *se = g->plotWidget()->axisScaleEngine(axis);
              QJsonObject data;
              data["from"] = std::min(scDiv->lowerBound(), scDiv->upperBound());
              data["to"] = std::max(scDiv->lowerBound(), scDiv->upperBound());
              data["scaleType"] =
                      (se && se->transformation()
                       && se->transformation()->type() == QwtScaleTransformation::Log10)
                              ? QStringLiteral("log")
                              : QStringLiteral("linear");
              data["inverted"] = se ? se->testAttribute(QwtScaleEngine::Inverted) : false;
              data["title"] = g->axisTitle(axis);
              data["visible"] = g->plotWidget()->axisEnabled(axis);
              // Grid: the axis id is a QwtPlot axis (yLeft=0, yRight=1,
              // xBottom=2, xTop=3).  Y axes drive the grid's Y lines,
              // X axes its X lines (same mapping as setAxisGrid).
              Grid *grid = g->grid();
              if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight) {
                  data["majorGrid"] = grid && grid->yEnabled();
                  data["minorGrid"] = grid && grid->yMinEnabled();
              } else {
                  data["majorGrid"] = grid && grid->xEnabled();
                  data["minorGrid"] = grid && grid->xMinEnabled();
              }
              data["backbones"] = g->axesBackbones();
              data["lineWidth"] = g->plotWidget()->axesLinewidth();
              data["majorTicksLength"] = g->majorTickLength();
              data["minorTicksLength"] = g->minorTickLength();
              // Grid pen styling for the same X/Y grid lines this axis drives
              // (mirrors the fields setAxisGrid accepts).
              if (grid) {
                  if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight) {
                      data["gridColor"] = grid->majPenY().color().name();
                      data["gridLineStyle"] = static_cast<int>(grid->majPenY().style());
                      data["gridLineWidth"] = grid->majPenY().width();
                  } else {
                      data["gridColor"] = grid->majPenX().color().name();
                      data["gridLineStyle"] = static_cast<int>(grid->majPenX().style());
                      data["gridLineWidth"] = grid->majPenX().width();
                  }
              }
              QJsonObject root;
              root["success"] = true;
              root["data"] = data;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "getPlotAssociations",
          [](const QJsonObject &args) -> std::string {
              // Read-only: per-curve X/Y/xErr/yErr column associations for
              // the active graph.  Shape matches AssociationsDialog's
              // PlotAssociationEntry (curveName, tableName, xCol, yCol,
              // xErrCol, yErrCol).
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const QStringList names = g->analysableCurvesList();
              QJsonArray arr;
              for (int i = 0; i < names.count(); i++) {
                  QJsonObject o;
                  o["curveName"] = names[i];
                  // analysableCurvesList() skips error-bar curves, so look
                  // the curve up by name (not by index) to stay aligned.
                  if (auto *dc = dynamic_cast<DataCurve *>(g->curve(names[i]))) {
                      Table *t = dc->table();
                      const QString tname = t ? t->name() : QString();
                      // Strip the "Table_" prefix so the dialog shows the
                      // bare column names (as the desktop AssociationsDialog
                      // does).
                      auto strip = [&tname](const QString &full) {
                          if (!tname.isEmpty() && full.startsWith(tname + "_"))
                              return full.mid(tname.length() + 1);
                          return full;
                      };
                      o["tableName"] = tname;
                      o["xCol"] = strip(dc->xColumnName());
                      o["yCol"] = strip(dc->yColumnName());
                      // Error-bar associations live in separate curves and
                      // are not exposed by DataCurve::plotAssociation().
                      o["xErrCol"] = QString();
                      o["yErrCol"] = QString();
                  } else {
                      o["tableName"] = QString();
                      o["xCol"] = QString();
                      o["yCol"] = QString();
                      o["xErrCol"] = QString();
                      o["yErrCol"] = QString();
                  }
                  arr.append(o);
              }
              QJsonObject root;
              root["success"] = true;
              root["data"] = arr;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "updatePlotAssociation",
          [](const QJsonObject &args) -> std::string {
              // Re-associate a curve's X/Y data columns.  Consumes the
              // structured contract sent by AssociationsDialog:
              // {curveIdx, tableName, xCol, yCol, xErrCol, yErrCol}.
              // Mirrors AssociationsDialog::changePlotAssociation() for the
              // plain 2-column case: column ids are of the form
              // "<tableName>_<colName>".  Error-bar reassignment
              // (xErrCol/yErrCol) would require re-targeting a separate
              // QwtErrorPlotCurve and is downgraded to a warning log (no UI
              // dialog) until supported.
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              const int idx = args["curveIdx"].toInt(-1);
              const QStringList names = g->analysableCurvesList();
              if (idx < 0 || idx >= names.count())
                  return jsonError("curve index out of range");
              auto *dc = dynamic_cast<DataCurve *>(g->curve(names[idx]));
              if (!dc) return jsonError("curve not a data curve");
              const QString tableName = args["tableName"].toString();
              const QString xCol = args["xCol"].toString();
              const QString yCol = args["yCol"].toString();
              if (tableName.isEmpty() || xCol.isEmpty() || yCol.isEmpty())
                  return jsonError("missing table/column association");
              const QString fullX = tableName + "_" + xCol;
              const QString fullY = tableName + "_" + yCol;
              if (dc->xColumnName() != fullX || dc->yColumnName() != fullY) {
                  dc->setXColumnName(fullX);
                  dc->setYColumnName(fullY);
                  dc->loadData();
              }
              const QString xErrCol = args["xErrCol"].toString();
              const QString yErrCol = args["yErrCol"].toString();
              if (!xErrCol.isEmpty() || !yErrCol.isEmpty())
                  qWarning("[SciDAVis] updatePlotAssociation: error-bar reassociation "
                           "(xErr=%s, yErr=%s) not supported yet; X/Y reassigned only",
                           qPrintable(xErrCol), qPrintable(yErrCol));
              g->updatePlot();
              g->notifyChanges();
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "getCurveInfo",
          [](const QJsonObject &args) -> std::string {
              // Read-only: row range + max rows for one curve.  Shape
              // matches CurveRangeDialog's CurveInfoData (curveIdx,
              // curveName, startRow, endRow, maxRows; the dialog displays
              // the 0-based rows as 1-based).
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int idx = args["curveIdx"].toInt(-1);
              const QStringList names = g->analysableCurvesList();
              if (idx < 0 || idx >= names.count())
                  return jsonError("curve index out of range");
              QJsonObject data;
              data["curveIdx"] = idx;
              data["curveName"] = names[idx];
              if (auto *dc = dynamic_cast<DataCurve *>(g->curve(names[idx]))) {
                  data["startRow"] = dc->startRow();
                  data["endRow"] = dc->endRow();
                  data["maxRows"] = dc->table() ? dc->table()->numRows() : 0;
              } else {
                  data["startRow"] = 0;
                  data["endRow"] = 0;
                  data["maxRows"] = 0;
              }
              QJsonObject root;
              root["success"] = true;
              root["data"] = data;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "setCurveRange",
          [](const QJsonObject &args) -> std::string {
              // Apply a 0-based row range to a curve (mirrors
              // CurveRangeDialog::accept -> DataCurve::setRowRange).
              MultiLayer *ml = resolvePlot(args);
              Graph *g = ml ? ml->activeGraph() : nullptr;
              if (!g) return jsonError("no active graph");
              int idx = args["curveIdx"].toInt(-1);
              const QStringList names = g->analysableCurvesList();
              if (idx < 0 || idx >= names.count())
                  return jsonError("curve index out of range");
              auto *dc = dynamic_cast<DataCurve *>(g->curve(names[idx]));
              if (!dc) return jsonError("curve not a data curve");
              const int maxRows = dc->table() ? dc->table()->numRows() : 0;
              const int start = qBound(0, args["startRow"].toInt(0), qMax(0, maxRows - 1));
              int end = args["endRow"].toInt(-1);
              if (end < 0)
                  end = qMax(0, maxRows - 1);
              else
                  end = qBound(0, end, qMax(0, maxRows - 1));
              dc->setRowRange(qMin(start, end), qMax(start, end));
              g->updatePlot();
              QJsonObject p;
              p["title"] = QObject::tr("Curve Range");
              p["text"] = QObject::tr("Set range of %1 to rows %2..%3")
                                  .arg(dc->yColumnName())
                                  .arg(qMin(start, end) + 1)
                                  .arg(qMax(start, end) + 1);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "arrangeLayers",
          [](const QJsonObject &args) -> std::string {
              // No-dialog core of LayerDialog::update() minus the
              // QMessageBox popups: set layer count, grid, canvas,
              // alignment, margins and spacing, then lay them out.
              MultiLayer *ml = resolvePlot(args);
              if (!ml) return jsonError("no active plot");
              const int graphs = qBound(0, args["layers"].toInt(ml->layers()), 100);
              ml->setLayersNumber(graphs);
              if (!graphs) {
                  QJsonObject p;
                  p["title"] = QObject::tr("Arrange Layers");
                  p["text"] = QObject::tr("Removed all layers from %1").arg(ml->name());
                  p["icon"] = QStringLiteral("information");
                  scidavisEmitEvent(QStringLiteral("message"), p);
                  scidavisEmitEvent(QStringLiteral("plotListChanged"));
                  return "{\"success\":true}";
              }
              const bool fit = args["autoLayout"].toBool(false);
              if (!fit) {
                  ml->setCols(qMax(1, args["gridCols"].toInt(1)));
                  ml->setRows(qMax(1, args["gridRows"].toInt(1)));
              }
              if (args["customCanvas"].toBool(false))
                  ml->setLayerCanvasSize(args["canvasWidth"].toInt(600),
                                         args["canvasHeight"].toInt(400));
              ml->setAlignement(args["alignH"].toInt(0), args["alignV"].toInt(0));
              ml->setMargins(args["marginLeft"].toInt(0), args["marginRight"].toInt(0),
                             args["marginTop"].toInt(0), args["marginBottom"].toInt(0));
              // setSpacing(rowGap, colGap) — row gap first, then column gap.
              ml->setSpacing(args["rowsGap"].toInt(5), args["colsGap"].toInt(5));
              ml->arrangeLayers(fit, args["customCanvas"].toBool(false));
              QJsonObject p;
              p["title"] = QObject::tr("Arrange Layers");
              p["text"] = QObject::tr("Arranged %1 layer(s) of %2").arg(graphs).arg(ml->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "getActiveWindowInfo",
          [](const QJsonObject &) -> std::string {
              // Read-only: name/label/caption-policy of the active MDI
              // subwindow (RenameWindowDialog's ActiveWindowInfo).
              if (!g_mainWindow) return jsonError("no mw");
              QMdiSubWindow *sub = g_mainWindow->d_workspace.activeSubWindow();
              MyWidget *w = qobject_cast<MyWidget *>(sub);
              if (!w) return jsonError("no active window");
              QJsonObject data;
              data["name"] = w->name();
              data["label"] = w->windowLabel();
              data["type"] = QString::fromLatin1(w->metaObject()->className());
              data["captionPolicy"] = int(w->captionPolicy());
              QJsonObject root;
              root["success"] = true;
              root["data"] = data;
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "renameWindow",
          [](const QJsonObject &args) -> std::string {
              // No-dialog replica of RenameWindowDialog::accept():
              // rename (via ApplicationWindow::renameWindow), then update
              // label + caption policy and sync the project tree.
              if (!g_mainWindow) return jsonError("no mw");
              const QString windowName = args["windowName"].toString();
              MyWidget *w = nullptr;
              if (!windowName.isEmpty()) {
                  for (MyWidget *cand : g_mainWindow->windowsList())
                      if (cand->name() == windowName) { w = cand; break; }
              }
              if (!w)
                  w = qobject_cast<MyWidget *>(g_mainWindow->d_workspace.activeSubWindow());
              if (!w) return jsonError("window not found");
              const QString text = args["newName"].toString();
              const QString label = args["newLabel"].toString();
              const int policy = args["captionPolicy"].toInt(int(w->captionPolicy()));
              if (!text.isEmpty() && text != w->name()) {
                  if (!g_mainWindow->renameWindow(w, text))
                      return jsonError("rename failed");
              }
              w->setWindowLabel(label);
              w->setCaptionPolicy(static_cast<MyWidget::CaptionPolicy>(qBound(0, policy, 2)));
              g_mainWindow->setListViewLabel(w->name(), label);
              g_mainWindow->modifiedProject(w);
              QJsonObject p;
              p["title"] = QObject::tr("Rename Window");
              p["text"] = QObject::tr("Renamed %1").arg(w->name());
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              // No dedicated window-list event on the ArkTS side; refresh
              // the list the renamed window belongs to so the new name
              // propagates to the UI.
              if (w->inherits("Table"))
                  scidavisEmitEvent(QStringLiteral("tableListChanged"));
              else if (w->inherits("MultiLayer"))
                  scidavisEmitEvent(QStringLiteral("plotListChanged"));
              return "{\"success\":true}";
          } },

        { "getStartPath",
          [](const QJsonObject &) -> std::string {
              // FindDialog shows this as the search start folder; the
              // project folder is not exposed, so default to the root.
              QJsonObject root;
              root["success"] = true;
              root["path"] = QStringLiteral("/");
              return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
          } },

        { "find",
          [](const QJsonObject &args) -> std::string {
              // Minimal search over window names/labels/folder names; the
              // found subwindow is activated (same as the desktop Find).
              if (!g_mainWindow) return jsonError("no mw");
              const QString text = args["text"].toString();
              if (text.isEmpty()) return jsonError("empty search text");
              g_mainWindow->find(text, args["searchWindowNames"].toBool(true),
                                 args["searchWindowLabels"].toBool(false),
                                 args["searchFolderNames"].toBool(false),
                                 args["caseSensitive"].toBool(false),
                                 args["partialMatch"].toBool(true),
                                 args["includeSubfolders"].toBool(true));
              QJsonObject p;
              p["title"] = QObject::tr("Find");
              p["text"] = QObject::tr("Searched for \"%1\"").arg(text);
              p["icon"] = QStringLiteral("information");
              scidavisEmitEvent(QStringLiteral("message"), p);
              return "{\"success\":true}";
          } },

    };
    return reg;
}

extern "C" __attribute__((visibility("default")))
const char *scidavis_call(const char *cmd, const char *jsonArgs)
{
    static std::string s_ret;

    QCoreApplication *core = QCoreApplication::instance();
    if (!core) {
        s_ret = "{\"success\":false,\"error\":\"Qt not running\"}";
        return s_ret.c_str();
    }
    // UAF fix (2026-08-16): once teardown begins (aboutToQuit / window
    // destroyed), refuse to queue anything — a command queued now could run
    // against a half-destroyed object graph while the QPA plugin still pumps
    // events during teardown (crash 2026-08-15 22:39:33).
    if (g_qtShuttingDown.load()) {
        s_ret = "{\"success\":false,\"error\":\"Qt shutting down\"}";
        return s_ret.c_str();
    }

    // Deadlock guard (device appfreeze THREAD_BLOCK_6S 20260727155932): the
    // Qt thread can synchronously call back into the ArkTS JS thread through
    // the QPA plugin (e.g. Table::insertColumns -> setOverrideCursor ->
    // QOpenHarmonyJsFunction::call).  If this JS thread blocks on the Qt
    // thread at the same time, both deadlock and the watchdog kills the app.
    // Mutation commands are therefore queued fire-and-forget; query commands
    // (read-only, no QPA callbacks) wait with a 300ms timeout fallback (P0
    // 2026-08-12: was 3s — each timeout froze the ArkTS main thread, and
    // repeated startup timeouts were what the watchdog saw as ANR).
    const std::string cmdName(cmd ? cmd : "");
    const std::string argsCopy(jsonArgs ? jsonArgs : "{}");
    const bool isQuery = queryCommands().count(cmdName) > 0;

    struct CallState {
        QSemaphore done;
        std::string result;
        std::atomic<bool> abandoned { false };
    };
    auto state = std::make_shared<CallState>();

    QMetaObject::invokeMethod(core, [cmdName, argsCopy, state]() {
        // P1 (2026-08-12, plan 0245 §4 B / O4): the waiting JS thread already
        // gave up (query timeout) — skip the handler entirely instead of
        // executing it late.  Executing abandoned queries only fed a flood
        // of stale work into the Qt event queue after a timeout burst (plan
        // 0245 §2.4 R4); the caller is gone, so the result is dead weight.
        if (state->abandoned.load()) {
            state->done.release();
            return;
        }
        // UAF fix (2026-08-16): skip execution once teardown began — the
        // window may already be destroyed (g_mainWindow is a QPointer, so
        // the handlers' null checks are safe too, but the flag also covers
        // the during-destructor window where the pointer is not yet null).
        if (g_qtShuttingDown.load()) {
            state->done.release();
            return;
        }
        // P1 (2026-08-14): modalEnded drop self-heal.  The event channel is
        // a nonblocking TSFN (qohos.cpp) — a full queue DROPS the event, so
        // a lost modalEnded would leave the ArkTS getUiState polling paused
        // forever.  A queued command only runs here when the Qt event loop
        // is free, i.e. no modal loop is open; if the modal stack is empty
        // but the shell may still believe a modal is up, re-emit
        // modalEnded.  No-op when no modal is pending or one is open.
        scidavis_flush_modal_ended();
        std::string s_result;
        const std::string &cmdStr = cmdName;
        QJsonDocument doc = QJsonDocument::fromJson(
                QByteArray(argsCopy.c_str(), int(argsCopy.size())));
        QJsonObject argsObj = doc.object();

        const auto &registry = commandRegistry();
        auto it = registry.find(cmdStr);
        if (it != registry.end()) {
            try {
                s_result = it->second(argsObj);
            } catch (const std::exception &e) {
                s_result = jsonError(std::string("exception: ") + e.what());
            } catch (...) {
                s_result = jsonError("unknown exception");
            }
        } else {
            s_result = jsonError("unknown cmd: " + cmdStr);
        }
        if (state->abandoned.load())
            OHOS_LOG("scidavis_call[%s] finished after timeout: %s",
                     cmdStr.c_str(), s_result.c_str());
        else
            state->result = std::move(s_result);
        state->done.release();
    }, Qt::QueuedConnection);

    if (!isQuery) {
        // Fire-and-forget: the action runs as soon as the Qt event loop is
        // idle.  Returning now keeps the JS thread free to service any QPA
        // callback the action triggers (cursor, window title, ...).
        s_ret = "{\"success\":true,\"queued\":true}";
        return s_ret.c_str();
    }

    // P0 (2026-08-12): 300ms instead of 3s — the caller's polling loop
    // (now ≥1.5s apart, no inner retry) absorbs a timeout as a cheap miss
    // instead of freezing the ArkTS main thread.
    if (state->done.tryAcquire(1, 300)) {
        s_ret = state->result;
    } else {
        // Qt thread busy or blocked on a QPA callback: give up so the JS
        // thread can drain it; the queued lambda will still run later.
        state->abandoned.store(true);
        s_ret = "{\"success\":false,\"error\":\"timeout\"}";
    }
    return s_ret.c_str();
}

int main(int argc, char **argv)
{

    QCoreApplication::setOrganizationName("SciDAVis");
    QCoreApplication::setApplicationName("SciDAVis");

    Application app(argc, argv);

    // QMdiSubWindow title-bar fix (2026-08-13): guarantee a sane
    // PM_TitleBarHeight so MDI sub-window title bars (and their × close
    // buttons) never collapse to zero height.
    app.setStyle(new MdiTitleBarStyle);

    app.installEventFilter(new InputProbe);
    s_shortcutClock.start(); // wall-clock fallback for the shortcut-map dedup

#ifdef __APPLE__
    // set python home to bundle specific directory
    setenv("PYTHONHOME",(QCoreApplication::applicationDirPath()+"/../Resources").toStdString().c_str(),true);
#endif
    
    ApplicationWindow::getSettings();
    
    QStringList args = app.arguments();
    args.removeFirst(); // remove application name

    if ((args.count() == 1) && (args[0] == "-a" || args[0] == "--about")) {
        ApplicationWindow::about();
        exit(0);
    } else {
        // OHOS: enlarge the default application font by 3pt for tablet
        // readability.  Set BEFORE the ApplicationWindow is constructed so
        // its appFont (a QFont() copy of the app default) captures the
        // enlarged size — initFonts() then derives tableTextFont /
        // tableHeaderFont / the menu & toolbar chrome from it, keeping the
        // whole UI consistent (the a6-baseline QPA default is ~9pt, too
        // small on the 2456x1600 MatePad).  Window geometry is untouched.
        // User round-5 #4: the QPA default CJK fallback is a SERIF face
        // (宋体-like, visibly different from the ArkTS shell which uses the
        // system HarmonyOS Sans 黑体), so load the SC face directly and pin
        // the family — harmless (no-op fallback) if the path is unreadable.
        QFont uiFont = app.font();
#ifdef SCIDAVIS_OHOS
        {
            const QString hms = QStringLiteral("/system/fonts/HarmonyOS_Sans_SC.ttf");
            const int fid = QFontDatabase::addApplicationFont(hms);
            if (fid >= 0) {
                const QStringList fams = QFontDatabase::applicationFontFamilies(fid);
                if (!fams.isEmpty())
                    uiFont.setFamily(fams.first());
                // User round-5 #4 (follow-up): app.setFont() alone does not
                // reach every widget — some pick their font from
                // QSettings-carried families (Table user font) or their own
                // QFont copy, and their CJK fallback chain then lands on
                // the QPA default (serif) again.  A global QSS rule pins
                // the family on EVERY widget while leaving sizes/colors
                // untouched (no app-wide QSS existed before this).
                if (!fams.isEmpty())
                    qApp->setStyleSheet(QStringLiteral(
                        "QWidget { font-family: 'HarmonyOS Sans SC'; }"));
            }
            qWarning("[SciDAVis] font: addApplicationFont(%s) -> %d families=%s",
                     qPrintable(hms), fid,
                     qPrintable(QFontDatabase::applicationFontFamilies(
                                    qMax(fid, 0)).join(QLatin1Char(','))));
        }
#endif
        if (uiFont.pointSize() > 0)
            uiFont.setPointSize(uiFont.pointSize() + 3);
        app.setFont(uiFont);
        ApplicationWindow *mw = new ApplicationWindow;
        g_mainWindow = mw;
        // UAF fix (2026-08-16): arm the shutdown gate as soon as the main
        // window is gone or the event loop is leaving — from that point on
        // scidavis_call refuses to queue and queued lambdas skip execution
        // (crash 2026-08-15 22:39:33: windowsList() on destroyed window).
        QObject::connect(mw, &QObject::destroyed, []() {
            g_qtShuttingDown.store(true);
        });
        QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
            g_qtShuttingDown.store(true);
        });
        // MDI activation → ArkTS menu refresh.  QMdiArea::subWindowActivated
        // fires for every real active-window change: clicking a subwindow in
        // the workspace, the Windows→win:* menu entries (activateSubWindow →
        // setActiveSubWindow), and the F5/F6 next/prev-window actions.  This
        // single hook keeps the ArkTS menu in sync for all of them without
        // redundant emissions (the signal only fires when the active window
        // actually changes).
        QObject::connect(&mw->d_workspace, &QMdiArea::subWindowActivated, mw,
                         [](QMdiSubWindow *sub) { emitWindowActivated(sub); });
        // Clipboard bridge (Phase 2): mirror Qt-side clipboard changes to
        // ArkTS, which writes them into the OHOS system pasteboard.  P3
        // (2026-08-16): the 512KB cap was too small for real data tables
        // (large paste into the system pasteboard was silently truncated);
        // raised to 4MB with a warning so oversized payloads are still
        // logged, not silent.  The NAPI event channel handles one 4MB
        // UTF-16 event without issue (device-verified transfers).
        QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, mw, []() {
            QClipboard *cb = QApplication::clipboard();
            if (!cb)
                return;
            QString text = cb->text();
            if (text.isEmpty())
                return;
            const int kClipboardCap = 4 * 1024 * 1024;
            if (text.size() > kClipboardCap) {
                qWarning("[SciDAVis] clipboardChanged truncated %d -> %d chars",
                         text.size(), kClipboardCap);
                text.truncate(kClipboardCap);
            }
            QJsonObject p;
            p["text"] = text;
            scidavisEmitEvent(QStringLiteral("clipboardChanged"), p);
        });
        // P1 (2026-08-12, plan 0245 §4 B / O2): defer ALL post-construction
        // init to the first event-loop iteration so app.exec() starts
        // immediately.  Registered BEFORE the qtReady handshake below — the
        // ArkTS shell only queries after qtReady, so by then every
        // window/toolbar/statusbar step above is done and the first
        // getUiState finds Qt idle (before this, queries issued while this
        // block was still running timed out 300ms×3 at startup; plan 0245
        // §2.4 R1/R5).
        QTimer::singleShot(0, [mw, args]() {
            mw->applyUserSettings();
#ifdef SCIDAVIS_OHOS
            // Shortcut fix (2026-08-16): the alpha_v6 plugin never delivers a
            // window-activation event, so QApplication::activeWindow() stays
            // null and the QAction shortcut context matcher rejects every
            // match (Ctrl+Z/C/V/X dead).  See triggerActionShortcut below.
            // Sync the persisted language MODE to the ArkTS shell at startup
            // (three-mode language switch, 2026-08-14): the shell must start
            // in the same language Qt loaded from settings, not the device
            // default, or the two layers disagree until the user re-opens
            // Preferences.
            ohosNotifyLanguage(mw->appLanguage.toUtf8().constData());
#endif
            // OHOS: hide all native Qt toolbars — they render at desktop pixel
            // sizes through the XComponent (unusably small at DPR=1) and their
            // QToolButton tooltips/popups would create a second top-level window
            // (single-window QPA -> SIGSEGV).  The toolbar is rebuilt in ArkTS.
            for (QToolBar *tb : mw->findChildren<QToolBar *>())
                tb->hide();
            // OHOS: hide the native QMenuBar — the ArkTS MenuBar replaces it
            if (auto *mb = mw->menuBar())
                mb->hide();
#ifdef SCIDAVIS_OHOS
            // OHOS: hide all QDockWidgets — restoreState() may have made them visible.
            // The ArkTS shell provides its own panels (ProjectTree, ResultsLog).
            for (QDockWidget *dw : mw->findChildren<QDockWidget *>()) {
                // Issue-list #4 (2026-08-14): the dock title-bar "float" (全屏)
                // button turns the dock into an independent top-level window,
                // which the single-window QPA cannot create — the layout breaks
                // (double chrome + blank workspace on device).  Drop the
                // Floatable feature so the button no longer exists.
                dw->setFeatures(dw->features() & ~QDockWidget::DockWidgetFloatable);
                dw->hide();
            }
            // OHOS: hide the native QStatusBar — the ArkTS StatusBar replaces
            // it.  An unhidden QStatusBar shrinks the centralWidget (QMdiArea)
            // by its height (~39px), so the Qt viewport bottom inset lands
            // above the ArkTS BottomToolBar and exposes the QMdiArea frame's
            // white background (the bottom white strip on device).
            if (auto *sb = mw->statusBar())
                sb->hide();
#endif
            // OHOS (2026-08-13): the close-confirm dialogs ("Do you want to
            // hide or delete 'X' ?") are embedded and rendered by the bridge
            // (embedDialog, spec 13/14) — they were disabled previously only
            // because the single-window QPA could not show a QMessageBox.
            // Re-enable for desktop parity: closing a table/matrix/plot asks
            // for hide / delete / cancel.
            mw->confirmCloseTable = true;
            mw->confirmCloseMatrix = true;
            mw->confirmClosePlot2D = true;
            mw->confirmClosePlot3D = true;
            mw->confirmCloseFolder = true;
            mw->confirmCloseNotes = true;
            mw->newTable();
            mw->activateSubWindow();
            mw->savedProject();
#ifdef SEARCH_FOR_UPDATES
            if (mw->autoSearchUpdates) {
                mw->autoSearchUpdatesRequest = true;
                mw->searchForUpdates();
            }
#endif
            mw->parseCommandLineArguments(args);
        });
    }
    // P0 (2026-08-12): startup handshake.  Registered AFTER the O2 init
    // singleShot above, so it fires only once the event loop has run the
    // deferred init — the ArkTS shell may then issue its first getUiState
    // and always finds Qt idle.  Without the signal the shell waits its
    // full 60s fallback budget before polling (qtReadyTick stays 0), and
    // querying DURING init was what interlocked with the QPA callbacks.
    QTimer::singleShot(0, []() { scidavisEmitEvent(QStringLiteral("qtReady")); });
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    return app.exec();
}
