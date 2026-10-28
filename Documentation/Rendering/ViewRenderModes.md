# Editor view render modes

Scene viewports and scene-rendered asset previews use the authored `Editor` resource. `ScenePreview` remains a view-purpose hint for stable thumbnail exposure; it now selects `xiiRenderPipelineProfileConfig::s_szEditorPipeline`, just like `EditorScene`. Explicit resource references select that resource directly. Mesh, animated mesh, animation clip, skeleton, material and decal factories share this policy. Texture and cube-texture inspection retain `xiiTexturePreviewPass` and its channel/mip controls.

## Rendering contracts

The toolbar filters the reflected enum through `IsSupportedVisualizationMode`. Selecting a mode travels in the existing view redraw message, sets the engine view's render mode, and is resolved separately by each pipeline instance during graph construction. `ViewRenderModes.h` supplies the persistent numeric values to both C++ and HLSL. Existing values are unchanged.

`Editor` composes `Scene`, then visualizes its explicit outputs before editor overlays and presentation. It now connects AO, velocity, ambient diffuse radiance, confidence-weighted screen reflection radiance, bloom and exposure. These inputs were previously disconnected. `Scene` exports bloom and the conditional overdraw image. The `EnvironmentSpecular` visualization pin retains its serialized name but receives the actual current reflection contribution.

Inactive reflection and disabled AO have explicit authored additive-zero and multiplicative-one defaults, respectively. Each is a cleared 1x1 texture selected only when the real optional signal is absent. These are the same neutral contributions used by the normal composition. They do not replace active effect data. Unused clears are culled by the render graph.

Geometry diagnostics extend the existing conditional indexed surface query. They consume the canonical packed vertex layout, current skinning, resident meshlet indices, selected LOD, primitive ranges, instance flags and material coverage. They share visibility inputs with both opaque rendering paths. The query emits only for wireframe or a requested geometry diagnostic. Indexed rasterization also enforces the same selected primitive range as the mesh-shader path.

Diagnostic uniform uploads precede the managed graphics pass. Graph dependencies and resource versions express their ordering. Recorded visualization work retains its pipeline and sampler. Diagnostics write their entire output image without reading normal graded color, allowing unused tone-mapping/grading work and its allocations to be culled. No resource-name discovery or per-mode pipeline factory is involved.

## Available modes and interpretation

