// OpenHarmony 没有 fc-list：直接读取系统字体目录，从 sfnt 'name' 表取字体族名。
import fs from 'node:fs';
import path from 'node:path';

const FONT_DIRS = ['/system/fonts', '/data/fonts'];
const EXT = /\.(ttf|otf|ttc|otc)$/i;

function readAt(fd, position, length)
{
	const buf = Buffer.alloc(length);
	const n = fs.readSync(fd, buf, 0, length, position);
	return n === length ? buf : buf.subarray(0, n);
}

function decodeName(buf, platform)
{
	if (platform === 3 || platform === 0)
	{
		const swapped = Buffer.from(buf);
		swapped.swap16();
		return swapped.toString('utf16le');
	}
	return buf.toString('latin1');
}

// 返回一个 sfnt 字体（偏移 base）的族名；优先英文，其次任意 Windows/Unicode 名称
function familyOfFont(fd, base)
{
	const head = readAt(fd, base, 12);
	if (head.length < 12) return null;
	const numTables = head.readUInt16BE(4);
	const dir = readAt(fd, base + 12, numTables * 16);
	let nameOffset = -1, nameLength = 0;
	for (let i = 0; i + 16 <= dir.length; i += 16)
	{
		if (dir.toString('latin1', i, i + 4) === 'name')
		{
			nameOffset = dir.readUInt32BE(i + 8);
			nameLength = dir.readUInt32BE(i + 12);
			break;
		}
	}
	if (nameOffset < 0 || nameLength < 6 || nameLength > 4 * 1024 * 1024) return null;
	const table = readAt(fd, nameOffset, nameLength);
	const count = table.readUInt16BE(2);
	const strings = table.readUInt16BE(4);
	const best = {};
	for (let i = 0; i < count; i++)
	{
		const r = 6 + i * 12;
		if (r + 12 > table.length) break;
		const platform = table.readUInt16BE(r);
		const language = table.readUInt16BE(r + 4);
		const nameId = table.readUInt16BE(r + 6);
		if (nameId !== 1 && nameId !== 16) continue;
		const length = table.readUInt16BE(r + 8);
		const offset = table.readUInt16BE(r + 10);
		const raw = table.subarray(strings + offset, strings + offset + length);
		const text = decodeName(raw, platform).trim();
		if (!text) continue;
		const english = (platform === 3 && language === 0x409) || (platform === 1 && language === 0);
		const rank = (nameId === 16 ? 2 : 0) + (english ? 4 : 0) + (platform === 3 ? 1 : 0);
		if (best.rank == null || rank > best.rank) { best.rank = rank; best.text = text; }
	}
	return best.text || null;
}

export function readFontFamilies(file)
{
	const fd = fs.openSync(file, 'r');
	try
	{
		const tag = readAt(fd, 0, 4).toString('latin1');
		const families = [];
		if (tag === 'ttcf')
		{
			const header = readAt(fd, 0, 12);
			const num = Math.min(header.readUInt32BE(8), 64);
			const offsets = readAt(fd, 12, num * 4);
			for (let i = 0; i + 4 <= offsets.length; i += 4)
			{
				const f = familyOfFont(fd, offsets.readUInt32BE(i));
				if (f) families.push(f);
			}
		}
		else
		{
			const f = familyOfFont(fd, 0);
			if (f) families.push(f);
		}
		return families;
	}
	finally
	{
		fs.closeSync(fd);
	}
}

export function listOpenHarmonyFonts(dirs = FONT_DIRS)
{
	const out = new Set();
	for (const dir of dirs)
	{
		let names;
		try { names = fs.readdirSync(dir); } catch (e) { continue; }
		for (const name of names)
		{
			if (!EXT.test(name)) continue;
			try { for (const f of readFontFamilies(path.join(dir, name))) out.add(f); }
			catch (e) { /* 跳过损坏字体 */ }
		}
	}
	return [...out].sort((a, b) => a.localeCompare(b));
}
