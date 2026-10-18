# Native rendering pass rebuild

Current source inventory: **101 concrete reflected operation classes** in
`Source/Engine/GraphicsCore/Pipeline/Passes`. The authoring interface/subgraph
classes and helper implementations are excluded. This is an inventory of real
classes, not a claim that the complete renderer has been rebuilt.

The stage-selector implementation, runtime, payloads, helpers and binary/source
compatibility converter have been deleted. No rendering Builtin source file
remains. The old Deferred and EditorScene source/binary assets have also been
removed completely; rebuild their graphs from native operations. The removed game/editor/asset-preview policy defaults are now empty. Previously
authored references to these paths produce missing-resource errors. They must be
updated or satisfied by genuinely rebuilt assets before the migration is complete.

## Operations in source

| Directory | Count | Reflected operations |
| --- | ---: | --- |
| AmbientOcclusion | 2 | `xiiAmbientOcclusionPass`, `xiiAmbientOcclusionTemporalPass` |
| Atmosphere | 2 | `xiiAtmosphereCompositePass`, `xiiAtmosphereConstantsPass` |
| Composition | 3 | `xiiRadianceAddPass`, `xiiConfidenceRadiancePass`, `xiiVolumetricCompositePass` |
| Exposure | 2 | `xiiAutoExposurePass`, `xiiLuminanceHistogramPass` |
| Geometry | 8 | `xiiDepthRasterPass`, `xiiDirectionalMeshConstantsPass`, `xiiDirectionalMeshRasterPass`, `xiiGBufferRasterPass`, `xiiMeshDrawCommandsPass`, `xiiMatrixRasterConstantsPass`, `xiiSurfaceRasterConstantsPass`, `xiiVertexPullDrawCommandsPass` |
| GlobalIllumination | 13 | `xiiDDGIProbeSchedulePass`, `xiiDDGIProbeUploadPass`, `xiiDDGIAtlasPreparePass`, `xiiDDGIEnvironmentSeedPass`, `xiiDDGIProbeGatherPass`, `xiiScreenSpaceGlobalIlluminationCompositePass`, `xiiScreenSpaceGlobalIlluminationPass`, `xiiSparseVoxelGatherPass`, `xiiSparseVoxelSchedulePass`, `xiiSparseVoxelUploadPass`, `xiiSparseVoxelPoolPreparePass`, `xiiSparseVoxelEnvironmentSeedPass`, `xiiIrradianceLayerPass` |
| Lighting | 13 | `xiiBRDFLutPass`, `xiiLightClusterViewPass`, `xiiClusterBoundsPass`, `xiiClusterListClearPass`, `xiiClusterLightAssignmentPass`, `xiiClusterLightSamplingPass`, `xiiWorldLightsUploadPass`, `xiiWorldDirectionalLightPass`, `xiiEnvironmentLightConstantsPass`, `xiiDirectionalSurfaceLightingPass`, `xiiClusteredLocalSurfaceLightingPass`, `xiiDiffuseSurfaceLightingPass`, `xiiEnvironmentSpecularPass` |
| Output | 5 | `xiiBloomPass`, `xiiColorGradingPass`, `xiiPresentPass`, `xiiToneMappingPass`, `xiiUpscalePass` |
| Particles | 2 | `xiiParticleIntegratePass`, `xiiParticleIntegrationConstantsPass` |
| Preparation | 2 | `xiiViewConstantsPass`, `xiiWorldInstancesUploadPass` |
| RayTracing | 2 | `xiiRayTracingScenePass`, `xiiSceneRayProbePass` |
| Reflections | 3 | `xiiHybridReflectionCompositePass`, `xiiScreenSpaceReflectionsPass`, `xiiScreenSpaceReflectionsTemporalPass` |
| Refraction | 2 | `xiiScreenSpaceRefractionPass`, `xiiScreenSpaceRefractionSnapshotPass` |
| Residency | 2 | `xiiGeometryUploadPass`, `xiiMaterialUploadPass` |
| Sensors | 4 | `xiiDepthSensorPass`, `xiiRgbSensorPass`, `xiiInfraredSensorPass`, `xiiLidarSensorPass` |
| Shadows | 8 | `xiiShadowCascadePass`, `xiiVirtualShadowFeedbackPass`, `xiiVirtualShadowFeedbackReadbackPass`, `xiiVirtualShadowFeedbackResetPass`, `xiiVirtualShadowTablesPass`, `xiiVirtualShadowSamplingConstantsPass`, `xiiVirtualShadowRasterWorkPass`, `xiiVirtualShadowPagePublishPass` |
| Streaming | 3 | `xiiGeometryLodFeedbackPass`, `xiiGeometryLodFeedbackReadbackPass`, `xiiGeometryLodFeedbackResetPass` |
| Temporal | 3 | `xiiBufferHistoryPass`, `xiiTemporalAntiAliasingPass`, `xiiTextureHistoryPass` |
| Transparency | 1 | `xiiWeightedBlendedOITResolvePass` |
| Utility | 8 | `xiiBufferExportPass`, `xiiBufferForwardPass`, `xiiClearTexturePass`, `xiiCounterResetPass`, `xiiDepthRegionClearPass`, `xiiDepthTargetPass`, `xiiTextureForwardPass`, `xiiTextureSelectionPass` |
| Visibility | 10 | `xiiDepthPyramidPass`, `xiiInstanceCullPass`, `xiiInstanceOcclusionPass`, `xiiLodHistoryResetPass`, `xiiMatrixVisibilityViewPass`, `xiiMeshletCullPass`, `xiiMeshletOcclusionPass`, `xiiMeshletDispatchBoundsPass`, `xiiMeshletDispatchPass`, `xiiVisibilityViewPass` |
| Visualization | 3 | `xiiDebugOverlayPass`, `xiiSurfaceQueryPass`, `xiiViewRenderModePass` |

Every operation above declares concrete typed pin members through
`XII_MEMBER_PROPERTY`. No static pin type table or stage selector is used.
Properties remain reflected members and retain normal editor attribute handling.

## State ownership

- WorldInstancesUpload publishes the existing world's sparse instance/pose uploads.
  Repeated occurrences use the same graph upload and buffer versions. No hierarchy,
  scene object registry or transform propagation is added.
- InstanceCull applies supplied flags, masks, frustum and residency tests and writes
  generation-validated LOD selections. LodHistoryReset initializes every entry on
  the GPU, including unselected/empty candidates. Neither consumes live world state.
- MeshletDispatchBounds reduces the selected, generation-validated LODs on the GPU;
  MeshletDispatch creates indirect compute dimensions without a CPU count readback.
  MeshletCull compacts supplied work into graph-owned pairs, using primitive ranges,
  conservative transformed/posed bounds and eligible normal cones. Its own reflected
  budget controls physical allocation. The count saturates at capacity and a required
  overflow output reports dropped work; consumers must provide an explicit overflow
  policy. No renderer silently treats an overflowed list as complete.
- VertexPullDrawCommands converts the supplied compact meshlet list into portable
  DrawIndirect arguments and an explicitly bounded indirect count. Residency metadata
  and primitive ranges are validated before a candidate emits work. It owns
  no rasterization, shader asset residency, scene lifecycle or CPU count readback.
- SurfaceRasterConstants captures an immutable current-view snapshot and device table
  bases. SurfaceQuery consumes that uniform buffer and explicit draw/count buffers,
  emits one managed raster operation, and publishes surface identity/depth/normal
  data plus the nearest depth texture. Shader selection is an explicit reflected
  property; unsupported mesh shading fails rather than changing the authored path.
- BufferHistory retains structured GPU buffers in completion-checked private
  slots, publishes explicit previous-frame pins and declares GPU readiness waits.
- VisibilityView captures camera/frustum and selected component indices during
  setup. Execute consumes owned immutable frame data. Its parameters belong to
  each reflected occurrence. LOD tokens advance with that occurrence, including
  across interleaved device frames from other worlds.
- GeometryUpload and MaterialUpload publish the existing device residency services.
  They allocate no world-specific copies of shared mesh or material assets.
- CounterReset creates one graph-owned counter with a reflected initial value;
  visibility and indirect work can compose independently initialized counters.
- BRDFLut requests the device-shared immutable lookup only when present. Generation
  signals a GPU fence; subsequent consumers declare device-side readiness waits.
- ViewRenderResourceManager now owns pipeline instances directly. There is no
  hidden rendering runtime or automatically created lighting context behind a view.

## Deleted operations still requiring native rebuilding

Deletion of a legacy operation does not count as conversion. Existing low-level
GAL services and shaders remain available as implementation building blocks.
Do not restore the deleted runtime or put its complete state into a new manager.

| Responsibility | Work still required |
| --- | --- |
| Visibility | Instance/frustum/flags/mask culling and LOD selection/history are native. Meshlet culling and indirect dispatch preparation are now native; the InstanceVisibility and nested MeshletVisibility assets exercise them. Depth-pyramid construction and texture history are now independent native nodes. Instance occlusion is now an independent node with an ordinary authored resource. Meshlet occlusion is now independently authored as well. LOD streaming table initialization, demand generation and asynchronous request readback are independent native operations, consumed by the authored sample graph. Both vertex-pull and mesh-shader indirect command preparation are native. The old visibility manager/system and their shaders are deleted; remaining sample shadow/VSM/raster orchestration needs conversion. |
| Geometry | Vertex-pull/mesh indirect draw preparation and nearest depth raster (mesh or vertex pull) are native. GBuffer material/identity/normal/roughness raster and object/skinning motion consume explicit inputs. Standalone camera/jitter history and velocity dilation remain required. SurfaceQuery now emits raster work directly from supplied constants/commands, without calling the legacy picking orchestrator. |
| Lights | Cluster layout/bounds, list clear/assignment, emitted-power CDF preparation, world light snapshot/upload, directional selection and environment constants are independent native operations with authored assets. The old lighting context and registry are deleted. Shared device IES residency and reflection-probe selection remain. |
| Shadows | Directional cascade construction/snapshot upload is native. Local atlas allocation/clear, directional/spot/point raster, ray shadow tracing/denoising, contact shadows and explicit virtual residency/publication operations. Feedback reset, generation and private asynchronous readback are now native; page raster uses ordinary ShadowMeshDepth instances. Replace the deleted composite-only ShadowMemory fixture with native operation tests. |
| Atmosphere/environment | Transmittance, multiple scattering, sky irradiance convolution and environment/probe resources. Constants and aerial perspective composition are native. |
| Direct illumination | Directional and clustered analytical local surface evaluation are now independent native operations in WorldSurfaceLighting. ReSTIR DI temporal/spatial/visibility/emissive visibility, local shadows, all-directional evaluation and complete forward/deferred composition remain. |
| Indirect illumination | DDGI and sparse-voxel scheduling/upload/preparation/environment seeding are independent native operations; their old context GPU orchestration is deleted. Geometry-dependent radiance updates, probe/environment preparation, ray GI tracing, ReSTIR GI temporal/spatial reuse and denoising remain. Confidence layering, diffuse material response and environment-specular BRDF response are now native operations; complete scene compositions remain open. |
| Reflections | Ray-traced reflections and denoising. Screen trace, temporal filter and hybrid composition are native. |
| Volumes | Froxel allocation, fog initialization, light injection, integration, resolve and temporal reprojection. |
| Decals | Upload, cull/batch, projected resolve and mesh decal draw. |
| Transparency | Transparent geometry and particle raster. Particle integration and weighted resolve are native. |
| Post-processing granularity | Split remaining multi-operation implementations, including bloom extraction/downsample/filter/upsample, at meaningful graph resource boundaries. Existing reflected classes are not automatically the final decomposition. |
| Assets/consumers | Rebuild complete game, editor, forward/deferred, shadow and specialized graphs using supported native operations. DepthOnly and GBuffer currently cover their explicit geometry responsibilities. Convert custom sample graph orchestration into authored resources. |

CPU frustum/source-update/draw-list arrays from the deleted runtime are not a new
scene API to reproduce. World transforms, component/resource identities, geometry
selection and incremental GPU records must continue to come from existing owners.

## Initial rebuild verification (historical)

Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest and EditorFrameworkTest, including the editor
plugins and processes they depend on (`Workspace/native-staticlink-build.log`).

- Full GraphicsCore CPU suite: 40 subtests pass
  (`Workspace/native-rebuild-core-suite.log`).
- Vulkan NativePreparation: shared immutable BRDF lookup, repeated nested
  occurrences, two views, compute-to-graphics readiness and analytic readback pass
  (`Workspace/native-preparation-gpu.log`).
- Vulkan PipelineExtraction: hybrid demand, ray deformation and world instances
  pass. Native producer/counter buffer readback validates the camera parameters,
  transforms, object identity, zero/nonzero independent counters and shared sparse
  uploads (`Workspace/native-preparation-world-gpu.log`).
- Qt EditorDocuments: all 15 subtests pass, including node/pin properties and
  deterministic export of the seven retained SDK assets
  (`Workspace/native-preparation-library.log`).
- XII's StaticLinkUtil regenerated GraphicsCore's retention anchors, covering all
  41 native pass translation units. It reports an existing implementation-header
  include warning for ResourceHandleReflection.h. A separate static-library build
  was not run (`Workspace/native-pass-staticlink.log`).

Initial verification exposed GGX near-mirror NaNs; the native generation shader
now uses a stable denominator and clamps the hemisphere sample. The readback test
passes against the analytic Schlick limit. An old CPU test dereferenced the removed
Deferred asset; it has been rebuilt around an actual native authored resource and
checks runtime display parameters through their native resolver. Final CPU tests
pass after these corrections.

The two-camera world regression reports 288 bytes uploaded once for one changed
instance, and zero bytes for a warmed static frame. Its uncontrolled Debug packing
sample measured 2.574 ms/update for 1024 unchanged components. This is not a controlled
comparison or GPU-frame benchmark; no speedup is claimed.

D3D12 is excluded. Full rendering/GPU/editor-engine suites are not passing evidence
for this phase: the old complete renderer assets are deleted. The following test
consumers still refer to those assets and must be rebuilt with native graphs:

- GraphicsFoundationTest/Pipeline/RenderPipelineExecutionTest.cpp
- EditorEngineProcessFrameworkTest/RenderOperationFailureTest.cpp
- EditorEngineProcessFrameworkTest/SceneMeshPickingTest.cpp
- EditorEngineProcessFrameworkTest/SceneShapeIconTest.cpp
- EditorEngineProcessFrameworkTest/SensorMemoryTest.cpp
- EditorEngineProcessFrameworkTest/TemporalHistoryTest.cpp
- EditorEngineProcessFrameworkTest/ViewportTest.cpp

Application/editor rendering that relied on the removed default graphs remains
unavailable until native raster/lighting/shadow operations and their assets are
rebuilt. This migration is active, incomplete, and has no compatibility fallback.


## Native visibility verification - 2026-10-16

Three classes added: InstanceCullPass, LodHistoryResetPass and BufferHistoryPass.
All consume typed member pins, with no static pin tables, stage checks, scene
ownership, automatic extraction demand, or new service manager. AsyncCompute is a
reflected culling property. Persistent temporal storage is an independently wired
operation rather than hidden culling state. A missing previous input uses an
initialized invalid record; previous/current logical aliasing and invalid layouts
fail with occurrence diagnostics.

Debug builds pass for all four affected test targets and their editor consumers
(`Workspace/native-visibility-final-build.log`, native-visibility-resize-build.log).
All 41 GraphicsCore CPU subtests pass (`Workspace/native-visibility-core.log`).
The Vulkan NativeVisibility test runs eight phases through two views, each with two
instances of one serialized definition. GPU readback verifies masks, flags,
frustum rejection, candidate selection, LOD hysteresis, geometry generation changes,
recycled object IDs, explicit temporal feedback, per-view invalidation, empty input,
67-to-129-entry resizing and the final record beyond the original capacity. Every
occurrence is checked; writable previous buffers remain distinct. The CPU test
accepts the temporal cycle and rejects its equivalent intra-frame cycle.

Vulkan PipelineExtraction passes after the native visibility-token change
(`Workspace/native-visibility-world-gpu.log`). World producer readback verifies
independent camera tokens across an unrelated device frame and a cut in only one
camera. Incremental uploads still report 288 bytes once for one changed instance
across two cameras, and zero warmed static upload bytes. The uncontrolled Debug
packing sample is 2.436 ms/update for 1024 unchanged components; no performance
comparison is claimed. All 15 Qt EditorDocuments tests pass, including the retained
asset library and pin properties (`Workspace/native-visibility-qt.log`).

Initial GPU fixture failures placed a sphere tangent to the frustum, where correct
conservative culling keeps it visible; the outside test now moves it fully beyond
the plane. Resize fixture expectations were updated to the actual candidate capacity.
Final results above pass after these fixture corrections.

These are native source operations and tests, not a completed world visibility or
rendering asset. Authored native visibility assets, remaining meshlet/Hi-Z/feedback
nodes and the missing complete renderer graphs still need implementation. History
copy bandwidth, deep GPU queue overlap and large-scene dispatch limits have not been
benchmarked. The full renderer suites remain unverified while their old complete
pipeline assets are absent. D3D12 remains excluded.


## Authored meshlet visibility verification - 2026-10-16

The ordinary InstanceVisibility asset contains seven native operations, explicit
previous-frame LOD wiring and eight boundary outputs. MeshletVisibility references
that independently authored definition and adds six operations: three independently
wired CounterReset occurrences, dispatch bounds reduction, indirect dispatch
preparation and meshlet culling. Its seven authoring nodes expand to thirteen
executable occurrences, with twelve output mappings. The interface exposes the
meshlet overflow flag. No default pipeline selector or rendering Builtin file was
introduced. The unused meshlet budget has been removed from VisibilityView: that
property belongs to MeshletCull, which owns the output allocation.

Both source assets are authored with actual Qt document/node commands, reflected
member properties, persistent document identities and the production exporter.
The permanent authoring tests write only test output. Nine SDK assets now have
binary packages checked against deterministic production export. The world GPU
regression consumes two references to the nested MeshletVisibility asset, with an
override reaching the second instance's nested VisibilityView. Two cameras share
the canonical world upload and retain separate temporal state.

Validation:

- Debug GraphicsCoreTest, GraphicsFoundationTest and EditorFrameworkTest builds
  pass, including their dependent engine/editor targets
  (`Workspace/native-meshlet-author-build.log`, `native-meshlet-final-build.log`).
- Full GraphicsCore CPU suite passes (`Workspace/native-meshlet-final-core.log`).
  This includes reflected pin/property budgets, temporal cycles, composition,
  provenance, serialization and selective invalidation tests.
- Vulkan NativeVisibility passes both tests (`Workspace/native-meshlet-final-gpu.log`).
  Readback covers real component/residency/removal, two views and repeated nested
  instances, LOD history/generation changes, primitive ranges, posed bounds, rotated
  nonuniform and mirrored transforms, empty work, growing buffers and a deliberately
  undersized instance-specific meshlet budget. Counts saturate safely, overflow is
  reported, and sampled compacted pairs identify eligible instances and meshlets.
- NativeAssetAuthoring passes both source/export tests
  (`Workspace/native-meshlet-author.log`). Qt EditorDocuments checks all nine SDK
  assets and pin-property integration (`Workspace/native-meshlet-final-qt.log`).

Intermediate verification caught an incorrect dependency path in the new authoring
fixture, a shared-pointer declaration in its GPU test and an HLSL reserved identifier.
They were corrected; the final build and focused results above pass. No controlled
meshlet performance benchmark or complete application-rendering test has been run.
D3D12 remains excluded. A meshlet overflow fallback, the remaining legacy visibility
service consumers, native raster/lighting/shadow operations and complete game/editor
pipeline assets remain work in progress. The architecture migration is incomplete.


## Native vertex-pull command preparation - 2026-10-16

VertexPullDrawCommandsPass is an independent compute operation in the Geometry
source directory. Its six data inputs and two data outputs are typed member pins;
AsyncCompute is a reflected member property. Output records match GAL's portable
DrawIndirect layout (vertex count, instance count, first vertex, first instance).
First instance resolves the compact meshlet index without a backend-specific draw-ID
semantic. The output count is bounded by physical input/output capacity. Invalid
active candidates and unused slots are zeroed. Hardware compute bounds are validated
before graph emission. The current implementation dispatches over the backed meshlet
capacity; sparse-work dispatch optimization and its bandwidth cost are not benchmarked.

The VertexPullDraws source asset references MeshletVisibility and one native command
producer. Its two authoring nodes expand to fourteen operations and expose fourteen
buffer interfaces, including overflow. Both source and binary are produced with real
Qt document/node commands and the production exporter. The real-world Vulkan test
consumes two instances of this resource across two cameras, overrides a VisibilityView
three levels down, and checks generated draw arguments before and after component
removal. Synthetic tests use distinct LOD metadata ranges and verify nonzero first-
vertex offsets for clipped primitive ranges. MeshletCull now rejects metadata whose
LOD does not match the selected, generation-validated history.

Validation evidence for this phase is recorded in Workspace/native-draw-build.log,
native-draw-final-build.log, native-draw-final-core.log, native-draw-final-gpu.log,
native-draw-author.log and native-draw-final-qt.log. Final builds pass for
GraphicsCoreTest, GraphicsFoundationTest and EditorFrameworkTest, with dependent
engine/editor targets. All 44 GraphicsCore CPU subtests, both Vulkan visibility
subtests, all three NativeAssetAuthoring subtests and all 15 Qt EditorDocuments
subtests pass. The asset-library test compares all ten packaged binaries with
production exports and repeat exports. The initial CPU fixture assumed
that only data pins exist; it was corrected to test the required input and indirect
output contracts while preserving the common ordering pins. D3D12 is excluded.

This prepares draw commands, not a completed raster pipeline. No rendering Builtin
file or compatibility graph was added. Ten ordinary SDK assets are now packaged.
The next deletion targets remain GpuScenePicking's hidden constants/command/raster
orchestration, SurfaceQueryPass's call into that orchestration, and the editor
SceneMeshPickingPass's embedded visibility/raster stack. Its capture/query state
must become an explicitly wired consumer of a native surface output. Existing
SceneMeshIdentity, SceneLodSelection and SceneLodSections tests still call the old
helper and need native resource fixtures before it is removed. The complete game,
editor, lighting, shadow and remaining specialized pipelines are still unfinished.


## Explicit surface rasterization - 2026-10-16

SurfaceQueryPass has been rebuilt as raster execution only. Its calls into
GpuSceneRasterManager::AddPickingPass, implicit vertex-pull command generation,
residency lookup, hidden constants production and automatic mesh-to-vertex fallback
are removed. RasterConstants, DrawCommands and DrawCommandCount are required typed
member inputs. The existing reflected UseMeshShaders property explicitly selects a
supported shader path, with vertex pulling the default. Unsupported mesh/wireframe
features and malformed canonical layouts fail during Build with occurrence diagnostics.
The pass publishes both RGBA32UInt surface data and a shader-readable D32 depth image.
It permits compatible hardware render-pass scheduling rather than disabling merging.
Shader/sampler references are retained in frame-owned execution data.

SurfaceRasterConstantsPass captures the current view and device residency bases,
with reflected vertex-layout properties and checked offsets. It produces the shared
uniform layout without temporal history, scene ownership or a new service manager.
Motion rasterization must receive a separate explicit history contract.

