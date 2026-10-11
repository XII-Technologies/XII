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
- Scene and asset-preview grids now share `xiiEditorGrid` CPU geometry and the
  standard debug-render graph passes. The removed grid extractor, renderer,
  mutable GPU buffer, shader/layout setup, and render-data class are deleted.
  Local/global placement, ten-step major lines, orthographic camera alignment,
  and zoom-dependent spacing remain; orthographic grids now account for camera
  position when placing their background plane. Tiny snap steps use bounded
  double-precision density calculations instead of overflowing an integer factor.
- Mesh, animated-mesh, and animation-clip views now belong to their document's
  render-world module and use the normal default pipeline resource. Mesh
  statistics query current vertex streams/counts rather than dereferencing a GPU
  buffer and dividing by its stride. Those three view sources compile; the full
  asset plugin still fails in its unmigrated contexts and other previews.
- `EditorEngineProcessFrameworkTest`, `EditorFramework`, and `EditorEngineProcess`
  build in Windows Debug. All seven Vulkan integration subtests pass, with new
  grid readback checks for visible lines and depth occlusion, and geometry checks
  for rotated local placement, orthographic zoom, tiny spacing, extreme viewports,
  and overflow rejection. Logs: `Workspace/editor-grid-build.log`,
  `Workspace/editor-grid-tests.log`, and `Workspace/editor-grid-plugin-build.log`.
- Skeleton previews now create views in their document's render-world module,
  use the default pipeline resource, and query current skeleton joint counts.
  Picking synchronized bone handles uses the shared CPU geometry rather than
  properties of the removed Editor picking pass. The shared pixel picker honors
  viewport offsets and pixel centers, returns the actual ray origin, and clears
  hit fields on failure without changing IPC routing fields. Uninitialized
  cameras are rejected before computing projection matrices.
- All seven Vulkan integration subtests pass, including offset viewport picking,
  stale reply clearing, zero-area viewport and uninitialized camera rejection
  (`Workspace/editor-skeleton-view-tests.log`). The skeleton view source compiles;
  the asset plugin remains incomplete in other contexts and previews
  (`Workspace/editor-skeleton-view-build.log`). Skeleton GUI manipulation remains
  unverified until the complete plugin and Editor build.
- Material and decal previews now create world-owned views and consume the normal
  pipeline resource. The unused MATERIAL_PREVIEW view permutation is removed
  (no shader references it). Material preview geometry uses the authoritative
  common mesh layout, which already includes UV1 and color; obsolete input-layout
  streams and the removed mesh-buffer helper are gone. All generated material
  preview meshes have one material slot, replacing the guessed 20-slot override.
- Mesh and texture contexts use current material overrides. Mesh IPC replacement
  acquires world write access and clears old overrides before applying the new
  list, preventing stale slots when the list shrinks. Decal previews share one
  typed decal resource handle across their six projected instances. Pipeline
  export errors now pass their string view to the status type explicitly.
- These migrated sources compile (`Workspace/editor-preview-bindings-build.log`).
  The remaining asset-plugin compilation failures are the animation contexts and
  texture view implementations; full preview GUI workflows remain unverified.
- Animated-mesh and animation-clip contexts now own current skinned-mesh
  components. Clip scrubbing samples runtime animation resources directly;
  thumbnail sampling uses the midpoint without advancing or toggling world
  simulation, and restores the document's scrub position afterward. Duration
  reporting is independent of whether a preview mesh is selected.
- Shared `xiiAnimationPreview` reuses CPU pose/palette scratch storage and maps
  skeleton joint names to the mesh's palette indices and inverse bind matrices.
  Sparse slots are identity matrices. Missing joints, duplicate indices, invalid
  bind matrices, nonfinite scrub positions, and clips on meshes without skeletons
  fail with `xiiResult`; failed updates clear stale component pose data. Resources
  are acquired per update, so reloads do not retain borrowed resource pointers.
