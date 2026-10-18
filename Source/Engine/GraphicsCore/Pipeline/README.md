# Resource-driven render pipelines

`xiiRenderPipelineResource` is a standard ResourceManager resource. Its immutable
`xiiRenderPipeline` compilation is shared; `xiiRenderPipelineInstance` owns mutable
pass objects and frame bindings for one view. `xiiRenderGraph` remains the GPU
execution system: it resolves resources, schedules work, inserts synchronization,
and executes command lists. Core's graph contains only structural dependencies.

## Asset selection

```cpp
auto resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(assetId);
view.SetRenderPipelineResource(resource);
```

Views start without a pipeline. Normal game and editor entry points resolve an
explicit resource reference or the corresponding reflected
`xiiRenderPipelineProfileConfig` policy in the application platform profile.
An explicit view reference takes precedence. Empty policy, unavailable resources
and invalid graphs fail with diagnostics; no fallback graph is constructed.
Game, scene editor, asset preview, texture preview and cube preview policies select
ordinary resources. Packaged editable sources and compiled SDK snapshots live in
`Data/Base/RenderPipelines`; project assets use ordinary asset GUID/cache loading.

Pipeline assets have a reflected `xiiRenderPipelineSettings` document object.
Clearing graph selection shows its properties in the existing inspector. Authored
required device features and output format/sample contracts constrain execution;
they never construct topology or substitute a pipeline. Referenced definitions'
device requirements accumulate in their containing resource. A definition's final
output contract applies when it is the root; subgraph boundaries use typed pin
contracts. The compiler validates settings without a GPU, and each runtime build
checks the selected device and output before constructing algorithm work. These
runtime checks use compiled values without reflection. Older resources lacking the
additive settings field retain unconstrained defaults, and older source documents
initialize a persistent settings object for their next save.

`SetPipelineParameter` and `RemovePipelineParameter` provide view overrides.
Overrides replace values with the same top-level key; nested dictionaries are
replaced as a unit. The `DisplayOutput` dictionary accepts the reflected
properties of `xiiDisplayOutputSettings`, including nested `Exposure` and
`ColorGrading` dictionaries. Values are validated before use and do not modify
the view's display defaults. Other parameter names belong to consuming passes.

`AddPass`, `RemovePass`, `Connect`, and `Disconnect` edit a descriptor. Pass indices
identify entries within that descriptor; removing a pass updates indices and
removes incident connections. Reconnect consumers when removing an intermediate
producer. Changes become active by publishing a replacement through ResourceManager,
not by mutating an existing compilation. `Compile` supports offline validation and
returns `xiiResult` plus an optional diagnostic; failure leaves its output unchanged.

## Passes and bindings

Derive a reflected concrete type from `xiiRenderPipelinePass`. Declare writable
configuration properties and concrete typed pin members through `XII_MEMBER_PROPERTY`,
and implement `Build`. The base `GetPins` discovers the reflected members; operations
must not maintain a parallel array of pin types.
Constructors, `Configure`, `GetPins`, and `ValidatePipeline` must be CPU-only because
compilation may run on a resource loading thread. A separate object is constructed
for every pass in every runtime instance. Consumers can retain arbitrary typed state
without placing it in the resource definition.

`GetProperties` captures writable reflected members, arrays, value maps, embedded
settings and owned settings objects for the asset compiler. `Configure` restores
the same representation when compiling or initializing an instance. Nested objects
retain their reflected property names; polymorphic owned objects also retain their
registered type. Borrowed object pointers and unsupported property shapes fail
instead of silently disappearing from the asset. Computed read-only properties
are excluded, and `ParameterOverrides` stays in the descriptor's separate
occurrence-local parameter scope. Owned settings follow their native type's normal
destruction rules and are recreated separately for each runtime instance.
Traversal is bounded to 32 nested object levels and 65536 properties/elements.
This reflection work occurs during export/configuration, not frame execution.

Pins describe textures, buffers, or construction ordering. Several edges may connect
the same pair of passes through different pins. Resource inputs have one producer;
order inputs allow multiple predecessors. Required inputs must be connected. Declared
texture formats must agree; `Unknown` permits format resolution at runtime. Input
handles and resolved formats are checked before consumer construction, and required
outputs are checked after producer construction. Reflected pin requirements can
narrow format, dimension, sample count, width/height and required GAL binding flags.
Zero/Undefined requirements accept the producer's description; these constraints
validate resources rather than silently resizing an algorithm's output. Native
requiredness, direction, resource kind, access metadata, cardinality and temporal
semantics remain owned by the pass. Buffer layout and algorithm-specific policies
still belong to its validation/build implementation.

Pin settings are persistent owned document objects selected through the generic
Qt pin inspector. Their schema metadata is read-only, with editable display names,
format requirements and structured resource requirements. Connections continue to
use stable schema names. Commands preserve connected edges, pin/property selection
and authored connection IDs through edits; nested settings, copy/paste, dynamic
pin changes and undo/redo use the normal document infrastructure. Older pipeline
documents initialize missing pin objects on load, and packaged source assets have
been migrated through the production serializer/compiler.

