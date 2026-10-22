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

## 7. Functions, Parameters, and Return Values

Functions in XII **MUST** express clear responsibilities, communicate their contracts through their interfaces, and preserve predictable behavior across supported platforms and configurations.

Function signatures should make dependencies, ownership, mutability, failure behavior, and observable side effects apparent wherever practical.

This section extends the principles established in §1.1 (Correctness), §1.2 (Ownership and Lifetime), §1.3 (Interface Design), §3 (Naming Conventions), and §5 (Types and Interfaces).

### 7.1 Functions Should Perform One Coherent Operation

A function **SHOULD** perform one coherent operation at a consistent level of abstraction.

This does not require every function to contain only a few statements. A substantial implementation may remain appropriate when its operations collectively fulfill one clearly defined responsibility.

#### General Requirements

- Functions **MUST** have a clearly identifiable purpose.
- Prefer separating operations with distinct failure, ownership, synchronization, or lifecycle semantics.
- Do not combine unrelated responsibilities merely to reduce the number of functions.
- Avoid extracting trivial wrappers that introduce navigation overhead without establishing a meaningful boundary.
- Functions **SHOULD** operate at a consistent level of abstraction.
- Prefer meaningful helper functions when they isolate reusable logic, enforce invariants, or clarify complex algorithms.
- Avoid excessive parameter lists when they indicate that a function performs unrelated operations or depends on too much external state.
- Do not split tightly coupled operations when doing so would expose invalid intermediate states or compromise correctness.
- Function names **MUST** accurately describe the work performed and its observable effects.

**Prefer:**

```cpp
xiiResult LoadDocument(xiiStringView sPath);
xiiResult ValidateDocument();
xiiResult SaveDocument(xiiStringView sPath);
```

Each operation has a distinct responsibility and may encounter different failure conditions.

A higher-level operation may coordinate these steps when appropriate:

```cpp
xiiResult OpenDocument(xiiStringView sPath)
{
  XII_SUCCEED_OR_RETURN(LoadDocument(sPath));
  XII_SUCCEED_OR_RETURN(ValidateDocument());

  return XII_SUCCESS;
}
```

The coordinating function remains cohesive because its responsibility is opening a valid document.

#### Avoid Excessive Fragmentation

Do not extract every expression into a separate function merely to reduce function length.

A helper function should provide at least one meaningful benefit:

- Encapsulation of a distinct responsibility.
- Reuse by multiple consumers.
- Enforcement of an invariant.
- Clarification of a complex algorithm.
- Isolation of platform-specific behavior.
- Separation of independently testable logic.
- Establishment of a meaningful failure or synchronization boundary.

Function length alone **MUST NOT** determine whether an implementation should be divided.

**Guiding principle:**

> A function should perform one coherent operation, but cohesion matters more than line count.

### 7.2 Prefer Explicit Inputs and Dependencies

Functions **SHOULD** receive the information required for their operation through explicit parameters, object state, or clearly documented subsystem dependencies.

Avoid hidden mutable global state when an explicit dependency would improve correctness, reuse, or testability.

#### General Requirements

- Required inputs **SHOULD** be apparent from the function signature or the owning object's documented state.
- Avoid retrieving mutable global state when the caller already possesses the required context.
- Do not introduce hidden dependencies on the active frame, current thread, global device, or singleton unless those dependencies are part of an established subsystem contract.
- Functions **SHOULD NOT** silently depend on unrelated initialization or execution state.
- Prefer passing the narrowest context required by the operation.
- Do not pass entire subsystem objects when only a small, well-defined portion of their capabilities is required, unless doing so is consistent with the architecture.
- Avoid introducing unnecessary parameter forwarding through multiple layers when the dependency naturally belongs to an owning component.
- Execution context **MUST** remain valid throughout the operation.

**Prefer:**

```cpp
xiiResult Simulate(xiiSimulationWorld& world, const xiiSimulationSettings& settings, xiiTime deltaTime);
```

**Avoid:**

```cpp
xiiResult Simulate()
{
  auto* pWorld    = GetGlobalActiveWorld();
  auto* pSettings = GetGlobalSimulationSettings();

  // Hidden dependencies on mutable global state.
}
```

The preferred interface makes its dependencies explicit and allows the operation to be invoked in different simulation contexts.

#### Callback and Delegate Context

Callbacks and delegates **MUST** receive or safely capture the context required for their execution.

For example:

```cpp
using SimulationDelegate = xiiDelegate<void(xiiSimulationWorld&, const xiiSimulationSettings&, xiiTime)>;
```

A delegate **MUST NOT** retain borrowed references beyond their guaranteed lifetime.

When callbacks execute asynchronously or survive subsystem transitions, their captured ownership and executable-code lifetime must follow §6.11.

**Guiding principle:**

> Dependencies should be visible at the boundary where they matter. Hidden state should not determine behavior that callers reasonably expect to control.

### 7.3 Parameter Passing

Parameter types **MUST** accurately express input semantics, mutability, optionality, and ownership.

Choose parameter-passing mechanisms according to the type's size, semantics, lifetime requirements, and intended use.

| Parameter category | Preferred form | Rationale |
|---|---|---|
| Small scalar values | By value | Simple semantics without unnecessary indirection |
| Enum values and handles | By value | Clear identity and inexpensive copying |
| Small mathematical types | By value when appropriate | Avoid unnecessary indirection where copying is efficient |
| Large read-only objects | `const T&` | Avoid copying while prohibiting mutation through the reference |
| Required mutable objects | `T&` | Express non-null borrowed access with permitted mutation |
| Required read-only objects | `const T&` | Express non-null borrowed access without mutation |
| Optional mutable objects | `T*` | Allow absence while permitting modification |
| Optional read-only objects | `const T*` | Allow absence without permitting modification |
| String inputs | `xiiStringView` where appropriate | Accept supported string representations without ownership transfer |
| Collection inputs | `xiiArrayPtr<T>` or `xiiArrayPtr<const T>` as appropriate | Express non-owning contiguous access |
| Output parameters | `T&` or `T*` with documented output semantics | Communicate produced values |
| Input/output parameters | Mutable reference or pointer | Communicate intentional modification of existing state |
| Ownership transfer | Movable owning type | Make transfer of ownership explicit |
| Callable inputs | Appropriate callable or delegate type | Communicate invocation and lifetime requirements |

#### General Requirements

- Prefer pass-by-value for inexpensive value-like types.
- Avoid copying large objects when borrowed access is sufficient.
- Use `const` references for read-only access to objects that are expensive or inappropriate to copy.
- Use mutable references only when mutation is part of the function's contract.
- Prefer references for required objects and pointers for optional objects, unless existing API conventions establish different semantics.
- Do not use non-const references merely to avoid copying when the function does not modify the argument.
- Do not use output parameters when a direct return value is simpler and communicates the result adequately.
- Ownership transfer **MUST** be explicit and consistent with the owning type.
- Non-owning parameter views **MUST NOT** be retained beyond their documented lifetime.
- Parameter types **SHOULD** avoid unnecessarily constraining callers to a particular container implementation.

**Prefer:**

```cpp
xiiResult CalculateBounds(xiiArrayPtr<const xiiVec3> positions, xiiBoundingBox& out_Bounds);
```

This signature communicates that the function reads the positions and writes the resulting bounding box.

#### Pass-by-Value Versus References

Pass-by-value is not automatically faster for every small type.

Calling conventions, alignment, register availability, and the type's copy semantics can affect performance.

For mathematical and SIMD types, follow established XII conventions and measured performance requirements.

Do not mechanically replace `const T&` with `T` based solely on `sizeof(T)`.

**Guiding principle:**

> Parameter types should communicate how data is used, whether it may change, and whether its ownership is transferred.

### 7.4 Output and Input/Output Parameters

Output parameters **SHOULD** use descriptive names that distinguish produced values from ordinary inputs.

XII commonly uses the `out_` and `inout_` naming conventions. These are recommended documentation conventions rather than mandatory type-dependent prefix requirements for all parameters.

#### Naming Conventions

- Use `out_` for parameters whose primary purpose is to receive a produced value.
- Use `inout_` for parameters whose existing value is intentionally read and modified.
- Names **SHOULD** retain applicable type-dependent prefixes after the direction marker.
- Do not use output-direction prefixes for parameters that are merely mutable implementation details.
- Output parameters **MUST** have documented validity guarantees on success and failure.
- Prefer direct return values when the function produces one simple result and does not require independent failure reporting.

**Example:**

```cpp
xiiResult GetTaskCompletion(xiiTaskHandle hTask, xiiUInt64& out_uiCompletionValue);
```

For an input/output parameter:

```cpp
void NormalizeWeights(xiiArrayPtr<float> inout_Weights);
```

#### Failure Semantics

When a function returns `xiiResult` and also writes output parameters, the contract **MUST** establish whether those outputs are modified when the operation fails.

Where practical, prefer leaving output parameters unchanged on failure.

**Prefer:**

```cpp
xiiResult ParseCount(xiiStringView sText, xiiUInt32& out_uiCount)
{
  xiiUInt32 uiParsedCount = 0U;

  XII_SUCCEED_OR_RETURN(ParseUnsignedInteger(sText, uiParsedCount));

  out_uiCount = uiParsedCount;
  return XII_SUCCESS;
}
```

The output is committed only after parsing succeeds.

This provides a clear failure guarantee without requiring the caller to inspect partially modified output state.

#### Multiple Outputs

Several related output values may indicate that a small result structure would provide a clearer interface.

However, do not introduce a new structure merely to eliminate two straightforward output parameters.

Choose the representation that best communicates the operation's contract.

**Guiding principle:**

> Output parameters must make data flow and failure behavior explicit. Prefer committing observable changes only after successful completion when practical.

### 7.5 `const` Correctness

`const` correctness **MUST** be maintained throughout function interfaces and implementations.

Read-only access should be expressed through the type system wherever practical.

#### General Requirements

- Member functions that do not modify observable object state **SHOULD** be marked `const`.
- Read-only reference parameters **SHOULD** use `const T&`.
- Read-only pointer parameters **SHOULD** use `const T*`.
- Avoid exposing mutable access through `const` interfaces unless the operation has a justified logical-constness contract.
- Do not use `const_cast` to bypass interface restrictions without a concrete and documented reason.
- Prefer immutable local values when reassignment is unnecessary and immutability improves clarity.
- Use `mutable` only for members whose modification is compatible with the type's logical constness.
- A `const` member function **MUST NOT** be assumed thread-safe solely because it is declared `const`.

**Prefer:**

```cpp
class xiiSimulationClock
{
public:
  XII_ALWAYS_INLINE xiiUInt64 GetStepCount() const
  {
    return m_uiStepCount;
  }

private:
  xiiUInt64 m_uiStepCount = 0U;
};
```

#### Logical Constness

A `const` method may legitimately update internal implementation details, such as a cache or statistics counter, when those changes do not violate the method's observable contract.

Such mutation **MUST** remain safe under the type's concurrency guarantees.

For example, lazily populating a mutable cache inside a `const` function can introduce data races if multiple threads invoke the function concurrently.

#### Shallow Constness

`const` qualification does not necessarily make all indirectly referenced objects immutable.

A const smart pointer or const container holding pointers may still permit mutation of the pointed-to objects, depending on its API.

Interfaces **MUST** consider the actual mutability of reachable state rather than relying exclusively on top-level `const`.

**Guiding principle:**

> Use `const` to express permitted mutation accurately, but do not confuse logical immutability with thread safety or transitive immutability.

### 7.6 `[[nodiscard]]` and Return-Value Significance

Return values that communicate failure, ownership, identity, or other correctness-critical information **SHOULD** be marked `[[nodiscard]]`.

The attribute helps identify accidental result omission but does not replace explicit error-handling contracts.

#### General Requirements

- Functions returning a failure-sensitive result **SHOULD** use `[[nodiscard]]` where the relevant result type is not already annotated.
- Resource creation functions returning owning objects or handles **SHOULD** use `[[nodiscard]]`.
- Functions returning values whose omission would likely indicate a programming error **SHOULD** use `[[nodiscard]]`.
- Do not apply `[[nodiscard]]` mechanically to every trivial getter without considering existing API conventions.
- Do not suppress return-value diagnostics merely to satisfy compilation.
- If ignoring a result is intentional, use the established mechanism and ensure that doing so is safe under the operation's contract.
- `[[nodiscard]]` **MUST NOT** be treated as a guarantee that callers cannot discard the result.

**Prefer:**

```cpp
xiiResult InitializeSubsystem(); // xiiResult is [[nodiscard]] by convention.

[[nodiscard]] xiiResourceHandle CreateResource(const xiiResourceDescriptor& descriptor);

[[nodiscard]] bool IsResourceValid(xiiResourceHandle hResource) const;
```

An operation returning an important result should make its expected usage apparent.

#### Type-Level Attributes

If `xiiResult` or another return type is already annotated with `[[nodiscard]]`, repeating the attribute on every function may be unnecessary.

Contributors **SHOULD** follow the conventions established by the relevant XII type and surrounding APIs.

**Guiding principle:**

> Important return values should be difficult to ignore accidentally. The type system and compiler diagnostics should reinforce explicit handling.

### 7.7 Results, Failures, and Error Propagation

Expected runtime failures **MUST** be represented through explicit error mechanisms, consistent with §1.1.

`xiiResult` is appropriate when an operation needs to communicate success or failure without returning detailed error information directly.

#### General Requirements

- Functions that may legitimately fail **MUST** provide an observable failure mechanism.
- Failure-sensitive results **MUST NOT** be silently discarded.
- Check operation results before executing dependent operations.
- Prefer early failure propagation when the current function cannot meaningfully recover.
- Use established XII result macros and methods where they improve clarity.
- Do not return `XII_SUCCESS` unless the operation has fulfilled its documented success conditions.
- Error logging **SHOULD** occur where sufficient diagnostic context exists.
- Avoid logging the same failure repeatedly at every propagation layer.
- Do not use `xiiResult` to conceal programming errors or violated API preconditions.
- Failure paths **MUST** preserve the documented invariants and cleanup guarantees of the owning component.

**Prefer:**

```cpp
xiiResult LoadConfiguration(xiiStringView sPath)
{
  xiiFileReader file;

  XII_SUCCEED_OR_RETURN(file.Open(sPath));

  return ParseConfiguration(file);
}
```

#### Result Propagation

Use `XII_SUCCEED_OR_RETURN` when the called operation's failure can be propagated without additional handling.

```cpp
XII_SUCCEED_OR_RETURN(InitializeStorage());
XII_SUCCEED_OR_RETURN(LoadResources());
XII_SUCCEED_OR_RETURN(ValidateState());
```

When additional diagnostic information or recovery behavior is required, use an explicit check.

```cpp
if (LoadConfiguration(sPath).Failed())
{
  xiiLog::Error("Failed to load configuration from '{0}'.", sPath);
  return XII_FAILURE;
}
```

#### Ignoring Results

Use `.IgnoreResult()` only when intentionally discarding the result is permitted by the operation's contract.

**Avoid:**

```cpp
SaveConfiguration(sPath).IgnoreResult();

// A required save may have failed silently.
```

Ignoring a result can be appropriate when failure is genuinely inconsequential or explicitly handled through another mechanism.

Such decisions **SHOULD** be clear from the surrounding implementation or documented where necessary.

#### Rich Error Information

`xiiResult` communicates success or failure but does not inherently carry a detailed failure reason.

When callers need structured diagnostics, error categories, partial results, or recovery information, use an appropriate result structure or explicit error output.

Do not introduce an elaborate generic error framework when a simple `xiiResult` and contextual diagnostic are sufficient.

**Guiding principle:**

> Failure is part of an operation's contract. Propagate it explicitly, preserve invariants, and report useful diagnostics without unnecessary complexity.

### 7.8 Default Arguments

Default arguments **SHOULD** represent common, safe, and unsurprising behavior.

They are appropriate when omitting an argument has a stable meaning that is understandable from the function's name and contract.

#### General Requirements

- Default arguments **MUST** preserve valid behavior.
- Defaults **SHOULD** represent the most common or least surprising operation.
- Avoid defaults that depend on hidden mutable state or ambiguous execution context.
- Do not use default arguments to conceal required dependencies.
- Avoid long parameter lists containing multiple unrelated defaulted values.
- Boolean parameters **SHOULD** be avoided when their meaning is unclear at the call site.
- Prefer a configuration structure when an operation has several independently configurable options.
- Changes to public default arguments **MUST** consider source compatibility and behavioral effects on existing callers.
- Default arguments **MUST NOT** be redeclared inconsistently across declarations.

**Prefer:**

```cpp
xiiResult StartSimulation(xiiUInt32 uiMaxIterations = 128U);
```

**Avoid:**

```cpp
StartSimulation(true, false, true, 128U);
```

The latter obscures the purpose of each boolean argument.

#### Configuration Structures

When several independent options are required, a configuration structure can provide clearer call sites and allow future extension.

**Prefer:**

```cpp
struct xiiSimulationOptions
{
  bool      m_bEnableTracing    = false;
  bool      m_bAllowParallelism = true;
  xiiUInt32 m_uiMaxIterations   = 128U;
};

xiiResult StartSimulation(const xiiSimulationOptions& options);
```

However, do not introduce a configuration structure for every function with two or three clearly understood parameters.

#### Default Arguments and Virtual Functions

Default arguments are resolved statically according to the declaration visible at the call site, while virtual dispatch selects the implementation dynamically.

Therefore, default arguments **SHOULD NOT** differ across overrides.

Where virtual interfaces require default behavior, consider a non-virtual convenience wrapper that delegates to the virtual operation with explicit arguments.

**Guiding principle:**

> Defaults should simplify common use without concealing meaning, dependencies, or behavioral differences.

### 7.9 Overloads and Operation Semantics

Function overloads **SHOULD** represent the same conceptual operation applied to different supported argument forms.

Do not use overloads to conceal significantly different behavior, ownership requirements, or side effects.

#### General Requirements

- Overloads **MUST** preserve a coherent semantic relationship.
- Functions with substantially different responsibilities **SHOULD** use distinct names.
- Overloads **MUST NOT** unexpectedly change ownership transfer or lifetime semantics.
- Avoid overload sets that create ambiguous conversions or surprising overload resolution.
- Do not introduce overloads solely to avoid explicit conversions when those conversions clarify intent.
- Prefer consistent failure and return-value semantics across related overloads.
- Overload implementations **SHOULD** share a common implementation where doing so reduces duplication without introducing unnecessary indirection.
- Changes to overload sets **MUST** consider their effects on existing call sites and implicit conversions.

**Prefer:**

```cpp
xiiResult LoadResource(xiiStringView sPath);
xiiResult LoadResource(const xiiResourceDescriptor& descriptor);
```

Both functions perform resource loading using different forms of input.

**Avoid:**

```cpp
void Process(xiiResourceHandle hResource);
void Process(xiiUInt32 uiMilliseconds);
```

The two operations have unrelated meanings despite sharing a name.

#### Lifecycle Semantics

An operation named `ExecuteTask()` should not implicitly initialize an entire task system or advance an unrelated scheduler unless that behavior is part of its documented contract.

Prefer distinct operations for distinct lifecycle stages.

**Guiding principle:**

> Overloading should provide alternative ways to perform the same operation, not hide unrelated behavior behind a common name.

### 7.10 `noexcept` and Exception Boundaries

XII does not permit C++ exceptions for ordinary engine error handling.

Expected failures **MUST** be represented through explicit mechanisms such as `xiiResult`, error outputs, or appropriately designed result types.

Exceptions may be encountered within deeply isolated platform or third-party integration code where their use is unavoidable. Such exceptions must not escape into ordinary engine-facing APIs.

#### General Requirements

- Engine-facing functions **MUST NOT** use exceptions to communicate expected runtime failures.
- Do not introduce `throw` expressions into ordinary engine code.
- Use `noexcept` only when the operation can satisfy the exception guarantee under its supported execution environments.
- Do not annotate functions `noexcept` merely because the current compiler configuration disables exceptions.
- Functions marked `noexcept` **MUST NOT** permit exception propagation from platform or third-party operations.
- Exception-generating dependencies **MUST** be isolated at appropriate integration boundaries.
- Where exception handling is available and unavoidable, caught failures **SHOULD** be translated into the engine's established error-reporting mechanisms.
- Destructors **MUST NOT** propagate exceptions.
- Do not use exception specifications as substitutes for documenting ordinary failure behavior.

#### Platform Exception Boundaries

Where a third-party or platform API may throw exceptions, the integration layer should contain the exception mechanism when the build configuration supports catching those exceptions.

The translated failure should be returned through an ordinary engine-compatible mechanism.

If the relevant environment does not permit exception handling, the integration must use an appropriate non-throwing API or another explicitly supported boundary strategy.

**Guiding principle:**

> Expected failures are explicit results. Exception behavior, when unavoidable, must remain isolated from ordinary XII engine contracts.

### 7.11 Returning References, Pointers, and Views

Returned pointers, references, and views **MUST** have clear validity and invalidation guarantees.

A returned reference communicates access, not ownership.

The relevant lifetime and borrowing requirements are established in §§1.2 and 6.3–6.8.

#### General Requirements

- Never return references or views into local temporary storage.
- Returned borrowed access **MUST** remain valid for the documented duration.
- Do not return mutable references to internal state when doing so permits callers to violate invariants.
- Prefer returning small value-like objects directly when copying is inexpensive.
- Use handles when independently managed identity and validation are required.
- Do not return pointers into containers without considering relocation and removal invalidation.
- Access to shared mutable objects **MUST** respect their synchronization requirements.
- Prefer scoped access or controlled operations when a returned pointer cannot safely outlive an internal lock.
- Do not expose borrowed access across asynchronous boundaries without sufficient lifetime guarantees.

**Avoid:**

```cpp
const xiiString& GetName() const
{
  xiiString sName = BuildName();
  return sName;
}
```

The returned reference refers to an object destroyed when the function returns.

**Prefer:**

```cpp
xiiString GetName() const
{
  return BuildName();
}
```

Returning an owning value avoids the lifetime violation.

#### Internal Storage

Returning references to internal storage may be appropriate for stable, read-only access when the owning object's lifetime and invalidation rules are explicit.

