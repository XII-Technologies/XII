# Material authoring and native material resources

The Editor's authoring modes share `xiiMaterialResourceDescriptor`, the native
material asset writer, `xiiMaterialResource`, immutable schemas, and runtime
instances. Authored surface shaders now execute through the native GPU-scene
raster passes. Standard materials, generated graphs, and shader files use the
same instances, visibility, geometry residency, render graph, and pipeline cache.

## Authoring sources

| Editor workflow | Serialized mode | Source |
| --- | --- | --- |
| Standard Material | `Native` | Reflected native surface properties |
| Material Graph | `Custom` | Existing serialized Visual Shader nodes and connections |
| Custom Shader | `File` | Independently editable `.xiiShader` source |
| Inherited Material | `BaseMaterial` | Existing material reference and sparse parameter overrides |

The serialized enum values and node type identities remain unchanged. New
materials default to Standard Material. Model imports already author native
descriptors through `MeshImportUtils`; that path remains in use.

All modes now write version 8 native material content. The separate Editor
version 7 writer, its compression/embedded-texture implementation, unused material
configuration parsing, and disabled prefab-diff dumping were removed. Existing
resource readers remain available for existing assets; no parser aliases,
forwarding headers, or alternate runtime were added. Resource references own
texture loading in newly written assets.

Changing modes preserves the graph, shader-file selection, and native surface
properties. Shader parameter values are retained by shader source and property
name in document metadata. Restoration runs before the original transaction ends
so it participates in undo/redo without recursively broadcasting property events.
An incompatible restored property produces a diagnostic. Graph-backed inheritance
resolves the base graph's generated shader independently of its retained file
selection.

## Material Graph

The existing Qt node scene supplies searchable creation, selection, copy/paste,
drag connections, undo/redo, pan/zoom, and graph arrangement. The Material Graph
dock is available alongside the 3D preview, with a separate shader diagnostics
dock.

Compilation uses `Core/Graph`: reachable document nodes and pin connections form
an `xiiGraphWithData` topology, Core's topological sort schedules producers before
consumers, and strongly connected components locate imported cycles for node/port
diagnostics. Shared producers are emitted once. Domain pin types remain in the
material Editor; the Core graph contains no shader or Qt concepts. The existing
document format remains authoritative for authoring and serialization.

Numeric math pins explicitly declare polymorphic numeric types. Connections
resolve scalar/vector dimensions through the graph. Scalar splats and linear
RGBA conversions are permitted; arbitrary vector truncation requires an explicit
component-conversion node. Existing graphs that relied on implicit truncation
must insert that conversion. Cycles and incompatible connections are rejected,
and generator diagnostics identify the node and input/output port.

The node library includes numeric reroutes, comments, UV transforms, gradients,
and smooth 2D value noise, plus 34 additional typed nodes:

- Math: one minus, safe reciprocal/divide, smoothstep, step, inverse lerp, remap,
  ping-pong, posterize, and positive modulo.
- Color and layers: luminance, explicit RGB extraction, desaturation, contrast,
  screen/overlay/soft-light blending, and height blending.
- Coordinates and normals: tiling/offset, rotation about a pivot, polar conversion
  in both directions, mirrored UVs, parallax offset, normal unpack, and strength.
- Procedural: checkerboard, radial/ring gradients, stripes, Voronoi, and fixed
  four-octave fractal, ridged, and turbulent noise.

These use the existing node configuration format and shader backend. Vector
constructor outputs now declare their actual dimensions. Safe arithmetic bounds
zero denominators; procedural nodes add no resource bindings or permutations.
Texture binding limits are checked by the selected shader backend instead of an
arbitrary count of texture nodes, including disconnected ones.
Reusable material functions, subgraphs, triplanar nodes, and a complete advanced
surface output are not added.

All shared Editor node scenes also support attached node comments through the
node context menu's Add/Edit/Remove Comment actions. Multiline comments render as
wrapped plain text in fitted bubbles above the node body. Short comments shrink
to their text; long comments wrap without enlarging the node. They participate in
document undo/redo, deletion, copy/paste,
and save/reopen through graph layout metadata. They do not alter transform
hashes, shader source, or domain properties. The material-specific free comment
node remains available; attached comments work across other node document types.

