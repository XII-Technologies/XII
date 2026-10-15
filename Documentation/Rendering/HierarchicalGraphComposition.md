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
  already caches compilation, handles GPU hazards, queues, resource lifetimes,
  culling and submission batching. Hierarchical authoring must feed these paths.
* The render pipeline editor uses `DocumentNodeManager`, reflected native pass
  schemas, document UUIDs, object mirroring and ResourceManager assets.
* `RenderWorldModule` already extracts from `xiiWorld` and caches static component
  packets. Components invalidate that cache. `SceneDatabaseManager` converts those
  packets into stable GPU identities and compact scene arrays. These are derived
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

## Validation record

Implementation and validation results are recorded as phases are completed.
