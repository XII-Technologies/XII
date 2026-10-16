# Editor-authored rendering migration

## Audit baseline

Branch `TE/RenderPipelinePasses` starts at `0172d45c7`. This document tracks the
complete resource/editor/pass migration; the earlier scene migration and animated
ray geometry work do not establish completion of this objective.

| Existing facility | Findings and migration target |
| --- | --- |
| `Pipeline/RenderPipelineResource`, descriptor/compiler/instance | Standard ResourceManager loading, reflected binary descriptor serialization, immutable compiled snapshots, view-local instances and definition invalidation already exist. Preserve and extend them. |
| `Pipeline/Implementation/View.cpp` | Constructor selects `GetDefaultPipeline()` implicitly. Remove this selection; require a configured resource and diagnose an absent selection. |
| `Pipeline/Implementation/BuiltinRenderPipeline.cpp` | One reflected stage selector exposes 18 composite stages. `GetDefaultDescriptor()` constructs the entire stack in C++; `GetDefaultPipeline()` registers it as an in-memory resource. Replace the factories with authored assets and the selector with focused reflected operations. |
| `SharedPluginScene/Scene/Implementation/SceneRenderPipeline.cpp` | Editor scene factory mutates the default descriptor to insert selection, picking and icons. Replace with an ordinary configured preview asset. |
| Asset preview views / `xiiTexturePreviewPass::GetPipeline` | Texture/cube previews create cached graphs in C++. Supply explicit preview resources through the same configuration/resource path. Audit other preview views as consumers. |
| `RenderPipelineAsset` document/manager/window | Normal creation, save, transform, object mirroring, node property grid, reflected node discovery and subgraph expansion exist. Node templates special-case built-in stage strings. Remove that special list after decomposing passes. |
| ToolsFoundation `xiiDocumentNodeManager` / GuiFoundation node editor | Pins are reflected runtime descriptions but are not document selections. Qt clicks always start a wire; selection propagation handles nodes/connections only. Add generic document-backed pin inspection, preserving drag wiring and document undo/serialization. |
| Render pipeline pin schema | Texture/buffer/order, direction, optionality, format and temporal output exist. Schemas are unreflected POD-style descriptions; mutable per-pin settings, persistent pin identity and richer contracts are missing. Keep immutable type metadata separate from authored configuration. |
| Core `xiiPlatformProfile` / Editor asset profiles | Existing reflected project/platform settings and runtime chunk persistence are the integration point for pipeline references and policy. Do not add a renderer settings registry. |
| World extraction / GAL / Core graph composition | Preserve authoritative world state, demand aggregation, derived GPU buffers, explicit graph hazards, shared definitions and provenance. No new scene database or graph framework. |
| `Source/Samples/GpuDrivenScene/GpuDrivenScene.cpp` | Specialized low-level GAL/render-graph sample builds its demonstration graph directly. Its drawing/shadow/presentation operations also need review during focused-pass migration; passing this sample does not establish resource-only authored rendering. |

## Operation inventory

`BuiltinRenderPipelinePassData.h` currently declares 84 operation-data structures.
Their setup/execute routines already expose useful GPU resource boundaries, but
`BuiltinRenderPipelineRuntime.cpp` chooses their composition in C++.

| Area | Existing operations to preserve and expose where coherent |
| --- | --- |
| Frame/visibility | Residency uploads, world instance upload, acceleration structures, frustum/LOD selection, draw building, light preparation/classification/clustering, reflection-probe selection, froxel allocation. Separate world/device preparation from camera algorithms. |
| Depth/geometry | Depth raster, Hi-Z generation/history, occlusion, motion vectors/dilation, G-buffer, normal/roughness surfaces. |
| Shadows | Cascade setup, local atlas allocation, directional/spot/point depth, ray-traced/contact shadows, filtering/denoising and shadow sampling constants. |
| Lighting | BRDF LUT, atmosphere transmittance/multiscatter, sky irradiance, ambient occlusion/denoise, DDGI/voxel gathering, ReSTIR DI/GI temporal/spatial/visibility, deferred direct/indirect light, ray GI/reflections and denoisers, SSR/composition, volumetric fog. |
| Surfaces/transparency | Opaque composition, hair/water/subsurface/eye rendering, particles, decals, weighted OIT and composition. Keep internal implementation steps together only when they lack an independent contract. |
| Screen/output | SSGI/composition, refraction snapshot/resolve, planar reflections, histogram/exposure, TAA, upscale, bloom, tone mapping, debug visualization, color grading, sensor output and presentation. |

