#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Read-only PR style checks; never commit or push contribution code."""

import argparse
from pathlib import Path
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--base", required=True)
parser.add_argument("--formatter", required=True)
args = parser.parse_args()
if not re.fullmatch(r"[0-9a-f]{40}", args.base):
    parser.error("--base must be a full commit SHA")

excluded = ("Source/ThirdParty/", "Source/Engine/Foundation/ThirdParty/", "Data/Tools/")
suffixes = {".cpp", ".c", ".cc", ".cxx", ".h", ".hpp", ".inl", ".cmake", ".BuildFilter"}
names = subprocess.check_output(
    ["git", "diff", "--name-only", "--diff-filter=ACMR", "-z", args.base, "HEAD", "--"],
).decode("utf-8").split("\0")
files = [name for name in names if name and not name.startswith(excluded)
         and (Path(name).suffix in suffixes or Path(name).name == "CMakeLists.txt")]
output = Path("Workspace/Quality")
output.mkdir(parents=True, exist_ok=True)
failed = False
for name in files:
    first_line = Path(name).read_text(encoding="utf-8-sig").splitlines()[0:1]
    prefix = "#" if Path(name).suffix in {".cmake", ".BuildFilter"} or Path(name).name == "CMakeLists.txt" else "///"
    if first_line != [f"{prefix} Copyright (c) Theophilus Eriata. All Rights Reserved."]:
        print(f"{name}: missing repository copyright header", file=sys.stderr)
        failed = True
code = [name for name in files if Path(name).suffix not in {".cmake", ".BuildFilter"} and Path(name).name != "CMakeLists.txt"]
if code:
    diff = subprocess.check_output(["git", "diff", "-U0", "--no-color", args.base, "HEAD", "--", *code])
    result = subprocess.run(
        [sys.executable, "Utilities/ClangFormatDiff.py", "-p1", "-style", "file", "-binary", args.formatter, "-verbose"],
        input=diff, capture_output=True, check=True,
    )
    if result.stdout:
        (output / "clang-format.patch").write_bytes(result.stdout)
        print("Changed lines need formatting; apply Workspace/Quality/clang-format.patch", file=sys.stderr)
        failed = True
raise SystemExit(1 if failed else 0)