- All eight Vulkan Editor integration subtests pass, including exact scrub
  samples, reordered/sparse palettes, failure recovery, component application
  without simulation, bind-pose handling, and live clip reload through the
  ResourceManager (`Workspace/editor-animation-preview-tests.log`). The test
  caught and prompted a fix for invalid bind matrices before multiplication.
  These checks verify CPU pose data and component state, not final skinned GPU
  output. Both migrated animation contexts compile; the asset plugin still fails
  in skeleton context visualization and texture views
  (`Workspace/editor-animation-context-build.log`). GraphicsCore's existing
  regression suite also passes (`Workspace/editor-preview-bindings-tests.log`).
- Downstream `EditorFramework` and `EditorEngineProcess` builds also pass after
  the shared animation and picking changes
  (`Workspace/editor-animation-consumers-build.log`).
- Texture inspection now has a reflected resource-driven pipeline pass in shared
  Editor infrastructure, with world-owned views, native packed geometry, managed
  render-graph drawing, and the existing debug-overlay stages. GPU bindings are
  acquired each build; settings and pass instances belong to individual views.
  Channel/alpha/coverage selection, automatic/explicit mip sampling, sRGB and
  linear output targets, and engine cube-face orientation are supported.
- All nine Vulkan Editor integration subtests pass, including GPU readback of
  channel/mip values and all six cube faces, independent view settings, pipeline
  reload from serialized definitions, and invalid-setting rejection
  (`Workspace/editor-texture-preview-tests.log`). The cube test caught the
  forward-camera/reversed-depth mismatch and verifies visible near faces.
- Texture and cube document contexts now retain only resource handles and IPC
  inspection settings. Preview mesh objects, per-document preview materials,
  resource-event callbacks and unused internal material assets are removed;
  statistics read acquired loaded resources and mip-size shifts are bounded.
  Both context/view pairs compile (`Workspace/editor-texture-context-build.log`).
  The full asset plugin's remaining failure is the skeleton context's reference
  to the removed animated-mesh component; its visualization migration is pending.
- Shared framework tests, EditorFramework and EditorEngineProcess build after
  texture integration (`Workspace/editor-texture-consumers-build.log`). Texture
  GUI interactions remain unverified until the complete Editor/plugin build.
- Explicit asset-browser folder renames/deletions now remove the complete
  tracked subtree, including nested folder records. Files are removed before
  folders, descendant folders before parents, and case variants/unrelated
  adjacent prefixes are preserved. The new regression failed with five errors
  before the fix and now passes on the workspace ReFS volume.
- All 17 ToolsFoundation subtests pass with output on the host's NTFS temporary
  volume (`Workspace/editor-subtree-tests.log`), including normal and forced
  fallback directory watchers. The explicit subtree regression also passes on
  ReFS (`Workspace/editor-subtree-refs-tests.log`). Native probes identified a
  separate ReFS notification limitation: case-only directory renames produce
  no immediate event; extended notifications return ERROR_INVALID_FUNCTION.
  The existing watcher-only casing test therefore still fails on ReFS, while
  the Editor's explicit NotifyOfChange path is covered and passes. Temporary
  native watcher tracing was removed; no filesystem-specific workaround added.
- Skeleton authoring now has a shared, Qt-independent asset-tool model preserving
  the existing reflected type names, versions and properties. Import coordinates,
  collider/constraint settings and preview configuration belong to tooling; native
  runtime resources receive parent-before-child joints, rest matrices, inverse
  bind matrices and bounds. Removed runtime skeleton builders/visualizers and Ozz
  authoring APIs are not restored. Mesh import presets also move out of the runtime
  declarations: their deleted implementation had left unresolved symbols.
- Conversion uses `xiiResult`, preserves reflected coordinate systems through
  signed-scale decomposition, and rejects malformed ownership, duplicate/empty
  names, invalid coordinate settings and non-invertible/non-finite poses. Conversion
  and ownership cleanup are iterative. All 10 Vulkan Editor framework subtests pass
  (`Workspace/editor-skeleton-model-tests.log`), including authoring reflection and
  native resource round trips, import-setting preservation and the 65,535-joint
  boundary. The dependent test executable builds. Skeleton document/preview
  integration and full Editor workflows still require migration and verification.
