#!/usr/bin/env python3
"""Patch DT_NEEDED entries in .so files: replace absolute/relative paths with bare .so names."""
import struct
import sys
import os

def read_elf_dynamic(filename):
    with open(filename, 'rb') as f:
        data = f.read()
    # Find .dynamic section
    # Simple approach: scan for DT_NEEDED (tag=1) and DT_SONAME (tag=14)
    # ELF header
    ei_class = data[4]  # 1=32-bit, 2=64-bit
    if ei_class == 2:
        # 64-bit
        e_shoff = struct.unpack_from('<Q', data, 40)[0]
        e_shentsize = struct.unpack_from('<H', data, 58)[0]
        e_shnum = struct.unpack_from('<H', data, 60)[0]
        e_shstrndx = struct.unpack_from('<H', data, 62)[0]
    else:
        print(f"32-bit not supported")
        return

    # Find .dynamic and .dynstr sections
    dyn_offset = None
    dyn_size = None
    dynstr_offset = None
    dynstr_size = None

    for i in range(e_shnum):
        sh_off = e_shoff + i * e_shentsize
        sh_type = struct.unpack_from('<I', data, sh_off + 4)[0]
        sh_offset = struct.unpack_from('<Q', data, sh_off + 24)[0]
        sh_size = struct.unpack_from('<Q', data, sh_off + 32)[0]
        if sh_type == 6:  # SHT_DYNAMIC
            dyn_offset = sh_offset
            dyn_size = sh_size
        elif sh_type == 3:  # SHT_STRTAB
            # This might be .dynstr
            if dynstr_offset is None:
                dynstr_offset = sh_offset
                dynstr_size = sh_size

    if dyn_offset is None or dynstr_offset is None:
        print(f"No .dynamic or .dynstr found")
        return

    # Parse dynamic entries (64-bit: 16 bytes each, d_tag + d_val)
    entries = []
    i = 0
    while i < dyn_size:
        d_tag = struct.unpack_from('<q', data, dyn_offset + i)[0]
        d_val = struct.unpack_from('<Q', data, dyn_offset + i + 8)[0]
        entries.append((d_tag, d_val))
        if d_tag == 0:  # DT_NULL
            break
        i += 16

    # Collect NEEDED and SONAME entries
    needed_entries = []
    soname_entry = None
    for idx, (tag, val) in enumerate(entries):
        if tag == 1:  # DT_NEEDED
            # Read string from dynstr
            str_start = dynstr_offset + val
            str_end = data.index(b'\x00', str_start)
            s = data[str_start:str_end].decode('utf-8', errors='replace')
            needed_entries.append((idx, val, s))
        elif tag == 14:  # DT_SONAME
            str_start = dynstr_offset + val
            str_end = data.index(b'\x00', str_start)
            s = data[str_start:str_end].decode('utf-8', errors='replace')
            soname_entry = (idx, val, s)

    return data, dyn_offset, dynstr_offset, entries, needed_entries, soname_entry

def patch_needed_inplace(filename):
    result = read_elf_dynamic(filename)
    if result is None:
        return False
    data, dyn_offset, dynstr_offset, entries, needed_entries, soname_entry = result

    changes = []

    # Also patch NEEDED entries that reference a .so.N version when the actual
    # file in the HAP is .so (no version suffix). e.g. libcoreLetos.so.1 -> libcoreLetos.so
    # Build a set of "known bare .so names" from the files in the same directory
    import re
    own_name = os.path.basename(filename)

    for idx, str_offset, s in needed_entries:
        # Check if it contains a path separator
        if '/' in s or '\\' in s:
            bare = os.path.basename(s.replace('\\', '/'))
            changes.append(('NEEDED', s, bare, str_offset))
        # Check for versioned .so.1, .so.1.0.0 etc that won't exist as bare .so
        m = re.match(r'^(lib\S+\.so)\.\d+', s)
        if m and '/' not in s:
            bare = m.group(1)
            changes.append(('NEEDED', s, bare, str_offset))

    if soname_entry:
        idx, str_offset, s = soname_entry
        # Strip version suffix from SONAME too
        m = re.match(r'^(lib\S+\.so)\.\d+', s)
        if '/' in s or '\\' in s:
            bare = os.path.basename(s.replace('\\', '/'))
            changes.append(('SONAME', s, bare, str_offset))
        elif m:
            bare = m.group(1)
            changes.append(('SONAME', s, bare, str_offset))

    if not changes:
        return False

    print(f"  {os.path.basename(filename)}:")
    for typ, old, new, off in changes:
        print(f"    {typ}: {old} -> {new}")
        # Patch the string in dynstr
        old_bytes = old.encode('utf-8')
        new_bytes = new.encode('utf-8')
        # Pad with nulls
        if len(new_bytes) > len(old_bytes):
            print(f"    ERROR: new name longer than old, skipping")
            continue
        padded = new_bytes + b'\x00' * (len(old_bytes) - len(new_bytes) + 1)
        data = bytearray(data)
        str_pos = dynstr_offset + off
        data[str_pos:str_pos + len(padded)] = padded
        data = bytes(data)

    with open(filename, 'wb') as f:
        f.write(data)
    return True

if __name__ == '__main__':
    files = sys.argv[1:]
    patched = 0
    for f in files:
        if os.path.exists(f):
            if patch_needed_inplace(f):
                patched += 1
    print(f"Patched {patched}/{len(files)} files")