However, a `const` reference is not automatically safe if another thread can mutate or destroy the referenced object.

**Guiding principle:**

> Returned access must remain valid for as long as callers are permitted to use it. Ownership, invalidation, and concurrency guarantees must be explicit.

### 7.12 Inline and Force-Inline Functions

Inlining decisions **SHOULD** be based on language requirements, implementation clarity, and demonstrated performance needs.

The `inline` keyword and compiler-specific force-inline mechanisms serve different purposes and must not be treated as interchangeable guarantees.

#### General Requirements

- Use ordinary `inline` where required for header-defined non-template functions with external linkage.
- Small, stable accessors **MAY** be defined directly within class declarations.
- Non-trivial non-template implementations **SHOULD** remain in `.cpp` files unless header placement is justified.
- Use `XII_ALWAYS_INLINE` sparingly and consistently with established performance-sensitive code.
- Do not assume that ordinary `inline` guarantees machine-code inlining.
- Do not use force-inlining to compensate for inefficient algorithms or unnecessary abstraction layers.
- Avoid excessive header-defined implementation code that increases compilation time and dependency propagation.
- Performance-motivated force-inlining **SHOULD** be supported by measurements, consistent with §1.5.
- Inlining decisions **MUST NOT** violate linkage, One Definition Rule (ODR), or binary interface requirements.

**Prefer:**

```cpp
class xiiTaskCounter
{
public:
  XII_ALWAYS_INLINE xiiUInt32 GetCount() const
  {
    return m_uiCount;
  }

private:
  xiiUInt32 m_uiCount = 0U;
};
```

The accessor is trivial and suitable for a header definition.

For larger implementations, prefer ordinary out-of-line definitions.

#### Force-Inlining Trade-offs

Force-inlining may reduce call overhead in appropriate hot paths, but it can also:

- Increase generated code size.
- Increase instruction-cache pressure.
- Increase compilation time.
- Duplicate substantial implementation code.
- Prevent the compiler from making more effective code-size trade-offs.

An aggressively inlined implementation is not necessarily faster.

**Guiding principle:**

> Inline for correctness and clarity where required; force-inline only when the performance benefit justifies the cost.

### 7.13 Function Preconditions and Postconditions

Function contracts **MUST** distinguish required preconditions from expected runtime failures.

A precondition describes what must be true for a valid call. A postcondition describes the guarantees established after an operation completes.

#### General Requirements

- Preconditions **SHOULD** be apparent from the interface or documented where they are not obvious.
- Contract violations **SHOULD** be detected through appropriate assertions or explicit enforcement mechanisms.
- Expected runtime failures **MUST** use the applicable error-reporting mechanism.
- Successful operations **MUST** establish their documented postconditions.
- Failed operations **MUST** preserve their documented failure guarantees.
- Do not use assertions as substitutes for validating untrusted input.
- Do not return `XII_FAILURE` for every programmer error merely to avoid defining correct API contracts.
- Functions **MUST NOT** silently accept invalid states that compromise internal invariants.

**Example:**

```cpp
void SetCapacity(xiiUInt32 uiCapacity)
{
  XII_ASSERT_DEV(uiCapacity > 0U, "Capacity must be greater than zero.");

  m_uiCapacity = uiCapacity;
}
```

Here, a positive capacity is a programmer-facing precondition.

By contrast:

```cpp
xiiResult LoadSettings(xiiStringView sPath)
{
  xiiFileReader file;

  XII_SUCCEED_OR_RETURN(file.Open(sPath));

  return ParseSettings(file);
}
```

File opening and parsing can fail during otherwise valid calls and therefore expose expected runtime failures.

**Guiding principle:**

> Preconditions define valid calls. Results communicate expected failure. Postconditions define what callers may rely on after completion.

### 7.14 Side Effects and Observable Behavior

Function names and signatures **SHOULD** communicate meaningful side effects.

An operation's observable behavior must be consistent with the expectations established by its interface.

#### General Requirements

- Query functions **SHOULD NOT** modify externally observable state unless their contract explicitly permits it.
- Functions that mutate state **SHOULD** use names that communicate the operation being performed.
- Do not conceal resource allocation, synchronization, scheduling, or substantial I/O behind apparently trivial accessors.
- Operations that may block for significant or unbounded periods **SHOULD** make that behavior apparent through their naming or documented contract.
- Avoid unexpected global state changes within functions that appear to operate on local data.
- Side effects **MUST** remain consistent across overloads that represent the same conceptual operation.
- Functions **SHOULD** avoid partially committing externally observable state before recoverable validation has completed.

**Prefer:**

```cpp
bool IsTaskComplete(xiiTaskHandle hTask) const;

xiiResult WaitForTask(xiiTaskHandle hTask);
```

These names distinguish a non-blocking query from an operation that may wait.

**Avoid:**

```cpp
bool GetTaskComplete(xiiTaskHandle hTask);
```

If the implementation also waits for the task to finish, its name conceals important execution behavior.

**Guiding principle:**

> Callers should not need to inspect an implementation to discover consequential side effects.

### 7.15 Function Complexity and Reviewability

Functions **SHOULD** remain understandable as cohesive units of behavior.

Complexity should be controlled through appropriate decomposition, explicit state transitions, and meaningful abstraction boundaries rather than arbitrary limits on function length.

#### General Requirements

- Avoid deeply nested control flow when guard clauses or restructuring can simplify the implementation.
- Avoid functions that coordinate unrelated responsibilities or manipulate unrelated subsystem state.
- Prefer descriptive intermediate variables when they clarify non-obvious expressions.
- Do not introduce unnecessary temporaries or helpers that make straightforward logic harder to follow.
- Keep resource acquisition, use, and release relationships visible.
- Error paths **SHOULD** remain easy to follow and verify.
- Complex algorithms **SHOULD** document important invariants and non-obvious reasoning.
- Do not introduce generic abstractions solely to reduce apparent function complexity.
- Refactoring **MUST** preserve externally observable behavior unless a behavioral change is intentional.

A long function implementing one cohesive algorithm may be clearer than several tightly coupled functions that require readers to reconstruct control flow across multiple files.

Conversely, a short function may still be poorly designed if it combines unrelated responsibilities or conceals significant side effects.

**Guiding principle:**

> Optimize functions for comprehension and correctness, not arbitrary line counts. Structure should reveal intent, dependencies, and failure behavior.

## 8. Control Flow and Expressions

Control flow and expressions in XII **MUST** be explicit, predictable, and correct across supported platforms, compilers, and execution environments.

Prefer straightforward branching, well-defined evaluation semantics, and mathematically sound operations over compact expressions that obscure program behavior.

Control-flow decisions should preserve the invariants established in §1.1, follow the statement conventions in §2.4, and respect the error-handling requirements in §7.7.

### 8.1 Prefer Early Exits and Guard Clauses

Guard clauses **SHOULD** be used to handle invalid inputs, unmet preconditions, and recoverable failures before entering the main body of an operation.

Early exits reduce unnecessary nesting and make the successful execution path easier to understand.

#### General Requirements

- Prefer early returns when they simplify control flow and make failure conditions immediately apparent.
- Validate untrusted inputs before using them in operations that require specific ranges or invariants.
- Distinguish programmer contract violations from expected runtime failures, as established in §1.1.
- Do not use deeply nested conditionals when guard clauses express the same behavior more clearly.
- Error paths **MUST** preserve resource ownership, synchronization, and cleanup guarantees.
- Prefer RAII for resources that require cleanup across multiple return paths.
- Do not introduce multiple early returns when they obscure a required common finalization step.
- Avoid redundant validation when a preceding operation already establishes the necessary invariant.

**Prefer:**

```cpp
xiiResult ProcessDocument(xiiStringView sPath)
{
  if (sPath.IsEmpty())
    return XII_FAILURE;

  xiiFileReader file;

  XII_SUCCEED_OR_RETURN(file.Open(sPath));

  return ParseDocument(file);
}
```

**Avoid:**

```cpp
xiiResult ProcessDocument(xiiStringView sPath)
{
  if (!sPath.IsEmpty())
  {
    xiiFileReader file;

    if (file.Open(sPath).Succeeded())
    {
      return ParseDocument(file);
    }
  }

  return XII_FAILURE;
}
```

The preferred implementation expresses the unsuccessful conditions first and leaves the primary operation at the function's normal indentation level.

#### Cleanup and Early Returns

Early returns **MUST NOT** bypass required cleanup.

Where cleanup is necessary, prefer scoped ownership or another established resource-management mechanism rather than duplicating cleanup logic across failure branches.

A function with a single exit point may be preferable when a required finalization step cannot be represented safely through RAII.

**Guiding principle:**

> Reject invalid states early and keep the successful path clear, but never simplify control flow at the expense of cleanup or invariants.

### 8.2 Use the Simplest Appropriate Branching Construct

Branching constructs **SHOULD** match the nature of the decision being expressed.

Do not introduce runtime polymorphism, template machinery, or complicated conditional expressions when ordinary control flow adequately expresses the requirement.

#### General Requirements

- Use `if` and `else` for general conditional logic.
- Use `switch` when selecting among discrete values, particularly enum states.
- Use `if constexpr` when compile-time conditions determine valid template implementations.
- Prefer ordinary compile-time constants over preprocessor conditionals when C++ language constructs are sufficient.
- Avoid long chains of runtime type checks when a genuine polymorphic interface would better represent the required behavior.
- Do not introduce inheritance or virtual dispatch solely to replace a small, stable conditional.
- Branches **MUST** preserve the same documented invariants regardless of the selected path.
- Exhaustive branching over enum values **SHOULD** make newly introduced values detectable through compiler diagnostics or review.

#### Discrete Alternatives

**Prefer:**

```cpp
xiiResult ProcessTask(xiiTaskState::Enum state)
{
  switch (state)
  {
    case xiiTaskState::Pending:
      return QueueTask();

    case xiiTaskState::Running:
      return UpdateTask();

    case xiiTaskState::Completed:
      return FinalizeTask();
  }

  return XII_FAILURE;
}
```

When an enum is intended to be exhaustive, explicitly listing its supported values makes the contract easier to inspect.

A final failure return can handle invalid numeric representations without hiding missing cases behind a `default` branch.

Whether compiler warnings detect new enum values depends on the compiler and configured warning policy.

#### Default Cases

A `default` branch is appropriate when unknown values have a meaningful, documented fallback.

**Prefer when unknown values are expected:**

```cpp
switch (message.m_Type)
{
  case xiiMessageType::Connect:
    HandleConnection();
    break;

  case xiiMessageType::Disconnect:
    HandleDisconnection();
    break;

  default:
    HandleUnknownMessage(message);
    break;
}
```

Do not use a silent `default` merely to suppress warnings when every known enum value requires explicit handling.

#### Compile-Time Branching

Use `if constexpr` when template logic depends on compile-time type properties.

```cpp
template <typename T>
void ResetValue(T& value)
{
  if constexpr (std::is_trivially_copyable_v<T>)
  {
    value = T{};
  }
  else
  {
    value.Reset();
  }
}
```

This example assumes the supported non-trivially-copyable types provide `Reset()`.

Compile-time branching **MUST** still establish a valid contract for every supported template instantiation.

**Guiding principle:**

> Choose control flow according to the decision being made. Prefer explicit branches over unnecessary dispatch machinery.

### 8.3 Loops and Iteration

Iteration **MUST** preserve container invariants, element validity, and any ordering guarantees required by the operation.

Select the loop construct that most clearly communicates whether the operation depends on elements, indices, or iterator behavior.

#### General Requirements

- Prefer range-based loops when only element access is required.
- Use index-based loops when the index has semantic meaning or is necessary for the algorithm.
- Use explicit iterators when container mutation, traversal rules, or specialized iteration requires them.
- Do not repeatedly perform expensive computations in loop conditions when their results can be safely reused.
- Avoid unnecessary allocations, copies, and indirect access in demonstrated hot loops.
- Do not mutate a container during iteration unless the iteration pattern explicitly supports the mutation.
- Container removal **MUST** account for iterator, pointer, reference, and index invalidation.
- Do not assume that container removal preserves order unless the selected operation guarantees it.
- Loop termination **MUST** be well-defined, including for empty collections and boundary values.
- Avoid modifying the loop control variable unexpectedly inside the body.

#### Element-Based Iteration

**Prefer:**

```cpp
for (const xiiResourceEntry& entry : m_Entries)
{
  ProcessEntry(entry);
}
```

Use mutable references only when modification is intended.

```cpp
for (xiiResourceEntry& entry : m_Entries)
{
  entry.m_bActive = false;
}
```

#### Index-Based Iteration

Use indices when element positions are part of the operation's meaning.

```cpp
for (xiiUInt32 uiIndex = 0; uiIndex < m_Entries.GetCount(); ++uiIndex)
{
  UpdateEntry(uiIndex, m_Entries[uiIndex]);
}
```

Index types **SHOULD** be compatible with the container's supported index range.

Avoid narrowing a container's element count into a smaller integer type without establishing that the value fits.

#### Removal During Iteration

Removal operations may shift elements, replace removed elements with the last element, or invalidate iterators.

For example, when removal order is unimportant and `RemoveAtAndSwap()` provides the expected semantics:

```cpp
for (xiiUInt32 uiIndex = 0; uiIndex < m_Entries.GetCount();)
{
  if (m_Entries[uiIndex].m_bExpired)
  {
    m_Entries.RemoveAtAndSwap(uiIndex);
    continue;
  }

  ++uiIndex;
}
```

The index is not advanced following removal because another element may have been moved into the current position.

Do not use swap-based removal when preserving the order of surviving elements is a documented requirement.

#### Loop Progress

Every potentially unbounded loop **MUST** have a defined progress or termination mechanism.

This is especially important for polling, task scheduling, synchronization, and resource reclamation.

Busy-wait loops **SHOULD NOT** be introduced when an appropriate blocking or event-driven mechanism is available.

**Guiding principle:**

> Iterate according to the semantics of the data and the operation. Container mutation, ordering, and progress must remain explicit.

### 8.4 Comparators and Ordering Contracts

Comparators used by sorting and ordered containers **MUST** satisfy the ordering requirements of the relevant algorithm.

For standard strict-ordering algorithms, the comparator must establish a strict weak ordering over the supported input domain.

#### General Requirements

- Strict ordering comparators **MUST** return false when comparing equivalent values.
- Do not use `<=` or `>=` as strict ordering comparators.
- Comparators **MUST** produce consistent results for unchanged inputs.
- Comparator behavior **MUST NOT** depend on mutable state that can change during sorting.
- Avoid comparators with observable side effects.
- Do not modify compared elements from within the comparator.
- Sorting **MUST NOT** be assumed stable unless the algorithm explicitly guarantees stability.
- Tie-breaking behavior **MUST** be defined when equal primary keys require deterministic ordering.
- Comparators involving floating-point values **MUST** account for NaNs or establish a valid input domain that excludes them.

**Prefer:**

```cpp
m_Entries.Sort([](const auto& a, const auto& b)
{
  return a.m_iPriority > b.m_iPriority;
});
```

This sorts entries by descending priority.

However, it does not guarantee the relative order of entries with equal priorities if the sorting algorithm is unstable.

#### Deterministic Tie-Breaking

When deterministic ordering among equal priorities is required, use an explicit secondary key.

```cpp
m_Entries.Sort([](const auto& a, const auto& b)
{
  if (a.m_iPriority != b.m_iPriority)
    return a.m_iPriority > b.m_iPriority;

  return a.m_uiRegistrationOrder < b.m_uiRegistrationOrder;
});
```

The registration order must represent a stable and well-defined value for the relevant sorting operation.

Do not introduce artificial secondary keys unless deterministic tie ordering is a genuine requirement.

#### Invalid Comparators

**Avoid:**

```cpp
m_Entries.Sort([](const auto& a, const auto& b)
{
  return a.m_iPriority >= b.m_iPriority;
});
```

For equal values, the comparator returns true in both directions.

This violates the strict ordering requirements and can result in undefined behavior when used with sorting algorithms that require strict weak ordering.

#### Floating-Point Ordering

Ordinary floating-point `<` does not establish strict weak ordering over a domain containing NaNs.

When sorting floating-point data that may contain NaNs, define an explicit ordering policy or reject invalid values before sorting.

Do not use approximate equality in a comparator without verifying that the resulting relation still satisfies the ordering requirements.

**Guiding principle:**

> Ordering is an algorithmic contract. Comparators must be mathematically valid, deterministic for their supported inputs, and consistent with required tie semantics.

### 8.5 Boolean Logic and Conditional Expressions

Boolean expressions **SHOULD** make their intended meaning and evaluation order apparent.

Prefer straightforward predicates over compact expressions that require careful reconstruction of operator precedence.

#### General Requirements

- Use parentheses when mixing logical operators in expressions whose grouping may be misinterpreted.
- Prefer descriptive intermediate predicates for complex domain-specific conditions.
- Avoid unnecessary negations and double-negative state names.
- Do not introduce intermediate boolean variables for every trivial condition.
- Do not rely on implicit precedence when explicit grouping materially improves clarity.
- Avoid boolean expressions that combine unrelated side effects.
- Use short-circuit operators deliberately when later operands depend on earlier conditions.
- Conditional expressions **MUST** preserve the documented evaluation and failure semantics.

**Prefer:**

```cpp
const bool bValidSession = session.IsActive() && session.GetSessionId() == uiExpectedSession;

const bool bHasDependencies = pScheduler != nullptr && pResourceManager != nullptr;

if (!bValidSession || !bHasDependencies)
  return XII_FAILURE;
```

This separates session validity from dependency availability.

#### Short-Circuit Evaluation

Logical `&&` and `||` operators evaluate their operands with defined short-circuit behavior.

**Prefer:**

```cpp
if (pResource != nullptr && pResource->IsReady())
{
  ProcessResource(*pResource);
}
```

The resource is accessed only when the pointer is non-null.

Do not replace logical operators with bitwise operators when short-circuit evaluation is required.

#### Avoid Redundant Predicates

**Avoid:**

```cpp
if (bInitialized == true)
{
  Execute();
}
```

**Prefer:**

```cpp
if (bInitialized)
{
  Execute();
}
```

For negative conditions:

```cpp
if (!bInitialized)
  return XII_FAILURE;
```

Do not sacrifice readability merely to eliminate parentheses or intermediate variables.

**Guiding principle:**

> Boolean logic should communicate domain meaning clearly while preserving correct precedence and short-circuit behavior.

### 8.6 Avoid Hidden Mutations and Evaluation-Order Dependencies

Expressions **MUST NOT** rely on undefined or unspecified evaluation behavior.

Side effects should be explicit, appropriately sequenced, and easy to inspect.

#### General Requirements

- Do not modify the same scalar object multiple times in an expression when the modifications are unsequenced.
- Avoid combining resource creation, validation, mutation, and consumption in unnecessarily complex expressions.
- Prefer separate statements when evaluation order affects correctness.
- Do not rely on compiler-specific operand evaluation behavior.
- Function arguments **SHOULD NOT** contain interdependent side effects whose order matters.
- Increment and decrement operations **SHOULD** be used where their effects are obvious.
- Do not use comma expressions to combine unrelated operations.
- Side effects **MUST** remain visible when failure handling or synchronization depends on their ordering.

**Avoid:**

```cpp
ProcessValues(++uiIndex, uiIndex++);
```

The expression modifies the same variable through multiple arguments and obscures the intended sequencing.

**Prefer:**

```cpp
++uiIndex;

const xiiUInt32 uiCurrentIndex = uiIndex;
++uiIndex;

ProcessValues(uiCurrentIndex, uiIndex);
```

The intended sequence is explicit.

#### Resource Creation and Validation

**Prefer:**

```cpp
xiiResourceHandle hResource = CreateResource(descriptor);

if (hResource.IsInvalidated())
  return XII_FAILURE;

InitializeResource(hResource);
```

**Avoid:**

```cpp
InitializeResource(CreateResource(descriptor));
```

The avoided form prevents the caller from inspecting creation failure before attempting initialization.

This separation is especially important when the second operation requires a valid handle.

**Guiding principle:**

> Observable state changes should occur in an explicit, understandable sequence. Expression compactness must not conceal evaluation or failure behavior.

### 8.7 Integer Conversions and Arithmetic

Integer arithmetic **MUST** respect the range, signedness, and representation requirements of the values being manipulated.

Overflow and narrowing errors can lead to invalid allocations, incorrect indexing, corrupted resource descriptions, and undefined behavior.

#### General Requirements

- Use integer types whose widths and signedness match the domain's required range.
- Check for overflow before calculations whose results control allocation sizes, offsets, strides, or buffer capacities.
- Do not assume that widening the destination prevents overflow if intermediate arithmetic occurs in a narrower type.
- Validate potentially negative signed values before converting them to unsigned types.
- Narrowing conversions **MUST** be intentional and range-checked when the source may exceed the destination range.
- Use explicit casts where they communicate intentional conversions.
- Avoid unnecessary casts that conceal incorrect type choices.
- Division and modulo operations **MUST** establish valid nonzero divisors.
- Signed arithmetic **MUST NOT** rely on overflow.
- Unsigned wraparound **MUST** be intentional and documented when its behavior is required.
- Integer comparisons **SHOULD** avoid unintended signed/unsigned conversions.
- Avoid arithmetic involving invalid sentinel values unless the sentinel has been checked.

#### Multiplication Overflow

**Prefer:**

```cpp
if (uiStride == 0U)
  return XII_FAILURE;

if (uiElementCount > xiiMath::MaxValue<xiiUInt64>() / uiStride)
  return XII_FAILURE;

const xiiUInt64 uiByteCount = static_cast<xiiUInt64>(uiElementCount) * uiStride;
```

The example assumes that both operands are unsigned and that zero stride is invalid under the operation's contract.

Checking before multiplication prevents the result from wrapping.

#### Intermediate Arithmetic

**Avoid:**

```cpp
xiiUInt32 uiWidth  = descriptor.m_uiWidth;
xiiUInt32 uiHeight = descriptor.m_uiHeight;

xiiUInt64 uiPixelCount = uiWidth * uiHeight;
```

