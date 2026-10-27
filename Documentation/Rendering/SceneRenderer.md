# Default Scene and Editor rendering

`Data/Base/RenderPipelines` contains two default authored resources: **Scene** and
**Editor**, plus independently authored **SceneAtmosphere** and
**SceneDirectionalShadows** workloads composed by Scene. Directional shadows reuse
**SceneShadowCascade** for each cascade's caster rasterization. The two roots are
defaults, not an enumeration of supported workloads.
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
lights, optional world directional shadows, ambient diffuse and emission, spatial
ambient visibility, optional world atmosphere, exposure, tone mapping, color grading and
presentation. It does not require mesh shaders or ray tracing. Material loading
and shader failures remain errors; a clear output is not evidence of scene rendering.

Scene surface images are viewport-local. Atmospheric lookup images have their
own authored dimensions. The root `ViewTarget` input receives
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

These defaults intentionally exclude local/virtual/ray shadows, GI, reflections,
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
exports all three sources with the production compiler into the ignored cache, checks
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

## World atmosphere

`SceneAtmosphere.xiiRenderPipelineAsset` is a reusable physical atmosphere
workload, referenced by its source GUID from Scene. Its four operations are world
atmosphere constants, transmittance integration, approximate multiple scattering
and per-pixel sky/aerial application. Scene supplies the actual depth, opaque HDR,
camera snapshot and selected directional illumination. Its output replaces HDR
before histogram/exposure, bloom, tone mapping and the public `HDRSceneColor`
interface. Editor consumes this same Scene output before its overlays.
Scene's public HDR interface retains its RGBA32Float SRV/UAV contract through an
authored requirement on this subgraph occurrence. Other uses of the workload can
retain the inactive application's input format.

| Resource | Format and extent | Dependency |
| --- | --- | --- |
| AtmosphereConstants | Uniform buffer, optional | Selected world component and reflected physical settings |
| Transmittance | RGBA32Float, authored 256 by 64 | AtmosphereConstants |
| MultiScatter | RGBA32Float, authored 32 by 32 | AtmosphereConstants and transmittance SRV |
| AtmosphereRadiance | RGBA32Float at input depth extent when active | Both lookup SRVs, camera/environment uniforms, immutable scene depth and radiance |

Lookup images declare SRV/UAV usage and remain graph-owned, occurrence-local
transients. They regenerate when the workload executes; no global LUT registry or
content cache is introduced. Their default image footprint is 272 KiB per active
occurrence before graph allocation overhead. Repeated views do not share mutable
image contents. The four operations require compute, without mesh shaders or
hardware ray tracing.

The constants operation authors `UseWorldAtmosphere=true` and
`RequireWorldAtmosphere=true`. World extraction and existing priority/stable-ID
selection determine its component. With no selected component, or `Enabled=false`,
it publishes no optional constants: both LUT operations emit no work and the
application forwards radiance unchanged. Inactive application preserves its
input format and resource instead of allocating a replacement. Existing authored
fallback behavior remains the default for other uses of the constants operation.
Valid constants with a missing or malformed LUT remain an explicit build failure.

Component physical coefficients, planet/up/altitude, sun parameters and sample
counts remain reflected. Lookup dimensions and queue selection remain independent
pass properties. The implementation integrates Rayleigh/Mie scattering and
absorption along sky and geometry segments, with ozone extinction and the existing
solar disk. Orthographic cameras use parallel rays from their pixel's camera plane.
This path does not supply environment convolution, atmospheric shadows, clouds or
participating-volume lighting; approximate multiple scattering and bounded ray
marching are explicit quality limits.

Vulkan image acceptance uses a real material-bearing object about one kilometre
from the camera and a world directional light. At 65 by 49, atmosphere changes
3,104 sky HDR pixels, all 81 geometry HDR pixels and 1,895 final pixels. Depth and
object identities remain identical. No-component, authored disable and component
deletion restore exact input/output equality. Tests cover independent worlds and
views, physical-setting mutation, odd resizing, camera-history invalidation,
orthographic rays, component recreation and continued rendering after another
world is destroyed. Reloading the referenced resource changes lookup extents to
65 by 17 and 17 by 9, verifying actual dependency recompilation. Separate checks
preserve RGBA16Float on inactive application and reject missing active LUTs.
The native Editor test verifies visible sky through its Scene composition and
restores its original geometry/background pixels after component deletion.

The production export fixture writes the ordinary file-system asset redirection
table `RenderPipelines/PipelineLibrary.xiiAidlt` beside the generated binaries.
The standalone sample mounts that cache/table, so GUID dependencies resolve in a
fresh process. Normal Editor/project packaging uses Scene's source-GUID transform
and package dependency on SceneAtmosphere; the isolated preview test exercises a
project asset table containing both resources. Generated binaries and lookup tables
remain outside source control.

Validation passed on Windows/MSVC Debug with Vulkan: all four CTest targets
(RendererAssetPreparation, GraphicsCoreTest, GraphicsFoundationTest and
EditorEngineProcessFrameworkTest), the EditorDocuments group and focused atmosphere,
native transport and Editor view groups. The final public-HDR contract was exported
again and exercised through atmosphere readbacks. A separate sample process rendered
300 frames using the generated GUID table, without a world atmosphere component;
active sky/aerial output is established by the GPU comparisons above. Initial broad
regressions exposed an incomplete preview package and an inconsistent synthetic
orthographic transport camera; both fixtures were corrected and passed. Other
backends, interactive desktop acceptance and representative performance remain
unverified.

