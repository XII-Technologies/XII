# World-authoritative rendering migration

## Migration map

| Legacy responsibility | Final owner / replacement |
| --- | --- |
| Render objects, parents, local/global transforms, activation and bounds | Existing `xiiWorld`, `xiiGameObject`, render components and world spatial system. Delete the renderer hierarchy and its mutation API. |
| Mesh/material assignments, sections, forced LOD and animation pose | Existing mesh, LOD and skinned components and resource handles. Extraction packets carry immutable frame inputs. |
| Compact instance records, GPU identity, previous transforms and upload intervals | Focused GPU instance buffer owned by the existing render world module. Only extraction updates it; no scene manipulation API or independent hierarchy. |
| Geometry and material GPU addresses | Existing device residency services, acquired from resource handles. No per-scene asset index tables. |
| Camera culling, LOD hysteresis, exposure and image history | Views / graph occurrences; visibility consumes world instance buffers and a per-view candidate stream. |
| Picking subsets | Candidate indices into the same world extraction; validate authoritative object/component handles on readback. Remove picking-owned scene copies. |
| BLAS allocations | Shared device geometry resource cache for static meshes; frame-sliced world-derived deformation and dynamic BLAS for animated components. |
| TLAS transforms and membership | Derived world extraction and rotating resources owned by each pipeline occurrence/view. The explicit low-level world path has its own default owner. Remove the global ray-tracing instance registry. |
| CPU render-side spatial hierarchy | Delete; use the world spatial system and existing GPU culling. |

## Deleted infrastructure

The complete `Source/Engine/GraphicsCore/Scene` implementation is removed:
`SceneDatabase`, `SceneDatabaseManager`, `SceneDatabaseManagerState`,
`SceneExtractedMeshes`, `SceneMeshSource`, `SceneSkinning`, `SceneSpatialHierarchy`
and `SceneTypes`, including their public headers, implementation files, reflected
types, context/object IDs, revision/commit machinery and static-link registrations.
`SceneSystemsTest.cpp` is deleted; its renderer-authoring tests are replaced by
tests using real world objects and mesh components. The small, pure skinning
validation helper now lives under `Meshes/Implementation/ExtractedMeshSkinning`.

The old global `RayTracingScene` service, its instance handle table and its
create/update/destroy instance API are also removed. Retained BLAS allocation and
build algorithms live in `RayTracingResources`; this service owns device geometry
leases only. TLAS membership/transforms come from extraction, and mutable TLAS
frames belong to the requesting world or pipeline runtime. Visibility contexts
no longer own copies of instance/pose buffers. Picking no longer owns a separate
scene or synchronizes another set of rendering objects.

CMake discovers these sources rather than listing them manually. Regeneration
removes the deleted files from generated projects; no permanent exclusions or
compatibility implementations are retained.

## World and frame integration

Existing component lifecycle, bounds and transform notifications invalidate the
existing render world cache. Mesh and light components explicitly opt into
camera-independent packet sharing. A component is dispatched once for all
requesting cameras in a frame; static packets persist until component/transform
or mesh-resource invalidation. Camera-dependent/custom components retain their
per-view extraction behavior. Tags and activation are checked before cache reuse.

Resource-manager callbacks only set an atomic invalidation flag. At the next CPU
extraction boundary the module drops borrowed packets and applies invalidation.
Invalidated owned packets are retired until all current views release their
borrowed pointers. No callback accesses movable component storage asynchronously.

Compiled pass requirements are aggregated in `PreAsync`. Existing world modules
can inspect the aggregate before preparing their derived data. Extraction runs
after authoritative transforms in `PostTransform`; graph recording/execution
follows extraction. GPU callbacks capture upload bytes and resource references,
never component pointers. The existing GPU frame tracker controls rotating-buffer
reuse and retirement across all queues. World teardown and high-level shutdown
release world GPU caches before residency/device shutdown; the shutdown hook uses
the existing world registry rather than adding another registry.

## Derived GPU representation

`xiiGpuInstanceData` is a focused extraction packer owned by the existing
`xiiRenderWorldModule`. It has no object creation, parenting, transform propagation,
geometry/material setter or serialization API. Its dense records are GPU draw
candidates, including authored mesh sections/LODs and component-local instances.
World/component handles remain authoritative; resource handles acquire existing
device geometry/material residency. There are no per-world asset index tables.

The shader record remains 288 bytes. Current/previous transforms, conservative
skinned bounds, material classification, section primitive ranges, LOD controls,
visibility masks and skinning palette offsets retain their shader ABI. Sections
share a LOD representative and pose offsets. Motion settles once after movement
stops. Instance-local matrices compose with the object's world transform; changing
instance membership invalidates previous index identities before another readback
can resolve them to shifted instances.

GPU draw tokens are non-reused 32-bit runtime identities, allocated with the
existing atomic facility and failing closed on exhaustion. Resolution validates
world index, object/component generation, owner relationship, activation and
instance topology revision. Dense compaction changes buffer indices without
reassigning surviving tokens. Tokens and graph handles are never serialized IDs.