The multiplication may overflow at the 32-bit width before its result is assigned to a 64-bit variable.

**Prefer:**

```cpp
const xiiUInt64 uiPixelCount = static_cast<xiiUInt64>(descriptor.m_uiWidth) * descriptor.m_uiHeight;
```

Widening an operand before multiplication causes the multiplication to occur in the wider type.

This is sufficient only when the resulting value is guaranteed to fit within that type. Otherwise, an overflow check is still required.

#### Narrowing Conversions

**Prefer:**

```cpp
if (uiElementCount > xiiMath::MaxValue<xiiUInt32>())
  return XII_FAILURE;

const xiiUInt32 uiCount = static_cast<xiiUInt32>(uiElementCount);
```

Do not perform narrowing conversions merely to satisfy a function signature without confirming that the value fits.

#### Signed and Unsigned Values

**Avoid:**

```cpp
xiiInt32 iOffset = GetOffset();

xiiUInt32 uiOffset = static_cast<xiiUInt32>(iOffset);
```

If `iOffset` is negative, conversion to an unsigned type produces an unintended large value.

**Prefer:**

```cpp
const xiiInt32 iOffset = GetOffset();

if (iOffset < 0)
  return XII_FAILURE;

const xiiUInt32 uiOffset = static_cast<xiiUInt32>(iOffset);
```

#### Division and Modulo

**Prefer:**

```cpp
if (uiBucketCount == 0U)
  return XII_FAILURE;

const xiiUInt32 uiBucketIndex = uiHash % uiBucketCount;
```

Never assume a divisor is nonzero merely because normal execution typically supplies a valid value.

When nonzero is a programmer-facing precondition rather than recoverable input validation, enforce it according to §7.13.

**Guiding principle:**

> Integer correctness requires control over widths, ranges, and conversions. Validate arithmetic before a result can overflow, narrow incorrectly, or violate an invariant.

### 8.8 Floating-Point Arithmetic and Comparisons

Floating-point calculations **MUST** account for finite precision, rounding behavior, and the possibility of special values.

Numerical code should use comparison and tolerance policies appropriate to the domain rather than assume that floating-point values behave like exact real numbers.

#### General Requirements

- Do not use exact equality for approximate numerical results unless exact representation or identity is part of the contract.
- Use tolerances appropriate to the scale, precision, and expected error of the operation.
- Avoid using a single universal epsilon for unrelated numerical domains.
- Consider absolute and relative tolerances when comparing values of different magnitudes.
- Handle NaNs and infinities when they may arise from external inputs or calculations.
- Do not assume floating-point expressions are associative.
- Avoid transformations that change numerical behavior unless their accuracy and performance implications are understood.
- Numerical algorithms **SHOULD** account for loss of precision, cancellation, and accumulation error where relevant.
- Do not rely on exact reproducibility across platforms unless the required compiler, instruction, and floating-point behavior is explicitly controlled.
- Floating-point values used in ordering, hashing, or identity comparisons **MUST** follow the requirements of those operations.

#### Approximate Equality

**Avoid:**

```cpp
if (fCalculatedValue == fExpectedValue)
{
  AcceptResult();
}
```

**Prefer when approximate equality is intended:**

```cpp
const float fDifference = xiiMath::Abs(fCalculatedValue - fExpectedValue);

const float fScale = xiiMath::Max(xiiMath::Abs(fCalculatedValue), xiiMath::Abs(fExpectedValue));

const float fTolerance = fAbsoluteTolerance + fRelativeTolerance * fScale;

if (fDifference <= fTolerance)
{
  AcceptResult();
}
```

This comparison combines absolute and relative tolerances.

It assumes the inputs and tolerance parameters satisfy the algorithm's finite-value and nonnegative-tolerance requirements. Validate those assumptions where they are not guaranteed.

#### Exact Equality

Exact floating-point equality may be appropriate for:

- Comparing values that are required to match an exact representation.
- Checking values assigned from the same defined constant.
- Certain sentinel values where their meaning is explicitly defined.
- Algorithms whose mathematical contract requires bitwise or exact-value identity.

Do not replace every floating-point equality comparison with an epsilon check mechanically.

#### NaNs and Infinities

NaN values have unusual comparison semantics, including the fact that ordinary equality and ordering comparisons do not behave like comparisons over real numbers.

When finite values are required, validate them before performing calculations that depend on finite arithmetic.

```cpp
if (!xiiMath::IsFinite(fDeltaTime))
  return XII_FAILURE;
```

This assumes the operation treats non-finite time values as invalid input.

#### Numerical Stability

Prefer numerically stable formulations when equivalent mathematical expressions exhibit different error behavior.

For iterative simulations, reductions, and long-running computations, consider how rounding error accumulates over time.

Reordering floating-point operations for performance **MUST** account for its effect on numerical accuracy and reproducibility.

**Guiding principle:**

> Floating-point correctness is defined by acceptable numerical error and explicit domain assumptions, not by the appearance of mathematical equivalence.

### 8.9 Switch Fallthrough

Fallthrough between `switch` cases **MUST** be intentional and immediately recognizable.

Use the standard `[[fallthrough]]` attribute where supported and consistent with the surrounding code.

#### General Requirements

- Implicit fallthrough **MUST NOT** occur accidentally.
- Intentional fallthrough **SHOULD** use `[[fallthrough]]`.
- Do not rely solely on comments to indicate fallthrough when the compiler supports the standard attribute.
- Each case **SHOULD** terminate through `break`, `return`, another control transfer, or explicit fallthrough.
- Avoid fallthrough when duplicating a small amount of straightforward code would communicate the behavior more clearly.
- Multiple case labels sharing the same body do not require `[[fallthrough]]` between consecutive labels.
- Fallthrough **MUST NOT** bypass required initialization or violate variable scope rules.

**Prefer:**

```cpp
switch (state)
{
  case xiiTaskState::Failed:
    LogTaskFailure();
    [[fallthrough]];

  case xiiTaskState::Cancelled:
    ReleaseTaskResources();
    break;

  case xiiTaskState::Completed:
    FinalizeTask();
    break;
}
```

The failure case performs additional logging before intentionally sharing cleanup behavior with cancellation.

#### Shared Case Bodies

When multiple values require identical behavior, group their labels.

```cpp
switch (state)
{
  case xiiTaskState::Failed:
  case xiiTaskState::Cancelled:
    ReleaseTaskResources();
    break;

  case xiiTaskState::Completed:
    FinalizeTask();
    break;
}
```

This is not an implicit fallthrough error; both labels intentionally select the same statements.

**Guiding principle:**

> Switch cases should have explicit termination behavior. Intentional fallthrough must be visible to both readers and compiler diagnostics.

### 8.10 Conditional Compilation and Feature Selection

Conditional compilation **SHOULD** be limited to cases where behavior genuinely differs by platform, compiler, build configuration, or compile-time feature availability.

Prefer ordinary C++ constructs when they provide equivalent functionality without introducing unnecessary preprocessor complexity.

#### General Requirements

- Use established XII feature and platform macros rather than inventing alternative detection mechanisms.
- Follow the repository's conventions for `XII_ENABLED` and related compile-time feature checks.
- Do not use runtime checks for features that are intentionally unavailable in a given build configuration when compile-time exclusion is required.
- Do not use preprocessor conditionals when `if constexpr` or ordinary constant expressions provide a clearer solution.
- Platform-specific code **SHOULD** remain within the appropriate platform implementation rather than spread throughout portable modules.
- Conditional branches **MUST** preserve the documented interface contract across supported configurations.
- Avoid deeply nested preprocessor conditionals.
- Build configurations **MUST NOT** silently change public type layouts or exported interfaces when those interfaces require binary compatibility.

**Example:**

```cpp
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  InitializeWindowsPlatform();
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  InitializeLinuxPlatform();
#else
  InitializeGenericPlatform();
#endif
```

The exact feature macros and supported fallbacks must follow the existing repository definitions.

#### Compile-Time Versus Runtime Capabilities

Compile-time availability and runtime capability are separate concepts.

A feature may be compiled into the engine but remain unsupported by the active device or execution environment.

Use runtime capability queries when feature support depends on detected hardware, drivers, or system configuration.

Do not assume that compilation of a backend implies support for every optional feature.

**Guiding principle:**

> Use conditional compilation to express build-time availability and runtime checks to express actual capabilities. Keep both mechanisms explicit and appropriately scoped.

### 8.11 Assertions and Unreachable States

Assertions **SHOULD** be used to detect violated internal invariants and programmer-facing preconditions.

They **MUST NOT** replace ordinary error handling for expected runtime failures.

#### General Requirements

- Assertions **MUST** express conditions expected to hold during every valid execution of the relevant code path.
- Do not rely on assertions to perform required initialization, mutation, or cleanup.
- Do not place essential side effects inside assertion expressions.
- Expected failures caused by external data or environmental conditions **MUST** be handled through the appropriate error mechanism.
- Unreachable-state assumptions **MUST** be justified by established invariants.
- Do not introduce compiler-specific unreachable hints when an invalid state can legitimately occur.
- Assertions **SHOULD** include diagnostics that identify the violated contract.
- Do not use assertions merely to suppress handling of a state that the public API permits.

**Prefer:**

```cpp
XII_ASSERT_DEV(uiIndex < m_Entries.GetCount(), "Resource index is outside the valid range.");

ProcessEntry(m_Entries[uiIndex]);
```

This is appropriate when the caller is required by contract to supply a valid internal index.

For externally supplied indices, use explicit runtime validation instead.

#### Essential Side Effects

**Avoid:**

```cpp
XII_ASSERT_DEV(InitializeResources().Succeeded(), "Initialization failed.");
```

If assertions are disabled, the initialization operation may not execute.

**Prefer:**

```cpp
const xiiResult result = InitializeResources();

XII_ASSERT_DEV(result.Succeeded(), "Initialization failed.");
```

This version preserves execution of the operation regardless of whether the assertion is active.

If initialization failure is an expected runtime condition, the failure should instead be propagated or handled explicitly.

**Guiding principle:**

> Assertions verify invariants; they must never be required for correct execution or substitute for recoverable error handling.

### 8.12 Expression Complexity and Readability

Expressions **SHOULD** remain understandable without requiring readers to reconstruct several independent operations or subtle language rules.

Prefer explicit intermediate values when they clarify units, ownership, ordering, or domain meaning.

#### General Requirements

- Avoid excessively nested conditional operators.
- Do not combine unrelated calculations and side effects into a single expression.
- Prefer named intermediate values when they clarify important assumptions or units.
- Avoid introducing intermediate values that merely duplicate obvious expressions.
- Use parentheses to make non-obvious precedence explicit.
- Prefer ordinary statements over complicated expression constructs when side effects or failure behavior are involved.
- Expression refactoring **MUST** preserve evaluation order and numerical behavior where those characteristics are part of the contract.
- Do not assume that algebraically equivalent expressions have identical integer-overflow or floating-point behavior.

**Avoid:**

```cpp
const xiiUInt32 uiResult = bEnabled ? (bReady ? uiPrimary : uiFallback) : (bDefault ? uiDefault : 0U);
```

**Prefer:**

```cpp
xiiUInt32 uiResult = 0U;

if (bEnabled)
{
  uiResult = bReady ? uiPrimary : uiFallback;
}
else if (bDefault)
{
  uiResult = uiDefault;
}
```

The second form makes each decision explicit.

A simple conditional expression remains appropriate when it selects between two straightforward values.

```cpp
const xiiUInt32 uiCapacity = bUseLargeBuffer ? uiLargeCapacity : uiSmallCapacity;
```

Do not replace every conditional operator with a multi-line branch mechanically.

**Guiding principle:**

> Expressions should make calculations and decisions easier to understand, not compress multiple responsibilities into fewer lines.

### 8.13 Determinism and Observable Ordering

Operations whose correctness depends on execution order **MUST** establish that order explicitly.

This applies to scheduling, event processing, serialization, reductions, resource transitions, and other systems where observable results depend on sequencing.

#### General Requirements

- Do not assume iteration order is deterministic unless the container explicitly guarantees it.
- Do not rely on hash-table iteration order for behavior requiring stable ordering.
- Sorting **MUST** establish the required tie-breaking semantics when equivalent keys have observable consequences.
- Concurrent execution **MUST NOT** be assumed deterministic without an explicit synchronization or scheduling contract.
- Side-effect ordering **MUST** remain well-defined when observable behavior depends on it.
- Serialization and persistent formats **MUST** use stable ordering where the format requires reproducible output.
- Do not introduce global ordering requirements when operations are independent and the additional synchronization would be unnecessary.
- Performance optimizations that reorder operations **MUST** preserve required observable semantics.

#### Container Ordering

**Avoid when stable output is required:**

```cpp
for (const auto& entry : m_ResourceMap)
{
  SerializeResource(entry);
}
```

A hash-based container may not provide a stable iteration order.

Where deterministic serialization is required, collect and order the relevant entries according to a defined key before serializing them.

#### Parallel Execution

Concurrent operations may complete in different orders across executions.

When completion order affects externally visible state, the implementation must either enforce a deterministic order or explicitly document that ordering is not guaranteed.

Do not impose determinism on unrelated independent operations unless it is a genuine system requirement.

**Guiding principle:**

> Ordering must be intentional. When behavior depends on sequence, establish that sequence through documented contracts rather than incidental container or scheduling behavior.

## 9. Modern C++23 Features

XII targets C++23 and should take advantage of modern language facilities when they improve correctness, expressiveness, maintainability, or measurable performance.

Modern C++ features **MUST** be adopted deliberately. Language availability alone does not justify introducing a feature when an existing engine abstraction or simpler construct already expresses the intended behavior.

The requirements in this section complement §1.1 (Correctness), §1.5 (Performance), §5 (Type Design), §6 (Ownership and Lifetime), and §7 (Function Interfaces).

### 9.1 Use Modern C++ When It Improves Clarity

C++23 is XII's language baseline, but the availability of a feature in the language standard does not guarantee complete support across all compilers, standard libraries, or target platforms.

#### General Requirements

- Modern C++ features **SHOULD** be preferred when they provide clearer semantics, stronger compile-time guarantees, or simpler implementations.
- Features **MUST** be supported by the minimum compiler and standard-library versions required by XII.
- Do not introduce language features merely because they are newer than an existing implementation.
- Prefer standard language facilities over custom implementations when the standard facility meets the engine's requirements without introducing incompatible dependencies.
- Existing XII abstractions **SHOULD** remain authoritative where they provide engine-specific allocation, reflection, serialization, or platform behavior.
- Do not introduce new generic wrappers around standard language features without a demonstrated requirement.
- Avoid depending on compiler extensions in portable engine code unless those extensions are isolated behind established platform abstractions.
- Changes to compiler or standard-library requirements **MUST** consider every supported platform and build configuration.
- Modernization **MUST NOT** alter observable behavior, ownership semantics, or binary compatibility unintentionally.

#### Feature Availability

Before introducing a less commonly used C++23 feature, consider:

1. Whether every supported compiler accepts the required syntax.
2. Whether the required standard-library implementation is available.
3. Whether the feature interacts correctly with XII's build configurations.
4. Whether it introduces additional dependencies or binary compatibility concerns.
5. Whether the resulting implementation is clearer than the existing alternative.

Feature-test macros **SHOULD** be used when conditional support is necessary and consistent with the repository's platform configuration system.

**Guiding principle:**

> Modern C++ should simplify engineering, not complicate it. Prefer language features that strengthen contracts and reduce unnecessary implementation complexity.

### 9.2 Type Deduction with `auto`

Use `auto` when type deduction improves readability without concealing important semantic information.

Explicit types remain valuable when they communicate precision, ownership, resource identity, or conversion behavior.

#### General Requirements

- Use `auto` when the type is immediately apparent from the initializer.
- Prefer explicit types when the type communicates meaningful domain semantics.
- Use `auto` for iterators, verbose template instantiations, and generic code where spelling the complete type adds little value.
- Do not use `auto` merely to avoid understanding the type returned by an operation.
- Avoid deduced types when unintended signedness, precision, copying, or reference removal could affect correctness.
- Use `auto&` and `const auto&` when reference semantics are intended.
- Avoid unintended copies caused by plain `auto` when the initializer produces a reference.
- Public interfaces **SHOULD** expose concrete return types when those types form part of the documented contract.
- Do not use abbreviated function templates accidentally when a non-template interface is intended.

**Prefer when the type communicates meaning:**

```cpp
const xiiUInt64 uiSimulationStep = simulation.GetStepCount();

xiiUniquePtr<xiiSimulationData> pData = CreateSimulationData();
```

**Prefer when the initializer makes the type obvious:**

```cpp
auto pData = XII_DEFAULT_NEW(xiiSimulationData);

auto it = m_Resources.GetIterator();
```

#### Reference Preservation

Plain `auto` generally deduces a value type rather than preserving the reference category of the initializer.

**Avoid when mutation of the original is intended:**

```cpp
auto entry = m_Entries[uiIndex];

entry.m_bActive = false;
```

**Prefer:**

```cpp
auto& entry = m_Entries[uiIndex];

entry.m_bActive = false;
```

When the type is short and meaningful, an explicit reference may be clearer:

```cpp
xiiResourceEntry& entry = m_Entries[uiIndex];
```

#### `decltype(auto)`

Use `decltype(auto)` only when preserving the exact deduced reference or value category is necessary.

It **SHOULD NOT** be used as a general replacement for `auto`.

A function returning `decltype(auto)` may unintentionally expose references to internal storage or temporary objects if its return expression is not carefully designed.

#### Abbreviated Function Templates

C++20 permits `auto` in function parameters to introduce template parameters implicitly.

```cpp
void ProcessValue(const auto& value)
{
  // Generic implementation.
}
```

This is appropriate for genuinely generic functions, but not when the supported parameter types should be constrained by an explicit interface.

**Guiding principle:**

> Deduce types when their meaning remains obvious. Spell them explicitly when they communicate ownership, precision, identity, or API contracts.

### 9.3 `constexpr`, `consteval`, and `constinit`

Compile-time facilities **SHOULD** be used when they improve correctness, eliminate unnecessary runtime work, or communicate initialization requirements.

Do not introduce elaborate compile-time computations when ordinary constants or runtime initialization provide a simpler solution.

#### `constexpr`

Use `constexpr` for values and functions that are intended to support constant evaluation.

**Prefer:**

```cpp
static constexpr xiiUInt32 MaxWorkerThreads = 64U;
static constexpr xiiUInt32 DefaultAlignment = 16U;
```

Functions may also be declared `constexpr` when they can meaningfully operate in constant-evaluation contexts.

```cpp
constexpr xiiUInt32 AlignUpPowerOfTwo(xiiUInt32 uiValue, xiiUInt32 uiAlignment)
{
  return (uiValue + uiAlignment - 1U) & ~(uiAlignment - 1U);
}
```

This function requires a nonzero power-of-two alignment and arithmetic that does not overflow. Those preconditions must be established by its callers or enforced by an appropriate implementation.

The use of `constexpr` does not automatically guarantee that every invocation is evaluated at compile time.

#### `consteval`

Use `consteval` when an operation **MUST** be evaluated at compile time.

Appropriate applications may include compile-time validation, constant metadata generation, and calculations whose runtime invocation would violate the intended design.

Do not use `consteval` merely because a function happens to support constant evaluation.

#### `constinit`

Use `constinit` for static or thread-local variables that require static initialization without necessarily being immutable.

```cpp
constinit xiiUInt32 s_uiInitializationCount = 0U;
```

`constinit` guarantees static initialization but does not make the variable constant or thread-safe.

#### General Requirements

- Prefer `constexpr` over macros for ordinary typed compile-time constants.
- Use `consteval` only when compile-time evaluation is a required contract.
- Use `constinit` when preventing dynamic initialization is important.
- Do not confuse compile-time initialization with immutable state.
- Compile-time computation **MUST** preserve the same correctness requirements as runtime computation.
- Avoid excessive template instantiation and compile-time work without a demonstrated benefit.
- Do not use compile-time facilities to conceal complex initialization logic.

**Guiding principle:**

> Use compile-time evaluation to enforce meaningful guarantees, not merely to move complexity from runtime into compilation.

### 9.4 Strongly Typed Enums and Engine Enum Wrappers

Enumeration types **SHOULD** express closed sets of discrete values with well-defined semantics.

XII provides established enum wrapper and reflection patterns. These conventions take precedence when types participate in engine reflection, serialization, tooling, or configuration systems.

#### General Requirements

- Prefer strongly typed enum representations over unscoped global constants.
- Follow XII's established enum wrapper conventions when reflection or serialization requires them.
- Enum values **MUST** use meaningful names consistent with their domain.
- Underlying storage types **SHOULD** reflect the required value range and serialization contract.
- Numeric enum values **MUST NOT** be changed casually when used in persistent formats or public binary interfaces.
- Do not assume enum declaration order represents execution order, priority, or dependency ordering.
- Do not introduce closed enum types for concepts that are intentionally extensible through registration.
- Invalid or unknown enum values **MUST** be handled according to the API's contract.
- Enum conversion and validation **SHOULD** use established XII helpers where available.

#### Engine Enum Wrappers

**Illustrative XII-style enum:**

```cpp
struct XII_CORE_DLL xiiTaskExecutionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Immediate = 0U,
    Deferred,
    Parallel,

    ENUM_COUNT,

    Default = Deferred
  };
};
```

This example illustrates the enum declaration structure, not a requirement to introduce this particular type.

The wrapper's associated reflection, conversion, and registration requirements must follow the conventions of the owning module.

#### Scoped Enums

Standard `enum class` is appropriate for internal or standalone types that do not require an established XII enum wrapper.

```cpp
enum class ProcessingState : xiiUInt8
{
  Idle,
  Running,
  Completed
};
```

Do not mechanically convert existing reflected enum wrappers into `enum class` without evaluating serialization, reflection, and compatibility requirements.

#### Enum Exhaustiveness

Switch statements over closed enums **SHOULD** make newly added values visible during maintenance.

When every value requires explicit handling, avoid a silent `default` that conceals missing cases.

See §8.2 for the relevant control-flow requirements.

**Guiding principle:**

> Use enums to represent genuinely closed sets of alternatives. Preserve XII's reflection conventions and do not confuse numeric representation with semantic ordering.

