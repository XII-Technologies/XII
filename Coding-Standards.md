# XII Engine C++ Coding Standards and Engineering Guidelines

This guide defines what readable, correct, robust, maintainable, and performance-conscious C++ looks like in XII. A style guide cannot replace design review, tests, benchmarks, platform documentation, or sound engineering judgment. Its purpose is to make routine decisions predictable and to make exceptions explicit.

**Precedence when rules differ:** correctness and portability come first; then documented public API and serialized-format compatibility; then the repository's root `.clang-format`, `.editorconfig`, build settings, and target-specific conventions; then this document; then local neighboring code for cases this document leaves open. Do not reformat unrelated code merely to apply a new preference. This document does **not** claim to reproduce exact `.clang-format` or `.editorconfig` values, which must be read from the target checkout.

The words **MUST**, **SHOULD**, and **MAY** indicate mandatory rules, expected defaults (exceptions need a reason), and permitted alternatives. XII-specific APIs shown in examples are taken from the supplied engine code where possible. Examples labeled **illustrative** show a design pattern and may require adaptation to the exact current API.

## Contents

1. [Core principles](#1-core-principles)
2. [Source layout and formatting](#2-source-layout-and-formatting)
3. [Naming conventions](#3-naming-conventions)
4. [File organization and headers](#4-file-organization-and-headers)
5. [Types, classes, and interfaces](#5-types-classes-and-interfaces)
6. [Initialization, ownership, and lifetime](#6-initialization-ownership-and-lifetime)
7. [Functions, parameters, and return values](#7-functions-parameters-and-return-values)
8. [Control flow and expressions](#8-control-flow-and-expressions)
9. [Modern C++23 features](#9-modern-c23-features)
10. [Memory, allocators, and containers](#10-memory-allocators-and-containers)
11. [Strings, identifiers, hashes, and handles](#11-strings-identifiers-hashes-and-handles)
12. [Errors, assertions, diagnostics, and logging](#12-errors-assertions-diagnostics-and-logging)
13. [Concurrency and synchronization](#13-concurrency-and-synchronization)
14. [Performance-sensitive engine code](#14-performance-sensitive-engine-code)
15. [Graphics, render graphs, and GPU lifetimes](#15-graphics-render-graphs-and-gpu-lifetimes)
16. [Reflection, serialization, and public API compatibility](#16-reflection-serialization-and-public-api-compatibility)
17. [Platform, build, plugins, and subsystem lifecycle](#17-platform-build-plugins-and-subsystem-lifecycle)
18. [Comments and documentation](#18-comments-and-documentation)
19. [Testing, tooling, and code review](#19-testing-tooling-and-code-review)
20. [Common mistakes and corrected examples](#20-common-mistakes-and-corrected-examples)
21. [Reference examples](#21-reference-examples)
22. [Contributor and reviewer checklists](#22-contributor-and-reviewer-checklists)
23. [Maintenance and exceptions](#23-maintenance-and-exceptions)

## 1. Core Principles

### 1.1 Correctness Before Cleverness

- Code **MUST** express the intended semantics and maintain its documented invariants on all supported platforms.
- Prefer straightforward implementations over clever solutions that obscure ownership, ordering, lifetime, or failure behavior.
- Avoid introducing generic abstractions without a concrete consumer and a clearly defined responsibility.
- Do not introduce additional states, registration layers, configuration enums, or caches without a demonstrated need.
- Make invalid states difficult to represent. Validate untrusted inputs and enforce preconditions at system boundaries.
- Operations that can legitimately fail **MUST** expose their failure through an explicit mechanism, such as `xiiResult`. XII does not use C++ exceptions for ordinary error handling.
- Callers **MUST NOT** silently discard failures that affect correctness.

**Prefer:**

```cpp
xiiResult LoadConfiguration(xiiStringView sPath)
{
  xiiFileReader file;

  if (file.Open(sPath).Failed())
    return XII_FAILURE;

  return ParseConfiguration(file);
}
```

**Avoid:**

```cpp
void LoadConfiguration(xiiStringView sPath)
{
  xiiFileReader file;

  file.Open(sPath); // Failure is ignored.

  ParseConfiguration(file); // May operate on an unopened file.
}
```

The preferred implementation makes the operation's success or failure observable to the caller and prevents parsing when opening the file fails. It also propagates parsing failures rather than claiming success unconditionally.

#### Expected Failures vs. Contract Violations

Not every invalid argument warrants a `xiiResult`.

A distinction **MUST** be maintained between expected runtime failures and programming errors:

- **Expected runtime failures** arise during otherwise valid operations and must be handled explicitly. Examples include inaccessible files, malformed external data, unavailable network services, and resource initialization failures.
- **Contract violations** indicate incorrect use of an API or a violated internal invariant. These should be prevented through API design and detected using assertions or other appropriate contract-enforcement mechanisms.

For example, if a function requires a nonzero interval as part of its documented contract, and its callers are entirely under engine control, an assertion is more appropriate:

```cpp
void SetFrameInterval(xiiUInt32 uiInterval)
{
  XII_ASSERT_DEV(uiInterval > 0U, "Frame interval must be greater than zero.");

  m_uiFrameInterval = uiInterval;
}
```

If the interval originates from an external configuration file, the value should instead be validated at the configuration boundary, where invalid input can be reported as a recoverable failure.

**Error-handling requirements:**

- Use `xiiResult` for operations where success or failure must be communicated to the caller.
- Use assertions to detect violations of documented preconditions and internal invariants.
- Do not use `xiiResult` as a substitute for enforcing API contracts.
- Do not return `XII_SUCCESS` unless the operation has fulfilled its documented success conditions.
- Do not silently ignore a failed operation when subsequent execution depends on its success.
- Propagate failures to the appropriate boundary unless the current function can meaningfully handle or recover from them.
- Do not use `xiiResult` when callers require structured error information beyond success or failure. Use an appropriate result type or explicit error output instead.
- C++ exceptions **MUST NOT** be used for ordinary error handling. Exceptions are permitted only where unavoidable within deeply isolated platform-specific implementations and **MUST NOT** propagate across engine API boundaries.

**Guiding principle:**

> Expected runtime failures are values, not exceptions. Programming errors are contract violations, not ordinary runtime failures.

### 1.2 Make Ownership and Lifetime Visible

Ownership, lifetime, and access semantics **MUST** be explicit and consistent. A component should make it possible to determine who owns a resource, how long it remains valid, and under what conditions it may be accessed.

A reviewer should be able to answer:

- **Creation:** Who creates or acquires the object or resource?
- **Ownership:** Who is responsible for managing its lifetime?
- **Destruction:** Who destroys or releases it, and when?
- **Lifetime:** Can it outlive the object, subsystem, or context on which it depends?
- **Invalidation:** What invalidates its pointers, references, views, iterators, handles, or GPU resources?
- **Concurrency:** Which threads may access or modify it, and what synchronization is required?

Names, types, and interfaces **SHOULD** communicate these relationships without requiring callers to inspect implementation details.

#### Ownership Requirements

- Every dynamically managed object or resource **MUST** have a clearly defined owner or ownership policy.
- Prefer automatic storage and deterministic destruction where practical.
- Use RAII to associate resource acquisition with cleanup whenever ownership can be represented by object lifetime.
- Use `xiiUniquePtr` for exclusive dynamic ownership and `xiiSharedPtr` only when shared ownership is genuinely necessary.
- Use non-owning pointers, references, spans, and views for borrowed access. Their lifetime **MUST NOT** exceed that of the referenced storage.
- Use opaque, generation-checked handles for independently managed objects whose lifetime is controlled by a registry, manager, or subsystem.
- Do not expose owning raw pointers without an explicit and documented ownership-transfer contract.
- Avoid implicit ownership transfer, hidden lifetime extension, and unnecessary shared ownership.
- Destruction and release operations **MUST** preserve the owning component's invariants, including when an operation fails.

**Prefer:**

```cpp
class xiiDocument
{
public:
  explicit xiiDocument(xiiUniquePtr<xiiDocumentData>&& pData)
    : m_pData(std::move(pData))
  {
  }

  // Borrowed access. The reference is valid only while
  // the document and its data remain alive.
  const xiiDocumentData& GetData() const
  {
    return *m_pData;
  }

private:
  // Exclusive ownership with deterministic destruction.
  xiiUniquePtr<xiiDocumentData> m_pData;
};
```

**Avoid:**

```cpp
class xiiDocument
{
public:
  explicit xiiDocument(xiiDocumentData* pData)
    : m_pData(pData)
  {
  }

  // Ownership and lifetime are ambiguous.
  xiiDocumentData* GetData()
  {
    return m_pData;
  }

private:
  xiiDocumentData* m_pData;
};
```

The preferred implementation makes exclusive ownership explicit through the type system. The document controls the lifetime of its data, and consumers receive borrowed access without acquiring ownership.

The avoided implementation leaves callers uncertain whether the pointer is owned, borrowed, transferred, or expected to be deleted elsewhere.

#### Handles and Resource Lifetimes

Handles are appropriate when resources are managed independently of the objects that reference them.

- Handles **MUST NOT** imply ownership unless explicitly documented.
- A handle **MUST** be validated before accessing its associated resource whenever its validity cannot be guaranteed by the calling contract.
- Reusable handle slots **SHOULD** employ generation counters or equivalent mechanisms to detect stale references.
- Resource destruction **MUST** invalidate the corresponding logical handle according to the subsystem's documented lifetime rules.
- Do not expose internal resource pointers when a handle-based interface is required to preserve lifetime or synchronization guarantees.
- Avoid using handles for objects whose lifetime is already clearly and safely expressed through ordinary value or reference semantics.

A generation-checked handle protects against stale logical references but does not automatically guarantee that the underlying resource remains alive during concurrent access.

Likewise, GPU resource lifetime must account for asynchronous execution. Releasing a logical handle does not necessarily mean the underlying allocation can be destroyed immediately if submitted GPU work still references it.

#### Borrowed Access and Invalidation

Non-owning access **MUST** have a clearly established validity period.

- Returned pointers, references, spans, and views **MUST NOT** outlive their backing storage.
- Container mutation, relocation, resource replacement, and subsystem shutdown **MUST** be considered when establishing validity guarantees.
- APIs **SHOULD** avoid returning references to internal storage when subsequent operations may invalidate them unexpectedly.
- Do not retain borrowed references across asynchronous operations unless their lifetime is explicitly guaranteed.
- Borrowed access **MUST NOT** be mistaken for thread-safe access. Synchronization requirements remain independent of ownership.

#### Concurrency and Destruction

Ownership and thread safety are separate concerns.

- Shared ownership does not imply synchronized access to mutable state.
- Components **MUST** document concurrent access restrictions when they cannot be inferred from the API contract.
- Destruction **MUST NOT** race with active operations that access the object or resource.
- Asynchronous operations **MUST** establish how their dependencies remain valid until completion.
- Deferred destruction **SHOULD** be used when resources remain referenced by queued operations, background tasks, or GPU submissions.
- Synchronization and lifetime guarantees **MUST** be enforced by the responsible subsystem rather than delegated implicitly to unrelated callers.

**Guiding principle:**

> Ownership determines who is responsible for a resource. Lifetime determines when it is valid. Access rules determine who may use it and under what conditions. These contracts must remain explicit, independently enforceable, and consistent throughout XII.

### 1.3 Small Interfaces, Cohesive Implementations

Classes, services, and modules **MUST** have clearly defined responsibilities and expose only the functionality required by their consumers. Public interfaces should describe supported operations rather than reveal implementation details.

- Follow the principle of **high cohesion and low coupling**. Group functionality that shares a coherent responsibility, and separate functionality that changes for unrelated reasons.
- Expose the smallest public API that satisfies demonstrated use cases. Do not add methods, overloads, or extension points for hypothetical requirements.
- Keep internal storage, data structures, synchronization mechanisms, and implementation-specific state private unless explicitly required by a documented contract.
- Prefer intention-revealing operations over exposing mutable internal state.
- Maintain invariants within the component that owns them. Callers **MUST NOT** be required to understand or preserve another component's internal invariants.
- Avoid unnecessary manager classes, service layers, registries, and wrapper abstractions. Introduce them only when they establish meaningful ownership, coordination, or architectural boundaries.
- Do not fragment cohesive functionality into multiple classes merely to satisfy an arbitrary class-size or method-count restriction.
- Prefer stable interfaces that permit implementations to evolve without requiring changes throughout the codebase.

**Prefer:**

```cpp
class xiiResourceRegistry
{
public:
  [[nodiscard]] xiiResourceHandle RegisterResource(xiiStringView sPath);

  void UnregisterResource(xiiResourceHandle hResource);

  [[nodiscard]] bool IsRegistered(xiiResourceHandle hResource) const;

private:
  xiiHashTable<xiiResourceHandle, ResourceEntry> m_Resources;
};
```

**Avoid:**

```cpp
class xiiResourceRegistry
{
public:
  // Exposes implementation details and permits external mutation.
  xiiHashTable<xiiResourceHandle, ResourceEntry>& GetResources();

  // Callers must understand and maintain internal invariants.
  void RebuildResourceIndices();
};
```

The preferred interface allows the registry to control resource registration, removal, and lookup without exposing its underlying container or requiring callers to maintain its internal consistency.

The implementation can subsequently replace its hash table, introduce synchronization, or change its indexing strategy without affecting consumers.

#### Abstraction Boundaries

An abstraction should exist because it establishes a useful boundary, not merely because it reduces the apparent complexity of an individual implementation.

Before introducing a new interface, class, or service, consider:

- **Responsibility:** Does the abstraction own a coherent responsibility?
- **Consumers:** Is there a concrete consumer that benefits from the abstraction?
- **Invariants:** Does the abstraction protect meaningful invariants or ownership boundaries?
- **Coupling:** Does it reduce dependencies, or merely relocate them?
- **Extensibility:** Is extension genuinely required, or is the abstraction speculative?
- **Complexity:** Does the abstraction simplify the overall system rather than only the code immediately surrounding it?

A small public interface does not necessarily imply a small implementation. Complex internal algorithms and data structures are acceptable when they are necessary to fulfill a clearly defined responsibility.

Conversely, a class with few methods is not automatically cohesive if those methods address unrelated concerns.

**Guiding principle:**

> Expose capabilities, not implementation details. Abstractions should protect invariants, clarify ownership, and reduce coupling without introducing unnecessary indirection.

### 1.4 Think Across Engine Boundaries

XII is a modular, multi-platform engine whose components operate within a shared architectural and runtime environment. Changes to one subsystem can affect other modules, platform implementations, plugins, tools, and applications, even when those dependencies are not immediately visible.

Changes **MUST** be evaluated against the contracts of the affected subsystems, not merely the compilation or correctness of the modified source file.

- Consider the full dependency chain before modifying shared interfaces, data structures, resource representations, or execution semantics.
- Preserve module boundaries and dependency direction. Lower-level modules **MUST NOT** acquire dependencies on higher-level modules merely to accommodate a local implementation.
- Prefer changes at the layer that owns the relevant responsibility. Do not introduce workarounds in dependent modules when the underlying issue belongs to a shared abstraction.
- Avoid coupling platform-independent functionality to a particular operating system, graphics API, compiler, or hardware architecture.
- Changes to public interfaces **MUST** account for existing consumers, including plugins, tools, scripting interfaces, and external integrations.
- Changes to shared behavior **MUST** preserve documented invariants across all supported platforms and configurations.
- Do not introduce cross-module dependencies, initialization requirements, or global state without a demonstrated architectural need.

#### Compatibility Requirements

Compatibility extends beyond source-level compilation. Depending on the affected subsystem, changes may influence binary interfaces, serialized data, runtime behavior, resource lifetimes, and externally consumed formats.

The following changes **MUST** receive compatibility consideration:

- **Public headers and exported symbols:** Function signatures, symbol visibility, type layouts, calling conventions, and binary compatibility.
- **Enums and flags:** Numeric values, underlying types, bit assignments, serialization mappings, and assumptions made by external consumers.
- **Handles and resource identifiers:** Validity rules, generation semantics, ownership, lifetime, and compatibility with existing resource managers.
- **Reflection and type registration:** Type names, properties, metadata, inheritance relationships, versioning, and registration behavior.
- **Serialization and binary formats:** Schema changes, field ordering, versioning, backward compatibility, and migration requirements.
- **Shaders and GPU interfaces:** Resource bindings, constant-buffer layouts, shader permutations, pipeline interfaces, and backend-specific restrictions.
- **Plugin interfaces:** Loading, initialization, shutdown, exported functionality, and compatibility across module boundaries.
- **Threading and synchronization:** Execution ordering, thread affinity, shared-state access, and CPU/GPU synchronization guarantees.
- **Build configuration:** Conditional compilation, feature flags, compiler differences, static and dynamic linking, and platform-specific dependencies.

Compatibility requirements **MUST** be assessed according to the documented contract of the affected interface. Not every internal implementation change requires backward compatibility, but changes to persistent formats or externally consumed interfaces must not silently invalidate existing consumers.

#### Platform and Backend Independence

Platform-independent modules **MUST** express behavior through abstractions that can be implemented consistently across supported backends.

- Do not expose backend-specific types through platform-independent interfaces unless the interface explicitly represents a native interoperability boundary.
- Do not assume that behavior guaranteed by one platform or graphics API is universally supported.
- Backend implementations **MUST** preserve the common interface's documented semantics or explicitly report unsupported functionality.
- Optional functionality **SHOULD** be represented through capability queries or existing feature mechanisms rather than assumptions about the active backend.
- Do not introduce platform-specific branches into shared code when the behavior belongs in a platform implementation.
- Consider differences in memory alignment, endianness, pointer size, synchronization, filesystem behavior, and compiler support where relevant.

An implementation that works on one backend is not necessarily correct across the engine.

#### Dependency and Change Propagation

Changes **SHOULD** remain localized to the component that owns the affected responsibility.

When a modification requires updates across multiple modules, determine whether the change represents:

1. A legitimate evolution of a shared contract.
2. An implementation detail that has leaked across abstraction boundaries.
3. An unnecessary dependency or architectural coupling.
4. A missing responsibility in an existing subsystem.

Do not introduce additional abstraction layers solely to prevent every change from propagating. Some coordinated changes are legitimate and preferable to maintaining unnecessary compatibility mechanisms.

Conversely, repeated changes across unrelated consumers may indicate that an implementation detail has been exposed too broadly.

#### Validation Across Boundaries

Validation **MUST** reflect the scope and potential impact of the change.

- Compile and test the directly affected module and its relevant dependents.
- Verify supported platform and backend configurations when modifying shared or platform-sensitive functionality.
- Test affected resource lifetimes, initialization sequences, shutdown paths, and error-handling behavior.
- Validate reflection and serialization compatibility when modifying reflected types or persistent data.
- Verify shader compilation, resource binding compatibility, and pipeline behavior when changing GPU-facing interfaces.
- Consider existing plugins, tools, and applications that depend on the modified contract.
- Update relevant tests and documentation whenever observable behavior or public contracts change.

A successful local build is necessary but not sufficient evidence of correctness for changes that affect shared contracts.

Where complete cross-platform or cross-backend validation is impractical, the limitations **SHOULD** be documented, and the unverified configurations identified during review.

**Guiding principle:**

> A change is correct only when it preserves the intended contracts of every affected boundary. Local correctness must not come at the expense of system-wide consistency, portability, or maintainability.

### 1.5 Optimize From Evidence

Performance is a fundamental requirement of XII, but optimization **MUST** be guided by measurable evidence, representative workloads, and an understanding of the underlying system.

Correctness, maintainability, and performance are complementary engineering goals. Code should be designed efficiently from the outset without introducing unnecessary complexity in pursuit of unverified performance improvements.

- Establish correctness before optimizing. An implementation that produces incorrect results more quickly is not an improvement.
- Prefer efficient algorithms, appropriate data structures, and predictable memory access patterns over low-level optimizations with negligible measurable benefit.
- Use profiling data, allocation measurements, benchmarks, and representative workloads to identify genuine bottlenecks.
- Do not sacrifice clarity, safety, or architectural boundaries for speculative performance gains.
- Conversely, do not disregard allocations, virtual dispatch, cache misses, synchronization overhead, or unnecessary data movement in demonstrated hot paths.
- Consider performance across the entire operation rather than optimizing isolated instructions without accounting for their broader effects.
- Performance-sensitive implementations **SHOULD** document non-obvious optimization decisions and the assumptions on which they depend.
- Optimizations **MUST NOT** introduce undefined behavior, violate ownership contracts, or compromise correctness across supported platforms.

#### Algorithmic Efficiency Before Micro-Optimization

Algorithmic complexity and data representation generally have greater consequences than isolated instruction-level improvements.

- Select algorithms based on expected input sizes, access patterns, and computational requirements.
- Consider both asymptotic complexity and practical costs, including allocation overhead, memory locality, branching, and synchronization.
- Prefer contiguous storage when access patterns benefit from spatial locality and stable addresses are not required.
- Avoid repeated computations, allocations, and conversions when their results can be reused safely and meaningfully.
- Do not introduce caches without considering invalidation, memory consumption, synchronization, and maintenance costs.
- Avoid unnecessary abstraction overhead in performance-critical paths, but do not assume that abstraction inherently prevents optimization.

**Prefer:**

```cpp
void UpdateParticles(xiiArrayPtr<Particle> particles, float fDeltaTime)
{
  for (Particle& particle : particles)
  {
    particle.m_vPosition += particle.m_vVelocity * fDeltaTime;
  }
}
```

**Avoid:**

```cpp
void UpdateParticles(xiiArrayPtr<Particle*> particles, float fDeltaTime)
{
  for (Particle* pParticle : particles)
  {
    // Unnecessary indirection if particles can be stored contiguously.
    pParticle->m_vPosition += pParticle->m_vVelocity * fDeltaTime;
  }
}
```

When particles have value semantics and do not require stable individual addresses, contiguous storage can improve memory locality, reduce allocation overhead, and simplify ownership.

Pointer-based storage remains appropriate when object identity, polymorphism, stable addresses, or independent lifetimes are required. The preferred representation depends on the actual contract and workload.

#### Memory Allocation and Data Locality

Memory management is an important performance consideration, particularly in frequently executed systems.

- Avoid unnecessary dynamic allocations in established hot paths.
- Prefer reusing existing storage when doing so does not complicate ownership or introduce excessive retained memory.
- Reserve container capacity when the expected element count is known and repeated growth would be costly.
- Consider cache locality, memory fragmentation, alignment, and access patterns when selecting data structures.
- Avoid unnecessary copies of large objects, but do not replace inexpensive value semantics with pointer indirection without justification.
- Prefer stack allocation for small, bounded temporary objects when their lifetime and size make it appropriate.
- Use custom allocators, memory pools, or arenas only when their benefits justify the additional complexity and lifetime constraints.

Allocation count alone is not a sufficient performance metric. Allocation size, frequency, lifetime, contention, and locality can be equally important.

#### Dispatch and Abstraction Costs

Abstractions **SHOULD** be evaluated according to their actual runtime costs rather than assumptions about their implementation.

- Virtual dispatch is acceptable when runtime polymorphism serves a meaningful architectural purpose.
- Avoid repeated indirect calls in proven hot loops when a simpler dispatch strategy provides a measurable improvement.
- Prefer compile-time dispatch where the set of operations is known and the resulting code remains manageable.
- Do not replace clear interfaces with complex template machinery solely to eliminate unmeasured dispatch overhead.
- Consider the effects of inlining, instruction-cache pressure, code size, and compiler optimization when evaluating alternatives.

Eliminating an individual virtual call does not necessarily improve overall performance if the surrounding computation is dominated by memory access, synchronization, or other costs.

#### Concurrency and Synchronization

Parallel execution **MUST** be evaluated in terms of useful throughput, scheduling overhead, and synchronization requirements.

- Do not assume that additional threads or tasks automatically improve performance.
- Minimize unnecessary contention, blocking, and shared mutable state in performance-critical systems.
- Prefer clearly defined ownership and data partitioning over introducing synchronization around poorly structured shared state.
- Consider lock contention, atomic operations, memory ordering, false sharing, and task scheduling overhead where measurements indicate their relevance.
- Avoid excessively fine-grained parallelism when scheduling and coordination costs exceed useful computation.
- Synchronization optimizations **MUST** preserve the required memory-ordering and visibility guarantees.
- Do not introduce lock-free algorithms solely because they appear faster. Their correctness and performance must be demonstrated under representative contention.

#### CPU and GPU Performance

Graphics and heterogeneous computing workloads require consideration of both CPU and GPU execution costs.

- Distinguish CPU-side submission overhead from GPU execution time when identifying rendering bottlenecks.
- Measure synchronization stalls, resource transitions, memory transfers, and command submission overhead where applicable.
- Avoid unnecessary CPU/GPU synchronization, particularly operations that force either processor to wait for work that could otherwise proceed asynchronously.
- Consider resource lifetime, memory residency, bandwidth, occupancy, and execution dependencies when optimizing GPU workloads.
- Do not assume that reducing the number of API calls or render passes necessarily improves performance.
- Evaluate optimizations across supported graphics backends and representative hardware configurations when backend behavior may differ.

Reducing CPU overhead is not necessarily beneficial if the change increases GPU execution time, memory consumption, synchronization cost, or frame latency.

#### Benchmarking and Profiling

Performance claims **SHOULD** be supported by reproducible measurements.

- Establish a baseline before implementing a performance optimization.
- Profile representative workloads rather than relying exclusively on synthetic microbenchmarks.
- Use appropriate tools to distinguish CPU execution time, GPU execution time, memory usage, allocation behavior, and synchronization overhead.
- Measure both typical and worst-case behavior when latency or frame-time consistency is important.
- Account for warm-up effects, caching, compiler optimization settings, background activity, and measurement variability.
- Compare results under equivalent conditions, including build configuration, hardware, input data, and execution environment.
- Repeat measurements sufficiently to distinguish meaningful improvements from noise.
- Include correctness validation to ensure the optimized implementation preserves the original behavior.

For real-time workloads, average execution time alone is insufficient. Frame-time variance, latency spikes, and worst-case execution behavior may be more important than peak throughput.

#### Optimization Trade-offs

Performance optimizations frequently introduce trade-offs between execution time, memory consumption, code size, complexity, and portability.

Before accepting a non-trivial optimization, consider:

1. **Evidence:** Has a measurable bottleneck been identified?
2. **Impact:** Does the optimization meaningfully improve the target workload?
3. **Correctness:** Are all documented invariants and observable semantics preserved?
4. **Complexity:** Is the additional implementation and maintenance cost justified?
5. **Portability:** Does the improvement hold across the relevant platforms and hardware?
6. **Regression risk:** Could the optimization degrade other workloads or execution paths?
7. **Verification:** Can the performance improvement be reproduced and tested?

An optimization that substantially complicates an implementation for an insignificant improvement **SHOULD** be rejected unless the improvement satisfies a documented performance requirement.

Conversely, additional implementation complexity may be justified when measurements demonstrate a meaningful improvement in a critical subsystem.

**Guiding principle:**

> Design for efficiency, measure before optimizing, and preserve correctness throughout. Performance improvements should solve demonstrated problems rather than anticipated ones.

## 2. Source Layout and Formatting

### 2.1 Formatting Authority

Consistent source formatting is essential for readability, maintainability, and effective code review. XII establishes formatting rules through repository-level configuration files to ensure that source code remains uniform across contributors, development environments, and supported platforms.

The root `.clang-format` file is the authoritative source for C++ formatting rules that can be expressed by ClangFormat. The repository's `.editorconfig` governs applicable editor settings, including indentation, whitespace, line endings, and character encoding.

Contributors **MUST NOT** introduce personal formatting conventions that conflict with these configurations.

#### Formatting Requirements

- Modified C++ source code **MUST** be formatted using the repository's configured ClangFormat rules and compatible formatter version.
- The repository's `.clang-format` configuration **MUST NOT** be overridden through personal editor settings or alternative formatting configurations.
- Formatting **MUST** remain consistent regardless of the editor, IDE, operating system, or development environment used.
- Contributors **MUST NOT** manually introduce alignment, indentation, or spacing conventions that conflict with the configured formatter.
- Existing brace placement, declaration layout, spacing, and naming conventions **SHOULD** be preserved where they are not governed by automated formatting.
- Formatting changes **MUST NOT** alter program semantics, preprocessor behavior, or documented interfaces.
- Source files **MUST NOT** contain trailing whitespace, inconsistent indentation, unintended mixed line endings, or unresolved merge conflict markers.
- Source text **SHOULD** use UTF-8 encoding unless a specific file format, platform interface, or toolchain requires otherwise.
- File encodings and line-ending conventions **MUST NOT** be changed without considering repository configuration and tooling compatibility.

#### Scope of Formatting Changes

Formatting changes **MUST** remain proportional to the scope of the work being performed.

- Functional pull requests **MUST NOT** include unrelated repository-wide or mass formatting changes.
- Contributors **SHOULD** format only the files or code regions affected by their modifications when formatting an entire file would introduce substantial unrelated changes.
- Existing formatting inconsistencies outside the scope of a change **SHOULD** be addressed separately unless correcting them is necessary for the current modification.
- Dedicated formatting changes **MUST** be separated from functional changes whenever practical.
- Formatting-only changes **MUST NOT** intentionally modify program behavior.

Keeping formatting changes separate from functional changes reduces review noise, preserves meaningful version-control history, and simplifies regression analysis.

#### Automated Formatting and Tooling

Automated formatting **SHOULD** be integrated into the development workflow to reduce manual effort and prevent inconsistencies.

- Editors and IDEs **SHOULD** be configured to discover and use the repository's `.clang-format` and `.editorconfig` files.
- Contributors **SHOULD** use automated formatting before submitting changes for review.
- Continuous integration **MAY** enforce formatting compliance using the repository's approved formatter configuration.
- Automated formatting tools **MUST NOT** silently rewrite unrelated source files as part of a functional change.
- Formatter upgrades or modifications to formatting configuration **MUST** be reviewed independently because they may affect a substantial portion of the codebase.
- Generated source files **SHOULD** be formatted consistently when supported by their generators. Contributors **MUST NOT** manually format generated output when those changes would be overwritten by regeneration.

Where automated formatting cannot express a particular convention, contributors **SHOULD** follow the established style of the surrounding code.

#### Formatting Exceptions

Certain source constructs may require deliberate formatting to preserve readability, compatibility, or tool-generated structure.

Examples include:

- Preprocessor macros whose formatting affects continuation or expansion behavior.
- Generated code whose layout is controlled by an external generator.
- Platform-specific declarations requiring prescribed syntax or layout.
- Embedded source languages, shader code, or structured data that follow separate formatting rules.
- Tables or aligned declarations where automated formatting would significantly reduce readability.

Exceptions **MUST** be narrowly scoped and justified by a concrete technical requirement.

ClangFormat suppression directives **SHOULD NOT** be introduced merely to preserve personal formatting preferences. Where suppression is necessary, it **SHOULD** encompass the smallest practical region.

**Guiding principle:**

> Formatting is a repository-wide contract, not an individual preference. Automate consistency, minimize unrelated changes, and keep code reviews focused on semantics rather than presentation.

### 2.2 Braces and Indentation

Consistent brace placement and indentation are essential for expressing control flow clearly and preventing ambiguity. XII follows Allman-style braces for class definitions, function definitions, and multi-line control blocks, with indentation and spacing governed by the repository's `.clang-format` configuration.

Braces and indentation **MUST** make the logical structure of the code immediately apparent without requiring readers to infer statement ownership.

#### General Requirements

- Class, struct, namespace, function, and multi-line control-flow definitions **MUST** follow the brace placement prescribed by `.clang-format`.
- Use consistent indentation according to the repository's configured indentation width.
- Opening and closing braces **MUST** clearly delimit the scope they introduce.
- Closing braces **MUST** align with their corresponding declarations or control statements.
- Do not introduce unnecessary nesting when guard clauses or early returns provide a clearer implementation.
- Braces **MUST NOT** be omitted when doing so introduces ambiguity or makes the control flow difficult to follow.
- Do not manually align statements or braces in ways that conflict with the repository's formatter.

**Prefer:**

```cpp
class xiiResourceRegistry
{
public:
  void Clear()
  {
    m_Entries.Clear();
  }

private:
  xiiDynamicArray<xiiUInt32> m_Entries;
};
```

The structure makes access control, function scope, and member declarations immediately distinguishable.

#### Single-Statement Control Blocks

Braces **MAY** be omitted for simple, single-statement conditions and loops when the resulting control flow remains unambiguous.

This convention is particularly appropriate for guard clauses, validation checks, and early returns.

**Prefer:**

```cpp
xiiResult Initialize(xiiDevice* pDevice)
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  if (m_bInitialized)
    return XII_SUCCESS;

  return InitializeResources(pDevice);
}
```

Braces **SHOULD** be omitted only when all of the following conditions are satisfied:

- The controlled body contains exactly one statement.
- The statement remains readable without additional scope.
- No ambiguity exists regarding control-flow ownership.
- The condition and its body are sufficiently simple to understand immediately.
- Omitting braces is consistent with the surrounding code and formatter configuration.

Braces **SHOULD** be introduced when the controlled statement becomes more complex or when additional statements are added.

#### Multi-Statement Control Blocks

Braces **MUST** be used when a conditional branch or loop contains multiple statements.

**Prefer:**

```cpp
if (bNeedsRebuild)
{
  ResetResources();
  RebuildResources();
  UpdateResourceBindings();
}
```

**Avoid:**

```cpp
if (bNeedsRebuild)
  ResetResources();
  RebuildResources();
  UpdateResourceBindings();
```

In the avoided implementation, only the first statement is controlled by the condition. The remaining statements execute unconditionally, potentially violating the intended control flow.

Braces are therefore a correctness mechanism as well as a formatting convention.

#### Nested Conditions and Ambiguous Control Flow

Nested conditionals **MUST NOT** rely on implicit `else` association.

**Avoid:**

```cpp
if (bVisible)
  if (bReady)
    Process();
  else
    LogNotReady();
```

The `else` belongs to the innermost unmatched `if`, which may not reflect the intended behavior.

**Prefer:**

```cpp
if (bVisible)
{
  if (bReady)
  {
    Process();
  }
  else
  {
    LogNotReady();
  }
}
```

Where possible, simplify nested conditions using guard clauses or combined predicates.

**Prefer when equivalent:**

```cpp
if (!bVisible)
  return;

if (!bReady)
{
  LogNotReady();
  return;
}

Process();
```

Early returns **SHOULD** be preferred when they reduce nesting without obscuring cleanup, resource ownership, or control-flow requirements.

#### Conditional Branches

For `if`, `else if`, and `else` chains, braces **SHOULD** be applied consistently across branches when any branch requires a multi-statement body.

**Prefer:**

```cpp
if (state == xiiResourceState::Ready)
{
  ProcessResource();
  UpdateStatistics();
}
else if (state == xiiResourceState::Pending)
{
  QueueResource();
}
else
{
  LogResourceFailure();
}
```

Avoid mixing braced and unbraced branches when doing so makes the structure less predictable.

Simple conditional chains **MAY** omit braces where permitted by the formatter and where every branch remains unambiguous.

#### Loops and Iteration

Loop bodies follow the same rules as conditional statements.

Braces **MAY** be omitted for simple, single-statement loops.

**Prefer:**

```cpp
for (xiiUInt32 uiIndex = 0; uiIndex < m_Entries.GetCount(); ++uiIndex)
  ProcessEntry(m_Entries[uiIndex]);
```

Braces **MUST** be used for multi-statement loop bodies.

**Prefer:**

```cpp
for (xiiUInt32 uiIndex = 0; uiIndex < m_Entries.GetCount(); ++uiIndex)
{
  ResourceEntry& entry = m_Entries[uiIndex];

  if (!entry.m_bActive)
    continue;

  ProcessEntry(entry);
  UpdateStatistics(entry);
}
```

Loop bodies **SHOULD** remain focused and avoid excessive nesting. Where iteration logic becomes difficult to follow, consider extracting a cohesive operation into a separate function.

#### Scope and Lifetime

Braces also define lexical scope and therefore influence object lifetime and deterministic destruction.

- Introduce explicit scopes when a shorter lifetime is necessary for correctness, synchronization, or resource management.
- Do not introduce additional scopes merely for visual grouping when they serve no meaningful purpose.
- Variables **SHOULD** be declared in the narrowest practical scope that satisfies their intended use.
- RAII-managed resources **MUST** remain alive for the entire duration of operations that depend on them.
- Scope boundaries **MUST NOT** accidentally invalidate borrowed references, pointers, views, or iterators.

**Prefer:**

```cpp
{
  XII_LOCK(m_Mutex);
  UpdateSharedState();
}

// The lock is released before performing independent work.
ProcessPendingTasks();
```

An explicit scope is justified here because it determines the duration of synchronization rather than serving as a purely stylistic grouping.

#### Maintenance and Review

Code structure **SHOULD** remain easy to modify without introducing accidental changes in control flow.

- When adding statements to an unbraced control block, introduce braces before expanding the body.
- Review nested branches carefully when modifying conditions or adding `else` clauses.
- Avoid formatting that visually suggests a statement belongs to a scope when it does not.
- Do not rely on indentation alone to express control flow; C++ semantics are determined by statement structure and braces.
- Prefer restructuring deeply nested logic over adding layers of braces that obscure the operation's intent.

**Guiding principle:**

> Braces define control flow, scope, and lifetime. Use them consistently to eliminate ambiguity, preserve correctness, and make the structure of an implementation immediately understandable.

### 2.3 Alignment and Spacing

Consistent alignment and spacing improve readability by making relationships between declarations, expressions, and operations immediately apparent. Formatting **MUST** follow the repository's `.clang-format` configuration, which determines the exact alignment and spacing rules.

Alignment should clarify related code without introducing unnecessary whitespace or making maintenance more difficult.

#### General Requirements

- Use consistent spacing around operators, assignments, declarations, and expressions as prescribed by the repository's formatter.
- Related declarations and initializations **MAY** be aligned when doing so improves readability and is supported by the formatter.
- Do not manually introduce alignment that conflicts with automated formatting.
- Avoid aligning unrelated statements or declarations across large regions of code.
- Use blank lines to separate logically distinct operations, not individual statements without meaningful separation.
- Do not introduce excessive vertical whitespace that unnecessarily fragments cohesive implementations.
- Preserve consistent spacing within classes, functions, namespaces, and control-flow structures.

**Prefer:**

```cpp
xiiUInt32 m_uiFrameIndex = 0U;
xiiUInt32 m_uiPassCount  = 0U;
bool      m_bActive      = false;
```

Related member declarations may benefit from alignment because their types, names, and initializers form a compact logical group.

**Avoid:**

```cpp
xiiUInt32                    m_uiFrameIndex = 0U;
xiiUInt32                    m_uiPassCount  = 0U;

bool                         m_bActive      = false;

// Unrelated declarations should not dictate alignment.
xiiUniquePtr<xiiResourceData> m_pResourceData;
```

Alignment **SHOULD NOT** introduce large amounts of whitespace merely to maintain common columns across unrelated declarations.

#### Logical Spacing

Blank lines **SHOULD** communicate transitions between logically distinct operations.

**Prefer:**

```cpp
const xiiUInt32 uiWidth  = descriptor.m_uiWidth;
const xiiUInt32 uiHeight = descriptor.m_uiHeight;

if (uiWidth == 0U || uiHeight == 0U)
  return XII_FAILURE;

const xiiUInt64 uiPixelCount = static_cast<xiiUInt64>(uiWidth) * uiHeight;

AllocateStorage(uiPixelCount);
InitializeStorage();
```

Avoid inserting blank lines after every declaration or statement when those statements form one cohesive operation.

Similarly, do not compress unrelated stages of an implementation into a single uninterrupted block.

**Guiding principle:**

> Alignment should reveal relationships, and whitespace should reveal structure. Neither should introduce unnecessary complexity or compete with automated formatting.

### 2.4 Statement Structure

Statements **MUST** communicate their purpose clearly and avoid unnecessary combinations of declarations, assignments, conditions, and side effects.

Prefer explicit operations and predictable control flow over compact expressions that require additional interpretation.

#### General Requirements

- Declare one independently meaningful variable per statement.
- Initialize variables at the point of declaration whenever practical.
- Prefer declarations in the narrowest scope that satisfies their intended use.
- Avoid assignment inside conditional expressions unless it materially improves clarity.
- Use parentheses to clarify complex boolean expressions, particularly when mixing logical operators.
- Do not combine unrelated side effects into a single statement.
- Avoid comma expressions when ordinary sequential statements communicate the same behavior more clearly.
- Prefer early returns for invalid inputs and unmet preconditions when they simplify control flow.
- Do not rely on implicit operator precedence when an expression could reasonably be misinterpreted.
- Do not introduce temporary variables that merely rename an expression without improving readability, correctness, or reuse.

#### Declarations and Initialization

**Prefer:**

```cpp
const xiiUInt32 uiWidth  = descriptor.m_uiWidth;
const xiiUInt32 uiHeight = descriptor.m_uiHeight;

if (uiWidth == 0U || uiHeight == 0U)
  return XII_FAILURE;
```

**Avoid:**

```cpp
xiiUInt32 uiWidth, uiHeight;

uiWidth = descriptor.m_uiWidth;
uiHeight = descriptor.m_uiHeight;
```

Separate declarations make initialization and variable lifetime easier to inspect.

Multiple declarations in a single statement **SHOULD** be avoided when their variables have independent meanings or initialization requirements.

#### Assignments and Conditions

Assignments within conditions **SHOULD** be avoided when separating the operations improves clarity.

**Prefer:**

```cpp
xiiResource* pResource = FindResource(sName);

if (pResource == nullptr)
  return XII_FAILURE;

ProcessResource(*pResource);
```

**Avoid:**

```cpp
if (xiiResource* pResource = FindResource(sName))
  ProcessResource(*pResource);
else
  return XII_FAILURE;
```

The second form is valid C++, but the first makes lookup, validation, and subsequent processing distinct operations.

C++17-style conditional initializers **MAY** be used when their scope restriction provides a clear benefit and the condition remains straightforward.

#### Complex Expressions

Complex conditions **SHOULD** be structured so that their intent is immediately understandable.

**Prefer:**

```cpp
const bool bRequiresUpdate = bDirty || bForceUpdate;
const bool bCanUpdate      = bInitialized && !bPendingDestruction;

if (bRequiresUpdate && bCanUpdate)
  UpdateResource();
```

**Avoid:**

```cpp
if ((bDirty || bForceUpdate) && bInitialized && !bPendingDestruction)
  UpdateResource();
```

Named intermediate predicates are appropriate when they clarify domain-specific meaning, are reused, or make a complex condition easier to verify.

Do not introduce intermediate variables for every simple expression.

#### Side Effects and Evaluation Order

Statements with side effects **MUST** preserve clear and predictable evaluation semantics.

**Prefer:**

```cpp
UpdateResourceState();
NotifyDependents();
RecordStatistics();
```

**Avoid:**

```cpp
UpdateResourceState(), NotifyDependents(), RecordStatistics();
```

Avoid expressions that modify and access shared state through multiple operands when their evaluation order is unclear or dependent on subtle language rules.

**Guiding principle:**

> Each statement should express a clear operation. Prefer explicit initialization, understandable conditions, and predictable evaluation over unnecessary compactness.

### 2.5 Includes and Whitespace

Header dependencies **MUST** be explicit, organized, and consistent with XII's module architecture.

Include ordering should communicate the relationship between the current translation unit, its module, and its dependencies while preserving independent compilation and minimizing unnecessary coupling.

#### General Requirements

- Use one `#include` directive per line.
- Include only headers required by the declarations or implementation.
- Prefer the narrowest appropriate header rather than importing a broad subsystem header unnecessarily.
- Do not rely on transitive includes for types, macros, or functions directly required by the current file.
- Public headers **MUST** be self-contained and compile independently when included in a valid translation unit.
- Public headers **MUST NOT** depend on a precompiled header to provide missing declarations or dependencies.
- Avoid circular header dependencies. Use forward declarations where appropriate and where complete type information is not required.
- Do not use relative paths to navigate between module include directories when the repository's module-qualified include paths are available.
- Preserve the established include ordering and grouping conventions.
- Separate meaningful include groups with a single blank line.
- Do not introduce unnecessary blank lines within a cohesive include group.

#### Header Files

Header files **MUST** begin with the repository's required license and copyright notice, followed by `#pragma once`.

For headers belonging to a dynamically linked module, the module's `DLL.h` **MUST** be the first included header immediately following `#pragma once`.

No other include directive or forward declaration should appear between `#pragma once` and the module's `DLL.h`.

**Prefer:**

```cpp
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Result.h>

#include <GraphicsFoundation/Resources/Resource.h>
```

The module's `DLL.h` provides the export and import definitions required by its public interfaces.

It **MUST NOT** be omitted from headers that rely on module export macros merely because another included header happens to provide those definitions transitively.

Headers for modules that do not use a DLL export header are exempt from this requirement.

#### Implementation Files


C++ implementation files **MUST** begin with the repository's required license and copyright notice. For implementation files that use a module precompiled header, the corresponding module PCH **MUST** be the first include immediately following that notice, before all other header dependencies.

Other dependencies **SHOULD** then be grouped according to their architectural relationship.

A typical implementation file follows this structure:

1. Module precompiled header.
2. Required headers from lower-level modules.
3. Headers from the current module.
4. Additional dependencies from other permitted modules or application-specific components.

**Prefer:**

```cpp
#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/IdTable.h>
#include <Foundation/Threading/Mutex.h>

#include <GraphicsFoundation/Resources/Resource.h>

#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
```

The ordering of include groups **MUST NOT** be interpreted as permission to introduce architectural dependencies that violate the module dependency graph.

Where project-specific conventions establish a different ordering, those conventions take precedence unless they conflict with the repository's formatting or build requirements.

#### Forward Declarations

Forward declarations **SHOULD** be used when they meaningfully reduce compilation dependencies without compromising correctness or interface clarity.

They are generally appropriate for types referenced exclusively through pointers or references when no complete definition is required.

**Prefer:**

```cpp
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiResourceManager;

class XII_GRAPHICSCORE_DLL xiiResourceLoader
{
public:
  void SetResourceManager(xiiResourceManager* pManager);

private:
  xiiResourceManager* m_pManager = nullptr;
};
```

A forward declaration **MUST NOT** replace an include when a complete type is required, such as for a by-value data member, inheritance, inline member access, or another construct requiring the full definition.

Forward declarations **SHOULD NOT** be introduced solely to eliminate includes when doing so obscures important interface dependencies.

#### Dependency Hygiene

Header dependencies influence compilation time, coupling, and the scope of rebuilds.

- Avoid including implementation-only dependencies in public headers.
- Move implementation-specific includes into `.cpp` files where practical.
- Do not include headers solely because they were historically present.
- Remove unused includes when modifying the surrounding code, provided doing so does not introduce unrelated changes.
- Do not introduce monolithic convenience headers into low-level modules when narrowly scoped dependencies are available.
- Changes to public headers **SHOULD** consider their impact on downstream compilation and module boundaries.

**Guiding principle:**

> Every include should represent an intentional dependency. Headers must stand independently, module boundaries must remain explicit, and include ordering must follow the repository's established conventions.

### 2.6 Section Separators

Section separators **MAY** be used to organize substantial implementation files and distinguish meaningful groups of functionality.

Their purpose is navigation and structural clarity, not decoration.

#### General Requirements

- Use the repository's established separator style where section dividers are appropriate.
- Prefer section separators between logically distinct implementation groups.
- Do not insert separators between every small function or helper.
- Avoid excessive or deeply nested visual section hierarchies.
- Section organization **SHOULD** reflect the responsibilities of the implementation rather than arbitrary file length.
- Do not use section separators to compensate for unrelated responsibilities that should be separated into different components or translation units.

**Prefer:**

```cpp
//////////////////////////////////////////////////////////////////////////

xiiResourceRegistry::xiiResourceRegistry() = default;

xiiResourceRegistry::~xiiResourceRegistry()
{
  Clear();
}

//////////////////////////////////////////////////////////////////////////

void xiiResourceRegistry::Clear()
{
  m_Entries.Clear();
}
```

Separators can improve navigation when distinguishing construction and destruction, resource management, serialization, or other substantial implementation groups.

For short implementation files containing a small number of closely related functions, additional separators are generally unnecessary.

#### Structural Organization

When section separators are used, related implementations **SHOULD** remain grouped consistently.

Examples of meaningful groups include:

- Construction, initialization, and destruction.
- Public interface implementations.
- Resource creation and release.
- State management and synchronization.
- Serialization and deserialization.
- Internal algorithms and helper functions.
- Platform-specific implementations.

Section dividers **MUST NOT** replace clear function names, cohesive class design, or appropriate source-file organization.

An excessively large implementation file should be evaluated for responsibility boundaries rather than divided indefinitely with decorative comments.

**Guiding principle:**

> Use visual separators to clarify meaningful implementation boundaries, not to disguise unnecessary complexity or fragmented design.

### 2.7 File Endings and Comments

Source files **MUST** maintain consistent file structure, meaningful documentation, and a clean version-control history.

Comments should explain intent, constraints, and non-obvious decisions rather than repeat information already apparent from the code.

#### File Requirements

- Source files **MUST** end with a single newline.
- Trailing whitespace and unintended mixed line endings **MUST NOT** be introduced.
- Required copyright and license notices **MUST** follow the repository's established form.
- Existing license notices **MUST NOT** be removed or modified without authorization.
- Contributors **MUST NOT** copy personal or third-party copyright notices into unrelated files without an applicable legal or licensing basis.
- Required static-link registration macros **MUST** be preserved where applicable. See §17.
- Generated source files **MUST** retain any required generator metadata, registration declarations, or structural markers.

#### Commenting Requirements

Comments **SHOULD** explain information that cannot be communicated adequately through names, types, interfaces, or straightforward implementation structure.

Appropriate comments include:

- Non-obvious algorithmic decisions.
- Platform-specific behavior or limitations.
- Ownership, lifetime, or synchronization constraints.
- Requirements imposed by external APIs or binary formats.
- Performance-sensitive assumptions supported by measurements.
- Intentional deviations from otherwise established conventions.
- Compatibility requirements that are not apparent from the surrounding code.

**Prefer:**

```cpp
// Resource destruction is deferred until submitted GPU work
// no longer references the underlying allocation.
QueueResourceDestruction(hResource);
```

**Avoid:**

```cpp
// Destroy the resource.
DestroyResource(hResource);
```

Comments that merely restate the next operation add noise without improving comprehension.

#### Obsolete Code and Temporary Changes

- Do not retain commented-out implementations as a substitute for version control.
- Obsolete code **MUST** be removed rather than indefinitely disabled.
- `#if 0` blocks **SHOULD NOT** be used to preserve abandoned implementations.
- Temporary debugging statements, experimental code, and instrumentation **MUST** be removed before submission unless intentionally retained as supported functionality.
- `TODO` and `FIXME` comments **SHOULD** identify a concrete issue or required action rather than serve as vague placeholders.
- Workarounds **SHOULD** explain the underlying constraint and, where appropriate, reference the associated issue or dependency.

**Avoid:**

```cpp
#if 0
void ProcessResources()
{
  // Previous implementation retained indefinitely.
}
#endif
```

Version control already preserves historical implementations. Keeping disabled code in active source files increases maintenance burden and obscures the current implementation.

#### Comment Accuracy

Comments are part of the maintained source contract.

- Comments **MUST NOT** contradict the implementation.
- When behavior changes, associated comments and documentation **MUST** be updated.
- Avoid speculative comments that describe intended future behavior as though it already exists.
- Do not document guarantees that the implementation does not enforce.
- Prefer removing an inaccurate comment over preserving it for historical reasons.

**Guiding principle:**

> Source files should contain only the active implementation and the documentation necessary to understand it. Comments must preserve intent, explain constraints, and remain accurate as the code evolves.


### 2.8 Header and Implementation File Organization

XII follows a consistent source-file organization in which implementation files are placed within an `Implementation` directory alongside their corresponding public headers.

This convention separates interface declarations from implementation details while preserving the logical grouping of related functionality.

#### General Requirements

- Public header files (`.h`) **MUST** reside in the directory corresponding to their module and subsystem.
- C++ implementation files (`.cpp`) **MUST** reside in an `Implementation` subdirectory within the same directory as their corresponding headers.
- The `Implementation` directory **MUST** use the exact capitalization established by the repository.
- Related headers and implementation files **SHOULD** share the same base filename to make their relationship immediately apparent.
- Implementation files **MUST NOT** be placed directly alongside public headers unless an established repository convention or technical requirement explicitly permits it.
- Private implementation headers **SHOULD** reside in the `Implementation` directory when they are not intended for consumption outside the subsystem.
- Do not introduce unnecessary directory hierarchies or separate implementation directories when an existing subsystem structure is sufficient.

#### Directory Structure

**Prefer:**

```text
GraphicsCore/
└── Pipeline/
    ├── RenderGraph.h
    ├── RenderGraphManager.h
    └── Implementation/
        ├── RenderGraph.cpp
        └── RenderGraphManager.cpp
```

**Avoid:**

```text
GraphicsCore/
└── Pipeline/
    ├── RenderGraph.h
    ├── RenderGraph.cpp
    ├── RenderGraphManager.h
    └── RenderGraphManager.cpp
```

The preferred structure keeps public interfaces immediately discoverable while grouping their implementations in a predictable location.

#### Implementation-Specific Headers

Headers that define private implementation details **SHOULD** remain within the `Implementation` directory when they are not part of the module's public interface.

Examples include internal helper classes, private data structures, and implementation-specific utilities.

Such headers **MUST NOT** be treated as public API merely because they are accessible through the repository's include paths.

If a private header becomes necessary for multiple independent subsystems, its responsibility and appropriate module placement **SHOULD** be reconsidered rather than exposing implementation details by convenience.

#### Exceptions

Exceptions **MAY** be permitted for generated sources, platform-specific implementations, third-party code, or files subject to established build-system conventions.

Any deviation **SHOULD** follow an existing repository pattern and have a clear technical justification.

**Guiding principle:**

> Keep public interfaces visible and their implementations nearby. A predictable directory structure should make code easy to locate, navigate, and maintain without exposing unnecessary implementation details.


## 3. Naming Conventions

Consistent naming is essential for readability, maintainability, and discoverability across XII's modules. Identifiers should communicate their purpose, type semantics, ownership context, and relationship to the surrounding system without requiring unnecessary inspection of their declarations or implementations.

XII uses a combination of PascalCase, descriptive identifiers, and type-dependent prefixes to maintain consistency across its C++ codebase.

**Preserved baseline rule:** Type-dependent prefixes are **mandatory for member variables only** when an applicable type is known. Local variables and function parameters may use type-dependent prefixes, but their use remains at the programmer's discretion.

Examples throughout this document generally use type-dependent prefixes for local variables and parameters to remain consistent with common XII conventions. These examples **MUST NOT** be interpreted as making such prefixes mandatory outside member variables.

### 3.1 Type and Symbol Names

Names **MUST** be descriptive, consistent with established engine terminology, and appropriate to the responsibility of the symbol.

| Symbol | Convention | Example |
|---|---|---|
| Engine classes and structs | `xii` + PascalCase | `xiiResourceManager` |
| Enum wrapper types | `xii` + PascalCase | `xiiResourceState` |
| Enum values | PascalCase | `Unloaded`, `Loaded`, `Invalid` |
| Functions and methods | PascalCase | `Initialize()`, `CreateResource()` |
| Accessors | `Get`, `Is`, `Has`, `Can`, `Set` as appropriate | `GetCount()`, `IsInitialized()` |
| Local variables | Descriptive; type prefix optional | `uiResourceCount`, `resourceCount` |
| Function parameters | Descriptive; type prefix optional | `sPath`, `path` |
| Private/protected instance fields | `m_` + applicable type prefix | `m_uiCount`, `m_pResource` |
| Private/protected mutable static fields | `s_` + applicable type prefix | `s_uiInstanceCount` |
| Private/protected static constants | PascalCase, preferably `constexpr` | `MaxResourceCount` |
| Compile-time constants | PascalCase, consistent with scope | `DefaultAlignment` |
| Macros and compile switches | Established `XII_` convention | `XII_ASSERT_DEV`, `XII_ENABLED` |
| Template type parameters | Semantic PascalCase | `ValueType`, `AllocatorType` |
| Template non-type parameters | Descriptive, following their semantic role | `Capacity`, `Alignment` |
| Header files | Name of primary type or responsibility | `ResourceManager.h` |
| Implementation files | Corresponding header base name | `ResourceManager.cpp` |
| Namespaces | Established subsystem convention | `xiiInternal` |

#### General Requirements

- Public symbols **MUST** use terminology consistent with the subsystem that owns them.
- Names **SHOULD** describe responsibilities, behavior, or meaning rather than incidental implementation details.
- Avoid abbreviations that require private knowledge of the implementation.
- Prefer established engine terminology over inventing alternative names for existing concepts.
- Do not encode unnecessary implementation details into identifiers when those details may change.
- Distinguish operations with different semantics through meaningful names rather than arbitrary suffixes.
- Do not rename stable public symbols solely for stylistic consistency when doing so would create unnecessary compatibility changes.
- C++ keywords, reserved identifiers, and implementation-reserved naming patterns **MUST NOT** be used for user-defined symbols.

**Prefer:**

```cpp
xiiResult InitializeResources();
xiiUInt32 GetResourceCount() const;
bool IsResourceReady(xiiResourceHandle hResource) const;
```

**Avoid:**

```cpp
xiiResult InitRes();
xiiUInt32 GetRC() const;
bool Check(xiiResourceHandle hResource) const;
```

Names should be understandable without relying on undocumented abbreviations or examining the implementation.

### 3.2 Type-Dependent Member Prefixes

Type-dependent prefixes are **mandatory for member variables** when the member's type belongs to a recognized category.

These prefixes communicate the semantic type of the stored value and are applied after the applicable member or static marker.

| Type family | Prefix | Examples |
|---|---|---|
| Signed integers, including signed atomic integers and `ptrdiff_t` | `i` | `m_iPriority`, `s_iLogLevel` |
| Unsigned integers, including `size_t` | `ui` | `m_uiFrameIndex`, `m_uiCount` |
| Floating-point types (`float`, `double`) | `f` | `m_fDeltaTime`, `m_fThreshold` |
| Boolean values, including atomic booleans | `b` | `m_bInitialized`, `m_bActive` |
| Handle types | `h` | `m_hResource`, `m_hTexture` |
| Raw and smart pointers | `p` | `m_pDevice`, `m_pResource` |
| NUL-terminated C strings | `sz` | `m_szDisplayName` |
| Non-NUL-terminated character pointers | `p` | `m_pTextBegin` |
| XII string objects and string views | `s` | `m_sName`, `m_sPath` |
| Vector and SIMD vector types | `v` | `m_vPosition`, `m_vVelocity` |
| Quaternion types | `q` | `m_qRotation` |
| Matrix and SIMD matrix types | `m` | `m_mTransform` |
| Other types | No additional type prefix | `m_Descriptor`, `m_Blackboard` |

#### Prefix Requirements

- Member variables **MUST** use their applicable type-dependent prefixes.
- Prefixes **MUST** represent the semantic type of the value rather than a convenient or misleading interpretation.
- Type aliases and wrappers **SHOULD** follow the semantic category they represent when that category is unambiguous.
- Do not assign pointer prefixes to integer values merely because those values represent memory addresses.
- Handles **MUST** use the `h` prefix rather than the `p` prefix, even when they refer to independently allocated resources.
- Smart pointers such as `xiiUniquePtr` and `xiiSharedPtr` use the `p` prefix because they represent pointer-based access and ownership.
- Types without an established prefix **MUST NOT** be assigned arbitrary prefixes merely to maintain a visual pattern.
- Prefixes **SHOULD** remain consistent when a member's type changes. A type change that alters the appropriate prefix should trigger a corresponding identifier review.

**Prefer:**

```cpp
class xiiSimulationState
{
private:
  xiiUInt64 m_uiSimulationStep = 0U;
  xiiInt32  m_iPriority        = 0;
  bool      m_bActive          = false;
  float     m_fDeltaTime       = 0.0f;

  xiiHashedString               m_sName;
  xiiResourceHandle             m_hResource;
  xiiUniquePtr<xiiResourceData> m_pResourceData;

  xiiVec3 m_vPosition;
  xiiQuat m_qRotation;
  xiiMat4 m_mTransform;

  xiiDynamicArray<xiiUInt32> m_ActiveIndices;
};
```

The spelling `m_mTransform` is intentional:

- The first `m_` identifies an instance member.
- The second `m` identifies a matrix.

Likewise, `m_pResourceData` identifies a member containing a pointer-like object, while `m_hResource` identifies a resource handle.

#### Semantic Types and Aliases

A prefix must not misrepresent the underlying abstraction.

For example:

```cpp
xiiUInt64          m_uiGpuAddress = 0U;
xiiResourceHandle  m_hResource;
xiiUniquePtr<Data> m_pData;
```

A GPU address represented as an unsigned integer uses `ui`, even if the value refers to a location in GPU memory.

An opaque resource handle uses `h`, regardless of how the handle is implemented internally.

The naming convention must remain faithful to the type's public semantics rather than its private representation.

### 3.3 Public Fields

The existing XII naming policy does **not** require the `m_` prefix for public non-static data members.

However, many existing public engine structures use `m_` consistently, particularly configuration objects, descriptors, reflected structures, and serialized data.

For new public data structures, contributors **SHOULD** follow the naming convention established by the surrounding module.

#### General Requirements

- Public fields **SHOULD** use a consistent naming style within the same structure.
- New configuration and descriptor structures **SHOULD** follow the established conventions of neighboring public data types.
- Public field names **MUST** communicate their intended meaning and, where relevant, units or state.
- Do not mix prefixed and unprefixed public fields arbitrarily within a new structure.
- Existing public fields **MUST NOT** be renamed solely for aesthetic consistency without evaluating compatibility implications.
- Changes to reflected or serialized public fields **MUST** consider their effect on persistent data, tools, scripts, plugins, and external consumers.

**Prefer:**

```cpp
struct xiiSimulationSettings
{
  bool      m_bEnableSimulation = true;
  xiiUInt32 m_uiMaxIterations   = 128U;
  float     m_fFixedTimeStep    = 0.016f;
};
```

This structure follows a consistent field convention and communicates each value's semantics through its name.

Public fields may be accessed directly when the structure intentionally represents data rather than an encapsulated behavioral abstraction.

Do not introduce getters and setters for every field of a simple configuration or value structure unless doing so protects meaningful invariants or provides necessary behavior.

### 3.4 Private, Protected, and Static Fields

Private and protected instance fields **MUST** use the `m_` prefix followed by the appropriate type-dependent prefix, where applicable.

Private and protected mutable static fields **MUST** use the `s_` prefix followed by the appropriate type-dependent prefix.

Compile-time constants should use descriptive PascalCase names.

**Prefer:**

```cpp
class xiiCounter
{
public:
  XII_ALWAYS_INLINE xiiUInt32 GetCount() const
  {
    return m_uiCount;
  }

private:
  static constexpr xiiUInt32 MaxCount = 1024U;

  static xiiUInt32 s_uiLiveCounters;

  xiiUInt32 m_uiCount = 0U;
};
```

#### General Requirements

- Instance fields **MUST** be distinguishable from local variables and parameters through the `m_` prefix.
- Private and protected mutable static fields **MUST** use `s_`.
- Private and protected static constants **SHOULD** use PascalCase without the mutable-static prefix.
- Prefer `constexpr` for compile-time constants when supported by the required semantics.
- Avoid mutable static state when ordinary object ownership or explicitly managed subsystem state provides a clearer design.
- Static data **MUST** have well-defined initialization, synchronization, and destruction behavior.
- Public mutable static fields **SHOULD** be avoided when they expose internal state or permit uncontrolled modification.

The existing naming policy exempts public static fields from the private-static naming requirement.

Nevertheless, public mutable state should not be introduced casually. Prefer named constants or controlled operations when invariants or synchronization requirements must be preserved.

### 3.5 Fixed-Size Arrays

The existing XII naming policy permits a relaxed member-prefix convention for fixed-size arrays.

Both of the following declarations are permitted:

```cpp
bool m_bActiveFlags[3];
bool mActiveFlags[3];
```

For new code, the conventional typed-member form is **RECOMMENDED** because it remains consistent with other member declarations.

#### General Requirements

- Fixed-size arrays **MAY** use the relaxed member-prefix convention established by the original policy.
- New fixed-size array members **SHOULD** use the conventional `m_` and applicable type prefix.
- A class or structure **SHOULD** use one consistent naming style for its fixed-size array members.
- Existing array members **MUST NOT** be renamed solely to eliminate a permitted naming exception.
- Array names **SHOULD** describe the meaning of their elements rather than merely identify the storage as an array.

**Prefer for new code:**

```cpp
class xiiInputState
{
private:
  bool      m_bButtonPressed[8] = {};
  xiiUInt32 m_uiEventCounts[8]  = {};
};
```

A fixed-size array exception does not remove the requirement for meaningful names or consistent member organization.

### 3.6 Boolean and Query Names

Boolean identifiers and query methods **SHOULD** communicate conditions that can be read naturally in control-flow expressions.

A boolean name should make its true and false meanings immediately apparent.

#### General Requirements

- Boolean member fields **MUST** use the `b` type prefix where applicable.
- Prefer affirmative condition names over unnecessary negations.
- Avoid ambiguous terms such as `Flag`, `Status`, `State`, or `Check` when a more specific condition can be expressed.
- Boolean query methods **SHOULD** use `Is`, `Has`, or `Can` according to their semantics.
- Queries **SHOULD NOT** modify observable object state unless their contract explicitly communicates that behavior.
- Do not use `Get` for boolean queries when `Is`, `Has`, or `Can` communicates the intended meaning more precisely.

**Prefer:**

```cpp
if (m_bSimulationActive && pWorld != nullptr)
{
  AdvanceSimulation();
}
```

**Avoid:**

```cpp
if (!m_bSimulationDisabled && pWorld != nullptr)
{
  AdvanceSimulation();
}
```

Prefer query names that communicate distinct meanings:

| Prefix | Intended meaning | Example |
|---|---|---|
| `Is` | Current state or property | `IsInitialized()` |
| `Has` | Possession or availability | `HasPendingTasks()` |
| `Can` | Capability or permission | `CanExecute()` |
| `Get` | Retrieve a value or object | `GetResourceCount()` |
| `Set` | Assign or configure state | `SetPriority()` |

These conventions are semantic guidelines rather than a requirement to mechanically rename established public APIs.

**Guiding principle:**

> A boolean name should describe what is true, and a query method should communicate what it evaluates.

### 3.7 Names Must Distinguish Identity, Units, and Scope

Names **MUST** distinguish concepts that share similar underlying representations but have different meanings.

This is especially important in systems involving resource handles, identifiers, time measurements, memory offsets, simulation state, and asynchronous execution.

#### General Requirements

- Use distinct names for logically different identifiers, even when their underlying types are identical.
- Include units in identifiers when values could otherwise be misinterpreted.
- Distinguish indices, counts, offsets, capacities, and sizes according to their actual semantics.
- Distinguish CPU submission, GPU completion, and application-level frame progression when those concepts are independently tracked.
- Avoid generic names when the surrounding scope does not make their meaning unambiguous.
- Prefer domain-specific terminology over implementation-dependent descriptions.
- Names **MUST NOT** suggest guarantees that the underlying value or operation does not provide.

**Prefer:**

```cpp
xiiResourceHandle hResource;
xiiUInt64         uiSimulationStep;
xiiUInt64         uiCompletedStep;
xiiUInt32         uiBufferSizeBytes;
xiiUInt32         uiElementCount;
xiiUInt32         uiElementOffset;
float             fElapsedTimeSeconds;
```

**Avoid:**

```cpp
xiiResourceHandle hObject;
xiiUInt64         uiIndex;
xiiUInt64         uiDone;
xiiUInt32         uiSize;
xiiUInt32         uiCount;
xiiUInt32         uiOffset;
float             fTime;
```

The shorter names may be acceptable within a tightly constrained local scope where their meaning is unambiguous. However, public interfaces and long-lived member fields should use sufficiently descriptive names to remain understandable independently of nearby implementation details.

#### Units and Measurements

Unit-bearing identifiers **SHOULD** use explicit unit suffixes when the unit is not already guaranteed by a strongly typed abstraction.

Examples include:

- `uiBufferSizeBytes`
- `uiTimeoutMilliseconds`
- `fElapsedTimeSeconds`
- `fDistanceMeters`
- `uiFrequencyHertz`

Do not mix values expressed in different units without explicit conversion.

Where a dedicated type already encodes units, repeating the unit in every identifier may be unnecessary.

#### Identity and Ordering

Identifiers should distinguish identity from position or ordering.

For example:

```cpp
xiiUInt32 uiResourceIndex;
xiiUInt32 uiExecutionOrder;
xiiUInt32 uiPriority;
```

These names describe different concepts:

- An **index** identifies a position in a collection.
- An **execution order** describes sequencing.
- A **priority** expresses precedence according to a defined scheduling policy.

Do not call an execution index a priority unless the system actually implements priority semantics.

**Guiding principle:**

> Names should communicate what a value represents, not merely how it is stored. Identity, units, ordering, and scope must remain distinguishable.

### 3.8 Abbreviations, Acronyms, and Spelling

Abbreviations and acronyms **MUST** remain consistent with established XII terminology and commonly understood technical conventions.

Names should favor clarity over brevity, particularly in public APIs and cross-module interfaces.

#### General Requirements

- Prefer descriptive identifiers over abbreviations that require implementation-specific knowledge.
- Preserve established engine terminology and widely recognized technical acronyms.
- Do not invent multiple abbreviations for the same concept within a subsystem.
- Avoid introducing synonyms for established terms unless they represent meaningfully different concepts.
- Public APIs **SHOULD** use sufficiently descriptive names to remain understandable without implementation context.
- Identifiers, comments, documentation, assertions, and diagnostic messages **MUST** use correct spelling and terminology.
- Acronym capitalization **SHOULD** follow the existing conventions of the surrounding API.
- Do not rename stable public symbols solely to expand an established abbreviation without considering compatibility consequences.

**Prefer:**

```cpp
xiiUInt64 GetCompletedFrameIndex() const;
xiiUInt32 GetActiveResourceCount() const;
bool IsResourceResident() const;
```

**Avoid:**

```cpp
xiiUInt64 GetCFI() const;
xiiUInt32 GetARC() const;
bool IsResRes() const;
```

#### Established Acronyms

Common technical abbreviations are acceptable when they are widely recognized or already established within XII.

Examples include:

| Acronym | Meaning |
|---|---|
| `CPU` | Central Processing Unit |
| `GPU` | Graphics Processing Unit |
| `GAL` | Graphics Abstraction Layer |
| `API` | Application Programming Interface |
| `ID` | Identifier |
| `UI` | User Interface |
| `SRV` | Shader Resource View |
| `UAV` | Unordered Access View |
| `RT` | Render Target or Ray Tracing, depending on context |
| `SIMD` | Single Instruction, Multiple Data |
| `ECS` | Entity Component System |

An acronym **SHOULD NOT** be used when its meaning is ambiguous within the relevant subsystem.

For example, `RT` may refer to a render target or ray tracing. Where both concepts appear in the same context, prefer a more explicit name.

#### Terminology Consistency

A subsystem **SHOULD** establish consistent terminology for its primary concepts.

For example, if an API uses the term `ResourceHandle`, related interfaces should not arbitrarily alternate between `ResourceId`, `ResourceReference`, and `ResourceToken` unless those names represent distinct abstractions.

Likewise, distinguish registration, creation, acquisition, initialization, and loading when their operations have different lifecycle semantics.

Spelling and terminology are particularly important for public interfaces because names become part of the contract that developers rely on.

**Guiding principle:**

> Names are part of the interface. Favor clarity, preserve established terminology, and use abbreviations only when their meaning is sufficiently well understood.

## 4. File Organization and Headers

Source files and public interfaces **MUST** be organized to preserve clear module boundaries, minimize compilation dependencies, and support independent development across XII's subsystems.

This section extends the source layout requirements established in §2, focusing on header design, dependency management, translation-unit boundaries, template organization, and binary interfaces.

Formatting, include ordering, license placement, and the `Implementation` directory convention are governed by §§2.1–2.8 and are not repeated here.

### 4.1 One Clear Primary Responsibility per File

Each source file **SHOULD** have a coherent primary responsibility corresponding to a class, subsystem operation, or closely related group of functions.

- Prefer organizing files around meaningful responsibilities rather than arbitrary implementation size.
- Keep implementation-specific helpers near the component that owns them.
- Do not move unrelated types into generic `Utilities`, `Common`, or `Helpers` files merely because their implementations are small.
- Extract reusable functionality into independent files only when it has a clear responsibility and demonstrated consumers.
- Avoid distributing a single cohesive implementation across multiple translation units without a meaningful architectural reason.
- Avoid accumulating unrelated functionality in an existing file solely because it is convenient.
- File names **SHOULD** identify their principal type, interface, or responsibility.

Public headers and corresponding implementation files follow the directory organization defined in §2.8.

```text
Core/
└── ResourceManager/
    ├── ResourceManager.h
    ├── ResourceManagerTypes.h
    └── Implementation/
        ├── ResourceManager.cpp
        └── ResourceManagerTypes.cpp
```

Related types may share a file when their responsibilities are inseparable and separating them would introduce unnecessary dependencies or fragmentation.

**Guiding principle:**

> Organize files around cohesive responsibilities, not arbitrary size limits or superficial classification.

### 4.2 Header Self-Sufficiency and Complete Types

Public headers **MUST** be independently compilable and must declare their dependencies explicitly.

A header must not rely on another header, a precompiled header, or a particular inclusion order to provide required declarations.

The include organization and module `DLL.h` placement requirements are defined in §2.5.

#### General Requirements

- Public headers **MUST** contain the declarations and dependencies necessary for their independent compilation.
- A header **MUST NOT** rely on transitive includes for symbols it directly requires.
- Complete type definitions **MUST** be available wherever the C++ language requires them.
- Forward declarations **SHOULD** be used where legal and beneficial to reduce unnecessary dependencies.
- Avoid including implementation-specific headers in public interfaces unless their definitions are genuinely required.
- Public headers **SHOULD** minimize dependencies on unrelated modules.
- Header self-sufficiency **SHOULD** be validated through independent compilation or automated header checks.

**Prefer:**

```cpp
// ResourceLoader.h

#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Result.h>

class xiiResourceManager;

class XII_CORE_DLL xiiResourceLoader
{
public:
  xiiResult LoadResource(xiiStringView sPath);

  void SetResourceManager(xiiResourceManager* pManager);

private:
  xiiResourceManager* m_pManager = nullptr;
};
```

The forward declaration is sufficient because the header only stores a non-owning pointer to `xiiResourceManager`.

The corresponding implementation file includes the complete definition when it accesses the manager's members.

#### Incomplete Types and Ownership

Forward declarations are not universally interchangeable with includes.

Complete types are generally required for:

- Non-static data members stored by value.
- Base classes.
- Inline functions that access type members.
- Operations requiring an object's size or alignment.
- Template instantiations whose definitions require complete types.
- Destruction or allocation operations requiring a complete type.

Smart pointers may support declarations involving incomplete types, but their destruction and instantiation requirements must be respected.

**Prefer:**

```cpp
// SimulationController.h

#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Types/UniquePtr.h>

class xiiSimulationData;

class XII_CORE_DLL xiiSimulationController
{
public:
  xiiSimulationController();
  ~xiiSimulationController();

private:
  xiiUniquePtr<xiiSimulationData> m_pData;
};
```

```cpp
// Implementation/SimulationController.cpp

#include <Core/CorePCH.h>

#include <Core/Simulation/SimulationController.h>
#include <Core/Simulation/SimulationData.h>

xiiSimulationController::xiiSimulationController() = default;

xiiSimulationController::~xiiSimulationController() = default;
```

This pattern is valid when the smart pointer's implementation supports incomplete types and the out-of-line destructor is instantiated with the complete definition available.

Do not assume that all ownership wrappers have identical incomplete-type requirements.

**Guiding principle:**

> Headers must provide every dependency required by their contracts while avoiding dependencies needed only by their implementations.

### 4.3 Header Inclusion and One Definition Rule

XII uses `#pragma once` as its established header inclusion mechanism, as specified in §2.5.

In addition to preventing repeated inclusion, header design **MUST** respect C++ linkage and the One Definition Rule (ODR).

#### General Requirements

- Header definitions **MUST** be safe for inclusion in multiple translation units.
- Ordinary non-inline functions and variables with external linkage **MUST NOT** be defined in headers unless the language explicitly permits the intended definition pattern.
- Use `inline` or `inline constexpr` where appropriate for definitions intentionally shared across translation units.
- Avoid namespace-scope mutable state defined in headers.
- Do not rely on `#pragma once` to prevent ODR violations across separate translation units.
- Header-defined templates, inline functions, and constants **MUST** have consistent definitions across all translation units.
- Conditional compilation **MUST NOT** produce incompatible definitions of the same externally linked type or symbol.

**Avoid:**

```cpp
#pragma once

xiiUInt32 GetDefaultCapacity()
{
  return 1024U;
}
```

Including this header in multiple translation units can produce multiple definitions of the same externally linked function.

**Prefer:**

```cpp
#pragma once

XII_ALWAYS_INLINE xiiUInt32 GetDefaultCapacity()
{
  return 1024U;
}
```

Alternatively, declare the function in the header and provide its definition in the corresponding `.cpp` file.

For non-trivial functions, the latter is generally preferred.

**Guiding principle:**

> Header inclusion protection prevents repeated inclusion within a translation unit; correct linkage and definition placement prevent conflicts across translation units.

### 4.4 Public Interface Exposure and Dependency Boundaries

Public headers form part of a module's contract and influence every consumer that includes them.

Changes to public headers must therefore consider dependency direction, compilation costs, portability, and interface stability.

#### General Requirements

- Public interfaces **MUST NOT** expose private implementation details without a documented architectural reason.
- Backend-specific declarations **SHOULD** remain within their corresponding backend modules.
- Platform SDK and third-party types **SHOULD NOT** appear in portable public interfaces unless native interoperability is an explicit responsibility.
- Implementation-only dependencies **MUST** remain outside public headers where practical.
- Avoid exposing concrete storage containers when the contract requires only controlled operations or abstract resource access.
- Do not include large subsystem headers solely to obtain a small declaration when a narrower dependency is available.
- Public headers **MUST** respect the module dependency architecture.
- Cyclic dependencies **MUST** be resolved through appropriate interface design rather than dependence on include order.

#### Resolving Cyclic Dependencies

Circular dependencies frequently indicate unclear ownership or excessively coupled interfaces.

When resolving them, consider:

1. Replacing unnecessary includes with forward declarations.
2. Moving implementation-specific dependencies into `.cpp` files.
3. Extracting genuinely shared types into an appropriate lower-level header.
4. Separating unrelated responsibilities into distinct components.
5. Revising ownership or interface contracts when two components depend on each other's implementation details.

Do not introduce generic interface layers solely to eliminate every cycle without evaluating whether the additional abstraction improves the architecture.

#### Public Versus Private Dependencies

A dependency belongs in a public header when it is necessary to express or compile that header's public contract.

Dependencies required only for internal algorithms, storage management, platform operations, or implementation-specific functionality should remain private.

This separation helps reduce rebuild propagation and permits implementations to evolve without unnecessarily affecting consumers.

**Guiding principle:**

> A public header should reveal what a component provides, not how the component happens to implement it.

### 4.5 Internal Linkage and Translation-Unit Helpers

Symbols used exclusively within a single implementation file **SHOULD** have internal linkage or otherwise remain inaccessible outside that translation unit.

This prevents accidental dependencies on implementation details and reduces the risk of symbol collisions.

#### General Requirements

- Prefer unnamed namespaces for non-member helper functions, constants, and implementation-specific types that are local to a translation unit.
- File-local `static` functions and variables **MAY** be used when consistent with neighboring implementations.
- Do not export helper symbols without a demonstrated external consumer.
- Do not place translation-unit implementation details in public namespaces unnecessarily.
- Helpers **SHOULD** be located near the operations that use them when doing so improves readability.
- Avoid introducing globally accessible utility functions for logic used by only one component.
- Helper names **MUST** remain descriptive even when their scope is limited.

**Prefer:**

```cpp
namespace
{
  bool IsValidCapacity(xiiUInt32 uiCapacity)
  {
    return uiCapacity > 0U;
  }

  constexpr xiiUInt32 DefaultCapacity = 1024U;
}
```

An unnamed namespace limits these declarations to the translation unit.

For class-specific helpers that require private member access or participate in class invariants, prefer private member functions when that more accurately expresses their responsibility.

#### Avoid Unnecessary External Linkage

**Avoid:**

```cpp
bool ValidateInternalResourceState(const xiiResourceState& state)
{
  return state.m_bInitialized;
}
```

If this function is used only in one translation unit, external linkage provides no benefit.

Prefer giving it internal linkage unless it intentionally forms part of a shared interface.

**Guiding principle:**

> Keep implementation details local to the smallest scope that requires them. External visibility should be intentional.

### 4.6 Templates, Inline Functions, and Implementation Files

Templates and inline functions require particular attention because their definitions may need to be visible to multiple translation units.

Their placement **MUST** satisfy C++ instantiation and linkage requirements without unnecessarily increasing compilation dependencies.

#### Template Requirements

- Template definitions **MUST** be visible at their required instantiation points unless explicit instantiation is used.
- Template implementations **SHOULD** follow the existing module conventions for headers and `_inl.h` files.
- Do not place template definitions exclusively in `.cpp` files unless all required specializations are explicitly instantiated and the supported instantiation contract is documented.
- Avoid unnecessarily large template implementations in public headers when the code can be organized into a dedicated implementation include.
- Template interfaces **SHOULD** expose only the constraints and operations required by their consumers.
- Do not introduce template abstractions solely to avoid a small amount of ordinary implementation duplication.

#### Inline Functions

- Small accessors and simple operations **MAY** be defined inline when consistent with surrounding code.
- Substantial non-template implementations **SHOULD** remain in `.cpp` files.
- Do not assume that the `inline` keyword guarantees machine-code inlining.
- Do not move implementations into headers solely in pursuit of speculative performance improvements.
- Header-defined functions **MUST** respect linkage and ODR requirements.
- Performance-motivated inlining decisions **SHOULD** follow the evidence-based optimization requirements in §1.5.

**Prefer:**

```cpp
class xiiSimulationClock
{
public:
  XII_ALWAYS_INLINE xiiUInt64 GetStepCount() const
  {
    return m_uiStepCount;
  }

  void Advance(xiiTime deltaTime);

private:
  xiiUInt64 m_uiStepCount = 0U;
};
```

The trivial accessor is suitable for a header definition, while `Advance()` may contain substantial logic and belongs in the corresponding implementation file.

#### Template Implementation Includes

Where the repository uses `_inl.h` files, these **SHOULD** remain associated with the corresponding template declarations and follow the established include structure.

Do not introduce additional source-file extensions or template organization patterns without a clear benefit.

**Guiding principle:**

> Place definitions where C++ requires them, but keep implementation complexity out of public headers whenever practical.

### 4.7 Export Boundaries and Binary Interfaces

Modules intended for dynamic linking **MUST** use the appropriate XII DLL export and import declarations.

Public symbol visibility is part of the module boundary and must be managed deliberately.

The placement of each module's `DLL.h` is defined in §2.5.

#### General Requirements

- Exported classes, functions, and variables **MUST** use the appropriate module export macro.
- Symbols **MUST NOT** be exported solely for convenience when they are internal implementation details.
- Avoid unnecessary exposure of internal types, storage layouts, or ownership mechanisms through exported interfaces.
- Cross-module ownership and destruction responsibilities **MUST** be explicit.
- Public binary interfaces **MUST** consider calling conventions, type layout, alignment, and supported compiler or platform requirements.
- Changes to exported interfaces **MUST** be reviewed for compatibility with their intended consumers.
- Plugin and dynamically loaded module boundaries **MUST** account for initialization, shutdown, object lifetime, and module unloading.
- Do not assume that successful compilation guarantees binary compatibility.

**Example:**

```cpp
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
public:
  xiiResult Initialize();
  void Shutdown();
};
```

A public class may require the module export macro when its methods or type are intended to be consumed across dynamic library boundaries.

A forward declaration alone does not necessarily require an export annotation unless its particular usage or compiler contract requires one.

#### ABI Considerations

Exporting a concrete C++ class may expose aspects of its binary representation to dependent modules.

Potential compatibility concerns include:

- Data member layout and alignment.
- Virtual function table organization.
- Inheritance structure.
- Constructor and destructor behavior.
- Compiler and standard-library compatibility.
- Template and container implementation details.
- Allocation and deallocation across module boundaries.
- Exception and runtime-library configuration.

Where independently versioned binary compatibility is a requirement, opaque handles, controlled interfaces, or ABI-stable façade patterns **SHOULD** be considered.

These mechanisms **MUST NOT** be introduced automatically for every internal engine interface. Their complexity should be justified by actual deployment and compatibility requirements.

#### Exported Ownership Contracts

When resources are created in one module and released in another, their ownership and destruction mechanism must be explicit.

Avoid interfaces that permit callers to release module-owned allocations through an incompatible allocator or runtime.

Where appropriate, provide destruction operations through the same subsystem responsible for creation.

**Guiding principle:**

> Export only intentional contracts. Symbol visibility, ownership, and binary compatibility are architectural decisions, not merely compiler annotations.

### 4.8 Precompiled Headers and Compilation Independence

Precompiled headers are a build optimization and **MUST NOT** be treated as implicit dependency declarations.

The implementation-file ordering requirements, including placement of the repository's license notice before the module PCH, are defined in §2.5.

#### General Requirements

- Implementation files belonging to PCH-enabled modules **MUST** use the corresponding module PCH according to the repository's build configuration.
- Public headers **MUST NOT** depend on a PCH for required declarations.
- Implementation files **MUST** include their associated public declaration and other directly required headers.
- Removing or disabling PCH support **SHOULD NOT** reveal missing dependencies in otherwise valid source files.
- A PCH **MUST NOT** be used to conceal circular dependencies or invalid include relationships.
- Avoid adding narrowly used dependencies to a module PCH solely to eliminate local include directives.
- Changes to shared PCH content **SHOULD** consider compilation impact across the entire module.

**Prefer:**

```cpp
// Copyright and license notice.

#include <Core/CorePCH.h>

#include <Foundation/Containers/DynamicArray.h>

#include <Core/Simulation/SimulationController.h>
#include <Core/Simulation/SimulationData.h>
```

The PCH accelerates compilation, while the remaining includes document the translation unit's actual dependencies.

#### Compilation Validation

When modifying shared headers, PCH configuration, or module dependencies, contributors **SHOULD** consider validating builds with PCH disabled where the build system supports that configuration.

This helps identify accidental dependencies that ordinary PCH-enabled builds might conceal.

**Guiding principle:**

> A precompiled header may accelerate compilation, but it must never determine whether otherwise valid source code can compile correctly.

### 4.9 Public Header Stability and Compilation Cost

Public headers are frequently included throughout the engine and can have a disproportionate effect on build times and dependency propagation.

Changes to widely included headers **SHOULD** consider both architectural impact and compilation cost.

#### General Requirements

- Keep widely consumed headers focused on the declarations required by their consumers.
- Avoid introducing heavyweight dependencies into foundational headers without a demonstrated need.
- Prefer implementation-side changes when a public contract does not need to change.
- Do not introduce large implementation bodies into public headers solely to reduce the number of source files.
- Avoid unnecessary macro definitions in shared headers.
- Shared macros **MUST** use appropriate naming and scope conventions to minimize collisions.
- Changes to common type definitions **SHOULD** consider their effects on downstream compilation, reflection, serialization, and binary compatibility.
- Build-time improvements **SHOULD** be measured rather than assumed.

Forward declarations, explicit dependencies, and implementation separation can reduce compilation costs, but they should not compromise interface clarity or correctness.

**Guiding principle:**

> Shared headers are high-impact dependencies. Keep their contracts focused, their dependencies intentional, and their implementations appropriately isolated.

## 5. Types, Classes, and Interfaces

Types and interfaces **MUST** communicate their intended responsibilities, ownership semantics, invariants, and permitted operations.

XII favors cohesive types, explicit contracts, predictable object lifetimes, and controlled state transitions. Object-oriented abstractions should serve concrete architectural purposes rather than introduce unnecessary indirection or complexity.

The ownership and lifetime requirements established in §1.2, interface design principles in §1.3, and naming conventions in §3 apply throughout this section.

### 5.1 `struct` Versus `class`

Use `struct` for transparent data types whose members intentionally form a public data contract. Use `class` for types that encapsulate behavior, enforce invariants, manage resources, or control access to internal state.

This distinction communicates design intent. In C++, `struct` and `class` have equivalent capabilities and object representation rules; their primary language-level differences are default member accessibility and default inheritance accessibility.

#### General Requirements

- Use `struct` for plain data records, descriptors, configuration objects, and related value types whose members are intentionally accessible.
- Use `class` when internal state requires controlled mutation or when the type establishes meaningful behavioral invariants.
- Do not introduce getters and setters for every field of a transparent data structure without a concrete reason.
- Do not expose mutable fields merely to avoid implementing operations that preserve invariants.
- A `struct` **MAY** contain constructors, helper functions, and validation methods when these complement its role as a transparent data type.
- A `class` **MAY** contain public data when required by an established contract, but such exposure **SHOULD** be intentional.
- Do not select `struct` or `class` based on assumptions about performance, allocation, or memory layout.
- If a data structure develops invariants that cannot be reliably maintained through public mutation, reconsider its representation and interface.

**Prefer:**

```cpp
struct xiiSimulationSettings
{
  xiiUInt32 m_uiMaxIterations = 128U;
  float     m_fFixedTimeStep  = 0.016f;
  bool      m_bEnableGravity  = true;
};

class xiiSimulationController
{
public:
  xiiResult Initialize(const xiiSimulationSettings& settings);
  void Shutdown();

  bool IsInitialized() const;

private:
  bool m_bInitialized = false;
};
```

The settings structure represents a transparent configuration contract, while the controller encapsulates lifecycle behavior and the state transitions required to maintain it.

**Guiding principle:**

> Use `struct` to represent data and `class` to protect behavior and invariants. Choose based on semantics, not perceived complexity or performance.

### 5.2 Valid Initial States and Object Invariants

Objects **MUST** enter a well-defined state when constructed. Their initial state should be valid for the operations permitted by their documented contract.

A valid initial state does not necessarily mean a fully initialized or operational state. Some engine components require an explicit initialization phase, but their behavior before initialization must remain well-defined.

#### General Requirements

- Members **SHOULD** receive meaningful default values through default member initializers or constructors.
- Do not assume that callers will initialize every field of a public configuration structure manually.
- Members **MUST NOT** be read before they have been initialized.
- Default construction **MUST** establish a documented state.
- Partially initialized objects **MUST NOT** expose operations that require completed initialization without appropriate precondition enforcement.
- Sentinel values **MUST** have defined meanings and must not be confused with ordinary valid values.
- Prefer explicit state representations when several distinct lifecycle states must be represented.
- Avoid introducing additional state variables when the state can be derived reliably and efficiently from existing information.
- Initialization failures **MUST** leave objects in a documented state that permits safe cleanup, recovery, or destruction.

**Prefer:**

```cpp
struct xiiTaskSchedulerSettings
{
  xiiUInt32 m_uiWorkerCount   = 1U;
  xiiUInt32 m_uiQueueCapacity = 1024U;
  bool      m_bEnableTracing  = false;
};
```

**Avoid:**

```cpp
struct xiiTaskSchedulerSettings
{
  xiiUInt32 m_uiWorkerCount;
  xiiUInt32 m_uiQueueCapacity;
  bool      m_bEnableTracing;
};
```

The avoided implementation permits uninitialized members when a settings object is default-initialized without value initialization.

#### Initialization Failure

When construction cannot reasonably report an expected runtime failure, prefer a separate initialization operation or a factory that exposes the failure explicitly.

**Prefer:**

```cpp
class xiiDataStore
{
public:
  xiiResult Open(xiiStringView sPath);
  void Close();

  bool IsOpen() const;

private:
  bool m_bOpen = false;
};
```

The `Open()` operation may fail because of external conditions, such as an inaccessible file or invalid data.

Such failures should be represented through `xiiResult` or an appropriate richer error mechanism, consistent with §1.1.

An object that requires explicit initialization **MUST** prevent invalid operations before initialization and maintain safe destruction semantics.

**Guiding principle:**

> Every object must have a valid, predictable state throughout its lifetime, including before initialization, after failure, and during destruction.

### 5.3 Constructors and Initialization

Constructors **MUST** establish the invariants required for safe object use and destruction.

Prefer straightforward initialization over constructors that perform substantial work, introduce hidden dependencies, or obscure failure behavior.

#### General Requirements

- Prefer default member initializers for stable, context-independent defaults.
- Use constructor member initializer lists for values supplied through constructor parameters.
- References and `const` members **MUST** be initialized during construction.
- Members **SHOULD** be initialized in their declaration order to avoid misleading initialization dependencies.
- Constructors accepting a single argument **SHOULD** be marked `explicit` unless implicit conversion is intentional and documented.
- Prefer `= default` for constructors that require no custom behavior.
- Use `= delete` to explicitly prohibit construction or conversion operations where appropriate.
- Do not call virtual functions from constructors or destructors expecting derived-class dispatch.
- Constructors **MUST NOT** publish an object for external use before its required invariants are established.
- Avoid performing recoverable operations in constructors when those operations may legitimately fail and require error reporting.
- Constructor behavior **SHOULD** remain predictable and avoid unnecessary global state changes.

**Prefer:**

```cpp
class xiiFrameRing
{
public:
  explicit xiiFrameRing(xiiUInt32 uiCapacity)
    : m_uiCapacity(uiCapacity)
  {
    XII_ASSERT_DEV(uiCapacity > 0U, "Frame ring capacity must be greater than zero.");
  }

private:
  xiiUInt32 m_uiCapacity;
};
```

Here, a positive capacity is an explicit construction precondition. The assertion detects a programmer contract violation; it does not replace validation of untrusted input at the boundary where that input enters the system.

#### Initialization Order

C++ initializes members in declaration order, regardless of the order in which they appear in the constructor initializer list.

**Prefer:**

```cpp
class xiiBufferRange
{
public:
  xiiBufferRange(xiiUInt32 uiOffset, xiiUInt32 uiCount) :
    m_uiOffset(uiOffset), m_uiCount(uiCount)
  {
  }

private:
  xiiUInt32 m_uiOffset;
  xiiUInt32 m_uiCount;
};
```

Initializer lists **SHOULD** follow declaration order to make actual initialization behavior apparent.

#### Recoverable Construction Failures

XII does not use exceptions for ordinary error handling.

Constructors should therefore avoid operations that require recoverable failure reporting when a separate initialization method or factory provides a clearer contract.

**Prefer:**

```cpp
class xiiResourceDatabase
{
public:
  xiiResourceDatabase() = default;

  xiiResult Initialize(xiiStringView sDatabasePath);
  void Shutdown();
};
```

A factory may also be appropriate when partially initialized objects must never be exposed to callers.

The choice between explicit initialization and factory construction should reflect actual ownership and lifecycle requirements, not an arbitrary preference for one pattern.

**Guiding principle:**

> Constructors establish invariants. Recoverable initialization failures belong in operations that can communicate them explicitly.

### 5.4 Destructors and RAII

Resource Acquisition Is Initialization (RAII) **SHOULD** be used whenever resource ownership can be represented through object lifetime.

Resources acquired by an object should be released deterministically when that ownership ends.

#### General Requirements

- Destructors **MUST** preserve safe cleanup semantics.
- Destructors **MUST NOT** propagate exceptions across engine boundaries.
- Destruction **SHOULD** be deterministic and avoid unnecessary hidden work.
- Resources **MUST** be released using the mechanism appropriate to their ownership contract.
- RAII guards **SHOULD** represent one clear responsibility, such as locking a mutex, managing temporary state, or owning a resource.
- Explicit cleanup operations **MUST** account for subsequent destruction.
- Destructors **SHOULD NOT** perform unbounded waiting or unexpected expensive operations unless the type's contract explicitly requires them.
- Resource cleanup **MUST** respect outstanding asynchronous operations and external lifetime dependencies.
- Destructors **MUST NOT** depend on virtual dispatch to derived-class implementations.

#### Explicit Cleanup and Destruction

When a type provides both an explicit cleanup operation and destructor-based cleanup, repeated cleanup must not violate invariants.

**Prefer:**

```cpp
class xiiScopedOperation
{
public:
  ~xiiScopedOperation()
  {
    End();
  }

  void End()
  {
    if (!m_bActive)
      return;

    ReleaseResources();
    m_bActive = false;
  }

private:
  void ReleaseResources();

  bool m_bActive = true;
};
```

The `End()` operation is idempotent: invoking it more than once does not repeat resource release.

This is particularly useful for scoped operations that may finish explicitly before their owning object is destroyed.

However, idempotent cleanup should not be introduced universally. If repeated cleanup represents a programming error, an assertion or explicit lifecycle contract may be more appropriate.

#### Asynchronous Resource Destruction

Logical ownership and physical destruction may occur at different times.

For example, a GPU resource may become unavailable to new callers while its underlying allocation remains alive until submitted GPU work has completed.

Likewise, an asynchronous task may require captured state to remain valid until execution finishes.

Destructors and cleanup operations **MUST** respect these dependencies.

**Guiding principle:**

> Resource ownership should determine cleanup responsibility, while the resource's actual lifetime must satisfy every outstanding dependency.

### 5.5 Copy and Move Semantics

Copy and move operations **MUST** reflect the semantic identity, ownership, and lifetime requirements of a type.

Do not make types copyable or movable merely because C++ permits the operations.

#### Value Types

Value-like types **SHOULD** support copying when duplication is meaningful and reasonably efficient.

Examples include mathematical vectors, simple descriptors, configuration records, and small independent data objects.

Copying such objects should produce independent values with equivalent observable state.

#### Unique Ownership

Types that own resources exclusively **MUST NOT** permit copying unless a meaningful deep-copy operation is intentionally provided.

Moving may be appropriate when ownership can be transferred without violating invariants.

A move operation **MUST** leave both the destination and moved-from object in documented, safely destructible states.

#### Identity-Bearing Types

Managers, synchronization objects, resource registries, device contexts, and other identity-bearing types are generally noncopyable.

Moving such objects may also be unsafe when other components retain pointers, references, registrations, callbacks, or addresses referring to the original object.

**Prefer:**

```cpp
class xiiTaskScheduler
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTaskScheduler);

public:
  xiiTaskScheduler() = default;
  ~xiiTaskScheduler();

  xiiTaskScheduler(xiiTaskScheduler&&) = delete;
  xiiTaskScheduler& operator=(xiiTaskScheduler&&) = delete;
};
```

The move operations are explicitly prohibited because moving a scheduler may invalidate synchronization or registration state associated with its identity.

#### General Requirements

- Copy and move operations **MUST** preserve documented invariants.
- Exclusive ownership **MUST NOT** be duplicated accidentally.
- Moving a type **MUST NOT** invalidate externally registered addresses or references unless the contract explicitly permits it.
- Prefer compiler-generated copy and move operations when their behavior matches the intended semantics.
- Custom special member functions **SHOULD** be introduced only when ownership, resource management, or invariants require them.
- When declaring custom destructors or copy operations, review whether move operations must be explicitly defined or deleted.
- Do not infer a type's move behavior solely from the name of an engine macro.
- The actual definitions of copy-disabling macros **MUST** be considered before relying on their effects.

Prefer the Rule of Zero for types whose members already manage their own resources correctly.

For ownership-bearing types that require custom special member functions, define or prohibit the relevant operations deliberately.

**Guiding principle:**

> Copying duplicates a value; moving transfers state or ownership. Both operations must be valid according to the type's semantics, not merely its syntax.

### 5.6 Inheritance and Virtual Dispatch

Inheritance **SHOULD** represent a genuine substitutability relationship, an established extension contract, or a deliberate form of interface polymorphism.

Prefer composition when the objective is merely to reuse implementation details.

#### General Requirements

- Public inheritance **MUST** preserve the behavioral contract of the base type.
- Derived implementations **MUST NOT** weaken required preconditions or violate established postconditions.
- Overridden virtual functions **MUST** use `override`.
- Use `final` only when preventing further inheritance or overriding is an intentional design requirement.
- Polymorphic base classes **MUST** have an appropriate virtual destructor when deletion through a base pointer is permitted.
- Do not introduce virtual methods solely to make a type hypothetically extensible.
- Avoid deep inheritance hierarchies that obscure ownership, initialization, or behavioral responsibilities.
- Do not use inheritance merely to share member variables or common helper implementations.
- Virtual dispatch overhead **SHOULD** be evaluated through profiling when it occurs in demonstrated hot paths, as described in §1.5.

**Prefer:**

```cpp
class xiiSimulationBackend
{
public:
  virtual ~xiiSimulationBackend() = default;

  virtual xiiResult Initialize() = 0;
  virtual void Shutdown() = 0;
  virtual void StepSimulation(float fDeltaTime) = 0;
};

class xiiCpuSimulationBackend final : public xiiSimulationBackend
{
public:
  xiiResult Initialize() override;
  void Shutdown() override;
  void StepSimulation(float fDeltaTime) override;
};
```

The derived type implements an established behavioral interface rather than inheriting solely to reuse implementation.

#### Virtual Destruction

A base class intended for polymorphic deletion **MUST** provide a public virtual destructor or another explicitly enforced destruction mechanism.

A protected non-virtual destructor may be appropriate for interfaces that prohibit deletion through the base type and delegate destruction to a controlled owner.

Do not add virtual destructors to every class automatically. Polymorphism and destruction semantics must justify their use.

#### Avoiding Artificial Polymorphism

Do not introduce base classes, virtual functions, or runtime type registries when the set of implementations is fixed and ordinary composition or direct calls provide a simpler design.

Conversely, do not replace a cohesive polymorphic interface with manual type identifiers and conditional dispatch without a demonstrated benefit.

**Guiding principle:**

> Inheritance defines behavioral relationships. Use it to express substitutability, not merely to share code or anticipate hypothetical extensions.

### 5.7 Interface Granularity and Capability Exposure

Interfaces **MUST** expose meaningful capabilities while minimizing unnecessary knowledge of internal state.

The general principles of small, cohesive interfaces are established in §1.3. This section focuses on their application to C++ type design.

#### General Requirements

- Public methods **SHOULD** represent meaningful operations rather than direct access to implementation details.
- Avoid exposing internal containers, registration tables, synchronization objects, or mutable storage without a documented requirement.
- Do not return unrestricted mutable references to manager-owned resources when doing so bypasses invariants.
- Prefer controlled operations when resource mutation requires validation, synchronization, or lifecycle management.
- Do not introduce multiple layers of accessors that merely forward calls without establishing meaningful contracts.
- Avoid expanding interfaces to support hypothetical consumers.
- Interface methods **SHOULD** have clearly defined preconditions, postconditions, and failure behavior.
- Read-only access **SHOULD** be preferred when mutation is unnecessary.
- Borrowed access **MUST** have valid and understandable lifetime guarantees.

**Prefer:**

```cpp
class xiiTaskRegistry
{
public:
  xiiResult RegisterTask(xiiTaskHandle hTask);
  xiiResult UnregisterTask(xiiTaskHandle hTask);

  bool IsRegistered(xiiTaskHandle hTask) const;
};
```

**Avoid:**

```cpp
class xiiTaskRegistry
{
public:
  xiiDynamicArray<TaskEntry>& GetMutableEntries();
};
```

The avoided interface exposes internal storage and permits callers to bypass registration rules, potentially invalidating indices, handles, or synchronization assumptions.

#### Capability-Oriented Interfaces

Interfaces should communicate what consumers are permitted to do, not merely what data the implementation contains.

However, exposing an immutable view or container may be appropriate when iteration itself is a documented capability and its lifetime and invalidation semantics are well-defined.

Controlled interfaces should not become unnecessarily elaborate wrappers around operations that are already safe and self-explanatory.

**Guiding principle:**

> Expose the operations consumers need while retaining control of the invariants consumers must not violate.

### 5.8 Encapsulation and Invariant Preservation

Encapsulation is meaningful only when it protects an actual contract.

Marking a field `private` does not guarantee encapsulation if callers can obtain unrestricted mutable access to the same state.

#### General Requirements

- State transitions **MUST** preserve the invariants owned by the component.
- Mutable access to internal state **SHOULD** be restricted when arbitrary modification could violate those invariants.
- Methods **SHOULD** express valid transitions rather than requiring callers to coordinate internal fields manually.
- Internal synchronization requirements **MUST NOT** be bypassed through unguarded public references or pointers.
- Avoid exposing implementation storage solely to simplify a caller's code.
- Do not introduce accessors that provide no meaningful encapsulation benefit.
- Invariants **MUST** remain valid after operations that report failure, subject to their documented failure guarantees.
- Components **SHOULD** centralize validation and transition logic rather than duplicate it across unrelated callers.

**Prefer:**

```cpp
class xiiSimulationSession
{
public:
  xiiResult Begin()
  {
    if (m_bActive)
      return XII_FAILURE;

    m_bActive = true;
    return XII_SUCCESS;
  }

  void End()
  {
    m_bActive = false;
  }

  XII_ALWAYS_INLINE bool IsActive() const
  {
    return m_bActive;
  }

private:
  bool m_bActive = false;
};
```

**Avoid:**

```cpp
class xiiSimulationSession
{
public:
  XII_ALWAYS_INLINE bool& GetActiveState()
  {
    return m_bActive;
  }

private:
  bool m_bActive = false;
};
```

The avoided implementation allows external code to modify state without passing through the operations responsible for enforcing valid transitions.

The preferred example is intentionally simple; `Begin()` and `End()` are justified when they represent meaningful lifecycle transitions. A plain boolean field in a transparent configuration structure would not require this abstraction.

**Guiding principle:**

> Encapsulation is the enforcement of meaningful contracts, not the mechanical replacement of fields with getters and setters.

### 5.9 Separate Ownership from Execution Policy

Object ownership and execution policy are distinct architectural responsibilities.

A component may own, register, or manage the lifetime of an object without determining when or how that object executes.

These responsibilities **SHOULD** remain separate unless their coupling is an explicit part of the subsystem's contract.

#### General Requirements

- Registration **MUST NOT** implicitly imply execution unless documented by the owning subsystem.
- Ownership **MUST NOT** automatically imply scheduling, activation, or per-frame participation.
- Execution order, priority, frequency, and synchronization **SHOULD** be represented independently when they are distinct concepts.
- Avoid embedding global scheduling assumptions into otherwise reusable resource or object managers.
- Components **SHOULD** be reusable across execution contexts when their responsibilities permit it.
- Do not introduce a separate scheduler or execution abstraction when ownership and execution are inherently inseparable and no meaningful benefit would result.
- Changes to execution policy **SHOULD NOT** require restructuring ownership contracts unnecessarily.

#### Example

A simulation registry may own simulation instances while a separate scheduler determines which instances participate in a particular update.

```cpp
class xiiSimulationRegistry
{
public:
  xiiSimulationHandle CreateSimulation();
  xiiResult DestroySimulation(xiiSimulationHandle hSimulation);
};

class xiiSimulationScheduler
{
public:
  xiiResult ScheduleSimulation(xiiSimulationHandle hSimulation);
  xiiResult UnscheduleSimulation(xiiSimulationHandle hSimulation);

  void ExecuteScheduled(float fDeltaTime);
};
```

The registry owns simulation lifetime, while the scheduler controls execution participation.

This separation is appropriate when simulations may be created independently of scheduling or participate in different execution policies.

For smaller systems where ownership and execution are necessarily coupled, a single cohesive component may be preferable.

**Guiding principle:**

> Ownership determines who manages an object. Execution policy determines when and how it participates in work. Do not conflate these responsibilities without a concrete reason.

### 5.10 Favor Composition Over Unnecessary Inheritance

Composition **SHOULD** be preferred when a type needs to use or own another component rather than satisfy its behavioral contract.

Composition permits responsibilities to evolve independently and generally makes ownership and lifetime relationships easier to understand.

#### General Requirements

- Prefer composition for owned utilities, allocators, profilers, caches, synchronization objects, and supporting services.
- Use inheritance when substitutability or an established extension interface provides a meaningful benefit.
- Avoid inheriting from a type solely to access its implementation details or reuse a few methods.
- Do not introduce deep inheritance hierarchies to share common data members.
- Composed objects **MUST** have clearly defined ownership and lifetime relationships.
- Prefer direct composition over unnecessary service locators, global registries, or dependency injection layers.
- Dependencies **SHOULD** be provided explicitly when doing so clarifies ownership and testability.
- Do not fragment a cohesive type into numerous single-purpose objects without a demonstrated architectural benefit.

**Prefer:**

```cpp
class xiiSimulationSystem
{
public:
  xiiResult Initialize();
  void Shutdown();

private:
  xiiTaskScheduler   m_TaskScheduler;
  xiiSimulationClock m_SimulationClock;
};
```

**Avoid:**

```cpp
class xiiSimulationSystem : public xiiTaskScheduler, public xiiSimulationClock
{
public:
  xiiResult Initialize();
  void Shutdown();
};
```

The simulation system uses scheduling and timing capabilities; it is not itself necessarily a scheduler or a clock.

Composition communicates that relationship without exposing the implementation details of its supporting components through inheritance.

#### Dependency Lifetime

Composition also clarifies destruction order.

C++ destroys member objects in the reverse order of their declaration. Dependencies between composed members **MUST** respect this behavior.

When one member depends on another during destruction, their declaration order and cleanup contracts must ensure that the dependency remains alive for the required duration.

**Guiding principle:**

> Prefer composition to express what a component has or uses. Reserve inheritance for what a component genuinely is or promises to implement.

## 6. Initialization, Ownership, and Lifetime

Resource management in XII **MUST** be deterministic, explicit, and consistent with the lifetime guarantees of the underlying subsystem.

This applies to ordinary heap allocations, engine-managed resources, platform objects, asynchronous operations, GPU resources, plugin registrations, and any object whose lifetime extends beyond a single scope.

The general ownership principles established in §1.2 and the type design requirements in §5 apply throughout this section. This section defines the practical requirements for allocation, initialization, borrowing, stable identity, resource invalidation, and coordinated destruction.

### 6.1 The Ownership Rule

Every dynamically allocated object and long-lived resource **MUST** have an identifiable owner or an explicitly defined shared ownership policy.

Ownership determines which component is responsible for releasing a resource. Borrowed access does not transfer or extend that responsibility.

#### General Requirements

- Every resource **MUST** have a clearly defined ownership and destruction mechanism.
- Prefer automatic storage and value semantics when dynamic allocation or independent identity is unnecessary.
- Use `xiiUniquePtr` when exclusive heap ownership is appropriate.
- Use `xiiSharedPtr` only when multiple independent owners genuinely require shared lifetime management.
- Raw pointers, references, spans, and views **MUST** be treated as non-owning unless an API explicitly documents an ownership-transfer contract.
- Ownership transfers **MUST** be visible through the API and supported by the relevant type semantics.
- Do not introduce shared ownership merely to compensate for unclear lifetime relationships.
- Do not maintain competing ownership mechanisms for the same resource.
- Resource-owning objects **MUST** remain safely destructible after partial initialization or recoverable failure.
- Ownership and thread safety **MUST** be evaluated independently.

**Prefer:**

```cpp
struct xiiSimulationEntry
{
  xiiUniquePtr<xiiSimulation> m_pSimulation;
  xiiSimulationState          m_State;
};
```

The entry has exclusive ownership of its simulation object, while the associated state follows the entry's lifetime.

**Avoid:**

```cpp
struct xiiSimulationEntry
{
  xiiSimulation* m_pSimulation = nullptr;

  // Ownership is unspecified.
  // The caller may not know who destroys the simulation.
};
```

An owning raw pointer may be required when interacting with a specialized allocator or legacy interface, but its ownership contract **MUST** be explicit and consistently enforced.

#### Shared Ownership

Shared ownership introduces additional costs and lifetime complexity.

These may include:

- Reference-counting operations.
- Atomic synchronization or contention.
- Delayed and less predictable destruction.
- Ownership cycles.
- Difficulty coordinating resource release with subsystem shutdown.
- Resources remaining alive beyond plugin or device lifetime.

Shared ownership **SHOULD NOT** be the default solution to lifetime uncertainty.

When an object has one authoritative manager, prefer manager-owned storage and non-owning, validated access where appropriate.

**Guiding principle:**

> Every resource must have an accountable owner. Borrowing grants access, not ownership, and shared ownership must reflect an actual lifetime requirement.

### 6.2 Allocation and Allocator Compatibility

XII's allocation mechanisms **MUST** be used consistently with the allocator responsible for the resource.

The mechanism used to destroy an object must be compatible with the mechanism used to construct it.

#### General Requirements

- Use XII allocation facilities when required by the owning subsystem.
- Allocations made using `XII_NEW` **MUST** be released using the corresponding compatible XII destruction mechanism.
- Do not arbitrarily mix `XII_NEW` and `XII_DELETE` with unrelated C++ `new` and `delete` operations.
- Use `XII_DEFAULT_NEW` and `XII_DEFAULT_DELETE` to use the default allocator when no other allocator is required by the owning subsystem.
- Objects created through custom allocators **MUST** retain or otherwise have access to the allocator required for destruction.
- Containers **SHOULD** use the allocator associated with their owning subsystem when allocator selection is part of that subsystem's contract.
- Avoid introducing independent allocations when values can be stored directly without compromising lifetime or address stability.
- Allocator ownership and lifetime **MUST** be compatible with the lifetime of every allocation obtained from it.
- Memory **MUST NOT** be released through an allocator that has already been destroyed or invalidated.
- Cross-module allocations **MUST** respect the allocation and destruction contract of the module boundary.

**Example:**

```cpp
xiiUniquePtr<xiiSimulationData> pData = XII_DEFAULT_NEW(xiiSimulationData);
```

The example assumes the XII ownership wrapper and allocation macro support the corresponding allocator-aware construction and destruction semantics.

#### Explicit Allocator Dependencies

When an object requires a particular allocator, prefer expressing that dependency through its constructor or initialization contract.

**Prefer:**

```cpp
class xiiSimulationStorage
{
public:
  explicit xiiSimulationStorage(xiiAllocator* pAllocator) :
    m_Entries(pAllocator)
  {
    XII_ASSERT_DEV(pAllocator != nullptr, "A valid allocator is required.");
  }

private:
  xiiDynamicArray<xiiUInt32> m_Entries;
};
```

**Avoid:**

```cpp
class xiiSimulationStorage
{
public:
  xiiSimulationStorage() :
    m_Entries(GetGlobalSimulationAllocator())
  {
  }

private:
  xiiDynamicArray<xiiUInt32> m_Entries;
};
```

The avoided implementation introduces a hidden dependency on global allocator state.

A global or subsystem allocator may be appropriate when it is part of an established engine-wide allocation policy. However, allocator dependencies should not be hidden when explicit selection is necessary for correctness, testing, or lifetime management.

#### Allocator Lifetime

An allocator must remain valid until all allocations that depend on it have been released.

This requirement is particularly important for:

- Frame and temporary allocators.
- Pool and arena allocators.
- Plugin-specific allocators.
- Module-local heaps.
- Device-scoped resource allocators.
- Memory shared across dynamically loaded modules.

Resetting an arena or destroying a pool invalidates allocations owned by that allocator according to its documented semantics.

**Guiding principle:**

> Allocation and destruction are a single ownership contract. The allocator, allocation, and owning object must have compatible lifetimes.

### 6.3 Borrowing Is Not Ownership

A borrowed pointer, reference, or view provides temporary access to an object according to the owner's lifetime contract.

Borrowing **MUST NOT** be interpreted as ownership transfer or a guarantee that the referenced object will remain alive indefinitely.

#### General Requirements

- Borrowed access **MUST** remain within the validity period guaranteed by the owner.
- A borrowed pointer **MUST NOT** be destroyed by the borrower unless ownership transfer is explicitly documented.
- Borrowed access **MUST NOT** be retained beyond owner destruction or resource invalidation.
- Borrowed access **MUST NOT** be assumed thread-safe.
- Do not capture borrowed pointers in asynchronous operations without guaranteeing the referenced object's lifetime.
- Do not expose borrowed references when their validity cannot be adequately described or enforced.
- Prefer references when null is not a meaningful state and the lifetime contract permits reference semantics.
- Prefer pointers when optional access, nullability, or reseating is meaningful.

**Example:**

```cpp
xiiGALDevice* pDevice = m_pDevice.Borrow();

if (pDevice == nullptr)
  return XII_FAILURE;

InitializeDeviceResources(*pDevice);
```

The pointer is valid only while the owning object guarantees the device's lifetime.

The `Borrow()` operation does not extend that lifetime.

#### Borrowing Across Asynchronous Boundaries

Borrowed access becomes particularly dangerous when execution is deferred.

**Avoid:**

```cpp
void SubmitTask()
{
  xiiString sMessage = "Simulation complete";

  QueueTask([&sMessage]()
  {
    LogMessage(sMessage);
  });
}
```

The callback may execute after `sMessage` has been destroyed.

**Prefer:**

```cpp
void SubmitTask()
{
  xiiString sMessage = "Simulation complete";

  QueueTask([sMessage = std::move(sMessage)]()
  {
    LogMessage(sMessage);
  });
}
```

The callback owns its captured string, assuming the task system stores and executes the callable according to the required ownership contract.

Copying or moving captured data is not always necessary. Borrowing remains appropriate when the asynchronous operation's completion is guaranteed before the referenced storage becomes invalid.

**Guiding principle:**

> Borrowed access is valid only for the lifetime and synchronization guarantees explicitly provided by its owner.

### 6.4 Stable Identity Is Not Stable Address

Object identity and memory address are distinct concepts.

A handle identifies a logical object through an authoritative subsystem, while a pointer identifies a particular memory location.

Neither mechanism automatically guarantees the continued lifetime of the referenced object.

#### General Requirements

- Use handles when objects require independently managed identity, validation, or generation tracking.
- Use pointers and references when direct access is appropriate and their lifetime is guaranteed.
- Do not assume that a container preserves element addresses unless its contract explicitly guarantees that behavior.
- Container growth, insertion, removal, compaction, or relocation **MUST** be considered when storing pointers or references to elements.
- Generation-checked handles **SHOULD** be used when reusable registry slots could otherwise allow stale identifiers to reference unrelated objects.
- Handle validity **MUST** be checked when the calling contract does not guarantee validity.
- Stable allocation addresses **MUST NOT** be confused with permanent resource validity.
- Handles **MUST NOT** be treated as ownership tokens unless the subsystem explicitly defines them that way.

**Avoid:**

```cpp
xiiDynamicArray<Entry> m_Entries;

xiiIdTable<GraphId, Entry*, xiiLocalAllocatorWrapper> m_EntryTable;

// Pointers into m_Entries may be invalidated when
// the container reallocates or relocates its elements.
```

**Prefer when stable allocation addresses are required:**

```cpp
xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;

// Moving or growing the pointer container does not
// relocate the independently allocated Entry objects.
```

This representation preserves the addresses of surviving allocated entries during relocation of the container's pointer elements.

It does not preserve references after the corresponding entry is destroyed.

#### Handles and Generations

A generation-checked handle generally combines a resource slot or index with information that distinguishes different lifetimes of that slot.

When a resource is destroyed, its previous generation becomes invalid according to the registry's rules.

This prevents certain stale-handle errors, including accidental access to a newly allocated object that reuses an earlier slot.

However, generation tracking does not automatically provide:

- Thread-safe lookup.
- Atomic lifetime acquisition.
- Protection against concurrent destruction after lookup.
- Protection against generation wraparound.
- Ownership of the referenced resource.

These guarantees require separate mechanisms.

**Guiding principle:**

> Handles preserve logical identity through validation. Pointers provide direct access to memory. Neither alone establishes a lifetime or concurrency guarantee.

### 6.5 Lifetime-Safe Lookup and Concurrent Access

A successful resource lookup does not necessarily guarantee that the resource remains alive after the lookup completes.

This is especially important when registration, lookup, and destruction may occur concurrently.

#### General Requirements

- Resource lookup **MUST** respect the synchronization contract of the owning registry.
- A lookup mutex **MUST NOT** be assumed to protect a resource after the lock has been released.
- Returned pointers and references **MUST** remain valid for the documented duration of access.
- Resource destruction **MUST NOT** race with active access that requires the resource to remain alive.
- Lookup and lifetime protection **MUST** be coordinated when concurrent destruction is permitted.
- Do not return unrestricted mutable access to internally synchronized resources unless callers receive an enforceable synchronization contract.
- Thread-affine registries **MUST** document their permitted access and mutation contexts.
- Avoid introducing shared ownership solely for lookup convenience when scoped access or execution-boundary guarantees are sufficient.

**Avoid:**

```cpp
Entry* GetEntry(xiiResourceHandle hResource)
{
  XII_LOCK(m_Mutex);

  Entry* pEntry = nullptr;
  m_ResourceTable.TryGetValue(hResource, pEntry);

  return pEntry;
}
```

The mutex protects the lookup operation, but it does not necessarily protect the returned pointer.

Another thread may destroy the entry immediately after the function releases the mutex.

#### Valid Lifetime Strategies

The appropriate strategy depends on the subsystem's concurrency and execution model.

Common approaches include:

1. **Scoped locking:** Hold the required lock throughout the operation that accesses the resource.
2. **Scoped access guards:** Return an object that maintains the necessary lifetime and synchronization protection.
3. **Shared ownership:** Retain a strong ownership reference for the duration of access when shared lifetime semantics are appropriate.
4. **Execution-boundary mutation:** Permit registration and destruction only at defined synchronization points.
5. **Deferred reclamation:** Delay physical destruction until all active readers or operations have completed.

These strategies are alternatives rather than mechanisms that must all be implemented.

The simplest approach satisfying the actual concurrency requirements **SHOULD** be preferred.

#### Scoped Access

When resource access must remain synchronized, prefer an API that performs the operation within the required lifetime boundary.

For example:

```cpp
xiiResult UpdateResource(xiiResourceHandle hResource)
{
  XII_LOCK(m_Mutex);

  Entry* pEntry = nullptr;

  if (!m_ResourceTable.TryGetValue(hResource, pEntry))
    return XII_FAILURE;

  return pEntry->Update();
}
```

This pattern is appropriate when the mutex protects the entire operation and `Update()` does not violate the lock's reentrancy, ordering, or contention requirements.

Long-running operations should not automatically execute while holding a global registry mutex.

Where that would cause unacceptable contention, use a more appropriate scoped lifetime mechanism.

**Guiding principle:**

> A lookup establishes that an object existed at a particular moment. Safe access requires that its lifetime and synchronization guarantees remain valid throughout the operation.

### 6.6 Parent-Child and Subsystem Lifetimes

Resources frequently depend on other objects or subsystems for their operation and destruction.

These dependencies **MUST** be reflected in initialization and shutdown ordering.

#### General Requirements

- Dependent resources **MUST** be released before the infrastructure required for their destruction becomes unavailable.
- Initialization order **SHOULD** follow dependency direction.
- Shutdown order **SHOULD** reverse dependency order where appropriate.
- Explicit shutdown operations **MUST** prevent new work from acquiring resources that are being destroyed.
- Objects **MUST NOT** access dependencies after those dependencies have been destroyed or invalidated.
- Plugin-owned callbacks, delegates, and registrations **MUST** be removed before unloading the plugin code they reference.
- Background tasks **MUST** complete, cancel, or otherwise release their dependencies before the owning subsystem is destroyed.
- Shutdown **MUST** account for partially initialized subsystems.
- Avoid relying on unspecified static initialization or destruction order across translation units or dynamically loaded modules.

#### Dependency Ordering

For example, a rendering subsystem may have the following dependency hierarchy:

```text
GAL Device
  └── Device Resource Cache
      └── Render Graph Resources
          └── Frame Execution State
```

If each child depends on its parent during destruction, shutdown must release the dependent objects before the required parent infrastructure.

A typical shutdown sequence would therefore release:

1. Active frame execution state.
2. Graph-specific resources.
3. Device-scoped caches.
4. The underlying GAL device.

The exact sequence depends on the actual ownership and synchronization contracts.

It must not be inferred solely from the visual hierarchy.

#### Plugin Lifetime

Dynamically loaded plugins introduce an additional dependency: executable code may disappear when the module is unloaded.

Before unloading a plugin, the engine **MUST** ensure that no active operation retains a callback, virtual dispatch target, function pointer, or other executable reference into the unloaded module.

This includes asynchronous tasks and deferred callbacks that may execute after the plugin's shutdown function returns.

**Guiding principle:**

> Dependencies must outlive every operation that relies on them. Initialization establishes those dependencies; shutdown must dismantle them safely.

### 6.7 Resource Invalidation and Destruction

Resource invalidation **MUST** distinguish between invalidating a caller's reference, removing a resource from a registry, and physically destroying its underlying allocation.

These events may occur at different times.

#### General Requirements

- Public handles **SHOULD** be invalidated after successful destruction when the API supports explicit invalidation.
- Invalidating one handle variable **MUST NOT** be interpreted as invalidating copies held elsewhere.
- The authoritative registry **MUST** reject handles that are stale according to its generation and validity rules.
- Destruction operations **MUST** preserve registry consistency.
- Resource removal **MUST** prevent future lookups from acquiring access when the resource is no longer logically available.
- Physical destruction **MUST** respect outstanding references and asynchronous execution dependencies.
- Repeated destruction attempts **MUST** have documented behavior.
- Failed destruction operations **MUST** leave resource ownership and validity in a documented state.

**Example:**

```cpp
if (DestroyResource(hResource).Succeeded())
{
  hResource.Invalidate();
}
```

The caller invalidates its local handle only after the destruction operation reports success.

This does not revoke other copies of the handle.

The resource registry remains responsible for rejecting stale identifiers.

#### Logical and Physical Destruction

In asynchronous systems, resource destruction may involve multiple phases:

1. **Logical invalidation:** The resource becomes unavailable for new operations.
2. **Retirement:** Existing operations are allowed to complete or release their references.
3. **Physical destruction:** The underlying allocation and associated objects are released.

Not every subsystem requires all three phases.

A simple CPU-owned object with no outstanding references may be destroyed immediately.

GPU resources, asynchronously accessed objects, and concurrently readable registry entries may require deferred reclamation.

**Guiding principle:**

> Invalidating a reference prevents future intended use. Safe destruction additionally requires that no outstanding operation can still access the resource.

### 6.8 Non-Owning Views and Scope-Bound Data

Views, spans, borrowed strings, iterators, mapped memory, and similar abstractions provide access without necessarily controlling the lifetime of the referenced storage.

Their validity **MUST** remain within the lifetime and invalidation guarantees of the underlying owner.

#### General Requirements

- Non-owning views **MUST NOT** outlive their backing storage.
- Views **MUST NOT** be retained across container mutations that invalidate their referenced storage.
- Do not return views or references into temporary objects that are destroyed before the caller can use them.
- Do not persist borrowed stack storage in asynchronous callbacks or long-lived objects.
- Copy data into owning storage when its lifetime must extend beyond the original owner's guarantees.
- Mapping objects and scoped access helpers **MUST** respect their required unmapping or release operations.
- Borrowed views **MUST NOT** be assumed safe for concurrent mutation.
- Prefer explicit ownership when lifetime extension cannot be guaranteed reliably.

**Avoid:**

```cpp
xiiStringView MakeResourceName()
{
  xiiString sName = "SimulationResource";

  return sName;
}
```

The returned view references storage owned by a local string that is destroyed when the function returns.

**Prefer:**

```cpp
xiiString MakeResourceName()
{
  return xiiString("SimulationResource");
}
```

The returned string owns its storage.

#### Container Invalidation

Container operations may invalidate references, pointers, iterators, or views into their contents.

Code **MUST** respect the invalidation guarantees of the specific container being used.

Do not assume that all XII containers share the same relocation or iterator stability semantics.

For example, growing a contiguous dynamic array may relocate its elements, invalidating previously obtained pointers.

A container providing stable element addresses may have different guarantees, but element removal still invalidates access to the removed object.

#### Scoped Mapping

Memory obtained through a mapping operation may remain valid only until its corresponding unmap or release operation.

Such pointers **MUST NOT** be retained beyond that boundary.

This applies to CPU memory mappings, GPU-visible mapped ranges, temporary allocator storage, and similar scoped resources.

**Guiding principle:**

> A view grants access to existing storage; it does not extend the storage's lifetime. Persist ownership, not borrowed access, when data must survive its original scope.

### 6.9 Runtime and GPU Resource Lifetimes

CPU-side execution completion and GPU execution completion are different events.

A function returning from command recording, submission, or render graph execution does not necessarily mean that the GPU has finished accessing the submitted resources.

GPU resource lifetime **MUST** account for asynchronous execution.

#### General Requirements

- CPU-side submission completion **MUST NOT** be treated as proof of GPU execution completion.
- Resources referenced by submitted GPU work **MUST** remain valid until the GPU has completed the operations that require them.
- Resource retirement **MUST** use appropriate queue completion, fence, timeline, or equivalent synchronization guarantees.
- Command allocators, descriptor storage, upload buffers, temporary resources, and other GPU-visible allocations **MUST** respect their actual in-flight lifetimes.
- Frame-index progression **MUST NOT** be confused with GPU completion unless the frame-management contract explicitly establishes that relationship.
- Resource reuse **MUST** respect outstanding GPU dependencies.
- Deferred destruction mechanisms **MUST** preserve the resources and associated device infrastructure required for final cleanup.
- Device shutdown **MUST** account for outstanding submissions and pending resource retirement.
- Synchronization mechanisms **MUST** follow the guarantees of the active graphics backend.

#### Submission Versus Completion

Consider a typical sequence:

```text
CPU:
  Record Commands
        |
        v
  Submit Commands
        |
        v
  Execute() Returns
        |
        v
  Continue CPU Work

GPU:
        Execute Submitted Commands
                    |
                    v
              Signal Completion
```

The CPU may continue execution while the GPU is still processing previously submitted commands.

Destroying a resource immediately after the submission function returns may therefore be unsafe.

#### Deferred Resource Retirement

A rendering subsystem may logically release a resource immediately while retaining its physical allocation until the relevant GPU completion condition is satisfied.

For example:

```cpp
void ReleaseResource(xiiGALResourceHandle hResource)
{
  QueueResourceRetirement(hResource, GetCurrentSubmissionValue());
}
```

This is a conceptual example. The retirement system must ensure that the recorded completion value covers every GPU submission that may still access the resource.

A resource may have dependencies across multiple queues or submissions. The last submission recorded by a local subsystem is not necessarily sufficient to establish safe destruction.

#### Frames in Flight

Frame-buffering strategies commonly retain resources across several in-flight frames.

However, a fixed number of frames in flight is not itself a completion guarantee.

Resource reuse and destruction **MUST** be based on the synchronization semantics of the frame system and graphics backend.

Do not assume that advancing a frame counter by a particular amount proves that earlier GPU work has completed.

Detailed GPU synchronization, queue completion, and retirement requirements are covered in §15.

**Guiding principle:**

> CPU execution determines when work is submitted. GPU synchronization determines when its resources can be safely reused or destroyed. These lifetimes must never be conflated.

### 6.10 Initialization and Shutdown State Machines

Subsystem initialization and shutdown **MUST** follow a documented lifecycle.

A component should not require callers to infer which operations are valid from partially initialized internal state.

#### General Requirements

- Initialization and shutdown operations **MUST** define their valid calling conditions.
- Repeated initialization **MUST** have documented behavior.
- Repeated shutdown **MUST** either be supported safely or treated as an explicit contract violation.
- Initialization failures **MUST** leave the component in a state where cleanup or recovery is possible.
- Shutdown **MUST** prevent new work from accessing resources that are being retired.
- Lifecycle state **SHOULD** remain minimal and correspond to actual observable requirements.
- Do not introduce elaborate state machines for simple components that require only initialized and uninitialized states.
- Explicit initialization methods **SHOULD** return `xiiResult` when initialization may fail for expected runtime reasons.

**Example:**

```cpp
class xiiSubsystem
{
public:
  xiiResult Initialize()
  {
    if (m_bInitialized)
      return XII_FAILURE;

    if (InitializeResources().Failed())
      return XII_FAILURE;

    m_bInitialized = true;
    return XII_SUCCESS;
  }

  void Shutdown()
  {
    if (!m_bInitialized)
      return;

    ReleaseResources();
    m_bInitialized = false;
  }

  XII_ALWAYS_INLINE bool IsInitialized() const
  {
    return m_bInitialized;
  }

private:
  xiiResult InitializeResources();
  void ReleaseResources();

  bool m_bInitialized = false;
};
```

This example assumes `InitializeResources()` either cleans up any partial acquisitions before returning failure or leaves them in a state safely handled by the component's cleanup contract.

The implementation must not mark initialization as complete before all required invariants have been established.

#### Transactional Initialization

Where practical, initialization **SHOULD** provide an all-or-nothing observable result.

If initialization fails, the subsystem should either:

- Restore its previous valid state.
- Return to a documented uninitialized state.
- Enter an explicit recoverable failure state when restoration is impossible or inappropriate.

Partial initialization must not leave resources permanently owned without a valid cleanup path.

**Guiding principle:**

> Initialization establishes a usable state; shutdown dismantles it. Both operations must preserve ownership, invariants, and well-defined recovery behavior.

### 6.11 Asynchronous Operations and Cancellation

Asynchronous execution introduces lifetimes that may extend beyond the scope of the function that schedules the work.

Every asynchronous operation **MUST** establish ownership and cancellation rules for the data and resources it accesses.

#### General Requirements

- Captured objects **MUST** remain valid until asynchronous work completes or releases them.
- Cancellation **MUST NOT** be treated as synchronous completion unless the API explicitly guarantees it.
- Owners **MUST** coordinate outstanding tasks before destroying resources those tasks may access.
- Callbacks **MUST NOT** retain references to destroyed objects or unloaded plugin code.
- Task completion and resource release **MUST** obey the required synchronization and memory-visibility rules.
- Do not introduce shared ownership merely because an operation is asynchronous when scoped completion or explicit lifetime coordination provides a simpler solution.
- Asynchronous APIs **SHOULD** make ownership transfer and cancellation responsibilities visible.
- Shutdown paths **MUST** define how pending, executing, and completed tasks are handled.

#### Cancellation Versus Completion

Cancellation commonly represents a request that work should stop or avoid starting.

It does not necessarily guarantee that currently executing work has finished.

A component that requests cancellation and then immediately destroys task dependencies may therefore introduce use-after-free behavior.

Safe destruction may require waiting for task completion, transferring ownership of required data, or deferring reclamation.

**Guiding principle:**

> Scheduling transfers work into the future, not automatically the ownership of everything that work accesses. Asynchronous lifetime guarantees must be explicit.

### 6.12 Resource Lifetime Verification

Lifetime correctness **SHOULD** be validated through tests and development diagnostics, particularly for resource managers, concurrent registries, asynchronous execution, and device-scoped resources.

#### General Requirements

- Test initialization failure and partial cleanup paths where recoverable failures are possible.
- Verify that repeated cleanup does not violate the documented lifecycle contract.
- Test stale-handle rejection when generation-checked handles are used.
- Exercise container growth, removal, and relocation when references to stored elements are retained.
- Validate concurrent lookup and destruction when the subsystem permits those operations.
- Test subsystem shutdown with outstanding work when asynchronous execution is supported.
- Verify that plugin shutdown removes callbacks and registrations before unloading.
- Exercise GPU resource retirement under representative frames-in-flight and queue synchronization conditions.
- Use assertions and diagnostic instrumentation to detect violated ownership and lifetime contracts during development.
- Do not rely exclusively on assertions to prevent invalid resource access in production execution.

Relevant tests **SHOULD** focus on the actual lifetime guarantees of the component rather than merely confirming that normal construction and destruction succeed.

**Guiding principle:**

> Ownership and lifetime guarantees are correctness properties. They should be explicit in interfaces, enforced by implementations, and verified under failure, concurrency, and shutdown conditions.

## 7. Functions, parameters, and return values

### 7.1 Functions should do one coherent job

Use separate functions for meaningful steps, particularly when phases have different failure and synchronization semantics.

```cpp
xiiResult BuildGraph();
xiiResult CompileGraph();
xiiResult ExecuteGraph();
```

Avoid extracting trivial one-line wrappers that obscure control flow or make a user jump across many files without adding a useful boundary.

### 7.2 Prefer explicit inputs

Pass information at the point of use rather than reading hidden mutable globals. A graph build delegate that depends on a view should receive a view or explicitly documented equivalent context.

```cpp
using BuildDelegate = xiiDelegate<void(
  xiiRenderGraph&,
  xiiRenderGraphBlackboard&,
  const xiiView*)>;
```

### 7.3 Parameter passing

| Kind | Recommended form | Why |
|---|---|---|
| Small scalar or handle | By value | Simple, clear ownership |
| Small trivially copied math type | By value when appropriate | Avoid unnecessary indirection |
| Large read-only object | `const T&` | No copy, cannot mutate |
| Optional borrowed object | `T*` or `const T*` | Null can mean absent |
| Required borrowed object | `T&` or `const T&` | Non-null contract |
| String input | `xiiStringView` when appropriate | Supports multiple callers without ownership transfer |
| Collection input | XII non-owning array view where available | Avoid copies |
| Output parameter | Clearly named reference/pointer | Explicit output contract |
| Ownership transfer | Movable owning type | Shows lifetime transfer |

```cpp
xiiResult GetFrameCompletion(
  xiiUInt64 uiFrameIndex,
  xiiUInt64& out_uiCompletedFrame);
```

### 7.4 Output names

Use `out_` for outputs, and `inout_` when a parameter is intentionally read and modified. This is a **recommended** documentation convention, grounded in current XII usage, not a newly claimed original naming requirement.

### 7.5 `const` correctness

Mark read-only methods `const`, pass references as `const` when no mutation is needed, and use pointers to const data when callers must not modify the pointee.

```cpp
[[nodiscard]] xiiUInt64 GetFrameIndex() const { return m_uiFrameIndex; }
```

A `const` method is not automatically thread-safe. If it accesses changing shared state, synchronization is still required.

### 7.6 `[[nodiscard]]`

Mark return values that signal failure, ownership, or an important computed result as `[[nodiscard]]`. Avoid ignoring such values accidentally.

```cpp
[[nodiscard]] static xiiResult ExecuteGraph(...);
[[nodiscard]] static xiiRenderGraphHandle CreateGraph(...);
```

Do not mark every trivial getter `[[nodiscard]]` mechanically unless it aids correctness and matches neighboring interfaces.

### 7.7 Result handling

Check results immediately when failure affects subsequent work. Follow XII's established result macros and methods.

```cpp
XII_SUCCEED_OR_RETURN(frame.Begin(3U));

if (graph.Compile(settings, &sError).Failed())
{
  xiiLog::Error("Render graph compilation failed: {0}", sError);
  return XII_FAILURE;
}
```

Use `.IgnoreResult()` only when ignoring failure is intentional and documented by the contract; do not use it to silence genuine errors.

### 7.8 Default arguments

Use defaults for common, safe behavior. Avoid boolean arguments with unclear meaning at call sites; prefer an options record when there are multiple independent choices.

```cpp
// Readable default:
[[nodiscard]] static xiiResult BeginFrame(xiiUInt32 uiFramesInFlight = 3U);

// Hard to read:
// CreateGraph("Main", true, false, true, 2);
```

### 7.9 Avoid overly broad overloads

Overload only when the operations have closely related semantics. Two functions with the same name should not unexpectedly own different lifetimes or perform radically different work. Distinguish `BeginFrame()` from `ExecuteGraph()`; do not hide an implicit new GPU frame behind an operation that claims to execute an existing one.

### 7.10 `noexcept` and exceptions

Use `noexcept` only when the operation truly cannot propagate exceptions under the configured engine/toolchain environment. Do not promise `noexcept` merely because the current build disables exceptions. Engine-facing code should generally use explicit result and error mechanisms consistent with XII; code at third-party exception boundaries must translate failures safely.

### 7.11 Return references and pointers carefully

Never return references to local temporaries. Returned pointers/references into manager-owned storage need a lifetime contract. Consider copying a small value or providing a scoped callback instead of returning long-lived mutable access.

### 7.12 Inline and force-inline

Use ordinary `inline` for header-defined non-template functions when necessary. Use `XII_ALWAYS_INLINE` sparingly and consistently with performance-sensitive established code; it is a compiler hint/contract, not a substitute for measurement. Excessive force-inlining increases compile time and instruction-cache pressure.

---

## 8. Control flow and expressions

### 8.1 Prefer early exits

Guard invalid inputs and failure states up front. This keeps the main successful path less nested.

```cpp
xiiResult Execute(xiiGALDevice* pDevice)
{
  if (pDevice == nullptr)
    return XII_FAILURE;

  if (!m_bInitialized)
    return XII_FAILURE;

  SubmitCommands(pDevice);
  return XII_SUCCESS;
}
```

### 8.2 Use the simplest appropriate branching construct

- `if` for conditional logic.
- `switch` for clear discrete alternatives, such as enum states.
- Compile-time `if constexpr` for template specialization.
- Avoid long chains of type tests in place of a designed extension interface.

```cpp
switch (queue.GetValue())
{
  case xiiRenderGraphQueue::Graphics:
    return graphicsQueue;
  case xiiRenderGraphQueue::Compute:
    return computeQueue;
  default:
    return nullptr;
}
```

When enum values are intended to be exhaustive, consider how adding a value will be detected during review or compiler diagnostics. A silent default can conceal missing cases.

### 8.3 Loops

Use range-based loops when you need elements, index loops when the index is meaningful, and explicit iterators when removal or container behavior requires them.

```cpp
for (const auto& entry : m_Entries)
{
  ProcessEntry(entry);
}
```

Do not erase elements during range iteration unless the container and iteration pattern explicitly support it. Decide whether removal may reorder elements (`RemoveAndSwap`) and whether order is a contract.

### 8.4 Comparators must be valid

Sort comparators must define strict weak ordering. Do not return `<=` or `>=` as a comparator.

```cpp
m_Entries.Sort([](const auto& a, const auto& b) {
  return a->m_iPriority > b->m_iPriority;
});
```

The above orders priorities descending but does **not** guarantee ordering among ties if `Sort()` is unstable. Preserve stable tie order intentionally if the scheduling contract requires it.

### 8.5 Boolean logic

Make operator precedence obvious. Factor complex conditions into named predicates.

```cpp
const bool bValidFrame = frame.IsActive() && frame.GetFrameIndex() == uiExpectedFrame;
const bool bCanExecute = bValidFrame && pDevice != nullptr && pResourceCache != nullptr;

if (!bCanExecute)
  return XII_FAILURE;
```

### 8.6 Avoid hidden mutations

Avoid complicated expressions that modify the same value multiple times, obscure evaluation order, or mix resource creation with validation. Split side effects into statements.

### 8.7 Integer conversions and arithmetic

- Check overflow before multiplying buffer dimensions, strides, or counts.
- Use the signedness and width the domain requires.
- Use explicit casts when conversion is intentional, and verify range for narrowing conversions.
- Avoid converting potentially negative signed values to unsigned silently.

```cpp
if (uiElementCount > xiiMath::MaxValue<xiiUInt64>() / uiStride)
  return XII_FAILURE;

const xiiUInt64 uiByteCount = xiiUInt64(uiElementCount) * uiStride;
```

The example assumes a strictly positive stride; validate that separately if zero is invalid.

### 8.8 Floating-point code

Do not use exact equality for approximate numeric results unless exact representation is part of the contract. Define tolerances relative to scale and precision. Account for NaNs and infinities when values come from untrusted calculations or external data.

### 8.9 Switch fallthrough

Make intentional fallthrough obvious with `[[fallthrough]]` where supported and compatible with the surrounding code. Do not rely on a comment alone if the compiler supports the attribute.

---

## 9. Modern C++23 features

### 9.1 Use modern C++ when it improves clarity

C++23 is the language baseline, but a feature is useful only when supported by XII's actual platform toolchains and when its semantics improve the code. Do not introduce modern syntax merely to demonstrate novelty.

### 9.2 `auto`

Use `auto` when the type is explicit from the initializer, verbose without adding value, or intentionally generic. Prefer spelling the type when it communicates a resource boundary, precision, ownership, or conversion.

```cpp
const auto uiFrameIndex = frame.GetFrameIndex();

xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
```

Avoid opaque use of `auto` in public declarations when a precise type is part of the interface.

### 9.3 `constexpr`, `consteval`, `constinit`

Use `constexpr` for compile-time constants and functions that legitimately support constant evaluation. Use `consteval` only when runtime use is invalid by design. Use `constinit` for static initialization-order guarantees where supported and appropriate. Do not add compile-time machinery when simple constants suffice.

```cpp
static constexpr xiiUInt32 MaxFramesInFlight = 4U;
```

### 9.4 Scoped enums and engine enum wrappers

Prefer strongly typed enum representations. XII has established reflective enum wrapper patterns; follow the existing reflection requirements where serialization/tooling needs them.

```cpp
struct XII_GRAPHICSCORE_DLL xiiRenderGraphQueue
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Graphics = 0U,
    Compute,
    Transfer,

    ENUM_COUNT,

    Default = Graphics
  };
};
```

**Illustrative:** this is an example of the enum *format*, not a recommendation to add this particular type. Do not add a closed enum for an extensible set such as user-provided render-data categories without considering registration-based alternatives. Do not assume numeric enum order is a dependency order.

### 9.5 Concepts and constraints

Use concepts or `requires` to express meaningful template requirements when supported and when they clarify diagnostics. Avoid overengineering a concept hierarchy for one trivial call site.

```cpp
// Illustrative standard C++ example.
template <typename T>
concept UnsignedIntegral = std::is_integral_v<T> && std::is_unsigned_v<T>;
```

If this code is placed in XII source, include the actual standard headers directly and use existing project constraints when more appropriate.

### 9.6 Ranges and views

Use standard ranges when a pipeline is easier to understand than a simple loop and the repository toolchain fully supports the operation. Do not use lazy views when the underlying container may mutate or die before iteration. In hot paths, measure overhead and code generation.

### 9.7 Structured bindings

Use them when the component names are obvious from context; avoid discarding meaningful types or lifetimes inadvertently.

```cpp
for (const auto& [resourceName, resourceHandle] : resources)
{
  ProcessResource(resourceName, resourceHandle);
}
```

This is a standard-library-style illustrative example, not a claim that all XII containers expose pair iterators with this shape.

### 9.8 Lambdas and captures

Prefer explicit captures when callbacks may outlive the current scope. `this` captures require an enforced owner lifetime. Never register callbacks into long-lived systems without a corresponding unregister path.

```cpp
m_pWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& e) {
  if (e.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
    RequestQuit();
});
```

**Lifecycle warning:** remove or invalidate this handler before destroying its receiver if the event source may outlive it.

### 9.9 Move semantics

Move expensive unique owners instead of copying them. Do not call `std::move` on a `const` object expecting a real move, and do not return `std::move(local)` blindly when it may prevent named return-value optimization.

### 9.10 Casts

Use the narrowest explicit C++ cast appropriate to the conversion. `reinterpret_cast` and pointer punning demand strong justification and attention to alignment, aliasing, representation, and lifetime. Prefer engine helpers for checked casts if present.

```cpp
const float fWidth = static_cast<float>(uiWidth);
```

Avoid C-style casts because they conceal the conversion category.

### 9.11 `std` and XII abstractions

Prefer XII's established types at shared engine boundaries (`xiiString`, `xiiStringView`, `xiiDynamicArray`, `xiiResult`, `xiiDelegate`, `xiiUniquePtr`, etc.). The standard library is not forbidden. Use it when appropriate, but do not create public APIs that gratuitously mix incompatible ownership, allocation, or ABI conventions.

### 9.12 Coroutines and asynchronous code

Use coroutines only with a defined executor, cancellation model, ownership contract, error propagation, and shutdown behavior. Do not keep raw engine pointers across suspension unless their lifetime is guaranteed. A coroutine is not automatically thread-safe.

### 9.13 Modules and headers

Do not migrate headers to C++ modules piecemeal without a toolchain/build-system plan covering generated reflection, macros, DLL boundaries, PCH, and plugin compilation.

---

## 10. Memory, allocators, and containers

### 10.1 Choose storage from access patterns

Use a contiguous array when iteration, indexing, and locality dominate. Use a map/table when lookup is primary. Use a stable-address or handle-based container when moving elements would invalidate persistent references. Avoid combining multiple authoritative containers without defining which one owns the objects and how updates remain consistent.

| Requirement | Typical approach | Important caveat |
|---|---|---|
| Frequent sequential iteration | `xiiDynamicArray<T>` | Growth/removal may move elements |
| Small typical size, occasional spill | Suitable XII hybrid/small-array type | Confirm actual capacity/allocator behavior |
| Handle-based lookup | `xiiIdTable` | A mapping to `T*` does not own `T` |
| Hash lookup by key | XII hash table/map | Choose key hashing and equality deliberately |
| Stable heap address | `xiiUniquePtr<T>` stored in a collection | Pointer valid only until destruction |
| Non-owning contiguous range | XII array-view type | Underlying storage must outlive view |

### 10.2 Allocation on critical paths

Avoid per-element heap allocations in hot loops unless profiling shows they are necessary. Use frame arenas, pooled objects, retained capacity, or batched allocations where lifetime permits. Avoid introducing a custom allocator for an infrequently created object without evidence.

### 10.3 Reserve and capacity

Reserve capacity when expected count is known and the allocation matters. Do not call `Reserve()` redundantly every frame. A container's capacity is an optimization; its count defines initialized elements.

```cpp
m_VisibleObjects.Reserve(uiEstimatedVisibleObjectCount);
```

### 10.4 Pointer invalidation during growth

```cpp
// Unsafe if the table keeps pointers into moving array entries.
xiiDynamicArray<Entry> m_Entries;
Entry* pEntry = &m_Entries[0];
m_Entries.PushBack(anotherEntry); // May relocate storage.
```

Preferred alternatives include stable separate allocation or table entries keyed by generation-checked IDs. Do not assume a container's address stability; check its documented behavior.

### 10.5 Removal semantics

`RemoveAndSwap` is useful when order is irrelevant; it can reorder the collection. Ordered removal can shift elements and be more expensive. Do not optimize away order if it affects scheduling, behavior, serialization, or deterministic tests.

### 10.6 Allocator lifetime

An allocator must outlive all objects and containers using it. A manager-owned `xiiProxyAllocator` must not be destroyed before its graph entries and graph-owned allocations are released. Verify member destruction order and explicit `Clear()` calls during subsystem shutdown.

### 10.7 Cache semantics

Distinguish:

- **Ownership:** who ultimately releases an object.
- **Caching:** which completed/unused objects may be retained for reuse.
- **Pooling:** how compatible physical objects are recycled.
- **Aliasing:** how separate logical resources may share memory with non-overlapping uses.

Do not call an object a *cache* when it actually owns a graph's authoritative state without explaining its lifetime contract.

### 10.8 Memory alignment and layout

Respect alignment requirements for SIMD types, mapped GPU data, atomics, and API structures. Use the engine's alignment helpers and allocation APIs. Do not rely on undocumented packing, padding, or `sizeof` equivalence when transmitting data to a GPU or disk.

### 10.9 Data-oriented access

Prefer iteration over only the data required by a subsystem. An enormous heterogeneous array is simple to append to but can be expensive to filter for every pass. Maintain indices, categories, or sorted batches when a measured workload benefits—without forcing every caller to perform full scans.

### 10.10 Smart-pointer ownership cycles

Avoid ownership cycles, especially across editor modules, callbacks, plugin services, and GPU resources. If two systems need to refer to one another, make one relationship non-owning or use an explicit manager handle.

---

## 11. Strings, identifiers, hashes, and handles

### 11.1 Use the right string representation

| Type | Typical role |
|---|---|
| `xiiString` | Owned, dynamically constructed text |
| `xiiStringBuilder` | Efficient text construction and mutable concatenation |
| `xiiStringView` | Borrowed, non-owning text input |
| `xiiHashedString` | Stable, frequently compared or indexed name |
| `xiiTempHashedString` | Temporary hashed lookup key, when the target API supports it |

Do not intern every debugging string without a reason. Conversely, do not repeatedly hash the same immutable identity in a hot lookup path.

### 11.2 Name identity versus display labels

If names are identifiers, document uniqueness, case sensitivity, hashing, collision handling, and whether the name can change. If names exist only for diagnostics, do not make name equality govern object identity unnecessarily.

```cpp
struct xiiNamedGraph
{
  xiiHashedString m_sName;
  xiiRenderGraphHandle m_hGraph;
};
```

### 11.3 Avoid hidden conversions and lifetime traps

```cpp
// Safe concept: the owner copies or interns the name.
[[nodiscard]] xiiRenderGraphHandle CreateGraph(xiiStringView sName);
```

An API that stores the caller's `xiiStringView` without copying it must document the backing storage's lifetime; otherwise a temporary name can create a dangling reference.

### 11.4 Generation-checked handles

Use an engine handle for objects owned by a manager and referenced by clients. Keep handle types distinct even when they share an underlying ID representation.

```cpp
using xiiRenderGraphId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiRenderGraphHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRenderGraphHandle, xiiRenderGraphId);
};
```

**Illustrative:** verify the appropriate ID widths, wraparound policy, and friend declarations in the actual implementation. An eight-bit generation counter eventually wraps; generation checking reduces stale-handle risk but is not mathematically permanent protection against arbitrary reuse.

### 11.5 Handles cannot reveal arbitrary ownership automatically

If a graph handle must identify an owning context, either encode that relationship in the handle format or record it in a registry entry. Do not assume a global ID inherently contains a context ID.

### 11.6 One authoritative mapping

Prefer a single authoritative registration table and explicit membership/ownership lists. If both a context and a global graph table must be updated, update them atomically under the lifecycle synchronization contract or implement rollback.

### 11.7 String comparison semantics

Know whether the operation is a byte comparison, case-folded comparison, a normalized file path comparison, or hashed identity comparison. A hashed value alone may not be sufficient as a unique identifier unless the implementation checks full identity or otherwise defines collision handling.

### 11.8 Paths and user-facing strings

Use platform/path APIs for path construction and normalization. Never assume slash semantics, case sensitivity, or drive letters across Windows, Linux, and macOS. Prefer a clear separation between paths, virtual file-system identifiers, resource identifiers, and user-visible names.

---

## 12. Errors, assertions, diagnostics, and logging

### 12.1 Choose the correct failure channel

Use explicit results for recoverable operational failures. Use assertions for programmer invariants and impossible internal states. Use logging for diagnostics and user-visible failure context. These are complementary, not interchangeable.

| Situation | Default approach |
|---|---|
| Invalid external configuration | Validate and return failure |
| File or shader compilation error | Return failure plus actionable diagnostic |
| Null pointer that is forbidden by an internal invariant | Assert in development, plus runtime guard when needed |
| GPU device loss | Propagate actual failure, preserve diagnostic context |
| Optional feature unavailable | Graceful fallback or defined failure |
| Non-actionable telemetry event | Trace/profile/log according to verbosity policy |

### 12.2 Assertions do not replace release checks

```cpp
XII_ASSERT_DEV(pDevice != nullptr, "Render graph requires a GAL device.");

if (pDevice == nullptr)
  return XII_FAILURE;
```

Do this where invalid input can reach release builds. Never depend on debug-only assertion side effects.

### 12.3 Log useful context

A diagnostic should say which operation failed, on what object, and with which relevant values. Avoid logging only `"Failed"` or emitting noisy success messages every frame.

```cpp
xiiLog::Error(
  "Failed to compile render graph '{0}' for frame {1}: {2}",
  sGraphName, uiFrameIndex, sError);
```

Use the actual signature of XII's formatting and logging APIs. Do not log secrets, excessive object dumps, or pointers as though they were stable identities.

### 12.4 Do not continue after failure

```cpp
if (graph.Compile(settings, &sError).Failed())
{
  xiiLog::Error("Render graph compilation failed: {0}", sError);
  return XII_FAILURE;
}

return graph.Execute(pDevice, pView, &blackboard, pCache, pProfiler, &sError);
```

### 12.5 Error propagation and rollback

If registration updates several structures and fails partway through, leave the manager in its original valid state. Prefer constructing local temporary state, validating it, then committing changes. When rollback is complex, document the transaction boundary and test failures deliberately.

### 12.6 Optional return values

Use an explicit optional/nullable representation when absence is expected, not an assertion. If there are several distinct failure causes, do not reduce them all to an ambiguous `false` unless the caller only needs success/failure; preserve explanatory context for diagnostics.

### 12.7 Failure from destructors and shutdown

Shutdown paths should clean up as far as safely possible while preserving relevant error information. Avoid throwing through destructors or relying on logging facilities after their subsystem has already shut down.

### 12.8 Diagnostic level and performance

Avoid constructing expensive debug strings on hot paths when diagnostics are disabled. Use conditional logging and structured profiler zones where supported. Logging may itself involve locks and allocations.

### 12.9 Security and malformed input

Treat files, network packets, shader source, serialized objects, plugin data, and application arguments as potentially malformed. Validate sizes, offsets, recursion depth, encodings, indices, integer arithmetic, and allocation sizes before use. Assertions are not an input-validation strategy.

---

## 13. Concurrency and synchronization

### 13.1 Every shared mutable object needs a concurrency model

Choose and document one of:

1. Thread confined.
2. Immutable after publication.
3. Shared with external synchronization.
4. Internally synchronized.
5. Lock-free/atomic with a verified memory-order contract.

Do not call an API *thread-safe* merely because it uses a mutex somewhere.

### 13.2 Lock the complete operation

A locked table lookup followed by unlocked mutation is not protected.

```cpp
// Illustrative scoped approach. The lock must remain held through use.
XII_LOCK(m_Mutex);

Entry* pEntry = nullptr;
if (!m_GraphTable.TryGetValue(hGraph, pEntry))
  return XII_FAILURE;

pEntry->m_bEnabled = false;
```

An implementation must also account for re-entrant callbacks or lock ordering when user delegates are involved. **Do not invoke arbitrary build delegates under a general registry mutex** if they can register, unregister, log through locking subsystems, or acquire other locks.

### 13.3 Define lock ordering

If the render graph manager has both a registry mutex and a GPU frame mutex, document a single acquisition order. Never acquire them in opposite orders on different call paths. Prefer reducing nested locking rather than relying on a complicated order.

Example contract:

```text
Registry operations: registry lock; no GPU execution callbacks.
Frame execution: frame lock; snapshot/pin required graph entries; release registry lock;
                 compile/execute pinned entries; unpin at completion.
```

This is a design pattern, not a claim about XII's current locking implementation.

### 13.4 Active-frame mutation

Registering or destroying graphs while execution is iterating the registration set can invalidate iterators or callback targets. Either disallow mutation during execution or queue mutations for a safe boundary. A deferred approach requires explicit semantics for when requests become visible.

### 13.5 Atomics do not solve object lifetime

Atomic flags and counters are useful for narrowly defined state, but they do not make a destroyed object safe to dereference. Reference counting, ownership, quiescence, or proper scope guards are separate concerns.

### 13.6 Memory ordering

Use the simplest correct atomic ordering. Defaults may be adequate for infrequent control operations; acquire/release or relaxed semantics require proof of which data each operation publishes/observes. Do not optimize memory ordering by intuition.

### 13.7 Re-entrancy

Define whether `BeginFrame()` can nest, whether a delegate may trigger graph execution recursively, and whether callbacks may call registration APIs. Reject unsupported re-entrancy clearly before corrupting state.

### 13.8 Thread affinity

Rendering APIs may require command recording/submission, swap-chain access, or window operations on particular threads. Document and enforce those restrictions. Independent render graph instances do not automatically imply simultaneous GPU submission is supported.

### 13.9 Cancellation and shutdown

Before subsystem shutdown or plugin unload, prevent new submissions, stop callbacks, wait for or cancel CPU work, and retire GPU resources appropriately. Avoid waiting forever while holding a lock needed by the work being joined.

### 13.10 Avoid data races in diagnostics

Counters used for logging, statistics, profiler output, or debugging still need synchronization when updated concurrently. They are not exempt because they are “only diagnostics.”

---

## 14. Performance-sensitive engine code

### 14.1 Identify hot paths

Differentiate frame-critical loops, render graph compilation, shader resource binding, streaming, asset import, startup, and editor tooling. Their optimal tradeoffs differ. A micro-optimization that damages startup diagnostics may not help frame performance.

### 14.2 Algorithmic complexity first

If every rendering pass scans one giant render-data array, the work can grow with both the pass count and the number of objects. Prefer filtered views, buckets, indices, or batches when those fit the actual renderer's data model.

```text
Naive:             passes × all extracted objects
Indexed approach:  extraction/indexing + relevant objects per pass
```

Do not introduce category registries merely because another engine uses them; design the query workload first.

### 14.3 Reduce allocations and unnecessary work

- Avoid repeated temporary string construction, container growth, and descriptor creation in hot paths.
- Retain or cache expensive immutable state when invalidation is correct.
- Avoid recomputing global shader/permutation lookups unnecessarily.
- Move expensive work to a preprocessing/compile phase when inputs are stable.
- Do not rebuild a render graph solely to compensate for a missing lifetime or versioning contract.

### 14.4 Measure real workloads

Measure CPU time, GPU time, allocation counts/bytes, memory bandwidth, pipeline transitions, cache misses, and synchronization stalls where useful. Test debug, development, and optimized builds, noting that instrumentation itself changes performance.

### 14.5 Design for determinism

Deterministic ordering improves debugging, reproducibility, and testability. Define tie-breaking where order matters; don't rely on an unspecified hash-table iteration order for externally visible behavior.

### 14.6 Avoid overspecialized API flags

Do not accumulate `bAsync`, `bBackground`, `bEveryNFrames`, `bExternal`, and `bPresent` flags on one generic rendering operation without clear orthogonal semantics. Use purpose-built abstractions only when the responsibilities are genuinely distinct.

### 14.7 Cache invalidation

Every cache needs explicit answers to:

- What is the key?
- What constitutes compatibility?
- Who owns cached entries?
- When are they invalidated or retired?
- What happens across device reset, hot reload, and frames in flight?

### 14.8 Profiling allocations and command submission

Profile physical GPU allocation and transient reuse separately from logical graph resource creation. A fast CPU allocator does not make GPU memory oversubscription safe. Similarly, fewer CPU command calls do not guarantee better GPU performance if barriers or queue synchronization increase.

### 14.9 SIMD, data layout, and false sharing

Choose AoS versus SoA based on data access, not aesthetic preference. Keep atomic counters and frequently written worker-local data from sharing cache lines where contention matters. Measure the effect on each supported architecture.

### 14.10 Premature optimization versus avoidable waste

Do not overengineer trivial systems, but avoid obvious N² behavior or per-frame heap churn when a simple alternative is available. “Measure first” does not mean ignoring standard algorithmic costs.

---

## 15. Graphics, render graphs, and GPU lifetimes

These rules are particularly important for XII because the engine separates GraphicsCore orchestration from GraphicsFoundation/GAL resource and command APIs and can operate across graphics backends.

### 15.1 Keep the responsibilities distinct

| Layer | Responsibility |
|---|---|
| Render pipeline | Defines high-level rendering stages/nodes and connections |
| Render graph | Tracks logical passes/resources, dependencies, compile-time scheduling |
| Render graph manager | Owns graph instances, exposes selected execution, shares frame services |
| Blackboard | Stores information local to a graph invocation/instance |
| Resource cache/pool | Reuses compatible physical transient GPU allocations safely |
| GAL | Creates resources and records/submits GPU commands |
| GPU completion tracker | Establishes when submitted work has finished sufficiently for safe recycling |

Do not use a category enum as a substitute for real resource dependencies. Do not use graph registration order as proof that two GPUs or queues can safely share a resource.

### 15.2 Declare resource access before execution

A render graph can reason about resource hazards only when passes accurately declare what they read, write, and import. The following is adapted from XII's supplied Procedural Triangle sample:

```cpp
void SetupOffscreenPass(
  OffscreenPassData& data,
  xiiRenderGraphBuilder& builder)
{
  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  textureDescription.m_BindFlags = xiiGALBindFlags::ShaderResource |
                                   xiiGALBindFlags::RenderTarget;

  data.m_hOffScreenTexture = builder.WriteTexture(
    "OffScreenTexture",
    textureDescription,
    xiiGALResourceStateFlags::RenderTarget);
}
```

**Illustrative note:** the creation description above omits the required size; the real sample sets `m_Size` from the window. Include every required descriptor field in production code.

### 15.3 Resource versions and hazards

When writing a graph-managed resource, retain the returned versioned handle and use it for subsequent operations. Never reconstruct internal resource versions from names or indices or assume that a read handle remains the current version after a write.

```cpp
data.m_hOffScreenTexture = builder.ReadTexture(
  "OffScreenTexture", xiiGALResourceStateFlags::RenderTarget);

data.m_hOffScreenTexture = builder.WriteTexture(
  data.m_hOffScreenTexture, xiiGALResourceStateFlags::RenderTarget);
```

The underlying resource may be the same physical allocation while the logical version changes; the graph compiler manages the intended dependency model.

### 15.4 Correct state declarations

Use correct GAL resource-state and access flags. Misdeclaring a write as a read or omitting a resource entirely can create race conditions and synchronization bugs that do not reproduce reliably. Validation must supplement, not replace, precise pass declarations.

### 15.5 External/imported resources

Swap-chain back buffers, externally managed textures, persistent buffers, and histories need an explicit import/ownership contract. In the supplied sample, the presentation pass imports its back buffer:

```cpp
data.m_hBackBufferTexture = builder.ImportTexture(
  "BackBuffer",
  m_pSwapChain->GetBackBufferTexture(),
  xiiGALResourceStateFlags::CopyDestination);
```

Ensure the imported resource remains alive for the entire submitted use and that its entry/exit state and synchronization are correct.

### 15.6 Side effects and pass culling

Passes that write only an externally visible target or perform observable work may need explicit side-effect declaration so the graph compiler does not cull them.

```cpp
builder.SetPassSideEffects(true);
```

Do not mark every pass as having side effects merely to force execution; that defeats legitimate culling and can conceal missing dependencies.

### 15.7 Build, compile, execute

Keep these phases distinct. Graph construction should not perform GPU work that belongs to command execution. Compile should validate and plan dependencies/resources. Execute should record/submit the work according to the compiled plan.

```cpp
pGraph->BeginSetup(uiFrameIndex);
BuildDelegate(*pGraph, blackboard, pView);
pGraph->EndSetup();

XII_SUCCEED_OR_RETURN(pGraph->Compile(settings, out_pError));
return pGraph->Execute(
  pDevice, pView, &blackboard, pResourceCache, pProfiler, out_pError);
```

**Illustrative:** `BuildDelegate` above is a placeholder for a callable; use the actual stored delegate and its exact signature. Frame setup/teardown of the shared cache belongs to the selected frame lifecycle, not to a generic pass callback.

### 15.8 One graph, one logical blackboard

Keep graph-local resource handles and pass-to-pass metadata in that graph's blackboard. Do not share a blackboard across independent graphs merely because they share a physical resource pool. Logical handles from one graph generally do not identify resources in another graph's logical resource table.

```cpp
struct Entry
{
  xiiUniquePtr<xiiRenderGraph>         m_pGraph;
  xiiRenderGraphBlackboard             m_Blackboard;
  xiiRenderGraphManager::BuildDelegate m_BuildDelegate;
};
```

Use explicit imported persistent resources or a separate validated exchange mechanism for cross-graph communication.

### 15.9 Multiple views

Do not execute all registered graphs for every view unless that is an explicit scheduling policy. In an editor, a game view, scene view, and material preview can have different cameras, targets, pipelines, and resolutions. The selection of a graph and its view should be explicit when views are independent.

```cpp
XII_SUCCEED_OR_RETURN(
  xiiRenderGraphManager::ExecuteGraph(
    frame, hEditorGraph, pEditorView));

XII_SUCCEED_OR_RETURN(
  xiiRenderGraphManager::ExecuteGraph(
    frame, hGameGraph, pGameView));
```

This reflects the **proposed manager API** from the refactor discussion; verify its declaration before copying into a particular checkout.

### 15.10 Temporal history belongs to its consumer

TAA history, previous-frame transforms, probe histories, accumulated samples, and persistent simulation state need stable identity and invalidation rules. Two cameras generally must not accidentally share one TAA history. Define when histories reset due to view resize, camera cut, pipeline change, device loss, or resource recreation.

### 15.11 Transient pooling versus memory aliasing

Pool physical textures/buffers at an appropriate device scope. Maintain graph-local logical allocation mappings. Aliasing logical resources with non-overlapping lifetimes is a separate optimization requiring accurate lifetime intervals and GPU hazard control.

### 15.12 Frames in flight and retirement

A graph's `Execute()` returning is not proof of GPU completion. A physical resource cannot be made generally available to unrelated future work until all required GPU uses have completed or an explicitly synchronized aliasing scheme proves the reuse safe.

Conceptual lifecycle:

```text
Acquire compatible physical resource
          ↓
Record/submit GPU work
          ↓
Retire under submission/frame completion value
          ↓
Observe sufficient queue completion
          ↓
Return resource to available pool
```

With multiple queues, use a completion condition that covers all queues that accessed the resource. Do not assume a single graphics-queue fence covers asynchronous compute or copy queues unless the engine's submission contract makes it so.

### 15.13 Resource cache frame scope

If the transient cache is device-wide and frame-scoped, call its `BeginFrame()` and `EndFrame()` once per shared GPU frame, or implement/document safe re-entrant semantics. Do not silently reset it between viewport submissions within the same device frame.

### 15.14 Graph destruction and GPU execution

Before destroying graph-owned persistent GPU state, ensure outstanding GPU uses are retired. Separately protect the CPU-side graph instance from being destroyed during its build, compilation, or execution callback. CPU locking and GPU completion are separate obligations.

### 15.15 Render pass and framebuffer configuration

Use consistent attachment formats, sample counts, load/store actions, layouts/states, and framebuffer extents. Assert or fail when target size is zero or incompatible; don't rely on undefined backend behavior. Keep creation descriptions visibly populated and avoid reusing a stale descriptor after a resize.

### 15.16 Feature gates and portability

Check advertised GAL features before using optional functionality, such as ray tracing or specific queue capabilities. Capability checks must be based on the actual selected device and mode, not solely compile-time platform macros.

```cpp
const bool bRayTracingAvailable =
  pDevice->GetGraphicsDeviceAdapterProperties().m_Features.m_RayTracing ==
  xiiGALDeviceFeatureState::Enabled;
```

The exact feature query may vary by backend/API revision; use the current header as authority.

### 15.17 Render-thread synchronization

Protect CPU graph state and manager registries during concurrent setup and destruction. Avoid holding a global registry lock across plugin-provided delegates or GPU submission unless the lock contract explicitly permits it. A mutex around a lookup does not protect a returned pointer after unlocking.

### 15.18 Profiling

Use graph/pass names that remain useful in captures, log messages, and GPU timestamps. A shared profiler must support multiple graph submissions and safe timestamp-query lifetime across frames and queues. Do not assume per-graph scope solely because the profiler pointer is global.

### 15.19 Async compute and queue semantics

A pass's logical purpose is separate from its eligible queue. If a pass prefers a compute queue, the graph compiler must still account for queue capabilities, resource hazards, waits, and potential overlap. A category named `AsyncCompute` is not a substitute for a queue-placement decision.

### 15.20 Presentation and swap-chain sequencing

Acquire the correct back buffer, import it with a valid state, perform writes/copies, submit required work, and present according to the GAL/swap-chain contract. Handle minimize/zero-area/resize/device-loss paths explicitly. Never assume the current back-buffer pointer remains valid after swap-chain recreation.

---

## 16. Reflection, serialization, and public API compatibility

### 16.1 Treat reflected names as contracts

Class names, property names, enum values, serialization fields, and reflected signatures may be persisted in assets or consumed by tools. A rename that compiles locally can still break asset loading or editor workflows.

### 16.2 Follow existing reflection patterns

When a type requires engine reflection, use the appropriate XII declaration and implementation macros established by that type family.

```cpp
XII_DECLARE_REFLECTABLE_TYPE(
  XII_GRAPHICSCORE_DLL,
  xiiRenderGraphQueue);
```

**Illustrative:** the macro only declares the reflective type relationship; a complete implementation may require additional registration code elsewhere. Do not attach reflection to a type that never needs runtime inspection or serialization without a reason.

### 16.3 Serializable evolution

Document schema versions, default values, optional fields, enum migrations, and backwards compatibility. Add migration steps when assets saved by previous versions must continue to load. Test older data rather than assuming reflection will upgrade it automatically.

### 16.4 Binary layout and ABI

Do not assume C++ object layout is a stable disk/network/plugin format. Avoid serializing raw pointers, smart pointers, vtables, padding bytes, or native endian structures directly. Use explicit formats and checked conversion. Examine compiler/toolchain compatibility for exported C++ class layout across DLLs.

### 16.5 Public API changes

Before changing a public signature or handle type, check:

- Call sites across all modules, tools, and samples.
- Serialization/reflection or scripting surfaces.
- Plugin ABI and module export requirements.
- Tests, examples, docs, and error-handling conventions.
- Threading and ownership contracts.

### 16.6 Deprecated APIs

Deprecate when a staged migration is needed; remove directly when the project explicitly accepts a breaking change. Do not keep dead overloads indefinitely solely to avoid updating internal call sites.

### 16.7 Cross-DLL ownership

Allocate and destroy objects through compatible allocator/module boundaries. Do not assume a plugin can call arbitrary `delete` on engine-managed storage. Prefer manager destroy APIs or owner-side lifecycle operations.

---

## 17. Platform, build, plugins, and subsystem lifecycle

### 17.1 Target portability

XII targets Windows, Linux, and macOS where supported. Avoid hardcoding path separators, OS handle types, window-system assumptions, compiler-specific pragmas, graphics SDK types, or shell commands in portable libraries.

### 17.2 Conditional compilation

Prefer central engine feature/configuration macros rather than scattering raw platform/compiler checks when suitable macros exist.

```cpp
#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
  // Optional directory watcher integration.
#endif
```

Do not assume all feature macros are defined in every build. Compile relevant enabled and disabled branches.

### 17.3 Public platform abstractions

Keep graphics API details in backend layers and GAL interfaces. Avoid exposing Vulkan-specific or D3D12-specific types from a portable GraphicsCore public API unless the abstraction is intentionally backend-specific.

### 17.4 Subsystem startup dependencies

Establish explicit startup ordering for systems that depend on allocators, devices, shader managers, resource managers, or render-pass caches. Pair startup and shutdown in reverse dependency order.

```cpp
// Illustrative fragment based on XII subsystem usage.
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderGraphManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderPassCache",
    "PipelineCache"
  END_SUBSYSTEM_DEPENDENCIES

XII_END_SUBSYSTEM_DECLARATION;
```

Do not infer this is a complete subsystem declaration. The real manager defines startup/shutdown hooks as well.

### 17.5 Avoid static initialization-order hazards

Do not construct allocator- or device-dependent global objects before their subsystems start. Prefer a lightweight static façade with state created at the appropriate startup phase when consistent with existing engine architecture.

### 17.6 Device availability

Validate device availability before constructing GPU-dependent services. Ensure the graphics device remains alive until graph resources, GPU resource caches, profiler allocations, and queues have been retired and destroyed.

### 17.7 Plugin lifetime

Unregister callbacks, graph delegates, event handlers, and reflected resources owned by a plugin before unloading it. Otherwise the manager can retain callable state whose code no longer exists.

### 17.8 Static-link registration

Where XII relies on static-link macros, preserve the correct translation-unit marker:

```cpp
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderGraphManager);
```

Use the actual module and file names; do not copy a marker verbatim into an unrelated file.

### 17.9 Build targets and dependencies

Keep header/library dependencies as narrow as possible. Do not link a high-level engine module into a lower-level foundation module merely to use one helper. Avoid circular dependencies in build declarations even if a particular linker happens to accept them.

### 17.10 Backend validation

Run validation layers/debug facilities where available, particularly for GPU state, resource lifetime, synchronization, and shader binding changes. Confirm that a backend-specific optimization preserves portable semantics.

### 17.11 Tools and samples are production examples

Samples should use the current public API rather than bypassing the manager's lifecycle guarantees. A sample that manually resets a shared resource cache in the wrong scope teaches users an unsafe pattern, even if the sample visually works.

---

## 18. Comments and documentation

### 18.1 Document reasons, invariants, and contracts

Comments should explain *why*, *what must remain true*, and *what callers must know*. Avoid comments that simply restate the immediately following code.

```cpp
// A transient resource may still be in use by the GPU after Execute returns.
// Retire it only after the corresponding completion point becomes visible.
RetireResource(hTexture, uiSubmissionValue);
```

**Avoid:**

```cpp
// Increment the frame index.
++m_uiFrameIndex;
```

### 18.2 Public API documentation

Document ownership, invalidation, thread safety, re-entrancy, failure cases, units, and behavior of optional parameters. This is particularly important for frame scopes, graph handles, resource-pool methods, and callback APIs.

```cpp
/// Executes the selected render graph in an active GPU frame.
/// The manager owns the graph and its blackboard.
/// The graph cannot be destroyed or executed concurrently while this call runs.
/// Returns failure for an invalid handle, inactive frame, or graph compile/execute error.
[[nodiscard]] static xiiResult ExecuteGraph(
  const xiiRenderGraphFrame& frame,
  xiiRenderGraphHandle hGraph,
  const xiiView* pView);
```

The documentation above describes a **recommended contract**; enforce it in the implementation before publishing that contract.

### 18.3 `TODO`, `FIXME`, and temporary limitations

Write actionable notes with enough context to find the issue and understand the risk. Prefer linking a tracked issue if the project uses one. Remove stale TODOs that no longer describe current behavior.

```cpp
// TODO: Permit per-queue retirement once the completion tracker exposes
// all queue timelines independently.
```

### 18.4 Comment accuracy

Update comments whenever code behavior changes. This is especially important after removing category/frequency scheduling or changing `ExecuteFrame` to selected `ExecuteGraph` semantics. Incorrect documentation is worse than a missing trivial comment.

### 18.5 Avoid redundant diagrams in code

Use Markdown architecture documents for substantial diagrams and design rationale. Keep critical invariants adjacent to implementation so they are not separated from the code that must maintain them.

### 18.6 Documentation examples are maintained code

Code snippets in documentation must be internally consistent, clearly marked if illustrative, and updated when public API names change. Whenever practical, compile reference examples in sample/test targets or validate them in CI.

---

## 19. Testing, tooling, and code review

### 19.1 What quality gates already exist

The prior XII standards identify these project-specific practices:

- Pull-request checks validate changed-line formatting and copyright without rewriting contributors' branches.
- Native builds retain the repository's current warning policies.
- Extended MSVC analysis and CodeQL provide additional analysis.
- A custom Clang-Tidy naming plugin is a **specialist local tool**, not part of stock LLVM and not a required hosted check.
- `Documentation/Building.md#quality-checks` is the source for current commands and CI details.

Do **not** describe this specialist plugin as automatically installed with stock `clang-tidy`, and do not invent an exact mandatory formatter version or warning flag list without checking the active repository configuration.

### 19.2 Build and compile coverage

A change should compile on all relevant targets and configurations. Check the project's active CI matrix rather than assuming the matrix in a previous release still applies. In particular, validate both enabled and disabled platform/feature paths that the change touches.

### 19.3 Unit tests

Tests should cover success, failure, boundaries, and invariants. Keep tests deterministic and independent of timing where possible. Prefer descriptive names that explain behavior.

```cpp
// Illustrative test concerns for a graph manager:
// - Creating a graph returns a valid handle.
// - Destroying a graph invalidates lookup.
// - Destroying one graph does not affect another.
// - Invalid handles fail gracefully.
// - An inactive frame rejects execution.
```

### 19.4 Integration tests

Test interaction across real subsystems, not only isolated methods. Render graph manager changes should exercise device startup/shutdown, resource-cache frame setup, compiled pass execution, and graph destruction after submission.

### 19.5 GPU tests

Cover resize, swap-chain recreation, multiple views, frames in flight, transient reuse, async queues where supported, feature gating, imported resources, and device-loss behavior. Use backend validation tools as part of test runs where feasible.

### 19.6 Concurrency tests

Where concurrent use is supported, test graph execution against registration/unregistration, plugin shutdown, callback removal, and resource retirement. Instrumented tests and sanitizers are valuable when the toolchain permits them, but their absence does not make a race safe.

### 19.7 Property/invariant tests

Generation-checked handles should reject stale IDs under ordinary reuse. Registries should maintain their counts and ownership invariants through insertion, removal, failed registration, and shutdown. Sorting should respect comparator requirements and documented tie behavior.

### 19.8 Performance regression tests

Measure changes on representative workloads and hardware. Record baseline, methodology, compiler configuration, warmup, and variance. Keep performance comparisons focused; don't claim a speedup from a single cold-frame observation.

### 19.9 Documentation and examples

When changing an exported API, update samples and usage docs in the same change whenever possible. Ensure examples use the actual new signature rather than a deprecated overload.

### 19.10 Review for design, not just style

A cleanly formatted function can still have unsafe ownership, wrong synchronization, invalid GPU barriers, inefficient queries, or breaking serialization changes. Review API contracts and observable behavior before debating minor spelling or whitespace.

### 19.11 Suggested local validation sequence

1. Format only edited lines/files as permitted by the repository workflow.
2. Run compilation for the affected module and immediate dependents.
3. Run targeted unit/integration tests.
4. Exercise relevant graphics backends with validation, when applicable.
5. Inspect static analysis diagnostics without suppressing them blindly.
6. Verify ABI, reflection, serialized data, examples, and plugin compatibility where affected.
7. Run broader CI suites before merging according to project policy.

---

## 20. Common mistakes and corrected examples

This section is intended as a quick reference during implementation and review. The corrected examples show the principle; verify exact API availability against the current XII checkout.

### 20.1 Using a context handle when an individual graph is required

**Avoid:**

```cpp
// Ambiguous once a context contains several graphs.
UnregisterGraph(hContext);
```

**Prefer:**

```cpp
DestroyGraph(hGraph);
```

A handle should identify exactly the kind of resource the operation affects.

### 20.2 Global graph execution for unrelated views

**Avoid:**

```cpp
// Runs every registered graph against a single view, unless selection is built in.
ExecuteFrame(frame, pMaterialPreviewView);
```

**Prefer:**

```cpp
XII_SUCCEED_OR_RETURN(
  xiiRenderGraphManager::ExecuteGraph(
    frame, hMaterialPreviewGraph, pMaterialPreviewView));
```

The selection must be deliberate. A different design may group graphs by view, but should make that policy explicit.

### 20.3 Sorting enum categories as though they were dependencies

**Avoid:**

```cpp
// Do not rely on category order to satisfy depth/Hi-Z/occlusion dependencies.
passes.SortByCategory();
```

**Prefer:** express actual reads/writes and explicit graph dependencies; let compilation produce a valid schedule. Sorting independent ready passes is a separate, optional optimization.

### 20.4 Using registration priority for correctness

**Avoid:** relying on `iPriority = 100` to guarantee that Graph A's GPU writes are visible to Graph B.

**Prefer:** explicit resource import/export, ordering, transitions, and submission/queue synchronization. Priority can order CPU invocation without establishing every GPU hazard.

### 20.5 Dividing by an unchecked interval

**Avoid:**

```cpp
if ((uiFrameIndex % uiEveryNFrames) == 0U)
  Update();
```

**Prefer:**

```cpp
if (uiEveryNFrames == 0U)
  return XII_FAILURE;

if ((uiFrameIndex % uiEveryNFrames) == 0U)
  Update();
```

The better architectural question is whether fixed-frequency updates belong in the owning subsystem at all.

### 20.6 Assuming an ID table owns its pointer values

**Avoid:**

```cpp
xiiIdTable<GraphId, Entry*, xiiLocalAllocatorWrapper> m_GraphTable;
// No collection or allocator path actually owns Entry allocations.
```

**Prefer:** an explicit owning collection or a documented owning allocation strategy, with `m_GraphTable` providing non-owning lookup.

### 20.7 Returning unprotected pointers

**Avoid:**

```cpp
Entry* Find(xiiRenderGraphHandle hGraph)
{
  XII_LOCK(m_Mutex);
  Entry* pEntry = nullptr;
  m_GraphTable.TryGetValue(hGraph, pEntry);
  return pEntry;
}
```

**Prefer:** hold the appropriate lifetime guard through the operation or expose a narrow manager method. Locking only the lookup does not protect the object after return.

### 20.8 Allocating a new object per frame without necessity

**Avoid:** allocating and destroying a reusable scratch container every frame.

**Prefer:** retain storage and clear frame-local contents when appropriate. Check correctness when capacity reuse spans threads or devices.

### 20.9 Copying an owner unintentionally

**Avoid:** passing a unique pointer or heavy owned data by value to an observer.

**Prefer:** pass `const T&`, `T&`, or a borrowing pointer when ownership does not transfer.

### 20.10 Dereferencing before validating

**Avoid:**

```cpp
return s_pState->GetContext(hContext)->m_pGraph.Borrow();
```

**Prefer:** resolve, validate, protect lifetime, then use. Assertions can explain an invariant failure but must not be the only runtime safeguard when invalid input is possible.

### 20.11 Incorrect member prefixes

**Avoid:**

```cpp
class xiiTimer
{
private:
  xiiUInt32 m_iElapsedFrames;  // Unsigned type, signed prefix.
  bool      m_uiEnabled;       // Boolean type, unsigned prefix.
};
```

**Prefer:**

```cpp
class xiiTimer
{
private:
  xiiUInt32 m_uiElapsedFrames = 0U;
  bool      m_bEnabled        = false;
};
```

### 20.12 Confusing frame index with completion

**Avoid:** treating `uiCurrentFrame - 3` as guaranteed GPU completion simply because three frames are configured in flight.

**Prefer:** query the engine's completion tracker/fences and enforce the configured ring-buffer reuse wait contract.

### 20.13 Passing data that outlives its owner

**Avoid:** registering a delegate capturing a renderer object that may be destroyed while the registration stays active.

**Prefer:** destroy the registration before the captured owner, or use an explicit invalidation/lifetime mechanism.

### 20.14 Resetting a shared cache between view submissions

**Avoid:** calling a frame-scoped shared pool's `EndFrame()` after the first viewport when the same device frame has additional GPU submissions that depend on its allocations.

**Prefer:** give the cache one consistent device-frame boundary, or make nested/multiple scopes explicit and safe.

### 20.15 Overusing global state

**Avoid:** reaching into a manager singleton from the constructor of every internal helper just to obtain an allocator.

**Prefer:** inject the allocator or required service explicitly:

```cpp
explicit EntryStorage(xiiAllocator* pAllocator) :
  m_Entries(pAllocator)
{
}
```

### 20.16 Hiding important failure results

**Avoid:**

```cpp
pRenderGraph->Compile(settings).IgnoreResult();
```

**Prefer:** check the result, report the error, and stop before executing invalid state.

### 20.17 Inconsistent resource creation descriptors

**Avoid:** creating a texture without validating target size, format support, and binding flags.

**Prefer:** validate descriptors before physical creation and declare precise graph access flags in each pass.

### 20.18 Comments that no longer reflect behavior

**Avoid:** documentation claiming graph frequency is scheduled globally after the frequency feature has been removed.

**Prefer:** update documentation, samples, tests, and diagnostics in the same API migration.

### 20.19 Fixed-width assumptions in serialization

**Avoid:** serializing `size_t`, native pointer values, or padded C++ structs as a portable file format.

**Prefer:** explicit serialized widths, endianness, versions, and validation.

### 20.20 Premature generic abstractions

**Avoid:** creating an enum, registry, scheduler, and extension layer for a capability not yet used.

**Prefer:** define a narrow, testable operation, measure real limitations, and generalize when multiple use cases justify it.

---

## 21. Reference examples

These examples demonstrate preferred organization and style. They are **reference patterns**, not promises that each illustrative snippet compiles without surrounding declarations in the current engine checkout.

### 21.1 A small exported interface

```cpp
/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiRenderGraphFrame;
class xiiView;

using xiiRenderGraphId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiRenderGraphHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRenderGraphHandle, xiiRenderGraphId);

  friend class xiiRenderGraphManager;
};

/// Owns registered render graphs and coordinates their execution.
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphManager);

public:
  using BuildDelegate = xiiDelegate<void(
    xiiRenderGraph&,
    xiiRenderGraphBlackboard&,
    const xiiView*)>;

  xiiRenderGraphManager() = delete;

  [[nodiscard]] static xiiRenderGraphHandle CreateGraph(
    xiiStringView sName,
    BuildDelegate buildDelegate);

  [[nodiscard]] static xiiResult DestroyGraph(
    xiiRenderGraphHandle hGraph);

  // Further declarations belong here.
};
```

**Header-completeness note:** depending on XII's exact macro expansion and declarations, this example also needs the headers that define `xiiStringView`, `xiiResult`, and `XII_DISALLOW_COPY_AND_ASSIGN`. In production, include every direct dependency rather than relying on the shown transitive includes. The copyright line is illustrative of a file where that attribution is appropriate; do not copy it mechanically to a different owner's file.

### 21.2 Registration with stable ownership

```cpp
// Illustrative storage: the unique-pointer array owns entries;
// the ID table only indexes them.
struct Entry
{
  xiiUniquePtr<xiiRenderGraph>         m_pGraph;
  xiiRenderGraphBlackboard             m_Blackboard;
  xiiRenderGraphManager::BuildDelegate m_BuildDelegate;
};

using GraphTable = xiiIdTable<
  xiiRenderGraphHandle::IdType,
  Entry*,
  xiiLocalAllocatorWrapper>;

GraphTable m_GraphTable;
xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;
```

All insertion, removal, and pointer access must honor the lifecycle/synchronization contract. Never assume the `GraphTable` owns the entries.

### 21.3 A short failure-safe scope

```cpp
xiiResult RenderScene(
  xiiRenderGraphHandle hGraph,
  const xiiView* pView)
{
  xiiRenderGraphFrame frame;
  XII_SUCCEED_OR_RETURN(frame.Begin(3U));

  xiiRenderGraphCompileSettings settings;
  settings.m_bEnablePassCulling  = true;
  settings.m_bEnableCompileCache = true;

  xiiStringBuilder sError;
  if (xiiRenderGraphManager::ExecuteGraph(
        frame, hGraph, pView, settings, &sError).Failed())
  {
    xiiLog::Error("Failed to execute render graph: {0}", sError);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
} // frame destructor ends the active frame on every return path.
```

This assumes the proposed refactored manager API and that `xiiRenderGraphFrame` has the RAII semantics shown in the supplied manager header. A real application's GAL begin/end and presentation boundaries must still follow the active backend's contract.

### 21.4 A render-graph pass setup and execution pair

```cpp
struct BlitPassData
{
  xiiRenderGraphTextureHandle m_hBackBufferTexture;
  xiiRenderGraphTextureHandle m_hOffScreenTexture;
};

void SetupBlitPass(BlitPassData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hOffScreenTexture = builder.ReadTexture(
    "OffScreenTexture", xiiGALResourceStateFlags::CopySource);

  data.m_hBackBufferTexture = builder.ImportTexture(
    "BackBuffer", m_pSwapChain->GetBackBufferTexture(),
    xiiGALResourceStateFlags::CopyDestination);

  builder.SetPassSideEffects(true);
}

void ExecuteBlitPass(const BlitPassData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("Blit to Back Buffer");
  {
    cmd.CopyTexture(
      context.GetTexture(data.m_hOffScreenTexture),
      context.GetTexture(data.m_hBackBufferTexture));
  }
  cmd.EndDebugGroup();
}
```

This example is adapted from the supplied Procedural Triangle sample. In production, the swap chain must exist, the back buffer must be current, and the imported states must match actual GPU usage.

### 21.5 A manager-owned subsystem lifecycle

```cpp
// The public manager is a stateless facade.
xiiUniquePtr<xiiRenderGraphManagerState> xiiRenderGraphManager::s_pState;

void xiiRenderGraphManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Render graph manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiRenderGraphManagerState);
}

void xiiRenderGraphManager::Shutdown()
{
  s_pState.Clear();
}
```

For a complete implementation, ensure all render graphs, GPU resources, profiler state, and device-dependent services have been cleaned up in the appropriate higher-level shutdown phase before clearing the manager state.

### 21.6 A clear configuration record

```cpp
struct xiiRenderGraphExecutionSettings
{
  xiiUInt32 m_uiFramesInFlight = 3U;
  bool      m_bEnableProfiling = false;
  bool      m_bEnablePassCulling = true;
};
```

A configuration record is justified when it groups several independently meaningful settings. For a single `iPriority` argument, an extra description type may be unnecessary.

### 21.7 A reviewable numeric helper

```cpp
xiiResult ComputeBufferSize(
  xiiUInt64 uiElementCount,
  xiiUInt64 uiStride,
  xiiUInt64& out_uiByteCount)
{
  out_uiByteCount = 0U;

  if (uiStride == 0U)
    return XII_FAILURE;

  if (uiElementCount > xiiMath::MaxValue<xiiUInt64>() / uiStride)
    return XII_FAILURE;

  out_uiByteCount = uiElementCount * uiStride;
  return XII_SUCCESS;
}
```

This exposes the important constraint and avoids overflow without obscuring the intent.

### 21.8 A compact API migration note

When replacing a graph-context API with explicit graph selection, provide a migration note like:

```text
Before:
  CreateContext(name)
  GetGraph(context)
  BeginSetup/Compile/Execute manually
  DestroyContext(context)

After:
  CreateGraph(name, buildDelegate)
  ExecuteGraph(frame, graphHandle, view)
  DestroyGraph(graphHandle)

Behavior change:
  Only the requested graph executes for the supplied view.
  The manager owns graph, blackboard, and shared resource orchestration.
```

Migration notes should describe observable behavior changes, not only replacement names.

---

## 22. Contributor and reviewer checklists

### 22.1 Before writing code

- [ ] I know which module/layer owns the feature.
- [ ] I can state the object's ownership and lifetime in one sentence.
- [ ] I know whether the code is frame-critical, startup-only, or asynchronous.
- [ ] I understand what existing API/callers/serialized formats will be affected.
- [ ] I have checked neighboring patterns and the relevant root format/build configuration.
- [ ] I am not introducing an abstraction solely for a hypothetical future feature.

### 22.2 Before submitting code

- [ ] Changed code follows `.clang-format` and `.editorconfig`.
- [ ] Engine symbols, private members, and type-dependent prefixes follow XII's naming rules.
- [ ] Public header includes are sufficient and exported API boundaries are appropriate.
- [ ] Initialization leaves objects valid and all failure paths clean up correctly.
- [ ] Ownership of dynamic memory and GPU resources is unambiguous.
- [ ] Pointers, references, string views, array views, iterators, and handles do not outlive storage.
- [ ] Return values and errors are handled, not silently ignored.
- [ ] Assertions are not substitutes for externally reachable runtime validation.
- [ ] Integer arithmetic, conversions, array bounds, and buffer sizes are checked where needed.
- [ ] Thread safety, lock order, callback lifetime, and re-entrancy are documented and tested.
- [ ] GPU state transitions, queue synchronization, and retirement are correct where relevant.
- [ ] Graph-local blackboards and per-view histories are isolated properly.
- [ ] Feature flags and platform/backend conditions compile as intended.
- [ ] Reflection, serialization, and public API compatibility have been reviewed.
- [ ] Comments, documentation, tests, and samples reflect the new behavior.
- [ ] No unrelated mechanical reformatting or stale commented-out code remains.

### 22.3 Reviewer checklist

- [ ] Does the code solve the stated problem without unnecessary layers?
- [ ] Are public names precise enough to convey their responsibilities?
- [ ] Is there one clear authority for each resource and state transition?
- [ ] Can a caller construct an invalid object or call methods in an unsafe order?
- [ ] Are callbacks safe if the owning object or plugin is destroyed?
- [ ] Can concurrent operations invalidate pointers or iterators?
- [ ] Does the implementation distinguish CPU submission from GPU completion?
- [ ] Does it behave correctly with multiple views and frames in flight?
- [ ] Will the change work on the intended platforms and build configurations?
- [ ] Are diagnostics actionable and tests sensitive to likely regressions?
- [ ] Have measurements justified performance-sensitive complexity?

### 22.4 Special checklist: render graph change

- [ ] Every pass accurately declares reads, writes, imported resources, and side effects.
- [ ] All logical handles belong to the correct graph and resource version.
- [ ] Transient allocations and temporal histories have correct ownership.
- [ ] Execution selects the correct graph and view.
- [ ] Shared resources are not reset between unrelated submissions in one active frame.
- [ ] Registration/destruction cannot race active CPU execution.
- [ ] Physical GPU resources are not recycled before required queue completion.
- [ ] GPU profiler data and timestamps survive until readback is safe.
- [ ] The sample/test covers resize or zero-area swap-chain behavior when relevant.

---

## 23. Maintenance and exceptions

### 23.1 How to interpret exceptions

An exception to a **SHOULD** rule may be justified by an established platform constraint, compatibility requirement, performance evidence, or clearer local code. Document unusual choices in the PR or adjacent code where future maintainers need the rationale.

A **MUST** rule should not be bypassed informally. Where the current standards cannot be followed, update the policy or obtain the project's expected design approval, and add tests showing why the deviation is safe.

### 23.2 Avoid silent policy drift

When a new pattern becomes common—such as generation-checked resource handles, new GAL resource states, a different frame-scoping API, or a supported C++23 feature—update this guide with actual code examples and supported toolchain information. Do not present one experimental pattern as a universally required rule.

### 23.3 Authority and provenance

The original `Coding-Standards.md` supplied for this guide establishes **naming rules and tooling notes**; its requirements have been preserved here, including the important exception that type-dependent prefixes are mandatory only for member variables and that public/static/fixed-array fields have specific flexibility.

The additional sections are a **proposed engineering standard** synthesized from the supplied XII engine samples, render graph manager interfaces, and general C++23 systems programming practice. They should be reviewed against the current repository, backend implementations, tests, and CI configuration before being treated as enforceable historical XII policy. In particular, verify the repository's exact `.clang-format`, `.editorconfig`, compiler matrix, allocator contracts, reflection macros, and sanitizer/tooling availability.

### 23.4 Guiding test

When choosing between two designs, ask:

> Can a new contributor understand what this code owns, when it runs, what makes it fail, and which invariants it must preserve—without reading unrelated implementation files?

Prefer the design that gives the clearest reliable answer while meeting the actual performance and compatibility requirements.

---

*End of XII Engine C++ Coding Standards and Engineering Guidelines.*
