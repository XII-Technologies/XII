# XII Engine: Clang-Tidy 23 engineering profile

**Target:** LLVM / Clang-Tidy **23.1.x**, C++23, XII `Development` branch conventions.

**Deliverables:** `.clang-tidy` (root configuration) and `Utilities/CI/xii_clang_tidy_23.py` (safe, deterministic driver). This package is a proposal, not a direct GitHub modification.

## 1. Changes from the LLVM 22 profile

- **204 enabled check patterns** (plus the `-*` reset), **30 check options**.
- Preserves targeted static analyzer, `bugprone`, `cppcoreguidelines`, `misc`, `modernize`, `performance`, `portability`, and `readability` diagnostics.
- Adds LLVM 23's `bugprone-assignment-in-selection-statement`, `bugprone-missing-end-comparison`, `bugprone-unsafe-to-allow-exceptions`, `misc-static-initialization-cycle`, `performance-string-view-conversions`, and `performance-use-std-move` where applicable, plus other newly selected existing checks.
- Adds `misc-explicit-constructor`, `misc-use-anonymous-namespace`, `cppcoreguidelines-missing-std-forward`, named-parameter consistency, mathematical parentheses, redundant conditional diagnostics, and standard-library performance diagnostics.
- Adds regular `EnumConstantCase`, `ValueTemplateParameterCase`, and constant/class naming cases; keeps private/protected `m_` prefixes and uppercase macro names. `s_` field exceptions remain because private static fields have a different policy.
- Removes `modernize-use-nodiscard` from the blocking profile. The XII guide (§7.6) specifically advises **against mechanically annotating every trivial getter**. Significant-result rules need type-aware detection.
- Keeps C-style cast, unused `xiiResult`, default member initialization, and strict universal braces checks.
- **Does not enable** blanket `modernize-use-auto`, `modernize-use-trailing-return-type`, `modernize-use-std-format`, `modernize-use-std-print`, or `modernize-use-string-view`: these could override XII's existing API, ABI, and custom string/container conventions. These remain candidates for targeted review, not mandatory syntax substitutions.
- **Does not enable** blanket `cppcoreguidelines-owning-memory`, `cppcoreguidelines-no-malloc`, `modernize-avoid-c-arrays`, or `portability-avoid-pragma-once`: XII has custom allocators, permitted fixed arrays, and mandatory `#pragma once`.

## 2. Source-aligned enforcement

| XII Coding Standards | What the profile does | What remains outside Clang-Tidy |
|---|---|---|
| §2.1-2.3 | Mandatory braces diagnostics; checking declarations, redundant code and expressions | ClangFormat handles indentation, wrapping, alignment and blank lines. `.editorconfig` handles end-of-line policy. |
| §2.4, §8 | Isolate multiple declarations; detect assignments in selection statements, suspicious arithmetic and redundant conditions | Human intent and complexity cannot be inferred without semantic evidence. |
| §2.5, §4 | Include-cleaner for missing includes, include cycles and header misuse | Module-aware dependency groups, `DLL.h` first include, PCH first include, copyright, standalone header build. |
| §2.8 | AST diagnostics relevant to header hygiene | `Implementation/` directory placement requires a repository layout checker. |
| §3 | Function/method/enum/template PascalCase subset, `m_` instance-prefix subset, uppercase macros | Type-specific `m_ui`/`m_i`/`m_b`/`m_h`/`m_p`/`m_s`/`m_v`/`m_q`/`m_m`, `s_*` semantics, exact `xii` class prefixes, public-field exceptions, units and abbreviations. |
| §5-§6 | Constructors, copy/move, virtual destructors, RAII, use-after-move and static initialization cycles | XII allocator ownership, generation handles, GPU submission/completion lifetime, threading, object/pointer stability. |
| §7, §12 | `xiiResult` unused-return warnings, assertion side effects, errors in copy/move/lifetime operations | Proper `.IgnoreResult()` justification, expected runtime failures versus programming errors, appropriate logging levels and rollback. |
| §9 | Selected modern C++ correctness cleanups with minimal engine API substitution | Whether to adopt ranges, coroutines, `auto`, `std::string_view`, `std::expected`, modules, or replace XII facilities is architectural. |
| §10-§11 | Copy/move performance and standard-library string/view diagnostics | Engine custom containers, hashes, handle domains, serialization, and resource IDs. |

## 3. Policy conflicts to resolve **before** requiring all checks

1. **Mandatory braces (§2.2).** Current guide permits unbraced single-statement guards and loops. Replace those MAY/SHOULD clauses with MUST, update the 44 potential unbraced examples after distinguishing acceptable from intentionally bad examples.
2. **Member field exceptions (§3).** The new profile no longer silently grandfathers arbitrary `mCamelName` fields, but cannot distinguish arrays from other types. Do not mass-rename serialized/reflected members. Introduce grandfathered exceptions or migrate them deliberately.
3. **Source formatting.** `.clang-format` is still the authority; to make it deterministic, separately review `AllowShortFunctionsOnASingleLine`, `MaxEmptyLinesToKeep`, `KeepEmptyLinesAtTheStartOfBlocks`, alignment groups, and include categories. CI installs and invokes `clang-format-23`; validate repository-wide formatting changes with that version before publishing them.
4. **Naming.** Local/parameter prefixes remain optional in current §3.1. This profile does *not* invent a mandatory rule. A policy amendment and an XII-specific type-aware check would be needed to enforce a uniform local prefix convention.
5. **Governance.** Naming check auto-fixes must be disabled for public, reflected, and serialized APIs until compatibility and migrations are handled.

