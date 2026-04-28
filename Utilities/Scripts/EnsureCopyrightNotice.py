#!/usr/bin/env python3
"""
Add or ensure a copyright header in C/C++ source files.

Usage:
  python EnsureCopyrightNotice.py --root . --license-file LICENSE.header --owner "XII Technologies" --year 2026
  python EnsureCopyrightNotice.py --root Source --license "Copyright (c) $YEAR $OWNER" --dry-run
"""
import re
import shutil
import pathlib
import argparse

from datetime import datetime

CPP_EXTS = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}

DEFAULT_TEMPLATE = """Copyright (c) $YEAR $OWNER. All rights reserved.
"""

C_STYLE_BLOCK = ("/*", "*/")
LINE_PREFIX = "// "

def load_template(args):
    if args.license_file:
        return pathlib.Path(args.license_file).read_text(encoding='utf-8')
    if args.license:
        return args.license
    return DEFAULT_TEMPLATE

def render_template(tpl, year, owner):
    return tpl.replace("$YEAR", str(year)).replace("$OWNER", owner)

def make_block_comment(text):
    lines = text.strip().splitlines()
    body = "\n".join(" * " + l.rstrip() for l in lines)
    return f"/*\n{body}\n */\n\n"

def make_line_comment(text):
    lines = text.strip().splitlines()
    return "".join(LINE_PREFIX + l.rstrip() + "\n" for l in lines) + "\n"

def detect_existing_header(content):
    # Detect common header markers: Copyright, (c), All rights reserved
    header_re = re.compile(r'(?s)\A\s*(/\*.*?\*/\s*|(?://.*\n)+)')
    m = header_re.match(content)
    if not m:
        return None, content
    block = m.group(0)
    if re.search(r'Copyright|©|\(c\)|All rights reserved', block, re.I):
        return block, content[m.end():]
    return None, content

def process_file(path, new_header, args):
    text = path.read_text(encoding='utf-8')
    existing, rest = detect_existing_header(text)
    if existing:
        if existing.strip() == new_header.strip():
            return False, "unchanged"
        updated = new_header + rest
    else:
        updated = new_header + text
    if args.dry_run:
        return True, "would-update"
    # backup
    bak = path.with_suffix(path.suffix + ".bak")
    shutil.copy2(path, bak)
    path.write_text(updated, encoding='utf-8')
    return True, "updated"

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', default='.', help='Root directory to scan')
    p.add_argument('--license-file', help='File containing license template')
    p.add_argument('--license', help='Inline license template (use $YEAR and $OWNER)')
    p.add_argument('--owner', default='Your Organization', help='Copyright owner')
    p.add_argument('--year', type=int, default=datetime.now().year, help='Year to use')
    p.add_argument('--style', choices=['block','line'], default='block', help='Comment style')
    p.add_argument('--dry-run', action='store_true', help='Show changes without writing files')
    p.add_argument('--extensions', nargs='*', help='Extra extensions to include (e.g. .inl)')
    args = p.parse_args()

    tpl = load_template(args)
    rendered = render_template(tpl, args.year, args.owner)
    if args.style == 'block':
        header = make_block_comment(rendered)
    else:
        header = make_line_comment(rendered)

    root = pathlib.Path(args.root)
    exts = set(CPP_EXTS)
    if args.extensions:
        exts.update(args.extensions)

    changed = []
    for path in root.rglob('*'):
        if path.is_file() and path.suffix in exts:
            ok, status = process_file(path, header, args)
            if ok:
                changed.append((str(path), status))

    for f, s in changed:
        print(f"{s}: {f}")
    if not changed:
        print("No files changed.")

if __name__ == '__main__':
    main()
