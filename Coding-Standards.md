# XII Engine C++ Coding Standards and Engineering Guidelines

This guide defines what readable, correct, robust, maintainable, and performance-conscious C++ looks like in XII. A style guide cannot replace design review, tests, benchmarks, platform documentation, or sound engineering judgment. Its purpose is to make routine decisions predictable and to make exceptions explicit.

**Precedence when rules differ:** correctness and portability come first; then documented public API and serialized-format compatibility; then the repository's root `.clang-format`, `.editorconfig`, build settings, and target-specific conventions; then this document; then local neighboring code for cases this document leaves open. Do not reformat unrelated code merely to apply a new preference. This document does **not** claim to reproduce exact `.clang-format` or `.editorconfig` values, which must be read from the target checkout.

The words **MUST**, **SHOULD**, and **MAY** indicate mandatory rules, expected defaults (exceptions need a reason), and permitted alternatives. XII-specific APIs shown in examples are taken from the supplied engine code where possible. Examples labeled **illustrative** show a design pattern and may require adaptation to the exact current API.

---

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

---


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

Any reviewer should be able to answer:

- Who creates this object?
- Who destroys it?
- Can it outlive the object or system it references?
- What invalidates its pointers, views, iterators, handles, or GPU resources?
- Who may access it concurrently?

Names and types should communicate those answers where feasible. Use handles for independently managed, generation-checked objects; smart pointers for explicit ownership; and non-owning pointers or views for borrowed access.

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

### 1.4 Think across engine boundaries

A change in GraphicsCore can affect GraphicsFoundation, platform backends, plugins, tools, render pipelines, editors, reflection, serialization, and tests. Changes to shared headers, exported symbols, enums, handles, shaders, and binary formats require compatibility review beyond compilation of the edited file.

### 1.5 Optimize from evidence

Use profiling data, measured allocations, representative workloads, and correctness tests. Do not automatically trade clarity for speculative speed. Conversely, do not ignore allocations, virtual dispatch, cache misses, or synchronization in a proven hot path.

---

## 2. Source layout and formatting

### 2.1 Formatting authority

The root `.clang-format` is the authority for formatting that the tool can express. `.editorconfig` governs applicable whitespace, line endings, indentation, and encoding. Do not manually invent alignment rules that contradict either file.

- **MUST** format modified C++ code with the repository's configured formatter/version.
- **MUST NOT** include unrelated mass formatting in a functional pull request.
- **SHOULD** preserve the codebase's established brace and spacing conventions where not automated.
- **MUST** avoid trailing whitespace, accidental mixed line endings, and unresolved conflict markers.
- Source text should be UTF-8 where supported by the repository; do not change encodings without checking tooling and file-format constraints.

### 2.2 Braces and indentation

XII examples use Allman-style braces for function/class definitions and multi-line control blocks. Follow the formatter for exact brace placement.

```cpp
class xiiRenderGraphRegistry
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

Short guarded early returns often omit braces in neighboring XII code:

```cpp
if (pDevice == nullptr)
  return XII_FAILURE;
```

For multi-statement branches, nested conditions, and anything likely to grow, prefer braces:

```cpp
if (bNeedsRebuild)
{
  ResetGraph();
  RebuildResources();
}
```

Never write ambiguous nested unbraced `if` chains:

```cpp
// Avoid: the else binds to the innermost if.
if (bVisible)
  if (bReady)
    Render();
  else
    LogNotReady();
```

### 2.3 Alignment and spacing

Keep related initialization and member declarations readable. The formatter decides exact columns.

```cpp
xiiUInt32 m_uiFrameIndex = 0U;
xiiUInt32 m_uiPassCount  = 0U;
bool      m_bActive      = false;
```

Avoid arranging distant or unrelated statements into artificially wide columns. Maintain blank lines between logically separate operations, not after every statement.

### 2.4 Statement structure

- One declaration per line when each name has independent meaning or initialization.
- Avoid assignment inside conditions unless it materially improves clarity.
- Use parentheses to disambiguate complex conditions.
- Do not combine unrelated side effects on one line.

```cpp
const xiiUInt32 uiWidth  = targetSize.width;
const xiiUInt32 uiHeight = targetSize.height;

