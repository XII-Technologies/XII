# Hierarchical graphs and hybrid rendering extraction

## Existing contracts (inspected before implementation)

* `Core/Graph/Graph.h` owns runtime handles and adjacency. `GraphMutation.cpp`
  transfers topology, but leaves validators and subscriptions on their original
  graph objects. `GraphData.h` consumes explicit compaction maps. No composition
  bookkeeping belongs in these mutation paths.
* `GraphSnapshot` already serializes portable topology indices and explicitly
  produces new runtime handles when applied. Document UUIDs must be stored beside
  this topology, never inferred from a runtime handle.
* `RenderPipelineCompiler.cpp` configures reflected CPU prototypes, validates
  typed pins and topologically sorts Core topology. `RenderPipelineInstance`
  allocates separate mutable pass objects per view. The specialized `RenderGraph`
  already handles GPU hazards, queues, resource lifetimes, culling and submission
  batching. Its cache flag originally recorded a signature without reusing an
  execution plan. Hierarchical authoring feeds these existing paths.
* The render pipeline editor uses `DocumentNodeManager`, reflected native pass
  schemas, document UUIDs, object mirroring and ResourceManager assets.
* `RenderWorldModule` already extracts from `xiiWorld` and caches static component
  packets. Components invalidate that cache. The render world module packs those
  packets into stable GPU identities and compact draw arrays. These are derived
  data, not an independent authoritative scene.

## Implementation phases and verification

1. Add explicit, non-owning selected/induced graph views. Membership queries
   validate against the source. Refresh and remapping are explicit, so creating
   views never adds work to ordinary graph mutation. Test removal, reconnect,
   move, compaction and selected versus induced edges.
2. Add domain-neutral composition definitions using existing topology snapshots,
   persistent document UUIDs, named boundary mappings, shared definition
   references, instance parameters and path-targeted property overrides. Expand
   transactionally into `xiiGraph` with explicit provenance and boundary maps.
   Bound expansion and detect definition recursion; do not reject topology cycles.
   Test nested/repeated instances, parallel edges, overrides, deterministic
   expansion, missing definitions and recursion.
3. Adapt reflected render pipeline descriptors and compilation to Core expansion.
   Keep flattened execution and separate pass objects. Preserve source instance
   paths for diagnostics. Integrate references/interfaces with editor node assets
   and asset dependencies. Test serialization, remapping, independent histories
   and rendering-domain cycle policy.
4. Track definition dependencies and classified revisions separately from runtime
   handles. Presentation changes do not invalidate compilation. Runtime parameter
   changes must not invalidate resource allocation plans; pin/resource/topology
   changes do. Publish immutable compiled snapshots and replace them only at CPU
   frame boundaries. Test hot reload and selective dependent invalidation.
5. Declare pass extraction requirements, aggregate active view demand in the
   existing world module and skip unused categories before component dispatch.
   Preserve static invalidation, stable GPU identities and dirty uploads. Lazily
   initialize view features. Test component changes and specialized pipelines.

Each phase is committed independently after focused checks. Build CoreTest,
GraphicsCoreTest and affected editor consumers; run CPU tests and available GPU
integration tests where runtime behavior changes. Record actual results below.

## Ownership and compilation policies

Definitions and immutable compilations may be shared. Mutable pass objects,
history and bindings belong to one expanded occurrence in one view. World modules
own scene-derived CPU caches; view contexts own visibility and view-dependent
GPU data; device subsystems own residency, allocation and deferred destruction.
Document identity is `(definition UUID, instance UUID path, node UUID)`. Runtime
handles are valid only in their owning graph and need explicit remapping.

Topology cycles are legal in Core. Rendering rejects intra-frame dependency
cycles. Temporal feedback must use explicit previous-frame resources owned by a
pass occurrence, never a current-frame connection disguised as a history edge.
Definition recursion is a different error from a topology cycle.

Reflected pass count is not submission count. Flattened reflected passes build
the existing specialized render graph, whose compiler may batch compatible
operations while retaining declared resource hazards and side effects. Frame
bindings, transient allocations, callbacks and temporal resources must never be
cached as shared authoring data.

## Core composition API

`xiiGraphView` selects graph-owned handles without subscribing to graph events.
Selected views keep only explicitly selected edges; induced views discover current
edges between selected vertices. Membership validates current generations and
endpoints. `Refresh()` discards stale memberships; `Remap()` explicitly follows
copy/compaction maps; `CopyTo()` produces an independent graph and handle map.
Views stay attached to the graph object when topology moves. The source must
outlive its views. No ordinary graph mutation enumerates views or memberships.

`xiiGraphDefinition` pairs `xiiGraphSnapshot` indices with persistent document
UUIDs, optional consumer type/name keys and property dictionaries. Ports on plain
topology edges and internal boundary endpoints are optional. Only consumers such
as rendering require typed, named pins. A node with a definition UUID is a shared
reference; a node without one is a leaf. Named interfaces map to internal node
UUIDs and optional ports. Inputs can fan out; outputs have one unambiguous mapping.

