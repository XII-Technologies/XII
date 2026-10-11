# Editor modernization

This is an ongoing migration against the current Engine, not a completed-work report.
The authoritative objective is comprehensive Editor and tooling modernization,
including integrated workflows and compatibility. Passing library builds alone
does not satisfy it.

## Baseline and architecture assessment

Baseline: `2ac6681c3` on 2026-10-11, clean worktree. The Windows VS2026 workspace
uses Qt 6.11, DLLs, and individual translation units (unity disabled). The attempted
Debug build of `Editor`, `EditorEngineProcess`, `EditorProcessor`, and
`ToolsFoundationTest` fails in `EditorEngineProcessFramework` before higher-level
Editor targets compile. The log is `Workspace/editor-modernization-baseline.log`.
Failures include removed render-target types, moved component headers, missing
texture utilities, and mismatched reflected enum accessors. These are baseline
failures, not regressions from this modernization.

A repository scan covers 984 C++ files in 16 Editor, EditorPlugins, and tooling
library modules. It finds 78 unresolved include candidates, including generated
Qt UI headers which must not be treated as removed functionality. References
include 28 calls to the removed global render world, 7 actor references in a
window path already partly migrated to WindowManager, and obsolete pass/extractor
property access. Detailed scan evidence is in `Workspace/editor-api-inventory.json`.

Current Engine boundaries:

- Core owns worlds, object/component lifetime, resource handles, reflection-based
  serialization, window registration, and the domain-independent graph.
- GraphicsFoundation owns GAL resources, command queues, swap chains, texture
  readback, and device lifetime.
- GraphicsCore owns world-local `xiiRenderWorldModule` views, reusable pipeline
  resources, isolated runtime instances, and specialized render-graph execution.
- GameEngine supplies window output targets and application orchestration.
- ToolsFoundation owns document objects, commands, undo/redo, reflected properties,
  object mirrors, asset-independent serialization, and project infrastructure.
- GuiFoundation and EditorFramework own Qt presentation, actions, document windows,
  IPC clients, project UI, asset curation, and authoring services.
- Engine-process and domain plugins translate authoring state into real Engine
  objects and resources. They must consume these boundaries rather than restore
  the removed global renderer, actors, extractors, or render context.

## Subsystem disposition

| Subsystem | Disposition | Evidence and intended responsibility |
| --- | --- | --- |
| Document objects, commands, selection, serialization | Retain and verify | ToolsFoundation already owns these responsibilities; preserve UUIDs, undo/redo, save data, and reference validation. |
| Reflected property grids and Qt model/view adapters | Retain, audit lifetime | GuiFoundation and ToolsFoundation provide reusable metadata and commands; inspect event removal and document transitions. |
| Editor startup, project services, plugin discovery | Modernize where demonstrated | Existing modular startup is valuable; audit deployment, stale registrations, shutdown order, and reload. |
| Asset curation, import, transformation, dependency tracking | Retain and migrate consumers | Keep one asset/resource pipeline and existing identifiers; update stale domain APIs without duplicate caches. |
| Engine IPC and world object mirrors | Retain and modernize | Keep document UUID and Engine handle identity distinct; verify reflection messages and object destruction. |
| Embedded viewport window lifetime | Replace mixed ownership | Actor code and WindowManager handles coexist in one path. Register one owned window/output target through current WindowManager. |
| View creation, updates, resize, and destruction | Migrate | Use each document world's RenderWorldModule and supported view target/pipeline interfaces. |
| Thumbnails and screenshots | Consolidate | Replace duplicate staging/copy logic with Foundation GAL readback; ensure GPU completion and row-pitch correctness. |
| Preview lighting and fog | Migrate | Use current component types and settings; preserve persisted authoring property names where semantics remain valid. |
| Scene picking, selection, grids, icons, gizmos | Replace stale renderer integration | Existing user workflows remain valuable; implement supported rendering/readback rather than inert property setters or missing extractor classes. |
| Material, texture, mesh, animation, skeleton previews | Modernize by domain | Inventory moved versus removed Engine facilities; retain supported authoring and preserve source asset data. |
| Render pipeline graph tooling | Retain new resource model, verify end to end | Schemas come from reflected passes; Qt topology stays separate from Core graph semantics. Verify transform, load, reload, and view selection. |
| Visual scripting and state machines | Audit and migrate supported semantics | Do not fabricate runtime support or replace authored document models just to use Core graph. |
| Dead registrations, duplicate helpers, stale UI entries | Remove only with evidence | A missing include alone is not evidence that a valuable workflow should be deleted. Remove entry points together with genuinely obsolete implementations. |
| Platform and build integration | Modernize and verify | Keep Windows/Linux code portable; do not claim untested platforms or configurations. |

