# Source formatting

`format_changed_source.py` repairs XII copyright notices and runs ClangFormat
with the repository's `.clang-format` on entire selected C/C++ files. CMake
files (`CMakeLists.txt`, `.cmake`, and `.BuildFilter`) receive copyright repairs
only. It requires Python 3.8 or newer, Git, and a compatible ClangFormat executable
(CI currently uses `clang-format-23`).

To repair all tracked repository sources manually, run from anywhere inside the
checkout:

```sh
python3 Utilities/CI/format_changed_source.py --all --formatter clang-format-23
```

The example path is relative to the repository root; from another directory,
provide the appropriate path to the script. Both file discovery and relative
patch paths are resolved from the repository root. Third-party trees under
`Source/ThirdParty/`, `Source/Engine/Foundation/ThirdParty/`, and `Data/Tools/`,
all other `ThirdParty` directories, symbolic links, missing files, and untracked
files are excluded.

Commit or stash tracked local changes before running the tool. It modifies files
in place and writes a reusable binary Git patch to
`Workspace/Quality/source-style.patch` when repairs are needed. Use `--patch PATH`
to choose another output location. An existing patch at that location is removed
before processing; formatter failures return a nonzero exit status and do not
publish a success patch. Repairs made before a failure remain in the working tree
for inspection. Review the changes with `git diff` before committing them.

For CI or a branch-specific repair, select files added, modified, renamed, or
changed in type since the merge base of a full commit SHA and `HEAD`:

```sh
python3 Utilities/CI/format_changed_source.py --base FULL_COMMIT_SHA --formatter clang-format-23
```

`--base` and `--all` are mutually exclusive. Both modes format complete selected
C/C++ files, including unchanged lines and files with deletion-only changes, so
formatting and include ordering can converge to the configured style. Files
outside the selection remain untouched.

Run the formatting regression tests with:

```sh
python3 -m unittest discover -s Utilities/CI/tests -p test_format_changed_source.py -v
```

The real formatter integration test runs when `clang-format` is available on
`PATH`; the remaining tests do not require it.