The ordinary SurfaceQuery asset composes VertexPullDraws, MaterialUpload,
SurfaceRasterConstants and SurfaceQuery. Four authored nodes expand to seventeen
native operations and expose sixteen interfaces. DebugVisualization is rebuilt from
scratch around two visualization/overlay nodes; wireframe surfaces are explicit
external inputs, with no nested rasterization hidden inside debug composition.

The world regression now consumes the SurfaceQuery asset across two independent
views and two nested occurrences, with a mask override four levels down. It reads
actual surface IDs, encoded normals and depth, including the filtered and removed-
component cases. Test texture readback restores the source state after copying.
This is surface raster verification, not a complete lighting or editor pipeline.

Pending deletion remains explicit: GpuScenePicking.cpp and its AddPickingPass API
are still consumed by the old editor capture pass and direct native identity/LOD
fixtures. SceneMeshPickingPass still embeds visibility/raster orchestration and
must become a surface-input capture/query consumer with separate filtering nodes.
The complete renderer migration remains incomplete; no compatibility fallback or
rendering Builtin file has been added.


Validation for the surface phase:

- Debug GraphicsCoreTest, GraphicsFoundationTest and EditorFrameworkTest builds
  pass, including dependent engine/editor targets (`Workspace/native-surface-final-build.log`,
  native-surface-library-build.log).
- All 45 GraphicsCore CPU subtests pass (`Workspace/native-surface-core.log`).
- Both Vulkan NativeVisibility tests pass with actual vertex-pull surface/depth
  readback (`Workspace/native-surface-final-gpu.log`). Query-only varyings omit
  tangents and previous clip positions; the final Vulkan run has no unused-output
  interface warning. The native mesh-shader raster path has not been executed by
  this focused regression.
- All five NativeAssetAuthoring subtests pass (`Workspace/native-surface-author.log`).
  All 15 Qt EditorDocuments subtests pass, including deterministic production and
  repeat exports of eleven SDK packages (`Workspace/native-surface-qt.log`).
- Interface authoring now declares reflected Name/Input defaults consistently with
  the C++ definition; fresh inputs are explicitly authored as inputs. No legacy
  converter was introduced. Raster constants reject unaligned or out-of-range
  vertex layouts and a missing/disabled camera.

Intermediate builds caught a default-device shared-pointer declaration, a missing
standard-library include and an incorrect Order-pin enum in the fixture. Authoring
also caught the input-default mismatch described above. Final results pass after
correction. No controlled raster benchmark, complete game/editor renderer test or
D3D12 validation was run. The pending legacy helper/editor migrations above remain
required work.

## Legacy picking orchestrator deletion - 2026-10-16

Deleted `Visibility/Implementation/GpuScenePicking.cpp`, the public AddPickingPass
and IsPickingSupported APIs, their eager shader registrations, and the obsolete
GpuSceneIndexedCommandBuild shader. Repository source searches find no remaining
references to these APIs or shader. Surface rasterization and vertex-pull command
preparation remain separate reflected operations. Native geometry fixtures bind
their inputs through member RTTI and explicit handles rather than rediscovering
an orchestrator's internal buffer names.

SceneMeshPickingPass now captures a supplied RGBA32UInt Surface pin and forwards
it for other consumers. It no longer owns visibility contexts, geometry/material
table lookup, extraction packet lists, selected-object rendering or rasterization.
The old visibility resize path, capacities, candidate arrays and context destruction
have been removed from its private state. Its remaining completion-checked image
slots and validated world-owner metadata serve only asynchronous picking. Enabled
is reflected; Editor.Picking can disable capture without changing the supplied graph.

The existing world-derived GPU data now exposes a runtime data revision incremented
only when packed records, membership or pose bytes change. This reuses existing
change/version tracking, with no new scene registry or CPU record scan. Capture
invalidates previous images on that revision, view candidate selection and relevant
selection-policy changes. Resource events retain their loader-thread atomic epoch;
authoritative owner/component and transform validation still guards resolved pixels.

The old editor orchestration fixture has been rebuilt around the actual SurfaceQuery
resource and two capture occurrences in each of two views. It verifies occluding
world objects, tag filtering, moving/removing/recreating components, background,
nonzero viewport origins, independent camera invalidation, capture disable/re-enable
and rectangle queries, including a device configuration with mesh shaders disabled.
Selected-object and transparency filtering must be supplied by independently wired
preparation nodes; their former embedded implementation is removed. The full editor
pipeline and selection-overlay composition still need authored replacements.

Builds pass for GraphicsCoreTest, GraphicsFoundationTest,
EditorEngineProcessFrameworkTest and EditorFrameworkTest, including their dependent
engine/editor targets (`Workspace/native-capture-build.log`, final-build.log,
mesh-grid-build.log and fixture-final-build.log; the first log contains intermediate
fixture compilation failures, the later logs contain the corrected builds).

Final validation:

- All 45 GraphicsCore CPU subtests pass (`Workspace/native-capture-final-core.log`).
- Vulkan PipelineExtraction (3 subtests) and NativeVisibility (2) pass. Transform
  revisions settle on unchanged frames; pose-only revisions are checked by raster
  tests. Two-camera uploads remain 288 bytes once for one changed instance and zero
  bytes for a warmed static frame. The uncontrolled Debug packing sample measured
  2.622 ms/update for 1024 unchanged components; no speedup is claimed.
- NativeMeshCapture (2), NativeSceneIdentity (1), NativeSceneLod (4), MeshletRaster
  (1) and SkinningRaster (3) pass on Vulkan (`Workspace/native-capture-<group>.log`).
  These execute real mesh and indexed surface shaders, masked/translucent coverage,
  dense meshlets, independent LOD sections, deformation and reload isolation.
- All 15 Qt EditorDocuments tests pass, including deterministic exports of eleven
  SDK assets (`Workspace/native-capture-final-qt.log`).

Migration testing caught a fixture lookup of a deleted buffer name and replaced
it with explicit command/count outputs. Native mesh tests also exposed an arbitrary
65535 mesh-grid clamp in SurfaceRasterConstants; it now supports valid wider mesh
grids while rejecting uint linearization overflow, with CPU regression coverage.
An early capture assertion incorrectly required a completed new image to remain
unavailable; it now accepts unavailable or the correct new owner, never the old owner.
The last verification call initially could not execute because automatic approval
review reached its usage limit; it was retried through approval, without bypassing it.

No rendering Builtin files or scene-database classes were restored. The legacy
visibility service, depth/G-buffer/forward raster orchestration and incomplete full
game/editor pipelines remain migration targets. D3D12 is excluded; no controlled
end-to-end rendering benchmark or complete application-renderer validation is claimed.

## Explicit editor candidate selection - 2026-10-16

SceneInstanceSelectionPass in SharedPluginScene is a separate reflected preparation
operation. It filters the current view's world-derived candidate indices using
validated world identities, selection membership and material classification flags.
Enabled, SelectionOnly, PickSelected and PickTransparent are member properties;
the existing editor parameters supply runtime choices. Execute uploads owned index
bytes and never reads mutable world/component storage. Empty selections publish one
invalid-index sentinel. The pass owns no scene registry, geometry residency, temporal
images, visibility dispatch or rasterization. GraphicsCore still has 49 operation
classes; this additional editor operation belongs to its editor plugin.

VisibilityView accepts an optional typed InputCandidates buffer and forwards it
through an explicit graph read. It derives the exact candidate count from the buffer
description and rejects malformed layouts or lists exceeding the world-backed
dispatch range. With no supplied buffer, ordinary view selection remains available.
InstanceVisibility, MeshletVisibility, VertexPullDraws and SurfaceQuery now expose
this optional input through their authored nested boundaries. Their output counts
are unchanged; total interface counts are 9, 13, 15 and 17 respectively.

The new ordinary EditorMeshPicking resource wires selection into SurfaceQuery and
capture after surface rasterization. Three authored nodes expand to nineteen native
operations; outputs expose Surface, SceneDepth and VisibleMeshletOverflow. The editor
capture regression now loads this resource rather than reconstructing its rendering
stack in C++. A second capture of the same surface verifies independent writable
capture state. A nested SelectionOnly override exercises a distinct configuration
of the same definition. This graph supplies mesh picking, not the unfinished complete
editor lighting/selection-overlay pipeline, and no implicit default is installed.

All source assets are generated with document/node commands and production export
inside test output, then their task-owned SDK packages are updated deliberately.
Authoring tests generate their nested dependencies in test output as well, so a new
optional interface does not depend on an old packaged child asset. Twelve SDK assets
now have deterministic source-to-binary and repeat-export coverage. The unrelated
AmbientOcclusion asset hash edit in the worktree is excluded from this phase.

Verification for candidate selection:

- All four affected Debug test targets and their dependent editor/engine targets
  build (`Workspace/native-selection-verification-build.log`).
- All 45 GraphicsCore CPU subtests pass (`Workspace/native-selection-final-core.log`).
- Vulkan PipelineExtraction (3) and NativeVisibility (2) pass after the optional
  input change. The latest uncontrolled static packing sample is 2.709 ms/update;
  sparse upload behavior remains one changed 288-byte instance once across cameras
  and zero warmed static upload bytes (`Workspace/native-selection-<group>.log`).
- NativeMeshCapture's three Vulkan subtests pass (`Workspace/native-selection-final-capture.log`),
  including empty and selected-only lists, selected-owner exclusion, transparent
  material exclusion/re-enable, independent view policies and component lifecycle.
  One subtest disables mesh shaders on the device; rendering still uses the explicitly
  authored vertex-pull path.
- All six NativeAssetAuthoring subtests pass (`Workspace/native-selection-author.log`).
  All 15 Qt EditorDocuments subtests pass, comparing all twelve SDK packages against
  production exports and repeat exports (`Workspace/native-selection-final-qt.log`).

No full game/editor lighting pipeline, selection-overlay presentation or controlled
frame-performance benchmark is claimed. D3D12 remains excluded. Native depth/G-buffer
raster, remaining visibility/feedback orchestration and the complete resource-authored
rendering techniques remain required work before the architecture migration is complete.


## Native depth and mesh draw preparation - 2026-10-16

DepthRasterPass is a standalone reflected raster operation in Geometry. It consumes
canonical scene, pose, material and meshlet buffers plus separately supplied raster
constants and indirect draw/count buffers. It publishes only a shader-readable D32
nearest-depth target. UseMeshShaders explicitly selects mesh or vertex-pull shaders;
unsupported features fail rather than substituting a different path. Material alpha
coverage and skinning use the same canonical shader implementation as surface queries.
Depth-only shader interfaces omit normals, tangents, identity and motion varyings.

The obsolete AddDepthPrepass API, xiiGpuSceneDepthRasterDescription and reflection,
its embedded constants/raster graph construction, eager depth-shader registration,
GpuSceneDepth shader and GpuSceneRasterConstants header have been physically removed.
Both old depth consumers in the native identity/pose fixture now bind DepthRasterPass.
The legacy raster manager still owns G-buffer and Forward+ orchestration; those
remain deletion targets rather than new APIs behind the depth pass.

DepthOnly is an ordinary authored resource containing a VertexPullDraws reference,
MaterialUpload, SurfaceRasterConstants and DepthRaster. Its four authoring nodes
expand to seventeen native operations. Sixteen boundary mappings include the optional
candidate input, fourteen GPU data outputs and SceneDepth. The asset advertises only
Geometry extraction/GpuScene runtime demand, with no light/atmosphere initialization.
The Vulkan world fixture checks the predicted reversed-Z box depth directly across
two views and repeated nested instances, including filtering and component removal.

MeshDrawCommandsPass independently emits a portable uint3 mesh indirect command and
uint command count from supplied compact meshlet work. Reflected MeshGroupCountX/Y
are the row strides and must agree with raster constants. Build validates physical
capacity, padded dispatch dimensions and device limits before adding work. GPU count
is bounded by actual backing capacity, zero work writes deterministic zero arguments
and count, and integer division/remainder avoids overflow. It has no scene ownership,
history, residency, CPU count readback or implicit raster execution. Tests use a 4 x 2
row layout to cover all three dispatch dimensions. Authoring a mesh-specific reusable
visibility/depth resource and migrating remaining legacy mesh-command consumers are
still required; DepthOnly currently authors the explicit vertex-pull path.


Verification for depth and mesh commands:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest
  and EditorFrameworkTest build, including dependent editor/engine targets
  (`Workspace/native-depth-mesh-draw-build.log`).
- All 47 GraphicsCore CPU subtests pass (`Workspace/native-depth-core.log`), including
  independent depth pins/capabilities and mesh-command stride/overflow configuration.
- Vulkan NativeVisibility passes three subtests (`Workspace/native-depth-gpu.log`).
  AuthoredDepthOnlyInstances reads the analytical reversed-Z depth for real world
  meshes across two views/two occurrences, then verifies filtered/removed background.
  Synthetic GPU readback verifies 4 x 2 mesh row strides, z tiling, zero commands for
  empty work and bounded counts after instance-specific meshlet overflow.
- NativeSceneIdentity, MeshletRaster and SkinningRaster pass their five Vulkan
  subtests after migrating depth consumers (`Workspace/native-depth-<group>.log`).
  Mesh/indexed depth, dense primitive coverage, pose updates and context isolation
  execute real native shaders. These fixtures still use legacy visibility/G-buffer
  helpers for their unmigrated responsibilities; no complete-renderer claim follows.
- All seven NativeAssetAuthoring subtests pass (`Workspace/native-depth-author.log`).
  All 15 Qt EditorDocuments subtests pass, including deterministic export and repeat
  export of thirteen SDK assets (`Workspace/native-depth-qt.log`).
- Searches find no remaining AddDepthPrepass API, xiiGpuSceneDepthRasterDescription,
  old depth shader/constants references or rendering Builtin files in source/assets.

No controlled depth/mesh-command performance benchmark or complete game/editor
pipeline test was run. D3D12 remains excluded. The unrelated AmbientOcclusion asset
hash edit remains outside these commits. The architecture migration is incomplete.


## Native G-buffer raster and raster service deletion - 2026-10-16

GBufferRasterPass is a single reflected Geometry operation. Its eleven typed inputs
supply canonical scene/pose/geometry/material records, compact meshlet work,
raster constants, indirect arguments/count and a matching read-only D32 attachment.
Seven typed outputs publish albedo/occlusion, octahedral normal, material parameters,
emissive/transmission, compact normal/roughness, velocity and generation-validated
GPU object identity. Mesh and vertex-pull shaders are explicit authored alternatives.
The operation owns no world lifecycle, visibility, history, upload or pass selection.
Its frame data retains shader/sampler/rasterizer references; depth is neither cleared
nor rewritten. It permits normal render-graph merging rather than forcing a submission.

The repository audit found only fixture callers of AddGBufferPass and no callers of
AddForwardPlusPass. Those fixtures now bind GBufferRasterPass directly, preserving
object and skinning motion through supplied previous-view constants. The entire
GpuSceneRasterManager service, private state, startup/shutdown registration, public
raster description/output facade, embedded constant/raster graph construction and
old GpuSceneGBuffer/GpuSceneForwardPlus shader files are deleted. No compatibility
facade survives. The unused Forward+ entry point has been removed; a new independent
forward-lighting operation and authored graph are still required. Fixture-only layout
values are confined to the test helper and are not a replacement production API.

GBuffer is an ordinary source/binary resource with five authored nodes expanding to
eighteen operations. It references VertexPullDraws and explicitly connects material
upload, raster constants, depth and material raster. Twenty-three boundary mappings
include optional candidates, fourteen GPU outputs, depth and seven material targets.
It requests Geometry/GpuScene only. This resource currently uses current-view raster
constants: camera/jitter history still needs its independent native producer before
a complete temporal game/editor pipeline is available. Object/pose velocity from
supplied previous transforms remains supported by the raster operation and fixtures.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest
  and EditorFrameworkTest plus dependent engine/editor targets build
  (`Workspace/native-gbuffer-build-final.log`). The first build caught a missing
  XII POD annotation on a new half-float readback record; it was fixed and the
  affected targets then compiled successfully.
- All 48 GraphicsCore CPU subtests pass (`Workspace/native-gbuffer-core.log`).
- Vulkan NativeVisibility passes all four subtests, including real GPU readback of
  all seven G-buffer targets across two views/two independently configured nested
  instances, analytical nearest depth, empty/filtered work and component removal
  (`Workspace/native-gbuffer-gpu.log`).
- NativeSceneIdentity, MeshletRaster, SkinningRaster and NativeSceneLod pass nine
  Vulkan subtests. These verify mesh and vertex-pull material raster, pose velocity,
  dense primitive coverage, reload, LOD sections and no-mesh-shader queries
  (`Workspace/native-gbuffer-<group>.log`). An additional attempted filter named
  NativeSceneLodSections matched no test; its section tests are actually part of
  the passing NativeSceneLod group, not an unverified separate group.
- All eight NativeAssetAuthoring subtests pass (`Workspace/native-gbuffer-author.log`).
  All fifteen Qt EditorDocuments subtests pass, including deterministic production
  export/repeat export of fourteen SDK resources (`Workspace/native-gbuffer-qt.log`).
- Source/asset searches find no old raster manager, public facade, G-buffer helper,
  old Forward+ shader path or rendering Builtin files. Static-link retention removes
  the old registration and retains the new reflected operation.

No controlled performance benchmark or complete game/editor/lighting pipeline
validation was run. D3D12 remains excluded. Legacy visibility/Hi-Z/feedback and
lighting/shadow orchestration still require decomposition and deletion; complete
forward/deferred assets and camera/jitter history remain unfinished. The unrelated
AmbientOcclusion asset hash edit is excluded from this phase. The overall migration
is incomplete.


## Independent depth pyramids and texture history - 2026-10-16

DepthPyramidPass builds a transient R32Float reversed-Z minimum-depth mip chain from
an explicit single-sample D32Float or R32Float image. It has typed SceneDepth and
DepthPyramid members plus reflected AsyncCompute. It performs no history allocation,
scene extraction, visibility orchestration or frame rotation. Internal mip barriers
restore the render graph's whole-image state before downstream consumers run. Odd
source rows/columns are included in overlapping conservative footprints rather than
being discarded by a fixed 2 x 2 reduction. No unused data categories are requested.

TextureHistoryPass retains an explicitly supplied texture in completion-checked
storage private to each pass occurrence. PreviousTexture is an optional read-only
previous-frame output, with GPU readiness waits. All mips and array slices are copied
explicitly: the Vulkan CopyTexture implementation covers only the base subresource.
Slots are reused only after their last writer/consumer frame completes, and the last
published image is excluded from the next write. Recording a copy promotes history;
an abandoned setup/compile does not. Invalidation drops availability and allocation
replacement preserves resources still held by recorded work. The operation owns no
scene, camera, depth reduction or visibility policy.

DepthPyramid is a normal authored resource containing these two operations, with
SceneDepth input and current/previous depth outputs. Its temporal/optional boundary
contract survives editor export and hierarchical expansion. It requires Compute,
with no Geometry, Lights, Atmosphere or world runtime feature initialization.

The unused private view visibility-context handle, creation/destruction gate and
PrepareRenderResources loop are deleted. Views invalidate their actual pass objects
through the pipeline instance. Their unused virtual-shadow upload argument and
transitive visibility/shadow includes are removed. The unused manager APIs
PrepareHiZHistory, InvalidateHiZHistory, ImportPreviousHiZ, AddHiZBuildPass and
GetHiZMipLevelCount, plus visibility-system-owned Hi-Z allocation/eager initialization,
are deleted. Explicit old visibility clients still function while being migrated.
Tests now include their own material/geometry dependencies directly.

The new Vulkan readback regression exposed an existing image-region copy defect:
CopyTextureRegionPlatform used the source box maximum as the destination offset.
It now honors vDestinationPoint. The defect produced invalid out-of-bounds copies
and device loss before the fix; the validated native pyramid test now passes. No
D3D12 implementation is modified.

Verification:

- All four Debug test targets and dependent engine/editor targets plus GpuDrivenScene
  build (`Workspace/native-depth-history-build-complete.log`); subsequent focused
  builds compile the Vulkan copy fix and final readback regression.
- All 49 GraphicsCore CPU subtests pass (`Workspace/native-depth-history-core-final.log`).
  A first version of the new CPU test incorrectly reused an appended pin collection;
  that test was corrected and the full suite passes. Initial compile failures from
  const pointer binding, internal helper linkage and removed transitive test includes
  were also corrected before final validation.
- All nine NativeAssetAuthoring subtests pass, including temporal/optional interface
  inference on the new ordinary DepthPyramid resource
  (`Workspace/native-depth-history-author.log`). All fifteen Qt EditorDocuments
  subtests pass, deterministically exporting/re-exporting all fifteen SDK assets
  (`Workspace/native-depth-history-qt.log`).
- Vulkan NativeDepthPyramid::AuthoredInterleavedHistory reads every texel of every
  current/previous mip from D32 and R32 inputs. It covers 7 x 5 and 9 x 1 extents,
  odd-edge minima, two independently configured occurrences in each of two
  interleaved execution contexts, graphics/compute queues, skipped execution,
  invalidation, resize and a fourteen-frame submission burst with no per-frame
  readback/device-idle wait (`Workspace/native-depth-history-gpu-passed.log`).
- Vulkan NativeVisibility's four existing authored world/depth/material subtests
  pass (`Workspace/native-depth-history-visibility.log`). PipelineExtraction's
  three subtests pass, preserving a shared sparse 288-byte instance upload once
  across two cameras and zero warmed static upload bytes
  (`Workspace/native-depth-history-PipelineExtraction.log`). Its uncontrolled Debug
  packing sample is 2.471 ms/update; it is not a depth-pyramid benchmark.
- NativeSceneLod (4) and NativeMeshCapture (3) Vulkan subtests pass after deleting the
  unused view contexts (`Workspace/native-depth-history-<group>.log`). An initial
  extraction filter was attempted against the editor test executable, where it
  matches no tests; PipelineExtraction is correctly verified in GraphicsFoundationTest.
- Vulkan Tools passes six subtests, including real copied-pixel readback with distinct
  nonzero source/destination coordinates (`Workspace/native-depth-history-Tools-final.log`).
- Searches confirm the removed view field/preparation method and five manager-owned
  Hi-Z history APIs no longer occur. No rendering Builtin files or active scene
  database types have been introduced.

The original combined xiiGpuHiZPyramid remains in Visibility for GpuDrivenScene and
TemporalHistoryTest consumers. Those must migrate to authored native resources before
its header/implementation and old shader can be deleted. The remaining visibility
service, independent occlusion/streaming feedback, lighting/shadows and full game/editor
pipelines remain unfinished. There is no full application/runtime or D3D12 validation.

The generic history operation adds one explicit full-image GPU copy, including mip
levels, rather than rebuilding depth inside its storage owner. Its bandwidth and
per-mip view-creation costs have not been benchmarked against the combined legacy
path; no frame-performance improvement is claimed. Production code adds no device-idle
wait. The unrelated AmbientOcclusion asset hash edit remains outside these commits.
The overall architecture migration is incomplete.


## Compiled root boundaries and legacy Hi-Z deletion - 2026-10-16

Reusable pipeline resources now lower their root document interfaces to immutable
compiled binding slots. FindInterface resolves a name/direction once per compilation;
frame execution uses cached indices. Root input fan-out shares the caller's supplied
graph version. Required inputs, formats/bind flags, wrong resource kinds, duplicate
inputs, output-as-input indices and stale setup bindings are rejected before algorithm
work. Interface indices belong to their compilation, and frame values are not serialized
identities. Boundary metadata is included in compiled CPU memory accounting.