Compilation lowers authored pin objects into immutable contracts. Each contract
retains its source document identity and derives an occurrence identity from the
pipeline/node path. Subgraph interface constraints narrow the mapped internal pins
for that occurrence, including nested definitions and input fan-out. Conflicting
constraints fail rather than alter shared definitions. Explicit editor expansion
uses the same boundary map and reflected object-graph diff commands to preserve
settings. Runtime pass instances do not allocate authoring pin objects; frame
bindings validate the compiled descriptions, including directly published bindings.

`Build` obtains input handles from its context, emits specialized render-graph work,
and publishes outputs with `SetOutput`. Execution delegates must retain owned pass
state or frame data, never the temporary build context. Read/write declarations on
the render graph remain essential: an order pin alone does not declare GPU resource
access. `SetPassPreparation` runs uploads after resource resolution and barriers,
before a managed native render pass begins.

Independent operation types live in `Pipeline/Passes`, grouped by responsibility.
There is no built-in rendering catalog, composite stage selector or stage prerequisite
check. Repeated types and nested reusable definitions are supported. Ordinary authored
assets select algorithms and supply typed connections; compatible operations can share
GPU submissions or managed hardware render passes. The operation count does not imply
the submission count. See `Documentation/Rendering/NativePassInventory.md` for actual
conversion counts, deleted backends and algorithms still requiring rebuilding.

## Lifetime, reload, and threads

Resources contain configuration and CPU metadata, never view pointers or GAL objects.
Compilation uses the Core directed multigraph and its iterative topological sort.
Successful loading publishes a complete immutable snapshot under a mutex. Existing
instances retain their snapshot until `Synchronize` at the next frame boundary.
A changed snapshot recreates pass objects and invalidates temporal history. Missing
or invalid resources clear executable state and skip rendering that view.

Instances and views require external synchronization. Synchronize or destroy an
instance only after its previous CPU execution delegates have finished. GAL objects
follow the device's deferred destruction rules. Private pass caches, readback rings and
temporal histories belong to each runtime occurrence. Separate views and repeated
subgraphs receive separate mutable pass objects. Shared physical asset residency belongs
to existing device/resource services. Parameters and bindings use retained storage.
No hidden runtime initializes an unrelated rendering feature for a selected graph.

## Serialization and tooling

The descriptor uses Foundation reflected object-graph serialization. Its versioned
envelope bounds payloads to 32 MiB and checks length and checksum before deserialization.
This is an engine-generated asset format, not an untrusted interchange parser.
Compiled `.xiiBinRenderPipeline` files contain the normal asset header, format version
3, and the descriptor envelope. Obsolete compiled versions produce a diagnostic
requesting re-transformation of the source asset.

The editor node manager obtains schemas from reflected pass prototypes and member pins.
Node selection exposes reflected pass properties; pin selection exposes its persistent
configuration through the normal property grid and attribute handling. Subgraph references,
interfaces, overrides and explicit expansion use existing document commands. Asset
transformation constructs the descriptor, validates it through the same compiler, and
writes the resource format.
GraphicsCore and Core have no Qt dependencies. No new resource manager or asset system
is introduced.

## Cost and boundaries

Topology storage is O(V + E); pin schemas and bindings add O(P). Topological ordering
is O(V + E). Compilation also performs bounded pin searches and sorts connections in
O(E log E). Definition expansion is bounded and records source provenance. Limits are
4096 passes, 65536 connections, and 256 pins per pass. Frame construction follows a
precomputed flat order and connection intervals in O(V + E + P), excluding work inside
passes. It does not traverse the Core graph or recompile unchanged definitions.
The resource reports inclusive CPU memory estimates for topology, schemas, bindings,
and nested configuration; custom typed variant objects may own additional allocations
that cannot be inferred from reflection alone.

Intra-frame dependency cycles are rejected by the rendering consumer. Explicit temporal
pins describe pass-owned history from earlier frames and do not add intra-frame dependency
edges. The Core graph itself permits cycles. GPU scheduling and synchronization
are delegated to the render graph. The resource API does not imply thread-safe view
mutation, arbitrary reflection-plugin unloading, or automatic migration of obsolete
compiled binaries.

## Verification

`GraphicsCoreTest` covers compilation, invalid connectivity and formats, required
bindings, cycles, deterministic ordering, serialization corruption/truncation,
ResourceManager loading/reloading, retained snapshots, and isolated pass state.
`GraphicsFoundationTest` exercises native authored resources, explicit inputs, nested
instances, world extraction, GPU visibility/raster/feedback and private temporal state
with real GPU readbacks. `EditorFrameworkTest` checks Qt document commands, pin properties,
authoring and deterministic source/binary resources. `EditorEngineProcessFrameworkTest`
checks editor views, component lifetimes, LOD and skinning integration.

Current evidence and incomplete migration responsibilities are tracked in
`Documentation/Rendering/NativePassInventory.md`. Focused Vulkan suites pass, but the
full visible-geometry GpuDrivenScene sample still reports ErrorDeviceLost; it is not
runtime-verified. D3D12 is excluded at the user's request. Historical tests of the
removed composite rendering path do not establish correctness of the native rebuild.