## 4. Verify on Linux

Install matching **LLVM/Clang-Tidy 23.1.x** and **ClangFormat 23.1.x**. XII already has a `linux-clang-dev` CMake preset with `CMAKE_EXPORT_COMPILE_COMMANDS: ON`. Use a clean build tree when changing CMake compilers.

```bash
# Verify the exact LLVM 23 check names and options before any analysis:
clang-tidy-23 --config-file=.clang-tidy --verify-config

# Independently verify that XII's .clang-format is valid for LLVM 23:
clang-format-23 --dump-config --style=file >/dev/null

# Configure XII (with the LLVM 23 compiler available as clang/clang++):
cmake --preset linux-clang-dev

# Verify config through the runner (requires no compilation database):
python3 Utilities/CI/xii_clang_tidy_23.py --mode verify

# Analyze a single C++ TU including relevant header diagnostics:
python3 Utilities/CI/xii_clang_tidy_23.py --mode files \
    --file Source/Engine/GraphicsCore/Pipeline/Implementation/RenderGraph.cpp

# Analyze branch-changed source plus local owning-module header consumers:
python3 Utilities/CI/xii_clang_tidy_23.py --mode changed --base origin/Development

# Full source-owned scan (required periodically, and for cross-module headers):
python3 Utilities/CI/xii_clang_tidy_23.py --mode all --jobs 2

# ONLY safe, explicit brace fixes; refuses an unclean checkout and whole-repo mode:
python3 Utilities/CI/xii_clang_tidy_23.py --mode files \
    --file Source/Engine/GraphicsCore/Pipeline/Implementation/RenderGraph.cpp --fix-braces
```

**Note:** The path above is an example; confirm a file's exact location before invoking `--mode files`.

The runner validates LLVM's *major version*, invokes `--verify-config`, and then restricts analysis to C++ translation units under `Source/` with a compile command. It skips `ThirdParty`, `External`, `Generated`, `_deps`, `Workspace`, and `Vendored` path components. For modified headers it can select same-module TUs, but this is **not a complete reverse-include dependency graph**. Run `--mode all` in CI for cross-module validation. It avoids general `--fix` because that could rename public APIs or make performance/ownership transformations without review.

## 5. Recommended CI pipeline

1. **PR changed-code gate:** pinned LLVM 23 `--verify-config`, ClangFormat 23 formatting check, then targeted Clang-Tidy diagnostics on changed owned translation units. Record baseline exceptions for legacy sources; do not silently accept newly introduced violations.
2. **Nightly/full or module-based scans:** `--mode all` against a compilation database generated by the same intended toolchain and representative build flags. Use an actual C++23 compilation context; a check's success on an unrelated build configuration is insufficient.
3. **Native build + tests:** compiler diagnostics as errors for `-Wreorder` and `-Wimplicit-fallthrough` where supported, debug tests, header self-sufficiency, and platform-specific builds. Clang-Tidy is not a compiler or a correctness proof.
4. **No CI auto-merge or bulk rewrite:** tooling should report violations and produce reviewable, focused fixes. Fixes that affect public API, serialized names, layout, performance, or resource ownership require dedicated review.
5. **Custom XII plugin backlog:** implement type-specific member prefixes, xii-type naming, DLL/PCH include constraints, source path structure, result-discard policy, allocator/handle/CPU-GPU lifetime contracts, and exception/API boundary restrictions. `--load` supports plugin checks built against the **exact LLVM 23 ABI**; they do not magically exist in this YAML.

## 6. Validation status and provenance

- `python -m py_compile` on the supplied driver: **completed**.
- YAML parsed with PyYAML; check patterns and key-value options structurally validated: **completed**.
- Tool documentation checked against the **LLVM 23.1.0 release documentation**, including check names, `--verify-config`, and Clang-Tidy's diff-analysis limitations.
- **Actual `clang-tidy-23 --verify-config`, source compilation, diagnostics and fixes: NOT RUN.** The LLVM 23 binaries and a local XII checkout are not available in the artifact-building environment. Verify these on a configured Linux developer machine before enabling merge blocking.

References:
- https://releases.llvm.org/23.1.0/tools/clang/tools/extra/docs/clang-tidy/index.html
- https://releases.llvm.org/23.1.0/tools/clang/tools/extra/docs/clang-tidy/checks/list.html
- https://releases.llvm.org/23.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html
- https://github.com/XII-Technologies/XII/blob/Development/Coding-Standards.md