Existing specialized classes in scene/asset plugins remain distinct implementations.
Do not manufacture a numerical pass count or claim unsupported forward/volume
techniques. Compatibility analysis and GPU queue grouping belong in the existing
compiler, rather than a GPU submission per authored node.

## Completion gates

All gates require current source and executable evidence before closing:

1. Resource-only selection: authored source/compiled assets, explicit project/view
   references, migrated game/editor/preview entry points, missing-resource errors,
   and physical deletion of implicit factories/fallbacks.
2. Generic Qt pin inspection: independent selection, existing reflected widgets,
   commands, undo/redo, stable document identity, save/load/copy/paste, dynamic-pin
   selection cleanup, connected pin edits and render asset compilation.
3. Configuration: reflected project, pipeline, pass and pin settings; typed,
   explicitly permitted view/instance overrides; deterministic precedence,
   validation and effective configuration inspection.
4. Pass migration: focused concrete implementations, editable resource-authored
   compositions/subgraphs, deletion of stage orchestration and execution-time
   global resource discovery. Preserve algorithms and scoped mutable state.
5. Compiler/runtime: contracts, capabilities, resource propagation, provenance,
   selective invalidation, reload/lifetime isolation, safe aliases/hazards and
   efficient GPU scheduling for large graphs.
6. Verification/report: affected builds, actual Qt workflows, asset/version round
   trips, available Vulkan rendering paths, multi-view/world/reload tests, explicit
   backend/performance limits and final searches for obsolete production paths.

D3D12 remains excluded by the user's instruction. The complete objective remains
open until every gate is supported by evidence.

## Verified checkpoints

The generic node editor now supports persistent pin property objects through
`xiiDocumentNodeManager::GetPinPropertyObject`. Pin clicks inspect their backing
document objects in the existing property grid; dragging still wires nodes.
Native/visual pin rebuild order preserves live selections and clears selections
for removed pins. Document connections are selectable and support undoable deletion.
The Qt regression checks actual reflected widgets, read-only/range metadata,
editing, undo/redo, dynamic pins, wiring, copy/paste and saved identities.
Full EditorFrameworkTest and ToolsFoundationTest pass 13 and 28 subtests at this
checkpoint, and EditorPluginVisualScript builds against the shared API.

Four ordinary source/compiled assets now exist in `Data/Base/RenderPipelines`:
Deferred, EditorScene, TexturePreview and TextureCubePreview. The production
pipeline document supports offline authoring through the existing connection
policy. `EditorDocuments/PipelineAssetLibrary` opens the sources, reconstructs
their Qt graph, invokes the production compiler and verifies ResourceManager
loading and deterministic packaged bytes. All 13 EditorDocuments subtests pass
after this addition. The temporary one-time exporter and its build reference are
removed.

At that checkpoint, the resource-selection factories, composite stage implementation
and render-pin configuration integration remained migration targets; these checkpoints do not
close the complete objective. Local evidence is in ignored
`Workspace/pipeline-pin-*.log` and `Workspace/pipeline-assets-*.log` files.

`xiiRenderPipelineProfileConfig` now extends the existing reflected platform
profile with game, scene-editor, asset-preview and texture-preview resource
references. Explicit view references precede project policy; empty selections and
invalid runtime policy chunks produce no selected resource. Runtime chunks and
the existing reflected project serializer preserve the policy. Game-state main
view creation now selects the configured resource. All 25 GraphicsCore subtests
and 49 Vulkan EditorViews subtests pass, including both wireframe raster paths
using the packaged deferred asset. Evidence: `Workspace/pipeline-profile-*.log`.
Other entry points and constructor fallback deletion remain in progress.