Rotating physical instance/pose buffers belong to the world. Per-slot versions
coalesce changed records into upload ranges; an unchanged warmed slot uploads
zero bytes. Versions advance only when the graph records the upload. A graph setup
serial prevents reusing handles from an earlier setup or a recycled graph address.
Later cameras import the same physical buffers and upload no duplicate bytes.
Visibility, LOD hysteresis, Hi-Z and picking use per-view candidate indices into
these buffers. Their writable histories remain separate.

## Consumer and pipeline migration

Native visibility, local/directional shadows, builtin reflected pipeline runtimes,
view mesh resolution, editor picking/readback and the GPU-driven sample now consume
the world extraction. Uploads are passed through typed graph handles. The reflected
pass compiler, hierarchical graph expansion, instance overrides, provenance,
temporal occurrence isolation and graph scheduling remain in use.

`RayTracingResources::AddBuildPass` derives TLAS inputs from extracted transforms,
visibility, material and primitive ranges. Device BLAS leases are shared by mesh
resource revision/range; two frame-resource owners never share writable TLAS,
instance or hit-data allocations. Hit shaders account for section primitive
offsets. Non-indexed BLAS builds also offset their source vertex address correctly.

Animated ray geometry now consumes the extracted native mesh revision, skinning
palette and instance morph weights. A graph compute pass copies the native vertex
layout, applies additive morph deltas before skinning, and transforms normals by
the inverse transpose. Tangents retain their reflected handedness. Active joint
indices are range-checked; zero-weight influences do not access the palette.
Mesh resources need no CPU-readable vertex copy for this path.

The existing morph descriptor identifies dense delta vertices in the immutable
native buffer: `m_uiVertexOffset` starts the delta range for base vertices
`[0, m_uiVertexCount)`. Position, normal and tangent xyz carry deltas; other
streams retain their base values. Weighted delta bounds include zero and support
negative weights. This uses the existing serialized mesh format. Asset importers
that reject morph animation still require an authored native morph resource;
this change does not add importer support.

Deformation outputs and dynamic BLAS belong to the world, keyed by validated GPU
draw identity and frame slot. Repeated sections and views reuse the same native
vertex output; each primitive range has its own BLAS. Exact pose/morph/revision
comparison skips deformation and BLAS updates for unchanged warmed slots. BLAS
uses update-capable allocations and maximum build/update scratch capacity. TLAS
invalidation includes deformation revision even when the world transform is
unchanged. Each pipeline occurrence retains separate TLAS and hit-data frames.

Typed graph dependencies order deformation, BLAS construction and ray dispatch.
Readiness advances when commands record, so an abandoned graph setup cannot
silently mark an allocation initialized. Source leases, frame rotation, bindless
retirement and the existing device fence clock protect in-flight resources.
Hot reload replaces the captured native revision; no scene object lifecycle or
CPU transform hierarchy is introduced.

The sample authors mesh, material, hierarchy, animation transforms and sunlight
through a real world and components. It now uses the ordinary world/view update
and canonical Scene resource. Manual component extraction and the sample-owned
render graph are removed; production pass order comes from the selected resource.
See [SceneRenderer.md](SceneRenderer.md) for the current default contract.

Mesh component serialization advances its base type to version 2 for the authored
visibility mask. Version 1 loading retains the previous all-visible default.
World serialization tests round-trip mesh resources, instance transforms and
visibility. Removed database/context descriptors were runtime-only and were not
pipeline asset properties; pipeline asset formats and Core persistent document
identities are unchanged.

## Ownership audit decisions

| Facility | Decision and reason |
| --- | --- |
| Renderer object hierarchy and spatial tree | Delete; world transforms, hierarchy, bounds and spatial queries already own this state. |
| Global RT instance registry | Delete; extracted candidates determine membership and transforms. |
| Per-view instance/pose mirrors and picking-owned scenes | Consolidate into shared world GPU buffers plus view candidate streams. |
| Geometry/material residency and bindless tables | Retain device services; they own physical asset addresses and deferred retirement, not authored scene state. |
| GPU visibility, shadows, lighting and RT algorithms | Retain distinct algorithms; their writable resources follow view/occurrence/device lifetime. |
| Graph/resource caches and transient allocation | Retain existing compiler/GAL facilities; no parallel render scheduler or resource ownership framework. |

Geometry and material GPU allocation is now demand-triggered through the existing
services. Startup/configuration alone does not allocate their tables. World
pipelines initialize requested residency before entering the shared GPU frame;
direct low-level consumers can explicitly ensure initialization or acquire their
first geometry/material. Virtual-shadow physical buffers/atlas are also created
only by a shadow consumer. CPU-only pipeline tests verify that GPU instance,
geometry and material storage remain absent.

## Performance and verification

Debug packing measurements are diagnostic, not a release
performance comparison: the packer still traverses demanded draw candidates to
validate owners, residency, LODs and poses. Static packet reuse avoids redispatch
and cloning; GPU transfer cost follows changed ranges instead of world size or
camera count. No before/after production CPU/GPU benchmark baseline is available.