if (uiWidth == 0U || uiHeight == 0U)
  return XII_FAILURE;
```

### 2.5 Includes and whitespace

Use the repository's existing include ordering. Common XII implementation files include their module PCH first, then grouping by Foundation, Core, GraphicsFoundation, GraphicsCore, and application-specific headers. Keep one include per line and avoid redundant includes. A public header must include enough to compile independently; it must not depend on an implementation PCH accidentally supplying types.

```cpp
#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/IdTable.h>
#include <Foundation/Threading/Mutex.h>

#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
```

### 2.6 Section separators

Use section dividers when they help navigate a substantial implementation. Avoid decorative separators around every tiny helper.

```cpp
//////////////////////////////////////////////////////////////////////////

xiiRenderGraphFrame::xiiRenderGraphFrame() = default;

xiiRenderGraphFrame::~xiiRenderGraphFrame()
{
  End();
}
```

### 2.7 File endings and comments

- Use a newline at end of file.
- Do not leave commented-out old implementations or `#if 0` blocks indefinitely.
- Keep copyright/license notices in the repository's established form; do not copy a contributor's personal copyright into unrelated files without authorization.
- Keep the engine's required static-link registration macro where relevant; see §17.

---

## 3. Naming conventions

**Preserved baseline rule:** Type-dependent prefixes are **mandatory for member variables only** when an applicable type is known. Local variables and parameters may use type-dependent prefixes, and many XII call sites do, but the original policy explicitly leaves their naming to the programmer's discretion. The examples below often use prefixes for locals for consistency, **not** to silently make them mandatory.

### 3.1 Type and symbol names

| Symbol | Convention | Example |
|---|---|---|
| Engine class or struct | `xii` + PascalCase | `xiiRenderGraphManager` |
| Enum wrapper type | `xii` + PascalCase | `xiiRenderGraphQueue` |
| Enum values | PascalCase | `EveryFrame`, `Graphics` |
| Functions / methods | PascalCase | `CreateGraph()`, `BeginFrame()` |
| Accessors | `Get`, `Is`, `Has`, `Can`, `Set` as appropriate | `GetFrameIndex()`, `IsActive()` |
| Local names / parameters | Descriptive; type prefixes optional | `uiFrameIndex`, `frameIndex` |
| Private/protected instance fields | `m_` + type prefix when applicable | `m_uiFrameIndex`, `m_pGraph` |
| Private/protected mutable static fields | `s_` + type prefix when applicable | `s_pState`, `s_uiCount` |
| Private/protected static constants | PascalCase, ideally `constexpr` | `MaxPassCount` |
| Macros / compile switches | Established `XII_` convention | `XII_ASSERT_DEV`, `XII_ENABLED` |
| Template parameters | Semantic PascalCase | `IdTableType`, `ReturnType` |
| Files | Name for primary type/module | `RenderGraphManager.h` / `.cpp` |
| Namespaces | Consistent with owning module | `xiiInternal` |

Avoid abbreviations that require private knowledge. Established terms such as `GPU`, `GAL`, `ID`, `SRV`, `UAV`, `RT`, `UI`, and `API` are acceptable when consistent with neighboring APIs.

### 3.2 Type-dependent member prefixes

| Type family | Prefix | Examples |
|---|---|---|
| Signed integers, including signed atomic integers and `ptrdiff_t` | `i` | `m_iPriority`, `s_iLogLevel` |
| Unsigned integers, including `size_t` | `ui` | `m_uiFrameIndex`, `m_uiCount` |
| `float`, `double` | `f` | `m_fExposure`, `m_fDeltaSeconds` |
| Booleans / atomic booleans | `b` | `m_bFrameActive`, `m_bInitialized` |
| Handle types | `h` | `m_hGraph`, `m_hTexture` |
| Raw and smart pointers | `p` | `m_pGraph`, `m_pDevice` |
| NUL-terminated C strings | `sz` | `m_szDisplayName` |
| Non-NUL-terminated character pointers | `p` | `m_pTextBegin` |
| XII string objects / string views | `s` | `m_sName`, `m_sPath` |
| Vector / SIMD vector | `v` | `m_vPosition`, `m_vVelocity` |
| Quaternion | `q` | `m_qRotation` |
| Matrix / SIMD matrix | `m` | `m_mTransform` |
| Other types | No type prefix beyond member/static marker | `m_Blackboard`, `m_Description` |

