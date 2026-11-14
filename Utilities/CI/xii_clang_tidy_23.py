#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.
"""Verify, analyze, and apply curated LLVM 23 fixes to XII-owned C++ code.

Install as Utilities/CI/xii_clang_tidy_23.py. Run from XII's repository root.
Requires: Python 3.10+, clang-tidy-23, a CMake compile_commands.json for analysis.
No third-party Python packages required.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import subprocess
import sys

CPP_EXTENSIONS = {".cpp", ".cc", ".cxx"}
HEADER_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx", ".inl"}
EXCLUDED_COMPONENTS = {"ThirdParty", "External", "Generated", "_deps", "Workspace", "Vendored"}

# Only checks whose automatic fixes are narrowly scoped, mechanical, and
# compatible with XII's published conventions belong here. The --fix-safe
# option deliberately does NOT apply fixes from the entire .clang-tidy file.
# Clang-Tidy fix-its are not proof of semantic equivalence: review and build.
SAFE_FIX_CHECKS = (
    "readability-braces-around-statements",
    "readability-inconsistent-ifelse-braces",
    "readability-isolate-declaration",
    "readability-duplicate-include",
    "readability-uppercase-literal-suffix",
    "readability-avoid-const-params-in-decls",
    "readability-redundant-inline-specifier",
    "readability-redundant-control-flow",
    "modernize-redundant-void-arg",
    "modernize-use-nullptr",
    "modernize-use-override",
    "misc-include-cleaner",
)


def call(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)


def own_source(path: Path, root: Path) -> bool:
    try:
        rel = path.resolve().relative_to(root.resolve())
    except ValueError:
        return False
    return rel.parts[:1] == ("Source",) and not EXCLUDED_COMPONENTS.intersection(rel.parts)


def verify_toolchain(root: Path, binary: str) -> None:
    config = root / ".clang-tidy"
    if not config.is_file():
        raise RuntimeError(f"Missing XII configuration: {config}")
    version = call([binary, "--version"], root)
    if version.returncode != 0:
        raise RuntimeError(f"Cannot run {binary}: {version.stdout.strip()}")
    if not re.search(r"\b(?:LLVM|clang-tidy)\s+version\s+23(?:\.|\b)", version.stdout, flags=re.I):
        raise RuntimeError(f"LLVM 23 required; found: {version.stdout.strip()}")
    validation = call([binary, f"--config-file={config}", "--verify-config"], root)
    if validation.returncode != 0:
        raise RuntimeError(f"Clang-Tidy config rejected by LLVM 23:\n{validation.stdout}")
    print(f"Configuration accepted by {version.stdout.splitlines()[0].strip()}")


def compile_units(root: Path, db_path: Path) -> set[Path]:
    if not db_path.is_file():
        raise RuntimeError(f"Missing compilation database: {db_path}. Run cmake --preset linux-clang-debug first.")
    try:
        records = json.loads(db_path.read_text(encoding="utf-8"))
    except (ValueError, UnicodeError) as ex:
        raise RuntimeError(f"Invalid compilation database: {ex}") from ex
    units = set()
    for record in records:
        directory = Path(record["directory"])
        source = Path(record["file"])
        if not source.is_absolute():
            source = directory / source
        source = source.resolve()
        if own_source(source, root) and source.suffix in CPP_EXTENSIONS and source.is_file():
            units.add(source)
    return units


def diff_paths(root: Path, base_ref: str) -> set[Path]:
    base = call(["git", "merge-base", base_ref, "HEAD"], root)
    if base.returncode != 0:
        raise RuntimeError(f"Could not find merge-base against {base_ref}: {base.stdout}")
    commit = base.stdout.strip()
    changed = subprocess.run(["git", "diff", "--name-only", "--diff-filter=ACMR", "-z", commit, "--"],
                             cwd=root, capture_output=True, check=True).stdout
    # Git's -z output is byte-encoded; os.fsdecode safely handles non-UTF-8 names.
    return {(root / os.fsdecode(p)).resolve() for p in changed.split(b"\0") if p}


def select_units(root: Path, available: set[Path], mode: str, base: str, explicit: list[str]) -> tuple[list[Path], list[Path]]:
    if mode == "all":
        return sorted(available), []
    if mode == "files":
        candidates = {(root / item).resolve() for item in explicit}
        unknown = candidates - available
        if unknown:
            raise RuntimeError("Specified files are not compiled XII-owned C++ translation units:\n" +
                               "\n".join(str(p) for p in sorted(unknown)))
        return sorted(candidates), []

    changed = {p for p in diff_paths(root, base) if own_source(p, root)}
    selected = {p for p in changed if p in available}
    headers = sorted(p for p in changed if p.suffix in HEADER_EXTENSIONS)

    # Analyze matching implementation files where possible, then other TUs
    # from the owning module. Cross-module consumers still need full CI scans.
    for header in headers:
        rel = header.relative_to(root)
        matching = {p for p in available if p.stem == header.stem and p.parent.name == "Implementation" and
                    p.parent.parent == header.parent}
        if matching:
            selected.update(matching)
            continue
        if len(rel.parts) >= 3:
            module = rel.parts[:3]
            selected.update(p for p in available if p.relative_to(root).parts[:3] == module)
    return sorted(selected), headers


def check_clean_checkout(root: Path) -> None:
    status = call(["git", "status", "--porcelain", "--untracked-files=no"], root)
    if status.returncode != 0 or status.stdout.strip():
        raise RuntimeError("--fix-safe requires a clean tracked working tree; commit/stash changes first.")


def analyze(root: Path, binary: str, build: Path, source: Path, fix_safe: bool) -> tuple[str, int, str]:
    command = [binary, f"--config-file={root / '.clang-tidy'}", "-p", str(build), str(source)]
    if fix_safe:
        # Reset the full diagnostic profile and run ONLY the reviewed fix allowlist.
        # The repository profile sets WarningsAsErrors: '*'; suppress that policy
        # for the remediation pass so fixed warnings do not fail the invocation.
        # Compiler errors still fail, and --fix-errors is intentionally omitted.
        command.extend([
            "--checks=-*," + ",".join(SAFE_FIX_CHECKS),
            "--warnings-as-errors=-*",
            "--fix",
            "--format-style=file",
        ])
    result = call(command, root)
    return source.relative_to(root).as_posix(), result.returncode, result.stdout


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="XII repository root (default: CWD)")
    parser.add_argument("--clang-tidy", default="clang-tidy-23")
    parser.add_argument("--build", type=Path, default=Path("Workspace/linux-clang-debug"))
    parser.add_argument("--mode", choices=("verify", "changed", "all", "files"), default="verify")
    parser.add_argument("--base", default="origin/Development", help="Reference for changed-file analysis")
    parser.add_argument("--file", action="append", default=[], help="Source path, required for mode=files")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1, help="Parallel analysis jobs (default: CPU count)")
    parser.add_argument("--dry-run", action="store_true", help="List selected source files without analysis")
    parser.add_argument("--fix-safe", action="store_true", help="Apply the curated safe-fix allowlist in files, changed, or all mode")
    parser.add_argument("--allow-dirty", action="store_true", help="Permit autofixes with uncommitted tracked changes (local use; review resulting diff)")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    build = args.build if args.build.is_absolute() else root / args.build
    try:
        verify_toolchain(root, args.clang_tidy)
        if args.mode == "verify":
            if args.fix_safe or args.allow_dirty:
                raise RuntimeError("--fix-safe requires --mode=files, changed, or all")
            return 0
        if args.jobs < 1:
            raise RuntimeError("--jobs must be >= 1")
        if args.mode == "files" and not args.file:
            raise RuntimeError("--mode=files requires at least one --file")
        fix_safe = args.fix_safe and not args.dry_run
        if args.allow_dirty and not args.fix_safe:
            raise RuntimeError("--allow-dirty only applies to --fix-safe")
        if fix_safe:
            if args.allow_dirty:
                print("WARNING: Autofixing with uncommitted tracked changes. Review the resulting diff carefully.")
            else:
                check_clean_checkout(root)
        available = compile_units(root, build / "compile_commands.json")
        selected, headers = select_units(root, available, args.mode, args.base, args.file)
        if headers:
            print(f"NOTICE: {len(headers)} changed header(s); local module sampling cannot validate every downstream TU.")
            print("Use --mode all in CI to check cross-module consumers and public-header changes.")
        if not selected:
            if headers:
                raise RuntimeError("Changed headers have no available owning translation units. Run --mode=all.")
            print("No applicable XII-owned compilation units selected.")
            return 0
        print(f"Selected {len(selected)} / {len(available)} XII-owned translation units.")
        if args.dry_run:
            for source in selected:
                print(source.relative_to(root).as_posix())
            return 0
        failures = []
        workers = 1 if fix_safe else args.jobs
        if fix_safe:
            print(f"Applying {len(SAFE_FIX_CHECKS)} curated Clang-Tidy fix checks with .clang-format.")
            if args.jobs != 1:
                print("NOTICE: --fix-safe runs sequentially, regardless of --jobs, to avoid overlapping edits to shared headers.")
        with ThreadPoolExecutor(max_workers=workers) as pool:
            results = pool.map(lambda p: analyze(root, args.clang_tidy, build, p, fix_safe), selected)
            for name, code, output in results:
                print(f"\n===== {name} (exit {code}) =====")
                if output.strip():
                    print(output.rstrip())
                if code != 0:
                    failures.append(name)
        if failures:
            print(f"FAILED {len(failures)} unit(s): " + ", ".join(failures), file=sys.stderr)
            return 1
        if fix_safe:
            print("Safe-fix pass completed. Review the Git diff, then build and rerun analysis without --fix-safe.")
        else:
            print("Clang-Tidy completed without reported errors in selected units.")
        return 0
    except (OSError, RuntimeError, subprocess.CalledProcessError) as ex:
        print(f"ERROR: {ex}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())