## Directional shadows

Scene references `SceneDirectionalShadows.xiiRenderPipelineAsset` with the same
selected directional-light snapshot used by illumination. The workload contains
cascade preparation, an initialized atlas, four independently configured occurrences
of `SceneShadowCascade`, and directional visibility resolution. Each cascade
occurrence authors twelve operations: snapshot-derived visibility/raster constants,
LOD initialization, four counter resets, instance culling, meshlet bounds/dispatch,
meshlet culling, vertex-pull command generation and ordinary depth rasterization.
It reuses the native culling and raster algorithms rather than a shadow renderer
manager. Cascades above the active count emit no raster work and forward the atlas.

| Resource | Format and extent | Producer and consumer |
| --- | --- | --- |
| Directional light | Immutable CPU snapshot | World selection supplies illumination and `CastShadows` intent to cascade preparation |
| Cascades | Immutable CPU snapshot and uniform buffer | Stabilized camera/light projection supplies matching culling, raster and sampling data |
| Candidates, visibility, LOD, indirect commands | Structured buffers with explicit SRV/UAV/indirect dependencies | Per-cascade selection, existing GPU culling and draw preparation feed depth rasterization |
| Shadow atlas | D32Float, depth/SRV, 2 by 2 tiles for four cascades | Atlas clear and ordered cascade raster versions feed visibility sampling |
| Receiver flags | Sorted immutable `uint2` buffer, eight bytes per world draw record | World GPU identities/flags are copied during setup; per-pixel lookup honors receiver policy |
| Shadow visibility | R32Float at Scene surface extent, SRV/UAV | Atlas, depth, normals, object IDs and camera produce the directional illumination multiplier |

Defaults are four 1024 by 1024 cascades, 120 metres maximum distance and 50 metres
light-space depth padding. The atlas occupies 16 MiB before graph allocation
overhead. Extents, splits and padding remain reflected cascade settings. Sampling
exposes normalized depth bias, world normal bias and normal bias in shadow-texel
units. Four explicit comparison taps provide bilinear PCF, clamped within each
tile. The resolution-aware normal offset prevents flat receivers from acquiring
self-shadowing when atlas resolution changes. There is no temporal shadow history.

`Enabled=false`, no directional emitter, zero intensity or the selected emitter's
`CastShadows=false` produces no cascades, atlas or visibility. Directional lighting
then retains its original unshadowed behavior. Disabling this feature does not
select another shadow algorithm. The canonical vertex-pull path requires compute,
bindless resources and indirect draws; it requires neither mesh shaders, extended
storage-image formats nor hardware ray tracing. Atlas dimensions are checked against
the actual device limit before allocation, including multiplication bounds.

Caster candidates come from world-authoritative extraction with the view's tag
selection, not the main camera's compact visible-meshlet list. Shadow frusta include
the authored depth padding. Orthographic coverage uses pixels per world metre and
conservative meshlet culling without perspective cone rejection. Material shadow
flags, transparent exclusion, alpha coverage and the canonical current-pose depth
shader remain in the raster path. Each occurrence has separate transient contents.
The authored 262,144-meshlet capacity rejects an insufficient conservative world
bound before dispatch; it does not silently publish a truncated caster set.

Production Vulkan comparisons cover real box/floor geometry, materials and a world
sun, two independent worlds and enabled/disabled views. The initial 129 by 97 case
has 305 shadowed pixels and 118 changed final pixels in the first world. A caster
outside an orthographic camera's object-ID image changes 741 visible floor pixels.
Removing the caster, disabling its material's shadow flag, or disabling the light's
shadow flag restores the expected unshadowed output. Depth, object IDs and ambient
illumination remain identical between the enabled and disabled views, while direct
light is multiplied by the measured mask (within its RGBA16Float rounding).

Tests also change the referenced workload from four 1024-texel cascades to one
512-texel cascade, resize the viewport, reload Scene, invalidate camera history,
destroy an independent world and execute a retained canonical recording after its
world and pipeline instance are destroyed. The full comparison repeats on a device
with mesh shaders and extended storage formats disabled. Editor acceptance compares
changed geometry pixels through its existing Scene composition. Generated assets,
images and lookup tables remain untracked.

The retained-recording audit also replaced the main visibility producer's borrowed
`this` callback with occurrence-owned, revision-checked LOD state. Native depth and
G-buffer recording failures now reach graph execution diagnostics and stop dependent
output; the graph submits its valid command prefix through the existing abort path.

Windows/MSVC Debug validation passed all four CTest targets: production asset
preparation, GraphicsCore (79 subtests), GraphicsFoundation (111 subtests) and
EditorEngineProcessFramework (76 subtests), on Vulkan. EditorDocuments also passed.
All five authored sources export deterministically; Scene and Editor keep their
source identities and original required depth/HDR contracts. A fresh sample process
rendered 300 frames using the generated asset table with shadows inactive by its
light's default setting. Active shadow acceptance is provided by the comparisons
above. The first reload run exposed low-resolution receiver acne, which the final
texel-aware bias fixes; the recording regression verifies failure diagnostics,
stopped dependent work and a successful green-image readback after recovery.

This is a conventional directional cascade path. Local-light shadows, virtual-page
shadow residency, hardware-ray shadows, contact-shadow tracing, angular-source
penumbrae, atmospheric shadowing and cascade-blend quality remain separate work.
Animated/cutout shadow content, stereo, large-world performance, other backends and
interactive desktop acceptance require further effect-specific validation.