Organizational comment blocks are shared Editor annotations behind domain nodes.
Create one around selected nodes or at the graph cursor, drag its body to move
fully contained nodes in one undo step, or resize with its bottom-right handle.
The block menu edits its text/color, fits its bounds to contained nodes,
duplicates it, or removes it. Blocks persist with stable IDs and validated bounds
in graph layout metadata and do not affect shader compilation. Block duplication
copies the annotation; it does not duplicate its contained nodes. Normal Copy
and Paste actions include selected blocks, including block-only selections.
Selecting nodes with blocks copies their connections and attached notes as well.
Pastes assign fresh block IDs and preserve text, color, size, and relative layout
in one undo step. Regular paste places the group at the graph cursor; Paste at
Original Location retains its coordinates. Cross-document pasting uses the
document's existing graph MIME format. Unselected contained nodes are not copied
implicitly.

The built-in Pattern material now extracts the red channel explicitly before its
scalar roughness operation. This repairs the connection rejected by typed pin
validation while retaining the asset identity, texture references, and authored
layout. Its retained native surface defaults also match the runtime descriptor.

Generation uses the compiler's current section grammar, excludes disconnected
nodes from permutation checks, and does not emit an empty geometry stage. The
compiler backend remains responsible for expression optimization. The Editor
compiles graph and handwritten sources using the selected renderer's shader
platform and fixed, deduplicated material permutations. `.xiiShader` stage
sections and shared `[SHADER]` source remain supported; no `[STAGES]` declaration
is introduced. Owned material shader files were migrated directly to the current
section names.

### Compiler-owned material constants

`[MATERIAL_CONSTANTS]` contains buffer members, using HLSL declarations or XII's
shared shader-header macros. The standard compiler generates the single
`xiiMaterialConstants` declaration before active stage/shared source and runs it
through the normal preprocessor and backend. The generated graph and owned
material shader sources no longer declare that buffer manually. The old
`DefaultMaterialCB.h` was removed; shared default member declarations live in
`DefaultMaterialConstants.h`, included from the constants section.

```hlsl
[MATERIAL_PARAMETER]
Color Tint @Default(Color(1, 1, 1));
float Gain @Default(1);

[MATERIAL_CONSTANTS]
COLOR4F(Tint);
FLOAT1(Gain);

[PIXEL_SHADER]
float4 main() : SV_Target0 { return Tint * Gain; }
```

Do not manually declare `xiiMaterialConstants` in a shader using this section.
Empty/whitespace sections create no buffer. Stage-specific sections and shared
`[SHADER]` source retain their meaning, and the existing backend reflection
determines which stages actually use the buffer. Declaring shared constants does
not create a binding for an unrelated stage. Source line mapping includes blank
lines, so backend errors identify the authored declaration's original location.
This section declares shader storage; it does not introduce a separate GPU
allocation or material runtime.

## Instances and reload

`xiiMaterialInstance::RebindSchema` atomically installs a valid replacement schema.
It preserves explicit parameter overrides by stable ID and exact type, including
an override equal to the old default. Unmodified parameters receive new defaults.
Removed or incompatible overrides are returned as warnings. Texture dimensions
must match, and renderer-owned bindless/sampler indices are invalidated.

Resource-created instances no longer mark all copied defaults as overrides.
Sparse inherited resources no longer inject local scalar PBR defaults over their
parent's values. Ordinary instance parameter edits preserve the schema and only
dirty parameter data unless the parameter is a specialization.

The rebind API is explicit: automatically updating every independently created
instance after resource reload is not implemented. The existing material manager
continues refreshing resource-backed default instances in stable GPU slots.

## Preview lighting and exposure

