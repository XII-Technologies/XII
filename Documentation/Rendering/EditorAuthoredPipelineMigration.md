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

The resource-selection factories, composite stage implementation and render-pin
configuration integration remain migration targets; these checkpoints do not
close the complete objective. Local evidence is in ignored
`Workspace/pipeline-pin-*.log` and `Workspace/pipeline-assets-*.log` files.
