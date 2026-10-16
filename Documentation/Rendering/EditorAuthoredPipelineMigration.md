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

## Persistent render-pin contracts checkpoint

Render pipeline pins now have owned reflected document configuration objects with
persistent UUIDs, display names, required resource formats and texture dimension,
sample, extent and binding requirements. Native pass schemas remain authoritative
for resource kind, direction, access, cardinality, requiredness and temporal history;
their reflected metadata is read-only in the inspector. Requirements narrow a
contract rather than silently changing allocations. Invalid or conflicting
requirements fail compilation or runtime binding validation.

The production node manager initializes configurations during normal node creation,
dynamic schema changes and loading of older documents. Matching pins retain their
identities and settings; removed pins and incompatible edges use document commands
and undo. Clipboard duplication remaps owned objects and their persistent IDs.
The generic node editor routes creation through the document accessor, supports
authored display labels, and tracks changes in nested settings. The existing
document serializer now handles complete detached owned subtrees during paste.
There is no additional object registry or per-frame reflection path.

Subgraph instance interface configurations are serialized alongside definition
references. The existing Core expansion boundary mappings lower them onto each
occurrence's internal contracts. Nested and repeated instances remain independent,
and fan-out accumulates requirements from every consumer, including consumers after
an unconstrained first mapping. Unknown pins and conflicting contracts report the
responsible instance path. The compiled schema retains source identities and
deterministic occurrence identities; runtime pass objects omit authoring pin objects.
Expanding a subgraph in the editor applies a structured object-graph diff through
the existing accessor, preserving leaf arrays/maps/settings and interface contracts.

All four SDK pipeline source assets and compiled binaries have been regenerated
with the production serializer/compiler. Document, node, connection and layout
identities are retained. Loading older sources without configurations initializes
stable settings objects; normal save persists the conversion. Deterministic export
and packaged-byte equality remain tested. Authoring metadata increases packaged
binary sizes: Deferred 25,454 to 176,611 bytes, EditorScene 29,342 to 190,058 bytes,
and each texture preview 556 to 971 bytes. This footprint is not a GPU allocation
or submission increase. No controlled performance benchmark was run.

Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest, EditorFrameworkTest, ToolsFoundationTest and
EditorPluginVisualScript with their affected consumers. Current regressions cover
native/reflected contracts, direct-binding validation, nested shared definitions,
fan-out conflicts, deterministic serialization, actual Qt pin selection/property
widgets, compatible/incompatible connection edits, dynamic pins, copy/paste,
save/load, older documents, subgraph expansion/undo and production asset hot reload.
The hot-reload test retains an old immutable snapshot while independently
synchronizing two runtime instances to the replacement.

All 27 GraphicsCore, 28 ToolsFoundation, 47 Vulkan GraphicsFoundation, 73 Vulkan
editor-engine and 15 Qt editor subtests pass. Evidence is in ignored local
`Workspace/pipeline-pin-final-build.log`, `pipeline-pin-final-editor-build.log`,
`pipeline-pin-final-core-tests.log`, `pipeline-pin-final-tools-tests.log`,
`pipeline-pin-final-qt-recheck.log`, `pipeline-pin-verified-gal-tests.log` and
`pipeline-pin-verified-editor-gpu-tests.log`.

One full Qt run failed the native viewport's expected sphere-picking assertion;
the repeat passed picking, captures, resize and play-mode teardown. Its cause has
not been established, so this remains an intermittent verification issue rather
than a claimed fix. Earlier fixture setup and detached-subtree failures were
corrected before these final runs. Existing Vulkan unused shader-output warnings
remain. D3D12 is excluded; other platforms/configurations are not newly verified.

This closes the persistent render-pin integration gap described in earlier
checkpoints, not the complete objective. Pipeline-level settings, explicitly
permitted typed overrides and effective configuration inspection remain open.
Focused operation passes, authored operation subgraphs and specialized sample
migration must still replace `BuiltinRenderPipelineRuntime.cpp` stage orchestration.
Capability validation, the final compiler/runtime audit and performance evidence
also remain required.

## Pipeline contract checkpoint and pass migration priority

`xiiRenderPipelineSettings` is now a persistent reflected asset object. The
production pipeline window selects it when graph selection clears. Device feature
requirements and final output format/sample contracts survive save/load, export
and hot reload. Referenced resources contribute their device requirements to the
containing compilation. Checks run before algorithm construction without runtime
reflection. Older source documents initialize settings through normal commands;
older binary descriptors retain unconstrained defaults for the additive field.

The SDK assets now declare single-sample outputs, with compute support required
by Deferred and EditorScene. Their sources reproduce the packaged binaries through
the production compiler. No one-time exporter remains in the test source.
Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest, EditorFrameworkTest and EditorPluginVisualScript.
All 28 GraphicsCore, 47 Vulkan GAL, 73 Vulkan editor-engine and 15 Qt subtests pass.
The Qt run includes the actual pipeline window, selection clearing, settings undo,
save/reopen and independent old/new settings snapshots across reload. Evidence:
`Workspace/pipeline-settings-final-*.log`. Initial device-free validation and
fixture compilation/export errors were corrected before these final runs.
No controlled performance benchmark was run; these checks emit no graph operations
or GPU submissions. D3D12 remains excluded.

