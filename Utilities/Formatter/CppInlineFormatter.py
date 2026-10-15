#!/usr/bin/env python3
"""Collapse multiline C++ argument/parameter/template lists and control headers.

Requires: python -m pip install tree-sitter tree-sitter-cpp

This is a deliberately conservative *whitespace-only* formatter.  Lambda and
other statement bodies keep their layout, comments and preprocessor directives
are not moved, and edits are rejected if the C++ token stream changes or the
parse acquires additional errors.  It is not a replacement for clang-format.
"""

from __future__ import annotations

import argparse
import difflib
import fnmatch
import os
from pathlib import Path
import stat
import sys
import tempfile
from dataclasses import dataclass
from typing import Iterable

SOURCE_SUFFIXES = frozenset({
    ".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp", ".tpp",
    ".cc", ".cpp", ".cxx", ".ixx", ".cppm",
})
SKIP_DIRS = frozenset({
    ".git", ".hg", ".svn", ".venv", "node_modules", ".cache",
    "build", "_build", "cmakefiles", "thirdparty", "third_party",
    "external", "vendor",
})

# All of these are grammar nodes, not textual patterns.  Adding support for a
# new C++ grammar production normally requires only extending this set.
LIST_NODES = frozenset({
    "argument_list",                # calls, constructors, operator calls
    "parameter_list",               # functions, lambdas, function pointers
    "template_argument_list",       # Foo<A, B>
    "template_parameter_list",      # template<class A, class B>
    "lambda_capture_specifier",     # [a, b = f()]
    "condition_clause",             # if/else-if/while/switch + initializers
})
FOR_NODES = frozenset({"for_statement", "for_range_loop"})
DO_NODES = frozenset({"do_statement"})

# A parent list can collapse whitespace *around* these nodes, not inside them.
# The inner AST is still visited separately, so calls inside lambda bodies
# remain eligible for formatting.
OPAQUE_TYPES = frozenset({
    "compound_statement", "string_literal", "raw_string_literal",
    "char_literal", "comment", "ERROR", "system_lib_string",
    "requires_expression",
})

WHITE = frozenset(b" \t\r\n\v\f")


@dataclass(frozen=True)
class Edit:
    start: int
    end: int
    replacement: bytes


def make_parser():
    try:
        from tree_sitter import Language, Parser
        import tree_sitter_cpp
    except ImportError as exc:
        raise RuntimeError(
            "Missing Tree-sitter dependency. Run: "
            "python -m pip install tree-sitter tree-sitter-cpp"
        ) from exc
    language = Language(tree_sitter_cpp.language())
    # Parser(language) is supported by current official Python bindings.
    try:
        return Parser(language)
    except TypeError:
        parser = Parser()
        parser.language = language
        return parser


def walk_nodes(node):
    """Iterative traversal, including descendants inside lambda bodies."""
    stack = [node]
    while stack:
        current = stack.pop()
        yield current
        stack.extend(reversed(current.children))


def for_header(node, source: bytes) -> tuple[int, int] | None:
    """Find the parenthesized header of a for or range-for statement.

    Do not touch the body (which could be a compound or a single statement).
    Tree-sitter represents '(' and ')' as unnamed immediate children.
    """
    body = node.child_by_field_name("body")
    limit = body.start_byte if body is not None else node.end_byte
    opening = None
    closing = None
    for child in node.children:
        if child.start_byte >= limit:
            break
        token = source[child.start_byte:child.end_byte]
        if token == b"(" and opening is None:
            opening = child.start_byte
        elif token == b")" and opening is not None:
            closing = child.end_byte
    if opening is None or closing is None:
        return None
    return opening, closing


def atoms_for(node, start: int, end: int):
    """Yield leaf tokens, treating statements/literals as opaque atoms.

    Returning a marker for comments/directives allows the caller to refuse
    joining any whitespace across them. An opaque statement body is *not*
    dangerous; we simply preserve its internal line breaks.
    """
    stack = [node]
    while stack:
        current = stack.pop()
        if current.end_byte <= start or current.start_byte >= end:
            continue
        kind = current.type
        if kind in OPAQUE_TYPES or kind.startswith("preproc_"):
            yield current.start_byte, current.end_byte, kind
            continue
        if current.child_count == 0:
            if current.start_byte < current.end_byte:
                yield current.start_byte, current.end_byte, kind
            continue
        stack.extend(reversed(current.children))


