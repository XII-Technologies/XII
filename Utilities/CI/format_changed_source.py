#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Repair copyright notices and format selected C/C++ files in a clean checkout."""

import argparse
import os
from pathlib import Path
import re
import subprocess

COPYRIGHT = b"Copyright (c) Theophilus Eriata. All Rights Reserved."
BOM = b"\xef\xbb\xbf"
EXCLUDED = ("Source/ThirdParty/", "Source/Engine/Foundation/ThirdParty/", "Data/Tools/")
CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}
CMAKE_SUFFIXES = {".cmake", ".BuildFilter"}
NOTICE = re.compile(rb"(?i)^[ \t]*(?://+|#)[ \t]*Copyright\b[^\r\n]*Theophilus Eriata[^\r\n]*(?:\r?\n|$)")


def git(*args):
    return subprocess.check_output(["git", "--literal-pathspecs", *args])


def changed_paths(base):
    changes = iter(git("diff", "--name-status", "-z", "--find-renames", "--diff-filter=AMRT",
                       base, "HEAD", "--").split(b"\0")[:-1])
    for status in changes:
        previous = os.fsdecode(next(changes))
        current = os.fsdecode(next(changes)) if status.startswith(b"R") else previous
        yield previous, current


def fix_notice(path, prefix):
    original = path.read_bytes()
    bom = BOM if original.startswith(BOM) else b""
    contents = original[len(bom):]
    newline = b"\r\n" if contents.partition(b"\n")[0].endswith(b"\r") else b"\n"
    contents = contents.lstrip(b"\r\n")
    while match := NOTICE.match(contents):
        contents = contents[match.end():].lstrip(b"\r\n")
    updated = bom + prefix + b" " + COPYRIGHT + newline + newline + contents
    if updated != original:
        path.write_bytes(updated)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--base", help="Full commit SHA to compare against HEAD; format entire changed files")
    selection.add_argument("--all", action="store_true", help="Repair and format all tracked repository sources")
    parser.add_argument("--formatter", default="clang-format")
    parser.add_argument("--patch", type=Path, default=Path("Workspace/Quality/source-style.patch"))
    args = parser.parse_args()
    os.chdir(os.fsdecode(git("rev-parse", "--show-toplevel").rstrip(b"\n")))
    if args.base and not re.fullmatch(r"[0-9a-f]{40}", args.base):
        parser.error("--base must be a full commit SHA")
    if git("status", "--porcelain", "--untracked-files=no", "--ignore-submodules=all"):
        parser.error("tracked files must match HEAD before formatting")

    args.patch.unlink(missing_ok=True)
    if args.all:
        paths = [os.fsdecode(path) for path in git("ls-files", "-z").split(b"\0") if path]
    else:
        base = git("merge-base", args.base, "HEAD").decode().strip()
        paths = [current for previous, current in changed_paths(base)]
    files = []
    for current in paths:
        path = Path(current)
        if current.startswith(EXCLUDED) or path.is_symlink() or not path.is_file():
            continue
        is_cpp = path.suffix in CPP_SUFFIXES
        if is_cpp or path.suffix in CMAKE_SUFFIXES or path.name == "CMakeLists.txt":
            fix_notice(path, b"///" if is_cpp else b"#")
            if is_cpp:
                files.append(current)
    for current in files:
        subprocess.run([args.formatter, "--style=file", "--fallback-style=none", "-i", "--", current], check=True)

    patch = git("diff", "--no-ext-diff", "--no-textconv", "--no-color", "--binary", "--full-index", "HEAD", "--")
    if patch:
        args.patch.parent.mkdir(parents=True, exist_ok=True)
        args.patch.write_bytes(patch)
        print(f"Applied source fixes and wrote {args.patch}")
    else:
        print("Selected sources already follow repository style.")


if __name__ == "__main__":
    main()