RenderPipelineInstance::BeginFrame publishes explicit previous-frame resources before
current inputs exist. Repeating it within one setup is idempotent; context/parameter
changes require another setup. Build accepts typed texture/buffer or complete resource
bindings stamped with the existing render-graph setup serial, then executes the normal
compiled operation order. Ordinary view callers still use Build directly. This is frame
resource publication, not a rendering stage selector. Every reflected pass remains an
independent operation governed by its resource dependencies.

Separate runtime root instances also receive private resource namespaces when embedded
in the same graph. Cached namespace strings include the original Core occurrence path
for diagnostics. Their runtime discriminator is never an authored/document/graph handle
identity. Compilation/serialization stays deterministic and shared; no world state or
writable pass state is shared between these runtime roots. Native reflected member pins
and normal editor property/attribute behavior remain unchanged.

GpuDrivenScene now explicitly references the ordinary DepthPyramid resource, configurable
with -depth-pipeline. It resolves interface indices again when synchronization replaces
its compilation, reads PreviousDepthPyramid early, and supplies rendered SceneDepth later.
The authored depth pass controls its own compute-queue property. Resize invalidates its
actual pipeline instance instead of replacing a private Hi-Z ring. The temporal fixture
uses the same resource and boundary API, including independent roots in one graph.

The combined xiiGpuHiZPyramid and xiiGpuHiZPyramidDescription are physically deleted,
along with their header/implementation, old GpuHiZBuild shader and static-link retention.
The small native reduction constant layout now lives in DepthPyramidConstants.h. No
old Hi-Z object, description, API or compatibility wrapper remains in source/assets.
Depth reduction and general texture-history ownership remain separate native operations.

The sample exposed its older D24/forward-Z scene convention when binding the canonical
depth resource. It now uses D32Float, reversed projection, zero depth clear and GreaterEqual
comparison, matching native visibility/history and engine views. Both the small raster-only
and full default Vulkan configurations complete three frames successfully. This does not
mean the sample's remaining C++ visibility/shadow/presentation orchestration is migrated.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest,
  EditorFrameworkTest and GpuDrivenScene build with dependent engine/editor targets
  (`Workspace/native-boundary-build-complete.log`). The last sample-only build also
  verifies graceful selected-resource failure handling.
- All 50 GraphicsCore CPU subtests pass (`Workspace/native-boundary-core-verified.log`).
  CompiledRootBoundaryBindings checks input fan-out, forwarded output identity,
  required/duplicate/unknown inputs, output indices used as inputs, format rejection,
  stale bindings across fresh setups using the same frame number, idempotent begin,
  context/parameter consistency and serialization/recompilation. An initial existing
  output-contract diagnostic test caught an early-return ordering regression; output
  settings are again diagnosed before the missing-setup guard, and the full suite passes.
- Vulkan NativeDepthBoundary passes (`Workspace/native-boundary-NativeDepthBoundary-final.log`).
  The migrated fixture reads every retained mip across interleaved contexts and skipped
  frames, then embeds two standalone instances of the same resource in one graph and
  verifies distinct physical images containing 0.2 and 0.8 depth. NativeDepthPyramid's
  nested/odd-edge/frame-burst regression also passes (`Workspace/native-boundary-pyramid.log`).
- All fifteen Qt EditorDocuments subtests pass, including deterministic export of
  the fifteen SDK resources (`Workspace/native-boundary-qt.log`).
- Vulkan NativeVisibility (4), PipelineExtraction (3), NativePreparation (1),
  NativeMeshCapture (3) and SkinningRaster (3) pass (`Workspace/native-boundary-<group>.log`
  or corresponding -final logs). Sparse world upload remains one changed 288-byte
  instance once across cameras and zero warmed static upload bytes. The uncontrolled
  Debug packing sample is 2.414 ms/update; it is not a controlled performance comparison.
- GpuDrivenScene -renderer Vulkan -frames 3 -grid-width 2 -grid-height 2 -ray-tracing false
  exits 0 (`Workspace/native-boundary-sample-final.log`). The default GpuDrivenScene
  -frames 3 also exits 0 with 384 world-derived TLAS instances, 42 registered/41 compiled
  logical operations, one queue submission and 147 barriers
  (`Workspace/native-boundary-sample-default-verified.log`). No errors/warnings or device
  loss are reported in these successful runs. Frame completion is verified; a pixel
  correctness assertion or long-duration application benchmark is not claimed.
- An explicitly missing -depth-pipeline resource exits 1 with its path and a clear error,
  without selecting a fallback or raising an assertion
  (`Workspace/native-boundary-sample-missing.log`). The initial sample run rejected its
  old D24 input; the canonical D32/reversed-Z migration above resolves that failure.
- Source/asset searches find no old Hi-Z class/description/header/shader/constants
  references, compatibility wrappers or rendering Builtin files.

The inventory stays at 54 native operation classes; this phase adds reusable runtime
boundary support and removes a subsystem rather than inflating that count. There is
no controlled depth/history bandwidth benchmark and D3D12 remains excluded. The sample
still contains legacy C++ visibility/shadow/presentation orchestration; GraphicsCore's
remaining visibility, lighting and shadow services, camera motion history and complete
forward/deferred/editor assets remain migration targets. TemporalHistory's older full
pipeline test also awaits those rebuilt assets; the full editor-engine suite is not
claimed to pass. The overall architecture migration is incomplete.

No AmbientOcclusion source change is included in this phase. Its initially observed
unrelated hash edit no longer appears in the current worktree; this implementation does
not intentionally edit or stage that asset.


## Independent instance occlusion - 2026-10-17

InstanceOcclusionPass is the 55th concrete reflected operation. Its eight typed
member pins consume canonical instance/view records, a candidate list/count, a
separately initialized counter and an explicitly matching reversed-Z depth image.
It publishes a compact instance list/count. DepthBias and AsyncCompute are reflected
member properties; no stage selector, pin-type table, scene owner or implicit history
is introduced. The ordinary InstanceOcclusion resource contains this operation and
CounterReset, with five root inputs and two outputs inferred from native RTTI. The SDK
library now contains sixteen independently authored source/binary resources.

The shader projects all eight world bounds corners, conservatively keeps moving,
posed, always-visible, invalid or clip-crossing candidates, bounds candidate reads
and output reservations, and coarsens until four samples cover the entire projected
texel rectangle. An incomplete mip chain retains candidates whose full footprint
cannot be tested. This prevents a floor-log footprint selection from skipping an
interior depth hole. A temporal consumer must provide matching projection/image
inputs and invalidate stale occluders; this operation does not invent a history
validity policy or implicitly enable temporal occlusion in existing consumers.

The unused GpuShadowRasterManager::AddLocalLightPasses orchestration API and its
implementation are deleted, along with its header dependencies. The remaining shadow
raster entry point still has real sample/virtual-shadow consumers and requires native
replacement. Legacy instance/meshlet visibility orchestration is also still a migration
target; adding the native operation does not count those consumers as converted.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and GpuDrivenScene
  build, including dependent editor plugins/processes. Initial fixture compilation
  errors (ambiguous string conversion and plain enum access) were corrected before
  successful final builds (`Workspace/native-occlusion-build-final.log`,
  `native-occlusion-editor-build.log`, `native-occlusion-library-build.log`).
- All 51 GraphicsCore CPU subtests pass (`Workspace/native-occlusion-core-all.log`).
  NativePreparation verifies reflected configuration, invalid/nonfinite bias rejection,
  required typed inputs and nested occurrences with independent property overrides.
- Vulkan NativeOcclusion passes (`Workspace/native-occlusion-gpu.log`). Six scenarios
  read actual GPU lists/counts from two instances sharing an authored definition:
  solid depth, an interior hole, insufficient mips, empty input, invalid projection,
  and a shorter explicit count. Oversized counts are bounded to a 67-entry candidate
  allocation; invalid indices, disabled records, duplicates, equal-depth/bias behavior,
  movement, skinning, near-plane crossing and outside-image bounds are exercised.
  Writable graph handles are distinct between the two instances. Expected index
  multisets are specified independently of the shader's projection/mip calculations.
- All ten NativeAssetAuthoring fixtures pass; all fifteen Qt EditorDocuments tests
  pass, including source/binary byte equality and repeated deterministic export of all
  sixteen resources (`Workspace/native-occlusion-authoring.log`, `native-occlusion-qt.log`).
- Vulkan NativeVisibility's four subtests pass. GpuDrivenScene -renderer Vulkan -frames 3
  exits 0 with 384 world-derived TLAS instances, 41 compiled logical operations and one
  queue submission (`Workspace/native-occlusion-visibility.log`, `native-occlusion-sample.log`).
  This sample validates the shadow API deletion; it still uses its older visibility
  orchestration and is not evidence of production adoption of the new occlusion asset.
- Searches find no AddLocalLightPasses references in source/assets and no rendering
  Builtin source files. No compatibility adapter or second scene lifecycle is added.

No controlled GPU timing, bandwidth comparison or long-duration sample run was made.
The algorithm favors conservative visibility when supplied data is uncertain. D3D12
remains excluded. Meshlet occlusion, streaming feedback, legacy service deletion and
complete lighting/shadow/editor/game graph rebuilding remain incomplete.


## Independent meshlet occlusion and native visibility separation - 2026-10-17

MeshletOcclusionPass is the 56th concrete reflected operation, in Visibility. It
filters supplied absolute (instance, meshlet) pairs without owning frustum/LOD
selection, residency, temporal history or draw preparation. Nine typed member pins
include explicit scene/view/meshlet metadata, candidate pairs/count, independently
initialized output count and matching reversed-Z depth. DepthBias and AsyncCompute
remain reflected member properties. Output capacity derives from the candidate
allocation, so the filter cannot emit more pairs than its bounded input stream.

The ordinary MeshletOcclusion asset contains the filter and CounterReset, with six
root inputs and two outputs derived from native RTTI. The library now has seventeen
source/binary assets. Multiple instances reference one definition and configure their
own bias/queue properties. No Builtin file, stage check, manual pin-type table,
compatibility adapter or global scene/resource lookup is introduced.

Instance and meshlet occlusion share only a conservative depth projection/coverage
shader helper and a neutral DepthOcclusionConstants layout. The instance-specific
constants include is removed. Meshlet spheres use a Gram row-sum stretch bound,
covering nonuniform scale, reflection and shear. The depth projection also keeps
unknown coverage, posed/moving/always-visible candidates and insufficient mip chains.
A graph supplying temporal depth remains responsible for matching projection/image
and invalidating stale occluders. Production temporal occlusion is not implicitly
wired by these operations.

The canonical immutable xiiGpuVisibilityView record and unchanged version-3 reflection
move out of GpuVisibilitySystem into GpuVisibilityTypes, with ABI size/offset checks.
VisibilityViewPass now captures the existing view/camera/frustum directly. Every
native pass and both native visibility GPU fixtures are free of the legacy visibility
system header/API. An unused private compatibility AddPasses overload is deleted.
The older visibility service still has real unmigrated consumers and is not claimed
as removed; these type/snapshot changes prepare its deletion without a wrapper.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build, including dependent
  engine/editor plugins/processes (`Workspace/native-meshlet-occlusion-consumer-build.log`,
  `native-meshlet-occlusion-editor-engine-build.log`). An explicit frustum include and
  fixture prototype declarations were corrected after initial compile failures.
- All 52 GraphicsCore CPU subtests pass (`Workspace/native-meshlet-occlusion-cpu.log`).
  Meshlet configuration tests reject nonfinite/out-of-range bias and compile two
  independently overridden nested occurrences using RTTI-derived required boundaries.
- Vulkan NativeOcclusion's two subtests pass (`Workspace/native-meshlet-occlusion-gpu.log`).
  The instance scenarios still pass after sharing the shader calculation. Nine meshlet
  scenarios read lists/counts from biased and strict occurrences of one authored graph:
  solid/hole/incomplete pyramids, empty/oversized/truncated candidate counts, invalid
  projection, nonuniform scaling, shear and reflected scaling. Invalid scene/meshlet
  indices are skipped, disabled candidates are excluded, duplicates preserved, and
  absolute metadata indices offset by sixteen survive compaction exactly. Bias,
  movement, skinning, always-visible, equal-depth and clip/image crossings are checked.
- All eleven native authoring fixtures and all fifteen Qt EditorDocuments subtests pass;
  deterministic exports match packaged binaries for all seventeen resources
  (`Workspace/native-meshlet-occlusion-authoring.log`, `native-meshlet-occlusion-qt.log`).
- Vulkan NativeVisibility (4), PipelineExtraction (3) and NativeMeshCapture (3) pass
  (`Workspace/native-meshlet-occlusion-visibility.log`, `native-meshlet-occlusion-world.log`,
  `native-meshlet-occlusion-capture.log`). World extraction still uploads one changed
  288-byte record once across cameras and zero bytes for a warmed static frame.
  Uncontrolled Debug packing measured 2.522 ms/update for 1024 unchanged components;
  this is not a controlled comparison or GPU bandwidth benchmark.
- Source searches find no legacy visibility system/manager dependencies in native pass
  sources, no old InstanceOcclusionConstants includes, and no rendering Builtin files.

D3D12 remains excluded. Full renderer/editor-engine suites are not claimed to pass.
Remaining work includes independent streaming feedback, old visibility/shadow/lighting
orchestration deletion, production graph wiring with deliberate temporal validity,
complete editor/game/lighting/shadow assets and controlled large-scene GPU benchmarks.


## Native geometry LOD streaming feedback - 2026-10-17

Three coherent operations in Streaming bring the inventory to 59: table initialization,
missing-LOD demand generation and completion-checked asynchronous readback. The ordinary
GeometryLodFeedback resource wires those three nodes and exposes four RTTI-derived query
inputs plus its request table output. The SDK now contains eighteen source/binary assets.
All pins are typed reflected members. Capacity, PriorityTier, PurposeMask, AsyncCompute
and MaxPendingReadbacks are reflected properties. These operations declare no automatic
world extraction demand or rendering feature initialization.

Demand is generated from query candidates before resident draw selection. It applies
supplied masks/flags/frustum and geometry-level LOD ownership, and can request a forced
LOD even when no draw is currently resident. A bounded GPU hash table merges each full
geometry identity toward the finest requested LOD and strongest priority. Its key retains
the existing 24 index bits and 8 generation bits, with compile-time ABI checks; unlike the
older feedback implementation it does not truncate geometry indices to twenty bits.
Requests, priorities and purpose masks are frame-derived values, never serialized object
identities. Duplicate candidates collapse without creating another geometry registry.
The table reports raw, unique and overflow counts, including bounded-probe exhaustion.

Readback slots contain staging buffers and fences only. They belong to each pass occurrence,
are bounded by an authored limit, and are reused after frame completion and consumption.
CPU mapping uses DoNotWait after the copy fence completes. Backpressure is reported as
dropped captures; demand can be regenerated in a later frame. A failed recording whose
unsignaled copy has retired is discarded rather than permanently occupying its slot.
Execute captures retain their slot ownership independently of the runtime pass pointer.
Decoded requests are validated against the existing geometry residency handles and LODs;
retiring/stale records are rejected. Accepted requests call RequestResidency. Actual asset
loading/allocation remains in the existing device streaming tick, with no copied resource
manager, scene registry or component storage.

Verification:

- Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and
  EditorEngineProcessFrameworkTest, with dependent engine/editor plugins/processes
  (`Workspace/native-feedback-verified-build.log`, `native-feedback-final-gpu-build.log`,
  `native-feedback-dependent-build.log`). The GPU fixture's Core provenance field name
  was corrected after an initial compile error.
- All 53 GraphicsCore CPU subtests pass (`Workspace/native-feedback-cpu-final.log`).
  Tests cover independent reflected contracts, invalid table/staging/priority properties,
  full high-index/high-generation key decoding, nested overrides, serialization and compilation.
- Vulkan NativeGeometryFeedback passes (`Workspace/native-feedback-final-gpu.log`). Two
  graph instances in each of two runtime view contexts verify request content,
  distinct writable tables, different capacities/priorities/purposes, duplicate merging,
  empty and oversized query counts, forced missing/resident/invalid LODs, section-owner
  selection, flags/masks/frustum rejection and zero-resident geometry. A two-entry table
  reports overflow while an eight-entry table retains all four geometry identities.
  Generation-stale captures are rejected; valid asynchronous captures cause real fine-LOD
  residency through the existing service. An abandoned compiled graph never publishes a
  readback. A retired geometry's later feedback is rejected while its newly registered
  replacement remains at its explicitly requested coarse LOD. Immediate index reuse is
  not assumed: the existing ID table uses its own free-slot order.
- Initial GPU fixture failures used an unaligned canonical-record array and omitted the
  device streaming tick when executing a graph directly. The fixture now uses the aligned
  allocator, valid meshlet source data, explicit initial coarse residency requests and the
  existing streaming tick. Incorrect assumptions about immediate ID reuse and automatic
  coarse requests were removed; the final test above passes without compatibility code.
- All twelve native authoring fixtures and all fifteen Qt EditorDocuments tests pass;
  deterministic source/repeated/binary export matches all eighteen packaged resources
  (`Workspace/native-feedback-authoring.log`, `native-feedback-qt.log`).
- Vulkan PipelineExtraction's three subtests pass (`Workspace/native-feedback-world.log`),
  including specialized demand and shared sparse uploads. No rendering Builtin files or
  legacy visibility system dependencies occur in the new operations.

No controlled GPU timing or large-scene collision/readback-bandwidth benchmark was run.
The test waits for device completion to inspect results, so it does not establish sustained
queue overlap performance. Failed-recording retirement and staging backpressure are
implemented but not covered by a deliberate GPU submission-failure/overload injection.
D3D12 is excluded. The old visibility service's embedded feedback remains only with its
unmigrated consumers; these new nodes are not a claim that the service has been deleted.
Next work remains production resource graph wiring, legacy orchestration deletion and
complete lighting/shadow/editor/game graphs. The overall migration is incomplete.


## Explicit depth ownership and reusable shadow depth - 2026-10-17

Three operations bring the inventory to 62: MatrixRasterConstantsPass, DepthTargetPass
and DepthRegionClearPass. The existing DepthRasterPass is reused for shadow drawing;
there is no duplicated shadow raster backend or wrapper around the legacy manager.
DepthRaster version 2 consumes a required supplied D32 target, returns its written
version, and exposes an optional atlas viewport as a reflected member. Its implicit
image allocation, whole-image clear and live-view requirement are removed. Both mesh
and vertex-pull paths use viewport/scissor bounds. Immutable shader/signature/state
references are captured during setup rather than rediscovered during execution.

DepthTarget creates and initializes one graph-owned image. DepthRegionClear clears a
scissored supplied region to a reflected depth value, preserving neighboring texels.
Its immutable parameter buffer is private to the occurrence and retained by graph
imports; changes replace that allocation safely. MatrixRasterConstants captures authored
current/previous matrices and layout/dimensions into the canonical raster ABI without
a camera/world pointer. Shared geometry/material table ownership remains with the
existing device services. No scene objects, transform hierarchy or temporal state are
owned by these operations.

The ordinary ShadowDepth asset contains region clear, matrix constants and the same
DepthRaster operation, with ten external inputs and one depth output inferred from
native RTTI. It consumes prepared visibility/draw resources and does not perform light
selection, visibility orchestration or atlas residency. Two occurrences can draw into
explicitly chained regions of the same atlas with independently overridden matrices,
viewports and clear values. No render-stage checks, pin-type arrays or Builtin file is
introduced. The SDK has nineteen independently authored source/binary resources.

DepthOnly and GBuffer source/binary assets are rebuilt with explicit DepthTarget nodes;
their persistent document identities remain unchanged. Their flat occurrence counts
are now eighteen and nineteen. The native direct raster fixture also supplies a target
explicitly. Old DepthRaster authoring without this required connection fails validation;
there is no compatibility fallback or hidden target construction.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and
  EditorEngineProcessFrameworkTest build with their dependent engine/editor targets
  (`Workspace/native-shadow-final-build.log`, `native-shadow-queue-build.log`,
  `native-shadow-batching-build.log`). Initial fixture string/binding scope compilation
  errors were corrected. A Qt plugin copy was temporarily denied while its authoring
  process was active; the build passed after that process finished and released it.
- All 54 GraphicsCore CPU subtests pass (`Workspace/native-shadow-cpu-final.log`),
  including reflected typed inputs, read/write outputs, finite clear/matrix properties,
  viewport configuration and an actual out-of-bounds supplied-target failure diagnostic.
- Vulkan NativeDepthAtlas passes (`Workspace/native-shadow-batching-gpu.log`). Real cube
  geometry is rasterized through two instances of one ShadowDepth definition and two
  runtime contexts, with no live camera or world. Readback checks 0.7 reversed-Z surface
  depth, distinct projection coverage, independent 0.0/0.1 clear values, region corners
  and untouched 0.9 neighboring/background texels across six device frames.
- The initial region-clear implementation mapped a dynamic uniform inside an active
  Vulkan render pass, which validation rejected. Parameters are now immutable and
  prepared before execution. The fixture's initial forward-Z matrix was also replaced
  by an explicit reversed-Z orthographic matrix. Final validation/readback above passes.
- The warmed test compiles sixteen logical operations. Async scheduling produces six
  queue submissions and 26 barriers; disabling async queues through the existing compiler
  setting batches the same graph into one submission with the same 26 barriers and
  identical readback. Choosing graphics preparation for the small matrix uniform alone
  did not reduce the async submission count. These are graph scheduling counts, excluding
  external readback and one-time asset initialization; no GPU timing speedup is claimed.
- Vulkan NativeVisibility (4), NativeMeshCapture (3), SkinningRaster (3) and NativeSceneLod
  (3) pass (`Workspace/native-shadow-<group>.log`). All thirteen native authoring fixtures
  and all fifteen Qt EditorDocuments tests pass, including deterministic source/repeated/
  packaged binary equality for all nineteen resources (`native-shadow-authoring.log`,
  `native-shadow-qt.log`).
- Source searches find no rendering Builtin file and no legacy shadow/visibility system
  dependency in the new operations. No permanent compatibility adapter is added.

D3D12 remains excluded. Large-scene shadow timing, cascade/light-driven projection
preparation, atlas allocation/residency, virtual-page scheduling and production shadow
consumer migration still need work. GpuShadowRasterManager, virtual-shadow orchestration
and the sample's old rendering paths are not claimed to be deleted or migrated by this
phase. Full game/editor lighting graphs and the complete migration remain incomplete.


## Authored world visibility consumer migration

The library now contains 63 native reflected operations and 22 ordinary source/binary
assets. MatrixInstanceVisibility (7 expanded operations), MatrixMeshletVisibility (13)
and WorldMeshDraws (17) are independently authored resources with persistent document
identities. WorldMeshDraws combines nested visibility, native mesh command preparation
and the existing three-operation LOD feedback subgraph. Source export and repeated binary
compilation are deterministic in the editor resource tests.

MatrixVisibilityView owns reflected matrix, position, dimensions, flags and LOD settings
and concrete member pins. Typed instance parameters can supply changing query inputs
without mutating authored defaults or recompiling the definition. It snapshots the
existing world's instance count and component selection during setup under world read
access; execution reads immutable graph data. Each occurrence retains its own published
LOD token. StreamingPriorityTier similarly updates request urgency without replacing the
compiled graph. No rendering stage selector or manually enumerated pin-type schema is
introduced.