Preview lighting now belongs to `EditorEngineProcessFramework`. Scene and material
consumers include that implementation directly. Helpers own only their preview
objects and are destroyed before their world. Scene helpers retain export
exclusion using the existing Editor tag. Material preview worlds are entirely
transient, and their helpers remain untagged so hiding gizmos or rendering
thumbnails does not also hide the lighting. Preview geometry uses white vertex
colors instead of a hidden red tint.

The material preview has controls for geometry, object rotation, camera reset,
grid visibility, directional light orientation, illuminance in lux, light color,
temperature, ambient radiance, atmosphere, shadows, manual/automatic exposure,
exposure compensation, and reset operations. Final shading, base color, mapped
normal, roughness, and metallic diagnostics use existing renderer view modes.
These controls are transient document-window/engine-context state.

| Preset | Directional illuminance | Ambient radiance | Camera exposure |
| --- | ---: | ---: | ---: |
| Neutral Studio | 1000 lx | 10 | pi / 1000 |
| Outdoor Daylight | 100000 lx | 1000 | pi / 100000 |
| Low-Light Studio | 10 lx | 0.1 | pi / 10 |
| Unlit / Diagnostic | 0 lx | 0 | Base-color diagnostic selected by the Editor |

The reported brightness issue concerns material previews, not a serialized scene.
Four illumination defects were corrected without scaling authored scene lights:

- The histogram's reserved black/background bin previously contributed its
  sentinel luminance to automatic exposure, allowing the background area to
  increase exposure. Metering now excludes that bin; an entirely black frame
  retains valid history or uses manual exposure.
- Material documents now disable the generic simple-asset directional-light sync
  object. The material context owns its preview lighting, so a user preference
  cannot add a second, independently configured light.
- Native surface reflection omitted defaults and could initialize emission,
  roughness, normal strength, occlusion, and IOR differently from the C++
  descriptor. Reflected defaults now match the descriptor.
- Preview lights tagged as Editor helpers disappeared when gizmos were hidden,
  leaving dark previews and thumbnails. Material preview lighting now remains
  visible in these views; the GPU regression checks calibrated brightness, not
  just the existence of non-black pixels.

Material inspection defaults to stable manual exposure. Physically equivalent
light/exposure ratios can be compared without relying on adaptation convergence.
Arbitrary background selection and a separate ground-plane control are not added.

FeatureLab's locally modified `Main.xiiScene` and untracked material/mesh assets
were not changed. Captured GPU previews cover the calibrated native material
lighting and diagnostic presets.

## Verification and remaining rendering boundary

`MaterialAuthoring` exercises real material documents, the Qt window, typed graph
connections, cycle rejection, generation, source compilation, undo/redo,
save/reopen, parameter editing, and preview-state isolation. GPU `EditorViews`
tests exercise native resource loading, sparse inheritance, instance rebinds,
black-background exposure invariance, helper lifetime/export isolation, and native
material preview rendering and presets. Windows/MSVC Debug Editor and test-target
builds passed. The MaterialAuthoring Editor workflow passed, including connected
procedural and independent handwritten shader compilation. All 52 Vulkan
EditorViews tests passed with calibrated thumbnail brightness assertions and PNG
captures. Repository policy, changed-source formatting, and whitespace checks
also passed. Other platforms and D3D12 were not exercised. Compilation alone is
not a rendering acceptance result.