| Modes | Actual source and display |
|---|---|
| Default | Normal scene composition, exposure, tone mapping and grading. |
| Wireframe | Material-covered native triangle edges, tested against the solid reversed-Z scene depth. |
| Base Color | Linear RGB albedo. |
| Metallic / Roughness / Specular | Material channels are **roughness R, metallic G, dielectric specular B**. Each selected scalar is displayed as gray. |
| Surface Normals | Decoded octahedral world-space shading normals, mapped from -1..1 to 0..1. |
| Emissive | Actual HDR emissive RGB, displayed as `L / (1 + L)`. |
| Ambient Occlusion | Screen AO visibility; white is unoccluded. Material occlusion remains a separate albedo-alpha quantity. |
| Linear Depth | Camera-forward distance normalized between the actual near and far planes, including signed orthographic ranges. |
| Device Depth | Reversed-Z device depth: near is white, far is black. |
| Motion Vectors | Unjittered NDC velocity converted to UV displacement, multiplied by `MotionVectorScale` (default 10), then offset by 0.5. Red increases to the right, green increases down the screen; no motion is neutral gray. |
| Instance ID | Deterministic hash of the world's non-reused rendering identity. Background is black. |
| Surface Visibility | A nearest-surface visibility record retains the real world rendering identity, resident meshlet index plus one and source primitive index plus one. The displayed color hashes that tuple; the record also contains device depth. |
| Material Shading Model ID | Hash of the actual canonical material shading-model metadata plus one, including distinct lit and unlit models. |
| Meshlet Culling | Actual pre-compaction meshlet decisions: green kept, red frustum rejection, blue cone rejection. Overlapping candidate fragments mix by coverage count; rejected cone silhouettes are visible from either side. Instance selection, LOD and primitive routing precede this stage. Completely offscreen rejects remain outside the image. |
| Candidate Geometry Occlusion Fraction | The fraction of covered pre-compaction candidate fragments behind the actual solid scene depth, with reversed-Z tolerance. Blue means no hidden coverage; green/red mean increasing occluded coverage. This is a fragment-depth diagnostic, not a claim about hardware Hi-Z traversal or CPU occlusion. |
| Indirect Lighting | Ambient and screen-space indirect diffuse contribution, displayed as `L / (1 + L)`. |
| Reflection Contribution | Confidence-weighted reflection radiance actually added by the authored screen-reflection workload; zero when inactive. Uses the same HDR compression. |
| Direct Lighting (White Diffuse) | Current directional and clustered local lights evaluated on a white Lambertian surface, retaining the actual geometric normals and shadow sampling. Uses `L / (1 + L)`. Excludes ambient, bounce, emission and reflection composition. |
| Direct Diffuse / Direct Specular Lighting | Isolated energy-coupled diffuse or specular lobes of the production BRDF, including local finite-area lights and the existing directional/spot shadows. Uses the authored Scene `DirectLighting` output and the same HDR compression. |
| Light Cluster ID | Deterministic color of the exact tile/depth-slice index plus one from the current cluster layout. Orthographic projections use the producer's linear depth partition. |
| Cluster Light Evaluation Count | Number of bounded local-light list entries evaluated for a surface's cluster; on list overflow, the full view candidate count used by the renderer. `log2(count + 1) / 5` maps through blue/green/red; zero is black. Directional lighting is excluded. |
| Directional Shadow Visibility | The actual directional shadow visibility texture consumed by surface lighting, with receiver flags and filtering intact. White is lit; black is occluded. An inactive directional shadow contributes visibility one. |
| Directional Shadow Cascade | The current directional shadow snapshot's actual split selection at each reconstructed surface depth. Colors hash cascade index plus one; no active cascade or geometry is black. |
| Bloom | Thresholded bloom radiance with HDR compression. Inspection still computes the signal when its compositing strength is zero. |
| Vertex Normals / Tangents / Bitangents | Interpolated world-space vertex basis before normal mapping. Normals use the inverse transpose; tangent handedness accounts for reflected world transforms. |
| Vertex Colors | Linear normalized source vertex RGB. |
| UV0 / UV1 | Fractional source coordinates in RG, with blue zero. |
| UV Density | Square root of UV-area/world-area derivatives: texture repeats per world meter. Log2 density -4..4 maps through blue/green/red. Degenerate UVs map to the low end. This is independent of texture resolution. |
| Triangle Size | Actual projected triangle area from affine barycentric derivatives. Log2 pixel area 0..12 maps through blue/green/red. |
| LOD Level | Deterministic color of the selected resident LOD index plus one. |
| Static vs. Dynamic | GPU instance Static flag: blue for static classification, orange otherwise. |
| Meshlet ID | Deterministic color of the resident meshlet index plus one; shared geometry retains its meshlet identity across instances. |
| Overdraw | Additive count of material-covered fragments from submitted visible canonical meshlets, with depth rejection disabled. `log2(count + 1) / 5` maps through blue/green/red. It includes overlapping triangles within a meshlet and is not a count of CPU objects, culled geometry or every transparent/particle workload. |
| Exposure | Applied exposure multiplier in stops. -16 is blue, 0 green, +16 red. This full-view scalar diagnostic includes the background. |
| Tone Mapping Curve | The resolved per-view tone operator and SDR/HDR calibration, including current exposure. Horizontal axis is scene luminance from -10 to +10 stops; vertical axis is output relative to SDR white or the calibrated HDR peak. |
| Color Grading LUT | A sampled 16-cube lookup table of the resolved per-view saturation/contrast transform, flattened into 16 horizontal blue slices. R increases within each slice, G downward, and B across slices. Uses the identical shader function as normal grading. Spatial vignette and stochastic grain are excluded; the renderer has no imported artist LUT. |

Surface modes mask uncovered pixels. Bloom can extend into the background. Diagnostics bypass scene exposure and tone mapping; presentation still applies the output transfer. Camera matrix history now advances even when temporal jitter/TAA is disabled, so camera motion remains measurable without enabling temporal reconstruction. Camera cuts, size changes, resource replacement and interrupted world updates retain their history invalidation behavior.

## Limits

The remaining ray-tracing enum entries stay hidden and are rejected as explicit visualization settings. They still need dedicated outputs or evaluation paths; none is represented by an unrelated material channel.