- Shared skeleton preview geometry supports bones, exact-name highlighting,
  authored box/sphere/capsule/convex colliders, joint axes and swing/twist limits.
  It uses existing Core geometry and regular debug-render passes, retains no
  borrowed resource pointers, and computes conservative focus/thumbnail bounds.
  Authoring IPC is separate from native skeleton resources. Animation preview
  can apply an explicitly selected skeleton rather than the mesh default.
- All 11 Vulkan Editor framework subtests pass, including selected bone GPU
  readback, collider bounds, malformed metadata, and explicit skeleton component
  application (`Workspace/editor-skeleton-preview-tests.log`). Tests caught and
  corrected selection lookup by string-view storage and inward bounds rounding.
- Native skeleton, clip and animation-graph resources now have a registered file
  loader that consumes the standard transformed-asset envelope before exposing
  the native descriptor. Custom memory loaders continue supplying descriptors
  directly. Previously the default loader's path/header prefix was interpreted
  as descriptor data. File modification tracking and ResourceManager ownership
  remain standard; invalid envelopes and unsupported native versions are rejected.
- Transformed-file loading and file reload pass for native animation resources;
  malformed headers produce the expected missing-resource result without asserts.
  All 13 Vulkan Editor subtests and the GraphicsCore regression suite pass
  (`Workspace/editor-animation-loading-tests.log`,
  `Workspace/editor-animation-loading-core-tests.log`). Dependent engine process
  and test targets build (`Workspace/editor-animation-loading-build.log`).
- Skeleton document contexts now cache shared tooling geometry and apply the
  explicitly selected native skeleton to current skinned-mesh components. Each
  document owns its world, settings and mutable preview state. Reloaded topology
  requires matching authoring metadata; failed updates clear stale mesh poses.
  Reset releases old-world handles, selections and cached resource versions.
- All 13 Vulkan Editor integration subtests pass, including two documents sharing
  one skeleton, independent mesh selection, native resource reload, mismatched
  metadata recovery and document reset (`Workspace/editor-skeleton-reset-tests.log`).
  EnginePluginAssets and the integration executable build. Full Qt asset-plugin
  integration remains incomplete; model-import and several other asset sources
  still refer to removed APIs. Editor GUI workflows remain unverified.
- Skeleton reimport merging is shared tooling logic, independently testable from
  Qt and file parsing. It transfers imported hierarchy ownership, keeps document
  configuration and joint user settings by name, replaces generated colliders,
  computes manipulator frames in import coordinates, and rejects invalid input
  without replacing the existing document hierarchy. All 13 Vulkan integration
  subtests pass (`Workspace/editor-skeleton-merge-tests.log`).
- Convex authoring collider indices now use 32 bits instead of bytes, avoiding
  truncation above vertex 255. A versioned graph patch widens existing asset
  values; standard RTTI numeric conversion also reads legacy binary properties.
  Tests cover the old schema, 300-vertex round trips, preview geometry using
  index 299 and authoring IPC preservation. All 14 Vulkan integration subtests
  pass (`Workspace/editor-skeleton-import-final-tests.log`).
- ModelImporter now reads skeletons into native runtime descriptors plus separate
  tooling collider geometry through the existing Assimp dependency. It has no Qt
  or shared Editor plugin dependency and does not restore the removed importer
  hierarchy, runtime authoring types or offline Ozz construction. Unused private
  Ozz, meshoptimizer and mikktspace dependencies are removed from this module.
- Actual glTF file tests cover external buffers, Unicode paths, mirrored TRS
  matrices, parent ordering, inverse binds, longest joint-name matching and
  collider-local coordinates. Reload and invalid-source tests reject duplicate
  names, singular poses and shear while clearing stale output. All 14 Vulkan
  integration subtests pass; the test executable and EditorEngineProcess build
  (`Workspace/editor-skeleton-import-final-build.log`). Mesh and clip import
  consumers still require their own migration to current native descriptors.
