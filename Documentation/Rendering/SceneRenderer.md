# Default Scene and Editor rendering

`Data/Base/RenderPipelines` contains two default authored resources: **Scene** and
**Editor**, plus independently authored **SceneAtmosphere** and
**SceneDirectionalShadows**, **SceneScreenSpaceReflections**, **SceneScreenSpaceIrradiance**, **SceneTransparency** and **SceneParticles** workloads composed by Scene. Directional shadows reuse
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
ambient visibility, optional world atmosphere, spatial screen reflections, transparent
single-bounce screen-space diffuse transport, mesh and ballistic billboard layers and temporal
antialiasing, exposure,
tone mapping, color grading and presentation. It does not require mesh shaders or ray tracing. Material loading
and shader failures remain errors; a clear output is not evidence of scene rendering.

Scene surface images are viewport-local. Atmospheric lookup images have their
own authored dimensions. The root `ViewTarget` input receives
the view's external render target or current swapchain image. Presentation applies
the window viewport offset, scales the rendered image and preserves pixels outside
that rectangle. `InputCandidates` remains an optional geometry input. Outputs
including `SceneDepth`, `ObjectId`, `HDRSceneColor` and `GradedColor` are explicit
interfaces for composition and inspection.

### Opaque material lighting semantics

Canonical unlit surfaces and materials with `ReceivesLighting` disabled write
sampled base RGB plus emission into the existing emissive target. Ambient,
directional, clustered local, environment-specular and screen-reflection receivers
skip these surfaces; they still write ordinary depth, normals, albedo, IDs and
velocity and can appear in another surface's reflection. Forward transparent
unlit semantics remain unchanged.

```text
world geometry/materials -> G-buffer -> packed lighting policy -> ambient/direct/specular/SSR
                                 +-> sampled emission + unlit base -----------+-> HDR composition
                                                                            -> atmosphere/layers/TAA/display
```

The RGBA8 material target retains roughness, metallic and specular in RGB. Its
alpha is an explicitly packed evaluated-surface byte: bit 0 means unlit; bits
1 through 7 retain their runtime low-feature meanings, including transmission
at bit 6 and sheen at bit 7. The normal-texture flag is replaced because normal
sampling has already produced the G-buffer normal. Higher runtime flags never
saturate this byte and accidentally enable unrelated features. External producers
of these material images must use this packed contract, rather than truncating
or saturating the full runtime material flag word. The shared shader helper is
`Shaders/Pipeline/GBufferMaterial.h`. No target format, allocation or root
interface changes are needed. The retained ReSTIR shader consumers also reject
unlit receivers; shader compilation does not certify that optional algorithm.

`NativeScenePipeline/OpaqueMaterialLightingAndPackedFlags` reads production HDR,
ambient/direct light, emission, packed material and object identity through ten
phases with real geometry, a sun and a point light. Changing both light intensities
changes lit output but leaves unlit final images exactly equal. Unlit HDR equals
the actual emissive target and sampled authored color, including a base texture;
all direct and ambient energy is zero. Removing ReceivesLighting matches explicit
Unlit. GPU byte readbacks verify absent features, baked normal textures, transmission
and sheen without spurious low bits, and restoring the original material restores
the exact final image. Scene reflection acceptance additionally changes a known
reflecting floor to unlit: floor hits and energy become zero, then restore when
the lit mirror returns. Other surfaces continue reflecting it normally.

The correction passes Windows/MSVC Debug validation and complete GraphicsCoreTest
(81 subtests), Vulkan GraphicsFoundationTest (138) and
EditorEngineProcessFrameworkTest (76). Production RendererAssets exports all eight
source assets deterministically on two runs; EditorDocuments (20) and NodeLayout
(3) also pass. The production compiler successfully compiles indexed/mesh G-buffer
and both retained ReSTIR shader consumers for VK_SM66 and exits zero, but logs
file-access errors during sandboxed shutdown. Other backends and optional ReSTIR
runtime behavior remain unverified. Final Debug build checks cover GraphicsCore,
both graphics test executables, both Editor test executables and GpuDrivenScene.
The strengthened depth/identity assertions pass in the final native Scene group;
the rebuilt Vulkan sample explicitly completes 300 frames. No generated pipeline
binary or unrelated scene edit is committed.

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

These defaults intentionally exclude local/virtual/ray shadows, offscreen or multibounce GI,
environment/probe reflections and advanced particle graph rendering. Spatial screen reflections,
screen-space diffuse transport and temporal antialiasing are authored but opt-in.
Other retained operations and focused tests are not certification of complete production techniques.
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
exports all nine sources with the production compiler into the ignored cache, checks
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
transport. Scene deliberately does not use temporal AO yet. Its optional temporal camera now
supplies coherent previous-camera matrices and jitter, but the AO history shader
still needs jitter/depth reprojection and moving-scene acceptance. Its retained
operation has occurrence-local, completion-aware slots and revision-checked
recording ownership; that alone does not certify temporal AO.

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


## Temporal antialiasing

Scene authors `xiiTemporalCameraPass` and the existing, corrected
`xiiTemporalAntiAliasingPass` directly. These are two operation boundaries rather
than a new single-operation asset. The immutable typed camera sample feeds view
constants, surface raster constants, visibility preparation and reconstruction.
Atmosphere HDR feeds optional screen reflections, then TAA; resolved HDR feeds the existing exposure/display chain
and `HDRSceneColor`. Editor composes this same result before its overlays.
Existing root interfaces and source identities are preserved.

Scene authors the camera producer's `Enabled=false`. Enabling that occurrence is
the single policy switch for jitter and reconstruction. Disabled preparation uses
the original camera, and reconstruction forwards the exact input image without
allocating histories or loading a compute pipeline. A disabled resolver with an
explicitly enabled jitter producer is an error. Standalone uses can still supply
velocity/depth without a temporal camera sample; that older interface has no
camera reprojection for depth or sky and requires its caller's motion contract.