### 9.5 Concepts and Template Constraints

Concepts and `requires` expressions **SHOULD** be used when they express meaningful template requirements and improve compiler diagnostics.

Constraints should clarify what operations or properties a template requires rather than create unnecessary abstraction hierarchies.

#### General Requirements

- Use concepts when they communicate a reusable and meaningful semantic requirement.
- Prefer existing standard or XII concepts when they accurately express the required contract.
- Do not create a new named concept for every isolated type trait or trivial template condition.
- Constraints **SHOULD** reflect the operations and semantics actually required by the implementation.
- Do not use concepts merely to disguise an excessively generic interface.
- Constrained overloads **MUST** avoid unexpected ambiguity.
- Public template constraints **SHOULD** provide understandable diagnostics for unsupported types.
- Include the appropriate standard or engine headers explicitly when using constraint facilities.
- Avoid unnecessary template specialization when an ordinary function overload provides a clearer solution.

**Example:**

```cpp
#include <concepts>

template <typename T>
concept UnsignedInteger = std::unsigned_integral<T>;
```

This is an illustrative standard C++ concept and is not a requirement to duplicate it within XII.

Where the repository already provides suitable traits or constraints, prefer those established facilities.

#### Constrained Functions

```cpp
template <std::unsigned_integral T>
constexpr T CalculateMask(T uiBitIndex)
{
  return T{1} << uiBitIndex;
}
```

The caller must ensure that the shift count is smaller than the bit width of `T`.

A type constraint alone does not establish every runtime precondition of the operation.

#### Semantic Concepts

A concept that merely checks whether a method exists does not automatically prove that an implementation satisfies the method's semantic contract.

For example, a type with a `Reset()` method is not necessarily safe to reset in every execution state.

Document semantic requirements that cannot be enforced through compile-time syntax checks.

**Guiding principle:**

> Concepts should make generic contracts clearer and invalid instantiations easier to diagnose, without introducing speculative template architecture.

### 9.6 Ranges and Views

Standard ranges and views **MAY** be used when they express data transformation or iteration more clearly than ordinary loops.

Lazy evaluation and borrowed storage introduce lifetime and mutation considerations that must be understood before adopting a ranges-based implementation.

#### General Requirements

- Prefer ordinary loops when they express the operation more clearly.
- Use ranges and views when composition, filtering, or transformation materially improves readability.
- Standard ranges operations **MUST** be supported by the repository's required compiler and standard-library implementations.
- Do not retain lazy views beyond the lifetime of their backing storage.
- Container mutation **MUST** respect the iterator and reference validity requirements of the view.
- Avoid returning views that reference temporary or otherwise invalid storage.
- Do not assume a view owns the elements it exposes.
- Ranges-based implementations in performance-critical code **SHOULD** be evaluated for allocation behavior, generated code, and iteration overhead.
- Prefer XII container and view abstractions when interoperability, allocator requirements, or engine conventions make them more appropriate.

#### Simple Iteration

**Prefer:**

```cpp
for (const xiiResourceEntry& entry : m_Entries)
{
  if (!entry.m_bActive)
    continue;

  ProcessEntry(entry);
}
```

This loop is straightforward and does not benefit significantly from introducing a lazy range pipeline.

#### Range Pipelines

A ranges-based implementation may be appropriate when several transformations naturally compose.

```cpp
auto activeValues = values |
  std::views::filter([](const auto& value)
  {
    return value.IsActive();
  });
```

The view does not necessarily own `values`.

Its validity depends on the backing range, iterator stability, and the semantics of the selected adaptors.

#### Lazy Evaluation

Lazy views may execute predicates or transformations when elements are accessed rather than when the view is constructed.

Such operations **SHOULD NOT** contain surprising side effects.

Do not assume that iterating a view multiple times always repeats exactly the same observable behavior when its underlying data may change.

**Guiding principle:**

> Use ranges when they clarify data processing, but preserve explicit control over lifetime, mutation, and evaluation semantics.

### 9.7 Structured Bindings

Structured bindings **SHOULD** be used when they make the components of a tuple-like value or aggregate easier to understand.

They should not conceal important ownership, reference, or lifetime behavior.

#### General Requirements

- Use descriptive binding names that communicate the meaning of each element.
- Prefer `const auto&` when iterating read-only tuple-like elements without copying.
- Use `auto&` when mutation of the original elements is intended.
- Avoid copying large elements inadvertently through plain `auto`.
- Do not use structured bindings merely to shorten code when an existing named type communicates the same information more clearly.
- Structured bindings **MUST** respect the lifetime and invalidation guarantees of their source objects.
- Do not assume every XII container supports standard tuple-like structured binding syntax.

**Illustrative standard C++ example:**

```cpp
for (const auto& [resourceName, resourceHandle] : resources)
{
  ProcessResource(resourceName, resourceHandle);
}
```

This example assumes `resources` exposes suitable tuple-like elements.

For ordinary engine objects with descriptive member names, direct member access may be clearer.

#### Copy Versus Reference

**Avoid when mutation is intended:**

```cpp
auto [key, value] = *iterator;

value = newValue;
```

This may modify a local copy rather than the original stored value.

**Prefer when the underlying tuple-like element permits it:**

```cpp
auto& [key, value] = *iterator;

value = newValue;
```

The exact behavior depends on the type returned by the iterator.

**Guiding principle:**

> Structured bindings should expose the meaning of grouped values without obscuring whether those values are copied or borrowed.

### 9.8 Lambdas, Captures, and Callbacks

Lambdas **SHOULD** be used for localized behavior, callbacks, predicates, and small operations that naturally belong near their point of use.

Capture semantics **MUST** remain explicit when ownership, asynchronous execution, or object lifetime is relevant.

#### General Requirements

- Prefer explicit captures for callbacks that may outlive the current scope.
- Avoid broad reference captures such as `[&]` in asynchronous or long-lived callbacks.
- A captured `this` pointer **MUST** remain valid for every possible invocation.
- Callback registrations **MUST** have a defined invalidation or unregistration mechanism when the event source may outlive the receiver.
- Do not capture stack references in deferred operations unless their lifetime is guaranteed.
- Prefer value or move captures when a callback must own the captured data.
- Do not use shared ownership merely to simplify callback lifetime management when explicit unregistration is sufficient.
- Callback implementations **SHOULD** remain small and cohesive.
- Substantial callback logic **SHOULD** be extracted into an appropriately named member or helper function when doing so improves clarity.
- Lambda return types **SHOULD** be explicit when deduction would obscure an important interface or conversion.

#### Short-Lived Lambdas

**Prefer:**

```cpp
m_Entries.Sort([](const auto& a, const auto& b)
{
  return a.m_iPriority > b.m_iPriority;
});
```

The lambda executes synchronously within the sorting operation and does not retain references beyond its intended scope.

#### Long-Lived Callbacks

```cpp
m_EventHandler = [this](const xiiEventData& event)
{
  HandleEvent(event);
};
```

This is valid only when the callback registration and receiver lifetime are coordinated.

The handler **MUST** be removed or rendered permanently inactive before the receiver is destroyed if the event source could invoke it afterward.

#### Capturing Ownership

For deferred operations that require data to survive the scheduling scope, a move capture can make ownership transfer explicit.

```cpp
xiiString sMessage = "Task completed";

QueueTask([sMessage = std::move(sMessage)]()
{
  LogMessage(sMessage);
});
```

The task system must preserve the callable's lifetime until it is invoked or discarded safely.

#### Avoid Hidden Reference Captures

**Avoid:**

```cpp
void QueueUpdate()
{
  xiiUInt32 uiCounter = 0U;

  QueueTask([&]()
  {
    ++uiCounter;
  });
}
```

The task may execute after the local variable has been destroyed.

This is a lifetime error, not merely a stylistic concern.

**Guiding principle:**

> Lambdas make behavior local, but captures create lifetime dependencies. Every retained callback must have an explicit ownership and invalidation contract.

### 9.9 Move Semantics and Perfect Forwarding

Move semantics **SHOULD** be used when they express meaningful ownership transfer or avoid unnecessary copying.

A move operation must preserve the invariants of both the destination and the moved-from object.

#### General Requirements

- Prefer moving uniquely owned or expensive movable objects when ownership transfer is intended.
- Do not use `std::move` indiscriminately on inexpensive values.
- Do not assume that `std::move` itself performs a move; it is a cast enabling move construction or assignment where supported.
- Do not use `std::move` on a `const` object expecting ordinary mutable move semantics.
- Avoid using an object after it has been moved from unless its documented moved-from state permits the operation.
- Do not return `std::move(local)` mechanically when returning a local object by value.
- Use forwarding references and `std::forward` only when implementing genuine forwarding interfaces.
- Avoid introducing forwarding templates when ordinary overloads or concrete parameter types provide clearer contracts.
- Move operations **MUST NOT** invalidate externally registered identities or addresses unless the type explicitly supports that behavior.

#### Ownership Transfer

**Prefer:**

```cpp
xiiUniquePtr<xiiSimulationData> pData = CreateSimulationData();

m_pSimulationData = std::move(pData);
```

The transfer of exclusive ownership is explicit.

#### Return-Value Optimization

**Prefer:**

```cpp
xiiString BuildResourceName()
{
  xiiString sName = "SimulationResource";
  return sName;
}
```

**Avoid:**

```cpp
xiiString BuildResourceName()
{
  xiiString sName = "SimulationResource";
  return std::move(sName);
}
```

Applying `std::move` to a named local in a return statement can prevent named return-value optimization.

The ordinary return form allows the compiler to perform copy elision where permitted and use move construction where appropriate.

#### Perfect Forwarding

Use `std::forward` when preserving the value category of an argument is an actual requirement of a generic wrapper or constructor.

Do not introduce variadic forwarding constructors without considering overload resolution, accidental conversions, and interactions with copy or move constructors.

**Guiding principle:**

> Move semantics express transfer, not automatic performance. Move deliberately and preserve valid object states.

### 9.10 Casts and Type Conversions

Type conversions **MUST** be explicit when they may alter meaning, precision, representation, or ownership assumptions.

Prefer the narrowest C++ cast that accurately expresses the intended conversion.

#### General Requirements

- Use `static_cast` for well-defined explicit numeric and related conversions.
- Use `const_cast` only when constness changes are explicitly justified and safe.
- Use `reinterpret_cast` only when low-level representation conversion is necessary and valid.
- Avoid C-style casts because they obscure the conversion category.
- Narrowing conversions **MUST** be range-checked when values may exceed the destination range.
- Do not use casts to suppress legitimate type mismatches.
- Prefer XII's established checked casting or type conversion utilities where available.
- Pointer conversions **MUST** respect object lifetime, alignment, and type accessibility.
- Representation-dependent conversions **MUST** consider strict aliasing, object representation, and platform portability.
- Casts **MUST NOT** be used to bypass ownership or const-correctness contracts.

#### Numeric Conversions

**Prefer:**

```cpp
const float fWidth = static_cast<float>(uiWidth);
```

When converting to an integer type with a narrower range, validate that the original value is representable.

Do not assume that an explicit cast makes an otherwise invalid conversion safe.

#### `reinterpret_cast`

Low-level systems code may require reinterpretation of memory representations, especially within platform integration, serialization, or hardware interfaces.

Such casts demand careful consideration of:

- Alignment requirements.
- Strict aliasing rules.
- Object lifetime.
- Pointer provenance and validity.
- Endianness.
- Representation compatibility.
- Bounds and allocation size.

Where byte-level access is intended, prefer well-defined byte-oriented representations and copying facilities rather than pointer punning.

#### `std::bit_cast`

C++20 provides `std::bit_cast` for copying object representations between eligible trivially copyable types of equal size.

```cpp
const xiiUInt32 uiBits = std::bit_cast<xiiUInt32>(fValue);
```

This example assumes the implementation uses a 32-bit `float` and that the target representation is supported.

`std::bit_cast` does not eliminate the need to reason about the validity of the destination representation.

Use it when object-representation conversion is intended, rather than `reinterpret_cast`-based aliasing.

**Guiding principle:**

> A cast should reveal a deliberate conversion. It must never conceal invalid assumptions about representation, lifetime, precision, or ownership.

### 9.11 Standard Library and XII Abstractions

The C++ standard library is available to XII, but engine-level APIs **SHOULD** use established XII abstractions when those abstractions define important architectural contracts.

This is particularly relevant to allocation, memory ownership, reflection, serialization, containers, strings, delegates, and platform interoperability.

#### General Requirements

- Prefer established XII types at shared engine interfaces when they provide required engine semantics.
- Do not introduce parallel abstraction families without a demonstrated need.
- Standard-library facilities **MAY** be used when they are appropriate and compatible with XII's architecture.
- Avoid unnecessary conversion between standard and XII container or string types.
- Do not expose standard-library types across binary interfaces when doing so violates the module's ABI requirements.
- Allocation and destruction **MUST** remain compatible with the relevant ownership model.
- Do not replace established XII types merely because a similar standard-library facility exists.
- Do not recreate standard-library functionality unnecessarily when the existing facility satisfies the requirements.
- Standard-library usage **MUST** respect the repository's required compiler and standard-library versions.
- Include the required standard headers directly rather than relying on the PCH.

#### Common Engine Types

| Responsibility | Established XII abstraction |
|---|---|
| Owning strings | `xiiString` |
| Non-owning string access | `xiiStringView` |
| Dynamic arrays | `xiiDynamicArray` |
| Non-owning contiguous arrays | `xiiArrayPtr` |
| Explicit success/failure | `xiiResult` |
| Callable delegates | `xiiDelegate` |
| Exclusive pointer ownership | `xiiUniquePtr` |
| Shared pointer ownership | `xiiSharedPtr` |
| Resource identity | Appropriate XII handle type |

This table identifies commonly used engine abstractions rather than prohibiting corresponding standard-library types.

#### Interoperability

When integrating external libraries, standard types may be required by their APIs.

Perform conversions at clearly defined boundaries when doing so prevents allocation, ownership, or representation assumptions from leaking through the engine.

Avoid repeated conversions within established hot paths when direct interoperable representations are available.

**Guiding principle:**

> Prefer consistency at engine boundaries and interoperability at integration boundaries. Use the abstraction whose contract matches the requirement.

### 9.12 Coroutines and Asynchronous Code

C++20 coroutines provide a language mechanism for suspending and resuming execution, but do not independently define scheduling, cancellation, thread safety, or resource ownership.

Coroutines **MUST NOT** be introduced into XII without a clearly defined runtime and lifetime model.

#### General Requirements

- Coroutine use **MUST** have a defined executor or resumption policy.
- Suspension and resumption behavior **MUST** be documented.
- Objects referenced across suspension points **MUST** remain valid until their final use.
- Cancellation behavior **MUST** be explicit.
- Error propagation **MUST** follow XII's no-exception policy.
- Coroutine frame ownership and destruction responsibilities **MUST** be defined.
- Shutdown **MUST** account for suspended and outstanding coroutines.
- Do not assume coroutines execute on a single thread unless the executor guarantees it.
- Do not assume a coroutine is thread-safe because its source code appears sequential.
- Avoid introducing coroutine abstractions when the existing task system adequately expresses the required operation.
- Do not retain raw engine pointers across suspension points without enforceable lifetime guarantees.

#### Suspension and Lifetime

A suspension point may allow the current scope's external dependencies to change or disappear before the coroutine resumes.

Borrowed references and pointers remain subject to their original lifetime contracts.

In particular, a coroutine that suspends while holding a mutex, reference into a mutable container, or device-owned resource may require additional coordination.

#### Cancellation

Cancellation does not necessarily imply immediate completion.

The coroutine's owning system **MUST** define whether cancellation:

- Prevents future resumption.
- Requests cooperative termination.
- Causes completion with a failure result.
- Requires explicit draining or destruction of the coroutine frame.

Do not assume that destroying a cancellation token safely destroys all work associated with it.

#### Exception Handling

Coroutine implementations must not rely on exception propagation as their ordinary failure mechanism.

Promise types and coroutine runtimes **MUST** respect the engine's exception policy and supported compiler configurations.

**Guiding principle:**

> Coroutines simplify the expression of asynchronous control flow, but do not remove the need for explicit scheduling, cancellation, ownership, or synchronization contracts.

### 9.13 C++ Modules and Header Integration

XII currently relies on a header-based compilation architecture involving module export macros, precompiled headers, reflection code, generated sources, and plugin interfaces.

C++ modules **MUST NOT** be introduced piecemeal without an architectural and build-system plan.

#### General Requirements

- Do not migrate existing headers to C++ modules solely to adopt newer language features.
- Module adoption **MUST** account for all supported compilers and build systems.
- Generated reflection and serialization code **MUST** remain compatible with the proposed module architecture.
- DLL export/import behavior **MUST** be preserved.
- Precompiled header integration or replacement **MUST** be planned.
- Macro visibility and preprocessing requirements **MUST** be evaluated.
- Plugin and dynamically loaded module compilation **MUST** remain supported.
- Public interface compatibility **MUST** be considered before replacing established header contracts.
- Avoid maintaining duplicate header and module implementations without a clear transition plan.
- C++ module support **MUST** be validated across relevant platform and toolchain configurations before broad adoption.

#### Migration Considerations

A future C++ module strategy should evaluate:

1. Compiler and standard-library module support.
2. Build-system dependency scanning and compilation ordering.
3. Binary module interface artifact management.
4. Generated code and reflection integration.
5. Exported C++ interfaces and dynamic linking.
6. Platform-specific preprocessing.
7. Third-party library compatibility.
8. Incremental build performance.
9. Developer tooling and IDE support.
10. Compatibility with existing plugins and downstream consumers.

C++ modules may improve dependency isolation and compilation performance, but those benefits must be demonstrated within XII's actual build architecture.

**Guiding principle:**

> Language modules are an architectural change, not a formatting change. Adopt them only through a coordinated migration that preserves the engine's existing contracts.

### 9.14 `std::span`, `std::string_view`, and Borrowed Standard Views

Non-owning standard-library views **MAY** be used where they are appropriate, but their lifetime contracts must remain consistent with XII's established borrowing conventions.

The ownership and view lifetime requirements of §6.8 apply equally to standard and engine-specific views.

#### General Requirements

- Prefer XII's established view types at engine interfaces where they are part of the module contract.
- Standard-library views **MUST NOT** outlive their backing storage.
- Do not assume that `std::span` or `std::string_view` owns the referenced memory.
- Avoid returning views into temporary containers or strings.
- Conversions between XII and standard views **MUST** preserve bounds, representation, and lifetime validity.
- Do not introduce repeated view conversions when one representation can be used consistently.
- Mutable spans **MUST** be treated as borrowed mutable access.
- A const-qualified view object is not necessarily a view of const elements.

**Example:**

```cpp
void ProcessSamples(std::span<const float> samples)
{
  for (float fSample : samples)
  {
    ProcessSample(fSample);
  }
}
```

This is a standard-library example. Shared XII interfaces should normally use the established engine array-view convention where appropriate.

**Guiding principle:**

> Views describe access to storage, not ownership of it. Their simplicity must not obscure lifetime or mutability.

### 9.15 `std::optional`, `std::variant`, and Explicit State Representation

Modern vocabulary types can make optional values and alternative states explicit.

They **SHOULD** be used when their semantics fit the problem and their adoption does not conflict with established XII abstractions.

#### Optional Values

Use an optional representation when absence is a meaningful value rather than an error.

For example, an optional configuration setting may legitimately be unspecified.

Do not use optional values to conceal operations that should report failure.

#### Alternatives

Use a variant-like representation when a value may be one of a defined set of alternative types.

A variant is not automatically preferable to a polymorphic interface, tagged engine structure, or existing reflection-compatible representation.

#### General Requirements

- Optional values **MUST** distinguish absence from valid values unambiguously.
- Variant alternatives **SHOULD** represent a meaningful closed set of possibilities.
- Do not introduce optional wrappers when an existing invalid-handle or sentinel contract is clearer and already established.
- Do not use variant types as replacements for extensible runtime registries without considering their closed nature.
- Public vocabulary types **MUST** be compatible with reflection, serialization, and ABI requirements.
- Do not introduce unnecessary allocations or conversions to represent simple state.
- State alternatives **MUST** have documented validity and transition rules.

**Guiding principle:**

> Make absence and alternatives explicit when they are real domain concepts, but avoid introducing additional state representations without a concrete benefit.

### 9.16 Three-Way Comparison and Ordering

C++20 provides the three-way comparison operator (`<=>`) and associated comparison categories.

These facilities **MAY** be used when they simplify well-defined value ordering and equality semantics.

#### General Requirements

- Comparison operators **MUST** reflect the type's semantic identity or ordering contract.
- Do not generate ordering operators automatically when member declaration order does not represent meaningful comparison order.
- Defaulted equality **SHOULD** be considered for simple value-like types when memberwise equality is correct.
- Do not use three-way comparison merely to reduce the number of explicitly written operators.
- Comparison categories **MUST** accurately represent the supported ordering semantics.
- Floating-point comparison **MUST** account for partial ordering caused by NaNs.
- Resource handles and identity-bearing objects **SHOULD** use comparison semantics consistent with their documented identity rules.
- Ordering used by sorting and ordered containers **MUST** satisfy the required algorithmic contracts.

**Example:**

```cpp
struct xiiVersion
{
  xiiUInt32 m_uiMajor = 0U;
  xiiUInt32 m_uiMinor = 0U;
  xiiUInt32 m_uiPatch = 0U;

  auto operator<=>(const xiiVersion&) const = default;
};
```

The defaulted comparison is appropriate if versions are intentionally ordered lexicographically by major, minor, and patch values.

It would be inappropriate if the type required a different domain-specific ordering.

**Guiding principle:**

> Generate comparisons when memberwise semantics are correct. Otherwise, implement the actual domain contract explicitly.

### 9.17 Attributes and Compile-Time Diagnostics

Standard C++ attributes **SHOULD** be used when they communicate meaningful intent or enable useful diagnostics.

They must not replace the underlying correctness requirements.

#### Common Attributes