GpuDrivenScene no longer creates, destroys or submits work through a legacy visibility
context. Main, sensor and shadow queries use three runtime instances referencing the
selected WorldMeshDraws resource, with different required component flags and priority.
The sample resolves typed compiled outputs during setup and uses reflected native mesh
command strides. Its existing conservative GetMaximumMeshletCount bound rejects a graph
budget that cannot cover the world. Its existing world/module-owned view is disabled for
automatic execution because the registered sample graph owns frame submission. Native
Build calls hold the world's read marker. The redundant sample AsyncCompute setting and
cached CPU frustum were removed; individual reflected passes own scheduling preferences.

Repeated device geometry/material upload requests now return the same graph-local buffer
versions for a setup serial. Caches contain only handles, clear on a new device frame and
shutdown, and do not retain scene data or cross-graph handles. This prevents repeated
subgraph instances from overwriting shared device tables between consumers, which the
compiler correctly rejects as a physical-resource dependency cycle. Existing world
instance uploads already deduplicate similarly. RenderWorldModule skips execution when
no prepared enabled view exists, avoiding nested frame ownership for disabled query views.

Verification completed before the approval service became unavailable:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest, GpuDrivenScene and
  EditorEngineProcessFrameworkTest built, including affected dependencies. Logs:
  native-world-draw-final-build.log and native-shared-residency-final-build.log in Workspace.
- All 55 GraphicsCore CPU tests, 16 NativeAssetAuthoring fixtures and 15 EditorDocuments
  tests passed. The latter checked all 22 packaged resources against deterministic source
  export (native-world-draw-cpu.log, native-world-draw-authoring.log, native-world-draw-qt.log).
- Vulkan PipelineExtraction (3), NativeGeometryFeedback (1) and NativeVisibility (4)
  passed. The extended extraction fixture reads back two distinct matrix-query snapshots
  from one shared compilation, checks independent LOD publication/camera-cut behavior,
  and asserts repeated geometry/material/world uploads return identical graph bindings.
- Two cameras share one changed-instance upload of 288 bytes; warmed unchanged frames
  upload zero bytes. The existing Debug CPU packing fixture measured 48.377 ms for 20
  updates of 1024 unchanged components, or 2.419 ms/update in this run. This is not a
  Release benchmark or a measured speedup (native-shared-residency-extraction.log).
- Actual Vulkan GpuDrivenScene completed small-grid 3-frame, default-grid 3-frame and
  default-grid 30-frame runs. Missing -visibility-pipeline exits 1 with the selected path
  and no fallback (native-world-draw-sample-*-final.log; small run is *-small-shared.log).
- Broad EditorViews ran and failed: 22 assertions across SceneShapeIcons, ViewLifecycle,
  WireframeRendering and WireframeRenderingWithoutMeshShaders. DisabledViewsDoNotRecordFrames
  and existing WorldRenderFrames passed. Failures exposed references to deleted Deferred/
  EditorScene defaults and icon tests using unscoped resource names instead of bindings.
  Source corrections now use explicit native resources, reflected fixture member pins and
  actual wireframe surface identities. Their first build found an incorrect connection
  field name, now corrected to m_sInput. Rebuilding/rerunning these final fixture edits
  was initially blocked because automatic approval review reached its usage limit.
  The service recovered; final verification below supersedes that temporary blocker.

Five incremental implementation/fixture commits were pushed through 080a2ea0b. The
approval service resumed accepting requests, allowing the editor fixture rebuild to run.
Icon readback, explicit pipeline recovery and both native wireframe raster configurations
now execute successfully. The test log facility requires a single expectation with count
two for repeated identical diagnostics; its initial duplicate expectations were corrected.
Final full EditorViews verification passes all 47 subtests on Vulkan, including disabled
view frame ownership, icon rendering/picking, explicit view resource recovery, and real
wireframe coverage/validated world identities with both mesh-shader and vertex-pull
configurations (native-editor-fixtures-complete-build.log,
native-editor-fixtures-complete-views.log). NativeVisibility (4),
NativeGeometryFeedback (1), NativeSceneLod (3) and SkinningRaster (3) also pass after the
shared upload changes (native-shared-residency-visibility/feedback/lod/skinning.log).

At that phase, GpuVisibilityManager/System still had legacy test consumers and
RenderGraphManager hooks; the following phase deletes those implementations. GpuDrivenScene
still contains shadow/VSM, custom raster, ray-dispatch and presentation orchestration.
The frame-local visibility output aggregate remains an input to those consumers; it is
not a new lifecycle facade. Native explicit instance/meshlet occlusion remains available,
but the sample no longer implicitly applies old previous-frame Hi-Z. Wiring temporal
occlusion requires explicit matching projections and invalidation for changed/deleted
occluders, materials and residency. Full lighting/game/editor graphs, removal of remaining
oversized services and further fine-grained decomposition remain incomplete. D3D12 is
excluded. No complete-renderer or full GPU performance claim is made by this phase.


## Legacy visibility subsystem deletion

The legacy visibility execution subsystem is physically removed. Deleted files:

- Visibility/GpuVisibilityManager.h and Implementation/GpuVisibilityManager.cpp.
- Visibility/GpuVisibilitySystem.h and Implementation/GpuVisibilitySystem.cpp.
- Shaders/Visibility/GpuSceneHiZOcclusion.xiiShader, GpuSceneMeshletDispatchBuild.xiiShader,
  GpuSceneMeshletCull.xiiShader, GpuSceneMeshletHiZCull.xiiShader,
  GpuSceneLodFeedbackClear.xiiShader and GpuSceneCommandBuild.xiiShader.
- Shaders/Visibility/GpuSceneMeshletCull.h and GpuMeshletDispatchConstants.h.

This removes the global context ID table, named visibility-set registry, frame-sliced
view/LOD/feedback buffers, shader orchestration, synchronous manager frame maintenance,
old feedback hash implementation, context configuration/purpose reflection and streaming
statistics. RenderGraphManager no longer scans visibility contexts for a minimum ring or
calls their BeginFrame. Static-link references and generated build inputs are removed.
No compatibility wrapper or replacement context registry is added.

Ownership after removal:

| Removed responsibility | Native/current owner |
| --- | --- |
| View and flags snapshot | VisibilityView or MatrixVisibilityView reflected occurrence |
| World instance and skinning data | Existing RenderWorldModule/GpuInstanceData sparse extraction and upload |
| Instance culling and LOD selection | InstanceCull; generation/token-validated BufferHistory belongs to the occurrence |
| Work bounds and meshlet compaction | MeshletDispatchBounds, MeshletDispatch and MeshletCull |
| Depth culling | Explicit InstanceOcclusion/MeshletOcclusion with supplied matching depth and projection |
| Draw commands | MeshDrawCommands or VertexPullDrawCommands with reflected layout |
| Missing LOD requests/readback | Native Streaming operations and existing device geometry residency |
| Queue synchronization and transient allocations | Existing render graph, GAL fences and shared GPU frame owner |

GpuVisibilityResources.h retains only the frame-local xiiGpuVisibilityOutputs aggregate
needed by remaining shadow/emissive/VSM consumers. It contains graph handles and a capacity,
with no object identity registry, scene storage, callbacks, mutation API or lifecycle.
The shader-view ABI remains unchanged in GpuVisibilityTypes. The old GpuLodFeedback shader
record is deleted; native streaming owns its distinct generation-safe request ABI.

SceneLodSelection, SceneLodSections and SceneMeshIdentity now update the existing world
module's GPU extraction and consume the authored MatrixMeshletVisibility definition.
Their test-only fixture changes a reflected meshlet budget and binds concrete native
occlusion/reset/command members using schemas discovered from RTTI. It stores no scene
state and exposes no production API. Manual fixture frame numbers/private trackers were
replaced by the actual shared xiiRenderGraphFrame timeline required by BufferHistory.
Viewport ring coverage now supplies an explicit two-frame limit to that same frame API.
Obsolete assertions about deleted manager/handle construction were removed; mesh command
property bounds are checked on the actual native operation.

The migrated multi-section fixture exposed a native correctness bug: InstanceCull writes
the geometry group's representative ObjectId into each section's LOD entry, while the two
meshlet consumers compared that entry against each individual section ObjectId. The
consumers now resolve and validate the same bounded representative as InstanceCull. This
retains generation safety and shared hysteresis without dropping other sections or
changing authoritative GPU identities. The full material-routing/filtered-section suite
passes on both mesh and vertex-pull raster configurations. No expected results were relaxed.

Verification after physical deletion:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest,
  GpuDrivenScene and EditorFrameworkTest build with engine/editor dependencies, including
  EditorEngineProcess and EditorProcessor. Logs: native-visibility-deletion-final-build.log
  and native-visibility-deletion-editor-build.log. Removing a transitive legacy include
  exposed the raster fixture's missing GeometryResidency include, now explicit.
- All 55 GraphicsCore CPU tests pass (native-visibility-deletion-cpu.log).
- Vulkan NativeVisibility (4), PipelineExtraction (3), NativeGeometryFeedback (1),
  NativeOcclusion (2), NativeDepthAtlas (1), NativeSceneLod (4), SkinningRaster (3),
  NativeMeshCapture (3) and EditorViews (47) pass. Logs use
  Workspace/native-visibility-deletion-<group>.log.
- All 15 Qt EditorDocuments tests pass, including deterministic compilation of all
  22 ordinary source/binary resources and pin/property document behavior
  (native-visibility-deletion-editor-documents.log).
- Actual default Vulkan GpuDrivenScene runs 30 frames, exits 0 and logs no error/assertion
  (native-visibility-deletion-sample.log). No GPU timing speedup or Release benchmark is
  claimed. Removing global context/set maintenance and duplicate shader paths reduces
  overlapping CPU infrastructure; world/GPU upload ownership remains unchanged.
- Searches of Source and Data/Base/Shaders find no removed visibility class, context,
  configuration, purpose, statistics, include or shader path. Regenerated GraphicsCore
  project inputs contain no deleted files. Rendering Builtin searches remain empty.
  git diff --check passes.

The inventory remains 63 native operations and 22 resources: deletion is not counted as
new pass conversion. This phase completes visibility subsystem removal, not the overall
rendering migration. GpuShadowRasterManager, VSM raster orchestration, oversized lighting
services and sample/game/editor full-stack composition still require conversion/removal.
D3D12 remains excluded. Existing explicit temporal occlusion policies still require valid
client depth/projection/invalidation; no implicit unsafe history fallback is restored.


## Resource-driven standalone shadow consumer

The SDK now contains 23 ordinary source/binary pipeline assets and 63 concrete native
operations. ShadowMap is a two-node authored definition: DepthTarget and a reference to
ShadowDepth. The reference contains an ordinary reflected property override selecting
mesh rasterization. Expansion produces four native operations; no new pass wrapper or
raster backend was introduced. Input resource types/formats are inferred from the editor's
reflected PinConfiguration.Metadata objects, and ordinary persistent boundary nodes define
nine GPU buffer inputs and the D32 ShadowDepth output. Source export preserves nested
reference/override identities and is deterministic against the packaged binary.

GpuDrivenScene selects the resource with -shadow-pipeline. Its hardcoded sun-shadow target
allocation and GpuShadowRasterManager call are removed, along with ShadowTargetPassData
and the old raster-description construction. The sample supplies already extracted world
instances, poses, compact visibility, indirect commands and existing device residency
through the compiled resource interface. Runtime raster matrix, dimensions and mesh
command strides are scoped to its own pipeline instance. Missing or incompatible resources
fail explicitly, identify the selected asset and do not choose another pipeline.

MatrixRasterConstants, DepthTarget, DepthRegionClear and DepthRaster accept explicitly typed
occurrence parameters for changing raster inputs while retaining their reflected member
defaults. A small inline ReadParameter utility checks the existing variant dictionary;
it owns no configuration, registry, state or graph construction. Matrix/size/stride/bounds
validation remains with each operation. The compiler's normal resource descriptions and
lifetime cache handle an actual target size change; the immutable definition is reused.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and GpuDrivenScene
  build with dependent engine/editor targets (native-shadow-consumer-build.log,
  native-shadow-query-dual-build.log, native-shadow-parameter-cpu-build.log and
  native-shadow-pin-metadata-build.log).
- All 55 GraphicsCore CPU tests pass. Extended depth contracts reject wrong runtime types
  and malformed rectangles, accept explicit runtime target dimensions and check that the
  authored Width remains zero (native-shadow-parameter-cpu.log).
- All 17 NativeAssetAuthoring fixtures pass. The new document authors the nested property
  override through ordinary object commands. The first attempt to read ResourceType from
  the pin configuration itself failed; the corrected fixture reads its existing nested
  Metadata object (native-shadow-pin-metadata-authoring.log).
- All 15 Qt EditorDocuments tests pass, including deterministic source/binary equality
  for all 23 resources (native-shadow-map-final-editor.log).
- Vulkan NativeDepthAtlas now runs both mesh-shader and explicitly mesh-disabled vertex-pull
  paths. It reads actual cube depth through two instances of one definition and two runtime
  contexts across six frames per path. Later runtime parameters change both region projection
  and viewport and resize the target from 32x16 to 48x16. Readback verifies new coverage,
  distinct clear values, untouched neighboring pixels, unchanged reflected Width and the
  same compiled definition (native-shadow-query-dual-atlas.log).
- The mesh fixture compiles 15 logical operations; vertex-pull compiles 16. Each uses six
  async submissions or one submission with async queues disabled, with 25/26 barriers
  respectively and identical depth semantics. These are scheduling counts, not GPU timings.
- Actual Vulkan sample runs complete for a small-grid three-frame case and the default
  thirty-frame case, with no error/assertion. Final packaged asset verification also ran
  thirty frames successfully (native-shadow-map-sample-small.log,
  native-shadow-map-final-sample.log). Missing and incompatible selected assets exit 1
  with precise path/interface diagnostics (native-shadow-map-sample-missing.log and
  native-shadow-map-sample-incompatible.log).
- The default sample moves from 66 registered/59 compiled logical operations to 67/60,
  with seven culled operations in both runs. It retains one submission and 165 barriers,
  with no split barriers. The added independent clear operation does not add a submission
  in this configuration. No Release CPU or GPU timing improvement is claimed.
- Source searches find no old standalone sample shadow call/target payload and no rendering
  Builtin file. git diff --check passes. D3D12 remains excluded.

GpuShadowRasterManager still serves virtual-page rasterization through
VirtualShadowMap.cpp: that consumer and backend have not been deleted in this phase.
Virtual-shadow residency/planning must be separated from raster resource execution before
that deletion. The sample's cascade/VSM preparation, main shading and presentation also
remain C++ orchestration; full native game/editor/light composition is still incomplete.
This resource conversion is a concrete consumer migration, not a claim of full renderer
completion.


## Virtual shadow raster backend removed

The source library remains **63 native reflected operations**; the SDK now contains
**24 ordinary source/binary pipeline assets**. ShadowMeshDepth references the existing
three-operation ShadowDepth definition with a reflected mesh-raster override and an
external D32 target. Its interface is authored from reflected pin metadata, not a pin-type
table. The new asset is composition, not another raster class or a built-in pipeline.

Physically deleted:

- GraphicsCore/Lighting/GpuShadowRaster.h and Implementation/GpuShadowRaster.cpp,
  including xiiGpuShadowRasterManager and its legacy raster description/backend.
- Shaders/Pipeline/GpuShadowDepth.xiiShader and GpuShadowPageClear.xiiShader.
- Shaders/Pipeline/Passes/LocalShadow/GpuShadowRasterConstants.h.
- Their static-link anchor, lifecycle test expectations, includes and consumer APIs.

The ownership split is now explicit:

| Responsibility | Owner after this change |
| --- | --- |
| Scene objects, hierarchy, transforms, bounds, meshes, materials and animation | Existing world objects/components and RenderWorldModule extraction |
| Physical atlas allocation, feedback, page lookup and eviction | Existing VirtualShadowMap residency service |
| Bounded page selection, content revisions and completion retirement | Residency service; no raster algorithm or shader backend |
| Region clear, raster constants and depth drawing | Independent reflected DepthRegionClear, MatrixRasterConstants and DepthRaster operations |
| Reusable algorithm configuration and interfaces | ShadowMeshDepth asset referencing ShadowDepth |
| Mutable pass caches and temporal state | Separate client pipeline instance per physical page; content reuse invalidates history |
| Transient allocations, dependencies, barriers and submissions | Existing render graph |
| Mesh/material storage and immutable asset residency | Existing shared device/resource facilities |

GatherRasterPages ranks a deterministic bounded batch and snapshots projection, viewport,
logical page, physical slot, device frame and content revision. It touches GPU-use age and
claims work once per frame. Invalidation increments the revision even for already dirty
content. Selection never marks a page complete. Abandoned setup remains retryable.
Completion validates revisions again while recording, patches only current GPU records,
and signals a fence after the final atlas dependency. CPU dirty state retires only after
that fence completes and the revision still matches. Reconfiguration and recycled slots
cannot accept an old token. These are transient validation tokens, not serialized graph
or world identities. No scene registry, transform propagation or component mirror is added.

GpuDrivenScene selects -shadow-page-pipeline explicitly and binds compiled resource
interfaces. It caches the shared immutable compilation and creates private runtime
instances lazily for selected physical pages. Missing resources and incompatible D32
interfaces fail with the selected resource path; there is no legacy raster fallback.

Two integration defects surfaced during real-demand verification:

- The sample appended LODs after the mesh descriptor's existing first LOD, leaving an
  empty third LOD that visibility selected. It now edits the authored first LOD through
  the descriptor's mutable GetLODs overload and appends only the coarse LOD. A CPU test
  verifies both nonempty meshlet ranges and serialized thresholds. This does not change
  the immutable runtime mesh resource interface.
- The alias allocator rejected newly exported resources as reuse candidates but still
  allowed a later temporary to reuse a previously exported slot. Export slots now stay
  distinct beyond their last internal use. A GPU test verifies preserved exported bytes
  while two later ordinary temporaries still alias each other. Native visibility now
  alternates async and single-queue compilation in its repeated-instance fixture.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with engine/editor dependencies
  after the five source/shader deletions (native-virtual-page-final-build.log).
- All 57 GraphicsCore CPU tests pass, including revision-safe planning/recycling and the
  two-LOD authoring/serialization regression (native-virtual-page-final-cpu.log).
- All 18 NativeAssetAuthoring fixtures pass (native-virtual-page-authoring.log). All 15 Qt
  EditorDocuments tests pass, including deterministic source/binary equality for all 24
  packaged resources (native-virtual-page-final-editor.log).
- Vulkan RenderGraphAllocations passes with actual exported buffer readback
  (native-virtual-page-export-lifetime.log).
- Vulkan NativeVirtualShadowPages passes with actual cube depth and hashed GPU table
  readback. It covers two instances sharing one definition, distinct projections/pages,
  stale publication, abandoned setup retry, invalidation after recording, fence completion
  and physical-slot reuse (native-virtual-page-final-gpu.log).
- That fixture executes 17 logical operations / 6 submissions / 5 render-pass groups for
  its async phase, and 13/1/2 and 16/1/4 for later single-queue phases. Compatible work
  remains grouped without changing the resource graph's semantics. These are scheduling
  counts, not GPU-time measurements.
- Vulkan NativeVisibility (4), NativeDepthAtlas (2), PipelineExtraction (3),
  NativeGeometryFeedback (1), NativeSceneLod (4), SkinningRaster (3), NativeMeshCapture (3)
  and EditorViews (47) pass. Logs use native-virtual-page-regression-<group>.log. The first
  invocation of the LOD/skinning/capture filters used GraphicsFoundationTest and matched
  no tests; they were rerun successfully in their owning EditorEngineProcessFrameworkTest.
- Missing and incompatible selected virtual-page resources exit 1 with precise path and
  interface diagnostics (native-virtual-page-missing-resource.log and
  native-virtual-page-incompatible-resource.log).
- Source/shader searches contain no deleted shadow raster type, shader path or old
  AddRasterPasses API. Regenerated GraphicsCore project inputs omit all deleted files.
  Rendering Builtin searches remain empty and git diff --check passes.

**Full sample runtime is not passing.** After correcting the empty LOD, real screen depth
produces a 16-page batch in frame 4. The small-grid sample records 117 registered / 109
compiled operations in one submission with 182 barriers, then later loses the Vulkan
device. The default sample also fails. Disabling optional ray tracing still produces a
16-page batch (106 compiled operations, one submission, 177 barriers) and subsequently
fails with ErrorDeviceLost in CpuWaitOnlyFenceVulkan.cpp:123 (fence 30, request 31).
The bounded no-ray-tracing process was stopped after 30 seconds at that assertion.
Logs: native-virtual-page-clean-small-sample.log, native-virtual-page-lod-diagnostic.log,
native-virtual-page-no-rt-sample.log and native-virtual-page-assertion.txt. Earlier smoke
runs rendered no visible geometry because of the empty LOD and cannot be used as evidence
that the complete sample renderer worked. The dedicated native GPU regressions pass,
but do not resolve this application-level failure. No Release CPU/GPU timing improvement
or extraction/upload benchmark is claimed for this phase.

This phase completes deletion of the legacy shadow raster backend. It does not complete
the renderer rebuild: VirtualShadowMap.cpp still constructs feedback/upload/publication
operations, the sample still owns cascade preparation, main shading and presentation in
C++, and full game/editor/light composition remains unfinished. Virtual-shadow light/page
ownership also still needs explicit world/view scoping beyond this single-world client.
The Vulkan application failure must be diagnosed against actual visible geometry.
D3D12 remains excluded. No compatibility backend is restored to hide these gaps.

## Shared shadow feedback backend removed

Current inventory is **66 native reflected operations / 25 ordinary SDK assets**.
VirtualShadowFeedback is an independently authored three-node definition with explicit
SceneDepth and CascadeConstants inputs and Requests output. Reset, generation and
readback each use concrete reflected pin members and ordinary reflected settings.
No rendering Builtin file, stage selector or pin-type configuration table is added.

Deleted from VirtualShadowMap.cpp: AddFeedbackPasses, shared append storage, the
shared staging/fence ring, feedback pipeline loading, clear-frame bookkeeping and
BeginFrame readback orchestration. The old Shaders/Pipeline/VirtualShadowFeedback
path is gone; its algorithm now belongs to Shaders/Shadows/VirtualShadowFeedback.
The retained 16-byte GPU request ABI lives beside the native passes. Physical page
residency, lookup, eviction and bounded allocation remain in the existing device
service; no world objects, transforms, hierarchy or second scene registry are added.

The allocation budget is now shared across feedback consumers in one device frame.
A private readback occurrence owns bounded staging slots and completion fences.
It consumes immutable captures only after completion, retries abandoned recordings,
retains in-flight allocations, and discards captures from an old configuration epoch.
That epoch is transient validation state, not a persistent document or world identity.
The generator snapshots occurrence parameters without changing reflected defaults.
Zero active cascades produce no requests; cascade count and projection validity are
checked on the GPU. These passes declare no scene extraction or GPU scene features.

GpuDrivenScene selects -shadow-feedback-pipeline, validates its compiled input
interface and supplies explicit resource bindings. It loads no shader and constructs
no feedback algorithm itself. Missing or incompatible resources fail with their path.
Independent view/client instances share immutable compilation, not writable streams,
staging buffers, completion counters or reflected settings. Page identity still needs
world/view scoping beyond the current single-world sample; private feedback storage
alone does not complete that remaining ownership change.