## Migration order and evidence gates

1. Shared engine-process window, view, thumbnail, reflection-message, and lighting
   integration. Build its target and test ownership/readback behavior.
2. EditorFramework, GuiFoundation, project/document services, asset processing,
   plugin lifecycle, and IPC. Run ToolsFoundation tests and add focused tests for
   changed invariants.
3. Scene and asset plugins: world synchronization, view pipelines, picking,
   gizmos, material/shader tooling, supported graph editors, and preview resources.
4. Consolidate redundant paths and remove proven obsolete code, build entries,
   registrations, and UI. Preserve user asset and project formats or add migration.
5. Build all applicable applications/plugins/tools. Launch Editor and engine
   process, then verify project open/create, documents save/load, asset browsing
   and transforms, scene creation and object edits, properties, transforms,
   selection, viewport resize/rendering, reload, undo/redo, document switching,
   project close, and shutdown.

Each workflow above remains unverified until it has direct runtime or automated
evidence. Linux, macOS, and alternative build configurations require separate
verification. This inventory and its remaining gates will be updated as the
migration progresses; no requirement is waived because a subsystem is difficult.

## Shared viewport migration evidence

- `EditorEngineProcessFramework` builds in Windows Debug, with unity disabled.
- Embedded windows now own their output targets and are registered with
  `xiiWindowManager`; views belong to the document world's render module.
  Output destruction releases swapchain references before window teardown.
- Offscreen views use one output texture view; the pipeline owns depth and
  intermediate attachments. Thumbnail capture uses GAL texture readback, waits
  for GPU completion, respects mapped row pitches, and publishes no image on
  failure. Thumbnail cancellation restores the world's simulation state.
- Remote activation creates a world-local view, detaches the previous view from
  the shared window, and accepts redraws only for the active document and ID.
  Its former reversed creation condition and unreachable redraw handling are
  removed. Remote-window presentation still needs runtime verification.
- Directional preview lighting uses the current light component and deletes its
  objects when disabled or destroyed. The removed skybox/skylight components
  and global fog toggle have no equivalent supported preview-control API.
  Their obsolete toolbar actions and preference fields are removed together;
  remaining directional preference names are preserved. Scene atmosphere and
  volumetric-medium components remain available for native scene authoring.
  This does not migrate existing scene assets or delete their data. Unsupported
  preview preference fields are discarded when preferences are next saved.
- New `EditorEngineProcessFrameworkTest` coverage passes all four subtests on
  Vulkan: output ownership, camera-message serialization and light lifetime,
  thumbnail pixel readback/scaling, and world-local view lifecycle with isolated
  cameras, shared pipeline resources, viewport updates, and stale-view rejection.
  Vulkan is the only backend enabled in this build (`XII_BUILD_D3D12=OFF`).
- The dependent Editor/engine-process/processor build still fails on obsolete
  gizmo and extractor integrations, animation/state-machine headers, Editor image
  conversion, and other stale consumers. The Editor has not yet been launched;
  the integrated workflow gates above remain open.

## Shared Editor consumer migration evidence

- IPC clients send the current typed camera/render-mode fields and object-change
  payload. Thumbnail decoding uses GAL image descriptions, explicit Qt row pitch,
  and propagates save failures. Persisted monochrome/color wireframe preferences
  are patched to the supported wireframe mode.
- Moved component, state-machine, blackboard, and texture-converter includes now
  reference their current modules. `SharedPluginAssets` builds. The remaining
  `EditorFramework` build failures are missing gizmo and editable-skeleton
  integrations; those workflows still require substantive migration.
- `ToolsFoundationTest` builds. Document objects, command history, metadata,
  object mirrors, variant properties, and data-directory paths pass. The file
  model fails case-only directory rename and subsequent deletion. The failure
  repeats outside the sandbox. An isolated watcher probe confirms ordinary
  directory renames emit notifications but case-only directory renames do not on
  this Windows installation, through either Windows watcher path. No watcher
  implementation has been changed or passing asset-watching claim made. Logs:
  `Workspace/editor-tools-foundation-tests.log` and
  `Workspace/editor-tools-foundation-tests-unrestricted.log`.

## Frame scheduling and document lifetime evidence