`xiiGraphComposition::Expand()` resolves references into a fresh topology and
configured leaf records. It detects definition recursion, bounds depth, instance,
vertex and edge counts, and publishes only after validation succeeds. It preserves
ordinary topology cycles. The result records each source definition, element and
instance UUID path, plus every occurrence's flattened boundary endpoints. After
explicit compaction, call `xiiGraphExpansion::Remap()` with the returned map.

Definition defaults flow into descendants; explicit instance parameters override
inherited defaults at that level. Property overrides target complete relative
UUID paths ending at leaves. Each instance has one override record per target
path; outer instance overrides take precedence over inner ones. Missing targets
fail expansion instead of silently discarding configuration.

`xiiGraphCompositionCache` publishes immutable reference-counted expansion
snapshots. Publishers advance classified revisions when changing definitions;
replacing a document without a finer classification advances its definition
revision. Only reachable definitions become dependencies. Presentation revisions
do not invalidate execution snapshots. Previously held snapshots remain usable
after successful replacement or a failed compilation. Cache access and definition
publication require caller synchronization. Inspect output sidecars after expansion
returns, rather than during the destination graph's reset notifications.

## Reflected assets and execution

Pipeline descriptors support shared embedded definitions and independent resource
references. Native editor pass/document UUIDs become the persistent identities;
connection indices remain local to a serialized topology snapshot. Export sorts
document objects and connections by UUID. Serialization seeds object identities
from the document UUID, and tests verify repeatable bytes and execution ordering.
Editor node positions already remain outside the asset transformation hash.

`xiiRenderPipelineSubgraphPass` is an authoring reference node, with an asset
picker, parameter map and reflected override path records. Its pins are inferred
from the referenced, configured native pass schemas, including nesting and
overrides. `xiiRenderPipelineInterfacePass` connects external inputs/outputs to
internal pins through normal editor connections. Export lowers both authoring
node types to Core references and boundary mappings; they never execute as passes.
The context-menu expansion command creates configured leaf nodes, remaps internal
and external connections, preserves scoped parameters, and uses one undoable
transaction with rollback on failure.

Resource loading first publishes immutable authoring snapshots. Compilation then
acquires only reachable definition resources, without recursively compiling them
during loading. Definition recursion therefore produces a Core diagnostic instead
of a resource-loading deadlock. A parent checks captured child snapshots at its
next view synchronization and recompiles when a reachable child is replaced.
Unreferenced definitions do not become runtime dependencies. Existing executable
snapshots remain alive for their owners; failed synchronization drops stale view
execution state. Asset-browser transform/package dependencies also participate in
the existing editor asset curator.

The compiler expands before configuring reflected prototypes, validating native
resource schemas, checking all interface fan-out, and sorting current-frame
dependencies. It retains original UUID provenance and includes pass/instance paths
in diagnostics. Runtime traversal uses compiled arrays and binding offsets.

Every expanded occurrence in every view gets distinct pass objects, parameters,
bindings and scoped blackboards. Named resources are scoped by instance UUID path.
Blackboard writes stay local, with read-only parent inputs; execute/preparation
callbacks receive the same occurrence's blackboard used during construction.
Built-in occurrences also own separate temporal, lighting and visibility state.
Geometry/material uploads and world ray-tracing preparation run once per view
graph. Explicit imports of the same physical resource are unified so cross-instance
service accesses retain resource hazards, rather than creating hidden aliases.

Temporal outputs are explicitly marked by the native pin schema with
`m_bTemporalHistoryOutput`. `BuildTemporalHistory()` imports occurrence-owned
previous-frame resources before current-frame `Build()` calls. History connections
participate in binding validation but do not create current-frame ordering arcs.
Ordinary cycles still fail. Current-frame Build cannot replace temporal outputs.
Optional history may be absent initially; successful execution promotes history,
and camera cuts/failures invalidate it. The editor applies this same cycle policy,
including explicit self feedback, while other node graph consumers retain their
existing self-connection policy.

## Cache boundaries

| Change | Work retained or recomputed |
| --- | --- |
| Editor position/presentation | No asset transform from node position; Core execution cache ignores presentation revisions. |
| View/instance runtime parameters | Current parameter dictionaries reach owned pass objects; unchanged emitted topology reuses ordering/culling. |
| Resource format, size or allocation flags | Current descriptions and lifetimes are re-evaluated; resource cache leases use those descriptions. |
| Resource reads/writes, export roots, side effects or culling policy | Exact structural records invalidate topology analysis. |
| Reachable definition replacement | Parent authoring compilation is replaced at CPU frame synchronization; held immutable snapshots remain valid. |

The render graph cache stores only ordering and culling records, checked against
exact structural data rather than relying on hash equality. Every hit reconstructs
compiled pass records from the current callbacks and pass data. Queue scheduling,
imported states, resource lifetimes, barriers and native render-pass materialization
remain current. This intentionally avoids caching pointers or prior GPU state.
More aggressive lifetime/barrier caching would require additional resource,
queue/device and imported-state dependencies. The existing resource cache handles
physical reuse and deferred lifetime independently of authoring compilation.

