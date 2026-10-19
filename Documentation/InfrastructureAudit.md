# GitHub infrastructure modernization report

Audit and local verification date: 2026-10-18. Baseline commit:
`c1cad315e2eee4da62a16117d48ba661c946c81c`. Implementation branch: `TE/Maintenance`.
The repository was initially clean. License and attribution were preserved.

## 1. Audit findings

- The actual default branch is `Development`; original jobs also targeted `main`
  and `dev`. Two scheduled Windows workflows duplicated expensive work.
- Original release scripts built Dev, used obsolete executable/shader names,
  ignored shader errors, and broadly deleted symbols and test data before packaging.
- Formatting/copyright jobs modified contribution branches, used moving action
  references and unnecessary write credentials, and mishandled fork refs.
- Clang-Tidy assumed an old LLVM download plus custom tools and did not reliably
  propagate findings as failures. No workflow/YAML quality gate existed.
- CTest was integrated but lacked explicit hardware labels, complete-suite
  selection, isolated diagnostics, and reliable out-of-source asset discovery.
- Qt 6.11 and x64 Windows/Linux DXC paths differ from old workflow assumptions.
  D3D12 is deliberately disabled. macOS platform files do not establish a usable
  whole-engine or test-framework build.
- CONTRIBUTING was a placeholder, support referenced a nonexistent template,
  security promised a response deadline, and the README overstated platform support.
- There is no validated binary SDK installation/distribution contract or complete
  generated SBOM. The existing license says All Rights Reserved, not an open-source grant.

## 2. Implemented improvements

**CI and build integration.** Added an isolated CPU build filter and CMake presets;
preserved ordinary developer presets. A reusable native workflow builds and runs
the actual Foundation/Core/ToolsFoundation suites through CTest. An explicit
discovery check rejects missing/extra suites, including an empty selection.
Debug and Shipping use the engine's native configuration names. C++23 fallback
is disallowed. CTest receives the source SDK root, avoids saved test selection,
runs shared-fixture suites serially, labels headless/hardware/interactive suites,
and emits native JSON/HTML plus JUnit diagnostics.

**Efficiency and quality.** Replaced duplicate nightly builds with a representative
PR matrix and weekly extended checks. Added bounded compilation, cancellation,
seven-day diagnostics, a bounded Linux compiler-object cache and no unused CPU
submodule payloads. Added checksum-verified actionlint, ShellCheck integration,
YAML lint, local documentation links, action-pin/permission contracts, and
read-only changed-line formatting/copyright checks. Formatter errors fail and
suggestions remain patches; filenames with spaces and `.inl` files are covered.

**Security.** All external actions have immutable commit pins. PR build jobs have
read-only contents access, no publication secrets and no persisted Git credentials.
CodeQL analyzes first-party C/C++ and Actions in build-free mode. Dependency review
rejects newly introduced high/critical known dependency issues where GitHub's
dependency graph supports them. Dependabot manages action and submodule revisions.
The labeler is a metadata-only privileged PR job with no checkout or shell code.
Security reporting retains the existing contact and adds conditional private GitHub
reporting without invented deadlines or historical-version support guarantees.

**Releases and auxiliary publication.** Explicit matching numeric version tags,
default-branch ancestry, Windows/Linux Shipping CPU checks and quality checks gate
an attested, checksummed **draft source release** behind the `release` environment.
Only the same run's fixed-name artifact enters the privileged job; engine code
does not execute there. Source manifests preserve submodule identities without
claiming to bundle them. API generation keeps artifact-only mode by default;
optional reviewed publication uses a separately scoped environment token and a
normal push. Precompiled tool automation builds an ArchiveTool Shipping candidate
for review rather than force-pushing a broad collection into another repository.

**Documentation and community.** Rewrote README, CONTRIBUTING, SECURITY and SUPPORT;
added authoritative build and infrastructure guides and this verification record.
Updated the documentation index and coding-standard enforcement claims. Added a
PR template, build/platform form and issue-form configuration; enriched bug reports
and corrected malformed dropdown option values and stale labeler paths.

