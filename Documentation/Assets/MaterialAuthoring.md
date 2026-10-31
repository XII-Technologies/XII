# Native material authoring

Standard properties, Material Graphs, independent `.xiiShader` files, and inherited
materials share `xiiMaterialResourceDescriptor`, schemas, and runtime instances.
There is one native runtime. Ordinary parameter changes do not recompile shaders.

## Removed material system

The old material files and implementation are deleted:

- All nine textual `.xiiMaterial` files and their DDL loader.
- The old base/prototype material assets, Reference, sky, and probe visualization assets.
- The old vertex/pixel material framework, interpolator templates, material data,
  helper/state/configuration headers, simplified/clustered lighting implementation,
  unused sprite/lens-flare/sky/probe shaders, and their obsolete permutations.
- Material binary versions 4 through 8, compressed/embedded-texture loading,
  reserved compatibility fields, old Editor graph patches, and default-file APIs.

Material content is now **version 9 only**. Existing binary caches must be
re-transformed; the material document transform version is incremented to 10.
There are no old-format readers, forwarding headers, or compatibility aliases.
Deleted assets and assignments are intentionally not migrated. Pattern remains
as the current graph example. Missing/loading materials, highlight materials,
and preview materials are shared native descriptors created through the resource
manager, without material files. The missing-mesh source uses Pattern directly.
Debug shaders declare their own small stage interfaces instead of consuming the
removed material interpolator.

## Authoring and rendering

The Qt Editor supports standard properties, typed graph editing, shader-file
parameter inspection, shader diagnostics, and a live native preview. Switching
sources retains independent source data and document undo/redo. Graph compilation
uses Core's abstract Graph framework, reachable-node scheduling and cycle checks,
and the existing shader compiler. Comments and organizational blocks use shared
Editor layout metadata, undo/redo, and clipboard tooling.

`[MATERIAL_CONSTANTS]` is declared by the standard compiler and reflected only in
consuming stages. Custom surfaces use the native GPU-scene vertex interface and
`NativeMaterialSurface.h` / `NativeMaterialState.h`, with `RENDER_PASS` selecting
surface, depth, picking, and wireframe outputs. No `[STAGES]` section is required.
Native schemas retain fixed, explicit, and inherited shader permutations. Blend,
alpha, two-sided, and shading selections classify extraction and instances.

GPU count/prefix/scatter passes partition visible meshlets into material draws.
Standard and authored materials share geometry residency, visibility, the render
graph, and pipeline caches. Authored shaders execute in deferred, depth,
transparent, picking, and wireframe passes. Reflection validates bindings and
packs constants by names, types, and byte offsets. Immutable shader/resource
snapshots survive graph execution; binding caches refresh after compatible
shader, state, texture, or sampler reloads. Transparent lighting is shared with
standard materials.

Preview lighting uses physical light values, manual/automatic exposure, native
sky/shadows, presets, and diagnostic views. Preview state is separate from
material parameters and scene authoring.

## Current boundaries

Native surface dispatch accepts the canonical vertex/pixel interface. Other
shader domains retain their explicit renderer passes. Overdraw, topology/culling,
and ray-tracing diagnostics inspect canonical geometry. Shader displacement
requires conservative authored bounds. Independent instances currently rebind
schemas through the explicit validated API.

Secondary vertex color, old scene-color depth-fade/refraction nodes, and connected
subsurface-color output lack native interfaces and produce node/port diagnostics.
Native transparent transmission/refraction uses current material settings and
renderer features. Reusable graph functions, additional advanced outputs,
backend-error-to-node mapping, arbitrary ground/background controls, and GPU-adapted
exposure inputs remain additional work. Custom material-owned buffer and
standalone sampler override APIs are not implemented.

## Validation

Windows/MSVC Debug and Vulkan checks cover native creation/loading, graph and
handwritten shader rendering, transparent graphs, mixed material bins, constants,
textures, reloads, displacement, masked picking, source templates, document
history, clipboard blocks, Pattern, and preview lighting. The removal regression
also rejects binary versions 4 through 8 and checks file-independent native
fallbacks. D3D12 and other operating systems are not verified.

Removal validation: Debug Editor, EditorFrameworkTest,
EditorEngineProcessFrameworkTest, and GraphicsFoundationTest builds passed.
All nine Vulkan MaterialShaders cases passed (18.81 seconds), all 52 EditorViews
cases passed (67.81 seconds), and ShaderCompiler passed. Active debug and native
reflection-filter shaders compiled after replacing removed includes. The full
native Qt material-authoring run compiled the node library, templates and Pattern,
and completed serialization/history checks, but reported one window-focus
assertion failure in the clipboard block (200.71 seconds). This run is not claimed
as a fully passing GUI acceptance test. Source-format and repository checks passed.
Generated material caches were cleared so retained assets regenerate current
native content; caches and unrelated local asset edits are not committed.