Follow-up validation compiles every added node and the three vector constructors
through the production shader tool, including vector inputs on all ten new math
nodes, and opens, compiles, saves, and verifies the
identity of the actual Pattern asset in a scratch project. It also covers Core
Graph shared dependencies and malformed imported
cycles, the real Qt multiline comment dialog/actions/display and history round
trips, material-constant buffer reflection/offsets and exact stage masks, cache
reuse, and malformed-constant source locations. Organizational blocks are tested
through actual scene mouse dispatch for drag/resize, one-step grouped undo,
atomic movement rejection, fit/duplication/deletion, and bounds save/reopen. The
final rendering run passed 21 EditorDocuments tests, three NodeLayout tests, ShaderCompiler,
the expanded MaterialAuthoring workflow, and all 52 Vulkan EditorViews tests.
Node-comment and organizational-block captures were visually inspected.
Clipboard follow-up validation passed 22 EditorDocuments tests, three NodeLayout
tests, and the full MaterialAuthoring workflow. It covers actual Qt clipboard
data and actions, block-only/multi-block/mixed cross-document pastes, fresh IDs,
shared offsets, original-position paste, undo/redo, malformed-payload rejection,
save/reopen, and real material-window Ctrl+C/Ctrl+V action proxies. The material
test also processes the delayed Inspector fallback and checks that annotation
selection and generated shader source remain unchanged.
Selected Vulkan permutations of
all seven migrated built-in material shaders and `ReflectionFilteredSpecular`
compiled. The existing `SeparatedBilateralBlur` source has an unavailable
`BLUR_DIRECTION` permutation definition and could not be compiled; its only
change here removes an unused include of the deleted material-buffer header.

## Authored surface shader dispatch

The missing execution path had several causes: raster passes always selected
built-in shaders; schemas dropped authored permutations; graph output and shader
templates used the former vertex-stream interface; and shader blend/culling
permutations did not classify native instances. Schema bytes also use a different
packing contract from HLSL constants. These boundaries now resolve in the native
runtime.

Native schemas retain inherited shader permutations, and shader resources expose
fixed source permutations. Source-fixed values take precedence. Resolved blend,
two-sided, and shading selections classify native extraction and instances,
including inherited shader settings. The raster scheduler resolves
one immutable reflected binding snapshot per material, including its compiled
shader objects, signature, states, textures, and samplers. Visibility remains on
the GPU: count, prefix, and scatter kernels partition the existing visible
meshlets into linear storage and produce one indirect draw per material. Bin zero
keeps ordinary native materials in the same stream. No visibility readback or
second material runtime is introduced. Standard-only views retain their existing
mesh shader path; mixed views use native vertex pulling.

Authored shaders execute in the G-buffer, shadow depth, transparent layers, Editor
picking, and wireframe passes. The native surface contract uses
`GpuSceneSurfaceIndexed.h`, `NativeMaterialSurface.h`, and
`NativeMaterialState.h`. Declare the existing `RENDER_PASS` permutation; no new
stage declaration is required. Use `BLEND_MODE` when alpha modes are needed.
The canonical surface helper emits the appropriate native targets and coverage
for the selected pass. Transparent lighting is shared with standard materials,
including existing light clusters, shadows, atmosphere, and optional refraction.
Its blend state must match the native five-target layer contract.

Graph output now writes that contract directly. UVs, secondary UVs, vertex color,
object/world positions, normals/tangents, camera transforms, time, and manual
camera exposure use native data. Object-space conversions use the inverse
transform rather than the forward transform. The instance-color input reads the
native material instance's base color and opacity. Vertex offsets update current
and previous clip positions. Redundant attribute defines and empty geometry-stage
fragments were removed from the input library. Default, fallback, collision mesh,
skeleton, and hit-box shader sources migrate directly while retaining their paths
and asset references. Barycentrics come from native triangle indices, replacing
those visualization shaders' geometry stages. The material document transform
version is incremented so unchanged assets regenerate cached shader outputs for
the new interface; native runtime content remains version 8.

`[MATERIAL_CONSTANTS]` remains compiler-owned. The renderer copies compatible
parameters by reflected name, type, and byte offset; schema packing is never
assumed to equal HLSL packing. Cross-stage layouts, native binding types/stages,
reverse-Z depth, and transparent blending are validated before dispatch. Supported
material textures include 2D and cube resources with the texture's authored
sampler. Both combined and separate sampler reflection are handled. Optional
empty slots use the engine's shared missing-resource fallback when available;
required or incompatible bindings fail explicitly. Canonical scene buffers are
provided by their owning passes. Arbitrary material-owned buffers and standalone
sampler parameters are not added to the instance API.

Compatible parameter changes reuse compiled permutations. Binding caches also
observe shader objects, signatures, render states, and current texture/sampler
objects. Graph callbacks retain immutable GPU objects through execution, including
when a resource reload replaces the source object. This does not automatically
rebind independently created instances to new schemas; that existing API remains
explicit.

