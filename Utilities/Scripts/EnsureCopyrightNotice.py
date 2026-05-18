#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.
"""
Ensure a single-line copyright header is the absolute first line of each file,
followed by exactly one blank line, and that the file ends with exactly one newline.
Preserves original file encoding and BOM.

Behavior:
- Chooses comment prefix by file type:
    * C/C++ headers/sources (.c .cpp .cc .cxx .h .hpp .hh .hxx .inl) -> "///"
    * CMake files (*.cmake and CMakeLists.txt) -> "#"
    * Other extensions can be added via --map option
- Removes exact duplicate header lines (matching the full header string).
- Writes a .bak copy of the original bytes before modifying.
- Detects and preserves encoding (uses charset-normalizer or chardet if available).
- Supports --only to restrict processing to a subset of types (e.g., --only cmake).
- Use --dry-run to preview changes.
- Supports --files to process an explicit list of files and --omit-dir to skip directories.

Usage examples:
  python EnsureCopyrightNotice.py --root . --dry-run
  python EnsureCopyrightNotice.py --files src/foo.cpp include/bar.h --omit-dir ThirdParty --no-backup
  python EnsureCopyrightNotice.py --root . --extensions .cmake --body "Copyright (c) Theophilus Eriata. All Rights Reserved." --omit-dir thirdparty external

Requires (recommended):
  pip install charset-normalizer
"""
import sys
import shutil
import argparse
from pathlib import Path

# Optional encoding detection libraries
try:
    from charset_normalizer import from_bytes as cn_from_bytes  # type: ignore
    _HAS_CN = True
except Exception:
    _HAS_CN = False

try:
    import chardet  # type: ignore
    _HAS_CHARD = True
except Exception:
    _HAS_CHARD = False

print(f"Using charset-normalizer: {_HAS_CN}, chardet: {_HAS_CHARD}", file=sys.stdout)

# Default file type maps
DEFAULT_TYPE_MAP = {
    "cpp": {
        "extensions": ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.inl'],
        "prefix": "///"
    },
    "cmake": {
        "extensions": ['.cmake'],
        "filenames": ['CMakeLists.txt'],
        "prefix": "#"
    }
}

DEFAULT_BODY = "Copyright (c) Theophilus Eriata. All Rights Reserved."

# -------------------------
# Argument parsing
# -------------------------
def parse_args():
    p = argparse.ArgumentParser(description="Force a single-line header as the first line, per-file-type, preserving encoding.")
    p.add_argument('--root', default='.', help='Root directory to scan (ignored if --files is used)')
    p.add_argument('--files', nargs='*', help='Explicit list of files to process (overrides --root)')
    p.add_argument('--omit-dir', nargs='*', default=[], help='Directory names or relative paths to omit (case-insensitive).')
    p.add_argument('--body', help='Header body text (without comment prefix). Default is Theophilus header.')
    p.add_argument('--header', help='Full header line to insert (overrides body and prefix)')
    p.add_argument('--dry-run', action='store_true', help='Show changes without writing files')
    p.add_argument('--no-backup', action='store_true', help='Do not create .bak backups')
    p.add_argument('--extensions', nargs='*', help='Extra extensions to include (e.g. .txt) — applied to all types')
    p.add_argument('--only', nargs='*', help='Restrict to named types (e.g., cpp cmake). Default: all known types')
    p.add_argument('--fallback-encoding', default='utf-8', help='Encoding to use if detection fails (default: utf-8)')
    return p.parse_args()

# -------------------------
# Encoding helpers
# -------------------------
def detect_encoding_from_bytes(b: bytes):
    if b.startswith(b'\xef\xbb\xbf'):
        return 'utf-8-sig', True, 1.0
    if _HAS_CN:
        try:
            results = cn_from_bytes(b)
            best = results.best()
            if best:
                enc = best.encoding
                conf = float(best.fingerprint_confidence or 0.0)
                return enc, False, conf
        except Exception:
            pass
    if _HAS_CHARD:
        try:
            r = chardet.detect(b)
            enc = r.get('encoding')
            conf = float(r.get('confidence') or 0.0)
            return enc, False, conf
        except Exception:
            pass
    return None, False, 0.0

def read_file_preserve_encoding(path: Path, fallback_encoding: str):
    b = path.read_bytes()
    enc, had_bom, conf = detect_encoding_from_bytes(b)
    if enc:
        enc_norm = enc.lower().replace('_', '-')
        try:
            if enc_norm in ('utf-8-sig', 'utf8-sig'):
                text = b.decode('utf-8-sig', errors='strict')
                return text, 'utf-8-sig', True, b
            text = b.decode(enc, errors='strict')
            return text, enc, had_bom, b
        except Exception:
            pass
    try:
        text = b.decode('utf-8', errors='strict')
        return text, 'utf-8', False, b
    except Exception:
        try:
            text = b.decode(fallback_encoding, errors='surrogateescape')
            return text, fallback_encoding, False, b
        except Exception:
            text = b.decode('latin-1', errors='strict')
            return text, 'latin-1', False, b

