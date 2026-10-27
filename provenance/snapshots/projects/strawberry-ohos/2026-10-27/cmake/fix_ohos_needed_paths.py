#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Rewrite full-build-path Qt6 DT_NEEDED entries in an OHOS .so to basenames.

The Qt6 build-tree .so (qtbase-dev-harmonyos-build, the junction target of the
installed Qt6.12 kit) are produced without a SONAME. When libstrawberry.so links
against them the linker records the absolute build path (e.g.
``C:/code/Qt6-ohos/qtbase-dev-harmonyos-build/libQt6Core.so``) verbatim as a
DT_NEEDED entry. The OHOS musl loader cannot resolve a Windows path on device,
so ``dlopen("libstrawberry.so")`` fails at startup -> the app crashes on launch
(see cppcrash dump: "Error loading shared library C:/.../libQt6Core.so").

This script rewrites those entries in place to the basename
(``libQt6Core.so``), keeping the same byte length (null-padded) so .dynstr
offsets do not shift. Idempotent: a second run is a no-op (basenames carry no
drive prefix). Only touches drive-prefixed ``libQt6*.so`` NEEDED strings; leaves
everything else (soname'd deps like libglib-2.0.so.0, RPATH, etc.) untouched.
"""
import os
import re
import sys

# A drive-prefixed path whose last segment is libQt6<Name>.so, terminated by NUL.
# [^\x00]*? cannot cross a NUL, so each match stays within one .dynstr entry.
_NEED = re.compile(rb'[A-Za-z]:[\\/][^\x00]*?libQt6[A-Za-z0-9]+\.so\x00')


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: fix_ohos_needed_paths.py <elf.so>", file=sys.stderr)
        return 2
    path = sys.argv[1]
    if not os.path.isfile(path):
        print(f"[fix_ohos_needed] not a file: {path}", file=sys.stderr)
        return 2

    data = open(path, "rb").read()
    count = 0

    def repl(match: "re.Match[bytes]") -> bytes:
        nonlocal count
        count += 1
        full = match.group(0)                                  # C:/.../libQt6Core.so\0
        base = re.split(rb"[\\/]", full)[-1]                  # libQt6Core.so\0
        return base + b"\x00" * (len(full) - len(base))        # same length, NUL-padded

    new = _NEED.sub(repl, data)
    if count:
        open(path, "wb").write(new)
    print(f"[fix_ohos_needed] rewrote {count} full-path Qt6 DT_NEEDED(s) -> "
          f"basenames in {os.path.basename(path)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