The focused Vulkan `MaterialShaders` suite checks actual authored output, graph
roughness, mixed native/two-authored-material bins, reflected float/float3 packing,
incompatible parameter rejection, vertex and pixel constant consumption, texture
and shader-permutation reloads, instance parameter updates, displacement, masked
coverage, picking, and wireframe. The graph rendering fixture is compared exactly
with production Editor output, including normalized newlines. The Qt workflow
also compiles native input and transformation nodes through the production shader
tool, creates both PBR and fullbright shader templates through the actual dialog
with fixed/selectable options, and retains Pattern, document history, and
save/reopen checks. Template generation uses current section names and
compiler-owned constants; unsupported second/procedural vertex-color choices
are removed from the new-source dialog. Existing source files remain independent.

Remaining boundaries:

- Surface dispatch currently accepts native vertex/pixel shaders. Other shader
  domains and stage combinations continue to use their explicit renderer passes;
  they are not silently treated as native surfaces.
- Overdraw, topology/culling diagnostics, and ray-tracing diagnostics still inspect
  canonical geometry rather than execute authored material displacement.
- Displacement requires conservative authored mesh bounds; bounds are not inferred
  from arbitrary shader expressions. Graph vertex expressions must be valid in
  the vertex stage. Manual camera exposure is exposed; the GPU's adapted exposure
  is not fed back into graph evaluation.
- Secondary vertex color, depth-fade, old scene-color refraction nodes, and a
  connected subsurface-color output lack matching native interfaces. Their
  serialized data remains intact and reachable uses report node/port diagnostics.
  Native transparent transmission/refraction remains available through the
  renderer's existing material settings and enabled pass features.
- Specialized sky/probe visualization shader migration, additional advanced graph
  outputs, backend error-to-node mapping, reusable functions, automatic numeric
  narrowing migration, arbitrary ground/background controls, and exhaustive asset
  migration remain separate work. No compatibility wrappers or alternate runtime
  are used to conceal these boundaries.
- Validation here uses Windows/MSVC Debug and Vulkan. D3D12, other platforms,
  exhaustive shadow/refraction combinations, and full manual Editor acceptance
  remain unverified.

## Dispatch validation

Final Windows/MSVC Debug builds passed for `Editor`, `EditorFrameworkTest`,
`EditorEngineProcessFrameworkTest`, and `GraphicsFoundationTest`.

| Check | Result |
| --- | --- |
| Vulkan `MaterialShaders` | All 9 cases passed, 34.25 seconds |
| Vulkan `EditorViews` | All 52 cases passed, 52.27 seconds |
| Native Windows Qt `MaterialAuthoring` | Passed, 118.90 seconds |
| Native Windows Qt `EditorDocuments` | All 22 cases passed, 4.17 seconds |
| Native Windows Qt `NodeLayout` | All 3 cases passed, 2.35 seconds |
| `ShaderCompiler` | Passed, 0.65 seconds |
| Migrated built-in shaders | Five materials compiled for forward, depth, picking, and wireframe Vulkan variants (20 selected combinations) |
| Source formatting and repository policy | Passed |

The first native clipboard run encountered Windows clipboard access failures and
an invalid test iterator dereference. The test now returns after the failed
assertion. Qt's offscreen document checks passed, but offscreen material/window
and layout checks failed because that platform cannot supply the native window,
selection, and Vulkan swap-chain behavior. Final native Windows runs passed all
three groups; no application was closed to obtain clipboard access. The failed
offscreen runs are not counted as successful GUI acceptance.

Detailed local evidence is in `Workspace/authored-delivery-build.log`,
`authored-delivery-gpu.log`, `authored-editorviews-final.log`,
`authored-templates-windows-final.log`, `authored-windows-documents-final.log`,
`authored-native-nodelayout-final.log`, and `authored-delivery-compiler.log`.
These generated logs and scratch assets are not committed.