| Resource | Format and extent | Dependency |
| --- | --- | --- |
| TemporalCamera | Immutable setup-local CPU snapshot | Current/previous jittered matrices, inverse matrix, NDC jitter, size, world update and device frame identities |
| PerFrameCamera / RasterConstants / VisibilityView | Existing uniform/structured layouts | All consume the same current camera; raster also receives actual previous camera and jitter |
| Velocity | G-buffer RG16Float at render resolution | Unjittered NDC motion from previous object transform and previous skinning palette |
| Current color/depth | Scene RGBA32Float and reversed-Z D32Float | Completed opaque lighting and atmosphere, plus actual raster depth |
| Resolved and retained color | RGBA32Float SRV/UAV in Scene; RGBA16Float also supported by the operation | Stable-grid velocity reprojection, YCoCg neighborhood clipping and reflected motion blend |
| Retained depth | R32Float SRV/UAV at the same extent | Jittered previous surface lookup, background/depth-disocclusion rejection |

Jitter uses an eight-sample Halton sequence, in NDC pixel units. Color history
accumulates on a stable pixel grid; raw depth history uses the difference between
previous and current jitter. Applying that difference to color instead would move
the accumulated image with the samples. Sky reprojection follows camera rotation
without translating an infinite background. Closest-depth velocity dilation uses
a 3 by 3 neighborhood. The reflected current contribution, motion response and
relative depth threshold remain independent algorithm settings.

History is invalid on first use, explicit camera cuts, incompatible extents or
formats, resource rebuilds and skipped world updates. Camera and color frame
identities must match before reuse. World-update identity avoids falsely resetting
one world's history when another world advances the device frame clock. Callers
mark discontinuous camera motion with the existing `InvalidateTemporalHistory`.
The first valid output after a reset is exactly current color. Disabling releases
cached ownership; recordings retain their own required resources.

Each occurrence owns a mutex/revision-checked state. Recordings retain immutable
camera data, pipelines, samplers and history slots, without borrowing a pass or
view. A slot cannot be overwritten until its last GPU frame completes and all
recordings/publications release that slot. External readiness uses GAL fences and
explicit graph dependencies. Old recordings cannot promote history after a cut.
Changing image size eventually retires old allocations; it does not break retained
recordings to achieve immediate release. Scene history costs 20 bytes per pixel
per retained slot (16 color plus 4 depth), excluding allocation overhead; the
number of slots follows actual in-flight and retained recordings.

Production Vulkan tests compare unjittered Scene, jittered current-frame Scene
and reconstruction, using two worlds with independent views, real red box/blue
floor materials and a sun. At 129 by 97, the static comparison changes 1,401 final
pixels. Summed frame-to-frame RGB byte variation over six warmed comparisons falls
from 187,966 to 152,587, about 19 percent. This is a deterministic fixture measure,
not a general quality or performance claim. GPU motion is compared with geometric
projection, including half-precision storage tolerance: 7,544 camera-motion pixels,
209 object-motion pixels and 256/194 pixels on two changing bone poses. The
comparison identifies 197 newly revealed pixels and verifies exact current-color
rejection instead of stale color.

The full comparison repeats with mesh shaders and extended storage formats
disabled (AO is explicitly disabled for that device). It exercises odd resize,
Scene resource reload, cuts, orthographic cameras, a paused/resumed independent
view, destruction of another world
and a retained full Scene recording executed after its world and pipeline are
destroyed. An RGBA16Float operation regression retains completed recordings while
advancing the same occurrence, checks distinct allocations, invalidates between
setup and execution, and verifies actual HDR output after owner retirement.
Editor output changes through the same authored choice and restores exact baseline
images when disabled. Generated captures are `SceneTemporal-Disabled.png`,
`SceneTemporal-Current.png` and `SceneTemporal-Resolved.png` in the GPU test output.

This path performs antialiasing at render resolution. Temporal upsampling,
particle reactive coverage, morph-history acceptance,
stereo, representative quality/performance evaluation and other backends remain
separate work. Moving surfaces in depth may conservatively reject valid history;
there is no previous-position/depth MRT dedicated to deforming surfaces. Temporal
AO and reflections retain their own history/reprojection acceptance requirements.

Windows/MSVC Debug validation passed all four CTest targets: production asset
preparation, GraphicsCore (79 subtests), GraphicsFoundation (114 subtests) and
EditorEngineProcessFramework (76 subtests), on Vulkan. EditorDocuments (20) and
NodeLayout (3) also passed. All five source assets export deterministically;
no compiled pipeline is tracked. The initial jitter-shifted color resolve was
corrected before the image results above. Fixture corrections account for
half-precision/raster motion rounding, closest-depth velocity dilation, paused
views that intentionally make no render attempt, and isolated layout dependency
registration. Other backends and interactive desktop acceptance remain unverified.


## Spatial screen-space reflections

`SceneScreenSpaceReflections.xiiRenderPipelineAsset` is an eight-operation workload
between atmosphere application and temporal reconstruction. Its source identity is
`{ 78cb6185-5e43-9953-e249-8fe1c45085f5 }`. Scene and Editor retain their existing
identities and interfaces; Scene additionally exposes optional `ScreenReflections`
(straight RGB with confidence alpha) and `ReflectionLighting` (confidence-weighted RGB).

The authored `Reflection Depth` texture-forward operation has `Enabled=false` and
`RequireInput=true`. Enabling this occurrence is the single reflection policy switch.
It publishes Scene depth to two depth-pyramid operations and as the explicit presence
input of the radiance-pyramid and BRDF producers. These presence inputs control setup;
the producers do not sample their texels. Absent depth suppresses all reflection GPU
work. Optional confidence composition forwards the original HDR resource exactly,
including its precision and read-only binding flags. Missing required resources in an
active workload are errors. The ordinary Scene default therefore preserves the
previous rendering behavior and cost.

