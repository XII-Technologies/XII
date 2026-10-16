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

`SetPipelineParameter` and `RemovePipelineParameter` provide view overrides.
Overrides replace values with the same top-level key; nested dictionaries are
replaced as a unit. The built-in `DisplayOutput` dictionary accepts the reflected
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
configuration properties, return a pin schema from `GetPins`, and implement `Build`.
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
outputs are checked after producer construction. Additional requirements such as
buffer layout, dimensions, access patterns, and domain policies belong to the pass's
validation or build implementation.

`Build` obtains input handles from its context, emits specialized render-graph work,
and publishes outputs with `SetOutput`. Execution delegates must retain owned pass
state or frame data, never the temporary build context. Read/write declarations on
the render graph remain essential: an order pin alone does not declare GPU resource
access. `SetPassPreparation` runs uploads after resource resolution and barriers,
before a managed native render pass begins.

The built-in catalog provides composite geometry, lighting, transparency, and screen
stages, plus individually connectable histogram, exposure, TAA, upscale, bloom, and
tone-mapping passes. Composite stages preserve existing internal device-dependent
choices and frame services. They use one shared built-in cache within each view;
duplicate instances of a built-in stage are rejected. Independent branched effects
use concrete pass types with their own state. The generic pipeline permits repeated
types and arbitrary acyclic topology. Built-in prerequisites are validated during
compilation; the authored Deferred resource explicitly supplies both ordering and pin edges.

## Lifetime, reload, and threads

Resources contain configuration and CPU metadata, never view pointers or GAL objects.
Compilation uses the Core directed multigraph and its iterative topological sort.
Successful loading publishes a complete immutable snapshot under a mutex. Existing
instances retain their snapshot until `Synchronize` at the next frame boundary.
A changed snapshot recreates pass objects and invalidates temporal history. Missing
or invalid resources clear executable state and skip rendering that view.

Instances and views require external synchronization. Synchronize or destroy an
instance only after its previous CPU execution delegates have finished. GAL objects
follow the device's deferred destruction rules. Built-in GPU caches belong to the
view resource context and survive topology changes for reuse; histories are invalidated
and the context is destroyed before device shutdown. Separate views own separate
caches and mutable pass objects. Parameters and bindings use retained storage.

## Serialization and tooling

The descriptor uses Foundation reflected object-graph serialization. Its versioned
envelope bounds payloads to 32 MiB and checks length and checksum before deserialization.
This is an engine-generated asset format, not an untrusted interchange parser.
Compiled `.xiiBinRenderPipeline` files contain the normal asset header, format version
3, and the descriptor envelope. Obsolete compiled versions produce a diagnostic
requesting re-transformation of the source asset.

The editor node manager obtains schemas from reflected pass prototypes. Built-in
creation templates configure their stage explicitly. Asset transformation constructs
the descriptor, validates it through the same compiler, and writes the resource format.
GraphicsCore and Core have no Qt dependencies. No new resource manager or asset system
is introduced.

## Cost and boundaries

Topology storage is O(V + E); pin schemas and bindings add O(P). Topological ordering
is O(V + E). Compilation also performs bounded pin searches and sorts connections in
O(E log E); built-in prerequisite validation can inspect O(V²) stage pairs. Limits are
4096 passes, 65536 connections, and 256 pins per pass. Frame construction follows a
precomputed flat order and connection intervals in O(V + E + P), excluding work inside
passes. It does not traverse the Core graph or recompile unchanged definitions.
The resource reports inclusive CPU memory estimates for topology, schemas, bindings,
and nested configuration; custom typed variant objects may own additional allocations
that cannot be inferred from reflection alone.

Cycles are rejected because render-pipeline construction requires dependency ordering.
Feedback uses pass-owned history from earlier frames. GPU scheduling and synchronization
are delegated to the render graph. The resource API does not imply thread-safe view
mutation, arbitrary reflection-plugin unloading, or automatic migration of obsolete
compiled binaries.

## Verification

`GraphicsCoreTest` covers compilation, invalid connectivity and formats, required
bindings, cycles, deterministic ordering, serialization corruption/truncation,
ResourceManager loading/reloading, retained snapshots, and isolated pass state.
`GraphicsFoundationTest` exercises two views sharing one resource with separate GPU
buffer readbacks, and the default offscreen pipeline over multiple frames with texture
readback. Validation is performed with the Vulkan backend enabled in the Windows
Debug workspace. Other backends and configurations require their own validation.

Validation on 2026-10-10: Debug builds of `GraphicsCoreTest`,
`GraphicsFoundationTest`, and `GpuDrivenScene` succeeded. All 17 GraphicsCore
subtests and all 40 GraphicsFoundation subtests passed, including both pipeline
GPU tests. The final Vulkan suite emitted no warnings or errors. Tests used
`-nosave -nogui -all`; no renderer filter was applied, so every compiled backend
was selected (this workspace enables Vulkan and disables D3D12).

That historical checkpoint preceded subsequent editor and Vulkan runtime repairs.
Current migration validation and remaining gates are tracked in
`Documentation/Rendering/EditorAuthoredPipelineMigration.md`,
`Documentation/Rendering/WorldRenderingMigration.md` and
`Documentation/Rendering/HierarchicalGraphComposition.md`; the historical results
above are not evidence for the current editor or runtime state. D3D12 is excluded
from the ongoing migration validation at the user's request.
