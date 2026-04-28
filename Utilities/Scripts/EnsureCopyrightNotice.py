#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.
"""
Ensure a single-line triple-slash copyright header is the absolute first line
of each C/C++ file and that exactly one blank line separates the header from
the rest of the file. Preserve original file encoding and BOM when possible.

Default header:
/// Copyright (c) Theophilus Eriata. All Rights Reserved.

Usage:
  python EnsureCopyrightNotice.py --root . --dry-run
  python EnsureCopyrightNotice.py --root src --header "/// Copyright (c) Theophilus Eriata. All Rights Reserved."
  pip install charset-normalizer
"""
from pathlib import Path
import argparse
import shutil
import sys

# Optional enc detection libraries
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

print("Has charset-normalizer:", _HAS_CN)
print("Has chardet:", _HAS_CHARD)

# Default file extensions to scan
CPP_EXTS = {'.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx', '.inl'}

DEFAULT_HEADER_LINE = "/// Copyright (c) Theophilus Eriata. All Rights Reserved."

def parse_args():
    p = argparse.ArgumentParser(description="Force a single-line header as the first line of C/C++ files, preserving encoding.")
    p.add_argument('--root', default='.', help='Root directory to scan')
    p.add_argument('--header', help='Exact header line to insert (no trailing newline required)')
    p.add_argument('--dry-run', action='store_true', help='Show changes without writing files')
    p.add_argument('--no-backup', action='store_true', help='Do not create .bak backups')
    p.add_argument('--extensions', nargs='*', help='Extra extensions to include (e.g. .inl)')
    p.add_argument('--fallback-encoding', default='utf-8', help='Encoding to use if detection fails (default: utf-8)')
    return p.parse_args()

def detect_encoding_from_bytes(b: bytes):
    """
    Return (encoding_name, had_bom, confidence_float).
    encoding_name may be None if detection fails.
    """
    # Detect BOM for UTF-8
    if b.startswith(b'\xef\xbb\xbf'):
        return 'utf-8-sig', True, 1.0
    # Try charset-normalizer first (recommended)
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
    # Fallback to chardet if available
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
    """
    Read bytes, detect encoding, decode to text. Return (text, encoding_used, had_bom, original_bytes).
    If detection fails, fallback_encoding is used with surrogateescape to avoid data loss.
    """
    b = path.read_bytes()
    enc, had_bom, conf = detect_encoding_from_bytes(b)
    if enc:
        # Normalize common names
        enc_norm = enc.lower().replace('_', '-')
        try:
            # If encoding is utf-8-sig, decode to remove BOM
            if enc_norm in ('utf-8-sig', 'utf8-sig'):
                text = b.decode('utf-8-sig', errors='strict')
                return text, 'utf-8-sig', True, b
            # decode normally
            text = b.decode(enc, errors='strict')
            return text, enc, had_bom, b
        except Exception:
            # decoding failed despite detection; fall through to fallback
            pass
    # Fallback: try utf-8 strict, then fallback_encoding with surrogateescape
    try:
        text = b.decode('utf-8', errors='strict')
        return text, 'utf-8', False, b
    except Exception:
        # Use fallback encoding with surrogateescape to preserve bytes
        try:
            text = b.decode(fallback_encoding, errors='surrogateescape')
            return text, fallback_encoding, False, b
        except Exception:
            # As last resort, latin-1 (never fails)
            text = b.decode('latin-1', errors='strict')
            return text, 'latin-1', False, b

def write_file_preserve_encoding(path: Path, text: str, encoding: str, had_bom: bool):
    """
    Encode text using encoding and write bytes. If encoding is 'utf-8-sig' or had_bom True for utf-8,
    write BOM accordingly.
    """
    enc_norm = encoding.lower().replace('_', '-')
    if enc_norm in ('utf-8-sig',):
        out_bytes = text.encode('utf-8-sig')
    else:
        # If original had BOM and encoding is utf-8, write utf-8-sig
        if had_bom and enc_norm in ('utf-8', 'utf8'):
            out_bytes = text.encode('utf-8-sig')
        else:
            # Use surrogateescape-safe encoding to avoid data loss for undecodable bytes
            out_bytes = text.encode(encoding, errors='surrogateescape')
    path.write_bytes(out_bytes)

def ensure_header_text(original_text: str, header_line: str) -> (str, bool):
    """
    Return (new_text, changed_flag).
    Rules:
      - Remove all lines that exactly equal header_line (ignoring trailing spaces).
      - Strip leading blank lines from the remaining content.
      - Prepend header_line + one blank line.
      - Ensure the file ends with exactly one newline character.
    """
    if original_text is None:
        original_text = ""
    # Normalize newlines to LF for processing
    s = original_text.replace('\r\n', '\n').replace('\r', '\n')

    # Split into lines (split removes newline chars; trailing '' indicates trailing newline)
    lines = s.split('\n')

    # Remove all lines that exactly equal the header (ignoring trailing spaces)
    header_stripped = header_line.rstrip()
    filtered = [ln for ln in lines if ln.rstrip() != header_stripped]

    # Remove leading blank lines
    i = 0
    while i < len(filtered) and filtered[i].strip() == "":
        i += 1
    remaining = filtered[i:]

    # Rebuild: header line, one blank line, then remaining content
    new_lines = [header_stripped, ""]  # header + exactly one blank line
    new_lines.extend(remaining)

    # Join and ensure exactly one newline at EOF
    new_text = "\n".join(new_lines).rstrip("\n") + "\n"

    # Direct comparison of normalized original to new_text so differences
    # in trailing-newline count are detected.
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

    # create backup of original bytes
    if backup:
        bak = path.with_suffix(path.suffix + ".bak")
        try:
            bak.write_bytes(original_bytes)
        except Exception as e:
            return False, f"error-backup: {e}"

    # write new text preserving encoding and BOM
    try:
        write_file_preserve_encoding(path, new_text, encoding_used, had_bom)
    except Exception as e:
        return False, f"error-write: {e}"

    return True, "updated"

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
            ok, status = process_file(p, header_line, args.dry_run, backup=not args.no_backup, fallback_encoding=args.fallback_encoding)
            changed.append((str(p), status))
        except Exception as e:
            changed.append((str(p), f"error: {e}"))

    for f, s in changed:
        print(f"{s}: {f}")
    if not changed:
        print("No files matched the configured extensions.")

if __name__ == "__main__":
    main()
