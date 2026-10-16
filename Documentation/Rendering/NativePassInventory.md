# Native rendering pass rebuild

Current source inventory: **41 concrete reflected operation classes** in
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
| Temporal | 1 | `xiiTemporalAntiAliasingPass` |
| Transparency | 1 | `xiiWeightedBlendedOITResolvePass` |
| Utility | 5 | `xiiBufferForwardPass`, `xiiClearTexturePass`, `xiiCounterResetPass`, `xiiTextureForwardPass`, `xiiTextureSelectionPass` |
| Visibility | 1 | `xiiVisibilityViewPass` |
| Visualization | 3 | `xiiDebugOverlayPass`, `xiiSurfaceQueryPass`, `xiiViewRenderModePass` |

Every operation above declares concrete typed pin members through
`XII_MEMBER_PROPERTY`. No static pin type table or stage selector is used.
Properties remain reflected members and retain normal editor attribute handling.

## State ownership

- WorldInstancesUpload publishes the existing world's sparse instance/pose uploads.
  Repeated occurrences use the same graph upload and buffer versions. No hierarchy,
  scene object registry or transform propagation is added.
- VisibilityView captures camera/frustum and selected component indices during
  setup. Execute consumes owned immutable frame data. Its parameters belong to
  each reflected occurrence.
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
| Visibility | Instance/frustum and meshlet culling, temporal Hi-Z, GPU LOD selection/feedback, indirect dispatch and draw preparation as independent nodes. Current low-level GPU visibility service/sample still needs decomposition. |
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