**Validation-driven test repairs.** Native execution exposed real issues, all fixed
without disabling tests or weakening engine warning policies:

- SDL-disabled screen/input/window stubs had unused parameters and a duplicate
  callback already implemented in common Core code.
- TestFramework's JSON message-name table omitted an enum value, causing a final
  result serialization crash. A compile-time size check now prevents recurrence.
- Generated `Data/Base` build directories shadowed source assets; CTest now passes
  an explicit source SDK root.
- Process fixtures bypassed core-system startup, then failed during teardown.
  They now follow the normal startup/shutdown lifecycle.
- A directory-watcher fixture assumed unobserved intermediate changes could not
  coalesce on ReFS. It now polls before recreating the deleted destination, still
  requiring all four events.
- A logging expectation assumed development messages existed in Shipping. The
  expectation follows the compiled feature while retaining output assertions.

## 3. CI support matrix

These are **configured hosted jobs**, not claims of successful hosted execution:

| Runner / architecture | Compiler | Configuration | Tests / purpose |
| --- | --- | --- | --- |
| windows-2025-vs2026 / x64 | MSVC, VS 2026 | Debug, Dev, Shipping | PR CPU suites |
| ubuntu-26.04 / x64 | GCC 15 | Debug, Dev, Shipping | PR CPU suites |
| ubuntu-26.04 / x64 | Clang 22 / LLD 22 | Debug, Dev, Shipping | PR CPU suites |
| macos-26 / ARM64 | Apple Clang, stable Xcode 26.6 | Debug, Dev, Shipping | PR CPU suites |
| windows-2025-vs2026 / x64 | MSVC with `/analyze` | Debug | Weekly/manual CPU suites and compiler analysis |
| Windows + Linux above | MSVC + GCC 15 | Shipping | Tag validation gates |

GPU devices, full editor flows and sanitizer configurations are excluded from this
hosted matrix. macOS ARM64 coverage applies to the three CPU suites only.

## 4. Verification results

Actual local native results (each row includes compilation and all three CTest suites):

| Environment | Compiler / CMake | Configuration | Result |
| --- | --- | --- | --- |
| Windows x64, ReFS development drive | MSVC 19.52.37025, VS 2026 / CMake 4.4.3 | Debug | 3/3 passed |
| Windows x64, same toolchain | MSVC 19.52.37025 / CMake 4.4.3 | Shipping | 3/3 passed |
| Ubuntu 26.04 x64 under WSL, native Linux scratch filesystem | GCC 15.2 / CMake 4.2.3 | Debug | 3/3 passed |

The local toolchains are not the exact hosted image snapshots. Their results do not
certify the hosted image versions, Clang 22, `/analyze`, or GitHub runtime behavior.
The Windows Shipping ArchiveTool target also compiled successfully.

The requested current-environment follow-up selects VS 2026 directly in
`ci-windows` and uses `windows-2025-vs2026` for every Windows workflow. All Linux
workflows now select Ubuntu 26.04, with GCC 15 and Clang 22 / LLD 22 centralized in
the CI presets. GitHub's current runner manifests confirm these hosted toolchains:
[Windows](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md)
and [Ubuntu](https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2604-Readme.md).
The VS 2026 preset was reconfigured without a generator override, rebuilt, and
passed all three Debug suites. The explicit GCC 15 preset was also configured.
Clang 22 is available on the hosted image but was not executed locally: installation
was blocked by an existing interrupted dpkg operation involving Perl packages in
the WSL environment. No unrelated package repair was performed.

Other completed checks:

- actionlint 1.7.12 on Windows and native Linux; Linux included ShellCheck.
- YAML lint of `.github` and its lint configuration.
- Full changed-source check using the pinned custom clang-format 21.0.0.
- Repository pins, reusable workflow paths, permissions boundaries, preset parent
  references, documentation links and Git whitespace checks.