Use the **semantic type**, not a convenient but misleading prefix. A 64-bit GPU address is not a `p` pointer unless represented as an actual pointer; a texture handle is `h`, not `p`. `xiiSharedPtr` and `xiiUniquePtr` use `p` because they express pointer ownership/access.

```cpp
class xiiRendererState
{
private:
  xiiUInt64                     m_uiFrameIndex = 0U;
  xiiInt32                      m_iPriority   = 0;
  bool                          m_bEnabled    = true;
  float                         m_fExposure   = 1.0f;
  xiiHashedString               m_sName;
  xiiRenderGraphHandle          m_hGraph;
  xiiUniquePtr<xiiRenderGraph>  m_pGraph;
  xiiRenderGraphBlackboard      m_Blackboard;
  xiiVec3                       m_vPosition;
  xiiQuat                       m_qRotation;
  xiiMat4                       m_mWorldToClip;
};
```

The `m_mWorldToClip` spelling is intentional: the first `m_` means *member*; the second `m` means *matrix*.

### 3.3 Public fields

The existing XII guidelines do **not** require an `m_` prefix for public non-static data members. Existing public engine data structures frequently use `m_` anyway. For **new POD/configuration-style structures**, prefer the surrounding module's established style and be consistent across the structure. Do not rename public fields merely for aesthetics; such names can be referenced by serialization, reflection, scripts, plugins, and application code.

```cpp
struct xiiRenderGraphCompileSettings
{
  bool m_bEnablePassCulling  = true;
  bool m_bEnableCompileCache = true;
};
```

### 3.4 Private/protected and static fields

```cpp
class xiiCounter
{
public:
  [[nodiscard]] xiiUInt32 GetCount() const { return m_uiCount; }

private:
  static constexpr xiiUInt32 MaxCount = 1024U;
  static xiiUInt32           s_uiLiveCounters;

  xiiUInt32 m_uiCount = 0U;
};
```

Existing rules exempt **public static fields** from the private-static naming requirement. Nevertheless, do not expose mutable global state casually. Prefer named constants or controlled APIs.

### 3.5 Fixed-size arrays: intentionally flexible

The original standard allows a relaxed prefix for fixed-size arrays:

```cpp
bool m_bActiveFlags[3]; // Consistent typed field form.
bool mActiveFlags[3];   // Also permitted by the existing array exception.
```

Prefer one style within a class. For new fields, `m_bActiveFlags` is easier to recognize, but this is a recommendation, **not** a retroactive mandatory rule.

### 3.6 Boolean and query names

Use names that read naturally in a condition:

```cpp
if (m_bFrameActive && pDevice != nullptr)
{
  SubmitFrame();
}
```

Avoid negations of negations and ambiguous state terms such as `m_bValidFlag` or `m_bStatus`. Favor `IsReady()`, `HasResources()`, or `CanExecute()` when implementing boolean queries.

### 3.7 Names must distinguish identity, units, and scope

Use names that distinguish context handles from graph handles, a frame index from a submission index, bytes from elements, and GPU completion from CPU submission.

```cpp
xiiRenderGraphHandle        hGraph;
xiiRenderGraphTextureHandle hTexture;
xiiUInt64                   uiFrameIndex;
xiiUInt64                   uiCompletedFrame;
xiiUInt32                   uiUploadBudgetBytes;
```

Name an object for what it **is**, not for an implementation detail that may change. Do not call an execution index a *priority* unless higher/lower ordering semantics are explicitly documented.