```text
SceneDepth -> Reflection Depth -> Closest Depth / Farthest Depth --+
                    |                                           |
                    +-> Filtered Radiance <- atmosphere HDR      |
                    +-> Integrate BRDF                           |
GBuffer normal/material/albedo + coherent camera ----------------+-> Trace Reflections
                                                                       |
                                                             Reflection Confidence
                                                                       |
atmosphere HDR -------------------------------------------------> Add Reflections -> TAA
```

| Resource | Format and dimensions | Producer / consumer |
| --- | --- | --- |
| Closest depth | R32Float, viewport-sized full mip chain, SRV/UAV | Reversed-Z maximum reduction / trace near bounds |
| Farthest depth | R32Float, viewport-sized full mip chain, SRV/UAV | Reversed-Z minimum reduction / trace far bounds |
| Filtered radiance | RGBA32Float, viewport-sized full mip chain, SRV/UAV | Area-weighted linear HDR reduction / roughness cone sampling |
| BRDF lookup | RG16Float, existing device-shared lookup extent, SRV/UAV | GGX integration / split-sum specular response |
| Camera | Exact 160-byte uniform | Existing view constants / world reconstruction and projection |
| Reflection constants | 48-byte uniform | Reflected traversal, distance, bias and material settings / trace |
| ScreenReflections | RGBA32Float, viewport-sized, SRV/UAV | Spatial trace / confidence weighting |
| ReflectionLighting | RGBA32Float, viewport-sized, SRV/UAV | Confidence weighting / HDR addition |
| HDRSceneColor | RGBA32Float in Scene | Atmosphere plus weighted reflection / TAA and display |

Depth reduction conservatively includes odd-edge texels. Radiance reduction weights
fractional source coverage so odd viewport dimensions do not discard bright edges.
Both producers handle one-dimensional and one-pixel images and restore per-mip GAL
states after reduction. Their graph callbacks own their pipelines, uniforms and
resource descriptions. The BRDF lookup remains an immutable device-owned lighting
resource; each recording retains its integration pipeline. A recording prepared
before another graph initializes the lookup also waits for its readiness fence;
setup order does not decide which recording performs integration. Its initial
imported state is resolved by GAL at execution, rather than captured before another
recording changes it. No new renderer manager
or mutable cross-occurrence reflection history is introduced.

Tracing walks complete projected hierarchy cell segments, descends on overlapping
near/far bounds, solves depth crossings at the finest level, and rejects departure
surface and backface hits while continuing the ray. It supports perspective and
orthographic camera rays, a finite world-space distance and surface bias, reflected
step/thickness/roughness limits, approximate GGX cone filtering of actual color mips,
split-sum material response and confidence fades at screen edges and distance limits.
The previous frame-dependent, biased roughness perturbation is removed. Raw output
uses RGBA32Float rather than the old RGBA16Float contract. Unrelated callers with an
explicit half-float output constraint must resample/convert that output themselves.

A miss contributes zero reflection and preserves the incoming scene radiance.
Offscreen and hidden surfaces therefore cannot appear; no environment, hardware-ray
or diffuse-irradiance algorithm is substituted. Environment convolution/probes and
SSR temporal filtering remain separate work. The old SSR temporal operation depends
on a different previous-surface contract and is deliberately absent here. This path
has no reflection history to invalidate on cuts, first frames, resize or reload.
Whole-scene TAA remains a separate authored choice; moving-reflection/reactive-mask
quality with TAA, stereo, representative performance and non-Vulkan backends still
need acceptance. Spatial roughness filtering is an approximation rather than a
multi-sample glossy transport solution.

The current GGX lookup requires `TextureUAVExtendedFormats` for RG16Float storage.
An enabled unsupported workload fails explicitly before allocation; the authored
reflection switch provides the disabled fallback. AO must also be disabled explicitly
on the same restricted device. The reflection path executes without mesh shaders.

### Reflection GPU acceptance

`SceneReflections` renders actual world-extracted box/floor geometry, materials and
sunlight through the production Scene resource. At 257 by 193, the mirror has 774
reflection pixels, 787 red-dominant trace pixels overall and 775 changed final pixels.
The test checks every RGB pixel against confidence weighting and HDR addition;
depth and object IDs remain identical with the feature disabled. Recoloring the box
produces blue reflection pixels and removes red reflection pixels. Rough floor
materials reject the trace; a dielectric floor reduces weighted reflection energy
from about 1,841 to 210. Removing the box produces zero hits and exactly unchanged
final output; restoring it reproduces the original trace bit-for-bit.

The test also exercises odd resize to 193 by 129, orthographic projection, object movement and camera cuts,
explicit history invalidation, dependency reload that changes the roughness policy
and restores it, independent worlds/views with red and blue
objects, and execution of a retained recording after its pipeline instance and world
retire. Restricted-device coverage verifies an explicit RG16 storage failure while
authored-disabled Scene still renders real geometry. Native producer tests read all
depth/color mips at odd, one-dimensional and one-pixel extents, and compare BRDF
integration to reference values. Optional composition tests cover required-input
failure, exact read-only RGBA16 forwarding, absent confidence and scaled RGBA32 output.
`CanonicalRenderer/EditorUsesScene` verifies 159 matching changed final pixels in
Scene and Editor and exact restoration after the reflection switch is disabled.
The Editor fixture reacquires its floor component by owner handle after world
updates, honoring component storage relocation.


Final Windows/MSVC Debug validation passed the four CTest targets:
RendererAssetPreparation, GraphicsCoreTest (79 subtests), GraphicsFoundationTest
(122 subtests) and EditorEngineProcessFrameworkTest (76 subtests). EditorDocuments
(20) and NodeLayout (3) also passed. All six source assets export byte-identically
through the production compiler, with source identities, dependency metadata and
existing HDR/depth contracts checked. The rebuilt Vulkan sample completed 300
frames with the default reflection switch disabled. Active reflection evidence
comes from the Scene/Editor image and buffer comparisons above. Pinned formatting,
changed-source and whitespace checks passed. Generated binaries, tables, captures
and logs are excluded from commits; D3D12 and other backends remain unverified.