- Seven infrastructure integration tests on Linux: clean formatting, formatter
  failure propagation, read-only suggestions, exact source snapshots/checksums,
  submodule exclusion/revision recording, version mismatch and tag/HEAD mismatch.
  The four packaging cases also passed on Windows; POSIX formatter fixtures are
  intentionally platform-specific and were executed on Linux.
- Expected hosted suite discovery and a negative check proving empty discovery fails.
- Inspected complete diffs and confirmed engine changes are limited to the
  SDL-disabled stubs; remaining C++ changes belong to the test infrastructure/fixtures.

Local detailed logs and JUnit files are retained under the ignored `Workspace`
directory with the `infrastructure-` prefix. Accepted native run logs are
`infrastructure-ctest-windows-debug-passed.log`,
`infrastructure-ctest-windows-shipping-passed.log`, and
`infrastructure-ctest-linux-debug-passed.log`; corresponding XML files have the same stems.

**Hosted execution blocker:** dispatching the existing `build-and-test.yml` returned
HTTP 422 because it is manually disabled in GitHub. Its replacement must be merged
and re-enabled before hosted certification. New workflows, CodeQL upload, dependency
review, protected publication, OIDC attestation and artifact transfer have not been
executed on GitHub during this task. No release/tag was created or published.

## 5. GitHub configuration requirements