### 3.8 Abbreviations, acronyms, and spelling

- Prefer unambiguous names like `GetCompletedFrame()` to `GetCF()`.
- Preserve stable engine terminology and standard graphics acronyms.
- Avoid inventing synonyms for the same concept (`graph registration`, `entry`, `instance`) in one API unless they mean different things.
- Spell identifiers and documentation correctly, especially public APIs and diagnostics.

---

## 4. File organization and headers

### 4.1 One clear primary responsibility per file

Pair public declarations with implementation files in the same module layout whenever practical:

```text
GraphicsCore/
  Pipeline/
    RenderGraphManager.h
    Implementation/
      RenderGraphManager.cpp
```

Do not move types into a generic `Utilities` file solely because they are small. Group helpers with their owner unless they have legitimate independent reuse.

### 4.2 Header self-sufficiency

Public headers must compile when included alone, without relying on a PCH or accidental include order. Include definitions for fields held by value; forward declare types used only via pointers/references where legal and useful.

```cpp
#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class xiiRenderGraphResourceCache;
class xiiRenderGraphTimestampProfiler;
```

A by-value field needs the complete type at the point where it is declared. Some smart-pointer declarations may be possible with incomplete types, but ensure the destructor and instantiation sites have full definitions.

### 4.3 `#pragma once` and include guards

Follow the established XII header convention (`#pragma once`). Do not combine guard styles unnecessarily. Include headers by their module-root paths instead of fragile relative traversals.

### 4.4 Header exposure and include hygiene

- Include what you use; do not depend on transitive includes.
- Keep backend-only headers out of portable public interfaces.
- Do not include a heavyweight implementation header when a forward declaration is enough.
- Avoid public headers that pull in platform SDKs or third-party libraries without an explicit API reason.
- Resolve cyclic dependencies by splitting interfaces, forwarding declarations, or moving implementation details to `.cpp` files—not by changing include order randomly.

### 4.5 Internal linkage and translation-unit helpers

For helpers used only by one `.cpp`, prefer an unnamed namespace or `static` according to local convention. Do not export helpers unnecessarily.

```cpp
namespace
{
  [[nodiscard]] bool IsValidInterval(xiiUInt32 uiInterval)
  {
    return uiInterval > 0U;
  }
}
```

### 4.6 Templates and inline code

Put template definitions where instantiations can see them, typically in headers or `.inl` files following module patterns. Do not hide a template body in a `.cpp` unless you explicitly instantiate every supported specialization.

Keep non-template method bodies in `.cpp` by default. Small, stable accessors may be inline if consistent with surrounding code; don't turn substantial implementations into header-only code just to avoid file navigation.

### 4.7 Export boundaries

Use the correct module DLL/export macro on exported public declarations, as in:

```cpp
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager;
```

Check symbol visibility, platform ABI, and plugin compatibility before exporting classes that contain implementation-heavy STL/custom container members. An ABI-stable façade or handle API may be more appropriate across independently versioned boundaries.

### 4.8 Implementation PCH

Implementation files may include their module PCH first if that module uses PCH, but must also include their own public declaration and direct dependencies as needed. PCH is a build accelerator, not an implicit dependency declaration.

---

## 5. Types, classes, and interfaces

### 5.1 `struct` versus `class`

Use `struct` for transparent records whose members form the public data contract. Use `class` for types with behavioral invariants, encapsulated state, significant lifecycle logic, or controlled mutation. This is an intention convention, not a layout or performance distinction.

```cpp
struct xiiRenderGraphRegistrationOptions
{
  xiiInt32 m_iPriority = 0;
};

class xiiRenderGraphManager
{
public:
  static xiiResult DestroyGraph(xiiRenderGraphHandle hGraph);

private:
  xiiRenderGraphManager() = delete;
};
```

If a `struct` grows invariants that must be protected, reconsider whether its public fields still make sense.

### 5.2 Data should start in a valid state

Give members meaningful defaults or initialize them in the constructor. Do not assume a user of a public configuration record will fill in every field.