- Skeleton asset documents now use the native importer and shared hierarchy
  merge, serialize native resource descriptors, and send tooling metadata through
  separate IPC for live views and thumbnails. Live custom resource streams no
  longer contain the file-loader envelope. Opening/reconnecting a window resends
  authoring data and highlights. Exposed-bone traversal is iterative and rejects
  invalid ownership. Asset type version 11 requests regeneration of old binaries.
- Skeleton document, transform and window sources compile individually with the
  Qt configuration (`Workspace/editor-skeleton-document-final-compile.log`,
  `Workspace/editor-skeleton-import-final-compile.log`,
  `Workspace/editor-skeleton-window-final-compile.log`). Other asset types still
  prevent the complete Qt plugin build, so import/undo/reconnect GUI workflows
  have not yet been verified as an integrated Editor application.
- The full Qt asset-plugin build now gets through the migrated skeleton sources.
  Remaining failures include mesh/clip import, animation graph pins, property
  animation resources, old texture settings and atlas APIs
  (`Workspace/editor-asset-plugin-current-build.log`). Gradient thumbnails use
  native image descriptions; material defaults use the current string-view API.
  Both changed sources compile individually (`Workspace/editor-gradient-document-compile.log`,
  `Workspace/editor-material-document-compile.log`). Their GUI workflows remain
  part of the pending integrated Editor verification.
- Native image dimensions now distinguish volume depth, array layers and cube
  faces. DDS reading retains texture dimensions and array metadata; cube/volume
  construction, scaling and mip generation preserve their native descriptions.
  Volume upload validation uses one subresource per mip, array upload pitch
  checks restart at each layer, and 3D resources retain their type at depth one
  and account for all uploaded slices. Both ImageDimensions regressions and all
  40 Vulkan GraphicsFoundationTest subtests pass
  (`Workspace/editor-image-dimensions-tests.log`,
  `Workspace/editor-texture-foundation-tests.log`).
- TextureConverter and LUT documents share the native texture asset writer:
  asset header, texture description, sampler description and DDS payload. Filter
  presets now resolve to explicit GAL sampler settings instead of casting preset
  numbers into unrelated GAL enum values. Texture/cube/LUT document versions
  request regeneration of cached binaries. Unsupported LUT stream transforms
  return failure rather than reporting a successful empty transform.
- All 15 Vulkan EditorEngineProcessFrameworkTest subtests pass, including native
  loader round-trips, ResourceManager loading of mipmapped 2D/3D/cube/LUT assets,
  depth-one volumes, GPU texture and sampler validation, 2D readback, and all 12
  filter presets through the actual converter executable
  (`Workspace/editor-texture-assets-tests.log`). TextureTest, TextureConverter,
  EditorEngineProcess and the integration test executable build. The migrated
  LUT source compiles individually with Qt; full texture document migration and
  the complete Qt asset-plugin build remain pending.
- Texture and cube documents now expose fixed converter sampler presets and
  native GAL address modes. Shared document graph patches migrate all 12 legacy
  filter values and four address modes from numeric or named properties; asset
  generators and imported material texture settings use the new properties.
  Preset compilation is shared with TextureConverter and leaves unrelated native
  sampler settings intact. MirrorOnce addressing is supported by conversion and
  CPU sampling, including extreme signed indices.
- All 16 Vulkan EditorEngineProcessFrameworkTest subtests and three
  ImageDimensions subtests pass (`Workspace/editor-texture-properties-tests.log`,
  `Workspace/editor-texture-properties-cpu-tests.log`). Texture/cube document and
  property sources compile individually with Qt. Full Editor workflows remain
  unverified; render-target assets still require migration from the removed
  binary format, and other asset plugin failures remain.
- Native texture resources now load mutable render-target definitions without
  image payloads or streaming quality levels. Texture and sampler creation
  failures propagate as missing-resource state; empty resources return null GPU
  handles safely. GPU memory accounting covers all native subresources, loader
  failure paths release temporary data, and texture asset writer I/O failures
  propagate through xiiResult without asserting stream operators.
