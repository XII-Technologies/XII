# Material authoring and native material resources

The Editor's authoring modes share `xiiMaterialResourceDescriptor`, the native
material asset writer, `xiiMaterialResource`, immutable schemas, and runtime
instances. This change establishes that common asset boundary and improves
authoring and preview behavior. It does **not** complete per-material shader
execution in the GPU-driven renderer.

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

Numeric math pins explicitly declare polymorphic numeric types. Connections
resolve scalar/vector dimensions through the graph. Scalar splats and linear
RGBA conversions are permitted; arbitrary vector truncation requires an explicit
component-conversion node. Existing graphs that relied on implicit truncation
must insert that conversion. Cycles and incompatible connections are rejected,
and generator diagnostics identify the node and input/output port.

The node library adds numeric reroutes, comments, UV transforms, linear gradients,
and smooth 2D value noise. These use the existing node configuration format.
Texture binding limits are checked by the selected shader backend instead of an
arbitrary count of texture nodes, including disconnected ones.
Comments are annotations, not resizable graph frames. Reusable material functions,
subgraphs, triplanar nodes, and a complete advanced surface output are not added.

Generation uses the compiler's current section grammar, excludes disconnected
nodes from permutation checks, and does not emit an empty geometry stage. The
compiler backend remains responsible for expression optimization. The Editor
compiles graph and handwritten sources using the selected renderer's shader
platform and fixed, deduplicated material permutations. `.xiiShader` stage
sections and shared `[SHADER]` source remain supported; no `[STAGES]` declaration
is introduced. Owned material shader files were migrated directly to the current
section names.

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

`xiiGBufferRasterPass` still selects its built-in vertex-pull/mesh shader rather
than dispatching a material schema's shader. The existing graph output also
targets the older material shader interface. Consequently, successful graph or
handwritten shader compilation and native resource serialization do **not** prove
their authored shader executes in the scene renderer. Per-material native shader
dispatch, the matching stage/resource binding contract, advanced graph surface
output, automatic graph conversion for implicit narrowing, and exhaustive asset
migration remain required before the comprehensive modernization is complete.