| Attribute | Intended use |
|---|---|
| `[[nodiscard]]` | Warn when an important return value is discarded |
| `[[maybe_unused]]` | Mark intentionally unused declarations |
| `[[deprecated]]` | Identify APIs scheduled for replacement or removal |
| `[[fallthrough]]` | Document intentional switch fallthrough |
| `[[likely]]` | Express an expected branch probability |
| `[[unlikely]]` | Express an expected uncommon branch |

#### General Requirements

- Use `[[nodiscard]]` according to §7.6.
- Use `[[fallthrough]]` according to §8.9.
- Do not introduce `[[maybe_unused]]` merely to conceal obsolete code or unused dependencies.
- Deprecation attributes **SHOULD** provide a useful migration message where supported.
- Branch-likelihood attributes **SHOULD** be reserved for cases supported by representative profiling or established workload knowledge.
- Do not use `[[likely]]` or `[[unlikely]]` as substitutes for profiling.
- Attributes **MUST** remain compatible with the repository's required compilers.
- Do not stack attributes mechanically when they provide no practical benefit.

#### Branch Prediction

**Example:**

```cpp
if (bUnexpectedFailure) [[unlikely]]
{
  HandleFailure();
}
```

This syntax communicates an expected uncommon branch.

However, compiler behavior varies, and branch-likelihood hints are not guaranteed to improve performance.

Use them only when their assumptions are justified and their effect is appropriate for the relevant workload.

**Guiding principle:**

> Attributes should communicate meaningful contracts or evidence-based assumptions, not decorate code without purpose.

### 9.18 Compile-Time Reflection and Metaprogramming Boundaries

Modern C++ template facilities can generate types, select implementations, and enforce compile-time constraints.

These capabilities **SHOULD** remain focused on requirements that genuinely benefit from compile-time reasoning.

#### General Requirements

- Prefer ordinary C++ constructs when they express the same behavior more clearly.
- Do not introduce complex template metaprogramming solely to eliminate small amounts of duplication.
- Compile-time mechanisms **SHOULD** provide understandable diagnostics for invalid use.
- Avoid unnecessary instantiation depth and excessive compile-time computation.
- Template-generated interfaces **MUST** preserve the intended ownership, lifetime, and type contracts.
- Compile-time metadata **MUST NOT** be confused with XII's established runtime reflection system.
- Do not replace engine reflection or serialization registration with unrelated template machinery without a coordinated architectural change.
- Metaprogramming abstractions **SHOULD** have concrete consumers and demonstrate a reduction in overall complexity.

#### Compile-Time Versus Runtime Reflection

XII's runtime reflection and registration mechanisms may support capabilities that ordinary C++ compile-time type inspection does not provide, including dynamic discovery, serialization, tooling, and plugin interoperability.

Compile-time traits and concepts may complement these systems but do not automatically replace them.

**Guiding principle:**

> Compile-time abstraction is justified when it strengthens contracts or eliminates meaningful runtime complexity. It should not become a parallel architecture without a demonstrated need.

## 10. Memory, Allocators, and Containers

Memory management in XII **MUST** be explicit, predictable, and appropriate for the lifetime, access patterns, and performance requirements of the owning subsystem.

XII provides specialized containers, allocators, ownership abstractions, and resource-management facilities. These should be selected according to their actual semantics rather than convenience, familiarity, or speculative performance assumptions.

This section extends §1.2 (Ownership and Lifetime), §1.5 (Optimization), §6 (Initialization, Ownership, and Lifetime), and §8 (Control Flow and Expressions).

### 10.1 Choose Storage According to Access Patterns

Container selection **SHOULD** be driven by the operations performed most frequently and the guarantees required by consumers.

No single container is optimal for every workload. Consider access complexity, iteration patterns, memory locality, allocation behavior, reference stability, and modification frequency.

#### General Requirements

- Prefer contiguous storage when sequential iteration, indexed access, and cache locality dominate.
- Prefer associative containers when lookup by key is a primary operation.
- Use handle-based storage when independently managed identity and stale-reference detection are required.
- Use stable-address storage when external references must survive container growth or relocation.
- Do not introduce multiple authoritative representations of the same data without a clear consistency and ownership model.
- Container selection **MUST** account for insertion, removal, relocation, and invalidation semantics.
- Avoid choosing containers solely by asymptotic complexity without considering practical workload characteristics.
- Prefer established XII containers over introducing incompatible storage abstractions without a demonstrated need.
- Changes to container representation **MUST** preserve the documented interface and lifetime guarantees.

| Requirement | Typical approach | Important consideration |
|---|---|---|
| Frequent sequential iteration | `xiiDynamicArray<T>` | Growth and removal may relocate elements |
| Fixed-size storage | `xiiStaticArray<T, N>` or appropriate fixed-capacity type | Capacity and overflow behavior must be understood |
| Small collections with occasional growth | Suitable XII hybrid-array type | Verify inline capacity and allocator behavior |
| Lookup by stable logical identity | `xiiIdTable` | Handle validity does not automatically guarantee concurrent lifetime safety |
| Hash-based lookup | `xiiHashTable` | Hashing, equality, memory overhead, and iteration order matter |
| Ordered associative access | Appropriate ordered container | Ordering requirements and insertion costs must be justified |
| Stable heap allocation | `xiiUniquePtr<T>` stored in a collection | Address remains stable only while the allocation exists |
| Non-owning contiguous access | `xiiArrayPtr<T>` | Backing storage must remain valid |
| Temporary allocations | Appropriate arena or frame allocator | Allocations must not outlive allocator reset |
| Independently managed resources | Registry with generation-checked handles | Ownership and synchronization remain separate concerns |

These are typical approaches, not mandatory mappings. The selected container must satisfy the actual subsystem contract.

#### Access Pattern Considerations

Before selecting a container, consider:

1. **Iteration:** Is the collection frequently traversed sequentially?
2. **Lookup:** Are elements located by index, identifier, key, or predicate?
3. **Mutation:** How frequently are elements inserted, removed, or reordered?
4. **Identity:** Must references remain valid independently of storage position?
5. **Ordering:** Is element order meaningful or externally observable?
6. **Memory:** What allocation, fragmentation, and locality characteristics matter?
7. **Concurrency:** Are operations performed concurrently, and what synchronization is required?
8. **Scale:** What are the representative and worst-case element counts?

Do not replace a straightforward container with a complex indexing structure when measurements do not justify the additional state and maintenance cost.

**Guiding principle:**

> Select containers according to the operations and guarantees they must support, not merely their familiarity or theoretical complexity.

### 10.2 Allocation on Performance-Critical Paths

Dynamic allocation can introduce latency, fragmentation, synchronization, and unpredictable execution costs.

Allocation behavior **SHOULD** be considered when designing frequently executed operations, particularly simulation updates, resource processing, rendering, task scheduling, and other latency-sensitive workloads.

#### General Requirements

- Avoid unnecessary heap allocations in demonstrated hot paths.
- Prefer retaining and reusing storage when its lifetime and memory cost justify doing so.
- Use pooled or arena-based allocation when allocation patterns and lifetimes are compatible with those mechanisms.
- Batch allocations when doing so reduces measurable overhead without obscuring ownership.
- Do not introduce custom allocators solely because an object is performance-sensitive.
- Allocation optimizations **SHOULD** be supported by profiling or representative allocation measurements.
- Avoid hidden allocations in frequently invoked interfaces when their cost is significant.
- Temporary storage **MUST** remain valid for the entire duration of its use.
- Memory reuse **MUST NOT** expose stale object state or violate construction and destruction requirements.

#### Reusing Storage

**Prefer when repeated allocation is measurable:**

```cpp
void CollectVisibleObjects(xiiArrayPtr<const xiiObject> pObjects)
{
  m_VisibleObjects.Clear();

  for (const xiiObject* pObject : pObjects)
  {
    if (pObject->IsVisible())
    {
      m_VisibleObjects.PushBack(pObject->GetHandle());
    }
  }
}
```

The collection retains its allocated capacity when `Clear()` preserves storage according to the container's contract.

This can avoid repeated allocation across successive calls.

The retained capacity also consumes memory between uses. Reuse is appropriate only when that trade-off is justified.

#### Allocation Strategies

Common allocation strategies include:

- **Stack allocation:** Appropriate for bounded, scope-local objects.
- **General-purpose heap allocation:** Appropriate for independently managed objects with less predictable lifetimes.
- **Pool allocation:** Appropriate for frequently created objects with compatible allocation sizes or reuse characteristics.
- **Arena allocation:** Appropriate for groups of allocations sharing a common lifetime.
- **Frame allocation:** Appropriate for temporary data guaranteed not to survive the frame allocator's reset boundary.
- **Persistent reserved storage:** Appropriate for collections that repeatedly operate near a known capacity.

These strategies are not interchangeable.

For example, frame allocation is unsafe when the allocated data is retained by an asynchronous task or GPU submission beyond the frame allocator's validity period.

#### Allocation in Real-Time Systems

Performance-sensitive systems **SHOULD** consider not only average allocation time but also latency variation and contention.

An allocator that provides good average throughput may still introduce unacceptable stalls under certain workloads.

Where predictable execution latency is required, measure allocation behavior under representative contention, object sizes, and allocation lifetimes.

**Guiding principle:**

> Reduce allocation overhead where it matters, but do not introduce complex memory-management machinery without evidence that it improves the intended workload.

### 10.3 Capacity, Reservation, and Container Growth

Container capacity and element count represent different properties and **MUST NOT** be treated interchangeably.

Capacity describes how much storage may be available without additional allocation. Count describes the number of logically contained elements.

#### General Requirements

- Reserve capacity when expected element counts are known and avoiding growth is beneficial.
- Do not reserve excessive memory without considering actual workload requirements.
- Avoid repeatedly reserving the same capacity in frequently executed operations.
- Do not assume that reserved capacity represents initialized or constructed elements.
- Container access **MUST** remain within the valid element count.
- Growth policies **SHOULD** balance allocation frequency against retained memory consumption.
- Capacity reductions **SHOULD** be intentional, particularly for repeatedly reused containers.
- Do not assume that clearing a container releases its allocation unless documented by the container.
- Reservation and growth operations **MUST** account for reference and pointer invalidation.

**Prefer:**

```cpp
m_VisibleObjects.Reserve(uiEstimatedVisibleObjectCount);

for (const xiiObject* pObject : pObjects)
{
  if (pObject->IsVisible())
  {
    m_VisibleObjects.PushBack(pObject->GetHandle());
  }
}
```

Reservation is beneficial when the estimate is meaningful and the container would otherwise require repeated growth.

#### Count Versus Capacity

**Avoid:**

```cpp
m_Entries.Reserve(128U);

ProcessEntry(m_Entries[127]);
```

Reservation alone does not establish that the collection contains 128 valid elements.

**Prefer:**

```cpp
m_Entries.SetCount(128U);

ProcessEntry(m_Entries[127]);
```

The preferred example assumes `SetCount()` establishes the required elements according to the container's documented construction and initialization semantics.

#### Repeated Reservation

Calling `Reserve()` repeatedly may be harmless when the requested capacity is already available, but unnecessary calls can obscure the intended allocation strategy.

Prefer reserving storage at initialization or another appropriate capacity-planning boundary when the expected requirement is stable.

**Guiding principle:**

> Capacity is an allocation property; count is an element-validity property. Reserve intentionally and never confuse available storage with constructed objects.

### 10.4 Pointer, Reference, and Iterator Invalidation

Container modifications may invalidate pointers, references, iterators, views, and indices into their elements.

Such invalidation **MUST** be considered whenever references to container-managed storage escape the immediate operation.

#### General Requirements

- Do not assume elements retain stable addresses across container growth.
- Verify the invalidation guarantees of the specific XII container being used.
- Do not retain pointers into relocatable storage across operations that may trigger relocation.
- Element removal **MUST** invalidate access to the removed object.
- Iteration **MUST** respect container mutation and invalidation rules.
- Stable addresses **MUST NOT** be confused with stable logical identity.
- Avoid unnecessary indirection when stable addresses are not required.
- Use handles when logical identity must remain independent of storage location.
- Generation-checked handles **MUST** be validated through their authoritative registry.
- Container relocation **MUST NOT** invalidate externally registered addresses when those addresses are contractually required to remain stable.

#### Relocation Hazards

**Avoid:**

```cpp
xiiDynamicArray<Entry> m_Entries;

Entry* pEntry = &m_Entries[0];

m_Entries.PushBack(anotherEntry);

// pEntry may now refer to invalid storage.
ProcessEntry(*pEntry);
```

Appending an element may require the dynamic array to relocate its existing elements.

The retained pointer may therefore become invalid.

#### Stable Allocation

When stable element addresses are genuinely required, independently allocated objects may be appropriate.

```cpp
xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;
```

Relocating the container's smart pointers does not relocate the independently allocated `Entry` objects.

However, removing or destroying an entry still invalidates pointers to that entry.

#### Index-Based References

Indices may survive relocation when logical ordering remains unchanged, but they can become invalid or refer to different elements after removal or reordering.

Do not treat an index as a persistent resource identity unless the owning subsystem explicitly guarantees that behavior.

For independently managed objects, generation-checked handles are generally more appropriate.

See §6.4 for the distinction between stable identity and stable address.

**Guiding principle:**

> Container stability is defined by documented invalidation rules. Never infer pointer, index, or iterator validity from the container's apparent implementation.

### 10.5 Removal, Ordering, and Compaction

Removal operations **MUST** preserve the ordering and identity guarantees required by the collection's consumers.

Different removal strategies offer different performance and semantic trade-offs.

#### General Requirements

- Use unordered removal only when element order is not part of the collection's contract.
- Ordered removal **MUST** be used when the relative order of surviving elements must be preserved.
- Removal **MUST** account for pointer, reference, iterator, and index invalidation.
- Do not optimize away deterministic ordering when it affects observable behavior.
- Use compaction when it provides a meaningful benefit and does not invalidate required external references.
- Avoid unnecessary stable ordering when no consumer depends on it.
- Removal during iteration **MUST** follow the container's documented mutation rules.
- Secondary lookup structures **MUST** remain consistent after removal or relocation.

#### Unordered Removal

When order is irrelevant, swap-based removal can reduce the number of elements that must be moved.

```cpp
for (xiiUInt32 uiIndex = 0; uiIndex < m_Entries.GetCount();)
{
  if (m_Entries[uiIndex].m_bExpired)
  {
    m_Entries.RemoveAtAndSwap(uiIndex);
    continue;
  }

  ++uiIndex;
}
```

The element moved into the removed position must still be examined.

#### Ordered Removal

Ordered removal preserves relative ordering but may require shifting subsequent elements.

It is appropriate when order affects:

- Execution sequencing.
- Scheduling.
- Serialization.
- Deterministic iteration.
- User-visible ordering.
- Stable relative ordering required by tests or algorithms.

Do not replace ordered removal with swap-based removal without evaluating these requirements.

#### Compaction

Compaction may improve locality and reduce fragmentation, but it can also invalidate addresses and indices.

A system that supports compaction **MUST** distinguish logical identity from physical location when external references must survive relocation.

**Guiding principle:**

> Removal semantics are part of the collection's contract. Optimize movement costs only when ordering and identity requirements permit it.

### 10.6 Allocator Ownership and Lifetime

An allocator **MUST** remain valid for every allocation that depends on it.

Allocator lifetime is a dependency relationship and must be preserved through initialization, ordinary execution, failure recovery, and shutdown.

The general allocator compatibility requirements are established in §6.2.

#### General Requirements

- Allocators **MUST** outlive the allocations and containers that rely on them.
- Objects **MUST NOT** release memory through an allocator that has already been destroyed.
- Allocator reset operations **MUST NOT** invalidate allocations that remain in use.
- Allocator ownership **SHOULD** be explicit when a component selects or manages its own allocator.
- Member declaration order **MUST** account for reverse-order destruction when members depend on an allocator owned by the same object.
- Explicit shutdown operations **MUST** release dependent resources before destroying their allocator.
- Plugin and module-specific allocators **MUST** remain available until all allocations requiring them have been released.
- Arena and frame allocator boundaries **MUST** account for deferred execution and asynchronous resource use.

#### Member Destruction Order

C++ destroys non-static data members in the reverse order of their declaration.

**Prefer:**

```cpp
class xiiStorageManager
{
private:
  xiiProxyAllocator            m_Allocator;
  xiiDynamicArray<xiiUInt32>   m_Entries;
};
```

This arrangement is appropriate when `m_Entries` uses `m_Allocator`, assuming the allocator is correctly supplied during construction.

Because `m_Entries` is declared after `m_Allocator`, it is destroyed first.

The allocator remains alive throughout the container's destruction.

#### Explicit Shutdown

```cpp
void xiiStorageManager::Shutdown()
{
  m_Entries.Clear();
}
```

Clearing entries may destroy contained objects, but whether it releases the container's backing allocation depends on the container contract.

If allocator teardown requires all backing allocations to be released before the manager itself is destroyed, the implementation must use the appropriate storage-release operation or lifecycle design.

Do not assume `Clear()` necessarily returns memory to the allocator.

#### Arena Reset

An arena allocator may invalidate many allocations simultaneously.

Resetting an arena is appropriate only when every object using its storage has completed its required lifetime.

For non-trivially destructible objects, the implementation must also ensure that required destruction occurs according to the allocator and container contracts.

**Guiding principle:**

> An allocator is part of the lifetime of every allocation it serves. Destruction order, reset behavior, and shutdown must respect that dependency.

### 10.7 Ownership, Caching, Pooling, and Aliasing

Ownership, caching, pooling, and memory aliasing are distinct concepts and **MUST NOT** be conflated.

A subsystem may implement several of these mechanisms simultaneously, but each must have a clearly defined responsibility and lifetime contract.

#### Definitions

| Concept | Responsibility |
|---|---|
| **Ownership** | Determines which component is responsible for releasing a resource |
| **Caching** | Retains reusable data or resources to avoid repeated computation or creation |
| **Pooling** | Recycles compatible objects or allocations according to defined reuse rules |
| **Aliasing** | Permits separate logical resources to share physical memory when their usage constraints allow it |
| **Registration** | Associates an object with a lookup, discovery, or identity mechanism |
| **Residency** | Describes whether a resource or allocation is present in a required memory domain |

#### General Requirements

- Resource ownership **MUST** remain explicit regardless of whether the resource is cached or pooled.
- A cache **MUST** define when entries become valid, stale, reusable, and eligible for eviction.
- A pool **MUST** define compatibility requirements for reusing an allocation or object.
- Memory aliasing **MUST** respect the lifetime and synchronization constraints of the underlying memory system.
- Registration **MUST NOT** be assumed to transfer ownership unless the API explicitly establishes that behavior.
- Caches **MUST NOT** silently become authoritative owners of unrelated subsystem state without an explicit contract.
- Do not introduce caches or pools without a demonstrated benefit.
- Cached resources **MUST** be invalidated when the assumptions underlying their correctness no longer hold.
- Resource eviction **MUST** respect outstanding references and asynchronous operations.
- The distinction between logical resource identity and physical allocation reuse **MUST** remain clear.

#### Cache Validity

A cached result may depend on:

- Resource contents.
- Configuration values.
- Platform capabilities.
- Compilation settings.
- Device state.
- Version identifiers.
- Other external dependencies.

The cache must define which changes invalidate its entries.

A cache that does not correctly represent its invalidation dependencies may return stale results even when its memory management is otherwise correct.

#### Pool Reuse

A pool may reuse memory without reusing an object's logical identity.

When physical allocations are recycled, new objects **MUST NOT** accidentally inherit stale state, references, or generation identifiers from previous occupants.

For GPU resources, reuse must also respect outstanding GPU execution dependencies.

#### Resource Aliasing

Aliasing permits multiple logical resources to use overlapping physical storage under defined conditions.

It is valid only when their memory usage, synchronization, and API requirements permit that overlap.

Two resources having non-overlapping apparent application lifetimes does not automatically prove that their GPU usage does not overlap.

**Guiding principle:**

> Ownership determines responsibility, caching determines retention, pooling determines reuse, and aliasing determines memory sharing. Each mechanism requires its own correctness contract.

### 10.8 Memory Alignment, Layout, and Representation

Memory layout and alignment **MUST** satisfy the requirements of the C++ object model and any external interface that consumes the data.

This is particularly important for SIMD operations, atomic types, binary serialization, GPU resources, and platform API structures.

#### General Requirements

- Allocations **MUST** satisfy the alignment requirements of the objects stored within them.
- Use XII's established alignment helpers and allocation facilities where appropriate.
- Do not assume compiler-generated structure padding is stable across different toolchains or build configurations.
- Do not rely on undocumented `sizeof()` equivalence between unrelated types.
- Binary data exchange **MUST** account for representation, alignment, field layout, and endianness where applicable.
- Do not reinterpret arbitrary byte sequences as C++ objects without satisfying object lifetime, alignment, and representation requirements.
- SIMD storage **MUST** meet the alignment requirements of the selected operations.
- Atomics **MUST** be correctly aligned and used according to their synchronization contract.
- Packed structures **SHOULD** be avoided unless required by an explicit external format or platform interface.
- Explicit padding **MAY** be used when a binary layout contract requires it.

#### Alignment

**Example:**

```cpp
struct alignas(16) xiiAlignedVectorData
{
  float m_fValues[4] = {};
};
```

The explicit alignment establishes a minimum alignment requirement for the type.

Such alignment should be introduced only when it is necessary for the intended operations or external layout contract.

#### Serialization

Do not serialize arbitrary object memory directly merely because a type appears to contain simple fields.

C++ object representations may contain:

- Padding bytes.
- Platform-dependent layouts.
- Pointer values.
- Endianness-dependent representations.
- Non-portable implementation details.

Persistent formats **SHOULD** define their field representations explicitly.

#### GPU Data Layout

GPU-facing data structures must satisfy the layout rules of the relevant graphics API and shader interface.

CPU structure size, member offsets, and alignment must not be assumed compatible with shader data layouts without verification.

Where necessary, validate layout assumptions using compile-time checks and established reflection or shader interface mechanisms.

**Guiding principle:**

> Memory layout is a correctness contract whenever data crosses representation boundaries. Alignment, padding, and object lifetime must be established explicitly.