## Transparent world meshes

Scene composes **SceneTransparency**, then **SceneParticles**, after spatial reflections and before TAA.
The root's opaque visibility query now explicitly excludes `Transparent` records;
otherwise the same surface would incorrectly overwrite depth and shade twice.
Editor continues to reference Scene. The new workload uses world-authoritative mesh
extraction and the canonical bindless material/sampler and current skinning path.
It adds one algorithm boundary, `xiiTransparentRasterPass`, and reuses the existing
visibility, LOD initialization, counters, instance/meshlet culling, dispatch bounds,
vertex-pull command generation and weighted resolve. Its thirteen operations are:

```text
Transparent View -> Cull Transparent Instances -> Meshlet Work Bound
    -> Dispatch Meshlets -> Cull Transparent Meshlets -> Transparent Draw Commands
    -> Raster Transparent Layers -> particle append -> shared root resolve -> TAA -> output
Initialize Transparent LOD + four explicit counters feed the culling operations.
The standalone mesh workload retains its own resolve; Scene overrides that resolve
to Bypass=true and resolves the combined mesh/particle targets once at the root.
```

`Transparent View.Enabled` is the workload switch. `SkipEmptySelection=true`
conservatively checks extracted eligible records before publishing a query.
Externally supplied GPU candidates retain conservative world-wide eligibility;
their values are not read on the CPU. No eligible content or a disabled query
produces no layer images and forwards the exact original HDR handle. Strict native
resolve remains the default; only this authored workload opts into absent inputs.
The separate LOD state initializes each frame with `LodHysteresis=0`; it does not
borrow the opaque occurrence's state or the older reusable buffer-history slots.

| Resource | Format and dimensions | Producer and consumer |
| --- | --- | --- |
| Scene instances, poses, geometry, meshlets, materials | Canonical structured buffers | World uploads -> transparent culling, vertex/pixel shaders |
| Visibility query and indirect commands/count | Canonical view snapshot and structured draw buffers | View preparation/culling -> raster; query presence gates setup |
| Depth | Viewport D32Float, one sample, depth attachment | Opaque raster -> read-only GreaterEqual test; transparency never writes it |
| Accumulation | Viewport RGBA32Float, RT/SRV, clear zero | Independent additive blending -> weighted alpha resolve |
| Revealage | Viewport R32Float, RT/SRV, clear one | Product of one minus opacity -> alpha composition |
| Additive | Viewport RGBA32Float, RT/SRV, clear zero | Sum of lit color times opacity -> HDR addition |
| Modulation | Viewport RGBA32Float, RT/SRV, clear one | Product of interpolated transmission factors -> HDR modulation |
| HistoryTrust | Viewport R32Float, RT/SRV, clear one | Product of one minus opacity for every mode -> conservative TAA rejection |
| HDR input/output | Matching RGBA16Float or RGBA32Float, input-sized | Immutable input -> distinct SRV/UAV resolve output; Scene uses full float |
| Light resources | Exact environment/camera constants, typed cluster layout, lights/counts/candidates/IES/grid/indices/overflow | Existing view/world producers -> fragment-local ambient, directional and clustered local lighting |
| Cascades and atlas, optional complete pair | Typed setup-local snapshot with explicit constant buffer dependency and D32 atlas | SceneDirectionalShadows -> PCF at each transparent fragment's world position |
| Atmosphere, optional complete group | Exact physical constants and RGBA32 transmittance/multiple-scattering LUTs | SceneAtmosphere -> integration to each transparent surface's own distance |

The shadow and atmosphere children expose these additional sampling resources;
their existing identities and outputs remain unchanged. Scene retains ViewTarget,
SceneDepth, ObjectId, HDRSceneColor and GradedColor and exposes optional
`TransparentAccumulation`, `TransparentRevealage`, `TransparentAdditive`,
`TransparentModulation` and `TransparentHistoryTrust` diagnostics. The root
`InputCandidates` policy applies to opaque geometry; transparency uses its own
view selection and supports a separately authored workload input.

Materials select alpha, additive or modulation through their effective alpha/blend
state, including blend-mode-only authored materials. The canonical texture alpha,
normal map, material samplers, two-sided coverage and unlit/emission semantics are
shared with opaque shading. Effective GPU alpha metadata also makes invisible
blend-mode-only surfaces stop intercepting Editor picking. Lighting evaluates the
same layered BRDF and local-light/IES helpers. Cluster overflow explicitly visits
the complete candidate list. Instance receiver flags govern directional shadows;
transparent meshes do not become shadow casters in the opaque cascade workload.

Weighted alpha uses `opacity * clamp(10 / (1e-5 + (depth/WeightDepthScale)^2 +
(depth/200)^6), .01, MaximumWeight)` with defaults 5 metres and 3000. Each fragment
adds `(litColor * opacity * weight, opacity * weight)`. Resolve divides accumulated
RGB by accumulated alpha, composites using revealage, adds the additive target,
then multiplies by modulation. This mixed-mode order is explicit and independent
of submission order; it approximates layered alpha rather than sorting surfaces.
HDR alpha is preserved. Half output clamps only composed RGB to the representable
range; full float retains values above 65504.

Active rasterization requires bindless vertex pulling, indirect count support,
independent attachment blending, five color attachments and floating-point blending
for the chosen targets. Unsupported capabilities or shader/pipeline/binding failure
report an error. The explicit fallback is the authored disabled workload. Empty
content also bypasses these requirements. No mesh shader, extended storage-image
format, ray tracer or unrelated transparency algorithm is substituted. Five layer
images cost 56 bytes per pixel; the distinct full-float HDR resolve adds 16 bytes
per pixel before graph lifetime reuse. Performance and large-world budgets have
not been certified.

Callbacks retain shader objects, signatures, pipeline components, samplers and
captured constants. Shader-resource reloads cannot change a prepared recording's
components. Uniform upload and transitions occur in a separate graph operation
before the managed render pass; draw bindings verify GAL-managed states. No callback
borrows a pass, component, view or world. All writable targets belong to the current
graph occurrence.