## Resource-only view selection checkpoint

The view constructor no longer selects a pipeline. Game-state main views resolve
the Game policy; scene views resolve EditorScene; remote views resolve Game; mesh,
animated mesh, animation, skeleton, material and decal views resolve AssetPreview;
texture and cube views resolve their respective preview policies. The editor
context takes its profile from the existing application and permits an explicit
resource reference or externally owned profile. Explicit references precede
profile policy, and camera updates refresh the configured selection.

The C++ default descriptor/resource factories, texture preview factory and both
`SharedPluginScene/Scene/SceneRenderPipeline` source/header files are deleted.
Tests load the packaged authored resources; mutation fixtures explicitly remap
loaded descriptor indices into execution order rather than reconstructing a
pipeline in C++. Missing selections leave no executable pipeline or extraction
demand. A view diagnoses failed selection/loading once until its resource changes
or synchronization succeeds. Clearing a selected resource clears execution state;
selecting a valid resource restores rendering without a fallback.

Rejected runtime profile chunks also remain rejected when saved again: runtime
serialization writes the effective empty references rather than retained authored
defaults. No scene ownership, component storage, graph-handle semantics or GPU
algorithms were changed by this checkpoint.

Debug validation uses `Workspace/vs2026x64` with `/m:1 /p:CL_MPcount=1`:

| Check | Result |
| --- | --- |
| GraphicsCoreTest | All 25 subtests pass. |
| GraphicsFoundationTest, `-renderer Vulkan` | All 47 subtests pass. |
| EditorEngineProcessFrameworkTest, `-renderer Vulkan` | All 73 subtests pass. |
| EditorFrameworkTest | All 14 subtests pass, including production source-document/compiler round trips and actual engine-process startup. |
| Affected builds | GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest, EditorFrameworkTest and EditorPluginVisualScript build; transitive game/editor-engine/processor and asset/scene plugin targets also build. |
| Source search | No matches for removed default/preview factories, scene pipeline helper, scene database or scene database manager in C++ headers/sources and CMake files. |

The new frame regression covers empty selection, suppressed repeated diagnostics,
explicit-over-project precedence, project policy changes, clearing selection and
recovery. Existing GPU suites continue to cover multi-view ownership, scene
picking, shadows, skinning, resource reload and temporal history. A full run exposed
an unsafe test fixture that temporarily removed the global device during allocation
failure injection. It now uses the existing temporal texture factory callback,
preserving failure/history-recovery coverage without racing resource-loading
threads. An incorrect duplicate expected-log registration in the new fixture was
also corrected. A rebuild attempted while Qt tests held loaded plugin files failed
to copy a Qt plugin; sequencing build and Qt execution resolved that file lock.

Final evidence: `Workspace/pipeline-resource-only-build.log`,
`pipeline-resource-only-editor-rebuild.log`, `pipeline-resource-only-core-tests.log`,
`pipeline-resource-only-gal-tests.log`, `pipeline-resource-only-editor-gpu-tests.log`
and `pipeline-resource-only-qt-tests.log`. These ignored local logs are not packaged
runtime data. Vulkan still reports existing unused shader-output warnings. D3D12
is excluded; other platforms/configurations are not newly verified here.

No new performance benchmark was run for resource selection. Immutable compiled
graphs and per-view execution instances remain shared/isolated through the existing
resource implementation; this checkpoint does not change GPU operation or submission
counts. It does not establish the later compiler optimization gates.

The complete objective remains open. Render pins still need reflected persistent
configuration objects integrated with the generic pin inspector and compilation;
pipeline settings and explicitly permitted typed instance overrides remain open.
`BuiltinRenderPipelineRuntime.cpp` still selects composite stage operations in C++,
and authored scene assets still use those composite nodes. Focused pass/state
migration, reusable operation subgraphs, the specialized low-level sample,
capability/contract validation and final optimization/workflow verification remain
required. This checkpoint closes the removed view factories, not those remaining
architecture gates.