Geometry diagnostics require the authored indexed query and the canonical packed vertex layout. Wireframe additionally requires native wireframe fill. Overdraw requires floating-point additive render-target blending. The existing Scene/Editor device requirements still apply. Setting a geometry-diagnostic query to a mesh-shader-only variant fails explicitly. Regression variants exercise native Forward+ with mesh shaders enabled, deferred rendering, and Forward+'s supported deferred fallback with mesh shaders disabled.

Runtime acceptance was performed on Windows/Vulkan. Other backends and operating systems, a full interactive multi-window Editor session, comprehensive animated-asset visual acceptance and performance benchmarking remain separate checks.

## Regression evidence

- `GraphicsCoreTest -filter Pipeline`: reflected configuration, composition, serialization and unsupported-mode rejection.
- `GraphicsFoundationTest -filter PipelineResources -renderer Vulkan`: analytical synthetic channel readback, normal decoding, HDR compression, signed orthographic/perspective depth, velocity axis conversion, mode switching and recovery to Default.
- `EditorFrameworkTest -filter RendererAssets`: all ten canonical sources exported by production code, every Editor visualization input connected, conditional geometry diagnostics enabled, and save/reload/re-export byte equivalence.
- `EditorEngineProcessFrameworkTest`, `CanonicalRenderer/ViewRenderModes`: actual source vertex attributes, material changes, selected LOD and meshlet colors, UV density, projected area, one-versus-two submitted fragments, raw wireframe/solid depth versus visible and occluded final edges, independent views, distinct instance colors, object/camera motion, applied exposure, mode recovery, odd resize, resource reload and mirrored tangent handedness.
- `EditorViews/AssetPreviewPipelineSelection`: actual factories for all six scene-preview categories and both texture categories, empty main profiles and explicit resource overrides.
- `EditorViews/MeshPreviewEditorPipeline`: production asset redirection, offscreen rendering and existing orthographic/grid/thumbnail behavior through Editor.
- `CanonicalRenderer/EditorUsesScene`: existing lighting, picking, AO, temporal, reflection, transparency, particle, refraction, GI, bloom, exposure and spotlight acceptance, extended with active/inactive reflection visualization.
- `EditorFrameworkTest -filter EditorIntegration` on native Windows Qt: actual viewport captures, picking, resizing, document controls, scale/rotation gestures and resource transformations. Synthetic wrapped-cursor gestures now use coordinates inside the active screen instead of fixed desktop coordinates, preserving the original assertions on multi-monitor hosts.

## Changed implementation and assets

- `Data/Base/RenderPipelines/Scene.xiiRenderPipelineAsset` and `Editor.xiiRenderPipelineAsset` were saved and regenerated through the production authoring/compiler APIs. The source GUIDs and existing graph connections are retained.
- `GraphicsCore/Pipeline/Declarations.h` and `Implementation/Declarations.cpp` define supported modes using the shared shader value list. `Passes/Visualization/{SurfaceQueryPass,ViewRenderModePass}.{h,cpp}`, `Passes/Temporal/TemporalCameraPass.h` and its implementation, and `Passes/Output/BloomPass.cpp` implement the rendering changes.
- `Data/Base/Shaders/Pipeline/` adds `ViewRenderModes.h`, `ViewRenderModeHelpers.h`, `GpuSceneDiagnostics.xiiShader`, `GpuSceneOverdraw.xiiShader` and `ExposureVisualization.xiiShader`; updates `ViewRenderMode.xiiShader`, `GpuSceneSurface{Vertex,Data,Indexed}.h` and `Passes/Output/ViewRenderModeConstants.h`.
- `EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h` and its implementation select the Editor resource. `Rendering/ScenePreviewPipeline.h` and its implementation are removed.
- Regression changes cover `AssetPreviewPipelineTest`, `ViewRenderModesTest`, `CanonicalRendererTest`, `MeshPreviewTest`, `ViewportTest`, `SceneWindowTest`, `PipelineAssetLibraryTest`, `ScenePipelineResourceTest`, `VisualizationTest`, `VisualizationOperationsTest` and `SceneTemporalTest` in their existing test modules.
- `Data/Tools/xiiEditor/Localization/en/Editor.txt`, this document and `Documentation/Assets/ModelImport.md` supply current labels and usage/validation documentation.

Build and run commands used for final validation are recorded in the pull request. Generated binaries, caches, logs and temporary authoring helpers are excluded from the change.
