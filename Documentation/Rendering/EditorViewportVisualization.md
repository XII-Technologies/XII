# Editor viewport grid and handles

Related: [issue #168](https://github.com/XII-Technologies/XII/issues/168).

## Composition and ownership

The authored Editor pipeline composes Scene's `GradedColor` and `SceneDepth`
interfaces, and uses Scene's camera constants and wireframe surface interfaces for
view visualization. Wireframe reuses Scene's visibility and draw buffers independently
of Editor picking or selection filters. The query runs only for wireframe views. The infinite
grid follows display-linear view visualization, before selection and icons, so
wireframe cannot overwrite it. The gizmo operation follows those aids, before debug text
and final presentation. Neither operation alters Scene depth. Presentation runs
once after the overlays, including the selected SDR/HDR conversion.

Scene-dependent asset previews wrap the asset profile's selected main renderer
in a C++ resource descriptor. The wrapper references that renderer, disables its
presentation by persistent instance path, and adds grid, gizmo and debug operations
before its own presentation. Specialized resources without the color/depth
interfaces retain their own behavior. Loading and failed resources retain normal
resource fallback diagnostics. There are no new authored preview pipelines.

The Editor retains manipulation mathematics, constraints, snapping, pivots,
transactions and undo/redo. Stable synchronized handles carry transforms, colors,
flags, active state, rotation delta and drag origin. Their compact render records
are copied into view-owned pipeline parameters. A document's submission scratch
is reused; each pipeline instance owns its GPU primitive buffers. Render-graph
imports retain those buffers until rendering completes. No handle creates a
scene object or component.

## Infinite grid

`xiiInfiniteGridPass` draws one fullscreen triangle. Camera-relative unprojection
intersects the configured local/world plane for perspective and orthographic
views. Parallel rays, backward intersections and clipped points are rejected.
Global orthographic grids retain camera-aligned planes through the world origin;
local orthographic grids retain the supplied plane. World axis colors follow the
visible axes, while local grids retain their local X/Y colors.

Three decade-spaced line levels crossfade with zoom. The configured snap density
is the finest permitted step. Periodic phases are reduced in double precision
before shader conversion; matrices contain camera rotation rather than large
world translations. This prevents additional grid precision loss, without
changing the engine's underlying floating-point world-coordinate representation.

Line filtering uses derivatives before periodic wrapping, minimum pixel coverage,
proportional subpixel coverage and unresolved-frequency fading. These follow the
filtering principles in [The Best Darn Grid Shader (Yet)](https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8).
The operation exposes line width and minor/major opacity; existing visibility,
snapping density and local/world grid controls remain authoritative.

## Dedicated handles

`xiiEditorGizmoPass` batches immutable filled templates and shader-expanded analytic
strokes. Rotation rings, active arcs, ticks and radial delta guides are generated
in the vertex shader. Translation and scale axes use compact built-in primitives;
translation planes have translucent fills and crisp borders. Hover, selected,
active and disabled states share one shading language. Active translation shows a
guide from the original pivot. Existing operation values remain in the Editor's
status bar and next to the active handle. Motion events are no longer discarded
by a timer; the release position is applied before committing the transaction.
Translation uses the constraint-plane intersection delta directly, including
orthographic ray origins, rather than an additional distance-based scale correction.

Thin strokes use screen-space expansion with analytic antialiasing. Filled shapes
use a private four-sample color/depth overlay and premultiplied resolve/composition.
On-top handles have their own priority layer, with subdued solids and dashed
strokes behind scene depth. Ordinary handles disappear behind scene geometry.
The scene depth resource is sampled, never replaced by handle depth.

Immediate CPU picking uses the same shape dimensions and per-view transform.
Ring centers remain unpickable; stroked edges have a four-logical-pixel tolerance.
Disabled and visualizer-only handles do not accept picks. Hover uses the live
picking identity; Begin/End/Cancel manipulation events drive active state.
There is no handle GPU readback or readback dependency during dragging.
Hover highlighting follows the current asynchronous picking reply. A click uses
a request identity and a bounded 50 ms IPC wait for its own result, instead of
starting a drag from the previous cursor position. A timeout yields no hit.
Drag-to-position requests scene surfaces beneath visible handles and icons. The
active input context controls this per request and view; ordinary hover picking
retains handle priority. The active placement view continues capturing surfaces
when hover-based picking is disabled, and selected-object exclusions apply only
to that view. Mesh surfaces take priority. A confirmed background hit may
intersect the visible grid plane using the shader's camera-relative settings;
the grid supplies a position and normal without a selectable object identity.
Hidden grids, out-of-range intersections and unavailable mesh captures do not
produce grid hits. Signed-axis handles align only to actual mesh hits. Grid
placement preserves the current orientation, including a preceding mesh
alignment in the same gesture. Grid-only gestures issue translation commands
without rewriting rotation properties. Movement queries remain asynchronous, while release
waits for its matching surface query before ending the document interaction.

Rotation tangents use the window coordinates returned by the projection helpers,
including their existing Y inversion. Axis scaling projects the rotated local
axis at the gesture origin: moving toward its tip grows that axis, and moving
toward the pivot shrinks it from either camera side. Uniform scaling retains the
right/up gesture. Snapping and the Begin/Interaction/End transaction events are
unchanged.

The global grid remains anchored to world zero during translation and Ctrl
camera-follow; local grids retain their gesture origin. Axis coverage fades when
its derivative footprint becomes unresolved near the horizon. Orthographic Scene
views use linear cluster-depth slicing across their signed camera depth interval;
perspective views retain logarithmic slicing.

Logical handle sizing, stroke width and picking tolerance account for per-view
device pixel ratio. Stroke widths also account for Scene render scale. Standard
and property handles support orthographic views. Custom OBJ handles remain
supported through the same submission, template and picking paths.
Uniform scaling has a camera-facing filled square with a dark border; its visual
and picking transforms match and it remains visible inside selected geometry.
It has priority over overlapping end-on axes in both drawing and picking.

The finite CPU grid, per-handle debug-renderer submission, duplicated visualizer
wire reconstruction and ordinary transform-handle OBJ references are removed.
`xiiGizmoGeometry` remains only for picking and immutable template creation.
General scene debug drawing remains a separate existing pipeline operation.

## Verification

`EditorViewportVisualization/InfiniteGrid` performs Vulkan readback assertions for
perspective/distant/shallow/local/orthographic grids, depth occlusion, visibility,
large-coordinate phase stability, all built-in primitives, hover/active feedback,
ring holes, custom geometry, repeated frames, independent views and resizing.
It emits representative PNG captures into the test output directory.

`EditorViews/GizmoHandles` verifies projection/DPI sizing, matching picking
transforms, disabled state, synchronized identity replacement and removal.
`CanonicalRenderer/EditorUsesScene` renders three signed-depth orthographic
wireframe panes alongside the perspective view and checks their grid composition.
`EditorViews/MeshPreview` renders real mesh/material content through the composed
preview resource. `RendererAssets/PipelineAssetLibrary` loads, exports and compiles
the authored Scene and Editor resources. Generated pipeline binaries remain in
the ignored asset cache and are never committed.

The native `EditorIntegration/Startup` run exercises click request matching,
surface placement with all seven handles, surface-normal alignment and release
queries after stale misses. It checks axis scaling in both camera projections,
from opposite camera sides, with rotated local axes, and vertical rotation
gestures through production input handlers. It also covers wireframe output,
grid visibility and resizing through the real Editor IPC path.
Its default-scene blue-atmosphere assertion still fails: the canonical Scene does
not currently provide that atmosphere output. This limitation is retained as a
failing acceptance check, not hidden by a fallback or a weakened assertion.

The drag-to-position/direction follow-up rebuilt Editor and both Editor test
targets in Windows Debug. All new native input assertions passed, including
seven placement handles and 24 axis-scale gestures. The existing atmosphere
assertion was the native suite's only failing assertion. All 48 Vulkan
`EditorViews` subtests and `EditorViewportInteraction/HandleFeedback` passed.

`EditorViewportInteraction/PlacementDocument` exercises the production edit tool
and document commands, including repeated updates and undo/redo. On Windows it
also renders two real meshes through a hidden Vulkan viewport, drives mouse-move
handlers and checks the selected object's document position. Its capture-disabled
mesh case and empty-grid case both failed before their respective fixes. It covers
perspective-grid placement, global/local orthographic planes, cancellation and
hidden-grid rejection without presenting windows or injecting desktop input.
The orientation regression starts with a rotated object and picks a real signed
axis handle. It verifies grid-only rotation preservation, mesh alignment, moving
from the mesh onto a differently oriented grid, undo/redo and fresh-gesture
state. These document-rotation assertions failed before the orientation fix.
Windows Debug builds and the focused placement tests passed. All 48 Vulkan
`EditorViews` subtests passed on repeat; the first broader run hit the unchanged
`AnimationGraphAsset` identical-descriptor reload assertion (root position 4
instead of 1). The Vulkan infinite-grid rendering/readback test also passed.

Windows Vulkan validation passed all 76 engine-process subtests, the authored
asset export tests, Editor document tests and handle-state feedback tests. Native
four-pane output and uniform-scale visibility were inspected; snapped orthographic
translation and undo/redo were exercised. The last press-event correction was
built, but its fast-drag desktop recheck was interrupted by window focus changes.
Ctrl camera-follow and the full set of property manipulators still need a complete
interactive acceptance pass. D3D12, stereo and mixed-DPI multi-monitor operation
have not been certified by this validation.
