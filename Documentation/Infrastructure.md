# Repository infrastructure

## Audit baseline

The 2026-10-18 audit verified that `Development` is the public repository's default
branch. The starting automation targeted a mixture of `dev`, `main`, and
`Development`; two nightly jobs duplicated Windows builds. Release packaging used
Dev binaries, deleted symbols/test data broadly, and invoked obsolete shader/tool
names with ignored errors. Some jobs used a broad PAT, moving action references,
submodule `--remote` updates, or force pushes to auxiliary repositories.

Formatting/copyright jobs modified PR branches, mishandled fork refs and filenames,
and carried write credentials into contribution execution. Clang-Tidy depended on
an old downloaded LLVM and custom tooling but did not reliably fail on findings.
There was no action lint, dependency update strategy, or meaningful cross-platform
test matrix. CTest existed, but did not label hardware requirements or force all
tests to execute without saved user state. The README overstated platform support,
CONTRIBUTING was a placeholder, and support referenced a nonexistent issue template.

Build evidence: C++23 target flags; Debug/Dev/Shipping configurations; MSVC and
GCC/Clang platform hooks; Qt 6.11 requirement; x64 Windows/Linux DXC downloads;
D3D12 forced OFF; native CTest integration; public SDL/PrecompiledTools submodules;
vendored third-party dependencies and `cgmanifest.json`. No CPack/installable SDK
contract or validated cross-platform binary packaging was found. The license
remains All Rights Reserved. Old Azure pipeline definitions contain stale `EZ_*`
options and branch names and are retained as historical external-service inputs,
not the supported GitHub/local build procedure.

## Workflow responsibilities and matrix

| Workflow | Trigger | Scope |
| --- | --- | --- |
| `build-and-test.yml` | PRs, Development pushes, manual | Build and Test: Windows x64 VS 2026 MSVC, Ubuntu 26.04 x64 GCC 15 / Clang 22, macOS 26 ARM64 Apple Clang; each in Debug, Dev and Shipping; FoundationTest, CoreTest, ToolsFoundationTest |
| `native-build.yml` | Reusable workflow only | Preset configuration, bounded compilation, test discovery, CTest, diagnostics, Linux ccache |
| `code-quality.yml` | PRs, Development pushes, manual, release call | Actionlint, YAML, pins/permissions/local links, changed-line formatting and copyright on PRs |
| `security.yml` | PRs, Development pushes, weekly, manual | CodeQL C/C++ and Actions; PR dependency review at high severity |
| `extended-validation.yml` | Weekly, manual | Windows VS 2026 MSVC Debug `/analyze`; Clang now runs in the primary matrix |
| `build-release-package.yml` | Explicit `v*` tags | Exact version/default-branch ancestry check; Windows/Linux Shipping CPU gates; quality gate; attested draft source release |
| `pull-request-labeler.yml` | PR metadata changes | Labels using base-branch configuration; no checkout or shell execution |
| `update-api-docs.yml` | Manual | Generate Doxygen HTML artifact; optional protected cross-repository publication |

The required aggregate job fails when any native matrix leg fails or is cancelled.
All twelve compiler/configuration combinations run for every PR and relevant push,
with up to six jobs running concurrently. The displayed aggregate check is
**Build and Test required**; update any branch rule that still names the former
**Native CI required** check. Compiler versions track the stable toolchains
available in the selected hosted images; preview Xcode/toolchains are not selected.
The standalone ArchiveTool candidate workflow was removed: it did not implement
the former precompiled-tools update process. ArchiveTool remains buildable with
the existing CPU preset, as described in the [build guide](Building.md).

Legacy formatting/copyright workflows are superseded by Repository quality;
the old monolithic CI and Clang-Tidy workflows are superseded by native/extended
validation and security analysis. Disable their registered GitHub workflows while
the branch replacement awaits merge. Historical runs remain available.

## Pull request labels

[`.github/labels.json`](../.github/labels.json) owns repository label names, colors
and descriptions, with path rules for `Area:`, `Module:` and `GitHub`.
Area labels describe cross-cutting concerns; module labels identify the engine,
tool library, editor or plugin family that owns the changed code. Tests and test
assets also select their owning module. Standalone tools and samples use area
labels; `TestFramework` has its own module label. Issue-form area selections are
report metadata and do not automatically apply GitHub labels.

After adding or renaming a module, update the catalog and regenerate the labeler:

```sh
python Utilities/CI/manage_labels.py generate
python Utilities/CI/manage_labels.py check
```