def joiner(source: bytes, left, right) -> bytes:
    """Prefer conventional punctuation spacing when eliminating a newline."""
    lhs = source[left[0]:left[1]]
    rhs = source[right[0]:right[1]]
    if lhs in (b"(", b"[", b"<"):
        return b""
    if rhs in (b")", b"]", b",", b";"):
        return b""
    return b" "


def propose_in_interval(node, start: int, end: int, source: bytes):
    atoms = list(atoms_for(node, start, end))
    if len(atoms) < 2:
        return
    # Explicitly do not rewrite a list with a comment, conditional compilation,
    # or an error node at its syntactic level. Opaque compound bodies are fine.
    if any(t == "comment" or t == "ERROR" or t.startswith("preproc_")
           for _, _, t in atoms):
        return
    for left, right in zip(atoms, atoms[1:]):
        a, b = left[1], right[0]
        if not (start <= a <= b <= end) or a == b:
            continue
        gap = source[a:b]
        if b"\n" not in gap and b"\r" not in gap:
            continue
        if not all(ch in WHITE for ch in gap):
            # Includes escaped newlines, inline comments and unknown syntax.
            continue
        # A lone backslash immediately before a newline is line splicing.
        # Do not risk changing its meaning.
        if source[max(0, a - 1):a] == b"\\":
            continue
        yield Edit(a, b, joiner(source, left, right))


def collect_edits(root, source: bytes) -> list[Edit]:
    unique: dict[tuple[int, int], Edit] = {}
    for node in walk_nodes(root):
        if node.has_error:
            # No inference about malformed code, macros, or partial snippets.
            # Descendants without errors are still visited independently.
            continue
        kind = node.type
        if kind not in LIST_NODES and kind not in FOR_NODES and kind not in DO_NODES:
            continue
        # Macro definitions can contain tokens that depend on physical newlines.
        ancestor = node.parent
        in_macro = False
        while ancestor is not None:
            if ancestor.type in {"preproc_def", "preproc_function_def"}:
                in_macro = True
                break
            ancestor = ancestor.parent
        if in_macro:
            continue
        if kind in LIST_NODES:
            bounds = (node.start_byte, node.end_byte)
        elif kind in DO_NODES:
            condition = node.child_by_field_name("condition")
            bounds = ((condition.start_byte, condition.end_byte)
                      if condition is not None else None)
        else:
            bounds = for_header(node, source)
        if bounds is None:
            continue
        for edit in propose_in_interval(node, *bounds, source):
            existing = unique.get((edit.start, edit.end))
            if existing is None:
                unique[(edit.start, edit.end)] = edit
            elif existing.replacement != edit.replacement:
                # Conflicting nested contexts: keep a separator for safety.
                unique[(edit.start, edit.end)] = Edit(edit.start, edit.end, b" ")
    edits = sorted(unique.values(), key=lambda e: (e.start, e.end))
    for first, second in zip(edits, edits[1:]):
        if first.end > second.start:
            raise RuntimeError("Overlapping edits detected; refusing to format")
    return edits


def apply_edits(source: bytes, edits: list[Edit]) -> bytes:
    parts: list[bytes] = []
    last = 0
    for edit in edits:
        parts.append(source[last:edit.start])
        parts.append(edit.replacement)
        last = edit.end
    parts.append(source[last:])
    return b"".join(parts)


def token_stream(node, source: bytes) -> list[tuple[str, bytes]]:
    """Compare all grammar tokens, not only expressions we explicitly target."""
    result: list[tuple[str, bytes]] = []
    for cur in walk_nodes(node):
        if cur.child_count == 0 and cur.start_byte < cur.end_byte:
            result.append((cur.type, source[cur.start_byte:cur.end_byte]))
    return result


def error_count(node) -> int:
    return sum(n.type == "ERROR" or n.is_missing for n in walk_nodes(node))


def format_source(source: bytes, parser) -> tuple[bytes, int, str | None]:
    original = parser.parse(source)
    edits = collect_edits(original.root_node, source)
    if not edits:
        return source, 0, None
    result = apply_edits(source, edits)
    updated = parser.parse(result)
    if error_count(updated.root_node) > error_count(original.root_node):
        return source, 0, "new parser errors introduced"
    if token_stream(original.root_node, source) != token_stream(updated.root_node, result):
        return source, 0, "C++ grammar token stream changed"
    return result, len(edits), None