The real GPU fixture exposed a Vulkan fence-domain defect: AddPendingSyncPoint
validated a queue submission counter against private caller signal values. It now
validates the caller value under the existing synchronization guard. A dedicated
real-queue regression uses private values 10000..10002 across graphics and transfer
submissions. D3D12 is untouched.

Verification for this phase:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies after
  backend deletion (native-shadow-feedback-deletion-build.log and
  native-shadow-feedback-editor-engine-build.log).
- All 59 GraphicsCore CPU tests pass, including nested reusable feedback definitions,
  separate overrides, shared per-frame allocation budget and configuration epochs
  (native-shadow-feedback-deletion-cpu.log).
- All 19 NativeAssetAuthoring fixtures and 15 EditorDocuments tests pass, including
  deterministic source/binary equality for all 25 resources
  (native-shadow-feedback-authoring.log and native-shadow-feedback-deletion-editor.log).
- Vulkan NativeShadowFeedback passes actual GPU request readback for two views and
  two instances per view, overflow, independent projections, configuration changes,
  abandoned setup and zero active cascades (native-shadow-feedback-final-gpu.log).
  Nine logical operations use one submission / 15 barriers in the single-queue phase
  and seven submissions / 15 barriers with async scheduling. These are scheduling
  counts, not timing measurements.
- Vulkan FenceSynchronization.IndependentCallerAndQueueValues, NativeGeometryFeedback
  and NativeVirtualShadowPages pass (native-shadow-feedback-fence.log,
  native-shadow-feedback-fence-geometry.log and native-shadow-feedback-deletion-pages.log).
- Vulkan NativeVisibility (4), NativeDepthAtlas (2), RenderGraphAllocations (1),
  PipelineExtraction (3), NativeSceneLod (4), SkinningRaster (3), NativeMeshCapture (3)
  and EditorViews (47) pass (native-shadow-feedback-regression-<group>.log).
- Missing and incompatible selected feedback assets exit 1 with explicit diagnostics
  (native-shadow-feedback-missing.log and native-shadow-feedback-incompatible.log).
- Searches find no old AddFeedbackPasses API, shared feedback ring or old shader path
  in production sources/shaders. Rendering Builtin file searches remain empty.

**The full Vulkan sample still fails.** Native feedback produces a real 16-page
batch in device frame 3. The sample records 118 registered / 110 compiled operations,
one submission and 186 barriers, then loses the device at CpuWaitOnlyFenceVulkan.cpp:123
(fence 46, request 47). The bounded process was stopped after 30 seconds at the
assertion (native-shadow-feedback-sample.log and native-shadow-feedback-assertion.txt).
Focused GPU tests do not establish application correctness or resolve this failure.
No Release extraction/upload timing or GPU benchmark is claimed.

The complete renderer rebuild remains unfinished. The residency service still
constructs upload/publication graph operations, the sample still prepares cascades,
main shading and presentation in C++, and the light/game/editor operation library
is incomplete. These remaining algorithms must become independent reflected passes;
restoring the deleted backend is not a migration path.

## Directional cascade preparation authored

Current source inventory is **67 native reflected operations / 26 ordinary SDK assets**.
ShadowCascade is one coherent stabilized-projection/snapshot operation, using the existing
ShadowCascadeUtils math. It declares CascadeConstants as a concrete reflected uniform
buffer output. LightDirection and the existing nested ShadowCascadeSettings are normal
reflected member properties; there is no pin-type table, stage selector or new manager.
The ordinary ShadowCascades source asset is authored through document commands and can
be referenced and overridden like the other pipeline definitions.

The pass snapshots the explicit view camera and viewport during Build, validates finite
perspective/orthographic inputs, constructs one to four stabilized cascades and uploads
an immutable uniform snapshot. Stereo requires an explicit eye view instead of silently
using an arbitrary eye. Unused ABI entries are identity matrices/zero values. Execution
retains no view, camera, world or component pointer. It requests no scene extraction,
material/geometry residency or unrelated rendering feature.

CPU sparse-page queries can inspect the derived cascade results only for the graph setup
that produced them. Configuration, failed construction or another graph setup cannot
return a previous snapshot. These small derived values belong to the pass occurrence;
they are not authoritative transforms, a scene hierarchy or serialized graph identities.
Repeated definitions and separate views own distinct snapshots and graph allocations.

GpuDrivenScene now selects -shadow-cascade-pipeline and consumes the compiled
CascadeConstants boundary. Its manual cascade settings, direct math invocation,
VirtualShadowCascadePassData and C++ upload operation have been deleted. Quality values
live in the asset. The selected resolution drives raster dimensions and the actual shadow
texture determines sampling texel size. The client locates the native CPU query producer
through the explicit boundary binding, allowing other cascade occurrences in the graph.
The remaining main-shading algorithm accepts one shadow projection; this sample reports
an incompatibility for a multi-cascade result instead of silently ignoring extra cascades.
The native operation itself supports one to four cascades, including repeated instances.
Full multi-cascade shading remains work for the native lighting/raster composition.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest and GpuDrivenScene
  build with their engine/editor dependencies (native-cascade-final-build.log). Earlier
  compilation errors in the new test/code were corrected before this successful build.
- All 60 GraphicsCore CPU tests pass, including reflected nested settings, rejected
  invalid values, independent subgraph overrides and missing-camera diagnostics
  (native-cascade-cpu.log).
- All 20 NativeAssetAuthoring fixtures pass (native-cascade-authoring.log). All 15
  EditorDocuments tests pass, including deterministic packaged/rebuilt/repeated binary
  equality for all 26 SDK assets (native-cascade-final-editor.log). The first library
  run exposed an outdated expectation that every non-preview asset except ShadowDepth
  required compute; ShadowCascades correctly needs no compute capability, and that test
  expectation was corrected.
- Vulkan NativeShadowCascades passes actual uniform-buffer readbacks over three shared
  GPU frames, two perspective/orthographic views and two reused definitions with one
  and three cascades. It verifies distinct camera projections, authored 120/90 metre
  far splits, immutable reflected defaults under parameter overrides, inactive ABI
  entries and camera mutation after setup. Failed frames and later setups reject stale
  CPU queries. Each graph uses one GPU submission; no geometry/material residency is
  initialized (native-cascade-final-gpu.log).
- Vulkan NativeShadowFeedback (1), NativeVirtualShadowPages (1), NativeDepthAtlas (2),
  NativeVisibility (4) and PipelineExtraction (3) pass
  (native-cascade-regression-<group>.log).
- Missing and incompatible selected cascade resources exit 1 with the selected path
  and an explicit interface diagnostic (native-cascade-missing.log and
  native-cascade-incompatible.log).
- Production sample searches find no manual cascade upload data/operation, direct
  ShadowCascadeUtils invocation or sample cascade quality structure. Rendering Builtin
  file searches remain empty; git diff --check passes.

The full Vulkan sample still fails with actual visible geometry: it produces 16 virtual
pages in frame 3, records 118 registered / 110 compiled operations in one submission
with 186 barriers, then reports ErrorDeviceLost at CpuWaitOnlyFenceVulkan.cpp:123
(fence 46, request 47). The bounded process was stopped at the assertion after 30 seconds
(native-cascade-sample.log and native-cascade-assertion.txt). This phase does not claim
an application runtime fix or Release timing improvement. D3D12 remains excluded.

Remaining source migration includes the residency service's upload/publication graph
construction, native world light/cluster preparation, the sample's main shading and
presentation, and complete game/editor/advanced lighting compositions. Virtual page
identity still needs explicit world/view scoping. The full renderer migration remains
incomplete; the cascade upload deletion is a verified incremental step.

## Virtual shadow table uploads authored

Current source inventory is **68 native reflected operations / 27 ordinary SDK assets**.
VirtualShadowTables is an independent upload/readiness operation with three concrete
reflected output members: PhysicalPageTable, VirtualPageTable and PhysicalAtlas.
The ordinary VirtualShadowTables source asset was authored through document commands;
the packaged binary is produced by the normal asset compiler. It requires virtual-shadow
device residency, no compute capability and no scene extraction, geometry or materials.

The residency service's AddUploadPass implementation and UploadHandles type have been
deleted. RenderWorldModule no longer silently inserts that operation or publishes its
scalar blackboard fields. The eight unused virtual-shadow blackboard keys are gone.
GpuDrivenScene explicitly selects -shadow-tables-pipeline, builds the resource and
consumes its compiled interface and setup-scoped layout. The native page raster fixture
uses the same resource. Missing or incompatible resources have no fallback.

The existing device service retains page allocation, demand and retirement metadata;
it has no scene-object, transform or component hierarchy. It captures immutable compact
table deltas and strong allocation references. The native pass records coalesced
physical ranges and changed virtual-table slices. The first capture freezes that device
frame: demand arriving after it is deferred to the next frame, so another view cannot
rewrite tables beneath an earlier view. Repeated instances and views reference the same
physical residency allocations; they do not share pass-local mutable configuration.
Canonical setup names deduplicate uploads within a graph. Execution uses typed captured
handles. Graphics-queue readiness orders earlier graph uploads before consumers on
other queues without another CPU scene copy.

Dirty slice bits are acknowledged only after a private GPU completion fence signals
and the capture's allocation epoch and per-slot revisions still match. Recorded but
abandoned work remains retryable after shared-frame retirement. Initial allocation
initializes free physical records too. Failed reconfiguration preserves the previous
settings, allocation epoch, frozen capture and pending retirements. Successful replacement
gets a new epoch; old captures retain their allocations for existing recordings.

Focused verification before the subsequent compiler-counter refinement:

- Debug FoundationTest, GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest
  and GpuDrivenScene build with dependencies (native-shadow-tables-final-build.log and
  native-shadow-tables-retirement-build.log).
- Foundation Configuration (4), all GraphicsCore CPU tests (61), NativeAssetAuthoring
  (21) and EditorDocuments (15) pass. The library fixture checks deterministic rebuilds
  and repeated serialization of all 27 packaged assets (native-shadow-tables-foundation,
  -cpu, -authoring and -editor.log).
- Vulkan NativeShadowTables verifies two nested references in each of two independently
  initialized views, actual physical/virtual buffer readbacks, immutable shared-frame
  capture, sparse edits, unchanged frames, abandoned recordings, allocation rollback and
  safe replacement epochs. Both view graphs submit before host readback waits. The first
  frame plans 384 bytes in two upload ranges; a later sparse slice plans 272 bytes in
  two ranges; an unchanged slice transfers zero bytes; noncontiguous pin edits plan
  288 bytes in three ranges. The second view plans zero duplicate transfer bytes.
  Each fixture graph uses one submission (native-shadow-tables-gpu.log). These are
  small deterministic regression measurements, not Release throughput benchmarks.
- Vulkan NativeVirtualShadowPages passes actual raster/table readbacks and content
  revision checks after the consumer migration. Its populated phases use 17 operations /
  6 submissions / 5 render-pass groups, 13 / 1 / 2 and 16 / 1 / 4 respectively
  (native-shadow-tables-pages.log).

The initial build also exposed two subsystem declarations ending at source line 87
inside one unity translation unit. A Foundation regression deliberately declares two
subsystems on the same physical line and verifies unique registrations. The subsystem
instance macro now uses the compiler's translation-unit counter abstraction. Validation
of the subsequently refined compiler wrappers is recorded separately below.

This phase does not complete the renderer rebuild. VirtualShadowMapManager still
constructs AddPageCompletionPass; that publication algorithm must become independently
authored. Device page identity still needs explicit world/view scoping. Native world
light/cluster preparation, sample main shading/presentation and complete game/editor/
advanced lighting graphs remain unfinished. The previously reproduced full-sample
Vulkan ErrorDeviceLost is unresolved; focused table tests do not prove application
correctness. D3D12 remains excluded.

### Table ownership audit follow-up

The obsolete xiiVirtualShadowPageUpdate/xiiVirtualShadowPageUpdateType reflection types,
GetPageTableUpdates API, CPU edit array, append operations and frame clears are deleted.
Repository search found no production consumer. The eviction regression now checks the
old mapping's removal, pinned mapping's survival and actual residency counts instead of
an unused journal length. Immutable upload deltas remain the single table transfer path.

An additional lifetime audit found that old feedback age alone did not prove an atlas
page reusable: a more recent frozen table could still reference it. Each physical slot
now records its last captured GPU frame. Eviction unmaps it from future captures, then
retains the physical page until every graph in that frame has retired. A small private
retirement list tracks only these physical allocations, not scene objects. Retired pages
count against the existing per-frame allocation-work budget across feedback submissions.
Demand can retry once the allocation is safe; no atlas storage is overwritten early.
RetiredPageCount distinguishes withheld physical capacity from free or resident pages.
Device restart releases that old allocation bookkeeping because it creates a distinct atlas.

The GPU regression now copies actual table contents on alternating graphics/compute
queues and checks compiler-generated queue waits. A second two-view fixture requests
replacement after the first view freezes a full table. Both views must still read the
old mapping, while CPU lookup has unmapped it and physical reuse is withheld. The next
retired frame permits reuse with a new content revision, rejecting the old raster ticket.
Final build/test outcomes for these follow-up changes are recorded below.

Final verification:

- The fresh Foundation-counter build succeeds for FoundationTest, GraphicsCoreTest,
  GraphicsFoundationTest, EditorFrameworkTest, EditorEngineProcessFrameworkTest and
  GpuDrivenScene with all dependencies (native-shadow-tables-counter-build.log).
  Foundation Configuration (4) passes (native-shadow-tables-counter-foundation.log).
  The three compiler wrappers are included with the user's approval; this is an MSVC
  build, not verification of GCC/Clang toolchains.
- The subsequent journal/retirement build succeeds for all five affected graphics,
  editor and sample targets (native-shadow-tables-retired-pages-final-build.log).
  Its first attempt exposed a missing XII_DECLARE_POD_TYPE annotation on the small
  retirement record; that compile error was fixed before the successful build.
- All 61 CPU tests, 21 NativeAssetAuthoring fixtures and 15 EditorDocuments tests pass
  again. The asset library checks all 27 source/binary resources, including deterministic
  recompilation and repeated-instance serialization (native-shadow-tables-verified-cpu,
  -authoring and -editor.log).
- Vulkan NativeShadowTables (2), NativeVirtualShadowPages (1), NativeVisibility (4),
  NativeDepthAtlas (2), PipelineExtraction (3), NativeGeometryFeedback (1),
  NativeShadowFeedback (1), NativeShadowCascades (1), RenderGraphAllocations (1),
  FenceSynchronization (1), NativeSceneLod (4), SkinningRaster (3), NativeMeshCapture (3)
  and EditorViews (47) all pass (native-shadow-tables-verified-<group>.log).
  FenceSynchronization was built and run with the user's concurrent FenceTest.cpp
  refinement, which is deliberately left out of these commits.
- The final actual-copy table fixture reports 384 bytes / 2 upload ranges / 1 submission
  initially, 272 / 2 / 1 for a sparse slice, 0 / 0 / 2 for an unchanged slice with a
  compute-queue copy consumer, and 288 / 3 / 2 for noncontiguous edits with that consumer.
  Both views submit before host readback waits. The second view plans no duplicate table
  transfer. These extra copy submissions belong to the regression's explicit cross-queue
  consumer; they are not one submission per reflected node.
- Missing and incompatible -shadow-tables-pipeline resources both exit 1 with the selected
  path and a specific interface diagnostic (native-shadow-tables-missing.log and
  native-shadow-tables-incompatible.log). Searches find no old shadow upload API, edit
  journal API/types, shadow blackboard keys, scene database classes or rendering Builtin
  filenames in the relevant production sources/assets. git diff --check passes.

The full Vulkan sample still fails: the 30-frame request exits 3 in device frame 3 after
producing 16 virtual pages and recording 118 registered / 110 compiled operations,
one submission and 186 barriers. The assertion is again CpuWaitOnlyFenceVulkan.cpp:123,
fence 46 / request 47 / ErrorDeviceLost (native-shadow-tables-sample.log and
native-shadow-tables-sample-assertion.txt). It did not complete the requested frames.

Retirement can delay new page admission until the last captured frame retires; this is
required to protect older GPU readers. The tiny two-view regression demonstrates reuse
in the next completed frame, not cache-churn throughput. No Release extraction/upload
benchmark or application performance improvement is claimed. The remaining publication,
light, main-shading, presentation and world/view identity work above is still outstanding.

## Typed frame data and independently authored page publication

Current source inventory is **70 native reflected operations / 29 ordinary SDK assets**.
VirtualShadowRasterWork selects one bounded immutable batch from explicit cascade and
residency snapshots. LightId and MaxPages are reflected member properties, with typed
occurrence overrides. VirtualShadowPagePublish consumes that batch, the final raster
atlas and both page tables. These are separate operations with concrete XII_MEMBER_PROPERTY
pin members. Neither constructs another pass or controls a complete rendering stack.

The existing reflected pin/binding architecture now supports FrameData alongside texture,
buffer, order and acceleration-structure pins. Existing resource-kind ordinals retain
their values. A native RTTI contract determines each data pin's type; assets cannot
redefine it. Immutable ref-counted snapshots belong to one graph setup. External bindings
cannot retag an older snapshot, and runtime validation rejects incorrect types, mixed
resource kinds and stale data. No GPU operation, allocation or submission is created merely
to forward a CPU snapshot. Core graph topology, handles and mutation guarantees are unchanged.

The Qt editor uses the existing property inspector for these pins. Their read-only DataType
metadata identifies the native contract, and orange distinguishes them from GPU resource
and ordering pins. Normal document commands preserve pin identities, display names,
connections, undo/redo and serialization. The asset compiler infers typed boundaries through
the same native schemas and Core composition expansion; no second node or property framework
has been added. Runtime snapshot values and graph handles are never serialized into assets.

ShadowCascade and VirtualShadowTables now publish explicit CascadeSnapshot and Residency
data outputs. Their pass-specific CPU query methods have been deleted. GpuDrivenScene no
longer searches concrete pass objects to locate those results, and no longer gathers page
work or invokes service-owned graph publication. It selects ordinary -shadow-work-pipeline
and -shadow-publish-pipeline resources and binds their interfaces. Its remaining per-page
ShadowMeshDepth instantiation loop is still a client-side composition target, not a complete
native shadow pipeline.

VirtualShadowMapManager's AddPageCompletionPass implementation/API is deleted. Its header
and implementation no longer depend on RenderGraph or command-list construction. The
remaining capture/retirement helpers validate immutable allocation provenance, page tickets,
frame/epoch, duplicate slots and byte bounds. Compact retirement tickets omit raster matrices
and viewports. Device allocation, CPU residency and completion bookkeeping remain with the
existing focused device service; no scene owner, object registry or hierarchy is introduced.

Publication uses a CPU-wait-only completion fence and signals it only after recording the
clean table writes. CPU dirty state retires after actual GPU completion; invalidated tickets
are skipped and abandoned recordings remain retryable. Both physical and virtual records
are updated coherently. Sparse updates use ReadWriteBuffer to preserve untouched records and
parent upload/reader dependencies; treating them as discard writes would misdescribe their
semantics. Final atlas/table resources remain explicit typed GPU pins, independently of the
CPU snapshot connection. Empty batches forward the supplied resources without GPU work.

The two new source/binary assets and updated ShadowCascades/VirtualShadowTables interfaces
were authored through real editor document commands. All 29 packaged binaries were refreshed
through the normal exporter for the added reflected DataType metadata. No runtime snapshot
payload or transient GPU identity is part of that serialization. Existing source assets with
no data pins retain their topology and authored configuration.

Verification for this phase:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies
  (native-shadow-publication-consumer-build.log). New test compilation errors involving
  dictionary construction, RTTI NewInstance conversion and borrowed compilation pointers
  were corrected before this successful build.
- All 64 CPU tests pass (native-shadow-publication-cpu.log), including nested/repeated
  frame-data definitions, independent instances, serialization, reference lifetime after
  instance teardown, incorrect types, bypassed setters and stale external snapshots.
- All 23 NativeAssetAuthoring fixtures pass (native-shadow-publication-authoring.log).
  All 16 EditorDocuments tests pass after package refresh (native-shadow-publication-editor-
  verified.log), including all 29 deterministic packaged/rebuilt/repeated exports.
  The first new inspector test incorrectly retained raw pin pointers across undo/redo;
  it now reacquires pins through persistent document identities. The intervening package
  equality failures exposed the expected added metadata and were fixed by normal export,
  not by weakening the verifier.
- Vulkan NativeVirtualShadowPages passes actual atlas pixel and physical/virtual table
  readbacks using two independently configured work instances and the selected publication
  asset. Incorrect allocation bindings and duplicate tickets are rejected without changing
  CPU dirty state. Invalidation after setup, abandoned setup and recycled raster tickets
  remain covered. Populated phases still use 17 / 6 / 5, 13 / 1 / 2 and 16 / 1 / 4 logical
  operations / submissions / render-pass groups (native-shadow-publication-
  NativeVirtualShadowPages.log).
- Vulkan NativeShadowCascades and NativeShadowTables pass after replacing the old query
  methods with typed bindings. Existing coalesced upload byte/range counts and cross-queue
  submission counts remain unchanged (native-shadow-publication-NativeShadowCascades.log
  and native-shadow-publication-NativeShadowTables.log).
- All 74 subtests in 14 selected Vulkan regression groups pass: native shadow tables/pages,
  visibility, depth atlas, extraction, geometry/shadow feedback, cascades, graph allocation,
  fence synchronization, scene LOD, skinning raster, mesh capture and editor views
  (native-shadow-publication-regression-results.json and its per-group logs).
  FenceSynchronization was built and run with the user's separate FenceTest.cpp changes;
  those changes are intentionally excluded from this implementation's commits.
- Missing and incompatible work/publication resources each exit 1 with an interface or
  resource diagnostic (native-shadow-publication-sample-results.json). Source searches
  find no active scene database types, old shadow snapshot query methods, service-owned
  page-completion graph API, or rendering Builtin filenames. git diff --check passes.

The full 30-frame Vulkan sample still fails in device frame 3 after producing 16 pages:
118 registered / 110 compiled / 8 culled operations, one submission and 186 barriers.
CpuWaitOnlyFenceVulkan.cpp:123 reports fence 46 / request 47 / ErrorDeviceLost. This run
did not exit within its 30-second bound and its own process was stopped (exit -1), rather
than completing the requested frames. Evidence is in native-shadow-publication-sample.log,
native-shadow-publication-sample-assertion.txt and native-shadow-publication-sample-results.json.
The focused passes and GPU readbacks do not establish whole-application correctness.

Additional isolation runs reproduce the device loss with one grid object and ray tracing
disabled (-grid-width 1 -grid-height 1 -ray-tracing false). The minimal graph records
73 registered / 65 compiled / 8 culled operations, one submission and 151 barriers,
with two virtual pages in device frame 3. Its assertion reports fence 32 / request 33 /
ErrorDeviceLost. The normal and synchronization-validation runs were bounded to 30 seconds;
GPU-assisted safe-mode validation was bounded to 45 seconds. All three required stopping
their own process. The installed SDK's synchronization validation reported no resource
hazard. GPU-assisted validation confirmed activation through its device-feature warnings,
but reported no shader access violation before the device loss. These runs rule out ray
tracing and the large grid as prerequisites; they do not establish a root cause. Logs and
assertion captures use native-shadow-publication-one-object, -sync-validation and
-gpu-validation prefixes in Workspace.