- All 18 Vulkan Editor integration subtests pass, including native render-target
  envelopes, four target formats, GPU clear/readback, forced file reload, retained
  old GPU handle lifetime, invalid definitions and failed writes
  (`Workspace/editor-render-target-verification-tests.log`). The complete
  GraphicsCoreTest suite also passes (`Workspace/editor-texture-graphics-core-tests.log`).
- Render-target documents now write native descriptions in a separate transform
  source. Shared texture profile configuration owns two target resolution slots
  with the former 256/512 defaults; dimensions are baked during transformation
  instead of depending on removed runtime resolution CVars. Legacy document slot
  identities and scale values migrate to profile settings. Existing project
  profiles retain defaults for new fields, and all resolution fields participate
  in the transform cache hash. Custom runtime CVar values must be transferred to
  the corresponding profile fields; runtime CVar resizing is no longer offered.
  Texture document version 8 regenerates cached binaries. Unsupported texture
  and cube stream transformations return explicit failure.
- Shared profile tests cover fixed and scaled dimensions, quantization, bounds,
  invalid values, cache hashes and old profile loading. The full asset-plugin
  build compiles migrated texture, cube, LUT and render-target sources; remaining
  failures concern mesh import, animation clips/graphs, property animation and
  decal atlas APIs (`Workspace/editor-asset-plugin-textures-build.log`). Texture
  document, property, manager and target sources also compile individually with
  Qt. Full Editor linking and GUI workflows remain pending.
- Mesh-import investigation exposed a native meshlet correctness issue: whole-buffer
  clustering crossed material sections, assigned material zero and inferred section
  ranges from reordered primitive offsets. Meshlet construction now has an explicit
  range API returning xiiResult, clusters each section independently, preserves
  material/section/LOD metadata and emits contiguous LOD spans. The builder resides
  in its own source, reads vertices once and reuses scratch storage across ranges;
  section identity matching uses hash buckets instead of repeated full scans.
  Invalid geometry clears generated output and propagates missing-resource state
  through creation and UpdateContent. Shared GPU buffers retain existing ranges;
  empty and point/line meshes remain supported. The native binary schema is unchanged.
- All 18 GraphicsCore subtests and 19 Vulkan Editor integration subtests pass
  (`Workspace/editor-meshlet-restored-core-tests.log`,
  `Workspace/editor-meshlet-restored-editor-tests.log`). Coverage reconstructs
  generated triangles and checks coverage, winding, material isolation, interleaved
  and shared LOD geometry, UInt16/UInt32 indices, serialization, deterministic
  rebuilding, invalid ranges/layouts/indices/positions, GPU buffer creation,
  immutable resource sharing and streamed compilation failures. GraphicsCoreTest,
  EditorEngineProcessFrameworkTest and GpuDrivenScene build successfully.
- The three-frame GpuDrivenScene smoke test fails with Vulkan ErrorDeviceLost at
  SwapChainVulkan.cpp:541. A controlled comparison using the prior committed
  whole-buffer meshlet algorithm reproduces the same assertion
  (`Workspace/editor-meshlet-gpu-smoke.log`,
  `Workspace/editor-meshlet-baseline-gpu-smoke.log`). The final implementation was
  restored, rebuilt and retested afterward. This renderer failure remains unresolved;
  the smoke test and integrated GUI workflows are not claimed as passing. Qt mesh
  and animated-mesh import still require migration from the removed importer framework.

- ModelImporter now reads source files directly into current native mesh descriptors,
  with separate geometry, skinning and material implementations. No removed importer
  hierarchy or Qt dependency is restored. Instance transforms, affine shear, mirrored
  winding, shared source material slots, two UV sets, vertex colors and section-aware
  meshlets are retained. Mesh-local named bone palettes preserve inverse bind poses;
  the strongest four influences use deterministic byte quantization, with exact
  normalized sums and rigid fallback for unweighted vertices. Invalid or unsupported
  native input returns xiiResult failure and clears all output.