When TAA is enabled, trust below .999 selects current color. Its private R32 depth
history records `-(depth+1)` for untrusted coverage, so the next frame rejects that
history even when the layer disappears and no trust image is produced. This
preserves HDR alpha and avoids a new temporal allocation. It deliberately sacrifices
temporal antialiasing on covered transparent pixels until transparent motion and
higher-quality reactive reconstruction are implemented. Opaque history behavior
remains unchanged; applications still explicitly invalidate camera cuts/projection
changes with `InvalidateTemporalHistory`.

### Transparent acceptance and limits

`SceneTransparency/RealWorldAlphaLayers` compares authored enabled/disabled Scene
output using real opaque background geometry and transparent mesh materials.
Twenty-nine phases check every RGB pixel against the actual accumulation,
revealage, additive and modulation readbacks; opaque depth and ObjectId stay equal.
The initial pane changes 2,401 final pixels. Tests cover blend-mode-only alpha,
additive, modulation, zero opacity, absent content, restoration, odd resize,
orthographic projection, movement, sun intensity, a separate shadow caster,
atmospheric attenuation and local lights. Shadow removal restores transparent red
energy from about 86.5 to 95.4; local lighting changes it from about 32.3 to 67.2.
Two differently colored layers retain the same composite within float tolerance
after their creation order reverses. Occluded layers contribute zero. A native
texture resource reload switches point wrap, point clamp and linear wrap at UVs
outside the image; both transparent color and opacity follow the authored sampler.
Linear interpolation acceptance allows finite hardware filter precision.

`TemporalWorldsViewsReloadAndRetainedRecording` interleaves red/blue independent
worlds with two temporal views each. Covered pixels match the current-color
reference, 2,116 interior disappeared pixels per world reject stale color, and
first frames, explicit cuts, resize, Scene reload, orthographic reset and movement
remain valid. A prepared graph executes and yields blue transparent pixels after
its pipeline instance and world retire. `ExplicitIndependentBlendFallback` disables
the actual device feature: active content fails explicitly while authored-disabled
content and empty active queries render. Native resolve tests cover both HDR
precisions, retained recordings, incomplete input pairs and shape errors; the
four-frame temporal fixture verifies rejection followed by resumed accumulation.
`CanonicalRenderer/EditorUsesScene` verifies 2,244 matching changed final pixels
through Scene and Editor, exact disabled/invisible/restored images, and opacity-aware
picking.

Sorted alpha, transparent shadow casting, motion-aware transparent TAA,
environment/probe specular and GI remain separate work. Thin-slab refraction and
ballistic particle extraction/rasterization are described below. Tests cover the
canonical material model; custom material shader permutations, transparent skinning, IES-specific
transparent scenes, stereo, other backends and representative performance remain
unverified.

Final Windows/MSVC Debug builds passed for GraphicsCore, GraphicsCoreTest,
GraphicsFoundationTest, EditorFrameworkTest, EditorEngineProcessFrameworkTest and
GpuDrivenScene. Full Vulkan suite execution passed GraphicsCoreTest (79 subtests),
GraphicsFoundationTest (127) and EditorEngineProcessFrameworkTest (76).
RendererAssets passed deterministic exports of all seven sources; EditorDocuments
(20) and NodeLayout (3) passed. The test executables and engine DLLs were built to
an isolated project-local runtime because the user's running Editor held the
normal output libraries; no running Editor process was stopped. Other backends
remain unverified. The first broad GPU run exposed an undefined preprocessor
variable in the shared vertex interface; an explicit optional-interface define
corrects it, and the full GPU rerun passes.


## Thin-slab screen refraction

`SceneTransparency/Raster Transparent Layers.EnableRefraction=true` is authored in
the canonical workload. The native operation defaults to false for existing custom
pipelines. A disabled transparent query, absent eligible content or materials without
the Transmission feature perform no refraction work. Existing blend behavior is
preserved when this choice is disabled. Explicit alpha/blend transparent materials,
including opacity one, select the path; opaque materials are not reclassified.

```text
opaque lighting -> atmosphere -> optional SSR -> immutable BackgroundRadiance ----+
opaque D32 -> graph transfer copy -> shader-readable D32 ------------------------+-> transparent slab trace
world surface + textures + IOR + thickness + coherent camera --------------------+
trace + surface lighting -> existing five OIT targets -> particle append -> root resolve -> TAA -> display
original opaque D32 ----------------------------------> read-only raster depth test
```

The existing Radiance child input now also feeds the raster's optional
`BackgroundRadiance` pin. No new asset, root interface, renderer manager or native
operation class is introduced. Reflected `RefractionDistance` (default 100 world
units), `RefractionSteps` (32, bounded to 128), and
`RefractionThicknessTolerance` (0.1 view-depth units) control traversal independently.

| Resource | Format and dimensions | Producer / consumer |
| --- | --- | --- |
| BackgroundRadiance | Viewport-sized one-sample RGBA16Float or RGBA32Float, SRV; Scene uses full float | Completed opaque atmosphere/reflections -> immutable hit radiance |
| Opaque depth snapshot | Matching one-sample D32Float, SRV/depth-capable | Explicit graph copy from SceneDepth -> projected hit reconstruction |
| Original SceneDepth | Matching D32Float | Opaque raster -> copy source, then read-only transparent depth attachment |
| Refraction constants | 16-byte uniform | Graph upload -> fragment traversal controls |
| Surface optics | Existing canonical material and sampled normal/transmission textures | Coherent material snapshot/bindless upload -> IOR, world-space thickness, tint and transmission |
| OIT targets and HDR output | Existing five targets and distinct resolve output | Forward optical/surface contribution -> shared mesh/particle composition |