### 10.9 Data-Oriented Access and Locality

Data-oriented design **SHOULD** be considered when a subsystem repeatedly processes large collections of objects or performs homogeneous operations over substantial datasets.

The goal is to organize data according to how it is accessed, not to replace every object-oriented abstraction with arrays.

#### General Requirements

- Prefer storage layouts that support the dominant access patterns of the subsystem.
- Avoid repeatedly scanning large heterogeneous collections when narrower indexing or categorization would provide a demonstrated benefit.
- Group frequently accessed data when doing so improves locality and reduces unnecessary memory traffic.
- Separate rarely accessed data from hot-path data when measurements justify the additional organization.
- Do not duplicate authoritative state across multiple representations without a defined synchronization or invalidation strategy.
- Secondary indices **MUST** remain consistent with the owning data.
- Data layout changes **MUST** preserve identity, ownership, and ordering requirements.
- Avoid introducing complex data-oriented structures for small collections where straightforward storage is sufficient.
- Evaluate data-oriented optimizations using representative workloads.

#### Array of Structures

An array of structures can be appropriate when operations frequently access most fields of each element.

```cpp
struct xiiParticle
{
  xiiVec3 m_vPosition;
  xiiVec3 m_vVelocity;
  float   m_fMass = 1.0f;
};

xiiDynamicArray<xiiParticle> m_Particles;
```

This representation keeps each particle's data together.

It may be suitable when updating position, velocity, and mass together.

#### Structure of Arrays

A structure-of-arrays representation may be preferable when operations repeatedly access only selected fields across many elements.

```cpp
struct xiiParticleStorage
{
  xiiDynamicArray<xiiVec3> m_Positions;
  xiiDynamicArray<xiiVec3> m_Velocities;
  xiiDynamicArray<float>   m_Masses;
};
```

This layout may improve locality for operations that process positions independently of other particle properties.

However, it introduces an invariant: corresponding array elements must remain aligned by index.

Insertion, removal, resizing, and reordering **MUST** preserve that relationship.

#### Secondary Indices

A subsystem may maintain a collection of handles or indices identifying a frequently accessed subset of objects.

```cpp
xiiDynamicArray<xiiObjectHandle> m_ActiveObjects;
```

Such indices can reduce repeated filtering costs, but they introduce additional state that must be maintained when objects change eligibility or are destroyed.

Secondary indices **SHOULD** be introduced only when their benefits justify those maintenance requirements.

#### Performance Considerations

Data-oriented representations may improve:

- Cache locality.
- Sequential memory access.
- SIMD utilization.
- Batch processing.
- Parallel work partitioning.
- Reduced pointer chasing.

They may also increase:

- State synchronization complexity.
- Update and removal costs.
- Memory duplication.
- Interface complexity.
- Difficulty preserving independent object identity.

The appropriate representation depends on the workload.

**Guiding principle:**

> Organize data for the operations that consume it. Optimize locality where it matters without sacrificing ownership, consistency, or architectural clarity.

### 10.10 Smart-Pointer Ownership Cycles

Shared ownership graphs **MUST** avoid cycles that prevent resources from being released.

This is particularly important for callbacks, editor services, plugin systems, asynchronous tasks, and resource managers.

#### General Requirements

- Shared ownership **MUST** reflect genuine lifetime responsibility.
- Do not introduce mutual strong ownership when one relationship is logically non-owning.
- Ownership graphs **SHOULD** have an identifiable direction or authoritative owner.
- Use weak references, non-owning pointers, or manager handles where appropriate to break ownership cycles.
- A non-owning reference **MUST** still have a valid lifetime and invalidation contract.
- Callback registrations **MUST** be released before their owning objects or executable code become invalid.
- Do not assume reference counting automatically prevents resource leaks.
- Shared ownership across plugin boundaries **MUST** account for module unloading and destruction requirements.

#### Ownership Cycles

**Avoid conceptually:**

```cpp
class xiiServiceA
{
  xiiSharedPtr<xiiServiceB> m_pServiceB;
};

class xiiServiceB
{
  xiiSharedPtr<xiiServiceA> m_pServiceA;
};
```

If both objects hold strong ownership of each other, their reference counts may never reach zero.

#### Prefer Directed Ownership

When one service owns another, express that relationship directly.

```cpp
class xiiServiceA
{
private:
  xiiUniquePtr<xiiServiceB> m_pServiceB;
};
```

The owned service may retain a non-owning reference to its owner only when that reference is required and its lifetime is guaranteed.

Where neither service should own the other, an authoritative manager or handle-based lookup may provide a clearer relationship.

#### Plugin and Callback Cycles

Callbacks can create indirect ownership cycles when they capture strong references to objects that own the callback registration.

A callback's lifetime must therefore be reviewed together with its capture semantics and registration owner.

**Guiding principle:**

> Shared references are not automatically shared responsibility. Make ownership directional wherever practical, and prevent cycles that obstruct deterministic destruction.

### 10.11 Memory Budgets and Resource Accounting

Systems that manage substantial or variable memory consumption **SHOULD** define appropriate memory accounting and capacity policies.

This is particularly relevant to caches, streaming systems, asset processing, simulations, and GPU resource management.

#### General Requirements

- Memory budgets **SHOULD** be explicit when a subsystem may otherwise grow without meaningful bounds.
- Resource accounting **MUST** distinguish logical resource sizes from actual physical allocation costs when the distinction affects correctness or budgeting.
- Do not assume that container element counts accurately represent total memory usage.
- Cached and pooled allocations **SHOULD** be included in memory accounting when they remain resident.
- Eviction and reclamation strategies **MUST** respect outstanding references and asynchronous execution.
- Budget enforcement **MUST** preserve valid resource states when allocation or reclamation fails.
- Avoid introducing complex budget-management systems when the subsystem's memory requirements are small and predictable.
- Memory accounting **SHOULD** be validated against allocator or platform measurements where practical.

#### Logical Versus Physical Memory

A resource's logical data size may differ from its actual allocation size due to alignment, metadata, allocator granularity, padding, and internal storage requirements.

For example, a texture's apparent pixel-data size may not equal the physical memory reserved by a graphics backend.

Budgets must use measurements appropriate to the resource-management decisions they control.

#### Cache Growth

A cache that retains every previously used resource without an eviction policy may exhibit unbounded memory growth.

Caches **SHOULD** establish an appropriate retention policy when resource volume is not naturally bounded.

Possible strategies include explicit capacity limits, memory budgets, age-based retirement, or reclamation under pressure.

The selection should follow actual workload requirements rather than a universal policy.

**Guiding principle:**

> Memory retained for performance remains a resource cost. Budgeting and accounting should reflect physical ownership and realistic workload behavior.

### 10.12 Memory Diagnostics and Validation

Memory correctness and performance **SHOULD** be verified using appropriate diagnostics, tests, and profiling tools.

The objective is to detect lifetime violations, allocation mismatches, invalidation errors, leaks, and performance regressions before they become difficult to reproduce.

#### General Requirements

- Allocation and destruction paths **SHOULD** be tested under normal execution and recoverable failure conditions.
- Resource-owning containers **MUST** preserve their invariants during insertion, growth, removal, and clearing.
- Memory diagnostics **SHOULD** cover allocator mismatch, double destruction, invalid access, and leaked ownership where applicable.
- Use supported sanitizers and diagnostic tools when they are compatible with the relevant platform and build configuration.
- Do not assume that successful execution without a crash proves memory correctness.
- Profile allocation count, allocated bytes, retained capacity, and fragmentation where those measurements matter.
- Validate memory behavior under representative workloads rather than relying exclusively on small synthetic tests.
- GPU memory diagnostics **SHOULD** account for asynchronous execution and deferred destruction.
- Tests **SHOULD** exercise subsystem shutdown and partial initialization failure paths.
- Performance improvements **SHOULD** include measurements demonstrating the intended reduction in allocation or memory overhead.

#### Development Assertions

Assertions may detect incorrect indices, unexpected allocation states, or violated container invariants during development.

However, assertions **MUST NOT** perform essential allocation, destruction, or cleanup work.

Expected allocation failures **MUST** follow the subsystem's explicit error-handling contract.

#### Sanitizers and Instrumentation

Where supported, memory diagnostics may include:

- AddressSanitizer for invalid memory accesses and lifetime errors.
- UndefinedBehaviorSanitizer for supported categories of undefined behavior.
- Leak detection for unreleased allocations.
- Heap profiling for allocation frequency and retained memory.
- Platform memory analyzers for fragmentation and allocation behavior.
- Graphics API validation and GPU-assisted diagnostics where available.

These tools provide complementary evidence. No single tool can establish every ownership or lifetime guarantee.

**Guiding principle:**

> Memory correctness must be demonstrated across allocation, mutation, failure, and destruction. Performance claims should be supported by measurements rather than assumptions.


## 11. Strings, Identifiers, Hashes, and Handles

Strings, identifiers, hashes, and handles serve distinct purposes in XII. Their representations **MUST** reflect the intended semantics of ownership, identity, comparison, lookup, and lifetime.

Choosing an appropriate representation is important for correctness, performance, and interoperability across engine modules. In particular, textual names must not be confused with object identity, hash values must not be assumed collision-free, and resource handles must not be treated as owning pointers.

This section extends §1.2 (Ownership and Lifetime), §3 (Naming Conventions), §6 (Resource Lifetime), and §10 (Memory and Containers).

### 11.1 Use the Appropriate String Representation

XII provides several string abstractions with different ownership, construction, comparison, and storage characteristics.

The selected string type **SHOULD** reflect how the text is produced, stored, and consumed.

| Type | Typical responsibility | Important consideration |
|---|---|---|
| `xiiString` | Owned string storage | Suitable when text must remain valid independently of its source |
| `xiiStringBuilder` | Mutable construction and manipulation | Appropriate for repeated formatting, concatenation, and modification |
| `xiiStringView` | Non-owning string access | Backing storage must remain valid |
| `xiiHashedString` | Persistent names with cached hashing | Useful for repeated comparisons and lookups |
| `xiiTempHashedString` | Temporary hashed lookup values | Appropriate when the receiving API supports compatible hashed lookup |
| `const char*` | C-compatible string interfaces | Requires explicit null-termination and lifetime assumptions |

#### General Requirements

- Prefer `xiiStringView` for read-only string inputs when ownership transfer is unnecessary.
- Use `xiiString` when a component must retain text independently of the caller.
- Use `xiiStringBuilder` when constructing or modifying text repeatedly.
- Use `xiiHashedString` when cached hashing provides a meaningful benefit for a persistent name.
- Use `xiiTempHashedString` for temporary lookup operations when supported by the target container or API.
- Do not store non-owning string views without establishing the backing storage's lifetime.
- Do not repeatedly compute hashes for stable identities in demonstrated hot paths when a suitable cached representation is available.
- Do not intern or persistently hash every temporary diagnostic string without a concrete requirement.
- Avoid unnecessary conversions among string types, particularly in frequently executed operations.
- String representations **MUST** preserve the encoding and comparison semantics required by their consumers.
- Do not assume that all string representations guarantee NUL termination.

#### Borrowed String Inputs

**Prefer:**

```cpp
xiiResult LoadDocument(xiiStringView sPath);
```

This permits callers to supply compatible string storage without requiring ownership transfer at the function boundary.

If the implementation needs to retain the path beyond the call, it must copy or otherwise safely acquire the required storage.

#### String Construction

**Prefer:**

```cpp
xiiStringBuilder sResourcePath;

sResourcePath.Append(sDirectory);
sResourcePath.AppendPath(sResourceName);
```

This is appropriate when a mutable string is being assembled from several components and the path operations match the filesystem abstraction's requirements.

Do not introduce a builder when a simple string value or view already satisfies the operation.

#### Hashed String Lookup

**Example:**

```cpp
xiiHashedString m_sResourceName;

xiiUInt32 uiResourceIndex = 0U;

if (!m_ResourceLookup.TryGetValue(xiiTempHashedString(sName), uiResourceIndex))
{
  return XII_FAILURE;
}
```

The temporary hashed representation avoids unnecessarily constructing a persistent hashed-string object for an immediate lookup, assuming the table's key and heterogeneous lookup contracts support the operation.

The actual hashing and equality guarantees depend on the XII string and container implementations.

**Guiding principle:**

> Use owned strings for retained data, views for borrowed access, builders for construction, and hashed strings where identity-oriented lookup justifies them.

### 11.2 Distinguish Names, Identifiers, and Display Labels

A textual name does not automatically represent a unique object identity.

XII subsystems **MUST** distinguish between names used for identification, labels used for presentation, and handles used for authoritative resource identity.

#### General Requirements

- Names used as identifiers **MUST** have documented uniqueness and comparison semantics.
- Display names **SHOULD NOT** determine object identity unless the subsystem explicitly defines that behavior.
- Resource identity **SHOULD** remain stable independently of diagnostic or user-facing names when the subsystem requires renaming.
- Do not assume that identical display labels identify the same object.
- Do not assume that distinct strings necessarily identify distinct objects when normalization or case-insensitive comparison is applied.
- Name-based registries **MUST** define how duplicate names are handled.
- Rename operations **MUST** preserve registry consistency and the documented identity contract.
- Names used for serialization or persistent references **MUST** have defined stability and compatibility requirements.
- Avoid maintaining redundant names and identifiers unless they serve distinct responsibilities.

**Example:**

```cpp
struct xiiNamedResource
{
  xiiHashedString   m_sName;
  xiiResourceHandle m_hResource;
};
```

The name provides a lookup or descriptive identity within a particular namespace, while the handle identifies the registered resource according to its manager's rules.

The two values should not be treated as interchangeable.

#### Identity Scope

Name uniqueness must be defined within an explicit scope.

Possible scopes include:

- A single object.
- A document or scene.
- A resource registry.
- A loaded plugin.
- An application instance.
- A persistent asset database.

A name unique within one scene is not necessarily unique across the entire engine.

Do not introduce global uniqueness requirements when a smaller namespace satisfies the actual consumer requirements.

#### Duplicate Names

A registry **MUST** define whether duplicate names are:

1. Rejected.
2. Replacements for existing registrations.
3. Permitted under different owning contexts.
4. Represented as multiple entries.
5. Resolved through an explicit namespace or qualification mechanism.

The selected behavior must be consistent across creation, lookup, renaming, serialization, and destruction.

**Guiding principle:**

> A name describes or locates an object within a defined namespace. An authoritative identifier establishes which object is being referenced.

### 11.3 String Ownership, Conversion, and Lifetime

String APIs **MUST** make ownership and lifetime requirements clear.

A function that accepts `xiiStringView` receives borrowed access to text. It does not automatically receive ownership of the backing storage.

The borrowing rules established in §6.3 and §6.8 apply to all non-owning string representations.

#### General Requirements

- A function retaining text beyond its invocation **MUST** establish ownership or a valid externally guaranteed lifetime.
- Do not retain `xiiStringView` objects that reference temporary strings.
- Do not assume that converting a string into a view extends the original string's lifetime.
- Avoid returning views into temporary builders or locally owned strings.
- Do not retain pointers to string storage across mutations that may reallocate or invalidate that storage.
- String conversions **MUST** preserve required encoding and representation semantics.
- Do not assume that string views are NUL-terminated.
- Interfaces requiring NUL-terminated strings **MUST** receive storage that satisfies that requirement.
- Asynchronous operations **MUST** own or otherwise guarantee the lifetime of text retained across execution boundaries.

#### Safe Ownership Transfer

**Prefer:**

```cpp
class xiiDocument
{
public:
  void SetName(xiiStringView sName)
  {
    m_sName = sName;
  }

private:
  xiiString m_sName;
};
```

The document stores an owning copy of the provided text.

The caller may destroy or modify its original string after `SetName()` returns without invalidating the document's stored name.

#### Unsafe Borrowing

**Avoid:**

```cpp
class xiiDocument
{
public:
  void SetName(xiiStringView sName)
  {
    m_sName = sName;
  }

private:
  xiiStringView m_sName;
};
```

This implementation stores a borrowed view without establishing that the caller's backing storage will remain valid.

It may be correct only when the lifetime dependency is explicitly documented and enforced.

#### NUL-Terminated Strings

C-compatible APIs often require pointers to NUL-terminated character sequences.

A string view representing an arbitrary substring does not necessarily satisfy this requirement.

Do not pass a string view's underlying pointer to such an API unless its termination contract is guaranteed.

When necessary, construct an appropriately terminated owning or temporary string representation at the integration boundary.

**Guiding principle:**

> String ownership must follow the duration of use. A borrowed view is an access mechanism, not a storage or lifetime guarantee.

### 11.4 Hashing and Hash-Based Lookup

Hashing **SHOULD** be used when it provides an appropriate representation for lookup, comparison, caching, or indexing.

A hash is a derived value and must not automatically be treated as a globally unique identifier.

#### General Requirements

- Hash functions **MUST** produce consistent results for equivalent keys under the defined comparison contract.
- Hash-table equality semantics **MUST** be compatible with the selected hash function.
- Hash collisions **MUST** be handled according to the container or identifier system's documented contract.
- Do not assume that a hash value uniquely identifies arbitrary input data.
- Do not persist implementation-dependent hash values as stable identifiers unless their algorithm and representation are explicitly part of the persistent format.
- Avoid repeatedly hashing unchanged values in performance-critical lookup paths when cached hashes are appropriate.
- Do not introduce persistent hash storage when its maintenance cost exceeds its demonstrated benefit.
- Hash-based lookup **MUST** preserve the required string comparison and normalization semantics.
- Do not use ordinary non-cryptographic hashes to establish security or authenticity.
- Hash-table iteration order **MUST NOT** be treated as deterministic unless the container explicitly guarantees it.

#### Hash Equality and Identity

A hash function maps a potentially large input domain into a finite range.

Different inputs can therefore produce the same hash value.

When exact identity matters, the implementation must either:

- Compare the original values following a hash match.
- Use an established string interning mechanism with documented identity guarantees.
- Use a registry-managed identifier that resolves collisions correctly.
- Explicitly define and accept the collision semantics of the representation.

Do not assume that a 32-bit or 64-bit hash is collision-free merely because collisions are uncommon for representative workloads.

#### Persistent Hashes

A hash value used in a persistent format requires a stable contract.

That contract should specify:

- Hash algorithm.
- Input encoding.
- Normalization rules.
- Case sensitivity.
- Width and representation.
- Versioning requirements.
- Collision handling.

Changing any of these properties may invalidate stored identifiers or serialized references.

#### Hashing and Performance

Hashing reduces some comparison costs but introduces its own computation and storage overhead.

For small collections, direct comparisons or linear scans may outperform a hash table because of locality and reduced indirection.

Container selection should follow §10.1 and the evidence-based performance requirements of §1.5.

**Guiding principle:**

> Hashes accelerate lookup and comparison; they do not inherently provide collision-free identity, deterministic ordering, or persistent compatibility.

### 11.5 Generation-Checked Handles

Handles **SHOULD** be used for objects whose identity and lifetime are managed independently by a registry, manager, or subsystem.

A handle represents a logical reference rather than a direct memory address.

Generation checking can detect stale references when registry slots are reused.

#### General Requirements

- Handle types **MUST** represent clearly defined resource categories or identity domains.
- Prefer distinct handle types for semantically different resources, even when their underlying representations are identical.
- Handle validity **MUST** be determined according to the authoritative registry's rules.
- Handles **MUST NOT** imply ownership unless explicitly documented.
- Reusable registry slots **SHOULD** employ generation tracking or another appropriate stale-reference detection mechanism.
- Handle construction and invalidation **MUST** preserve the representation's invariants.
- Invalid handles **MUST** have a defined representation or validity-checking mechanism.
- Generation exhaustion or wraparound **MUST** be considered when the handle format permits repeated slot reuse.
- Handle validation **MUST NOT** be confused with lifetime protection during concurrent access.
- Do not expose underlying handle bit fields unnecessarily through public interfaces.

#### Illustrative Handle Definition

```cpp
using xiiSimulationId = xiiGenericId<24, 8>;

class XII_CORE_DLL xiiSimulationHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiSimulationHandle, xiiSimulationId);
};
```

This example illustrates the use of a typed handle and an underlying identifier representation.

The actual handle declaration, identifier widths, friend declarations, storage layout, and generation policy must follow the owning subsystem's implementation.

The example does not prescribe a universal 24-bit index or 8-bit generation format.

#### Stale Handles

A generation-checked handle typically identifies both a slot and a particular lifetime of that slot.

When the object is destroyed and its slot is reused, the generation changes according to the registry's rules.

A stale handle can then be rejected instead of resolving to the new object.

However, generation checking does not guarantee permanent protection.

A finite-width generation counter can eventually wrap, potentially recreating an earlier encoded identifier if the slot is reused sufficiently many times.

The acceptable generation width and reuse policy must reflect the expected object churn and lifetime requirements.

#### Typed Handles

**Prefer:**

```cpp
xiiSimulationHandle hSimulation;
xiiResourceHandle   hResource;
```

**Avoid:**

```cpp
xiiUInt32 uiSimulationId;
xiiUInt32 uiResourceId;
```

Distinct handle types make accidental cross-subsystem usage more difficult and allow the compiler to enforce meaningful interface distinctions.

**Guiding principle:**

> Handles provide typed logical identity and permit validation. Their safety depends on the registry's generation, lifetime, and synchronization contracts.

### 11.6 Handle Ownership Domains and Context Identity

A handle's underlying numeric representation does not automatically identify its owning subsystem, manager, or execution context.

Where handles are valid only within a particular ownership domain, that relationship **MUST** be established and enforced.

#### General Requirements

- Every handle type **MUST** have a defined authority responsible for its validity.
- The scope of handle uniqueness **MUST** be documented when multiple registries or contexts can create the same handle type.
- Do not assume that a globally accessible handle type implies globally unique identifiers.
- Cross-context handle access **MUST** be validated when the API permits references from multiple contexts.
- Handle ownership **MAY** be represented through encoded identifiers, registry metadata, or an explicit context parameter.
- Do not increase handle width or introduce additional identifier fields without a demonstrated requirement.
- Resource lookup **MUST** use the correct authoritative registry.
- Handle transfer between contexts **MUST** be explicitly supported or rejected.
- Context destruction **MUST** invalidate or retire resources according to the context's ownership contract.

