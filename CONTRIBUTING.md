# Contributing to XII

Start with the [build guide](Documentation/Building.md) and
[coding standards](Coding-Standards.md). XII is a layered native engine; a change
to Foundation, rendering, or a public interface can affect many consumers.

## Development setup

Clone recursively and keep submodules at the revisions recorded by the parent
repository. Do not use `git submodule update --remote` as routine setup. Install
the toolchain for the platform you intend to change. CPU presets offer a smaller
starting point than the full editor solution and match hosted CI.

Use Debug for assertions and development diagnostics, Dev for optimized development,
and Shipping to verify release behavior. Keep generated files under `Workspace`
and `Output`. Build instructions and commands live in one place:
[Building.md](Documentation/Building.md).

## Engineering expectations

- Use C++23 with the existing `xii` types, naming, export macros, error handling,
  allocators, and reflection conventions. Follow neighboring code and the root
  `.clang-format` / `.editorconfig`; avoid unrelated formatting changes.
- Keep Foundation independent of higher engine layers. Put reusable systems in
  their owning module and respect existing public/private link dependencies.
- Preserve header/source organization, PCH conventions, and `Implementation`
  directories. Platform implementations belong behind the existing platform
  abstractions and feature macros.
- Keep graphics interfaces backend independent. Backend upload layouts, resource
  states, synchronization, and lifetime changes require actual device validation.
  ABI assertions or a configure step are insufficient evidence of runtime behavior.
- Add regression coverage for behavior changes using the existing TestFramework.
  Classify new suites as headless, hardware dependent, or interactive in CMake.
  Do not label a graphics/device test headless merely because its GUI is disabled.
- Update API comments, documentation, serialization versions, and affected consumers
  when changing interfaces or persisted formats. Build consumers of foundational changes.
- Preserve copyright, license notices, and third-party attribution. Discuss a
  dependency addition with its purpose, version, license, and update mechanism.

## Checks before submitting

Build and run the applicable CPU suites through CTest, and test both Debug and
Shipping when behavior depends on assertions or optimization. Run the relevant
graphics or editor tests on suitable hardware for those changes. Include backend,
driver, SDK, toolchain, and command details in the PR; distinguish executed tests
from unavailable environments.

The [quality-check commands](Documentation/Building.md#quality-checks) validate
configuration and changed source lines. Formatting checks provide a patch and
never push to contributor branches. Compiler warnings retain their existing
error policy. Compiler analysis and CodeQL complement code review; they are not
proof that a change is safe.

## Issues and pull requests

Use the issue forms for reproducible defects, build failures, documentation gaps,
and feature proposals. Include a commit, exact commands, relevant options, logs,
and expected/observed behavior. Redact secrets and private paths from logs.
Report security vulnerabilities [privately](SECURITY.md).

For substantial architecture or public API changes, open an issue describing the
problem, affected layers, alternatives, and validation plan before a large rewrite.
Keep implementation PRs focused and reviewable. Use a topic branch against
`Development`; keep unrelated edits out of the PR.

Commit messages follow `ModuleName: Feature.` (for example,
`GraphicsCore: Validate pipeline resource reloads.`). Explain the resulting
behavior, tests, known limitations, and compatibility impact in the PR template.
Respond to review with code or evidence and keep documentation synchronized.
Reviewers may request additional platform validation before merge. No particular
review or response time is guaranteed.

Be constructive and respectful in technical discussions. Do not include personal
attacks, harassment, or private information in issues or reviews.