The extra depth image costs four bytes per render pixel before graph lifetime
reuse. GAL cannot represent simultaneous DepthRead and ShaderResource texture
states on the same allocation here; the copy makes both dependencies explicit.
Graph callbacks retain constants, shaders, samplers and resource handles. Graph
resource descriptions are copied before additions can invalidate their borrowed
addresses. No refraction history or shared mutable image is introduced.

The shader refracts the incident camera ray into a parallel-plane dielectric slab
using Snell's law and world-space thickness. The exiting ray is parallel to the
incident ray with a lateral displacement. It marches bounded quadratic distances,
refines the first depth crossing, and samples the immutable HDR background at the
hit. Reconstruction uses the actual view-projection matrix and inverse for both
perspective and orthographic cameras. Foreground depth, invalid projection,
offscreen rays, excessive crossing error and exhausted traversal use the unshifted
opaque background explicitly. They do not sample a previous frame or substitute
environment lighting. IOR one has zero Fresnel attenuation and no displacement;
zero thickness has no displacement but retains the interface Fresnel factor.

Schlick Fresnel attenuates transmitted radiance; material transmission and tint
scale it independently of alpha coverage. Surface emission and existing fragment
lighting remain separate contributions. The background already includes aerial
perspective: front-segment scattering is accounted for when combining it with
the surface, avoiding a second atmospheric attenuation. The existing shadow,
atmosphere and combined shader variants support the same optical path. Original
opaque depth and ObjectId remain unchanged. Existing conservative transparency
trust rejects TAA history under refractive coverage and after it disappears.

### Refraction acceptance and limits

`SceneRefraction/DielectricSlabUsesSurfaceOpticsAndOpaqueBackground` compares two
production Scene resources with the native choice enabled/disabled, using actual
red/blue opaque geometry and a tilted transmitting panel. Twenty-four phases
read HDR, completed background, final color, depth and IDs. The initial panel
changes 925 final pixels; its center switches from blue background to red receiver
radiance with the analytically expected Fresnel factor. IOR one equals the actual
background. Tests exercise thickness zero, transmission zero, absent content,
normal/transmission textures, tint, perspective/orthographic projection, odd resize,
one-pixel extent, foreground rejection, offscreen/short-distance fallback, exact
restoration and source resource reload. IOR-one comparisons with atmosphere and
directional shadows also equal the actual background; lit combined variants
produce finite final output. Missing active background and invalid optical
parameters must report failure; an empty malformed workload still bypasses it.

The independent-world temporal fixture repeats first frames, camera cuts, resize,
reload, movement, disappearance and retained recordings with transmitting meshes.
Two worlds each have two independent views; 2,116 disappeared interior pixels per
world reject stale history. Actual IndependentBlend restrictions fail active
content while disabled/empty paths render. Editor composition changes 1,301
matching final pixels through Scene and Editor, with exact enabled/disabled
restoration.

This is bounded screen-space thin-slab transmission. Rough transmission blur,
volume absorption, closed-mesh entry/exit matching, dispersion, multiple refractive
interfaces and transparent/particle ray targets remain unsupported. Offscreen and
hidden geometry cannot contribute. Opaque materials with transmission retain their
previous deferred behavior and need a separate routing/coverage contract; imported
opaque transmission is not certified by these tests. Weighted OIT still approximates
overlapping layers. Stereo, representative performance and other backends remain
unverified. The older `xiiScreenRefractionTracePass` remains an explicit G-buffer
normal UV perturbation operation; its synthetic tests do not establish this path's
optical acceptance.

Windows/MSVC Debug builds passed for GraphicsCore, GraphicsCoreTest,
GraphicsFoundationTest, EditorFrameworkTest, EditorEngineProcessFrameworkTest and
GpuDrivenScene using the isolated project-local runtime. Complete suite execution
passed GraphicsCoreTest (81 subtests), Vulkan GraphicsFoundationTest (137) and
Vulkan EditorEngineProcessFrameworkTest (76). RendererAssets passed deterministic
exports of all eight source assets on two runs; EditorDocuments (20) and NodeLayout
(3) passed. The runs emitted existing Vulkan unused-output and offscreen Qt font
warnings. A final coherent-snapshot adjustment was rebuilt and rechecked with all
81 CPU subtests, the three SceneRefraction GPU subtests and Editor composition.
The rebuilt Vulkan sample explicitly completed 300 Scene renderer frames.
Retained material/texture reload across intervening frames remains unverified.
No generated pipeline resource is committed and no running user Editor
was stopped. These are direct executable suite runs; no new CTest result or
non-Vulkan backend result is claimed.

## Single-bounce screen-space diffuse transport

`SceneScreenSpaceIrradiance.xiiRenderPipelineAsset` is a three-operation workload
composed by the stable `XII.Scene/ScreenIrradiance` occurrence. It reuses existing
native operations; no renderer manager, view scheduling or retired fragments are added.

```text
SceneDepth -> Bounce Depth (optional gate) -> Trace Diffuse Bounce -> Irradiance
                       |                         ^                       |
                       +-> Direct And Emissive --+                       +-> Ambient Diffuse
                              ^                                         |   (AddIrradiance)
DirectLighting + Emission -----+                                         +-> TAA.ReactiveRadiance
G-buffer Normal + current Camera ----------------+
```

`Bounce Depth.Enabled` defaults to false. With no depth token, the gated radiance
addition and trace produce neither images nor GPU work. Ambient lighting retains
the previous result. Enabling the child produces current-frame transport from
visible direct and emissive radiance, before ambient, atmosphere or reflections;
indirect output cannot feed itself. Scene's optional `ScreenIrradiance` and
`BounceRadiance` diagnostics extend its existing interfaces. Editor composes the
same workload through its Scene occurrence.