- Views expose independent render activation. Disabled views retain their valid
  configuration and pipeline instances. Extraction, scene synchronization, and
  graph execution skip them. GraphicsCore tests pass after this change.
- Editor redraw requests activate their view for one frame. Shared document
  infrastructure consumes requests after presentation and reports which windows
  need presentation. Remote requests activate only the selected remote context.
- Document shutdown releases borrowed-world contexts before world owners. Layers
  detach their parent when their scene closes, preventing later access to a
  destroyed scene context.
- All four Editor engine-process integration subtests pass on Vulkan, including
  real graph execution/readback in a paused world, independent view activation,
  redraw expiration, view/window teardown, and shared-world shutdown ordering.
- Engine-process orchestration has been migrated to paused-world updates,
  requested-window presentation, balanced GAL frames, locked cache invalidation,
  and resource reclamation without rendering from the IPC callback. Its build
  still stops at the missing legacy gizmo renderer; application-level execution
  of this migration remains unverified until that integration is replaced.

## Bone authoring value migration evidence

- `xiiExposedBone` is a tooling-owned value in `ToolsFoundation/Animation`;
  runtime skeletons and poses remain in GraphicsCore. The existing reflected
  type name/version, properties, and binary field order are preserved.
- Hashing uses string contents and transform components rather than object
  memory, and canonicalizes signed zero to match value equality. A nonrecursive
  hierarchy evaluator rejects duplicate names, missing parents, cycles, invalid
  local transforms, and non-finite accumulated matrices with `xiiResult`.
- `ToolsFoundationTest` builds and all three `AnimationAuthoring` subtests pass:
  legacy field-order reads, binary/DDL reflection, variant round trips, hashing,
  unsorted/disconnected hierarchies, invalid input, and a 10,000-bone chain.
  This validates the shared data boundary; the full skeleton asset importer,
  preview, and interactive manipulation workflows still require migration.
- Property editing, manipulation, scene pose metadata, and skeleton parameter
  metadata now include the tooling value directly. Animation clip transformation
  no longer constructs an unused deleted skeleton authoring object. Manipulators
  validate hierarchies, rebuild when keys/counts change, respect visibility,
  reject singular manipulation transforms, and preserve translation/scale during
  rotation edits. Invalid property selections clear the widget's displayed values.
- A fourth `AnimationAuthoring` test passes real document commands, object mirror
  synchronization, bone override insertion/rotation, undo, and redo. The migrated
  property widget compiles; `EditorFramework` still fails at the missing gizmo
  handle integration, so manipulator compilation and GUI interaction are not yet
  verified. See `Workspace/editor-bone-consumers-build.log` and
  `Workspace/editor-bone-tests.log`.

## Gizmo replacement implementation evidence

- The replacement's shared geometry/picking layer uses Core's `xiiGeometry` and
  Utilities' OBJ loader. It implements all 13 shape categories referenced by
  current manipulators and visualizers, including file meshes. Geometry owns no
  game objects, render resources, or Qt objects. Local bounds accelerate solid
  picking; picking uses the actual triangles and lines without temporary storage.
- Ray queries support non-unit directions, transformed/non-uniformly scaled
  geometry, double-sided triangles, and world-space line tolerance. Invalid
  queries and misses preserve the output value. Build failures clear geometry.
  Malformed OBJ face references now fail before the shared loader indexes them.
- `EditorEngineProcessFrameworkTest` builds and all five Vulkan subtests pass,
  including geometry/index integrity for all procedural shapes, transformed hits,
  bounded line hits, file mesh triangulation, and invalid OBJ references. Existing
  thumbnail readback and paused-world execution tests also pass. Logs:
  `Workspace/editor-gizmo-geometry-build.log` and
  `Workspace/editor-gizmo-geometry-tests.log`.
- This is the geometry/picking stage of the replacement. Sync handles, viewport
  drawing, highlighting, and scene picking still need to be connected to current
  rendering APIs. Interactive gizmos and the full Editor build remain unverified;
  no old renderer/component implementation has been restored.

## Debug rendering integration evidence

- Debug visualization now uses render-graph-managed native render passes rather
  than creating an unused render-pass cache. Transient vertices use reusable
  dynamic GAL buffers with CPU write access instead of immutable mesh resources.
  Global constants resolve against the shader stages actually present in reflection.
  Failed shader permutations are rejected before dereferencing missing shaders.
- Debug contexts include both world identity and the world-local view handle.
  New views initialize their extracted debug contexts after handle assignment;
  teardown releases their transient/persistent data. World shutdown snapshots
  view handles before removal, fixing an ID-table iterator invalidation crash.
