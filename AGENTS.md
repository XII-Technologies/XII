# XII Engine - Agent Instructions

These instructions apply repository-wide to AI coding agents working on XII Engine.

## Source of truth

**Read and follow [`Coding-Standards.md`](Coding-Standards.md) before making code changes.** It is the authoritative repository coding standard, including naming, formatting, source organization, ownership, error handling, portability, performance, and review requirements. Do not replace its detailed rules with the summary below.

When instructions conflict, follow higher-priority instructions and repository-specific configuration; identify any unresolved conflict rather than silently choosing a convention. Check for more specific instructions within a subdirectory before editing there.

## Engineering expectations

- XII is a modular, multi-platform **C++23** engine for real-time applications, not exclusively a graphics engine. Preserve module boundaries and avoid graphics-specific assumptions in general-purpose code.
- Favor correctness, straightforward design, explicit dependencies, and enforceable invariants. Do not add speculative abstractions, caches, registration layers, or configuration options.
- Make ownership, borrowing, invalidation, synchronization, and resource lifetime clear. Follow the engine's allocator, container, and handle contracts.
- **Do not use C++ exceptions for ordinary engine errors.** Report recoverable failures through `xiiResult` or an appropriate explicit result type. Use assertions for programmer contract violations; validate untrusted input at runtime.
- Use existing XII types, facilities, and naming conventions where appropriate. Verify actual signatures, macros, and semantics in the source rather than assuming an illustrative example is a real API.
- Preserve cross-platform behavior, build variants, DLL/export boundaries, reflection, serialization, plugin compatibility, and asynchronous CPU/GPU lifetime guarantees when relevant.
- Optimize from evidence. Avoid unnecessary allocations in measured hot paths without sacrificing correctness or maintainability.

## File layout and formatting

- Honor the root `.clang-format` and `.editorconfig` configuration.
- Preserve the required license/copyright notice at the top of source files.
- In headers, put `#pragma once` after the license notice, followed immediately by the owning module's `DLL.h` where applicable.
- In implementation files using a PCH, put the module PCH as the first include after the license notice.
- Place `.cpp` files in the adjacent **`Implementation/`** directory beneath their corresponding headers, following existing module patterns.
- Keep public headers self-sufficient and include direct dependencies; do not rely on a PCH or accidental transitive includes.
- Avoid unrelated formatting or refactoring in functional changes.

## Workflow for changes

1. Inspect the affected code, its callers, tests, neighboring conventions, and relevant portions of `Coding-Standards.md`.
2. Identify the responsible module, public contracts, ownership/lifetime assumptions, supported configurations, and failure paths.
3. Make the smallest cohesive change that addresses the requirement; update tests and documentation as appropriate.
4. Run the applicable formatter, build, tests, and static checks supported by the environment. Never claim a check passed unless it ran.
5. Report what changed, relevant compatibility or performance considerations, tests actually run, and any checks you could not perform.

For detailed contributor and reviewer checklists, consult **§13 of `Coding-Standards.md`**. For exceptions and standards maintenance, consult **§14**.