## Structured reflected configuration checkpoint

The production asset compiler now calls `xiiRenderPipelinePass::GetProperties`
instead of capturing only writable member properties. The existing descriptor
property dictionary supports reflected arrays, value maps, embedded structures
and owned settings objects, including registered derived settings types and null
optional owners. `Configure` restores those shapes through the same reflection
metadata. Borrowed object pointers, unknown/read-only properties, malformed
containers and invalid enum names fail validation. Traversal and allocation are
bounded; invalid owned-array replacements release partial allocations while
retaining the previous owners. No additional settings registry, resource system,
scene ownership or per-frame reflection path was introduced.

`Pipeline/ReflectedPassProperties` verifies native nested settings, arrays/maps,
embedded structures, polymorphic/null ownership, malformed replacements, excessive
nesting, empty-container replacement, serialized resource compilation and isolated
settings in two runtime instances. The production native asset compiler regression
also verifies arrays/maps survive export and repeated shared-definition instances.
All four packaged source assets still reproduce their existing compiled bytes.

Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest, EditorFrameworkTest and EditorPluginVisualScript
and their transitive consumers. All 26 GraphicsCore, 47 Vulkan GraphicsFoundation,
73 Vulkan editor-engine and 14 Qt editor subtests pass. Local evidence is in
`Workspace/pipeline-properties-*.log`. Initial fixture compilation issues involved
engine arrays without initializer-list construction and const dictionary lookup;
both are corrected in these final results. Existing Vulkan unused shader-output
warnings remain, and D3D12 is excluded.

This removes the serialization/configuration gap needed by authored pin settings;
it does not yet close render-pin configuration or the other outstanding architecture
gates above. No new performance benchmark was run for this authoring/loading change.

## Connected-pin editing checkpoint

`xiiNodeCommandAccessor` now disconnects native edges while retaining their document
objects during pin edits. Compatible connections reconnect using their original
object, UUID, reflected type and custom authored properties. Connections whose pins
disappear or become incompatible are removed through the same undo transaction;
undo restores the original objects before reconnecting. Adjacency lists are
snapshotted before disconnecting, so fan-out edits do not iterate compacted storage.
The generic Qt scene preserves document selection while connection visuals are
temporarily removed and restores it when the connection is rebuilt.

The Qt regression uses a reflected connection with custom weight/label properties,
two outgoing edges, connected pin-setting edits, undo/redo, dynamic input removal,
deletion and restoration. Original connection IDs and authored values survive.
All 28 ToolsFoundation and 14 EditorFramework subtests pass; the affected libraries,
test targets and EditorPluginVisualScript build. VisualScript continues to receive
its existing disconnect/connect node events for type inference. Evidence is in
`Workspace/pipeline-connection-identity-*.log` and `pipeline-editor-race-build.log`.

The first full Qt run exposed a screenshot synchronization gap: both the grid-on
and grid-off captures contained grid lines. Captures now queue the document redraw
(including scene settings) before view and screenshot messages, replacing timed
grid delays with IPC ordering. The original image assertions remain unchanged;
the final grid-off capture has no grid and the complete viewport test passes.
The same run encountered the curator's file/document association assertion.
Asset metadata now rejects invalid persistent IDs and retires derived dependencies
and transform state when publication cannot associate the watched file. Normal
asset replacement, removal, dependency and processor workflows pass in the full
Qt suite; the particular read/publication race was not forced deterministically.

No new performance benchmark was run. These changes prepare correct generic
connected-pin authoring, but render-specific persistent pin objects and constraints
still need to be connected to that infrastructure. All other open gates remain in
scope, including typed overrides, focused operation passes, authored subgraphs and
the final compiler/runtime audit. D3D12 remains excluded.
