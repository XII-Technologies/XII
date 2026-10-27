# Default Scene and Editor rendering

`Data/Base/RenderPipelines` contains two default authored resources: **Scene** and
**Editor**. They are defaults, not an enumeration of supported rendering workloads.
Applications can select any compatible resource or construct a resource descriptor
in C++. Both paths use the same compiler, pipeline instances and render graph.

## Consolidation decisions

The hierarchical compiler from PR #429, world-authoritative extraction from #430,
native operations from #432 and resource/view runtime from #438 remain the common
infrastructure. The missing integration was a dependable default composition and
its consumers:

- Empty default world/Editor profile selections and the fragment collection did
  not establish an application rendering path. Scene now owns that authored path;
  Editor composes it through an ordinary resource reference.
- Root interfaces did not receive view-owned external targets. `ViewTarget` now
  provides that explicit boundary; other interfaces retain caller-owned semantics.
- Subgraph expansion synthesized output resolution constraints, including for
  buffers. It now preserves only authored texture constraints, allowing real
  composition and scaled views to retain their native pin contracts.
- The sample separately coordinated visibility, shadows, sensor and presentation
  fragments. It now exercises ordinary world/view execution and checks its result.
- The fixed preview assets and texture-allocation-only operation duplicated
  engine-owned definitions or graph responsibilities. Normal C++ resource
  descriptors and resource-producing operations replace those dependencies.

The broad GPU run also exposed a GAL lifetime regression: independently retained
objects used the device's internal allocator although they can destroy the last
reference to that device during their own destructor. `GetAllocator()` now returns
the existing parent allocator, honoring its documented lifetime contract. Internal
device helpers retain their tracking allocator. The existing last-buffer-view
lifetime regression exercises the failure directly.

## Implemented path

Scene renders world mesh components through shared GPU instance extraction,
view-local visibility and LOD selection, indirect vertex-pull draws, reversed-Z
depth, material G-buffer rasterization, directional and clustered analytical local
lights, ambient diffuse and emission, exposure, tone mapping, color grading and
presentation. It does not require mesh shaders or ray tracing. Material loading
and shader failures remain errors; a clear output is not evidence of scene rendering.

Scene resources are viewport-local images. The root `ViewTarget` input receives
the view's external render target or current swapchain image. Presentation applies
the window viewport offset, scales the rendered image and preserves pixels outside
that rectangle. `InputCandidates` remains an optional geometry input. Outputs
including `SceneDepth`, `ObjectId`, `HDRSceneColor` and `GradedColor` are explicit
interfaces for composition and inspection.

Editor references Scene and disables that occurrence's presentation through an
ordinary reflected property override. It adds filtered world surface queries for
mesh picking and selected-only, including occluded, selection; material/depth
visualization; shape icons and their asynchronous picking; and debug drawing.
The final Editor operation presents after these overlays. Picking resolves captured
GPU identities against live world object/component handles. Meshes and materials
in scene-dependent previews use the asset profile's main pipeline selection.

Display-linear color grading declares render-target usage as well as compute and
sampling usage, allowing these overlays to compose on the actual Scene output.
Overlay images remain viewport-local; window-coordinate picking maps into their
captured extents, including a scaled or offset view.

The default render scale is stable. Dynamic-resolution feedback is opt-in through
the existing minimum/maximum scale CVars. Unavailable GPU timings do not substitute
CPU loading time, and native resolution preserves odd dimensions and one-pixel views.

## Ownership and failure contracts

- Worlds and components own scene state. The render world module extracts demanded
  categories and synchronizes derived GPU instances and light records.
- Resource descriptors own immutable definitions. Compilations share definitions;
  pipeline instances own mutable pass objects, parameters and temporal state.
- Interfaces carry resources. Expanding a subgraph preserves authored resolution
  constraints; it does not invent constraints on buffer outputs or force all images
  to viewport resolution.
- Operations declare the resources they produce. `CreateTexturePass` is removed.
  The existing ClearTexture operation declares and clears a graph transient;
  ClearColor still clears a supplied target. Allocation, versions, barriers and
  retirement remain graph/GAL responsibilities.