A representative test expands 100 shared instances into 200 reflected passes,
100 isolated resources and one queue submission. Pass count does not determine
GPU submission count or native hardware render-pass count; the specialized graph
retains its existing scheduling, merging and side-effect rules.

## Hybrid extraction and ownership

Pass types independently declare CPU scene categories and GPU service requirements.
A CPU geometry/light consumer can request scene packets without initializing a GPU
scene or lighting system. Native built-in stages declare both kinds of demand.
Expanded occurrences contribute to compiled group and view requirements, and the
existing render world module aggregates active view demand before world updates.

Demand preparation runs late in PreAsync, after ordinary view configuration.
Extraction runs late in PostTransform, after authoritative transforms and normal
world-module derived updates; graph execution explicitly depends on extraction
in that phase. Views created after preparation wait until the next preparation
boundary. Categories are checked
before component dispatch and cache reuse. Demand changes invalidate the affected
view's extraction cache. Empty-demand views do not traverse scene objects. Static
component packets reuse the existing cache; component property changes, activation,
deactivation and static transform notifications invalidate relevant consumers.
Dynamic components are extracted on demand from their current world state.

World-owned GPU instance buffers consume component-authored packets as derived GPU
records; extracted instances do not create a second authoritative hierarchy.
Stable identities, compact arrays, skinning/LOD history and coalesced dirty upload
ranges remain in place. Identical setters leave revisions and uploads unchanged.
A moved instance uploads one settling frame so GPU previous transforms cannot
report motion forever after movement stops.

| Owner | State |
| --- | --- |
| World / existing world modules | Authoritative objects/components and incremental scene-derived CPU caches. |
| View | Extraction/filter results, derived GPU scene, render graph context and frame lifetime. |
| Expanded graph occurrence in a view | Pass objects, scoped parameters/blackboards, visibility and temporal resources. |
| Device services | Shared geometry/material residency, explicit world service uploads and deferred GAL allocation/destruction. |

## Verification

Verified on 2026-10-15 using the existing `Workspace/vs2026x64` Debug configuration:
Vulkan enabled, D3D12 disabled. Backend-aware GPU tests used their default renderer
enumeration. D3D12 was not built or executed.

Build command (serial MSBuild avoids contention in the local PDB service):

```powershell
cmake --build Workspace/vs2026x64 --config Debug --target CoreTest GraphicsCoreTest GraphicsFoundationTest EditorEngineProcessFrameworkTest EnginePluginAssets EditorPluginAssets GpuDrivenScene -- /m:1 /p:CL_MPcount=1
```

The targets above built successfully across the incremental validation runs.
Final rendering integration changes were also rebuilt through their editor,
GPU-test and sample consumers.

| Executable / arguments | Result |
| --- | --- |
| `CoreTest.exe -nosave -nogui -all` | All 24 subtests passed. |
| `GraphicsCoreTest.exe -nosave -nogui -all` | All 25 subtests passed. |
| `GraphicsFoundationTest.exe -nosave -nogui -all` | All 44 subtests passed on Vulkan. |
| `EditorEngineProcessFrameworkTest.exe -nosave -nogui -all -filter EditorViews` | All 49 subtests passed on Vulkan. |
| `GpuDrivenScene.exe -frames 12` | Completed 12 frames on Vulkan and exited successfully. |

Representative coverage:

* Core graph views and composition cover generation validation, current induced
  adjacency, selected edges, graph moves, mutation, copy/compaction remapping,
  nested/repeated definitions, boundary fan-out, path overrides and deterministic
  serialization. Existing graph mutation, validator and subscription tests pass.
* Core composition cache tests distinguish presentation changes from relevant
  revisions, ignore unreachable definitions and retain previous immutable
  snapshots across replacement or failed expansion. Definition recursion is
  rejected while ordinary topology cycles remain legal.
* Reflected pipeline tests cover nested repeated assets, independent parameters,
  source diagnostics, boundary typing, deterministic bytes, child hot reload,
  selective dependencies and safe held compilation snapshots. Exact topology
  cache tests change current callback data, resource size, side effects and
  culling policy. The 200-pass test verifies isolated allocations and batching.
* Native editor asset export tests preserve document/node UUIDs and independent
  instance overrides, infer external pin types, deduplicate shared asset
  references, and preserve the previous valid output on an invalid export.
  Existing editor tests exercise main-scene and selection query resources in one
  graph, with separate writable scene uploads and both mesh-shader paths.
* Vulkan pipeline tests execute repeated subgraphs and read back GPU results.
  Explicit temporal feedback is tested across two occurrences in two views,
  including disabled views and camera cuts. Current-frame cycles still fail.
* World demand tests aggregate independent views, skip unused components and
  empty-demand traversal, reuse static packets, invalidate property/transform
  changes, and consume a world module's current PostTransform update. CPU-only
  geometry/light consumers leave GPU-scene features uninitialized. Scene database
  tests cover unchanged revisions, dirty uploads and motion-history settling.

An integration regression exposed identical writable upload names in independent
visibility contexts. Upload names now follow the context's resource-set name;
the complete editor group and graphics suite passed after that correction.
Validation logs are kept locally in ignored `Workspace/hierarchy-*.log` files.