- Imported material metadata and embedded texture bytes are owned by the result.
  PBR factors, alpha mode/cutoff, two-sidedness, normal/occlusion strength, texture
  addressing/filtering, UV channels/transforms and layered references are retained.
  The glTF occlusion slot is identified by its strength metadata; ordinary lightmaps
  are not reinterpreted as occlusion. Source glTF UVs follow Assimp's canonical V
  conversion before the optional import flip. Conservative/aggressive simplification
  weld equivalent source corners first and compact unused vertices afterward.
- EditorEngineProcessFrameworkTest builds and all 21 Vulkan integration subtests
  pass (`Workspace/editor-mesh-import-test-build.log`,
  `Workspace/editor-mesh-import-tests.log`). Actual Unicode-path glTF and OBJ source
  fixtures cover transformed instances, PBR/embedded data, deterministic reimport,
  native serialization, skin/skeleton bind agreement, normalized/raw weights,
  invalid late input, unsupported morph/UV data, a 66049-vertex UInt32 mesh,
  triangle coverage and both simplification modes. Additional UV/color sets and
  animated morph targets are rejected because the current native rendering layout
  cannot represent them. Qt asset documents still need to adopt this reader.

- Static and animated mesh property UIs now use ModelImporter's color conversion
  enum and the current packed layout. Removed normal/UV/bone precision controls
  no longer imply unsupported runtime layouts. Shared graph patches remove those
  controls from saved documents and migrate named legacy color values (including
  the old Srgb spelling); numeric identities remain unchanged. Source filenames,
  simplification and explicit weight-normalization choices are retained. The
  animated mesh C++ default now agrees with its reflected normalized default.
- Both Qt mesh property sources compile independently with zero warnings/errors;
  all 22 Vulkan Editor integration subtests pass, including numeric/named property
  migration for static and animated assets (`Workspace/editor-mesh-properties-*.log`).
  Document transforms and material import still require migration before the full
  asset plugin can build; no integrated GUI verification is claimed.

- Material slot metadata now resides in SharedPluginAssets, with its existing
  reflected type identity and serialized properties intact. Obsolete Qt-only
  AssetUtils files are removed and consumers use the shared definition directly.
  Reconciliation follows source material labels and duplicate occurrence order,
  preserves user assignments and temporary highlights across reorderings, and
  retains index-based unnamed legacy slots. It operates in linear expected time
  with one hash table and a linked-index scratch array. Native shared material
  slots are deliberately independent of mesh section counts.
- All 23 Vulkan Editor integration subtests pass, including reorder, duplicate,
  removal, unnamed legacy, shared-slot and idempotent reconciliation checks
  (`Workspace/editor-mesh-slots-build.log`, `Workspace/editor-mesh-slots-tests.log`).
  Document and material import migration remains in progress. Investigation also
  confirms that the current binary material writer has no typed native PBR block;
  it needs migration before imported PBR fields can reach native runtime state.

- Native material assets now have version 8 typed PBR state in addition to the
  existing custom shader constants, permutations and texture bindings. Scalar
  fields, rendering domain/shading/blend/alpha modes, feature flags, sort priority
  and all eight native texture slots survive asset loading and reload. Version
  4-7 readers remain available for existing transformed content. Invalid typed
  blocks fail transactionally through xiiResult; destination write failures are
  returned without asserting stream operators on the external destination.
- Material documents offer Native mode using the existing reflected runtime
  descriptor, avoiding a second PBR property schema. Shader-file/base-material
  and visual-shader workflows retain their authored data. Native mode exposes
  the typed descriptor and hides phantom shader settings; resource dependencies
  are gathered explicitly for its nested handles and custom bindings. Document
  version 8 rebuilds transformed caches. The material document source compiles
  individually with Qt and no warnings/errors.
- All 24 Vulkan Editor integration subtests pass, including native material
  equality, every-byte typed-block truncation, unknown enums, non-finite input,
  failing destination streams, resource loading, custom constants, Unicode texture
  IDs and forced reload (`Workspace/editor-native-material-build.log`,
  `Workspace/editor-native-material-tests.log`). The legacy generic shader payload
  reader still uses its original stream checks; this slice validates the new typed
  block. Mesh material import is not yet connected to Native mode.