The full renderer conversion is still incomplete. Native light/cluster preparation,
complete game/editor/advanced lighting graphs, sample main shading/presentation and
world/view-scoped virtual page identity remain required. D3D12 is excluded; no Release
benchmark or whole-renderer success is inferred from the focused tests above.


## Authored view targets, presentation and external buffer lifetime

The current inventory is **71 native reflected operations / 32 ordinary SDK assets**.
SceneTargets composes the existing ClearTexture and DepthTarget operations; its color,
format and reversed depth clear remain ordinary reflected member properties. ViewPresentation
uses the existing Present operation. BufferExport contains the new narrow buffer-export
operation. All three source/binary pairs were authored and exported with normal document
commands. These are editable resources, not runtime C++ graph factories or Builtin files.
No extra clear or presentation class was added to increase the operation count.

GpuDrivenScene now selects -targets-pipeline, -presentation-pipeline and
-sensor-export-pipeline resources. Its SceneTargetsPassData, PresentPassData, combined target
initialization callback and final copy/export callback are deleted. The remaining raster
callback consumes explicit color/depth handles and declares read/write preservation;
string lookup of "GPU Scene Color" and "GPU Scene Depth" is gone. The second view's counter
is retained by the independently selected BufferExport resource, rather than an unrelated
presentation callback. Selected resources synchronize every frame; missing or incompatible
interfaces produce a path-specific failure with no fallback graph.

Present's preparation and execution now depend on owned setup captures, including shader
objects, sampler, display constants and output dimensions. The recorded callbacks retain no
pass, world, camera or mutable view pointer. Shader resource lookup happens during setup;
execution uses the captured pipeline description and existing GAL pipeline cache. Fullscreen
coverage replaces a redundant clear. Explicit scissor/viewport state covers the entire output.
Both offscreen outputs and swapchain images receive a declared final state; swapchain images
are explicitly exported in Present state. An explicit texture target takes precedence over
an attached swapchain and is treated as offscreen. Unsupported subview/mip targets, depth
inputs and missing output targets fail rather than silently using a different image.

BufferExport has native XII_MEMBER_PROPERTY input/output pins. It roots one supplied SRV
buffer and declares its final shader-readable state through the existing graph compiler.
It creates no scene registry, upload, copy, dispatch, GPU allocation, specialized pipeline
manager or new lifetime service. Physical retirement stays with the graph/resource cache.
World/component extraction and Core graph handle semantics are unchanged.

Verification:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies
  (native-view-boundary-final-consumer-build.log). The initial new fixture incorrectly named the
  existing frame scope and omitted the POD trait needed for constant-record readback;
  these fixture errors were corrected before the successful build.
- All 64 CPU subtests pass (native-view-boundary-cpu.log).
- All 26 NativeAssetAuthoring fixtures pass (native-view-boundary-authoring.log).
  All 16 EditorDocuments tests pass, including all 32 packaged/source/repeated deterministic
  exports (native-view-boundary-editor.log).
- Vulkan NativeViewBoundary passes three device frames with two independently configured
  views, landscape/portrait sizes, different clear colors and different paper-white values.
  Both graphs submit before host readback. Actual source/output pixels, reversed depth,
  uniform constants and exported counter values match. Every pass instance is destroyed
  after setup and before command recording. Each graph uses one submission. Missing targets,
  depth-as-color input and shader-resource views supplied as output targets are rejected,
  with no previous output exposed. Geometry and material residency remain uninitialized
  throughout (native-view-boundary-final-gpu.log).
- All 47 Vulkan EditorViews tests pass, including output capture and world/view lifecycle
  (native-view-boundary-editor-views.log).
- All six missing/incompatible target, presentation and sensor-export selections exit 1
  with a specific resource/interface diagnostic (native-view-boundary-sample-results.json).

The full Vulkan sample still exits 3 during the 30-frame request: device frame 3 produces
16 virtual pages and records 120 registered / 112 compiled / 8 culled operations, one
submission and 186 barriers. CpuWaitOnlyFenceVulkan.cpp:123 reports fence 46 / request 47 /
ErrorDeviceLost (native-view-boundary-sample.log and -sample-assertion.txt). This phase adds
two live logical operations while retaining the previous one submission and barrier count;
it is not a rendering throughput benchmark or proof of application correctness. D3D12 was
excluded. No Release performance or whole-renderer success is claimed.

The complete native renderer remains unfinished. The sample's main mesh shading/constants,
per-page graph instantiation and ray validation orchestration remain C++ migration targets.
World light/cluster preparation, complete game/editor/advanced lighting graphs and
world/view-scoped virtual shadow page identity remain outstanding. The latest authoritative
limitations supersede older phase notes about target creation and final presentation.

A final caller audit also found obsolete default-Deferred fixture loads in
GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h, called without an explicit asset by
EditorEngineProcessFrameworkTest/SensorMemoryTest.cpp, TemporalHistoryTest.cpp and
GraphicsFoundationTest/Pipeline/RenderPipelineExecutionTest.cpp. These registered tests
still need replacement compositions and migration; the passing EditorViews group does not
cover them. The deleted Deferred asset must not be restored as a fallback for these callers.


## Independent calibrated sensor operations

Current inventory: **74 concrete native reflected operations / 37 SDK assets**.
The combined SensorOutputPass header/implementation and Pipeline/SensorOutput shader
are physically deleted. Four independent Depth, RGB, Infrared and LiDAR operations
replace them in the Sensors directory, with separate shader programs. There is no
runtime sensor-type switch. Every pin uses XII_MEMBER_PROPERTY and an owned typed
handle; calibration and view-profile selection are reflected member properties.

The existing physical calibration fields now belong to reflected SensorCalibration.
SensorProfile version 3 derives from it and retains its physical Type. Native nodes
expose Calibration without a selectable algorithm Type: creating a Depth node in the
editor therefore cannot accidentally instantiate an RGB calibration. A binary RTTI
roundtrip covers both inherited calibration values and the profile's Type. Existing
immutable profile registration remains the owner of explicitly selected view profiles;
no additional world module, registry or scene representation is introduced.

Depth consumes only depth and canonical camera constants. RGB and Infrared additionally
consume radiance; LiDAR consumes octahedral normals and reflectance. Rolling shutter
requires explicit matching velocity and frame-time inputs; a shader permutation removes
those bindings for global shutter. Input dimensions, resource shapes, calibration and
physical algorithm mismatches fail during setup. Depth is reconstructed with the
supplied inverse projection, including a finite far plane, rather than assuming an
infinite-far near/depth relationship. Existing distortion, quantization and noise
functions remain shared shader math, not a combined executable algorithm.

SensorCapture is a non-allocatable reflected helper, excluded from the operation count.
It owns calibration validation and each occurrence's output image/result. Recorded work
captures its pipeline, immutable constants and refcounted result; it retains no mutable
pass/view/world pointer. A destroyed pass remains safe after graph setup. Output images
and retirement use existing GAL/graph facilities. Independent instances and views never
share a writable image merely because they reference the same authored definition.

Five ordinary source/binary pairs were authored/exported through editor document commands:
DepthSensor, RgbSensor, InfraredSensor, LidarSensor and DepthSensorDiagnostic. The diagnostic
combines existing depth-target and view-constant producers with the depth sensor. Its
empty reversed-depth image deliberately tests invalid/background measurements; it is not
a scene-geometry capture pipeline. ScreenEffects no longer embeds an unrelated sensor.
Its remaining node identities/connections were preserved; the optional motion interface
uses the existing zero-work texture-forward operation. No one-time asset editing code or
compatibility wrapper remains. Native sensor leaf interfaces come from reflected pins,
not manually duplicated pin-type arrays. Source assets declare Compute explicitly.

SensorMemory now selects DepthSensorDiagnostic explicitly; both old default-Deferred
loads and its wrapper allocation-failure pass are removed. It exercises the native
pass's allocation precondition directly, then verifies recovery through the authored
resource. The screen-effect GPU fixture consumes radiance through RGB as well as depth
through Depth, keeping its refraction/GI work live. PipelineResources' two-view fixture
now reads its private readback through the owning pass occurrence, replacing a broken
lookup by the former global buffer name; its test passes also use reflected typed pins.

Verification, Debug/Vulkan (D3D12 excluded):

- GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies:
  native-sensor-verified-build.log. Earlier overlapping builds/tests caused PDB and Qt
  deployment contention; final verification runs builds and tests sequentially.
- All 66 CPU subtests pass (native-sensor-final-verified-cpu.log), including inherited
  calibration binary serialization, native pin schemas and pipeline roundtrips.
- All 31 NativeAssetAuthoring fixtures pass (native-sensor-final-authoring.log).
  All 16 EditorDocuments tests pass with 37 packaged/source/repeated deterministic
  exports (native-sensor-verified-editor.log).
- NativeSensors passes actual GPU readbacks for four nested referenced definitions,
  with both global and rolling shutter (native-sensor-final-native-gpu.log). Analytic
  finite-projection depth/range, RGB response, infrared wavelength/response and
  unsaturated LiDAR incidence/return values match. Instances are destroyed before
  recording. Geometry/material services remain uninitialized. Initial fixture errors
  in texture slice pitch, legal shutter configuration and octahedral normal encoding
  were corrected before this successful run. GPU noise variants are not reference-tested.
- All 14 PipelineResources GPU tests pass (native-sensor-final-pipeline-resources.log),
  including live reusable screen effects and independent view/subgraph state.
- Both SensorMemory tests pass (native-sensor-final-memory.log): two views, 64 profile
  switches with frames in flight, output invalidation/recovery, readback and safe
  retirement after completion. Every retained retired image has only the test reference.
  Extraction requirements are empty; instance data and geometry/material services remain
  uninitialized. This establishes demand isolation, not an extraction throughput benchmark.
- All 47 EditorViews tests pass (native-sensor-final-editor-views.log).
- Source/data searches find no active SensorOutputPass references, and no GraphicsCore
  Builtin or SceneDatabase source files. Historical phase notes above are retained as history.

The full renderer conversion remains incomplete. TemporalHistoryTest.cpp and the
PipelineDefault/OffscreenRendering fixture in RenderPipelineExecutionTest.cpp still
load the removed Deferred asset through PipelineLibraryTestUtils.h's default argument.
Their real native compositions must be rebuilt; neither fixture was counted as passing
above. Native light/cluster preparation, complete game/editor/advanced lighting graphs,
the sample's remaining mesh/ray orchestration and world/view virtual page identities
also remain outstanding. The last full Vulkan sample run still failed with
ErrorDeviceLost as documented in the preceding phase; this sensor-only phase did not
rerun or resolve it. No Release benchmark or complete-renderer success is claimed.


## Authored directional mesh shading and successful full Vulkan sample

Current inventory: **77 concrete native reflected operations / 40 SDK assets**.
GpuDrivenScene's GpuDrivenDrawPassData, ExecuteGpuDrivenDraw, CreateSceneRenderPass,
shader preload/recording guard, framebuffer construction and two inline uniform uploads
are deleted. Its old Shaders/GpuDrivenScene.xiiShader and GpuDrivenSceneConstants.h
files are physically removed. Material creation now references the relocated engine
shader. There is no old shader fallback or compatibility wrapper.

DirectionalMeshConstants captures reflected ambient/sun/selection-pulse properties and
an explicit shadow image/projection. An optional typed CascadeSnapshot supplies the
projection and world-units-per-texel. Geometry/camera/layout constants reuse the existing
MatrixRasterConstants operation; they are not duplicated in a new constants system.
VirtualShadowSamplingConstants consumes explicit immutable Residency frame data and
captures page addresses, dimensions and the selected light. The existing device table
upload snapshot now also records virtual resolution and page size. Recording cannot
observe a later global configuration through these values. Neither producer initializes
lighting, owns scene objects or queries a live world during execution.

DirectionalMeshRaster consumes explicit structured streams, canonical raster/shading
uniforms, mesh indirect arguments, shadow images/tables and matching color/depth targets.
It preserves the supplied attachments and participates in managed graph render passes.
Shader objects, signature, states, sampler and target dimensions are retained by the
recorded graph. No pass, mutable camera, world, window or application pointer is captured.
Unsupported mesh/bindless capabilities and incompatible resource contracts fail setup.
Scissor support is declared in the shader; both uniform producers restore their declared
constant-buffer state after dynamic-map staging, following the existing matrix producer.

The directional shader uses the existing bounded canonical meshlet/primitive-range,
normal-transform, material-table and current-skinning paths. The shared surface interface
adds optional world-position/user-data outputs for this consumer; no second vertex or
meshlet decoder remains. This is the sample's directional Lambert shading with ordinary
and virtual shadow sampling, not a claim of a complete physically based forward pipeline.

DirectionalMesh, DirectionalMeshConstants and VirtualShadowSamplingConstants are ordinary
editable source/binary asset pairs, authored and exported through document commands.
DirectionalMesh composes four independent operations, including existing matrix constants.
Interfaces are derived from reflected member pins. The sample explicitly selects
-directional-pipeline, synchronizes that resource each frame, supplies its data interfaces
and publishes its returned color/depth to the separately selected depth/presentation
resources. Missing or incompatible selections report resource/interface diagnostics.
No runtime graph factory reconstructs this asset.

Verification, Debug/Vulkan; D3D12 excluded:

- GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies
  (native-directional-consumer-build.log). Subsequent uniform-state/fixture changes
  rebuild GraphicsFoundationTest/GpuDrivenScene successfully (native-directional-state-build.log
  and native-directional-final-readback-build.log). The first build exposed misuse of a
  shader-layout matrix wrapper; CPU matrices are now resolved before writing GPU records.
- All 66 CPU tests pass (native-directional-final-cpu.log).
- All 34 NativeAssetAuthoring fixtures pass (native-directional-authoring.log).
  All 16 EditorDocuments tests pass with 40 deterministic packaged/source/repeated exports
  (native-directional-editor.log).
- All 19 Native Vulkan subtests pass, including visibility/G-buffer/depth, atlas raster,
  virtual page publication/retirement, shadow tables, sensors, view boundaries and the new
  directional composition (native-directional-native-regressions.log; zero logged errors).
- NativeDirectionalMesh references one authored definition twice with different ambient
  values, light IDs and landscape/portrait dimensions. It renders a registered material
  through the existing device residency services over four frames and destroys every
  pipeline pass before final recording. GPU readback verifies exact material-modulated
  colors, preserved background, analytic culled-face depth, private images and uploaded
  per-occurrence shading/sampling constants. Its 23 compiled operations use one submission
  and 41 barriers. Initial fixture errors used a global size override for different targets,
  assumed an unregistered material slot and expected the wrong face under back-face culling;
  these were corrected without weakening the production contracts. The test also exposed
  and verified the explicit scissor/uniform-state corrections above.
- Missing and incompatible directional selections finish with exit 1; the normal sample
  completes 30 requested frames with exit 0 (native-directional-sample-results.json).
  After the final state fixes, the full sample completes 300 requested frames with exit 0
  (native-directional-endurance-sample-results.json and -sample.log). The log explicitly
  records 16 virtual pages in device frame 3 and completion of all 300 GPU-driven frames.
  No ErrorDeviceLost or other logged error occurs. Two benign unused mesh-output interface
  warnings remain; they are not resource hazards or validation errors.

The sampled steady full graph records 123 registered / 115 compiled / 8 culled operations,
one queue submission and 187 barriers. Compared with the preceding phase's 120 / 112 / 8,
one submission and 186 barriers, this adds three logical operations and one barrier while
preserving submission count. These are actual graph statistics, not a Release throughput
benchmark. The passing 30/300-frame runs supersede the preceding phase's device-loss
limitation for this configuration. Several implementation and shader/lifetime changes
were necessary, so these results do not isolate one proven cause in the removed callback.

The complete renderer migration remains unfinished. Native light/cluster preparation,
complete game/editor/advanced lighting graphs, ray-validation and per-page sample
orchestration, and world/view-scoped virtual page identities remain. The two obsolete
Deferred fixture loads in TemporalHistoryTest.cpp and RenderPipelineExecutionTest.cpp
and the helper's default argument still need removal/replacement. No claim of full engine
migration, all-backend correctness or a completed hundred-operation library is made.


## Native world ray probes and retained recording ownership

SceneRayProbe is an independent reflected closest-triangle operation. Its Scene and
Materials inputs are concrete typed member properties; Result is a structured GPU
buffer. Origin, direction, metric range, instance mask and queue preference are reflected
properties with normal editor attributes. Typed occurrence parameters can override the
ray without mutating authored defaults. Invalid values produce occurrence provenance in
the build diagnostic. No stage selector, pin-type array or scene lifecycle is introduced.

The shader returns hit status, the full generation-validated world GPU identity,
primitive index, hit kind, distance and barycentrics. HLSL InstanceIndex indexes the
existing canonical hit-metadata records explicitly supplied through Materials. The
hardware custom ID remains a compact TLAS index; it is not repurposed as a 32-bit world
identity or silently truncated to 24 bits. The first GPU readback caught this distinction
and the operation now consumes the existing metadata rather than inventing another ID
registry. This triangle-only geometric probe does not implement material alpha testing,
procedural intersections or a complete ray renderer.

Every occurrence/frame owns graph allocations for its constants, shader records and
result. The graph retains the compiled pipeline and explicit TLAS, and declares the
TLAS build dependency and metadata-buffer read. An empty eligible scene produces a
defined miss without allocating an SBT or initializing a ray shader. Unsupported hardware
still fails the declared device contract. Shader/PSO acquisition occurs during setup;
recording captures no world, view, mutable component storage or pass pointer.

SceneRayProbe and WorldRayProbe are ordinary editable source/binary assets, exported by
the editor document implementation. WorldRayProbe connects RayTracingScene -> SceneRayProbe
-> BufferExport and exposes Result. The library now contains 42 asset pairs. Repeated
references configure independent writable TLAS/results while using existing device BLAS
residency and world deformation caches. The sample selects this resource explicitly with
-ray-probe-pipeline and synchronizes it each frame. Its former AddPass validation callback,
persistent SBT, shader/permutation members, validation-only shader and unrelated startup
shader smoke checks are physically removed. No direct low-level graph execution callback
remains in GpuDrivenScene.cpp. Its per-page resource orchestration is still C++ code.

Recording ownership in the existing ray-tracing services is also corrected. A queued
TLAS build retains its occurrence frame state and geometry leases. Deformation callbacks
retain their existing cache entry, and queued dynamic BLAS records retain their build
entry even after the world or occurrence is destroyed. Replacing dynamic source geometry
allocates a new entry rather than overwriting an entry still referenced by queued work.
This changes retention of derived allocations; no scene registry, transform hierarchy or
new manager is added. Public frame-resource copying remains prohibited.

The full sample exposed a second issue that focused write-locked tests did not: the
scene-source pass used a mutable world-module lookup during read-only graph setup. It now
uses the const lookup, and the sample establishes the same world read boundary used by its
other world-consuming resources. Simulation/extraction still uses the existing world write
boundary, and execution uses retained immutable captures.

Verification, Debug/Vulkan; D3D12 excluded:

- All five affected consumers build with their dependencies: GraphicsCoreTest,
  GraphicsFoundationTest, EditorFrameworkTest, EditorEngineProcessFrameworkTest and
  GpuDrivenScene (native-ray-probe-consumer-build.log). Subsequent identity, read-boundary
  and final regression edits rebuild successfully (native-ray-probe-identity-build.log,
  -final-build.log, -world-boundary-build.log and -read-boundary-build.log).
- All 66 GraphicsCore CPU tests pass (native-ray-probe-final-cpu.log). All 36 native
  authoring fixtures and 16 EditorDocuments fixtures pass, including deterministic
  packaged/source/repeated exports for all 42 assets (native-ray-probe-identity-authoring.log
  and -identity-editor.log).
- All 20 native GPU subtests pass (native-ray-probe-final-native.log). NativeSceneRayProbe
  checks analytic cube hit distance and barycentrics, full owner identity, private TLAS and
  result buffers, mask/range changes, authoritative transform updates, empty-world misses,
  foreign/retired identity rejection after world recreation, invalid-parameter diagnostics
  and output clearing. It destroys every world/pass owner before final recording and
  performs setup under a read marker. Two nonempty instances compile to eight operations,
  one submission and 16-23 barriers; empty instances use four operations and four barriers.
- All three PipelineExtraction subtests pass (native-ray-probe-extraction.log). The
  deformation regression now destroys its occurrence owners before the final skin/morph
  and dynamic BLAS recording. Existing delayed frames, component replacement and source
  reload checks remain. The existing incremental fixture records 288 uploaded bytes once
  for a changed instance across two cameras, then zero bytes for a warmed static frame.
  Its Debug-only 1024-component packing timing is 51.013 ms for 20 updates (2.551 ms/update);
  this is a diagnostic measurement, not a Release performance comparison.
- Missing and incompatible ray-probe resources exit 1 with diagnostics. The full sample
  completes all 300 requested frames with exit 0 and 384 world-derived TLAS instances
  (native-ray-probe-sample-results.json and -sample.log). The initial full run exposed the
  missing world access boundary above; its assertion log is preserved separately. After
  correction there is no logged error/device loss. The two existing unused mesh-output
  warnings remain.
- With ray tracing disabled, 300 frames also complete with exit 0 despite deliberately
  selecting an absent ray-probe path (native-ray-probe-raster-only-results.json and .log).
  That unrequested resource and its TLAS/shader setup are never consumed.

The steady full sample records 124 registered / 116 compiled / 8 culled operations,
one queue submission and 190 barriers. The preceding phase recorded 123 / 115 / 8,
one submission and 187 barriers. The explicit result export adds one operation; private
shader records/constants and metadata reads remain within the same submission. No Release
throughput or cross-backend benchmark is claimed.

The renderer migration is still incomplete: native light/cluster preparation, complete
application/editor/advanced lighting graphs, per-page sample orchestration and world/view
virtual page identities remain. The obsolete Deferred fixture loads and helper default
listed in the preceding report also remain at this point. No claim of a completed
hundred-operation library or complete engine migration is made.


## Remove obsolete default-pipeline test orchestration

The final implicit Deferred descriptor loads in RenderPipelineExecutionTest.cpp and
TemporalHistoryTest.cpp are removed. PipelineLibraryTestUtils.h now requires an explicit
asset name. PipelineScheduling.OffscreenRendering composes the existing SceneTargets and
ViewPresentation assets by document identity, with a reflected color override and explicit
interface connection. World updates execute the selected graph, and GPU readback checks
every output pixel across a runtime display-parameter update. The two compiled operations
use one submission, request no scene extraction and initialize no geometry/material service.
The former greater-than-ten-passes assertion and bloom-only configuration are deleted.

The obsolete nine-feature temporal checker and InterleavedDefaultPipelines fixture are
physically deleted (238 lines). They depended on deleted orchestration, hardcoded global
resource names, removed ordering pins and implicit DDGI/voxel initialization. No test-only
legacy renderer is reconstructed to satisfy them. Existing native interleaved depth-history,
independent AO/SSR/TAA operation and failure-recovery tests remain. This removes invalid
coverage claims; complete advanced lighting graphs and their integration coverage still
require native implementation.

