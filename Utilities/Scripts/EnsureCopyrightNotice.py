#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.
"""
Safely insert or ensure a single-line triple-slash copyright header in C/C++ files.

Default header:
/// Copyright (c) Theophilus Eriata. All Rights Reserved.

Usage:
  python EnsureCopyrightNotice.py --root . --dry-run
  python EnsureCopyrightNotice.py --root src
"""
from pathlib import Path
import argparse
import shutil
import re

CPP_EXTS = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.inl'}

DEFAULT_HEADER_LINE = "/// Copyright (c) Theophilus Eriata. All Rights Reserved."
HEADER_TRAILING_BLANKS = "\n\n"

COPYRIGHT_KEYWORDS_RE = re.compile(r'Copyright|©|\(c\)|All rights reserved', re.I)

def parse_args():
    p = argparse.ArgumentParser(description="Safely add/ensure triple-slash copyright header.")
    p.add_argument('--root', default='.', help='Root directory to scan')
    p.add_argument('--header', help='Exact header line to insert (no trailing blank lines required)')
    p.add_argument('--dry-run', action='store_true', help='Show changes without writing files')
    p.add_argument('--no-backup', action='store_true', help='Do not create .bak backups')
    p.add_argument('--extensions', nargs='*', help='Extra extensions to include (e.g. .inl)')
    return p.parse_args()

def canonical_header(header_line: str) -> str:
    # Ensure single header line followed by a newline
    h = header_line.rstrip("\n")
    return h + HEADER_TRAILING_BLANKS

def read_lines(path: Path):
    return path.read_text(encoding='utf-8', errors='surrogateescape').splitlines(keepends=True)

def write_lines(path: Path, lines, backup=True):
    if backup:
        bak = path.with_suffix(path.suffix + ".bak")
        shutil.copy2(path, bak)
    path.write_text(''.join(lines), encoding='utf-8', errors='surrogateescape')

def find_leading_block(lines):
    """
    Return (start_idx, end_idx, block_type)
    - start_idx inclusive, end_idx exclusive
    - block_type: 'shebang', 'line_comments', 'block_comment', or None
    This finds the first contiguous leading region consisting of:
      optional shebang (line 0),
      followed by optional blank lines,
      followed by either a block comment (/*...*/) or consecutive line comments (//...).
    If nothing found, returns (0,0,None) meaning no leading comment block.
    """
    i = 0
    n = len(lines)
    # shebang
    if i < n and lines[i].startswith("#!"):
        i += 1
    # skip leading blank lines after shebang
    while i < n and lines[i].strip() == "":
        i += 1
    # now check for block comment
    if i < n and lines[i].lstrip().startswith("/*"):
        start = i
        # find end of block comment
        while i < n:
            if "*/" in lines[i]:
                i += 1
                break
            i += 1
        return start, i, 'block_comment'
    # check for consecutive line comments (// or ///)
    if i < n and lines[i].lstrip().startswith("//"):
        start = i
        while i < n and lines[i].lstrip().startswith("//"):
            i += 1
        return start, i, 'line_comments'
    return i, i, None

def block_contains_copyright(lines, start, end):
    block_text = ''.join(lines[start:end])
    return bool(COPYRIGHT_KEYWORDS_RE.search(block_text))

def process_file(path: Path, header_line: str, dry_run: bool, backup: bool):
    lines = read_lines(path)
    if not lines:
        # empty file: just write header
        new_text = canonical_header(header_line)
        if dry_run:
            return True, "would-update-empty"
        if backup:
            shutil.copy2(path, path.with_suffix(path.suffix + ".bak"))
        path.write_text(new_text, encoding='utf-8', errors='surrogateescape')
        return True, "updated-empty"

    # find leading region
    # keep shebang if present at top; find where to insert/replace
    # We will treat shebang specially: if present, insertion point is after shebang and any immediate blank lines.
    # find_leading_block returns the first comment block start/end (or same index if none)
    # but we need to preserve shebang if present at index 0
    # So detect shebang separately
    idx = 0
    n = len(lines)
    shebang_present = False
    if lines[0].startswith("#!"):
        shebang_present = True
        idx = 1
        # skip blank lines after shebang
        while idx < n and lines[idx].strip() == "":
            idx += 1

    # Now check for a leading comment block starting at idx
    start, end, block_type = find_leading_block(lines[idx:]) if idx < n else (0,0,None)
    # find_leading_block returned indices relative to idx; convert to absolute
    if block_type is not None:
        start += idx
        end += idx
    else:
        start = end = idx

    # Decide action:
    # - If there is a comment block and it contains copyright -> replace that block with header
    # - If there is a comment block and it does NOT contain copyright -> insert header after that block
    # - If no comment block -> insert header at idx (after shebang if present)
    header_full = canonical_header(header_line)
    if start < end and block_contains_copyright(lines, start, end):
        # Replace the entire block [start:end] with header
        new_lines = lines[:start] + [header_full] + lines[end:]
        # If the existing block already equals header (ignoring trailing whitespace), treat as unchanged
        existing_block = ''.join(lines[start:end]).strip()
        if existing_block == header_full.strip():
            return False, "unchanged"
        if dry_run:
            return True, "would-replace-header"
        write_lines(path, new_lines, backup=backup)
        return True, "replaced-header"
    else:
        # Insert header after the block (or at idx if no block)
        insert_at = end  # end is idx if no block
        # Avoid inserting duplicate header if header already present at insert_at
        # Check next non-blank line(s) for header_line
        lookahead = ''.join(lines[insert_at:insert_at+3])  # small window
        if header_line in lookahead:
            return False, "unchanged"
        new_lines = lines[:insert_at] + [header_full] + lines[insert_at:]
        if dry_run:
            return True, "would-insert-header"
        write_lines(path, new_lines, backup=backup)
        return True, "inserted-header"

def main():
    args = parse_args()
    root = Path(args.root)
    exts = set(CPP_EXTS)
    if args.extensions:
        exts.update(args.extensions)

    header_line = args.header if args.header is not None else DEFAULT_HEADER_LINE

    changed = []
    for p in root.rglob('*'):
        if not p.is_file():
            continue
        if p.suffix.lower() not in exts:
            continue
        try:
            ok, status = process_file(p, header_line, args.dry_run, not args.no_backup)
            if ok:
                changed.append((str(p), status))
        except Exception as e:
            changed.append((str(p), f"error: {e}"))

    for f, s in changed:
        print(f"{s}: {f}")
    if not changed:
        print("No files changed.")

if __name__ == "__main__":
    main()
