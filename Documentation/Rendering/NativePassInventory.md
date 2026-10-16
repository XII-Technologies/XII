# Native rendering pass rebuild

Current source inventory: **44 concrete reflected operation classes** in
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
| Visibility | 3 | `xiiInstanceCullPass`, `xiiLodHistoryResetPass`, `xiiVisibilityViewPass` |
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
| Visibility | Instance/frustum/flags/mask culling and LOD selection/history are native. Meshlet culling, temporal Hi-Z, LOD streaming feedback, indirect dispatch and draw preparation still need independent nodes. Current low-level visibility service/sample still needs decomposition, followed by authored world visibility assets. |
| Geometry | Depth raster and indexed fallback, GBuffer raster, motion vectors, velocity dilation, normal/roughness generation. SurfaceQuery already provides nearest-surface queries. |
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