```cpp
struct xiiGpuWorkDescription
{
  xiiUInt32 m_uiFramesInFlight = 3U;
  bool      m_bEnableProfiling = false;
};
```

Avoid ambiguous sentinel values unless they are defined and checked. Do not depend on zero-initialization of an uninitialized automatic object.

### 5.3 Constructors

- Prefer default member initializers for invariant defaults.
- Use member initializer lists for values requiring constructor parameters, references, const members, or allocator construction.
- Mark converting single-argument constructors `explicit` unless conversion is an intentional, documented part of the type.
- Never call virtual functions expecting derived behavior from a constructor or destructor.

```cpp
class xiiFrameRing
{
public:
  explicit xiiFrameRing(xiiUInt32 uiCapacity) :
    m_uiCapacity(uiCapacity)
  {
    XII_ASSERT_DEV(uiCapacity > 0U, "Frame ring capacity must be positive.");
  }

private:
  xiiUInt32 m_uiCapacity = 0U;
};
```

Prefer `= default` and `= delete` to unnecessary empty bodies or hidden declarations when compatible with established macros.

### 5.4 Destructors and RAII

Resources acquired by an object should be released deterministically, normally in its destructor. A destructor should not unexpectedly fail or perform unbounded waiting unless its contract explicitly says so.

```cpp
xiiRenderGraphFrame::~xiiRenderGraphFrame()
{
  End();
}
```

RAII guards should own a single well-defined responsibility. Ensure an explicit `End()` can safely be followed by destruction without ending twice.

### 5.5 Copy and move semantics

Choose deliberately:

- Value-like types: copying is fine when meaningful and reasonably cheap.
- Unique owners: noncopyable, movable when safe.
- Identity-bearing managers, mutex owners, device handles: copying usually makes no sense.
- Do not make a type movable if moving it would invalidate registered addresses, synchronization primitives, or callbacks into itself.

```cpp
class xiiGpuFrameScope
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuFrameScope);

public:
  xiiGpuFrameScope() = default;
  ~xiiGpuFrameScope();
};
```

If the project macro disallows only copying, verify whether moves are implicitly disabled or must also be explicitly deleted. Do not infer move behavior from the macro's name alone.

### 5.6 Inheritance and virtual dispatch

- Use inheritance for a genuine substitutability relationship or an established engine extension interface, not merely to reuse implementation.
- Mark overrides with `override`; use `final` only when further overriding must be prohibited.
- Give polymorphic bases an appropriate virtual destructor when deletion through the base is permitted.
- Minimize virtual calls in tight CPU loops when profiling shows a real cost; do not replace clean polymorphism with opaque type switches by default.

```cpp
class xiiRendererPlugin : public xiiRendererInterface
{
public:
  void Initialize() override;
  void Shutdown() override;
};
```

### 5.7 Interface granularity

An interface should expose capabilities, not containers or incidental state. Avoid publishing raw pointers to mutable manager-owned objects unless access is scoped and lifetime guaranteed.

```cpp
// Preferred manager operation:
[[nodiscard]] static xiiResult ExecuteGraph(
  const xiiRenderGraphFrame& frame,
  xiiRenderGraphHandle hGraph,
  const xiiView* pView);

// Risky as a general public API:
// static xiiDynamicArray<Entry*>& GetMutableEntries();
```

### 5.8 Encapsulation and invariants

A private field is not automatically safe if callers can obtain a mutable reference to it indefinitely. Use methods that preserve state transitions and invariants, particularly for frame, graph, resource, and thread lifecycle state.

### 5.9 Abstract ownership from execution policy

Do not conflate *who owns an object* with *when it runs*. A manager may own a graph without automatically executing it. Adding an object to a registry does not have to imply participation in every frame.

### 5.10 Favor composition

Use composition for owned utilities, allocators, profilers, caches, and synchronization. Avoid deep inheritance solely to share common member declarations.

---

## 6. Initialization, ownership, and lifetime

### 6.1 The ownership rule