- Texture documents now support custom native color-texture filtering: independent
  minification, magnification and mip filters, anisotropy, LOD bias and LOD bounds.
  Existing sampler presets and their identities remain unchanged; older documents
  retain preset behavior. Both converted textures and render targets use the same
  validation. Texture document version 9 invalidates transformed caches. All three
  affected Qt source files compile individually with zero warnings/errors.
- Converter sampler overrides parse signed finite values and reject malformed
  values, unknown filters, mixed anisotropic filtering and invalid LOD intervals
  through xiiResult. The native W address is now retained for 2D textures as well
  as volumes. Actual GPU tests discovered that Vulkan had never enabled the
  extension required by the already exposed MirrorOnce mode. It now enables that
  extension when available and returns failure on unsupported devices.
- All 24 Vulkan Editor integration subtests pass, including all twelve glTF
  minification/magnification combinations, base-level-only sampling with retained
  mip images, custom anisotropy/LOD bounds, native GPU sampler round trips and
  eleven invalid converter invocations (`Workspace/editor-custom-sampler-build.log`,
  `Workspace/editor-custom-sampler-tests.log`). The complete Editor plugin remains
  subject to the other documented asset migrations.

- File-backed native mesh resources now use the same standard asset-envelope
  loader as native animation resources. Custom memory loaders still supply raw
  descriptors. The loader checks the correct 16-bit mesh / 32-bit animation
  descriptor version and retains file timestamps for resource reload detection.
  Mesh reloads publish uniquely identified immutable GPU buffers, so cached or
  retained previous buffers cannot mask new geometry. All 24 Vulkan Editor
  integration subtests pass, including disk loading, forced reload, changed
  bounds, distinct GPU buffers and retained previous-buffer validity
  (`Workspace/editor-native-mesh-documents-build.log`,
  `Workspace/editor-native-mesh-documents-tests.log`).

- Texture assets expose three additional channel mappings that pack independent
  red-channel factors into green and blue, with neutral white for absent inputs.
  Existing mapping identities remain unchanged. Both source images remain asset
  dependencies; no detached packed-image derivative is generated. Texture
  document version 10 invalidates transformed caches. The reflected properties
  and document transform compile individually with Qt, and converter integration
  tests verify exact linear channel values for all three mappings (included in
  the 24 passing Vulkan Editor integration subtests above).

- Static and animated mesh documents now transform through the native mesh
  importer and packed resource descriptor. Procedural meshes use the same layout
  and section-aware meshlet compilation. Simplification, root transforms, skin
  weights, material labels and default skeleton references use current APIs.
  Slot changes are staged and background transforms request foreground import
  instead of modifying documents. Binary output failures propagate as results.
- Imported materials use Native mode and typed PBR fields, exact texture samplers,
  source-relative/embedded images, independent roughness/metallic channel packing,
  normal/occlusion strengths and correct alpha modes. Explicit user assignments
  are preserved. Unique sanitized names prevent duplicate material overwrites;
  failed generated documents are closed and removed rather than reused as blank
  assets. Unsupported layered or per-texture UV semantics fail with an actionable
  status instead of being silently flattened. Separate factor maps must share a
  sampler, matching the runtime's single metallic/roughness texture binding.
- All six affected Qt source files compile independently with zero warnings/errors.
  The full asset plugin build compiles both mesh documents and shared import
  utilities, but still fails in animation clip, animation graph, property animation
  and decal atlas integrations (`Workspace/editor-native-mesh-plugin-build.log`).
  All 24 Vulkan Editor integration subtests pass; no complete GUI import workflow
  is claimed while those plugin-wide migrations remain unfinished.

- The broader Vulkan GraphicsFoundation regression suite builds and all 40
  subtests pass, including real offscreen/pipeline execution and BLAS/TLAS
  creation (`Workspace/editor-native-sampler-foundation-build.log`,
  `Workspace/editor-native-sampler-foundation-tests.log`). Its first run failed
  TLAS creation and then dereferenced null. The device reported ray tracing
  enabled; rebuilding all Vulkan translation units resolved the creation failure
  without a backend source change (`Workspace/editor-native-sampler-vulkan-rebuild.log`).
  The TLAS test now guards initialization and creation failures, preserving the
  failing assertion while allowing useful diagnostics instead of a secondary
  null-pointer crash. Other backends and complete Editor GUI workflows remain
  unverified. The previously recorded GpuDrivenScene device-lost smoke failure
  has not been resolved or reclassified by this slice.