- All four debug shaders now use the current shader compiler's stage sections.
  `EditorEngineProcessFrameworkTest` builds and all six Vulkan subtests pass,
  including actual triangle drawing, GPU copy/readback, isolation between worlds
  with identical local view handles, one-frame consumption, and world destruction
  with live views. Logs: `Workspace/editor-debug-build.log` and
  `Workspace/editor-debug-tests.log`. A non-failing Vulkan shader-interface warning
  reports an unused vertex output.
- World debug data now freezes once per graph frame and is reused by every view.
  Persistent draws are prepared once, and info text is laid out for each viewport
  without modifying the frozen snapshot. Font/textured draws use the shared
  immutable clamp sampler; failed resource binding prevents a GPU draw.
- Extended GPU readback checks pass for two views consuming the same world draw,
  one-frame expiry, indexed boxes, textured rectangles, and translucent persistent
  text across unequal viewport widths. Three consecutive frames match a separately
  rendered reference byte-for-byte, detecting duplicated text or stale placement.
  Logs: `Workspace/editor-debug-multiview-build.log` and
  `Workspace/editor-debug-multiview-tests.log`. Overlay depth behavior and the
  Editor's gizmo handle integration remain to be implemented.
- World-space lines, triangles, and triangulated Core geometry now support an
  explicit overlay mode through the same renderer and upload batches. Overlays
  run after world/view depth-tested primitives with depth/stencil testing and
  writes disabled. No alternate Editor renderer or depth-clearing workaround is
  introduced. Core geometry validation returns `xiiResult` and rejects malformed
  indices, non-triangle polygons, non-finite data, and non-affine transforms
  before submitting any primitives.
- Vulkan readback verifies occlusion of ordinary geometry, visible overlay lines
  and triangles, transformed geometry, world/view ordering, and unchanged depth
  at every pixel after drawing an overlay. The six integration subtests pass;
  see `Workspace/editor-overlay-build.log` and `Workspace/editor-overlay-tests.log`.
  Connecting synchronized Editor handles and interactive picking remains open.
- Synchronized Editor handles now own settings and cached CPU geometry, with no
  world objects, gizmo components, mesh/material resources, or separate renderer.
  The shared view redraw submits them to GraphicsCore debug passes. Projection,
  viewport height, camera distance, and the user scale determine constant sizing;
  drawing and CPU picking use the same camera-facing transform. Orthographic
  filtering, overlay depth, highlighting, and non-pickable wire visualizers are
  retained. Mirrored Core geometry preserves front-face winding, and small
  invertible handle transforms no longer fail picking because of an absolute
  determinant epsilon.
- Document handle picking chooses overlay priority then nearest forward hit.
  IPC updates retain identity while invalidating geometry only when necessary;
  deleting a sync object unregisters its picking ID and clears its highlight.
  The remaining references to the removed gizmo component/renderer are gone.
  Scene picking still needs to combine this interface with current scene geometry
  queries; integrated Editor interaction is not yet verified.
- `EditorEngineProcessFrameworkTest` builds and all seven Vulkan subtests pass,
  including projected sizing, tiny nobs, camera-facing axes, IPC topology changes,
  hidden handles, picking priority, and registration teardown. GPU readback verifies
  handle rendering, highlighting, depth rejection, overlays, mirrored triangles,
  and wire visualizers. All 17 `GraphicsCoreTest` subtests pass. Logs:
  `Workspace/editor-gizmo-tests.log` and `Workspace/editor-gizmo-core-tests.log`.
  The broader `EditorFramework` build reaches two remaining errors in camera
  framing and transform-tool RTTI includes (`Workspace/editor-gizmo-editor-build.log`).
- `EditorFramework` now builds in Windows Debug with unity compilation disabled.
  Camera framing uses Foundation's current distance query, and transform tools
  include the authoritative game-object reflection declaration directly instead
  of depending on transitive headers. Verification: `Workspace/editor-framework-build.log`.
- `EditorEngineProcess` now builds as an application as well as the shared
  framework. Escape and Stop pass the current game-state API's documented
  `editor-esc` and `editor-force` request sources. This preserves game states'
  ability to distinguish user Escape from an explicit Editor Stop.
  `Workspace/editor-process-build.log` records the successful build. The scene
  plugin still fails on removed extractor, shadow-pool, animation declaration,
  and global render-world interfaces (`Workspace/editor-process-scene-build.log`);
  its viewport features require migration, not replacement API shims.