GraphicsCoreTest, GraphicsFoundationTest and EditorEngineProcessFrameworkTest rebuild
successfully (native-fixture-cleanup-build.log). All 66 CPU tests, both PipelineScheduling
subtests, all PipelineResources subtests, NativeDepthBoundary and TemporalHistory pass on
Vulkan (native-fixture-cpu.log, -scheduling.log, -resources.log, -depth-boundary.log and
-temporal.log). Tracked Source/Data/Base searches find no SceneDatabase/manager or old
Deferred/EditorScene binary path references. Ignored generated AssetCache lookup tables
still contain historical paths; they are not source assets or active implementations.
No render Builtin files or compatibility wrappers are added.


## Retain private temporal state through command recording

BufferHistoryPass and TextureHistoryPass previously captured their pass pointer in the
queued copy callback. Both now retain their existing private state until recording is
finished. Slot selection, completion checks, readiness fences, copy ranges, promotion and
invalidation semantics are unchanged. Each newly constructed pass still allocates its own
state; copying and assignment are explicitly prohibited, preserving the previous unique
ownership contract. No new temporal manager or shared writable cache is introduced.

NativeHistoryOwnership creates one definition with native depth/counter producers and
both history operations, references it twice with different reflected values, and builds
two independent pipeline roots. Actual GPU readback checks four private image histories
and four private buffer histories, first-frame absence and distinct values. Every pass is
destroyed before final setup completion/compilation/recording. Static assertions also
preserve the noncopyable contract. This regression exercises queued state promotion after
the original pass objects are gone, alongside earlier retained-history reads.

GraphicsCoreTest, GraphicsFoundationTest, EditorEngineProcessFrameworkTest and GpuDrivenScene
build with dependencies (native-history-retention-build.log and -final-build.log). All 66
CPU tests, 21 native GPU tests, 14 PipelineResources tests, PipelineTemporal,
NativeDepthBoundary, TemporalHistory and all three TemporalMemory tests pass on Vulkan
(native-history-retention-*-regressions.log and -cpu.log). The full sample again completes
300 frames with exit 0 (native-history-retention-sample-results.json and .log). D3D12 is
excluded. No new Release throughput measurement is claimed; the change adds retained
references to existing occurrence state, without copying authoring graphs or world data.

Current totals remain 78 concrete native pass classes and 42 asset pairs. Complete native
light/cluster preparation, full application/editor/advanced lighting graphs, per-page
sample orchestration and world/view-scoped virtual page identities remain outstanding.


## Native cluster layout, bounds, assignment and sampling

Five independent reflected operations replace the orphaned cluster shader path:

- LightClusterView captures a setup-owned camera/projection/layout snapshot. It supports
  explicit projections as well as a view camera, exact viewport sizes, bounded list budgets,
  and logarithmic or linear depth slices. It performs no GPU work and requests no scene
  extraction. Invalid ranges, singular/horizon-crossing projections and excessive allocations
  fail with the original occurrence provenance. The aligned snapshot contains no scene registry.
- ClusterBounds reconstructs world-space cell bounds from the supplied inverse projection.
  It intersects unprojected rays with camera-depth planes instead of assuming an infinite
  perspective depth formula. Finite/infinite perspective, asymmetric projections,
  orthographic cameras and partial edge tiles share the same operation.
- ClusterListClear initializes private fixed-offset uint2 grid entries, every invalid index
  and the overflow flag. Dispatch limits are checked, with a flattened two-dimensional
  dispatch when the device's X limit requires it.
- ClusterLightAssignment consumes explicit canonical lights, bounds and initialized lists.
  A conservative homogeneous projection interval limits candidate tiles; world-space sphere
  tests reject unrelated clusters. Atomic reservations saturate at the reflected capacity.
  An explicit overflow output reports dropped assignments. Directional and zero-intensity
  records do not enter local lists. Parallel slot order is not promised to be deterministic;
  compilation and authored expansion remain deterministic.
- ClusterLightSampling preserves the raw list and writes a separate packed 20-bit light
  index / 12-bit emitted-power CDF plus per-cluster totals. It preserves the existing
  photometric emitter measures for analytical light types. It owns no light lifecycle,
  GI history or surface shading. Sampling is independently omittable.

All pins are concrete XII_MEMBER_PROPERTY members. Properties and pin attributes use the
existing editor/reflection system. Each GPU operation captures owned constants, resources
and pipeline objects, with no execution-time pass, camera or world pointer. The compiler
can cull unused outputs/operations; no new stage selector or renderer manager is added.

The canonical xiiGpuLightData record, its RTTI and xiiGpuLightType now live in LightingData.h
and Implementation/LightingData.cpp. Local-shadow planning consumes that interface directly
instead of depending on the oversized lighting context. Record size, reflected type name
and properties are preserved; no compatibility alias retains the old nested light-type API.
The shared shader light-data header's shadow-fade helper also takes its camera position
explicitly, and every surface/volume caller is migrated.

ClusterGridBuild.xiiShader, LightListClear.xiiShader, LightListBuild.xiiShader,
LightSamplingBuild.xiiShader and LightClusteringConstants.h are physically deleted. Their
resource/global-constant assumptions are replaced by the new kernels and explicit pins.
The old lighting context itself is not yet deleted: world light packing/upload, IES residency,
local shadow and GI update consumers still need native ownership and operation migration.
This phase does not claim a completed world-light extraction path or forward/deferred renderer.

Six ordinary editable source/binary asset pairs are exported through normal editor commands:
LightClusterView, ClusterBounds, ClusterListClear, ClusterLightAssignment, ClusterLightSampling
and the five-operation ClusterLightLists composition. Inputs and outputs use existing graph
boundary mappings and persistent document identities. Total inventory is now 83 concrete
native pass classes and 48 asset pairs.


Verification, Debug/Vulkan; D3D12 excluded:

- GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build with dependencies
  (native-cluster-consumer-build.log, -sampling-build.log, -final-build.log,
  -final-fixture-build.log, -large-fixture-build.log and -projection-build.log).
  Initial builds corrected the existing AreAllSet API spelling and FrameData pin-kind name.
- All 66 CPU tests pass (native-cluster-projection-cpu.log). All 42 NativeAssetAuthoring
  fixtures and 16 EditorDocuments tests pass, including deterministic packaged/source/repeated
  exports for all 48 assets (native-cluster-final-authoring.log and -editor.log).
- All 23 native GPU subtests pass (native-cluster-final-native.log). The new cluster fixture
  reads actual buffers for two occurrences in each of two independent roots, with different
  capacities/resolutions, translated orthographic and asymmetric finite/infinite perspective
  projections, linear/log depth, edge tiles, empty input, parallel multi-group assignment,
  explicit overflow, private allocations, raw/packed indices, emitted power and monotonic CDFs.
  It destroys all pass instances before final recording. Negative ranges, excessive budgets
  and projection horizons report nested provenance and clear stale outputs.
- The large-list fixture initializes 4,196,352 grid/index entries and checks invalid indices,
  offsets and the cleared overflow flag after destroying its pass owner. Its observed device
  X-group limit is 2,147,483,647, so this run uses a one-dimensional dispatch; the helper's
  two-dimensional fallback is implemented but not hardware-exercised by this device.
- All 14 PipelineResources tests, three PipelineExtraction tests and NativeDepthBoundary pass
  (native-cluster-resources-regressions.log, -extraction-regressions.log and
  -depth-boundary-regressions.log). This includes world component updates, sparse multi-camera
  uploads, ray deformation and interleaved temporal resources.
- The full sample completes all 300 frames with exit 0 (native-cluster-sample-results.json
  and .log), including the migrated shared surface-lighting helper. The sample still uses
  directional shading rather than claiming a complete clustered forward/deferred pipeline.

The GPU fixtures exposed an aligned-allocation requirement in the immutable shader-layout
snapshot, a reserved HLSL identifier, an implicit camera-global dependency in the shared light
header and the editor's zero vector default. Each production issue is corrected. The CDF
reference allows one 12-bit step for FP32 halfway rounding, while requiring monotonicity,
correct raw indices, analytic emitted-power totals and an exact terminal CDF.

Each repeated cluster root records ten compiled operations, one submission and 46 barriers.
The CPU-only layout nodes do not add GPU operations. The large-list test takes about 0.17 s
including device setup and readback; this is not a GPU timing or Release throughput benchmark.
The full sample retains 124 registered / 116 compiled / 8 culled operations, one submission
and 190 barriers. Its topology does not yet include the new cluster composition.

World light packing/upload, IES/probe ownership, deletion of LightingSystem/lighting contexts,
complete application/editor/advanced lighting graphs, per-page sample orchestration and
world/view virtual page identities remain. The full renderer migration remains active.


## World-owned light uploads and legacy lighting-context deletion

LightingSystem.h and Implementation/LightingSystem.cpp are physically deleted.
xiiLightingSystem, xiiLightingSystemSettings, its unused quality preset, xiiLightingContext,
xiiLightingContextHandle/Id and the context ID table/factory/lookup/teardown APIs no longer
exist. The large light/camera/global upload facade, bind helpers, global name publication,
automatic local-shadow planning and eager DDGI/voxel ownership are removed. LightingManager
retains only the distinct demand-allocated immutable device BRDF lookup and its readiness.
It owns no camera, world, light array, hierarchy, context registry or GI state.

The existing DDGI and sparse-voxel update helpers now require an explicit graph uniform
buffer for light constants and declare that read. They no longer borrow a LightingSystem
pointer or bind arbitrary global frame resources. Their algorithmic contexts remain;
complete native update passes and advanced-lighting graph integration are still required.
The old settings were not referenced by current assets or production callers; no permanent
compatibility object/factory is retained to keep deleted renderer orchestration alive.

WorldLightsUpload is an independent reflected producer with Lights, IESProfiles, Counts
and Candidates typed member pins. It requests only the existing Lights extraction category
and Lighting GPU-data feature. No mesh/material residency, GI, shadow atlas or device BRDF
lookup is initialized merely because lights are consumed. Its ordinary editable asset is
exported through the editor document path. The library now contains 84 native pass classes
and 49 asset pairs.

xiiRenderWorldModule aggregates the existing view-independent component packets from views
that consume GPU lights and updates a focused WorldLightData owner once per world frame.
All seven analytical light kinds preserve canonical photometric intensities, temperature
colors, directions, influence bounds, source dimensions and shadow properties. Point/spot
IES references use the existing resource manager, are deduplicated within the world upload,
and are re-read at extraction so source reloads update GPU contents and content revisions.
Missing/invalid requested profiles report failure rather than silently changing the light.

WorldLightData contains only derived shader records, validated source associations and
completion-safe GPU upload slots. It offers no create/destroy/reparent/transform authoring
API, stores no object hierarchy, and introduces no entity or general handle framework.
Internal monotonically allocated light GPU tokens never alias a retired token; resolution
validates the authoritative component generation, world and activation. They are runtime
identities, not document/serialized handles. Compact table indices remain derived bindings.

Camera graphs share the world's immutable CPU upload snapshot and physical light/profile
buffers. Changed contiguous records become sparse upload ranges. Unchanged contents retain
the prior snapshot; queued work retains snapshot/state/resources independently of the world.
Writable slots are reused only within the same immutable frame or after their last frame is
complete. The uploaded version is promoted while recording, not during setup. Every view
gets a separate explicit candidate stream honoring its extracted tags/activation; consumers
must apply this stream instead of treating the world's union as that camera's selection.
The clustered consumer connection is implemented in the following phase; full surface-lighting
assets still need migration.

The cache is cleared when the world drops GPU-light demand, on world teardown and on device
release. This extends the existing world module and shared extraction lifecycle; no new
world module or scene ownership abstraction is added. CPU light packing still processes the
eligible light packets once per world update. Incremental GPU bytes and shared extraction
are implemented; a full per-property CPU packing-revision optimization is not claimed.
IES resources share resource-manager ownership, but the packed GPU profile table remains
world-scoped; shared device-level IES residency across different worlds is still outstanding.

Verification for this phase:

- Debug GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene built with serial MSBuild.
- 66 GraphicsCore CPU tests, 43 native asset-authoring fixtures and 16 editor document tests passed.
- Vulkan passed 25 native GPU tests, 14 pipeline resource tests, three extraction tests,
  the interleaved depth-boundary test, the failed-recording history test and three temporal
  memory tests. The two new world-light tests cover all seven light types, authoritative
  parent transforms, tagged views, activation/removal/recreation, validated GPU identities,
  IES hot reload, empty demand, an eight-frame burst without host waits and world/pipeline
  destruction before queued graph recording.
- Two views share the canonical physical buffers. A changed point transform uploads one
  144-byte record in one range/pass; warmed unchanged frames upload zero shared bytes.
  The seven-light initial snapshot including its IES profile uploads 9,216 bytes; profile
  reload uploads 8,480 bytes including affected record revisions. These are measured upload
  byte counts, not GPU timings or a Release throughput benchmark.
- The full Vulkan sample completed 300 frames with exit code zero after the legacy
  lighting-system deletion. D3D12 was excluded as requested.
- Source and asset searches found no remaining lighting-system/context production references.
  The former files and static-link registration are deleted. The full renderer migration
  remains active, including the explicit remaining work listed above.

## Per-view light selection through reusable cluster graphs

ClusterLightAssignment now exposes an optional Candidates member pin through RTTI.
ClusterLightLists maps that external interface to the independent assignment operation;
both editable source assets and exported resources were regenerated through editor documents.
No pass stage selector, manual pin-type array or fixed renderer stack was introduced.

When connected, assignment dispatches the exact structured uint candidate stream and writes
canonical world-table indices into the cluster lists. The shader checks each selected index
against the physical light capacity before reading it; the empty-selection sentinel is safe.
When unconnected, the generic algorithm still accepts the entire explicit light stream. A
private typed sentinel binding satisfies the shader layout in that mode; it is graph-owned,
declared through a transfer dependency and never used as a world-light selection.
The camera-specific lists and downstream sampling tables remain private graph allocations.

The world-light Vulkan regression now composes the ordinary WorldLightsUpload and
ClusterLightLists definitions, connecting both Lights and Candidates. It reads back the
actual cluster grid and canonical indices across all ten component/profile phases. The
excluded point light appears in the all-light camera and never in the tagged camera;
deactivated/empty selections yield no corresponding entries and no overflow. The existing
repeated-instance clustering fixtures exercise the unconnected generic input as well.

Validation passed 43 editor authoring fixtures, 16 editor document tests including deterministic
resource bytes, 66 GraphicsCore CPU tests (including 25 pipeline tests), 14 resource GPU tests,
three extraction GPU tests and all 25 native Vulkan GPU tests. Only Vulkan is built
in this checkout; D3D12 remains excluded. The library still has 84 concrete native passes and
49 asset pairs. Device-shared IES GPU residency, complete surface/advanced lighting graphs,
remaining legacy algorithm contexts and the other application/editor integration work are
still outstanding; the full rendering-stack migration is not complete.

## Focused environment illumination and directional-only extraction

WorldDirectionalLight is a reflected CPU snapshot producer. It selects the brightest
eligible directional emitter from this camera's existing extracted packets, breaking ties
by the full validated runtime component handle. Transform direction, photometric lux,
authored color temperature and angular source radius come from the authoritative component.
Inactive, removed, foreign-world or owner-mismatched components cannot supply the snapshot.
No emitter is represented by a new scene object or rendering-side hierarchy.

EnvironmentLightConstants is a separate reflected operation. Its optional typed
DirectionalLight input replaces the authored emitter, including an explicitly empty world
selection. Without that input, reflected Direction, Color, Illuminance and AngularRadius
properties provide an independently authored environment. AmbientRadiance belongs to this
operation. The immutable 48-byte result contains only those environment fields; no cluster,
contact-shadow, atlas, fog or ReSTIR configuration is bundled into it.

The world source requests existing Lights extraction without the Lighting GPU-table feature.
It does not initialize canonical GPU light tables or acquire unrelated point/spot IES
resources. The constants-only operation requires no scene extraction or lighting service.
The canonical color-temperature conversion is shared with WorldLightData through the existing
LightingData implementation, preserving its previous numeric response without another owner.

DDGI gather and the existing DDGI/sparse-voxel update helpers now require this explicit focused
environment buffer. Their shaders no longer consume PerFrameLight. DDGI gather retains its
pipeline and sampler in queued graph data instead of borrowing the pass object. Sparse-voxel
gather now retains those resources as well. The GI update
helpers still need decomposition into native scheduling/upload/update operations; this phase
does not present their algorithm contexts as the completed native GI pipeline. Other legacy
shaders still use the old frame-light ABI and must be migrated with their own consumers.

Verification for this phase:

- All five Debug consumers built: GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene. The first focused GPU run rejected two
  fixture mistakes (duplicate node names and a shader-resource exporter used for a uniform
  buffer). The corrected fixture uses unique names and an explicit uniform readback contract.
- 67 GraphicsCore CPU tests passed, including reflected environment member pins, property
  round trips, required uniform access and invalid emitter configuration.
- 45 native asset authoring fixtures and 16 editor document tests passed, including deterministic
  re-export of the two new source/binary assets. The library has 86 native classes and 51 asset pairs.
- All 27 native Vulkan GPU tests, 14 resource GPU tests and three extraction GPU tests passed.
  The new fixtures read back independently configured repeated environments after instance
  destruction; another composes WorldDirectionalLight and EnvironmentLightConstants for two
  tagged views across ties, brightness changes, world rotations, activation, deletion and
  component replacement. An unrelated missing point-light IES profile is never acquired.
  No canonical world GPU light table, GPU instances, geometry or material service is initialized.
- The irradiance regression also compiles and records a final graph after both repeated
  DDGI/sparse gather instances and their view are destroyed, then checks all four GPU images.
- ShaderCompilerTool compiled DDGIProbeUpdate and SparseVoxelRadianceUpdate for VK_SM60,
  reflecting xiiEnvironmentLightConstants in both. Their existing algorithm-context GPU
  update paths were not dispatched in this phase; their native replacements remain required.
- The constants payload is 48 bytes versus the former 128-byte all-feature light ABI. This is
  a layout/upload-size comparison, not a GPU timing or Release throughput benchmark. World
  selection is linear in the already-extracted camera light packets and adds no GPU operation.
  Only Vulkan is built here; D3D12 remains excluded.
- The full Vulkan sample completed 300 frames with exit code zero, retaining 124 registered /
  116 compiled / eight culled operations, one submission and 190 barriers. This sample does
  not yet compose the new environment/GI preparation; its run is a general integration
  regression, while the focused fixtures exercise the new producers and migrated gather.

Native DDGI/sparse scheduling and update decomposition, complete surface/advanced lighting
assets, device-shared IES GPU residency and the remaining application/editor integration
are still required. This phase does not complete the full rendering-stack migration.

## Independent DDGI probe scheduling, upload, preparation and environment seeding

The previous DDGI context GPU path is deleted: Initialize no longer takes a device,
CreateGpuResources and AddUpdatePass no longer exist, and the context owns no device,
shader pipeline or GPU atlas. The old root DDGIProbeUpdate shader and five unused DDGI
blackboard keys are removed. The retained context is a CPU-only toroidal probe scheduler;
its existing scrolling, prioritization, update-budget and relocation API remains tested.

Four independent reflected operations replace the orchestration:

- DDGIProbeSchedule owns one CPU scheduler per occurrence. It snapshots camera or explicit
  position, dimensions, spacing, budget and hysteresis. Explicit previous atlas inputs
  determine whether CPU validity/history can be retained. No history invalidates all probe
  eligibility before scheduling. The published snapshot and GPU-record arrays use XII's
  aligned allocator; queued consumers retain them independently of the schedule pass.
- DDGIProbeUpload creates graph-owned state/update buffers and constants from that snapshot.
  It uploads the existing 32-byte state and 32-byte update records; there is no global
  registry, GPU object lifecycle, scene transform authority or context-owned buffer table.
- DDGIAtlasPrepare creates private current irradiance/distance atlases. It clears a newly
  invalidated pair or copies the explicitly supplied history pair. Atlas storage belongs
  to the graph; its dimensions and device limits are validated before allocation.
- DDGIEnvironmentSeed applies the selected analytic sky/sun algorithm to the budgeted
  slots. This preserves the old non-RT environment seed and is explicitly authored, never
  a substitute silently selected for a missing ray-tracing pipeline. GPU validity is
  published only for updated slots. CPU scheduling feedback is committed during recording
  through retained snapshot state, with a frame guard against stale feedback.

DDGIEnvironmentProbes is an ordinary editable composition of those four operations and
two existing TextureHistory nodes. Temporal edges explicitly feed previous atlases to the
scheduler and preparation operation; current outputs feed history retention. Definitions
and compiled metadata are shared, while CPU schedules, mutable resources, history rings
and readiness fences remain private to each occurrence and execution instance.
Four leaf assets expose each operation independently. The library now has 90 native
operations and 56 asset pairs.

TextureHistory gains a reflected AsyncCopy property. Its default stays true, preserving
the prior transfer-queue policy for existing assets. This DDGI composition explicitly
disables it for small copies so independent probe fields can remain in one graphics batch.
DepthPyramid is re-exported with the explicit unchanged default. No pass stage selector,
manual pin-type array, centralized pipeline factory or compatibility context is added.

Irradiance remains RGBA16Float, with finite half-range saturation on writes. Distance
moments use RG32Float so ordinary larger probe spacing cannot overflow half-precision
squared distances. The two atlas planes now cost 16 rather than 12 bytes per texel.
Preparation and retention copy explicit whole atlases for safe temporal ownership; probe
state upload still processes all records, as the previous path did. These costs are
explicit; no Release throughput or GPU timing claim is made from layout sizes or tests.
The algorithm seeds an environment field; traced visibility, directional probe textures
and geometry-dependent radiance updates are not claimed as implemented by this operation.

The focused Vulkan fixtures exercise two repeated definitions in each of two independent
execution instances. They check every atlas texel (including padding), distance moments,
GPU cell/validity metadata and recorded update frames against the scheduled updates.
Coverage includes partial budgets, hysteresis with changing illumination, single-cell
scrolling, complete scrolls, explicit invalidation, dependent definition hot reload,
eight consecutive submitted frames without explicit host-idle waits, and destruction of
all pass instances before the final graph records. A separate actual-world fixture composes
WorldDirectionalLight, EnvironmentLightConstants and DDGIEnvironmentProbes for two cameras;
it changes component illuminance and camera position and verifies independent image history.
Only Lights extraction is requested; canonical world GPU light tables, mesh instances,
geometry and material services remain uninitialized.

The first GPU run caught default-allocator alignment errors for the new 16-byte-aligned
snapshot and records; aligned allocation fixes them. A clean baseline then recorded 14
compiled operations in four queue submissions per execution instance. The explicit small-copy
policy reduces that to one submission with the same 48 barriers and passing numerical/lifetime
checks. The regression asserts the one-submission cold path on the available Vulkan device.
This is observed batching evidence, not a GPU timing benchmark or a claim about every backend.

Verification for this phase:

- All five Debug consumers built: GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene. Initial compile/link mistakes in a
  pipeline pointer call and the test's use of a private helper were corrected before the
  final builds; the allocator failure described above was reproduced and fixed.
- 68 GraphicsCore CPU tests, 50 native authoring fixtures and 16 editor document tests passed.
  Source/binary export is deterministic for all 56 assets, including the refreshed DepthPyramid.
- Vulkan passed all 29 native GPU tests, 14 pipeline resource tests, three extraction tests,
  the interleaved depth-boundary fixture, the failed-recording temporal fixture and three
  temporal memory fixtures. Both new DDGI fixtures passed after the aligned-allocation fix
  and again after the copy-queue optimization.