Every allocation and long-lived resource **MUST** have an identifiable owner. Observers never silently acquire ownership. Use `xiiUniquePtr` when unique heap ownership is appropriate and `xiiSharedPtr` when shared ownership is required by the system's semantics.

```cpp
struct Entry
{
  xiiUniquePtr<xiiRenderGraph> m_pGraph;
  xiiRenderGraphBlackboard     m_Blackboard;
};
```

Do not use shared ownership as a default workaround for uncertain lifetime. Shared ownership makes destruction timing harder to reason about, especially around GPU resources and plugin unload.

### 6.2 Allocation must match the allocator

Use XII allocation conventions consistently. The allocator used for destruction must be compatible with the allocator used for construction. Do not mix `XII_NEW`/`XII_DELETE` with unrelated `new`/`delete` arbitrarily.

```cpp
xiiUniquePtr<xiiRenderGraph> pGraph =
  XII_NEW(pAllocator, xiiRenderGraph);
```

When using custom allocators, pass the allocator explicitly to objects that need it instead of looking up a global singleton in their constructor.

```cpp
class xiiGraphStorage
{
public:
  explicit xiiGraphStorage(xiiAllocator* pAllocator) :
    m_Entries(pAllocator)
  {
  }

private:
  xiiDynamicArray<xiiUInt32> m_Entries;
};
```

### 6.3 Borrowing is not ownership

A raw pointer from `Borrow()` provides access according to the owner’s lifetime contract; it is not a transferable lifetime guarantee.

```cpp
xiiGALDevice* pDevice = m_pDevice.Borrow();
if (pDevice == nullptr)
  return XII_FAILURE;
```

Do not store that pointer longer than the owner can promise it remains valid.

### 6.4 Stable identity is not stable address

A handle identifies an object through an authoritative registry. A pointer identifies one memory address. They serve different roles and have different failure modes.

**Wrong:**

```cpp
xiiDynamicArray<Entry> m_Entries;
xiiIdTable<GraphId, Entry*, xiiLocalAllocatorWrapper> m_GraphTable;

// Pointers into m_Entries can be invalidated by growth/removal.
```

**Better:**

```cpp
xiiDynamicArray<xiiUniquePtr<Entry>> m_Entries;
// Entry allocations remain at stable addresses while alive.
```

A stable address is still unsafe after destruction. A generation-checked handle protects lookup from some stale-ID errors, not concurrent use-after-free of pointers returned from the table.

### 6.5 Lifetime-safe lookups

Never mistake a lookup mutex for an object-lifetime guarantee.

```cpp
Entry* GetEntry(xiiRenderGraphHandle hGraph)
{
  XII_LOCK(m_Mutex);

  Entry* pEntry = nullptr;
  m_GraphTable.TryGetValue(hGraph, pEntry);
  return pEntry; // Unsafe if another thread can destroy it now.
}
```

Options include locking across the whole operation, returning a scoped access guard, maintaining stable shared ownership where justified, or making registration/destruction occur only at an execution-safe boundary. Document the selected rule.

### 6.6 Parent-child lifetimes

Destroy children before parents if the children depend on parent infrastructure. In render systems, release graph-specific objects before destroying the device-scoped resource cache or GAL device. Ensure plugin-owned delegates are removed before unloading their code.

### 6.7 Resource invalidation

Call `Invalidate()` on public handles after successful destruction when the API supports it, but do not rely on the local invalidation to revoke copies of a handle elsewhere. The registry must reject generation-stale handles.

### 6.8 Use-after-scope with views

`xiiStringView`, array views, spans, borrowed string storage, callbacks referencing stack variables, and GPU mapping helpers are non-owning or scoped. Never persist them without copying or extending ownership appropriately.

```cpp
// Avoid returning a view into a temporary string.
// xiiStringView MakeName() { xiiString sName = ...; return sName; }
```

### 6.9 Runtime and GPU lifetime differ

Completing CPU-side `Execute()` or destroying an object does not prove the GPU has finished reading its resources. Use queue completion, frame completion, or explicit fences/timelines to determine safe retirement. See §15.

---

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