#### Contextual Identity

Consider a system supporting multiple independently managed simulation worlds.

A handle allocated by one world may have the same underlying index and generation as a handle allocated by another world.

If those handles share a type, the implementation must prevent accidental cross-world resolution when such access is invalid.

Possible approaches include:

1. Encoding an owner or context identifier into the handle.
2. Recording the owner in authoritative registry metadata.
3. Requiring an explicit owning context during lookup.
4. Using globally allocated identifiers with documented uniqueness guarantees.

No single strategy is universally preferable.

The selected approach should reflect the actual lifetime, lookup, and cross-context requirements.

#### Avoid Hidden Ownership Assumptions

Do not infer ownership from unrelated properties such as:

- A handle's numeric magnitude.
- The order in which resources were created.
- The currently active frame.
- The currently active device.
- The global registry that happens to be accessible.
- A resource's display name.

Such assumptions are fragile unless they form part of an explicitly enforced contract.

**Guiding principle:**

> A handle identifies a resource only within the domain that defines its meaning. Ownership context must be represented or validated explicitly.

### 11.7 Authoritative Registries and Mapping Consistency

Systems maintaining names, handles, indices, or resource relationships **MUST** define which representation is authoritative.

Secondary mappings are useful for lookup and iteration but introduce consistency requirements.

#### General Requirements

- Each resource **MUST** have one clearly defined authoritative ownership or registration mechanism.
- Secondary lookup tables **MUST** remain consistent with authoritative state.
- Registration **MUST** preserve uniqueness and identity requirements.
- Resource destruction **MUST** remove or invalidate associated secondary mappings according to the lifecycle contract.
- Renaming **MUST** update affected name-based mappings consistently.
- Partial registration failures **MUST** leave the system in a documented valid state.
- Avoid duplicating complete resource state across multiple containers without a demonstrated need.
- Multi-container updates **MUST** be coordinated through appropriate synchronization or rollback mechanisms.
- Readers **MUST NOT** observe inconsistent intermediate states when the subsystem's concurrency contract prohibits them.
- Do not introduce global registries merely to simplify access to otherwise locally owned resources.

#### Authoritative and Secondary Mappings

A subsystem may use:

- A handle table as the authoritative identity registry.
- A name-to-handle table for lookup.
- A list of handles for ordered iteration.
- A context-to-handle table for membership.
- A cache for reusable derived data.

These structures serve different responsibilities.

Only one should be considered authoritative for each particular ownership or identity fact unless the system explicitly maintains a stronger consistency model.

#### Registration Failure

When registration requires modifying several structures, failure must not leave partially published resources.

**Prefer conceptually:**

```cpp
xiiResult RegisterResource(xiiStringView sName)
{
  if (m_NameLookup.Contains(sName))
    return XII_FAILURE;

  // Prepare and validate the required resource state.
  // Publish the resource only when registration can
  // satisfy the subsystem's consistency contract.

  return CommitRegistration(sName);
}
```

This example illustrates the required separation between validation and commitment.

It does not prescribe a specific transaction mechanism.

If publishing a resource requires multiple allocations or container updates, the implementation must provide rollback or another safe method of preserving consistency when an intermediate operation fails.

#### Concurrency

Consistency may require a lock spanning multiple registry updates, a staged publication mechanism, or another synchronization strategy.

Do not assume that individually thread-safe containers make a multi-container operation atomic.

Likewise, registry synchronization does not automatically protect the lifetime of objects returned from a lookup.

See §6.5 for lifetime-safe access requirements.

**Guiding principle:**

> Maintain one authority for each resource fact. Every secondary mapping must remain consistent with it across creation, mutation, failure, and destruction.

### 11.8 String Comparison, Normalization, and Equality

String comparison semantics **MUST** match the meaning of the data being compared.

Byte equality, case-insensitive equality, normalized path equality, and logical identifier equality are not interchangeable.

#### General Requirements

- String comparisons **MUST** use semantics appropriate to the domain.
- Do not assume that byte equality implies logical equivalence for every text representation.
- Do not introduce case-insensitive comparisons merely for user convenience when case sensitivity is part of the identifier contract.
- Unicode normalization **MUST** be considered when the application requires canonical textual equivalence.
- Name-based lookup and name-based insertion **MUST** use compatible comparison rules.
- Hashing **MUST** follow the same equivalence rules used by equality comparisons.
- Do not normalize identifiers in a way that silently merges previously distinct valid values.
- Public identifier comparison rules **SHOULD** remain stable across versions when persistent compatibility is required.
- Do not assume ASCII-only transformations are sufficient for arbitrary Unicode text.

#### Comparison Categories

| Comparison | Meaning | Typical use |
|---|---|---|
| Byte comparison | Exact encoded-byte equality | Binary identifiers, explicitly byte-oriented protocols |
| Case-sensitive text comparison | Text comparison preserving case distinctions | Case-sensitive resource names |
| Case-insensitive comparison | Equality under a defined case-mapping rule | Explicitly case-insensitive identifiers |
| Normalized text comparison | Equality after a defined normalization process | Text systems requiring canonical equivalence |
| Normalized path comparison | Equality according to a defined path model | Virtual filesystem or resource path handling |
| Hashed lookup | Lookup using a hash compatible with the equality relation | Name registries and associative containers |

A hashed lookup must preserve the same equality semantics as the corresponding non-hashed representation.

#### Hash and Equality Consistency

If two keys compare equal, they must produce compatible hash values according to the hash table's contract.

For example, a case-insensitive equality function cannot safely be paired with a case-sensitive hash that produces different values for keys considered equivalent.

#### Unicode and Normalization

Unicode strings may contain different encoded representations of text that appears visually identical.

Whether those representations should compare equal depends on the subsystem's contract.

Do not apply Unicode normalization automatically to every engine identifier.

For identifiers used in persistent formats, normalization policy changes may alter lookup results and compatibility.

**Guiding principle:**

> Equality is a domain contract. Hashing, lookup, normalization, and serialization must agree on what constitutes the same identifier.

### 11.9 Paths, Virtual Filesystems, and Resource Identifiers

Filesystem paths, virtual filesystem paths, resource identifiers, and display names **MUST** be treated as distinct concepts when their semantics differ.

XII operates across platforms with different filesystem conventions and must not assume that one platform's path representation applies universally.

#### General Requirements

- Prefer established XII path and filesystem utilities for path construction and manipulation.
- Do not concatenate filesystem paths using arbitrary string operations when a path-aware API is available.
- Do not assume that all platforms use identical path separators.
- Do not assume that all filesystems are case-sensitive or case-insensitive.
- Do not assume that drive letters, root paths, or volume representations are universally available.
- Relative paths **MUST** be interpreted against a defined base or virtual filesystem context.
- Do not assume lexical path normalization proves that two paths refer to the same physical file.
- Resource identifiers **SHOULD** remain independent of physical filesystem paths when the resource system supports virtualized lookup.
- User-facing labels **SHOULD NOT** be treated as authoritative filesystem identifiers.
- Paths received from external sources **MUST** be validated according to the filesystem and security boundaries of the receiving subsystem.
- Path comparison and hashing **MUST** use compatible normalization and equality semantics.

#### Path Categories

| Category | Meaning |
|---|---|
| Physical filesystem path | Refers to a location interpreted by the host filesystem |
| Virtual filesystem path | Refers to a location resolved through an engine-managed filesystem abstraction |
| Resource identifier | Identifies an asset or resource through the resource-management system |
| Display name | Human-readable text used for presentation or diagnostics |
| Relative path | A path interpreted against an explicitly defined base |
| Canonical path | A path resolved or normalized according to a particular filesystem or path contract |

These representations may contain similar text but do not necessarily have equivalent meanings.

#### Path Construction

**Prefer:**

```cpp
xiiStringBuilder sPath = sDirectory;
sPath.AppendPath(sFileName);
```

**Avoid:**

```cpp
xiiStringBuilder sPath;

sPath.Append(sDirectory);
sPath.Append("/");
sPath.Append(sFileName);
```

The preferred example delegates path separator handling to a path-aware operation.

The specific semantics of `AppendPath()` and the applicable virtual or physical filesystem rules must still be respected.

#### Path Normalization

Normalization may include:

- Separator normalization.
- Removal of redundant separators.
- Resolution of `.` components.
- Lexical handling of `..` components.
- Case normalization under a defined policy.
- Resolution relative to a known base.

Lexical normalization does not necessarily resolve symbolic links, filesystem aliases, mount points, or physical file identity.

Do not use string normalization alone as proof that a path is safe or contained within a permitted filesystem root.

#### Security Boundaries

Paths originating from plugins, user configuration, network requests, or other untrusted sources may require additional validation.

Validation **MUST** follow the actual filesystem operation and trust boundary.

In particular, string-prefix comparisons are not sufficient to establish secure filesystem containment when path traversal, symbolic links, or filesystem resolution can change the target location.

**Guiding principle:**

> Paths describe locations under a filesystem contract; resource identifiers describe engine-managed identity. Preserve the distinction and use platform-aware resolution.

### 11.10 Persistent and Serialized Identifiers

Identifiers used in serialized data, asset databases, network protocols, or other persistent formats require stronger stability guarantees than temporary runtime identifiers.

A runtime handle **MUST NOT** automatically be treated as a persistent identifier.

#### General Requirements

- Persistent identifiers **MUST** have documented uniqueness and stability requirements.
- Do not serialize runtime handles as durable references unless their representation and resolution semantics explicitly support persistence.
- Generation counters and registry indices **SHOULD** be treated as runtime implementation details unless otherwise documented.
- Persistent identifier formats **MUST** have defined encoding and versioning requirements.
- Identifier changes **MUST** consider migration and compatibility with existing data.
- Do not use process-local pointer values as persistent identifiers.
- Hash-derived persistent identifiers **MUST** define their hashing and collision policies.
- Deserialization **MUST** validate identifiers before resolving them to runtime resources.
- Resource resolution **MUST** distinguish missing, invalid, and unavailable resources where those states require different behavior.
- Identifier representations **SHOULD** remain independent of memory layout and process-specific allocation behavior.

#### Runtime Versus Persistent Identity

A runtime handle may contain an index and generation used to locate an object within the current process.

That representation may change when:

- The application restarts.
- Resources load in a different order.
- A registry is reconstructed.
- Resources are destroyed and recreated.
- An asset is loaded into another context.

A persistent resource identifier must retain its intended meaning across the operations and environments defined by its contract.

#### Serialization Boundaries

When loading persistent references, resolve them through an authoritative resource system rather than assuming that stored numeric values remain valid runtime handles.

**Guiding principle:**

> Runtime handles identify objects within an active ownership system. Persistent identifiers must survive the lifecycle and compatibility boundaries they are designed to cross.

### 11.11 Handle Validation and Resource Access

Handle validation **MUST** be performed by the subsystem that owns the associated identity and lifetime contract.

A handle's local validity representation is not necessarily sufficient to establish that its resource still exists.

#### General Requirements

- Distinguish structurally valid handles from handles that currently resolve to live resources.
- Resource lookup **MUST** reject handles that are invalid according to the authoritative registry.
- Stale-generation checks **MUST** occur where required by the registry contract.
- A successful lookup **MUST NOT** be assumed to extend resource lifetime beyond the documented access boundary.
- Concurrent destruction **MUST** be coordinated with resource access.
- Do not expose unchecked handle-to-pointer conversions through general public interfaces.
- Handle invalidation **SHOULD** occur after successful release where supported.
- Invalidating one handle variable **MUST NOT** be assumed to revoke copies held by other consumers.
- Do not introduce repeated registry lookups within critical paths without considering their cost and alternatives.
- Optimization of handle access **MUST NOT** bypass required validation or lifetime guarantees.

#### Structural Versus Registry Validity

A handle may encode a value that is not the designated invalid representation while still referring to a destroyed resource.

For example:

```cpp
if (!hResource.IsInvalidated())
{
  // The handle is not locally marked invalid.
  // The owning registry must still determine whether it
  // refers to a currently valid resource.
}
```

The exact validity methods depend on the handle implementation.

#### Scoped Access

When resource destruction may occur concurrently, prefer scoped access mechanisms that preserve validity for the duration of the operation.

A registry lookup protected by a short-lived mutex is insufficient if the returned pointer may be destroyed immediately after the mutex is released.

See §6.5 for the supported lifetime strategies.

**Guiding principle:**

> A handle is meaningful only through its authoritative registry. Validation establishes current identity; safe access additionally requires lifetime and synchronization guarantees.

### 11.12 Identifier and Hash Diagnostics

Identifiers and hashes frequently appear in logs, assertions, diagnostic tools, and debugging interfaces.

Diagnostic output **SHOULD** provide enough context to identify the affected resource without confusing implementation values with user-facing identity.

#### General Requirements

- Diagnostics **SHOULD** include meaningful resource names or identifiers when available.
- Do not rely exclusively on opaque hash values when a descriptive name can be reported.
- Handle diagnostics **SHOULD** distinguish the resource category and owning context when relevant.
- Do not assume that a raw numeric identifier is globally unique.
- Logging **MUST NOT** dereference invalid resources merely to obtain descriptive information.
- Diagnostic helpers **SHOULD** remain safe during failure handling and subsystem shutdown.
- Do not retain unnecessary diagnostic strings in performance-critical data structures without a demonstrated requirement.
- Diagnostic formatting **SHOULD** use established XII formatting and logging facilities.
- Error messages **MUST** avoid implying that a handle is valid merely because its encoded value is nonzero.

#### Diagnostic Context

**Prefer:**

```cpp
xiiLog::Error("Failed to resolve resource '{0}' in simulation context {1}.", sResourceName, uiContextId);
```

**Avoid:**

```cpp
xiiLog::Error("Resource lookup failed.");
```

The preferred diagnostic identifies the relevant operation and lookup context.

When name storage is unavailable or unsafe to access, reporting a validated handle representation or other available context may be appropriate.

#### Hash Diagnostics

A hash value may be useful for debugging lookup behavior, but it should not be presented as definitive proof of resource identity unless the identifier system explicitly guarantees that interpretation.

**Guiding principle:**

> Diagnostics should describe the resource and its context, not merely expose an opaque implementation value.

### 11.13 Identifier Design and Review

New identifier and handle types **SHOULD** be introduced only when they establish a meaningful distinction in ownership, identity, or validity.

Do not create additional identifier layers merely to rename existing integer values.

#### Design Considerations

Before introducing a new identifier or handle type, determine:

1. **Identity:** What exactly does the identifier represent?
2. **Authority:** Which subsystem creates and validates it?
3. **Scope:** Within which registry, context, process, or persistent domain is it unique?
4. **Lifetime:** When does the identifier become invalid?
5. **Reuse:** Can the underlying identifier value be reassigned?
6. **Generation:** Is stale-reference detection required?
7. **Ownership:** Does possession of the identifier confer ownership or only access?
8. **Concurrency:** Can lookup and destruction occur concurrently?
9. **Persistence:** Must the identifier survive process or registry reconstruction?
10. **Compatibility:** Is the representation part of a public binary or serialized contract?

The selected representation **MUST** satisfy the actual answers to these questions.

Avoid encoding unrelated properties into handles merely because spare bits are available.

Similarly, do not expose internal identifier fields unless consumers have a legitimate need to inspect them.

**Guiding principle:**

> An identifier is an architectural contract. Its type, scope, authority, lifetime, and reuse semantics must be deliberate and consistently enforced.


## 12. Errors, Assertions, Diagnostics, and Logging

Error handling in XII **MUST** distinguish expected operational failures from programming errors, preserve the invariants of affected components, and provide sufficient diagnostic context to identify and resolve problems.

XII does not permit C++ exceptions for ordinary error handling. Expected failures are communicated through explicit mechanisms such as `xiiResult`, appropriate result types, and diagnostic outputs. Exceptions may only be encountered within deeply isolated platform or third-party integration code where they are unavoidable and must not propagate through ordinary engine interfaces.

Assertions, error results, logging, and profiling serve different purposes and **MUST NOT** be treated as interchangeable mechanisms.

This section extends §1.1 (Correctness), §6 (Ownership and Lifetime), §7.7 (Result Handling), §7.10 (Exception Boundaries), and §8.11 (Assertions).

### 12.1 Choose the Correct Failure Channel

The mechanism used to report a problem **MUST** reflect whether the condition represents an expected runtime failure, a violated programming contract, or a diagnostic event.

#### General Requirements

- Use `xiiResult` or an appropriate error-returning type for recoverable operational failures.
- Use assertions to detect programming errors, violated preconditions, and internal invariants that must hold during valid execution.
- Use logging to provide diagnostic context, not as a substitute for returning or handling failure.
- Do not use exceptions for ordinary engine error handling.
- Do not use assertions as substitutes for validating untrusted input.
- Do not return `XII_FAILURE` for every programming error merely to avoid enforcing a documented contract.
- Do not continue an operation after a required dependency has failed unless a valid recovery or fallback path exists.
- Error-handling paths **MUST** maintain the documented ownership and state invariants of the affected components.
- Failure reporting **SHOULD** provide sufficient information for callers to determine whether recovery, retry, fallback, or termination of the current operation is appropriate.

| Situation | Preferred mechanism |
|---|---|
| Invalid external configuration | Validate and return explicit failure |
| File cannot be opened | `xiiResult` with contextual diagnostic |
| Malformed serialized data | Validation failure with actionable diagnostic |
| Shader or asset compilation failure | Explicit failure and compiler diagnostics |
| Network connection failure | Error result with relevant connection context |
| Resource allocation failure | Explicit failure or established allocator failure mechanism |
| Required pointer violates an internal contract | Assertion; runtime enforcement where required |
| Internal invariant is violated | Assertion or appropriate fatal-error mechanism |
| Optional capability is unavailable | Defined fallback or explicit unsupported-operation result |
| GPU device loss | Propagate failure and preserve backend diagnostic context |
| Invalid or stale resource handle | Contract enforcement or recoverable failure according to the API |
| Non-actionable telemetry event | Trace, profiling event, or appropriately leveled log |
| Unrecoverable subsystem corruption | Established fatal-error or termination mechanism |

#### Expected Failures Versus Contract Violations

An expected failure can occur during an otherwise valid operation.

For example, opening a file may fail because the file does not exist, access is denied, or the underlying filesystem encounters an error.

A contract violation occurs when an operation is invoked in a state prohibited by its documented interface.

**Expected failure:**

```cpp
xiiResult LoadConfiguration(xiiStringView sPath)
{
  xiiFileReader file;

  XII_SUCCEED_OR_RETURN(file.Open(sPath));

  return ParseConfiguration(file);
}
```

**Contract violation:**

```cpp
void SetCapacity(xiiUInt32 uiCapacity)
{
  XII_ASSERT_DEV(uiCapacity > 0U, "Capacity must be greater than zero.");

  m_uiCapacity = uiCapacity;
}
```

The second example assumes positive capacity is an enforced programmer-facing precondition. If capacity originates from untrusted external data, validation must occur before calling this function.

**Guiding principle:**

> Expected failures are communicated through results. Programming errors violate contracts. Diagnostics explain what happened without replacing either mechanism.

### 12.2 Assertions Do Not Replace Runtime Validation

Assertions are development-time mechanisms for detecting violated assumptions and internal invariants.

Their presence **MUST NOT** be required for correct execution in configurations where the assertions may be disabled.

#### General Requirements

- Assertions **MUST** express conditions expected to hold during valid execution.
- Do not place essential side effects inside assertion expressions.
- Do not assume that development assertions execute in every build configuration.
- Invalid external input **MUST** be handled through runtime validation.
- Runtime checks **MUST** remain active when their absence could permit invalid memory access, corrupted state, or other unacceptable behavior.
- Do not duplicate assertions and runtime checks mechanically when the API contract requires only one mechanism.
- Assertions **SHOULD** include meaningful diagnostic messages identifying the violated invariant.
- Do not use an assertion to suppress handling of a legitimate operational failure.
- Assertion conditions **SHOULD** remain inexpensive enough for their intended build configurations.

#### Runtime Validation

**Prefer when invalid input must be handled safely:**

```cpp
xiiResult InitializeStorage(xiiAllocator* pAllocator)
{
  if (pAllocator == nullptr)
    return XII_FAILURE;

  return CreateStorage(pAllocator);
}
```

This example treats the allocator as a potentially absent dependency that must be validated at runtime.

If a valid allocator is instead an unconditional internal precondition, an assertion may be more appropriate.

#### Assertion with Runtime Enforcement

Both mechanisms may be justified when a condition simultaneously represents a programming error and must be guarded against in release configurations.

```cpp
XII_ASSERT_DEV(pDevice != nullptr, "A valid device is required.");

if (pDevice == nullptr)
  return XII_FAILURE;
```

The assertion identifies an unexpected caller violation during development, while the runtime guard prevents the implementation from dereferencing a null pointer.

This pattern **SHOULD NOT** be applied indiscriminately. The necessity of runtime enforcement depends on the API's trust boundary and documented failure contract.

#### Assertion Side Effects

**Avoid:**

```cpp
XII_ASSERT_DEV(InitializeResources().Succeeded(), "Resource initialization failed.");
```

If assertions are disabled, `InitializeResources()` may not execute.

**Prefer:**

```cpp
const xiiResult result = InitializeResources();

XII_ASSERT_DEV(result.Succeeded(), "Resource initialization failed.");
```

This preserves the side effect independently of assertion evaluation.

However, if initialization failure is expected, handle or propagate the result rather than relying exclusively on an assertion.

**Guiding principle:**

> Assertions verify contracts. Runtime validation enforces safety at boundaries where invalid conditions can legitimately occur or must be defended against.

### 12.3 Log Actionable Diagnostic Context

Diagnostic messages **SHOULD** explain which operation failed, what object or subsystem was involved, and which relevant conditions contributed to the failure.

A useful diagnostic should reduce the amount of additional investigation required to identify the problem.

#### General Requirements

