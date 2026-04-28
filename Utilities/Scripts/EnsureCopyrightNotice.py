#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.
"""
EnsureCopyrightHeader.py

Scan a directory for C/C++ source/header files and insert or replace a single-line triple-slash copyright header.

Default header:
/// Copyright (c) Theophilus Eriata. All Rights Reserved.

Usage:
  python EnsureCopyrightHeader.py --root . --dry-run
  python EnsureCopyrightHeader.py --root src --header "/// Copyright (c) Someone Else. All Rights Reserved."
"""
from pathlib import Path
import argparse
import shutil
import re

# Default file extensions to scan.
CPP_EXTS: set[str]  = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.inl'}
DEFAULT_HEADER: str = "/// Copyright (c) Theophilus Eriata. All Rights Reserved.\n\n"

# Regex to detect an existing header block at the top of the file.
# Matches either:
#  - One or more lines starting with "///" or "//",
#  - a C-style block comment /* ... */.
HEADER_DETECT_RE = re.compile(
    r'(?s)\A\s*(?:(?P<triple>(?:///{1,}.*\n)+)|(?P<line>(?://.*\n)+)|(?P<block>/\*.*?\*/\s*\n*))'
)

COPYRIGHT_KEYWORDS_RE = re.compile(r'Copyright|©|\(c\)|All rights reserved', re.I)

def load_args():
    p = argparse.ArgumentParser(description="Add or ensure a triple-slash copyright header in C/C++ files.")
    p.add_argument('--root', default='.', help='Root directory to scan')
    p.add_argument('--header', help='Exact header text to insert (include trailing newline if desired)')
    p.add_argument('--dry-run', action='store_true', help='Show changes without writing files')
    p.add_argument('--backup', action='store_true', help='Create .bak backups before writing (default: True)', default=True)
    p.add_argument('--extensions', nargs='*', help='Extra extensions to include (e.g. .inl)')
    return p.parse_args()

def canonicalize_header(sHeader: str) -> str:
    # Ensure header ends with two newlines (header + blank line).
    if not sHeader.endswith("\n"):
        sHeader = sHeader + "\n"
    if not sHeader.endswith("\n\n"):
        sHeader = sHeader + "\n"
    return sHeader

def detect_existing_header(sContent: str):
    m = HEADER_DETECT_RE.match(sContent)
    if not m:
        return None, sContent
    block = m.group(0)
    # Only treat it as a header to replace if it contains copyright-like keywords.
    if COPYRIGHT_KEYWORDS_RE.search(block):
        sRemainingText: str = sContent[m.end():]
        return block, sRemainingText
    return None, sContent

def process_file(path: Path, sNewHeader: str, bIsDryRun: bool, bBackup: bool):
    text: str                = path.read_text(encoding='utf-8', errors='surrogateescape')
    existing, sRemainingText = detect_existing_header(text)
    if existing:
        if existing.strip() == sNewHeader.strip():
            return False, "unchanged"
        sUpdatedText = sNewHeader + sRemainingText
    else:
        sUpdatedText = sNewHeader + text
    if bIsDryRun:
        return True, "would-update"
    if bBackup:
        bak: Path = path.with_suffix(path.suffix + ".bak")
        shutil.copy2(path, bak)
    path.write_text(sUpdatedText, encoding='utf-8', errors='surrogateescape')
    return True, "updated"

def main():
    args = load_args()
    root = Path(args.root)
    exts = set(CPP_EXTS)
    if args.extensions:
        exts.update(args.extensions)

    header_text = args.header if args.header is not None else DEFAULT_HEADER
    header      = canonicalize_header(header_text)

    changed = []
    for p in root.rglob('*'):
        if not p.is_file():
            continue
        if p.suffix.lower() not in exts:
            continue
        ok, status = process_file(p, header, args.dry_run, args.backup)
        if ok:
            changed.append((str(p), status))

    for f, s in changed:
        print(f"{s}: {f}")
    if not changed:
        print("No files changed.")

if __name__ == "__main__":
    main()