- Native additive clips now initialize unanimated joint transforms to identity,
  so rest transforms are not applied twice during additive blending. Animation
  preview composes additive deltas over the skeleton rest pose before building
  its palette. Existing absolute clips retain rest-pose defaults. Direct native
  resource sampling and sparse/reordered preview palettes are covered by the
  passing Vulkan Editor suite (`Workspace/editor-animation-import-tests-build.log`,
  `Workspace/editor-animation-import-tests.log`; 25 subtests including the native
  animation reader under development).

- Native animation clip runtime hashes now include all key values/times and events,
  instead of only track key counts. File-backed forced reload tests distinguish
  edits with identical key counts and event-only edits, while checking loaded data.
  All 25 Vulkan Editor integration subtests pass
  (`Workspace/editor-animation-import-tests-build.log`,
  `Workspace/editor-animation-import-tests.log`).

- The ModelImporter animation reader now produces native named-joint clip
  descriptors through Assimp, with metadata-only clip discovery, selected-clip
  import, frame trimming and first-pose additive deltas. Linear keys retain
  authored times; STEP boundaries use consecutive native float times; cubic
  Hermite translation/scale and quaternion curves bake at the configured FPS
  while retaining source keys. Runtime sampling resolves names against reordered
  skeletons. Ambiguous names, malformed transforms/keys, morph/vertex channels,
  per-channel extrapolation/repeat and excessive output fail with xiiResult and
  a diagnostic. DEFAULT boundary behavior preserves rest transforms except for
  glTF's required clamping. Cubic baking is an approximation between samples.
- All 25 Vulkan Editor integration subtests pass, including Unicode source paths,
  clip discovery, trimming, one-frame clips, STEP/cubic vector and rotation
  values, additive sparse joints, failed imports and imported native asset
  file loading/sampling (`Workspace/editor-animation-import-tests-build.log`,
  `Workspace/editor-animation-import-tests.log`). No new animation format or
  joint-index dependency was introduced.

- Animation clip documents now transform with the native animation importer and
  descriptor, exposing explicit trim/bake FPS and preserving authored event order
  at equal times. Background source metadata changes request foreground import;
  foreground updates use document transactions. Document version 6 invalidates
  transformed caches. Import-all discovers labels without baking every source
  clip, sanitizes/free-selects filenames and cleans up newly created documents
  on failure before publishing output documents.
- The clip document, manager and window compile separately with Qt, zero warnings
  or errors (`Workspace/editor-animation-clip-AnimationClipAsset.cpp-build.log`,
  `Workspace/editor-animation-clip-AnimationClipAssetManager.cpp-build.log`,
  `Workspace/editor-animation-clip-AnimationClipAssetWindow.moc.cpp-build.log`).
  Full GUI import/undo verification remains pending other plugin migrations.
  Authored nonzero constant root motion is retained but explicitly prevents
  transformation: the current runtime neither stores that velocity nor consumes
  pose root motion. Timeline events serialize natively; runtime event dispatch
  remains unavailable. These are unresolved Engine capabilities, not completed
  Editor features.

- Shared document-to-native property copying now accepts an empty delegate as its
  default and actually honors an explicit property filter. The default retains
  the established exclusion of hidden properties. The default-call compilation
  failure was exposed by native animation graph authoring. All six DocumentObject
  subtests pass, including copied and filtered values
  (`Workspace/editor-native-animation-graph-document-tests.log`). The broader
  ToolsFoundation run passed AnimationAuthoring and DocumentObject, but failed
  FileSystemModel watcher/deletion assertions, including unsafe indexing after
  missing events (`Workspace/editor-native-animation-graph-tools-tests.log`).
  That independent failure remains under investigation.