- Error messages **SHOULD** identify the operation that failed.
- Include relevant object names, identifiers, paths, resource types, or context values when available.
- Preserve underlying platform or backend error information when it materially helps diagnosis.
- Prefer precise failure descriptions over generic messages such as `"Failed"` or `"Invalid state"`.
- Do not log successful high-frequency operations unnecessarily.
- Avoid excessive diagnostic output that obscures meaningful failures.
- Do not log credentials, secrets, authentication tokens, or other sensitive information.
- Do not dump large objects or binary buffers without a demonstrated diagnostic requirement.
- Do not treat raw pointers as stable resource identities.
- Diagnostic formatting **MUST** follow XII's established logging and formatting APIs.
- Diagnostic code **MUST NOT** access resources whose lifetime is no longer guaranteed.
- Messages **SHOULD** use established engine terminology and correct spelling.

**Prefer:**

```cpp
xiiLog::Error("Failed to load configuration '{0}': {1}", sConfigurationPath, sError);
```

**Avoid:**

```cpp
xiiLog::Error("Loading failed.");
```

The preferred diagnostic identifies the affected file and provides relevant failure context.

The examples assume that the diagnostic values are available and compatible with the selected logging API.

#### Structured Diagnostic Context

When several values are necessary to understand a failure, include them deliberately.

For example, a resource allocation failure may benefit from reporting:

- Resource type.
- Requested size.
- Required alignment.
- Owning subsystem.
- Relevant memory budget.
- Underlying allocator or platform error.

Not every diagnostic requires all available context.

Include the information needed to distinguish the failure from similar failures without producing unnecessarily verbose logs.

#### Logging During Failure Handling

Logging itself may allocate memory, acquire locks, or invoke formatting code.

Failure-handling paths **SHOULD** avoid diagnostics that introduce additional failure risks, particularly during low-memory conditions, device loss, or subsystem shutdown.

**Guiding principle:**

> Diagnostics should identify the failed operation, preserve relevant context, and help locate the cause without introducing unnecessary noise or additional failure risk.

### 12.4 Do Not Continue After Required Operations Fail

An operation **MUST NOT** continue into a state that depends on a failed prerequisite unless an explicit recovery or fallback path establishes the required invariants.

#### General Requirements

- Check failure-sensitive results before performing dependent operations.
- Propagate failures when the current function cannot meaningfully recover.
- Do not substitute default values for failed operations unless the fallback is part of the documented contract.
- Do not report success when required work was not completed.
- Failed operations **MUST** leave their owning components in documented valid states.
- Error paths **MUST** release or preserve acquired resources according to their ownership contracts.
- Do not retry failed operations indefinitely without a defined progress or termination policy.
- Recovery paths **MUST** establish that the original failure condition has been resolved or safely bypassed.

**Prefer:**

```cpp
xiiResult LoadAndValidateDocument(xiiStringView sPath)
{
  XII_SUCCEED_OR_RETURN(LoadDocument(sPath));
  XII_SUCCEED_OR_RETURN(ValidateDocument());

  return XII_SUCCESS;
}
```

**Avoid:**

```cpp
xiiResult LoadAndValidateDocument(xiiStringView sPath)
{
  LoadDocument(sPath).IgnoreResult();

  ValidateDocument().IgnoreResult();

  return XII_SUCCESS;
}
```

The avoided implementation reports success even if the document could not be loaded or validated.

#### Recovery and Fallback

Continuing after failure is appropriate only when the operation has a defined recovery strategy.

For example, loading an optional configuration file may permit falling back to documented defaults.

```cpp
xiiResult LoadOptionalSettings(xiiStringView sPath)
{
  if (LoadSettings(sPath).Succeeded())
    return XII_SUCCESS;

  ApplyDefaultSettings();

  return XII_SUCCESS;
}
```

This behavior is appropriate only when every failure from `LoadSettings()` is permitted to trigger the fallback.

If malformed configuration, permission errors, and missing optional files require different responses, the implementation must preserve enough error information to distinguish them.

**Guiding principle:**

> A failed prerequisite invalidates the dependent path until recovery establishes a valid alternative.

### 12.5 Error Propagation and Diagnostic Ownership

Errors **SHOULD** be propagated to the layer responsible for deciding how the failure affects the application or subsystem.

Logging and propagation are complementary, but repeated logging of the same failure can obscure its original cause.

#### General Requirements

- Propagate failures when the current layer cannot meaningfully resolve them.
- Add diagnostic context when crossing a boundary that provides relevant information.
- Avoid logging identical errors at every propagation layer.
- Preserve underlying failure information when it is needed for recovery or diagnosis.
- Do not replace specific errors with generic failures prematurely when callers require the distinction.
- Public interfaces **MUST** communicate failure through their documented return or output mechanism.
- Internal helpers **SHOULD** avoid deciding application-level failure policy unless that responsibility belongs to them.
- Error translation at platform boundaries **MUST** preserve the information necessary to distinguish important failure categories.

#### Propagation Without Additional Context

**Prefer:**

```cpp
xiiResult InitializeSubsystem()
{
  XII_SUCCEED_OR_RETURN(InitializeStorage());
  XII_SUCCEED_OR_RETURN(InitializeResources());

  return XII_SUCCESS;
}
```

This is appropriate when the underlying operations already provide sufficient diagnostics or when a higher layer is responsible for reporting the failure.

#### Propagation With Context

```cpp
xiiResult InitializeSubsystem()
{
  if (InitializeResources().Failed())
  {
    xiiLog::Error("Failed to initialize subsystem resources.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}
```

The diagnostic adds useful subsystem context without requiring every lower-level helper to produce redundant messages.

#### Logging Responsibility

For complex call chains, establish a consistent diagnostic strategy.

One practical approach is:

1. Low-level operations preserve detailed failure information.
2. Intermediate layers propagate failures and add context only when useful.
3. The responsible boundary logs the actionable failure or presents it to the caller.

This is a guideline rather than a universal requirement. Some failures must be logged immediately because critical diagnostic information is unavailable after returning.

**Guiding principle:**

> Propagate errors without losing their meaning. Log where context is useful, and avoid turning one failure into many redundant diagnostics.

### 12.6 Transactional Operations and Rollback

Operations that update multiple dependent structures **MUST** preserve consistency when an intermediate step fails.

Whenever practical, prefer preparing and validating changes before publishing them to externally observable state.

#### General Requirements

- Multi-step operations **MUST** define their failure guarantees.
- Prefer completing potentially failing preparation before modifying authoritative state.
- Partial failure **MUST NOT** leave registries, indices, or ownership structures inconsistent.
- Rollback operations **MUST** preserve the original invariants.
- Avoid publishing partially initialized objects to unrelated consumers.
- Resources acquired during failed operations **MUST** be released or retained according to a documented ownership contract.
- Concurrent readers **MUST NOT** observe inconsistent intermediate state when the subsystem guarantees atomic updates.
- Use synchronization appropriate to the transaction's scope.
- Do not introduce elaborate transaction frameworks for simple operations that can maintain consistency through straightforward ordering.
- Complex rollback paths **SHOULD** be tested with deliberate failure injection.

#### Prepare, Validate, Commit

A useful pattern for multi-step operations is:

1. **Prepare:** Construct temporary state and acquire required resources.
2. **Validate:** Confirm that all prerequisites and constraints are satisfied.
3. **Commit:** Publish the completed state to the owning subsystem.
4. **Rollback:** Release temporary resources or restore the previous valid state if an operation fails.

**Illustrative example:**

```cpp
xiiResult RegisterResource(xiiStringView sName, xiiResourceHandle& out_hResource)
{
  if (m_NameLookup.Contains(sName))
    return XII_FAILURE;

  xiiUniquePtr<ResourceEntry> pEntry =
    CreateResourceEntry(sName);

  if (pEntry == nullptr)
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(ValidateResourceEntry(*pEntry));

  return CommitResourceEntry(std::move(pEntry), out_hResource);
}
```

This example assumes `CommitResourceEntry()` provides the required consistency and failure guarantees.

If commitment updates multiple structures and may fail partway through, it must provide rollback or another safe atomic-publication strategy.

#### Failure Guarantees

Where practical, an operation **SHOULD** provide the strong failure guarantee: failure leaves externally observable state unchanged.

When that guarantee cannot reasonably be provided, the operation must at least preserve its documented invariants and clearly define which state changes may remain.

**Guiding principle:**

> Prepare before publishing. Failure must not leave partially committed state that violates the subsystem's contracts.

### 12.7 Optional Values and Recoverable Absence

Absence is not always an error.

Functions **SHOULD** distinguish expected absence from operational failure when callers need to handle those conditions differently.

#### General Requirements

- Use optional or nullable representations when the absence of a value is an expected outcome.
- Do not assert merely because a valid lookup finds no matching object.
- Do not use a generic failure result when expected absence is a distinct and meaningful outcome that callers must distinguish.
- Do not return null or an invalid handle without documenting its meaning.
- Multiple failure causes **SHOULD** remain distinguishable when callers require different recovery behavior.
- Avoid introducing complex error types when callers only need success or failure.
- Optional results **MUST** preserve clear ownership and lifetime semantics.
- Do not use sentinel values that overlap valid domain values without an explicit encoding contract.

#### Expected Absence

**Example:**

```cpp
[[nodiscard]] const xiiResourceEntry* FindResource(xiiStringView sName) const;
```

Returning `nullptr` may be appropriate when no matching resource exists and absence is a normal lookup outcome.

#### Operational Failure

A function that loads or constructs a resource may require a different result contract.

```cpp
xiiResult LoadResource(xiiStringView sPath, xiiResourceHandle& out_hResource);
```

This communicates an operation that can fail while producing a handle on success.

#### Rich Error Categories

When callers must distinguish missing files, malformed data, unavailable dependencies, and permission failures, a richer result representation may be appropriate.

Do not collapse these categories into a boolean if doing so prevents required recovery behavior.

**Guiding principle:**

> Absence is a value when it is expected. Failure is an outcome when an operation cannot fulfill its contract. Keep them distinct when the distinction matters.

### 12.8 Destructors, Shutdown, and Failure Reporting

Destruction and shutdown frequently occur when parts of the engine are no longer operational.

Cleanup paths **MUST** preserve resource safety without depending on services that may already be unavailable.

#### General Requirements

- Destructors **MUST NOT** propagate exceptions.
- Destructors **MUST** preserve safe cleanup behavior according to their ownership contracts.
- Cleanup **SHOULD** continue as far as safely possible when independent operations fail.
- Do not continue cleanup steps that depend on resources whose failure invalidated the required state.
- Explicit shutdown operations **MAY** return a failure result when callers must observe cleanup failures.
- Avoid performing unbounded waits during destruction unless required by the documented lifetime contract.
- Logging during shutdown **MUST** respect the lifetime of the logging subsystem.
- Do not access destroyed allocators, platform services, or graphics devices while reporting cleanup failures.
- Failure handling **MUST** account for partially initialized resources.
- Important shutdown failures **SHOULD** preserve diagnostic context before the relevant subsystem becomes unavailable.

#### Explicit Shutdown Results

When cleanup can legitimately fail and the caller needs to know, an explicit shutdown operation may expose the failure.

```cpp
xiiResult ShutdownSubsystem()
{
  xiiResult result = FlushPendingData();

  ReleaseOwnedResources();

  return result;
}
```

This example assumes resource release remains safe regardless of the flush result.

It also assumes `FlushPendingData()` does not invalidate the resources required by `ReleaseOwnedResources()`.

#### Destructors and Error Channels

Destructors cannot return `xiiResult`.

Consequently, operations requiring meaningful recoverable failure reporting **SHOULD** expose an explicit close, flush, or shutdown method when appropriate.

Destruction may then provide final safety cleanup without promising recoverable error reporting.

**Guiding principle:**

> Shutdown must preserve ownership and safety even when operations fail. Report meaningful failures while the required diagnostic infrastructure is still available.

### 12.9 Diagnostic Levels and Logging Performance

Logging and diagnostics can introduce meaningful runtime overhead through formatting, allocation, synchronization, and I/O.

Diagnostic instrumentation **SHOULD** be appropriate to the severity and frequency of the event.

#### General Requirements

- Select diagnostic severity according to the impact and actionability of the event.
- Avoid high-frequency informational or success logs in performance-critical paths.
- Do not eagerly construct expensive diagnostic strings when the corresponding logging level is disabled.
- Prefer existing XII logging and profiling mechanisms over introducing independent diagnostic systems.
- Repeated failures **SHOULD** be rate-limited or aggregated when unbounded logging would obscure useful information or degrade performance.
- Diagnostic instrumentation **MUST NOT** change required synchronization or lifetime behavior.
- Do not rely on logging for essential program state changes.
- Profiling and tracing **SHOULD** use established structured instrumentation where available.
- Logging overhead in demonstrated hot paths **SHOULD** be measured when relevant.
- Diagnostic output **MUST** remain safe under the subsystem's threading and shutdown contracts.

#### Diagnostic Categories

| Category | Typical purpose |
|---|---|
| Error | Operation failed or a required condition could not be satisfied |
| Warning | Unexpected or degraded behavior that permits continued operation |
| Information | Meaningful lifecycle or operational event |
| Debug | Development-oriented state or behavior details |
| Trace | Fine-grained execution information |
| Profiling | Timings, counters, allocations, and performance measurements |

Use the categories and APIs actually provided by XII rather than introducing parallel naming conventions.

#### High-Frequency Logging

**Avoid:**

```cpp
void UpdateSimulation()
{
  xiiLog::Info("Simulation update started.");

  AdvanceSimulation();

  xiiLog::Info("Simulation update completed.");
}
```

If this function executes frequently, the logs may become noisy and expensive without providing useful diagnostic value.

Prefer profiling scopes or selectively enabled trace diagnostics when detailed timing or execution information is required.

#### Diagnostic Construction

A disabled log statement should ideally avoid unnecessary formatting and allocation work.

Follow the behavior of XII's logging macros and functions when determining whether argument evaluation or formatting occurs.

Do not assume every logging API lazily evaluates its arguments.

**Guiding principle:**

> Diagnostics must be useful and proportionate. Their cost should not dominate the operation they are intended to explain.

### 12.10 Security and Malformed Input

Data originating outside a trusted subsystem **MUST** be treated as potentially malformed, inconsistent, or adversarial.

This includes file contents, serialized objects, network packets, shader source, plugin-provided data, configuration files, and application arguments.

#### General Requirements

- Validate untrusted input before using it to calculate memory sizes, offsets, indices, or allocation requirements.
- Check integer overflow before arithmetic involving untrusted sizes, counts, strides, or dimensions.
- Validate indices and offsets against the actual bounds of the referenced storage.
- Validate declared lengths against available data before reading or copying.
- Establish reasonable limits for recursion depth, nesting, object counts, and resource consumption where required.
- Validate enum values and identifiers before interpreting them as trusted internal state.
- Reject unsupported or malformed encodings where the consuming interface requires valid text.
- Do not use assertions as the sole validation mechanism for untrusted input.
- Do not assume that successful parsing guarantees semantic validity.
- Resource allocation decisions **MUST** account for validated sizes and applicable memory limits.
- Error handling **MUST NOT** expose sensitive data unnecessarily.
- Input validation **SHOULD** occur at the boundary where untrusted data enters the responsible subsystem.
- Validation **MUST** remain consistent with the actual platform and serialization contracts.

#### Bounds Validation

**Prefer:**

```cpp
xiiResult ReadPayload(xiiArrayPtr<const xiiUInt8> pData, xiiUInt64 uiOffset, xiiUInt64 uiLength)
{
  const xiiUInt64 uiDataSize = pData.GetCount();

  if (uiOffset > uiDataSize)
    return XII_FAILURE;

  if (uiLength > uiDataSize - uiOffset)
    return XII_FAILURE;

  return ProcessPayload(pData, uiOffset, uiLength);
}
```

Subtracting after validating the offset avoids overflow that could occur in a direct `uiOffset + uiLength` bounds check.

The example assumes `ProcessPayload()` accepts these index widths and respects the validated range.

#### Resource Exhaustion

Malformed or adversarial inputs may request unreasonable amounts of memory or processing work without containing an explicitly invalid field.

Input boundaries **SHOULD** establish appropriate resource limits when such requests could affect system availability.

Examples include:

- Excessively large array counts.
- Unbounded recursive structures.
- Oversized shader permutations.
- Pathological serialized object graphs.
- Unexpectedly large resource dimensions.
- Repeated registration requests.
- Excessive string or allocation lengths.

These limits should reflect supported workloads rather than arbitrary restrictions that prevent valid use.

#### Parsing and Validation

Parsing establishes that input conforms to a syntactic representation.

Semantic validation must separately establish that the parsed values satisfy the requirements of the consuming operation.

For example, a configuration may parse successfully while specifying incompatible settings or values outside the supported domain.

Both stages may require distinct failure reporting.

**Guiding principle:**

> External data is not trustworthy merely because it can be parsed. Validate representation, bounds, resource requirements, and semantic constraints before use.

### 12.11 Platform and Third-Party Error Translation

Platform APIs and third-party libraries may expose error mechanisms that differ from XII's ordinary result conventions.

Integration boundaries **MUST** translate these mechanisms into stable engine-facing contracts where appropriate.

#### General Requirements

- Platform-specific error handling **SHOULD** remain within the corresponding implementation layer.
- Preserve relevant native error codes or diagnostic context when they are necessary for diagnosis.
- Do not expose arbitrary platform-specific error types through portable interfaces without a documented interoperability requirement.
- Translate recoverable failures into `xiiResult` or an appropriate engine-compatible result type.
- Exceptions encountered at unavoidable third-party or platform boundaries **MUST NOT** propagate through ordinary engine APIs.
- Error translation **MUST NOT** silently convert failure into success.
- Do not discard distinctions between native failures when callers require them for recovery.
- Device-loss, out-of-memory, and other significant platform failures **MUST** follow the owning subsystem's documented recovery policy.
- Platform diagnostics **SHOULD** identify the failing operation and the relevant native error category.
- Do not assume that native error codes have identical meanings across platforms or backends.

#### Native Error Context

A portable operation may expose a simple failure result while the platform implementation records richer native details.

For example, a filesystem operation may return `XII_FAILURE` while reporting the relevant platform error through the established diagnostic system.

When callers require programmatic access to native or categorized errors, use an explicit error representation rather than requiring them to parse log messages.

#### Exception Boundaries

Where a third-party API can throw exceptions and the integration environment supports catching them, exception handling must remain inside the isolated adapter.

When the build configuration does not support C++ exception handling, the adapter must use a suitable non-throwing interface or another explicitly supported integration strategy.

**Guiding principle:**

> Platform differences belong at platform boundaries. Preserve useful native diagnostics while maintaining consistent engine-facing failure semantics.

### 12.12 Fatal Errors and Unrecoverable States

Some failures invalidate fundamental engine assumptions and cannot be safely recovered from within the current execution context.

Such failures **MUST** be distinguished from ordinary operational errors.

#### General Requirements

- Fatal-error mechanisms **MUST** be reserved for conditions where continued execution cannot preserve required correctness or safety guarantees.
- Do not terminate execution for ordinary recoverable failures merely because recovery is inconvenient.
- Unrecoverable corruption **MUST NOT** be hidden behind a generic success or fallback result.
- Fatal diagnostics **SHOULD** preserve the most relevant context available without relying on invalid subsystem state.
- Do not perform unsafe cleanup after corruption has invalidated the invariants required by that cleanup.
- Fatal-error handling **MUST** respect the engine's established platform and diagnostic mechanisms.
- Do not invent independent termination policies within individual components without architectural justification.
- Distinguish local operation failure from failure requiring subsystem or process termination.

#### Recovery Scope

A failure may be unrecoverable for one operation while remaining recoverable at a higher level.

For example:

- A document parser may reject a malformed file.
- A resource manager may fail to load an asset.
- A graphics subsystem may need to rebuild resources following a supported device-recovery event.
- An invalid fundamental memory invariant may prevent safe continuation.

The appropriate response depends on the actual guarantees of the affected subsystem.

Do not classify every hardware or platform failure as fatal without evaluating the supported recovery contract.

**Guiding principle:**

> Fatal errors indicate that safe continuation cannot be guaranteed. Reserve them for genuine unrecoverable states rather than ordinary operational failure.

### 12.13 Error-Handling Tests and Failure Injection

Correctness testing **SHOULD** cover failure behavior, not only successful execution.

Failure paths often exercise ownership, cleanup, synchronization, and state transitions that ordinary tests do not reach.

#### General Requirements

- Test expected failure conditions for operations with explicit failure contracts.
- Verify that failed operations preserve documented invariants.
- Exercise partial initialization and cleanup where multiple resources are acquired.
- Test registration rollback when several authoritative or secondary structures must be updated.
- Validate stale-handle rejection and resource invalidation where applicable.
- Test malformed external inputs, including boundary values and inconsistent lengths.
- Verify that required operations do not continue after failed prerequisites.
- Test shutdown behavior when resources or tasks remain outstanding.
- Use failure injection when it provides a practical means of exercising otherwise rare error paths.
- Do not require external infrastructure failures to occur naturally before validating critical recovery behavior.
- Diagnostic tests **SHOULD** focus on meaningful error context and severity rather than unnecessarily brittle exact-message comparisons.
- Tests **MUST NOT** depend on debug-only assertions to establish runtime behavior required in release builds.

#### Failure Injection

Useful failure-injection scenarios may include:

- Allocation failure.
- File-open failure.
- Partial resource initialization.
- Invalid serialized data.
- Registration conflict.
- Resource exhaustion.
- Backend initialization failure.
- Asynchronous cancellation.
- Simulated device loss where supported.
- Subsystem shutdown during pending work.

Injected failures should reproduce the relevant contract without introducing unrelated undefined behavior.

#### Recovery Verification

A successful failure-handling test should establish more than the presence of an error result.

Where applicable, verify that:

1. The failure is observable to the caller.
2. Relevant diagnostic context is preserved.
3. No required dependent operation executes incorrectly.
4. Acquired resources are released or retained safely.
5. Registries and secondary mappings remain consistent.
6. The component reaches its documented post-failure state.
7. Retry or recovery behaves correctly when supported.

**Guiding principle:**

> Failure handling is part of implementation correctness. Test the guarantees that remain after an operation fails, not merely whether an error was returned.

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