Repository quality checks reject stale generated rules, obsolete directory paths
and missing module coverage. Keep issue-form area choices aligned when adding
areas. To synchronize live labels with an authenticated maintainer's `gh` CLI:

```sh
python Utilities/CI/manage_labels.py sync --repo XII-Technologies/XII
python Utilities/CI/manage_labels.py sync --repo XII-Technologies/XII --apply
```

The first command previews changes. The second creates or updates managed label
metadata and verifies the result; it never deletes labels or changes issue/PR
assignments. Existing `Status:`, `Change:` and `Review:` metadata is preserved in
the catalog; these labels are assigned manually and have no automatic path rules.
The legacy `.github/create-tags.sh` entry point delegates to the same utility and
also previews changes unless passed `--apply`.
The metadata-only labeler runs from the PR base branch and synchronizes configured
path labels as files change, preserving unrelated labels. Catalog/workflow changes
take effect for PR automation once merged into the base branch. No PR code is
checked out or executed by this privileged workflow.

## API documentation publication

Only `update-api-docs.yml` pushes Git commits, to the existing APIDocumentation
repository. Publication remains opt-in from `Development` behind the documentation
environment. The publisher checks out the trusted source revision separately
from the disposable destination checkout and downloads only this run's HTML.

Before committing, `Utilities/CI/publish_api_docs.py` fetches the latest destination
`Development` branch. It replaces only the generated `docs/` directory, retaining
all other destination files and commits. If another writer advances the branch
before the push, it fetches again and recreates the generated documentation commit
on that new tip, up to five attempts. This also handles overlapping edits within
generated documentation without rebase conflicts. Identical output is a successful
no-op. Pushes never force history; authentication/policy failures still fail, and
continuous contention beyond the retry bound produces an explicit rerun request.
The published content identifies its original source SHA in the commit message;
retrying publication does not regenerate HTML from newer engine source.

## Hosted environments