- The GameEngine loading/missing fallback is an ordinary C++ resource descriptor
  containing ClearTexture and Present. Missing resources produce a diagnostic;
  `GetResourceAcquireResult()` distinguishes fallback acquisition from the selected
  resource. Compilation/build errors are not replaced by a successful scene result.
- `xiiView::GetLastRenderResult()` reports the world's latest build/submission
  attempt. Success does not mean GPU completion. Sample error exits inspect this
  result and require a final selected resource.

Fixed texture and cubemap previews are registered from C++ by
`xiiTexturePreviewPass::GetPipelineResource`. They use the normal resource compiler
and view runtime. Explicit per-view overrides remain supported; no SDK
preview asset duplicates these definitions.

The asset profile exposes one **Main Render Pipeline** reference, defaulting to
Scene. Applications, remote views and scene-dependent previews use this selection.
Interactive Editor scenes select the canonical Editor resource directly, and fixed
texture/cubemap utilities use their C++ descriptors. Explicit view references still
take precedence. Pipeline assets retain their authored Settings object and execution
contracts.

Profile version 3 serializes only the main reference. Version-1 GamePipeline and
version-2 ScenePipeline values migrate to MainRenderPipeline; the former empty
version-1 world default selects Scene. Version-2 explicit empty selections remain
empty. Retired Editor/preview profile selectors are discarded. An empty or invalid
main selection remains diagnosable. Migration handles Objects-only profile DDL
as well as files carrying an explicit legacy type table.

## Consolidation and validation boundary

The ninety old SDK fragment asset/binary pairs are retired. Operation-level GPU
and authoring tests construct their own resource descriptors; they do not require
an installed fragment library. Canonical asset tests load the actual Scene and
Editor sources, export them through the production compiler, compare repeated
export bytes, and validate their interfaces. The GPU-driven sample uses
its real world/view update and a selected resource; it has no custom graph builder
or manual component extraction.

Vulkan regression coverage exercises actual geometry/material/lighting pixels,
depth, object IDs and bindless base-color texture sampling, output transfer,
resource reload, world destruction, independent
worlds/views, resizing, offset/scaled presentation and pixels outside the viewport.
The canonical Editor test compares its unobstructed output with Scene, checks
visible/occluded selection and front-surface picking, switches resources, resizes
and checks icon picking. Fixed preview tests exercise real texture/cubemap output.
The sample has completed 300 Vulkan frames at 1440 by 810 through Scene.

These defaults intentionally exclude shadows, GI, reflections, atmospheric sky,
transparent/particle rasterization and temporal antialiasing. Their retained
operations and focused tests are not certification of complete production techniques.
Custom material shader permutations beyond the canonical GPU surface model,
stereo rendering, backend coverage beyond the configured Vulkan build, large-scene
performance and a complete interactive Editor acceptance session remain separate
verification work. Automated engine/Editor GPU execution does not establish those
claims.

Compiled `.xiiBinRenderPipeline` files are generated output and must never be
committed. The Editor transforms the authored sources into its normal
`Data/Base/AssetCache/Common/RenderPipelines` cache; profile defaults and the Editor
subgraph reference use the source asset GUIDs through the existing lookup table.
For standalone GPU tests/sample preparation, run `EditorFrameworkTest -noGui -run
-close -all -filter RendererAssets -outputDir <project-local-test-output>`. This
exports both sources with the production compiler into the ignored cache, checks
stable exports and contracts, and is the CTest fixture for the GPU suites. The
standalone sample mounts that cache; a missing transformation remains a loading
failure rather than generating a substitute renderer. Core CPU tests require no
transformed assets or Editor dependency.

If a preview reports a missing pipeline GUID, check **Main Render Pipeline** in
Asset Profiles against the actual asset. Retired or deleted selections remain
errors; they are not silently replaced by Scene. Applying the profile writes its
matching runtime configuration. A stale runtime profile can otherwise retain an
old selection even when the current profile schema has changed.

