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

## Authorial voice and external communications

When producing pull request descriptions, commit messages, changelogs, release notes, documentation, or other repository-facing content, agents **MUST** write from the perspective of the developer responsible for the work.

1. **Authorial representation:** Write as though the developer authored the content. Never refer to the developer as "the user" or describe the developer's interactions with an AI assistant.
2. **Conversational privacy:** Never disclose, summarize, quote, or reference private conversations, prompts, instructions, or clarifications in repository-facing content.
3. **Engineering substance:** Describe the implementation, technical rationale, design decisions, limitations, and verification results. Explain what changed and why, rather than narrating how requirements were communicated.
4. **Professional communication:** Maintain a direct, technically precise engineering voice. Avoid AI-generated commentary, conversational framing, unnecessary disclaimers, and self-referential statements.
5. **Factual integrity:** Never fabricate decisions, implementation details, test results, or verification claims. Clearly distinguish verified outcomes from outstanding work.
6. **Consistent application:** Apply these requirements regardless of which agent performs the work or which tool generates the content.

**Principle:** Repository-facing communication documents engineering work, not conversations between developers and their tools.

## Workflow for changes

1. Inspect the affected code, its callers, tests, neighboring conventions, and relevant portions of `Coding-Standards.md`.
2. Identify the responsible module, public contracts, ownership/lifetime assumptions, supported configurations, and failure paths.
3. Make the smallest cohesive change that addresses the requirement; update tests and documentation as appropriate.
4. Run the applicable formatter, build, tests, and static checks supported by the environment. Never claim a check passed unless it ran.
5. Report what changed, relevant compatibility or performance considerations, tests actually run, and any checks you could not perform.

For detailed contributor and reviewer checklists, consult **§13 of `Coding-Standards.md`**. For exceptions and standards maintenance, consult **§14**.

## Required branch, commit, and pull request process

Follow this process for every requested repository change, including documentation changes, unless the user explicitly specifies a different workflow. A request describing a feature or fix authorizes carrying the work through implementation, validation, commits, push, and opening a pull request. Do not stop at a proposal or ask for redundant confirmation between these steps. This does not authorize merging the pull request.

1. **Understand the description and inspect the checkout.** Identify the intended behavior, affected module, applicable instructions, and existing conventions. Inspect Git status before editing and preserve unrelated local changes throughout the task.
2. **Create a feature branch.** Use `TE/FeatureName` with a short, descriptive feature name. Fetch the target branch and start from its latest remote state; the current integration branch is `Development`. Verify the repository's current target branch before relying on that default. When continuing an existing task or pull request, reuse its appropriate feature branch. Do not accidentally include another feature branch's unmerged changes.
3. **Implement and commit incrementally.** Make small, cohesive changes and create focused commits at meaningful checkpoints using the exact format `ModuleName: Feature.` (including the final period). Stage only the files belonging to the task. Do not include unrelated user changes, generated build artifacts, or private local configuration. Do not split a trivial change artificially just to increase the commit count.
4. **Validate the final change.** Run the applicable formatter, builds, tests, and static checks. Inspect the final diff for scope and whitespace issues. Record the checks actually run, failures, and any verification gaps; do not claim success from builds or UI appearances alone.
5. **Push and open a pull request.** Push the feature branch and open a PR against the verified integration branch. Inspect recent human-authored PRs and follow their existing title and body scheme. Describe the final problem and behavior, key changes, and actual validation results, including failures or outstanding checks.
6. **Assign and set the milestone.** Assign the PR to Theophilus Eriata (`theoeriata`) and set milestone **number 1** (currently titled `1.0.0`). Resolve the current milestone title from GitHub rather than treating the number as its title.
7. **Verify and report completion.** Verify the PR's base and feature branches, title, body, assignee, milestone, and intended commit list. When running in the Codex app, attach the created PR to the chat using the available artifact tool. Return the PR link and a concise account of the change, validation, and material limitations. Never merge without explicit user authorization.
