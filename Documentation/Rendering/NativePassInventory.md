# Native rendering pass rebuild

Current source inventory: **49 concrete reflected operation classes** in
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
| Composition | 1 | `xiiOpaqueLightingCompositePass` |
| Exposure | 2 | `xiiAutoExposurePass`, `xiiLuminanceHistogramPass` |
| Geometry | 2 | `xiiSurfaceRasterConstantsPass`, `xiiVertexPullDrawCommandsPass` |
| GlobalIllumination | 4 | `xiiDDGIProbeGatherPass`, `xiiScreenSpaceGlobalIlluminationCompositePass`, `xiiScreenSpaceGlobalIlluminationPass`, `xiiSparseVoxelGatherPass` |
| Lighting | 1 | `xiiBRDFLutPass` |
| Output | 5 | `xiiBloomPass`, `xiiColorGradingPass`, `xiiPresentPass`, `xiiToneMappingPass`, `xiiUpscalePass` |
| Particles | 2 | `xiiParticleIntegratePass`, `xiiParticleIntegrationConstantsPass` |
| Preparation | 2 | `xiiViewConstantsPass`, `xiiWorldInstancesUploadPass` |
| RayTracing | 1 | `xiiRayTracingScenePass` |
| Reflections | 3 | `xiiHybridReflectionCompositePass`, `xiiScreenSpaceReflectionsPass`, `xiiScreenSpaceReflectionsTemporalPass` |
| Refraction | 2 | `xiiScreenSpaceRefractionPass`, `xiiScreenSpaceRefractionSnapshotPass` |
| Residency | 2 | `xiiGeometryUploadPass`, `xiiMaterialUploadPass` |
| Sensors | 1 | `xiiSensorOutputPass` |
| Temporal | 2 | `xiiBufferHistoryPass`, `xiiTemporalAntiAliasingPass` |
| Transparency | 1 | `xiiWeightedBlendedOITResolvePass` |
| Utility | 5 | `xiiBufferForwardPass`, `xiiClearTexturePass`, `xiiCounterResetPass`, `xiiTextureForwardPass`, `xiiTextureSelectionPass` |
| Visibility | 6 | `xiiInstanceCullPass`, `xiiLodHistoryResetPass`, `xiiMeshletCullPass`, `xiiMeshletDispatchBoundsPass`, `xiiMeshletDispatchPass`, `xiiVisibilityViewPass` |
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
| Visibility | Instance/frustum/flags/mask culling and LOD selection/history are native. Meshlet culling and indirect dispatch preparation are now native; the InstanceVisibility and nested MeshletVisibility assets exercise them. Temporal Hi-Z, LOD streaming feedback and mesh-shader indirect draw preparation still need independent nodes. Vertex-pull indirect commands are now a separate native operation. The remaining low-level visibility service/sample needs decomposition and deletion. |
| Geometry | Vertex-pull indirect draw preparation is native. Depth raster and indexed fallback, GBuffer raster, motion vectors, velocity dilation, normal/roughness generation. SurfaceQuery now emits raster work directly from supplied constants/commands, without calling the legacy picking orchestrator. |
| Lights | World light snapshot/upload, cluster construction, light-list clear/assignment, sampling preparation, probe selection. Remove the oversized lighting context after consumers use explicit resources. |
| Shadows | Cascade setup, local atlas allocation/clear, directional/spot/point raster, ray shadow tracing/denoising, contact shadows, explicit virtual residency/feedback/raster operations. Replace the deleted composite-only ShadowMemory fixture with native operation tests. |
| Atmosphere/environment | Transmittance, multiple scattering, sky irradiance convolution and environment/probe resources. Constants and aerial perspective composition are native. |
| Direct illumination | ReSTIR DI temporal/spatial/visibility/emissive visibility and deferred direct lighting. |
| Indirect illumination | Deferred indirect lighting, DDGI and sparse-voxel updates, ray GI tracing, ReSTIR GI temporal/spatial reuse and denoising. The gather nodes already exist. |
| Reflections | Ray-traced reflections and denoising. Screen trace, temporal filter and hybrid composition are native. |
| Volumes | Froxel allocation, fog initialization, light injection, integration, resolve and temporal reprojection. |
| Decals | Upload, cull/batch, projected resolve and mesh decal draw. |
| Transparency | Transparent geometry and particle raster. Particle integration and weighted resolve are native. |
| Post-processing granularity | Split remaining multi-operation implementations, including bloom extraction/downsample/filter/upsample, at meaningful graph resource boundaries. Existing reflected classes are not automatically the final decomposition. |
| Assets/consumers | Rebuild game, editor, forward/deferred, depth, shadow and specialized graphs using supported native operations. Convert custom sample graph orchestration into authored resources. |

CPU frustum/source-update/draw-list arrays from the deleted runtime are not a new
scene API to reproduce. World transforms, component/resource identities, geometry
selection and incremental GPU records must continue to come from existing owners.

## Verification status

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