See [the exact maintainer settings](Infrastructure.md#maintainer-settings): protect
`Development`, re-enable Native CI after merge, establish required aggregate/quality
checks, retain read-only default token permissions and fork approvals, enable
dependency graph/code scanning/private vulnerability reporting, restrict version
tags, and configure reviewed `release` and optional `documentation` environments.
These repository controls were not silently changed. A named environment in YAML
does not itself establish protection. Optional API publication needs a narrowly
scoped `API_DOCS_TOKEN` for the existing documentation repository.

## 6. Remaining limitations

- Hosted image/compiler validation remains required after the disabled workflow
  is enabled. Establish the existing CodeQL findings baseline before making scans
  required. Build-free scanning cannot replace a successful native build.
- No GPU/editor/macOS/ARM/runtime certification, sanitizer support, binary SDK,
  reproducibility guarantee or complete vendored-dependency CVE coverage is claimed.
- Existing CMake SDK downloads still lack a comprehensive checksum policy; CPU CI
  avoids those downloads. Vendored dependencies need human updates and license review.
- Broader precompiled editor/renderer tools require their own migration and runtime
  checks before external publication. Old Azure inputs remain historical and
  unvalidated; the new build guide is authoritative for supported procedures.
- Formal community governance and a new license grant require project-owner
  decisions; neither contacts nor enforcement commitments were invented.

## 7. Files changed

- Added workflows: `native-build.yml`, `code-quality.yml`, `security.yml`,
  `extended-validation.yml` under `.github/workflows`.
- Added `.github/actionlint.yaml` to recognize the verified Ubuntu 26.04 hosted
  label while retaining the runner-label rule in actionlint 1.7.12.
- Replaced/hardened: `build-and-test.yml`, `build-release-package.yml`,
  `update-api-docs.yml`, `update-precompiled-tools.yml`, `pull-request-labeler.yml`.
- Removed superseded workflows: `continuous-integration.yml`, `clang-format.yml`,
  `clang-tidy.yml`, `ensure-copyright-header.yml`.
- Added security/community/configuration files: `.github/codeql.yml`,
  `.github/dependabot.yml`, `.github/PULL_REQUEST_TEMPLATE.md`,
  `.github/ISSUE_TEMPLATE/build_failure.yml`, `.github/ISSUE_TEMPLATE/config.yml`,
  `.yamllint.yml`; updated existing bug/feature forms and labeler configuration.
- Reworked root README/contribution/security/support/coding documents; added
  `Documentation/Building.md`, `Documentation/Infrastructure.md` and this report;
  updated `Documentation/Readme.md` and Python line-ending attributes.
- Added `HeadlessTests.BuildFilter`, CI presets, CTest classification/arguments and
  required C++23 flags; updated the four applicable test registrations.
- Narrow C++ fixes: three SDL-disabled stub files, TestFramework runner/results,
  Foundation test entry point, watcher fixture and logging expectation.
- Added four CI scripts and two regression-test modules under `Utilities/CI`;
  repaired the existing `Utilities/ClangFormatDiff.py` parser.

For the complete path-level change list, use
`git diff --name-status c1cad315e2eee4da62a16117d48ba661c946c81c..TE/Maintenance`.

## Workflow retirement and publication contention follow-up

Removed `update-precompiled-tools.yml`: the replacement built only an ArchiveTool
candidate and did not provide a complete precompiled-tools distribution/update
contract. Local ArchiveTool builds remain documented. Eight workflows remain,
with distinct CI, reusable build, extended validation, quality, security, release,
PR metadata and API documentation responsibilities.

Using `gh`, disabled the registered legacy ClangFormat, Ensure Copyright Header
and Update Precompiled Tools workflows. Readback confirmed they are disabled;
the old ClangTidy and Continuous Integration entries were already disabled.
Historical runs were preserved. Native CI's pre-existing disabled state was not
changed; it still needs enabling after the replacement is merged into Development.

API documentation is the only remaining Git-commit publisher. Its tested utility
fetches the destination branch immediately before creating a commit, then fetches
and recreates the generated commit if a competing push wins. All outside-docs
history is retained and no push forces history. Five attempts bound contention;
permission/policy failures remain failures. Six real local Git integration tests
cover stale checkouts, overlapping generated-file pushes, identical publications,
permission rejection, retry exhaustion and protection of dirty checkouts.

Validation: all 23 infrastructure tests passed in WSL; actionlint, yamllint,
label catalog/module coverage, action pins, workflow permission boundaries,
documentation links and diff whitespace checks passed. No production API
documentation was published and the revised workflow was not run on hosted
runners. The workflow changes take effect after merge into the base branch.

## Build and Test matrix expansion

The primary workflow is now **Build and Test**, with the aggregate check
**Build and Test required**. Debug, Dev and Shipping each run with Windows VS 2026
MSVC, Linux GCC 15, Linux Clang 22 / LLD 22, and macOS 26 ARM64 Apple Clang from
the image's stable Xcode 26.6. This is twelve jobs, limited to six simultaneous
builds. Clang Shipping moved out of extended validation because the primary matrix
now covers it; weekly MSVC compiler analysis remains distinct.

The macOS preset reuses the CPU filter and existing Makefile platform support.
The port repairs stale platform macros, supplies the existing non-Windows
DirectXTex compatibility headers, uses Mach VM statistics, supports Darwin's
semaphore API and correct CLI executable paths, and adds missing standard headers
required by libc++. Existing capability flags remain explicit; no tests were
disabled to obtain passing results. The fallback message-loop constructor now
uses the declared implementation class.

Expanded validation exposed signed-zero bone hashes, a bit-exact Perlin regression
that rejected optimized GCC output, and an assertion-only image variable rejected
by Clang Shipping. The bone hash canonicalizes zero through integer bits. The
Perlin test retains the six original reference hashes to verify its compact
float32 reference fixture, then checks all 1,536 computed samples for finiteness
and agreement within 1e-6. Noise uses normal unity compilation and compiler
optimization settings; no per-source compiler overrides remain. The image check
compiles its validation only in development configurations. Warning policies
and failure propagation remain unchanged. New semaphore and executable-path
checks cover the platform behavior repaired here.

Local VS 2026 Shipping and Dev builds each passed FoundationTest, CoreTest and
ToolsFoundationTest. Hosted macOS Debug built and passed those same suites in
[the focused platform validation](https://github.com/XII-Technologies/XII/actions/runs/39207255628).
The temporary port-validation workflow has been removed; required coverage lives
in the primary matrix. Full-matrix results are recorded after its final run.