- The full Vulkan sample completed 300 frames with exit code zero. Its steady graph remains
  124 registered / 116 compiled / eight culled operations, one submission and 190 barriers.
  It does not yet use this DDGI composition; the focused actual-world fixture exercises that path.
- Repository searches found no old DDGI update-shader path, context graph factory or deleted
  DDGI blackboard constants in Source/Data. No rendering Builtin file or scene database was
  reintroduced. Vulkan is the available built backend; D3D12 was excluded as requested.

Sparse-voxel context GPU orchestration still needs the same native decomposition. Traced
DDGI visibility/relocation and geometry-dependent radiance, complete surface/advanced
lighting assets, shared device IES residency and remaining application/editor integration
are outstanding. The full rendering-stack migration remains active.

## Independent sparse-voxel radiance preparation and environment seeding

SparseVoxelRadianceContext is now CPU-only clipmap residency, brick-key validation,
eviction, refresh scheduling and commit feedback. Its device-taking initialization,
CreateGpuResources, AddUpdatePass, persistent GPU pool and shader pipeline are deleted.
The root SparseVoxelRadianceUpdate shader and four unused storage blackboard keys are
removed. Physical brick slots remain internal to this derived radiance algorithm; they
are not world objects, public scene manipulation indices or authoritative transforms.

Four reflected operations expose the former coherent boundaries:

- SparseVoxelSchedule snapshots the existing CPU scheduler, clipmap levels, valid pages
  and budgeted updates into owned aligned data. The optional PreviousPool input controls
  whether old CPU validity may be retained. Resource limits include an authored byte budget
  to reject impractical pool configurations before allocation.
- SparseVoxelUpload publishes the page table, level transforms, update records and constants
  into graph-owned buffers. Pages initially expose only valid prior bricks. Empty update
  lists use a typed minimum allocation; their logical count remains zero.
- SparseVoxelPoolPrepare creates a private current pool. An explicit history input is copied
  by a bounded compute dispatch, or the pool is zeroed. A separate typed dummy input avoids
  binding the same buffer simultaneously as an SRV and UAV on the first frame.
- SparseVoxelEnvironmentSeed writes the selected analytic environment field only into scheduled
  brick slots, applies per-brick temporal weight and publishes their current pages. The
  packed CPU keys decode into checked level/cell/page indices in the shader. Updated pages
  and radiance carry graph write dependencies, so gather cannot observe a published page
  before its corresponding dispatch completes. Recycled slots have zero history weight.
  Recording commits CPU feedback through retained, frame-guarded scheduling state.

SparseVoxelEnvironment is an ordinary editable composition of these operations and the
existing BufferHistory pass. The history edge is temporal and explicit. Definitions share
compiled metadata while each occurrence/view owns private residency state, writable buffers
and completion-safe history. Four independent leaf assets expose the individual operations.
BufferHistory now has the same reflected AsyncCopy choice as TextureHistory; this composition
selects graphics copies to avoid small transfer batches. Existing visibility assets retain
their default transfer policy and are re-exported with its explicit property.

This preserves the former isotropic sky/sun environment seed. Geometry voxelization,
directional radiance lobes and traced visibility are not implemented by this operation and
are not silently substituted by it. Current pool preparation and history retention copy
whole pools for safe immutable snapshots; page/level uploads remain whole tables as before.
No incremental GPU-pool-copy or Release throughput claim is made. The byte budget is an
explicit authoring constraint, not evidence of device allocation success or performance.

The Vulkan regression checks every radiance-pool element and every current page/level
record for two repeated definitions in each of two independent execution instances.
It covers partial update budgets, idle frames with zero updates, refresh with temporal
weights, one-cell and complete scrolls, reuse of physical slots, explicit invalidation,
dependent resource reload, eight consecutive submitted frames without explicit host-idle
waits and destruction of all pass instances before final recording. Page expectations
combine retained valid mappings and exactly the newly dispatched updates; unscheduled
new bricks remain inaccessible to gather. Pool bytes use canonical float4 storage.

A separate world fixture composes WorldDirectionalLight, EnvironmentLightConstants and
SparseVoxelEnvironment for two cameras. Component illuminance changes and a camera move
update the derived field while histories and physical pools remain independent. The
graph requests Lights packets only, leaving world GPU light tables, GPU mesh instances,
geometry residency and material services uninitialized. No separate scene lifecycle is added.

The focused fixture records 16 compiled operations in one queue submission with 42 barriers
per execution instance on the available Vulkan device. That is measured batching evidence,
not a GPU timing or Release throughput benchmark. The library now has 94 native operations
and 61 source/binary asset pairs. The old context no longer provides a parallel GPU path.

Verification for this phase:

- All five Debug consumers built: GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene. Initial source errors were an ambiguous
  const array view and a local settings name conflicting with a result macro; both were fixed
  before the final consumer build. Both focused Vulkan fixtures passed on their first run.
- 69 GraphicsCore CPU tests, 55 native asset authoring fixtures and 16 editor document tests
  passed. All 61 assets re-export deterministically, including the two visibility graphs
  refreshed with the unchanged default BufferHistory copy policy.
- Vulkan passed 31 native GPU tests, 14 pipeline resource tests, three extraction tests,
  the interleaved depth-boundary fixture, the failed-recording temporal fixture and three
  temporal memory fixtures. The new tests explicitly observe zero-update frames and
  zero-history writes into reused physical slots rather than assuming those cases occurred.
- The full Vulkan sample completed 300 frames with exit code zero; the steady graph retains
  124 registered / 116 compiled / eight culled operations, one submission and 190 barriers.
  This sample does not yet use the sparse composition; the focused actual-world test exercises it.
- Searches found no old sparse-update shader path, context GPU factory or deleted storage
  blackboard keys in Source/Data. No rendering Builtin file, legacy lighting context or scene
  database was reintroduced. Vulkan is the available built backend; D3D12 remains excluded.

Both GI context GPU orchestration paths have now been removed. Geometry-dependent radiance
updates, complete surface/advanced lighting graphs, shared device IES residency and remaining
application/editor integration are still outstanding. The full renderer migration remains active.

## Independent surface-light evaluation from explicit world resources

DirectionalSurfaceLighting evaluates the selected environment emitter against explicit
depth, normal, albedo, material, emissive/transmission and camera inputs. Its optional
Visibility input is a scalar visibility factor (one unoccluded, zero occluded); it is not
an implicit choice of shadow algorithm. Omitting it leaves the emitter unoccluded.
Scale and AsyncCompute are reflected member properties. This operation does not initialize
world GPU light tables, IES, clusters, mesh residency, GI or shadow services.

ClusteredLocalSurfaceLighting independently evaluates canonical analytical local lights.
It consumes explicit Lights, Counts, Candidates, IESProfiles, cluster Layout, LightGrid,
raw LightIndices and Overflow inputs. Normal operation uses the cluster's current raw list.
An overflow flag selects the entire supplied view candidate set for correctness, preserving
local-light energy without silently truncating it to the authored list capacity. Candidate,
canonical-record, cluster-list and IES accesses are bounds checked. Empty selections remain safe.
The existing finite-area emitter and spot/profile evaluation math is reused. No resource is
discovered through a global execution-time name registry.

The BRDF and local-light math are factored out of SurfaceLighting into shared headers.
Existing remaining consumers include the shared helpers; the new operations do not include
the monolithic PerFrameLight ABI or ReSTIR/shadow orchestration. The unused root DirectLighting
shader is physically deleted. Its remaining test consumer was an obsolete failure fixture
that referenced the already-deleted Deferred asset; that fixture now explicitly authors
native inputs, a single operation and presentation, and tests missing-shader recovery.

WorldSurfaceLighting is an ordinary editable composition. It references the existing world
light upload, directional selection, environment constants and cluster definitions, then
connects the two native surface evaluators. Surface/camera inputs are explicit boundaries;
directional and local contributions remain separate outputs so the caller controls composition.
This asset shades one selected directional environment emitter plus the eligible local lights.
It does not claim complete deferred/forward rendering, all directional emitters, local shadows,
ReSTIR or emissive-triangle integration. Their remaining algorithm resources are not deleted
as a shortcut to claiming the complete renderer has migrated.

The operations retain pipelines and immutable setup data in queued graph records, without
borrowing pass objects or live world component storage. Shared definitions do not share
writable output images. Validation requires matching single-sample 2D surface extents,
float-readable channel layouts, exact camera/environment buffer sizes and cluster/light
tables matching their compiled schemas. Output is nonnegative RGBA16Float with finite
half-range saturation; black/invalid depth contributes no radiance. Only the producer nodes
request extraction or world-scoped GPU light data. The two evaluator leaf assets can also
consume independently supplied resources without world extraction.

Surface-lighting verification - 2026-10-18:

- Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene, including their editor/process
  dependencies (`Workspace/native-surface-validation-build.log`). The corrected focused
  fixture also builds (`native-surface-photometric-fixture-build.log`).
- All 70 GraphicsCore CPU subtests, 58 asset-authoring subtests and 16 EditorDocuments
  subtests pass. The library fixture opens all 64 editable assets, instantiates Qt nodes
  and pins, and compares packaged, exported and repeated-export bytes for determinism
  (`native-surface-cpu-final.log`, `native-surface-authoring-final.log`,
  `native-surface-editor-documents.log`). The new assets are DirectionalSurfaceLighting,
  ClusteredLocalSurfaceLighting and WorldSurfaceLighting.
- Vulkan NativeSurfaceLighting checks ten world-change phases in two views, with
  repeated directional evaluator instances and nested cluster-property overrides.
  GPU readback checks point-light intensity and component effective-range attenuation,
  Type-C photometric profile sampling, component deactivation/reactivation, view-tag
  exclusion, empty selections/depth, transmission, independent scale/visibility inputs
  and the complete-candidate fallback when cluster capacity overflows. It then retires
  both views and the detached pass instance before recording its already-built graph
  (`native-surface-vulkan-passed.log`). No mesh/material service or GPU instance table
  is initialized by this world-light-only composition.
- All 32 native Vulkan subtests, 14 PipelineResources and three PipelineExtraction
  subtests pass (`native-surface-regression-Native.log`,
  `native-surface-regression-PipelineResources.log`,
  `native-surface-regression-PipelineExtraction.log`). The three migrated shader failure/
  recovery tests, one temporal-history test, three temporal-memory tests and one native
  depth-boundary test also pass (`native-surface-failure-recovery.log` and
  `native-surface-editor-*.log`). An initial regression script selected the latter groups
  in the wrong executable and ran no matching tests; they were rerun in
  EditorEngineProcessFrameworkTest. No empty filter result is counted as passing evidence.
- Initial fixture failures were corrected: SDK dependency mounting during authoring,
  nested-node lookup by definition identity, redundant differently rooted references,
  and analytical expectations that ignored the component's photometric range cap.
  A null-pipeline guard also prevents a secondary test crash from hiding compile errors.
- The full Vulkan sample completes 300 frames with exit code zero. Its steady graph
  remains 124 registered / 116 compiled / eight culled operations, one submission and
  190 barriers (`native-surface-sample.log` and `native-surface-sample-results.json`).
  This sample still uses its existing graph; the dedicated world fixture exercises the
  new surface composition. These checks are correctness evidence, not a controlled
  performance benchmark. Overflow evaluation intentionally costs O(view candidates).
- Repository searches find no active SceneDatabase/SceneDatabaseManager or root
  DirectLighting shader references and no rendering Builtin files. D3D12 is excluded;
  Vulkan is the available built backend. Spot/finite-area evaluator branches reuse the
  existing math but have not received new numerical GPU coverage in this phase.

Complete forward/deferred assets, remaining advanced lighting and geometry-dependent
GI updates, device-shared IES residency and application/editor integration remain open.
The full rendering migration is still active; these two operations bring the source
inventory to 96 concrete reflected passes and 64 editable source/binary asset pairs.


## Independent radiance combination and fog composition

The fixed OpaqueLightingComposite C++ class/header and OpaqueComposite shader are
physically deleted. Their five-input stack formula is now five authored operations
in SurfaceRadianceComposition: three RadianceAdd occurrences, ConfidenceRadiance and
VolumetricComposite. OpaqueComposition references that ordinary resource definition.
Its remaining reflection trace/filter/selection nodes are retained. No operation
creates another operation or chooses a renderer stage.

RadianceAdd has typed A/B/Radiance member pins and reflected finite nonnegative
ScaleA, ScaleB and AsyncCompute properties. It adds nonnegative supplied radiance
without treating alpha as an implicit mask. ConfidenceRadiance applies the supplied
signal alpha as saturated confidence. VolumetricComposite consumes scene radiance
and integrated scattering/opacity: scene * (1 - opacity) + scattering. Both confidence
and opacity saturate to their physical range. Invalid computed radiance is black;
outputs have alpha one. Required inputs are nonempty, matching, single-sample,
float-readable RGBA 2D images. These nodes have no world extraction or runtime scene
service requirements, no global resource lookup and no pass-object recording capture.

Addition and confidence outputs are RGBA32Float. This preserves intermediate sums
above 65,504 before fog attenuation. The final fog result is saturated RGBA16Float.
The added intermediate bandwidth/allocation is a real tradeoff; no automatic shader
fusion or speedup is claimed. Each operation remains separately authorable, and the
compiler schedules the logical graph through ordinary resource dependencies.

OpaqueComposition also loses its unused PlanarReflectionMap input/output, selection
and clear nodes, plus the five redundant input-echo output boundaries from the removed
fixed pass. The unused planar blackboard key is deleted. No compatibility-forwarding
nodes or replacement planar renderer were added. The replacement subgraph has a fresh
persistent document identity and explicitly remapped connections. Ordinary Before/After
boundary mappings preserve authored ordering through its independent operations.

Four new editable source/binary asset pairs expose RadianceAdd, ConfidenceRadiance,
VolumetricComposite and SurfaceRadianceComposition. The OpaqueComposition pair is
updated through the editor document commands and production compiler. The one-time
mutation helper was removed from source after exporting; the permanent authoring and
asset-library regressions exercise only the final native graph model.

Radiance-composition verification - 2026-10-18:

- Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene after source-list regeneration
  and physical legacy deletion (`Workspace/native-composition-deletion-configure.log`,
  `native-composition-final-build.log`). The active GraphicsCore project contains no
  removed source reference.
- All 71 GraphicsCore CPU subtests pass (`native-composition-cpu-final.log`). Coverage
  checks reflected weights, invalid configuration, typed output formats, absence of
  extraction/service requirements, removed planar/echo boundaries and two nested
  OpaqueComposition instances expanding to 24 native operations with distinct provenance.
- Vulkan NativeRadianceComposition covers two instances with independent ScaleA/ScaleB,
  confidence zero/out-of-range, fog opacity limits, 9-by-11 partial dispatch tiles,
  mismatched-extent rejection and retirement of every pass instance before recording.
  Analytical readback checks all output pixels, including a 140,000-radiance intermediate
  attenuated before conversion to half precision. The two-instance fixture records
  17 logical operations in one submission with 36 barriers
  (`native-composition-final-Native.log`).
- All 33 native Vulkan subtests, 14 PipelineResources and three PipelineExtraction
  subtests pass. Existing opaque/reflection output values and temporal resize behavior
  remain covered by the migrated resource fixture. Three shader failure/recovery,
  one temporal-history, three temporal-memory and one depth-boundary subtest pass
  (`native-composition-final-*.log`).
- All 16 EditorDocuments subtests pass, including deterministic packaged/exported/
  repeated bytes and Qt reconstruction for all 68 asset pairs. The 58 established
  native-authoring tests and four independent composition-authoring tests also pass
  (`native-composition-final-EditorDocuments.log`,
  `native-composition-final-NativeAssetAuthoring.log`,
  `native-composition-final-NativeCompositionAuthoring.log`).
- Initial fixture compile errors and a one-time asset-edit failure were fixed before
  final validation. The asset edit initially reused a removed node UUID while undo
  history retained the old object; the final asset uses a fresh persistent identity
  and remaps incident connections explicitly. No document lifetime guarantee was changed.
- The full Vulkan sample completes 300 frames with exit code zero, no assertion log,
  and its steady 124 registered / 116 compiled / eight culled operations in one
  submission with 190 barriers (`native-composition-sample.log` and
  `native-composition-sample-results.json`). Its existing sample graph is unchanged;
  the dedicated native fixture exercises the newly authored composition.
- Active Source/Data references to the removed opaque class, shader and planar key
  are absent. Vulkan is the exercised backend; D3D12 remains excluded. There is no new
  scene ownership or rendering manager. No controlled performance benchmark was run;
  RGBA32Float intermediates cost additional bandwidth and memory, and this phase does
  not implement automatic shader fusion.

The source inventory is 98 concrete reflected operation classes with 68 editable
source/binary asset pairs. Complete scene graphs, advanced/indirect illumination,
remaining composite granularity and application/editor rendering integration remain
unfinished. The complete renderer migration is still active.


## Independent indirect surface evaluation

Three native operations replace the mathematical responsibilities of the unused
IndirectLighting shader entry point, which is physically deleted. IrradianceLayer
combines coarse and detail incident-irradiance signals with confidence-aware layering.
DiffuseSurfaceLighting evaluates Lambertian material response. EnvironmentSpecular
evaluates supplied, already filtered environment radiance through a supplied BRDF lookup.
Neither surface evaluator performs probe selection, capture, GI scheduling or tracing.
The existing low-level reflection-probe sampling helper remains used by RTGI and
reflection shaders; native probe preparation/selection still requires implementation.

IrradianceLayer preserves a straight irradiance/confidence signal. For detail confidence
d and coarse confidence c, the weighted irradiance is detail*d + coarse*c*(1-d),
confidence is d + c*(1-d), and the stored RGB is weighted/confidence when confidence is
nonzero. Both confidences saturate to [0,1]; negative irradiance contributes zero.
This permits nested layers and applies the ambient baseline exactly once during
surface evaluation, rather than multiplying confidence repeatedly at each graph edge.

DiffuseSurfaceLighting consumes depth, albedo/authored occlusion, material and emission/
transmission data, plus explicit 48-byte environment constants. Irradiance and dynamic
occlusion are optional. It has no camera or normal pin and requests no GI/light/mesh
service. Its ambient fallback follows the existing indirect-lighting coefficient
convention: (ambient*(1-confidence) + weighted irradiance) * albedo / pi, modulated by
nonmetallic fraction, opaque fraction, authored/dynamic occlusion and reflected Scale.
This operation does not implement physical sky convolution.

EnvironmentSpecular consumes depth, normal, albedo/material, exact camera constants,
filtered radiance and a two-channel BRDF lookup. It has no emission/transmission or
environment-constant requirement. Roughness-aware Schlick Fresnel and the split-sum
lookup reproduce the existing material response. Optional dynamic occlusion combines
with authored albedo occlusion. Scale and AsyncCompute are reflected member properties.
All outputs are finite nonnegative RGBA32Float radiance (alpha one), except IrradianceLayer
whose alpha remains the combined confidence. No new world or device manager is created.

Surface geometry decoding is shared without forcing transmission inputs onto specular
operations. Queued records retain immutable setup data, pipelines and samplers rather
than pass objects or component storage. Resource validation checks nonempty compatible
single-sample 2D images, readable channel layouts and exact uniform layouts. The BRDF
lookup has its own extent and uses linear clamped sampling.

Five editable source/binary pairs expose IrradianceLayer, DiffuseSurfaceLighting,
EnvironmentSpecular, LayeredDiffuseLighting and SurfaceIndirectLighting. The layered
diffuse definition wires confidence layering into diffuse response. SurfaceIndirectLighting
references that definition and the specular leaf, then combines their outputs using
RadianceAdd. Its diffuse, specular and combined outputs remain explicit boundaries.
The editor fixtures reuse the existing offline document/converter helpers, now shared
in PipelineAuthoringTestUtils.h; there is no production C++ graph factory.

Indirect-surface verification - 2026-10-18:

- Debug builds pass for GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene after final source-list regeneration
  (`Workspace/native-indirect-final-configure.log`, `native-indirect-final-build.log`).
  The subsequent shadow-readback fixture correction also builds
  (`native-indirect-shadow-readback-build.log`).
- All 73 GraphicsCore CPU subtests pass. New coverage checks reflected properties,
  missing camera/normal prerequisites on diffuse, missing emission/environment-constant
  prerequisites on specular, optional inputs, float output contracts and two nested
  SurfaceIndirectLighting occurrences expanding to eight native operations with distinct
  provenance (`native-indirect-cpu-final.log`).
- NativeIndirectLighting executes two independent camera snapshots in separate graph
  contexts, each containing two instances with nested diffuse/specular Scale overrides.
  GPU readback checks every pixel of 9-by-11 outputs, confidence normalization/limits,
  ambient-only evaluation without irradiance or dynamic occlusion, metallic response,
  transmission, authored/dynamic occlusion, invalid depth, negative radiance and high
  irradiance that exceeds half precision. An analytic 4-by-4 BRDF lookup checks UV
  sampling across the image and camera changes. Invalid extents reject safely and all
  pass instances retire before queued recording. Each context records 29 logical
  operations in one submission with 49 barriers (`native-indirect-vulkan-final.log`).
  Filtered BRDF comparisons allow 5e-4 absolute error for finite texture interpolation
  precision; nonfiltered comparisons retain 1e-4 with relative tolerance for large values.
- The first full native batch exposed an existing test readback race in
  NativeShadowTables/FrozenViewsIncrementalUploadsAndRetirement. Odd phases produce
  capture buffers on compute, but their staging readback was submitted on graphics
  without a producer completion dependency. The fixture now waits for the compute
  queue's submitted fence after both view graphs are submitted. It preserves the
  concurrent-view test and changes no production retirement behavior. All 34 native
  Vulkan subtests then pass twice (`native-indirect-native-corrected.log` and
  `native-indirect-native-repeat.log`). The original failed batch remains in
  `native-indirect-final-Native.log`.
- All 14 PipelineResources, three PipelineExtraction, three shader failure/recovery,
  one temporal-history, three temporal-memory and one native depth-boundary subtest pass.
  All 16 EditorDocuments subtests pass, including Qt node/pin reconstruction and
  deterministic packaged/exported/repeated bytes for all 73 asset pairs. The 58 established
  native-authoring, four radiance-composition-authoring and five indirect-authoring tests
  pass (`native-indirect-final-*.log`). Initial fixture API-conversion errors and missing
  setup closure after expected rejection were corrected before final validation.
- The full Vulkan sample completes 300 frames with exit code zero, retaining its steady
  124 registered / 116 compiled / eight culled operations, one submission and 190 barriers
  (`native-indirect-sample.log`, `native-indirect-sample-results.json`). Its existing
  graph is unchanged; the dedicated fixture exercises the new indirect composition.
- Repository searches find no active removed indirect-entry-point reference, scene
  database type or rendering Builtin file. Vulkan is the exercised backend; D3D12 is
  excluded. No controlled performance benchmark was run. RGBA32Float preserves dynamic
  range at additional bandwidth/allocation cost; no automatic shader fusion is claimed.

The inventory is now 101 concrete reflected operations and 73 editable source/binary
asset pairs. Native reflection-probe preparation/selection, geometry-dependent GI
updates, advanced lighting, complete scene pipelines, remaining composite granularity
and application/editor integration remain unfinished. This phase does not complete
or restore the entire rendering stack.
