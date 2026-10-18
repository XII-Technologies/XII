#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Package an exact versioned source snapshot, without inventing a binary SDK."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--tag", required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
version = Path("version.txt").read_text(encoding="utf-8").strip()
if not re.fullmatch(r"\d+\.\d+\.\d+", version) or args.tag != f"v{version}":
    parser.error("tag must be v<version.txt>, using MAJOR.MINOR.PATCH")

def git(*arguments):
    return subprocess.check_output(["git", *arguments], text=True).strip()

commit = git("rev-parse", "HEAD")
if git("rev-parse", f"refs/tags/{args.tag}^{{commit}}") != commit:
    parser.error("HEAD must be the release tag's exact commit")
args.output.mkdir(parents=True, exist_ok=True)
archive = args.output / f"XII-{version}-source.tar.gz"
subprocess.run(["git", "archive", "--format=tar.gz", f"--prefix=XII-{version}/", "-o", str(archive), "HEAD"], check=True)
submodules = []
for entry in git("ls-tree", "-r", "HEAD").splitlines():
    metadata, name = entry.split("\t", 1)
    mode, kind, revision = metadata.split()
    if mode == "160000":
        submodules.append({"path": name, "commit": revision})
manifest = args.output / f"XII-{version}-source-manifest.json"
manifest.write_text(json.dumps({
    "version": version, "tag": args.tag, "commit": commit,
    "scope": "Tracked source only; submodule payloads excluded. Not a binary SDK.",
    "submodules": submodules,
}, indent=2) + "\n", encoding="utf-8")
with (args.output / "SHA256SUMS").open("w", encoding="utf-8", newline="\n") as checksums:
    for path in (archive, manifest):
        digest = hashlib.file_digest(path.open("rb"), "sha256").hexdigest()
        checksums.write(f"{digest}  {path.name}\n")