Observed on the configured Vulkan device in Windows Debug:

* Two cameras upload one changed instance record once: **288 bytes**, one range,
  one upload pass. Warmed unchanged frame slots upload **0 bytes**.
* 1,024 unchanged component candidates take approximately **2.3 ms per packing
  update** in focused runs (20 updates, approximately 46 ms total); the final full
  suite recorded **6.3 ms/update** (126 ms total). Debug timings vary substantially
  with the test workload and machine state; this is not a controlled baseline.
* The sample completes **120 frames**, extracts **768 LOD draw candidates** from
  **384 world mesh components**, and builds a **384-instance hardware TLAS**.
  A separate raster-only run with ray tracing disabled completes **30 frames**.
* A separate integration test constructs and executes two independently allocated
  TLAS builds from one world extraction; writable TLAS/hit-data allocations differ.

Serial Debug builds cover CoreTest, GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest, GpuDrivenScene, EnginePluginScene,
EnginePluginAssets and EditorPluginAssets. Test commands use `-noGui -all -close`;
GPU/editor tests additionally specify `-renderer Vulkan`.

| Suite | Successful subtests |
| --- | ---: |
| CoreTest | 24 |
| GraphicsCoreTest | 24 |
| GraphicsFoundationTest, Vulkan | 47 |
| EditorEngineProcessFrameworkTest, Vulkan | 73 |

The suites include Core selected/induced views, compaction and composition;
reflected nested/repeated assets, deterministic serialization/compilation,
boundary remapping, definition recursion versus intra-frame/temporal cycle
policies, hot reload and dependent invalidation; per-occurrence/view temporal
history; real GPU picking, raster skinning, LOD ranges/reload; component
activation/removal, foreign-world/stale IDs, instanced transform history;
camera-shared extraction/uploads; and specialized pipeline demand.

The live-world shutdown/restart test exposed an existing ResourceManager bug:
high-level shutdown disabled loading but startup did not reopen its loading gate.
`ResourceManager::OnEngineStartup` now restores those flags before renderer
startup acquires shaders. The regression verifies world-derived buffers release
at shutdown, new IDs do not alias previous IDs, and extraction/GPU upload resume
while the authoritative world remains alive.

Failures found and corrected during implementation: a non-reloadable test fixture
used `CreateResource` instead of the existing load/custom-loader path; a world
serialization fixture held an object pointer across compaction; and sample light
lookup selected a mutable world API under a read marker. Final results supersede
those failed runs. Editor Vulkan validation still reports unused shader-output
warnings; these are warnings, with no failed tests or device errors.

Local evidence is in ignored `Workspace/scene-migration-*.log` files, including
`completion-build`, `final-core-tests`, `final-graphics-tests`, `final-gpu-tests`,
`final-editor-tests` and `final-sample`.

## Animated ray geometry and optimized verification

`PipelineExtraction/RayTracingDeformation` performs actual hardware ray dispatch
and GPU readback. It checks two components sharing one mesh with independent
poses and morph weights, two independently allocated TLAS owners sharing world
deformation, warmed-slot reuse, animated silhouettes without object movement,
negative weights, reflected/nonuniform normal transforms, tangent handedness,
bind-pose restoration and mesh hot reload. Delayed readback spans two frame-ring
rotations without per-frame device-idle waits. Object deletion and replacement
validate stale identity rejection while earlier GPU results remain readable.
Malformed morph inputs fail without replacing the last valid extraction.

The opt-in `RenderingBenchmarks/WorldExtraction` fixture measures optimized
packing and real GPU incremental uploads across two cameras against the exact
pre-change renderer. Reproduction, measurements and scope are recorded in
[RenderExtractionPerformance.md](RenderExtractionPerformance.md).

The follow-up builds GraphicsFoundationTest in optimized Shipping and all affected
Debug renderer/editor/plugin/sample targets. Full Vulkan GPU suites pass all 47
subtests in both configurations. After retaining unchanged resource leases, the
three extraction subtests pass again in Debug and Shipping, including hot reload
and hardware ray readback. Seven optimized baseline/updated benchmark pairs pass
their exact upload checks. Local evidence uses `Workspace/deformation-*.log` and
`Workspace/extraction-*-shipping-*.log`.

Early follow-up runs exposed invalid shader syntax, an unaligned constant-buffer
update and a benchmark moving objects marked static. The shader syntax was
corrected, uniform updates use the existing discard-map path, and the benchmark
authors dynamic world objects. These failures are resolved in the passing runs
above. The benchmark's initial CPU regression also prompted removal of redundant
resource-handle assignments; final results include that change.

Only Vulkan is configured (`XII_BUILD_VULKAN=ON`, `XII_BUILD_D3D12=OFF`). D3D12 is
excluded from this follow-up at the user's request; other platform combinations
are not covered by these local results. Repository searches continue to find no
production references to the removed database, manager, scene handles, spatial
hierarchy or global RT instance registry.
