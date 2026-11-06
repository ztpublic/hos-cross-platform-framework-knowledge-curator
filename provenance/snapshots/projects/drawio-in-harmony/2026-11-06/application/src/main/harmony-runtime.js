// The paired HarmonyOS host launches secondary abilities using
// ATTACH_TO_STATUS_BAR_ITEM. Register its required status-bar item first.
import {fileURLToPath} from 'node:url';

let runtimeTray;
let openUrlHooked = false;
let shortcutBridgeInstalled = false;

export function initializeHarmonyRuntime(electron, iconPath)
{
	if (process.platform !== 'openharmony' || runtimeTray) return;
	const image = electron.nativeImage.createFromPath(iconPath).resize({width: 24, height: 24});
	if (image.isEmpty()) throw new Error('draw.io status-bar icon is missing');
	runtimeTray = new electron.Tray(image);
	runtimeTray.setToolTip('Draw.io in Harmony');
	runtimeTray.on('click', () => {
		const win = electron.BrowserWindow.getAllWindows().find(window => !window.isDestroyed());
		if (win) { win.show(); win.focus(); }
	});
	electron.app.once('will-quit', () => {
		if (runtimeTray && !runtimeTray.isDestroyed()) runtimeTray.destroy();
		runtimeTray = undefined;
	});
}

// 键盘快捷键：鸿蒙运行时把硬件键盘的 Ctrl/Alt 组合转给 Chromium 时丢了修饰键标志
// （Ctrl+A 到页面里成了普通的 a），但 Ctrl/Alt 键本身的按下/抬起事件照常送达。
// 这里按这些事件记录修饰键状态，拦下修饰键按住期间到达的普通按键，带上正确的
// 修饰键用 sendInputEvent 重新投递（重新投递的事件带修饰键标志，再次经过这里时直接放行）。
const shortcutKeyNames = {
	ArrowUp: 'Up', ArrowDown: 'Down', ArrowLeft: 'Left', ArrowRight: 'Right',
	Enter: 'Enter', NumpadEnter: 'Enter', Escape: 'Escape', Tab: 'Tab', Space: 'Space',
	Backspace: 'Backspace', Delete: 'Delete', Insert: 'Insert', Home: 'Home', End: 'End',
	PageUp: 'PageUp', PageDown: 'PageDown', Minus: '-', Equal: '=', BracketLeft: '[',
	BracketRight: ']', Backslash: '\\', Semicolon: ';', Quote: '\'', Slash: '/', Comma: ',',
	Period: '.', Backquote: '`', NumpadAdd: 'numadd', NumpadSubtract: 'numsub'
};

function shortcutKeyName(code)
{
	const match = /^(?:Key([A-Z])|Digit([0-9])|Numpad([0-9])|(F[0-9]{1,2}))$/.exec(code || '');
	if (match) return match[1] || match[2] || (match[3] && 'num' + match[3]) || match[4];
	return shortcutKeyNames[code];
}

export function installHarmonyShortcutBridge(electron)
{
	if (process.platform !== 'openharmony' || shortcutBridgeInstalled) return;
	shortcutBridgeInstalled = true;
	const states = new WeakMap();
	const reset = state => { state.Control = state.Alt = state.Shift = false; state.replayed.clear(); };
	electron.app.on('web-contents-created', (e, contents) =>
	{
		const state = {Control: false, Alt: false, Shift: false, replayed: new Map()};
		states.set(contents, state);
		contents.on('before-input-event', (event, input) =>
		{
			if (input.key === 'Control' || input.key === 'Alt' || input.key === 'Shift')
			{
				state[input.key] = input.type === 'keyDown';
				return;
			}
			if (input.type === 'keyUp')
			{
				const replay = state.replayed.get(input.code);
				if (replay != null)
				{
					state.replayed.delete(input.code);
					event.preventDefault();
					contents.sendInputEvent({type: 'keyUp', keyCode: replay.keyCode, modifiers: replay.modifiers});
				}
				return;
			}
			if (input.type !== 'keyDown' || input.control || input.alt || input.meta ||
				(!state.Control && !state.Alt))
			{
				return;
			}
			const keyCode = shortcutKeyName(input.code);
			if (keyCode == null) return;
			const modifiers = [];
			if (state.Control) modifiers.push('control');
			if (state.Alt) modifiers.push('alt');
			if (state.Shift || input.shift) modifiers.push('shift');
			event.preventDefault();
			state.replayed.set(input.code, {keyCode, modifiers});
			contents.sendInputEvent({type: 'keyDown', keyCode, modifiers});
		});
	});
	// A modifier released in another window never reaches this one.
	electron.app.on('browser-window-blur', (e, win) =>
	{
		const state = !win.isDestroyed() ? states.get(win.webContents) : null;
		if (state != null) reset(state);
	});
}

// 文件关联：宿主在应用已运行时把 file:// 的 Want 转成 open-url 事件。
// 转成与桌面版“第二实例带文件参数”相同的 second-instance 流程，由主进程授权并在新窗口打开。
export function hookHarmonyOpenUrl(app)
{
	if (process.platform !== 'openharmony' || openUrlHooked) return;
	openUrlHooked = true;
	app.on('open-url', (event, url) => {
		if (typeof url !== 'string' || !url.startsWith('file://')) return;
		event.preventDefault();
		let file;
		try { file = fileURLToPath(url); } catch (e) { return; }
		app.emit('second-instance', {}, [file], process.cwd());
	});
}