| Resource | Contract | Producer and consumer |
| --- | --- | --- |
| Depth | Viewport, single-sample D32, readable SRV | Opaque depth -> gate and world-space intersection reconstruction |
| Normal | Matching RG16Float octahedral normals, SRV | G-buffer -> receiver hemisphere and hit-facing test |
| DirectLighting / Emission | Matching RGBA32Float, SRV | Existing direct sum and G-buffer -> gated radiance addition |
| SourceRadiance | Matching RGBA32Float, SRV/UAV | Direct plus emission -> nearest-pixel trace; root BounceRadiance diagnostic |
| Camera | Exact 160-byte current camera uniform | View constants -> inverse projection and projection, including orthographic cameras |
| Irradiance | Matching RGBA32Float, SRV/UAV | Trace -> additive diffuse response, TAA rejection and root ScreenIrradiance diagnostic |
| Trace options | 32-byte constant buffer | Authored range, thickness, bias, sample/step counts and fixed sequence offset -> trace |

The two new full-resolution images total 32 bytes per pixel before graph aliasing;
there is no GI history or retained irradiance allocation. All allocations,
dependencies, transitions and retirement remain graph/GAL managed. Ordinary
compute and readable scene resources are required. Mesh shaders, ray tracing,
extended storage-image formats and independent blending are not GI requirements.
Unsupported active resource contracts, shader compilation or binding failures
remain reported failures. Disabling the gate is the explicit fallback.

The trace reconstructs actual world positions and projects hemisphere rays back
into the current depth image. It uses bounded quadratic-distance marching and
eight crossing refinements, then tests thickness, distance, receiver hemisphere
and hit-facing normals. Depth, normals and radiance use the same nearest-pixel
footprint. Authored defaults are 32 deterministic cosine-weighted samples, 64
steps, four world units, 0.1 thickness tolerance and 0.02 surface bias. Counts,
distances, sequence offset and queue preference remain reflected settings.
The sequence does not advance with unrelated device frames or other views.

The output is incident irradiance: pi times mean incident radiance. Valid depth
has confidence alpha one, including zero-contribution misses; sky is zero.
`Ambient Diffuse.AddIrradiance` preserves existing environment light and adds
irradiance times diffuse albedo/pi, metallic/transmission attenuation and authored
material occlusion exactly once. Unlit receivers remain unlit. Screen AO affects
ambient environment light; the trace already determines bounce visibility.
The operation's default confidence-replacement mode remains available separately.

TAA's optional `ReactiveRadiance` input uses this current irradiance to reject
history wherever RGB is positive. Private negative depth provenance also rejects
the following frame if GI disappears. The existing optional transparent trust
signal works alongside it. Flag-disabled fallback bindings are never sampled;
unaffected pixels continue accumulating. This conservative rule can also reject
history where material attenuation removes the final GI response. It establishes
safe initialization and disappearance behavior, not motion-aware GI reconstruction.

### Diffuse transport acceptance and limits

`SceneIrradiance/GeometryTransportsEmissionToDiffuseReceivers` compares authored
production Scene resources with the depth gate disabled/enabled. Real opaque
emissive walls illuminate a diffuse floor. Twenty phases read HDR, irradiance,
albedo, material, depth, IDs and presented images. Initially 5,879 floor pixels
receive red bounce and change final color. Every receiver RGB response matches
actual irradiance times the captured diffuse material response. Removing emission
or the emitter yields zero bounce and exact disabled output; a black blocker
reduces aggregate red energy from 11,846.55 to 6,502.31. Blue emission changes
transport color. Metallic, unlit and non-light-receiving materials retain their
lighting policy; half transmission halves diffuse response, and a normal pointing
away from the emitter removes bounce.

The fixture also checks perspective/orthographic projection, odd resize, one-pixel
extent, exact restoration, reload, cuts and async queue preference. Two independent
worlds with four views verify distinct images and temporal initialization, warm
frames, cuts, resize, reload and projection resets. All 2,848 disappeared interior
pixels reject stale GI. A retained recording still produces blue transport after
its pipeline instance and world are destroyed. This exposed and fixed an instance
uploader callback borrowing destroyed world state: callbacks now retain only a
separately owned, synchronized upload cache. A dedicated buffer readback regression
verifies dirty instance and identity-pose bytes after world destruction.

`SceneIrradiance/ReactiveRadianceRejectionAndResumedAccumulation` verifies first,
active-signal and disappearance output against current GPU input, followed by exact
resumption of 0.92 history / 0.08 current blending. Active missing radiance must
fail the view, while disabling the authored gate bypasses the incomplete workload.
The scene fixture runs with mesh shaders and extended texture UAV formats disabled.

Editor composition changes 5,879 matching final Scene/Editor pixels, with exact
enabled/disabled restoration. Final Windows/MSVC Debug builds pass GraphicsCore,
GraphicsCoreTest, GraphicsFoundationTest, EditorFrameworkTest,
EditorEngineProcessFrameworkTest and GpuDrivenScene using the isolated runtime.
Complete suite runs pass GraphicsCoreTest (81 subtests), Vulkan
GraphicsFoundationTest (140) and Vulkan EditorEngineProcessFrameworkTest (76).
The final missing-radiance acceptance additions also pass both SceneIrradiance
subtests. RendererAssets regenerates all nine sources deterministically on repeated
runs; EditorDocuments (20) and NodeLayout (3) pass, including the updated Scene
graph. The rebuilt Vulkan sample explicitly completes 300 Scene frames.
Existing unused Vulkan shader-output and offscreen Qt font warnings remain.
No generated pipeline binaries, validation artifacts or unrelated scene edits are
committed. These are direct executable runs, not an additional CTest certification.

This is bounded, single-bounce screen-space transport. Hidden/offscreen surfaces,
probe/voxel transport, multiple bounces, volume transport and environment-convolved
GI are not implemented by this path. Current-view direct radiance approximates
incident light at a secondary surface, including its view-dependent BRDF. There is
no radiance history, denoiser or spatial filtering; fixed sampling may produce
spatial banding. Representative quality and performance, moving-emitter temporal
reconstruction, stereo and non-Vulkan backends remain unverified. Existing DDGI
and sparse-voxel environment seeding/gather operations remain operation-level work.

## Ballistic billboard particles

SceneParticles is an independently authored three-operation workload:

```text
World Births -> Ballistic Trajectories -> Append Billboard Layers
    seeds + trajectory batches              ^ camera, opaque depth
    private surface materials --------------+
SceneTransparency raw layers ---------------+ -> shared root OIT resolve -> TAA
Spatial reflection radiance ----------------------^ original HDR input
```

`World Births.Enabled` gates the entire particle workload. Disabled upload, absent
systems or no live matching births emit no particle buffers; append forwards any
existing mesh layers exactly. With neither workload present, the root resolve
forwards the original HDR handle. Active rendering requires compute, bindless
textures/samplers, five float blend attachments and independent blending. Unsupported
active configurations fail explicitly; disabling particle upload is the authored
fallback. No mesh-shader or hardware-ray capability is needed.

The world component captures bounded deterministic initial conditions during
PostTransform updates, before render extraction. Point, sphere and volume-box
emitters support burst and continuous emission, lifetime removal, authored emitter
and system capacities, bindings and dropped-spawn counts. CPU birth metadata is
not a second evaluation of current particle positions. World-space births preserve
the transform at birth; local-space births follow the current owner transform.
Paused worlds retain their age and can display an initial burst for Editor preview.
Only the world clock advances emission; multiple views cannot advance it twice.

| Resource | Contract | Producer and consumer |
| --- | --- | --- |
| Births | Nonempty structured 128-byte initial state, age/lifetime and stable ID; bounded by MaximumParticles | Immutable world extraction -> upload -> trajectory compute |
| Trajectory batches | Structured 112-byte matrix, tint, size, soft distance and compact material index | Per-view upload -> trajectory compute |
| Evaluated particles | Distinct SRV/UAV structured 128-byte buffer | GPU analytic position/velocity, radius and quadratic lifetime fade -> billboard vertex shader |
| Surface materials | Private structured 160-byte canonical material records | Coherent parameter/texture snapshot -> billboard pixel shader |
| Camera | Exact 160-byte current temporal camera snapshot | Existing view constants -> camera-facing quads and depth reconstruction |
| Depth | Viewport single-sample D32; unchanged depth attachment plus separate D32 copy for sampling | Opaque raster -> read-only depth test and soft intersections |
| Transparent layers | Existing viewport RGBA32 accumulation/additive/modulation and R32 revealage/trust, or neutral new targets | Mesh raster -> read-modify-write billboard append -> one HDR resolve and temporal trust |

`Ballistic Trajectories.Acceleration` explicitly selects a world-space analytic
trajectory, defaulting to Z gravity. It is a distinct operation from the retained
Euler integration pass. Existing graph-runtime allocation/debug-label scheduling
is not an implemented particle graph executor. Graphs, force fields, collision,
neighbors/molecular simulation, events, readback scheduling and sorting are rejected
by this workload. Its billboard renderer is unlit, does not cast shadows and does
not substitute for stretched, ribbon, trail, mesh or volumetric renderers.

Billboards use renderer tint/size and canonical base-color/emissive texture and
sampler evaluation. Authored materials must explicitly select translucent,
additive or modulation blending; absent material selects a white translucent VFX
surface. ReceiveLighting and CastShadows must be false. This path does not yet apply
atmospheric segment scattering to particles. SoftParticleDistance fades coverage
against reconstructed opaque depth. Particles preserve scene depth and ObjectId;
Editor mesh picking continues through them. Covered pixels use the shared
conservative temporal trust, including rejection after particles disappear.

Each prepared recording owns its seed/batch/material values and GAL pipeline
references. Per-occurrence leases retain immutable bindless texture indices and
share identical sampler registrations. Reloaded textures receive new indices;
previous recordings retain the previous actual views. Retirement uses the active
submission frame, which can be newer than the graph's setup frame. Rebuilding a
pipeline or destroying its world does not retire a retained recording's resources.
There is no cross-view GPU simulation history to initialize on cuts, resize or reload.

### Particle output acceptance

The SceneParticles GPU tests compare authored enabled/disabled production Scene
resources with real background geometry. They verify all three blend modes through
five layer readbacks and the final HDR resolve, together with unchanged depth/IDs.
The initial billboard changes 1,296 presented pixels; resize changes 2,209 and the
orthographic case 2,401. Tests exercise world-space transform retention and reset,
opaque occlusion, a measured soft-depth fade, texture alpha reload to zero and back,
live material edits, burst fade/death, continuous births and overlapping transparent
world meshes. Zero-opacity, disabled and dead content restores the baseline exactly.

Two independent worlds with two views verify temporal first frames, motion,
disappearance (2,116 interior pixels per world), cuts, resize, pipeline reload and
orthographic resets. A retained recording renders its old textured particles after
its instance/world are destroyed, four intervening device frames and replacement of
the authored texture with zero-alpha data. A device with IndependentBlend disabled
rejects active particles while disabled/empty paths render normally. The production
Editor composition changes 1,369 matching Scene/Editor pixels and restores exactly.

These results certify the explicit unlit ballistic billboard path on Windows
Vulkan. Lit/atmospheric particles, additional renderer/simulation algorithms,
particle motion reconstruction, multiview/stereo, other backends and representative
large-effect performance remain acceptance work. The default descriptor's allocation
and scheduling flags do not turn the separate unfinished graph runtime into a
production simulation executor.


The final isolated MSVC Debug builds passed GraphicsCore and all affected test and
sample consumers. Full GraphicsCoreTest (80), Vulkan GraphicsFoundationTest (134)
and Vulkan EditorEngineProcessFrameworkTest (76) subtests passed. RendererAssets
regenerated all eight sources twice with equal bytes and stable document identities;
EditorDocuments (20) and NodeLayout (3) passed. The isolated mesh-preview fixture
includes the additional child dependency. Final particle mode assertions also pass
as a focused three-subtest Vulkan run. Logs and captures are project-local under
`Workspace/SceneParticles` and `Workspace/scene-particles-*.log`; binaries, caches
and temporary authoring helpers are excluded from commits.