Windows jobs explicitly select `windows-2025-vs2026`; all Linux jobs select
`ubuntu-26.04`, including quality, security, documentation, and release publication.
Apple Clang jobs select `macos-26` (ARM64) and the image's default stable Xcode;
the current image uses Xcode 26.6. The macOS preset uses Makefiles, supported by
the existing platform hooks, and needs neither Homebrew LLVM nor GCC masquerading
behind Apple's `gcc` alias. GCC and upstream Clang are covered on Linux; MSVC is
covered on Windows, where it is supported.
These are the latest suitable x64 hosted environments verified on 2026-10-18 in
[GitHub's runner inventory](https://github.com/actions/runner-images). Explicit
labels avoid the older Ubuntu image still used by `ubuntu-latest` during migration.
The CI Windows preset requires CMake 4.2+; legacy developer presets remain available.
Actionlint 1.7.12 predates the Ubuntu 26.04 label, so `.github/actionlint.yaml`
explicitly allows that verified hosted label without disabling runner-label validation.
PR workflows run without secrets, with read-only contents permissions and checkout
credentials disabled. The metadata-only labeler uses `pull_request_target` solely
to apply labels to forks; it never checks out or executes their code.

All external actions use full commit pins with version comments. Dependabot groups
weekly action updates and proposes monthly submodule updates. Vendored libraries,
CMake SDK downloads, distro packages and custom tools still require human review:
dependency review does not provide CVE coverage of arbitrary copied C++ sources.
`cgmanifest.json` is an existing inventory, not a complete generated SBOM.

CodeQL uses build mode `none`, allowing security analysis of first-party source
without Qt/editor or SDK builds. Its inferred compilation context has lower fidelity
than a successful traced native build, especially for generated/platform code; it
does not certify compilation. See [GitHub's compiled-language guidance](https://docs.github.com/en/code-security/reference/code-scanning/codeql/build-options-for-compiled-languages).
Do not also enable conflicting CodeQL default setup when using this advanced workflow.

## Resource policy

PR builds deliberately avoid a full OS/compiler/configuration Cartesian product.
Superseded PR builds cancel, matrix failures retain independent diagnostics, and
compilation is bounded at two build processes with MSVC `/MP2`. CPU builds avoid
Qt/SDL/DXC downloads and does not fetch unused submodule payloads. Linux ccache stores objects, not build trees, with separate
platform/architecture/compiler-preset/configuration/build-definition keys and
ccache's compiler/flag validation. Fork caches are scoped by GitHub and are not
promoted into a binary release pipeline. Release tags do not restore PR build trees.

Build/test diagnostics are retained seven days, manual candidates and release
staging artifacts fourteen days. Successful PR binaries are not uploaded. Review
cache size and runner memory from actual telemetry before increasing parallelism.
No reproducible-build guarantee has been established.

## Releases

`version.txt` is the release identity, currently a three-part numeric version.
Create an explicit `vMAJOR.MINOR.PATCH` tag at the reviewed commit after updating
that file. The workflow rejects a mismatched version, a tag that does not identify
HEAD, or a commit outside `Development` ancestry. Tag creation is a maintainer
operation; no PR merge publishes a release and this modernization creates no tag.

After Shipping CPU tests and repository-quality checks, the protected `release`
environment allows a privileged job to consume **only the fixed-name artifact from
the same run**, verify SHA-256 checksums, attest the source archive with GitHub OIDC,
and create a **draft** GitHub Release with generated notes. No engine code executes
in that publishing job. A maintainer reviews the draft, validation logs, licensing,
known issues and notes before publishing it manually.

The files are `XII-<version>-source.tar.gz`, a source manifest with the exact parent
commit and submodule revisions, and `SHA256SUMS`. Source archives exclude submodule
payloads; cloning recursively remains the supported build setup. These are source
snapshots, not SDKs or prebuilt engine distributions. Verify the downloaded archive
with `sha256sum --check SHA256SUMS` and
`gh attestation verify XII-<version>-source.tar.gz --repo XII-Technologies/XII`.

Binary distributions remain deferred until install/export layout, runtime libraries,
assets, third-party license payloads, platform smoke tests, and debug-symbol
packaging are established. The former broad copy-and-delete Dev package was not
a defensible release artifact. The obsolete full PrecompiledTools filter needs a
separate Qt/renderer migration, packaging contract and runtime validation before
an updater can be restored safely.

## Maintainer settings

Repository administrators must configure these controls; workflow files cannot
enforce repository settings themselves:

1. Protect `Development`: PR review, required **Build and Test required** and
   **Repository quality**, and no force pushes. Add CodeQL/dependency-review gates
   once the initial scans establish the existing findings baseline. Confirm exact
   displayed check names after the first hosted run. Merge queues require adding
   `merge_group` triggers before enabling them.
   The existing `build-and-test.yml` workflow was manually disabled at audit time,
   but is now enabled. Keep it enabled when merging the renamed workflow.
2. Default Actions token permissions to read-only; restrict allowed actions and
   require immutable SHA references where organization settings support it. Keep
   approvals for first-time fork contributors. Do not expose write secrets to PR jobs.
3. Enable the dependency graph, Dependabot alerts, code scanning and private
   vulnerability reporting. The repo is public; private forks may need appropriate
   GitHub security licensing. Enable push protection/secret scanning where available.
4. Create a **release** environment with required reviewers, prevent self-review
   where supported, and permit protected `v*` tags only. Use tag rulesets to restrict
   tag creation/update/deletion to maintainers. Without these settings GitHub may
   create an unprotected environment automatically; the YAML name is not approval.
5. For optional API publication, create a **documentation** environment restricted
   to `Development`, require review, and store `API_DOCS_TOKEN` there. Use a scoped
   token with contents write only on XII-Technologies/APIDocumentation. The build
   job needs no token. Publication uses a normal push, never a force push.
6. Do not attach persistent self-hosted GPU machines to untrusted fork workflows.
   A future hardware workflow needs maintainer-reviewed commits, an isolated runner
   group, disposable workspaces, installed SDKs/drivers, an active desktop if required,
   and device-feature preflight with explicit failure for unavailable mandatory features.

## Deferred validation

Hosted CPU jobs are not graphics/editor certification. The macOS CPU preset and
dependency adaptations cover the same three suites as Windows/Linux. This does
not certify the macOS editor, graphics runtime or unsupported platform services.
Linux editor and native Vulkan need dedicated runtime validation. Stock Clang-Tidy
does not contain the project's `xii-name-check` plugin, so the brittle autofix gate
is replaced with existing compiler warnings, MSVC analysis and CodeQL.

No repository sanitizer contract exists. ASan/UBSan/LSan require consistent
instrumentation of DLLs/dependencies and validation of custom allocator, plugin
loading and teardown paths; TSan also needs concurrency-suite/runtime qualification.
No sanitizer job is presented as supported without that evidence. SDK downloads
in the existing CMake system still need checksum enforcement in a focused dependency
hardening task; CPU CI avoids those downloads rather than caching unchecked SDKs.

The [build guide](Building.md) gives authoritative commands. The local/hosted
verification record is maintained in the [modernization report](InfrastructureAudit.md); unavailable
environments and real failures must remain explicit.