The user's latest direction makes removal of built-in pipeline orchestration and
conversion of the entire rendering stack into small reflected operations the next
implementation priority. Selecting a node must expose its authored pin properties
through the existing property grid, respecting reflection attributes. Configuration
work above does not close that migration or the remaining complete-objective gates.

## Independent output operations checkpoint

Upscale, Bloom, ToneMapping and ColorGrading now have concrete reflected pass
classes, individual member properties, explicit resource pins and instance-owned
GAL caches. Their algorithms and mutable configuration have been removed from the
stage implementation. Branches use occurrence-local allocation names and compiled
bindings; the algorithms do not query a global resource-name registry. Compute
requirements are collected from each operation during compilation.

Stage prerequisite and duplicate-stage rejection have been removed. Graph
connectivity determines ordering. Debug rendering accepts explicit color/depth
handles, including editor texture and scene-camera previews. Native pin discovery
preserves connections while authored properties are invalid; compilation reports
configuration errors. Reflected display-setting defaults now match native defaults.
Versioned source and descriptor conversions preserve node/pin identities. SDK
source assets reproduce their packaged resources through the production compiler.

Debug builds of GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and
EditorEngineProcessFrameworkTest pass. All 29 Core, 48 Vulkan GAL, 73 Vulkan
editor-engine and 15 Qt editor subtests pass. A real GPU readback verifies two
independently configured branches with 8x8/16x16 allocations and distinct bloom
thresholds over two frames. Nested shared-definition occurrences, reflected
configuration validation, legacy descriptor conversion, deterministic compilation,
shader failure/recovery and SDK export are covered. Evidence: ignored
Workspace/output-pass-*-final.log and output-operation-checkpoint-build.log.
The initial fixture barrier mismatch and editor default/pin-discovery failures were
fixed before these runs. D3D12 is excluded; no performance benchmark was run.

The remaining stage dispatcher and Builtin files are still migration work. This
checkpoint does not satisfy the request to remove the entire family or decompose
all rendering algorithms. Temporal reconstruction, exposure and presentation are
next; their state must belong to each independent pass occurrence.

## Independent exposure, temporal reconstruction and presentation

LuminanceHistogram, AutoExposure, TemporalAntiAliasing and Present are now ordinary
reflected pass types with member properties and explicit input/output bindings.
The former post-processing implementation file is deleted. Its four remaining
algorithms, pipeline caches, exposure buffer and TAA history pair have been removed
from the composite runtime. Temporal allocation is a small shared utility, with
failure-injection tests, rather than a stage-owned subsystem. The old memory utility
header and its includes are removed. Shader-failure coverage now lives in
RenderOperationFailureTest.cpp.

Each exposure/TAA occurrence owns its writable history. Frame constants snapshot
camera exposure, timing, dimensions and effective settings during Build. TAA exposes
previous color through an optional temporal pin, separate from current-frame input
dependencies. Native texture requirements describe dimensions, samples, format where
fixed, and shader-resource access. Presentation consumes its explicit graded-color
binding and publishes its target binding. Frame-local binding inspection replaces
test dependence on globally discovered output names.

The editor no longer fills stage-template/property arrays. The GetStages API and
stage configuration validator are deleted; available nodes come from reflected pass
types and category attributes. A regression verifies that invalid pass configuration
does not delete connected pins or their persistent property objects. Versioned source
conversion now also handles prior version-2 stage nodes. SDK documents and binaries
were regenerated through the production compiler; the temporary packaging code was
removed from the test fixture.

Debug builds of GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and
EditorEngineProcessFrameworkTest pass. All 29 Core, 49 Vulkan GAL, 73 Vulkan
editor-engine and 15 Qt subtests pass in Workspace/temporal-verified-*.log.
GPU readback verifies two instances of one shared exposure definition produce 1x
and 4x exposure, retain different buffers across three frames, and reuse those
allocations after explicit invalidation. Two independent TAA instances retain
separate histories through 64 resizes. Existing interleaved-view history checks,
shader failure/recovery, asset reload, editor inspection and deterministic SDK
export also pass. Initial test binding-index and history-import compilation errors
were corrected before final verification. One earlier full Qt run failed deleting
an asset-processor output at AssetProcessorProfileTest.cpp:62; two subsequent full
runs passed. That intermittent cause is unresolved. D3D12 remains excluded.
No controlled performance benchmark was run. Logical history publication adds an
import-only graph node that can be culled; no separate GPU submission is requested.

This is eight independently reflected operations from the original 84-operation
inventory. Ten composite branches and 76 operation-data structures remain in the
old runtime, with fourteen Builtin implementation files and the public selector
header still present. The complete requested deletion is not finished. The remaining
visibility, geometry, shadow, lighting, material, screen-effect and visualization
algorithms must be moved into real independent passes and authored compositions;
renaming the remaining runtime would not satisfy that work.