def iter_files(paths: Iterable[str], suffixes: frozenset[str], excludes: list[str]):
    seen: set[Path] = set()
    for raw in paths:
        root = Path(raw)
        if root.is_file():
            files = [root]
        elif root.is_dir():
            # Prune vendor/build trees without scanning their files.
            def pruned_walk():
                for folder, dirs, names in os.walk(root, followlinks=False):
                    dirs[:] = sorted(d for d in dirs if d.lower() not in SKIP_DIRS
                                     and not any(fnmatch.fnmatch((Path(folder) / d).as_posix(), pat)
                                                 or fnmatch.fnmatch(d, pat)
                                                 for pat in excludes))
                    for filename in sorted(names):
                        yield Path(folder) / filename
            files = pruned_walk()
        else:
            print(f"[missing] {root}", file=sys.stderr)
            continue
        for path in files:
            if path.is_symlink() or path.suffix.lower() not in suffixes:
                continue
            if any(fnmatch.fnmatch(path.as_posix(), pat)
                   or fnmatch.fnmatch(path.name, pat) for pat in excludes):
                continue
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            yield path


def write_atomic(path: Path, content: bytes) -> None:
    original_mode = stat.S_IMODE(path.stat().st_mode)
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", dir=path.parent,
                                         prefix=f".{path.name}.", suffix=".tmp",
                                         delete=False) as output:
            temp_name = output.name
            output.write(content)
        os.chmod(temp_name, original_mode)
        os.replace(temp_name, path)
    finally:
        if temp_name and os.path.exists(temp_name):
            os.unlink(temp_name)


def show_diff(path: Path, old: bytes, new: bytes) -> None:
    a = old.decode("utf-8", errors="surrogateescape").splitlines(keepends=True)
    b = new.decode("utf-8", errors="surrogateescape").splitlines(keepends=True)
    sys.stdout.writelines(difflib.unified_diff(
        a, b, fromfile=f"a/{path}", tofile=f"b/{path}", lineterm="\n"
    ))


def main(argv: list[str] | None = None) -> int:
    cli = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    cli.add_argument("paths", nargs="+", help="C++ files and/or directories")
    mode = cli.add_mutually_exclusive_group()
    mode.add_argument("--in-place", action="store_true",
                      help="Write changes to files (default: no writes)")
    mode.add_argument("--dry-run", action="store_true",
                      help="Check without writing (the default)")
    cli.add_argument("--diff", action="store_true", help="Show a unified diff")
    mode.add_argument("--check", action="store_true",
                      help="Exit 1 if any files require formatting (for CI)")
    cli.add_argument("--exclude", action="append", default=[], metavar="GLOB",
                     help="Exclude matching paths or names; repeatable")
    cli.add_argument("--ext", action="append", default=[], metavar="SUFFIX",
                     help="Add a recognized source extension, e.g. --ext .inc")
    args = cli.parse_args(argv)
    suffixes = SOURCE_SUFFIXES | frozenset(
        ext.lower() if ext.startswith(".") else "." + ext.lower()
        for ext in args.ext
    )
    try:
        parser = make_parser()
    except RuntimeError as exc:
        cli.error(str(exc))

    total = changed = unsafe = failures = gaps = 0
    for path in iter_files(args.paths, suffixes, args.exclude):
        total += 1
        try:
            original = path.read_bytes()
            updated, count, problem = format_source(original, parser)
            if problem:
                print(f"[skip: {problem}] {path}", file=sys.stderr)
                unsafe += 1
                continue
            if updated == original:
                continue
            changed += 1
            gaps += count
            if args.diff:
                show_diff(path, original, updated)
            if args.in_place:
                write_atomic(path, updated)
                print(f"[formatted] {path} ({count} joins)")
        except (OSError, RuntimeError, UnicodeError) as exc:
            failures += 1
            print(f"[error] {path}: {exc}", file=sys.stderr)

    print(f"Scanned {total} files; {changed} changed; {gaps} line joins; "
          f"{unsafe} rejected; {failures} failures", file=sys.stderr)
    if failures or unsafe:
        return 2
    if args.check and changed:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
