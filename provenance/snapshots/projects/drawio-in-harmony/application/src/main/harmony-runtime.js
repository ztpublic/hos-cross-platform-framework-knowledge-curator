// The paired HarmonyOS host launches secondary abilities using
// ATTACH_TO_STATUS_BAR_ITEM. Register its required status-bar item first.
import {fileURLToPath} from 'node:url';

let runtimeTray;
let openUrlHooked = false;

export function initializeHarmonyRuntime(electron, iconPath)
{
	if (process.platform !== 'openharmony' || runtimeTray) return;
	const image = electron.nativeImage.createFromPath(iconPath).resize({width: 24, height: 24});
	if (image.isEmpty()) throw new Error('draw.io status-bar icon is missing');
	runtimeTray = new electron.Tray(image);
	runtimeTray.setToolTip('draw.io');
	runtimeTray.on('click', () => {
		const win = electron.BrowserWindow.getAllWindows().find(window => !window.isDestroyed());
		if (win) { win.show(); win.focus(); }
	});
	electron.app.once('will-quit', () => {
		if (runtimeTray && !runtimeTray.isDestroyed()) runtimeTray.destroy();
		runtimeTray = undefined;
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
