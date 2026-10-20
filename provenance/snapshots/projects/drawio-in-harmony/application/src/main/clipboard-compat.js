// Keep draw.io's clipboard operations compatible with both Electron 37 and 44.
// Electron is injected so importing this module never requires ClipboardItem
// to exist as a named export on older runtimes.
export function createClipboardAction(electron)
{
	return async function clipboardAction(method, data)
	{
		const {clipboard, nativeImage, ClipboardItem} = electron;

		if (method === 'writeText')
		{
			return clipboard.writeText(data);
		}
		else if (method === 'readText')
		{
			return clipboard.readText();
		}
		else if (method === 'writeImage')
		{
			const image = nativeImage.createFromDataURL(data.dataUrl);
			const html = '<img src="' + data.dataUrl + '" width="' + data.w +
				'" height="' + data.h + '">';

			if (typeof ClipboardItem === 'function')
			{
				return clipboard.write([new ClipboardItem({
					'image/png': new Blob([image.toPNG()], {type: 'image/png'}),
					'text/html': html
				})]);
			}

			return clipboard.write({image, html});
		}
	};
}