def write_file_preserve_encoding(path: Path, text: str, encoding: str, had_bom: bool):
    enc_norm = encoding.lower().replace('_', '-')
    if enc_norm in ('utf-8-sig',):
        out_bytes = text.encode('utf-8-sig')
    else:
        if had_bom and enc_norm in ('utf-8', 'utf8'):
            out_bytes = text.encode('utf-8-sig')
        else:
            out_bytes = text.encode(encoding, errors='surrogateescape')
    path.write_bytes(out_bytes)

# -------------------------
# Type map and helpers
# -------------------------
def build_type_map(extra_exts):
    # Merge defaults and add any extra extensions to all types.
    tm = {}
    for name, info in DEFAULT_TYPE_MAP.items():
        exts = list(info.get('extensions', []))
        if extra_exts:
            exts.extend(extra_exts)
        filenames = list(info.get('filenames', []))
        tm[name] = {
            "extensions": set(e.lower() for e in exts),
            "filenames": set(filenames),
            "prefix": info['prefix']
        }
    return tm

def file_type_for_path(path: Path, type_map):
    name = path.name
    suffix = path.suffix.lower()
    for tname, info in type_map.items():
        if name in info.get('filenames', set()):
            return tname
        if suffix in info.get('extensions', set()):
            return tname
    return None

def make_full_header(prefix: str, body: str):
    # Single-line header (no trailing spaces), e.g. "/// Copyright..."
    return f"{prefix} {body}".rstrip()

# -------------------------
# Header enforcement
# -------------------------
def ensure_header_text(original_text: str, header_line: str) -> (str, bool):
    """
    - Remove all lines that exactly equal header_line (ignoring trailing spaces).
    - Strip leading blank lines from the remaining content.
    - Prepend header_line + one blank line.
    - Ensure the file ends with exactly one newline character.
    """
    if original_text is None:
        original_text = ""
    s = original_text.replace('\r\n', '\n').replace('\r', '\n')
    lines = s.split('\n')  # trailing '' if original ended with newline

    header_stripped = header_line.rstrip()
    filtered = [ln for ln in lines if ln.rstrip() != header_stripped]

    # Remove leading blank lines
    i = 0
    while i < len(filtered) and filtered[i].strip() == "":
        i += 1
    remaining = filtered[i:]

    new_lines = [header_stripped, ""]  # header + exactly one blank line
    new_lines.extend(remaining)

    # Join and ensure exactly one newline at EOF
    new_text = "\n".join(new_lines).rstrip("\n") + "\n"

    # Direct comparison to detect any difference (including trailing-newline count)
    changed = (s != new_text)
    return new_text, changed

def process_file(path: Path, header_line: str, dry_run: bool, backup: bool, fallback_encoding: str):
    try:
        text, encoding_used, had_bom, original_bytes = read_file_preserve_encoding(path, fallback_encoding)
    except Exception as e:
        return False, f"error-read: {e}"

    new_text, changed = ensure_header_text(text, header_line)
    if not changed:
        return False, "unchanged"

    if dry_run:
        return True, "would-update"

    if backup:
        bak = path.with_suffix(path.suffix + ".bak")
        try:
            bak.write_bytes(original_bytes)
        except Exception as e:
            return False, f"error-backup: {e}"

    try:
        write_file_preserve_encoding(path, new_text, encoding_used, had_bom)
    except Exception as e:
        return False, f"error-write: {e}"

    return True, "updated"

# -------------------------
# File collection with omit-dir and files list support
# -------------------------
def normalize_omit_dirs(omit_list):
    """
    Normalize omit entries into:
      - basename_set: lowercased last path segment for quick matching (e.g., 'thirdparty')
      - fullpath_set: lowercased full entry strings and repo-relative strings for substring matching
      - parts_list: list of tuples of lowercased path parts for contiguous-sequence matching
    Returns (basename_set, fullpath_set, parts_list).
    """
    basename_set = set()
    fullpath_set = set()
    parts_list = []
    cwd = Path.cwd()

    for entry in omit_list or []:
        if not entry:
            continue
        p = Path(entry)
        # basename (last segment)
        basename_set.add(p.name.lower())

        # full string form
        fullpath_set.add(str(p).lower())

        # if entry is inside cwd, add its relative form too
        try:
            rel = p.relative_to(cwd)
            fullpath_set.add(str(rel).lower())
        except Exception:
            pass

        # store the sequence of parts for contiguous matching
        parts = tuple(part.lower() for part in p.parts if part not in ('.', ''))
        if parts:
            parts_list.append(parts)

    return basename_set, fullpath_set, parts_list


def _has_contiguous_subsequence(path_parts, subseq):
    """
    Return True if subseq (tuple of parts) appears as a contiguous subsequence in path_parts (list).
    """
    if not subseq:
        return False
    n = len(path_parts)
    m = len(subseq)
    if m > n:
        return False
    for i in range(n - m + 1):
        if tuple(path_parts[i:i + m]) == subseq:
            return True
    return False