## Ambient visibility

Scene connects its actual depth and material normals to `xiiAmbientOcclusionPass`
with `UseRayTracing=false`. `xiiAmbientOcclusionTemporalPass` runs with
`UseHistory=false` to perform only edge-aware spatial filtering. Its `StableAO`
feeds the existing optional `Occlusion` input of diffuse surface lighting. The
normal HDR combination, display operations and Editor composition consume the
result. Root outputs retain their contracts; optional `AmbientOcclusion` exposes
the visibility mask for inspection.

| Resource | Producer and format | Consumer |
| --- | --- | --- |
| SceneDepth | DepthRaster, reversed-Z D32Float at render resolution | AO reconstruction and spatial edge rejection |
| NormalRoughness | GBufferRaster, RGBA8UNormalized | AO world-space hemisphere sampling |
| Normal | GBufferRaster, RG16SNormalized octahedral normals | Spatial edge rejection |
| RawAO | AmbientOcclusion, R8UNormalized SRV/UAV | Spatial filter |
| StableAO | Spatial filter, R8UNormalized SRV/UAV | DiffuseSurfaceLighting.Occlusion |

Both masks are graph transients at the current render resolution. The spatial
path allocates no surface history, readiness fences or retained mask images.
Sampling uses a fixed spatial phase and reconstructs neighbours at their actual
texel centers. The radius is in world units; pixel stride and sample counts
control the screen-space search. Scene authors radius 2, pixel stride 2, eight
directions, four steps and full spatial weighting. Other tuning remains reflected
on the two existing operations.

Setting the AO occurrence's `Enabled=false` publishes an absent optional mask.
The filter emits no work for that absence, and diffuse lighting preserves its
unoccluded result. It does not suppress direct lighting, emission or authored
material occlusion. R8 storage requires `TextureUAVExtendedFormats`; an enabled
path reports an explicit build failure when that capability is unavailable.
An authored disabled occurrence is the supported fallback, with no replacement
algorithm selected implicitly.

This integration is screen-space ambient visibility, with the usual missing
offscreen and hidden geometry. It is not ray-traced visibility or indirect light
transport. Scene deliberately does not use temporal AO yet: surface raster
constants currently repeat the current camera matrix as the previous matrix.
The retained temporal operation has occurrence-local, completion-aware slots and
revision-checked recording ownership, but needs a complete motion contract and
moving-scene acceptance before production temporal use.

Vulkan acceptance uses a material-bearing cube contacting a real floor, a world
directional light and manual exposure. Full image readbacks compare AO enabled
and disabled: depth and direct illumination are identical, ambient diffuse equals
the unoccluded result multiplied by the mask, and the HDR delta equals the ambient
delta. At 129 by 97, 492 final pixels change and minimum visibility is 189/255.
Deleting the occluder restores a neutral mask and identical final images while
the second world's contact occlusion remains active. Odd resizing, resource
reload, camera-history invalidation and independent views/worlds are exercised.

The canonical Editor GPU regression compares contact pixels with Scene and then
disables only its nested AO occurrence, verifying brighter Editor output and an
unchanged Scene view. Separate GPU cases execute AO recordings after their pass,
views and world are destroyed, reject history promotion after invalidation during
setup, and explicitly disable extended storage formats on the device. The latter
requires enabled AO to fail diagnostically and disabled AO to build without a
mask. Focused captures are `SceneAO-Enabled.png`, `SceneAO-Disabled.png` and
`SceneAO-Mask.png` in the test output directory. These are generated evidence,
not source assets or cross-backend certification.

The extension passed the Windows/MSVC Debug GraphicsCore and GraphicsFoundation
suites, the full EditorEngineProcessFramework suite, EditorDocuments, and the
production RendererAssets export fixture on Vulkan. Repeated Scene/Editor exports
are byte-identical, their source identities are preserved and no compiled pipeline
is tracked. Other backends, representative large-world performance and interactive
desktop acceptance remain unverified.