def path_is_omitted(path: Path, omit_basename_set: set, omit_fullpath_set: set, omit_parts_list: list):
    """
    Return True if the path should be omitted.

    Matching strategy (in order):
      - any path segment equals a basename in omit_basename_set (fast)
      - any omit_parts_list tuple appears as a contiguous subsequence of the path parts
      - any omit_fullpath_set entry is a substring of the file path (handles absolute vs relative)
      - any omit_fullpath_set entry is a substring of the resolved absolute path (best-effort)
    """
    # Normalize path parts once
    path_parts = [part.lower() for part in path.parts]

    # match any path segment basename
    for part in path_parts:
        if part in omit_basename_set:
            return True

    # contiguous subsequence match (handles entries like "Source/ThirdParty")
    for subseq in omit_parts_list:
        if _has_contiguous_subsequence(path_parts, subseq):
            return True

    # substring match against the file path string
    pstr = str(path).lower()
    for fp in omit_fullpath_set:
        if fp and fp in pstr:
            return True

    # try resolved absolute path substring match
    try:
        resolved = str(path.resolve()).lower()
        for fp in omit_fullpath_set:
            if fp and fp in resolved:
                return True
    except Exception:
        pass

    return False


def collect_target_files(args, type_map):
    """
    Return a list of Path objects to process based on args:
      - If args.files provided: use that list (filter missing and omitted).
      - Otherwise: scan args.root for known extensions and filenames, skipping omitted dirs.
    """
    # normalize_omit_dirs now returns three things: (basename_set, fullpath_set, parts_list)
    omit_basename_set, omit_fullpath_set, omit_parts_list = normalize_omit_dirs(args.omit_dir)
    extra_exts = [e if e.startswith('.') else f".{e}" for e in (args.extensions or [])]

    print("omit basenames:", sorted(list(omit_basename_set)), file=sys.stdout)
    print("omit fullpaths sample:", list(omit_fullpath_set)[:5], file=sys.stdout)
    print("omit parts list sample:", omit_parts_list[:5], file=sys.stdout)

    targets = []
    if args.files:
        for f in args.files:
            p = Path(f)
            if not p.exists():
                print(f"Skipping missing file: {f}", file=sys.stderr)
                continue
            if path_is_omitted(p, omit_basename_set, omit_fullpath_set, omit_parts_list):
                print(f"Skipping omitted file (in omitted dir): {f}", file=sys.stdout)
                continue
            targets.append(p)
        return targets

    # Root scanning
    root = Path(args.root)
    if not root.exists():
        print(f"Root path does not exist: {root}", file=sys.stderr)
        return []

    # Build combined extension set from type_map and extra_exts
    exts = set()
    filenames = set()
    for info in type_map.values():
        exts.update(info.get('extensions', set()))
        filenames.update(info.get('filenames', set()))
    exts.update(e.lower() for e in extra_exts)

    for p in root.rglob('*'):
        if not p.is_file():
            continue
        if path_is_omitted(p, omit_basename_set, omit_fullpath_set, omit_parts_list):
            # skip any file under an omitted directory
            continue
        if p.name in filenames or p.suffix.lower() in exts:
            targets.append(p)
    return targets

# -------------------------
# Main
# -------------------------
def main():
    args = parse_args()
    extra_exts = [e if e.startswith('.') else f".{e}" for e in (args.extensions or [])]
    type_map = build_type_map(extra_exts)

    # Determine which types to process
    allowed_types = set(type_map.keys()) if not args.only else set(args.only)

    body = args.body if args.body is not None else DEFAULT_BODY

    files = collect_target_files(args, type_map)

    if not files:
        print("No files to process.")
        return

    changed = []
    for p in files:
        # Determine file type; if unknown, skip
        ftype = file_type_for_path(p, type_map)
        if ftype is None or ftype not in allowed_types:
            # If user provided explicit --files, we still skip unknown types to avoid accidental edits
            print(f"Skipping (unknown type or not allowed): {p}", file=sys.stdout)
            continue

        prefix = type_map[ftype]['prefix']
        # If user provided --header, use it verbatim, otherwise build from prefix + body
        if args.header:
            header_line = args.header.rstrip()
        else:
            header_line = make_full_header(prefix, body)

        try:
            ok, status = process_file(p, header_line, args.dry_run, backup=not args.no_backup, fallback_encoding=args.fallback_encoding)
            changed.append((str(p), status))
            print(f"{status}: {p}", file=sys.stdout)
        except Exception as e:
            changed.append((str(p), f"error: {e}"))
            print(f"error: {p}: {e}", file=sys.stderr)

    # Summary
    updated_any = any(s in ("updated", "would-update", "replaced-header", "inserted-header", "would-replace-header", "would-insert-header") for _, s in changed)
    if not changed:
        print("No files matched the configured types/extensions.")
    elif not updated_any:
        print("No files changed.")
    else:
        print("Some files were updated. Review .bak files for originals if backups were enabled.")

if __name__ == "__main__":
    main()
