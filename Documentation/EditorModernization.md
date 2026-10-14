# Editor modernization

This is an ongoing migration against the current Engine, not a completed-work report.
The authoritative objective is comprehensive Editor and tooling modernization,
including integrated workflows and compatibility. Passing library builds alone
does not satisfy it.

## Native GPU mesh identity for SceneView picking

- Scene icon captures now use one internal `xiiScenePickingReadback` transport,
  which also reads the native packed RGBA32UInt mesh query output. Native owner
  validation remains with each consumer. Three slot tables pair images with their
  original metadata; completed slot metadata is consumed before another capture
  can reserve that slot. Same-frame reservations remain distinct. Reservations
  from graphs that did not execute are released at the next frame boundary, using
  the render graph's full 64-bit setup frame identity. Pending captures are drained
  before resizing or changing formats; no GPU submission or wait occurs in the
  transport. Overflow checks bound host pixel storage, and view/camera/target
  snapshots continue to reject stale queries.
- Windows Debug builds pass for EditorEngineProcessFrameworkTest,
  EditorEngineProcess and GraphicsCoreTest. All 41 Vulkan engine-process tests and
  all 18 GraphicsCore tests pass. The native mesh fixture fills all three capture
  slots, verifies fourth-capture backpressure, and compares asynchronously
  published packed pixels with GPU readback. Existing icon picking, marquee,
  resizing, camera/target invalidation and per-view isolation coverage passes
  through the consolidated transport. Evidence:
  `Workspace/editor-native-picking-readback-build.log`,
  `Workspace/editor-native-picking-readback-tests.log` and
  `Workspace/editor-native-picking-readback-core-tests.log`.
- Native `xiiView` projections now consistently use ZeroToOne reversed Z, matching
  depth prepasses, G-buffer rasterization and surface queries. Core cameras retain
  their conventional projection contract. Both eyes, including asymmetric stereo
  matrices, are converted when view matrices are cached. Camera replacement also
  invalidates caches even if the replacement's modification counters match.
  `xiiViewData` query implementation is separated into its own source file: rays
  start on the near plane and use an interior depth to support infinite far planes.
  Invalid/non-finite queries return `xiiResult` failure without changing outputs.
  Texture preview shaders consume the native projection directly; their previous
  local depth reversal was removed to preserve nearest cube-sphere coverage.
- Windows Debug dependent builds pass for GraphicsCoreTest and the engine-process
  tests. All 18 Core tests and all 41 Vulkan engine-process tests pass, including
  perspective/orthographic, negative orthographic near planes, asymmetric stereo,
  infinite far planes, camera replacement and invalid query tests. Native mesh
  raster readback now uses an actual `xiiView` projection. Cube preview face and
  resource reload tests also pass. Evidence:
  `Workspace/editor-native-view-projection-build.log`,
  `Workspace/editor-native-view-projection-tests.log` and
  `Workspace/editor-native-view-projection-core-tests.log`.
- A dedicated native surface-query raster now consumes the existing compact
  visibility stream, geometry residency and material tables. It produces one
  RGBA32UInt target containing native identity, depth bits and oct-encoded smooth
  geometric normal bits, plus its transient reversed-Z depth buffer. It needs no
  G-buffer or scene depth prepass. Callers choose transparent visibility separately
  from the main scene; zero-opacity translucent pixels and masked alpha holes
  reveal the surface behind them. G-buffer and picking share canonical mesh
  emission and material coverage shader code. `AddPickingPass` uses `xiiResult`,
  rejects invalid descriptions and clears its output on failure. Shader lifetime
  remains subsystem-owned; no Editor or Qt dependencies enter GraphicsCore.
- The Windows Debug engine-process target builds and all 40 Vulkan tests pass
  after this addition. Actual GPU readback verifies opacity-independent opaque
  coverage, masked rejection/acceptance, translucent rejection/acceptance and
  transparent visibility exclusion, including native component resolution and
  nearest-surface depth. It also verifies the packed normal and invalid inputs.
  The direct-graph fixture uses its own native completion tracker, independent of
  the registered graph manager's submission timeline. Evidence:
  `Workspace/editor-native-mesh-query-build.log` and
  `Workspace/editor-native-mesh-query-tests.log`.
- The native mesh-shader G-buffer now writes a context-local, generation-checked
  scene identity. Zero denotes background; application render IDs remain separate
  user data. This replaces the unused GPU object-index field without changing the
  GPU record size. Scene hot storage retains handles in place of its alive byte,
  adding three bytes per allocated object slot. Compaction preserves identity;
  removal and clearing advance the existing Foundation ID-table generations.
- Mesh synchronization keys packets by native object/component handles, source
  render ID and section. Different components or sections with colliding source
  IDs no longer overwrite one another. Shared geometry and material acquisition
  remain with the existing managers. Clearing a database also causes stale
  extraction entries to be retired and recreated on the next synchronization.
- `xiiSceneMeshSource` and `ResolveExtractedMesh` resolve native GPU identities to
  non-owning object/component handles and mesh sections in expected O(1), without
  allocating query containers. Lookup state belongs to the scene context and is
  retired with extraction entries. `xiiView::ResolveRenderedMesh` additionally
  validates live, active owners and view include/exclude tags. Resolution returns
  `xiiResult` and clears output on failure. Implementation is split between scene
  synchronization, source resolution and view validation.
- IDs are runtime identities, not asset identifiers. They must be paired with
  the producing context/view. The existing 24/8-bit Foundation handle has finite
  generation wraparound; asynchronous consumers must bound capture lifetime and
  validate their view/capture state, camera and resource changes. Querying requires
  a CPU frame boundary and world access; concurrent mutation is unsupported.
- Windows Debug builds pass for GraphicsCoreTest, EditorEngineProcessFrameworkTest,
  EditorEngineProcess and EditorFrameworkTest. All 18 GraphicsCore tests, all 40
  Vulkan engine-process tests and all 6 EditorFramework tests pass. The new test
  executes native geometry upload, visibility, depth and G-buffer rasterization,
  reads back the actual R32UInt result, and resolves the visible mesh to its native
  component. It also checks background zero, colliding component/section IDs,
  packet retirement, database clearing, invalid contexts and non-extracted scene
  objects. The initial raster fixture supplied a reversed-Z matrix; it now exercises
  the native view's projection as described above.
  Evidence: `Workspace/editor-scene-mesh-source-verified-build.log`,
  `Workspace/editor-scene-mesh-source-tests.log`,
  `Workspace/editor-scene-mesh-source-core-tests.log`,
  `Workspace/editor-scene-mesh-source-editor-build.log` and
  `Workspace/editor-scene-mesh-source-editor-tests.log`.
- This checkpoint supplies the native identity path for geometry picking. It
  does not complete SceneView's readback/IPC migration or establish a successful
  full Scene plugin build. SceneView query integration, selected-object click-through,
  selection overlays, indexed-fallback picking, full viewport integration and
  integrated Qt interaction still require verification/migration.
  The existing SceneView compile blocker documented below remains outstanding.

## Native skeleton pose authoring and simulation pullback

- Native `xiiSkeletonPoseComponent` now reflects a sparse `BoneTransforms` map of
  absolute runtime local transforms, plus its mesh-application setting. Overrides
  are keyed by joint name, survive skeleton reordering/reload, and leave unknown
  names available for later resource revisions. Removing an override restores the
  joint's rest pose. The pose manager also evaluates while simulation is disabled,
  so document edits reach the native pose without entering play mode.
- `SetLocalPose` validates a complete indexed input and binds it to the currently
  loaded skeleton's names before replacing overrides. Invalid inputs return
  `xiiResult` without modifying authored data. Component binary version 2 saves
  authored overrides; version 1 indexed poses resolve once against their skeleton.
  Unresolved version 1 data is retained when saving before resource availability.
  Pose implementation and mesh-palette construction have separate source files.
- Both native animation and pose components expose evaluated pose snapshots by
  joint name. Snapshots reject uninitialized poses and changed skeleton revisions,
  including reloads that preserve joint count. Transforms, names and pose counts
  are validated before a snapshot is returned. Runtime code has no Qt or exposed
  bone metadata dependency.
- Mesh palette mapping moved from preview-only code into GraphicsCore. Native
  components and previews now share resolution of mesh bone names, palette indices
  and mesh inverse bind poses. Sparse unused slots are identity; malformed mappings
  clear the output. Native mesh application reuses palette/scratch capacity and
  clears failed skinning output without discarding independent morph weights.
- Bone gizmos resolve the existing asset metadata hierarchy into native coordinates,
  baking import coordinates into default root joints once and preserving reflected
  scale signs. Native overrides already include those coordinates. Synthetic import
  metadata is not exposed as an editable joint. Rotation edits preserve translation
  and nonuniform scale and insert missing overrides through the command accessor.
  Manipulators now observe collection insertion/removal as well as value changes.
  Skeleton curator notifications defer metadata refresh to redraw, with owned event
  subscriptions. Exposed-bone metadata remains a tooling/asset format, not runtime
  component storage.
- Scene simulation pullback uses native component snapshots instead of the removed
  animation message. IPC carries skeleton identity, and layered scene documents
  apply transforms only to a matching skeleton component. Failed pose commands
  cancel the layer transaction. Animated components take precedence over static
  pose components when collecting an object's pose. Scene context teardown now
  resets UUIDs using the current API and relies on world-owned registered views
  rather than the removed per-frame game-state view submission call.
- Windows Debug builds of EditorFrameworkTest, EditorEngineProcessFrameworkTest
  and EditorEngineProcess pass. All 6 EditorFramework tests and all 39 Vulkan
  engine-process tests pass, including real document-to-world pose mirroring,
  insert/remove undo/redo, simulated curator refresh notifications, skeleton
  resource reload/reordering, component version 1 loading, version 2 world round
  trips, IPC serialization and reordered sparse mesh palettes. Production
  Scene2Document and SceneContext selected compiles pass with zero warnings/errors.
  Evidence: `Workspace/editor-native-pose-verified-build.log`,
  `Workspace/editor-native-pose-editor-tests.log`,
  `Workspace/editor-native-pose-engine-tests.log`,
  `Workspace/editor-native-pose-Scene2Document-compile.log` and
  `Workspace/editor-native-pose-SceneContext-compile.log`.
- Integrated Qt gizmo interaction, real curator-file reload notification delivery,
  play-the-game window rendering, the complete scene-plugin link and other
  platforms/backends remain unverified. SceneView still requires native picking
  integration; property animation, GreyBox authoring and legacy lighting asset
  migration remain separate outstanding work. This checkpoint does not establish
  a successful full Editor application build or launch.
  Full EnginePluginScene and EditorPluginScene builds were attempted after this
  checkpoint; both stop at EnginePluginScene's SceneView dependency on the removed
  `GraphicsCore/RenderWorld/RenderWorld.h`. That file also retains obsolete
  pass-property and readback picking APIs. Logs:
  `Workspace/editor-native-scene-plugin-build.log` and
  `Workspace/editor-native-scene-editor-plugin-build.log`.
- Follow-up review invalidates snapshots immediately when changing clips or
  animation graphs. Failed clip acquisition clears the evaluated pose instead of
  marking previous animation data as current. The regression test changes and
  removes a real clip and verifies snapshots fail until the next evaluation.
  Both test targets rebuild successfully and all 39 Vulkan engine-process / 6
  EditorFramework tests pass again. Evidence:
  `Workspace/editor-native-pose-reconfiguration-build.log`,
  `Workspace/editor-native-pose-reconfiguration-tests.log` and
  `Workspace/editor-native-pose-reconfiguration-editor-tests.log`.

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

- Native animation graph authoring now uses shared tooling. Shared tooling
  now owns reflected node settings, pose pin creation, document compilation and
  native descriptor validation. Qt retains the canvas, property presentation and
  document shell. The six evaluated native node kinds are exposed; IK enum values
  that currently fall through to rest pose are not advertised as implemented.
  Core graph topology checks disconnected cycles and evaluation depth before
  publishing a descriptor. Blend thresholds follow explicit pin order; transition
  priority follows authored array order. Document UUID order stabilizes native
  node indices across remove/undo and serialization.
- The graph test covers native asset loading, Blend1D/additive/layered/state-machine
  execution, per-instance parameters/state, invalid topology and depth. Those
  checks passed before adding document command coverage. The full 26-subtest Vulkan
  suite passed with the graph test running without an unnecessary graphics device
  (`Workspace/editor-native-animation-graph-cpu-tests.log`). Two earlier runs with
  that extra device failed at the later ViewLifecycle image allocation; isolated
  ViewLifecycle passed. This observation does not establish the underlying Vulkan
  device-lifecycle cause or resolve the earlier GpuDrivenScene device-lost issue.
- Actual document command tests exposed a null-parent dereference when adding a
  root node through xiiNodeCommandAccessor, and node removal bypassing the existing
  connection-aware removal command. Both are repaired. Pin resize
  undo/redo, DDL connections/position restoration and node remove/undo now pass.
  A hash mismatch after node remove/undo exposed unstable child enumeration;
  sorting by UUID stabilizes compiled indices. The corrected document test passes
  (`Workspace/editor-native-animation-graph-document-only.log`), and all 26 Vulkan
  integration subtests pass with this coverage
  (`Workspace/editor-native-animation-graph-document-integration.log`).
- All five graph Qt translation units compiled individually before the shared
  document compiler extraction; the document source also compiled afterward
  (`Workspace/editor-native-animation-graph-qt-build.log`). The fresh full asset
  plugin build compiled graph sources and remains blocked by removed PropertyAnim
  and TextureAtlasDesc includes (`Workspace/editor-native-animation-graph-plugin-build.log`).
  An earlier automatic approval usage limit prevented a test/build sequence; the
  normal approval path subsequently succeeded and validation above completed.
- Old pin-based animation graph documents are currently rejected before phantom
  type registration, preserving the original file instead of dropping unsupported
  data on save. This is a temporary explicit compatibility gap: migration of
  supported legacy pose operations remains required. Included graphs, trigger and
  numeric pin semantics have no current native equivalent. Native root motion,
  event dispatch and repeated-subgraph evaluation costs also remain open
  Engine/tooling concerns. The graph migration and overall
  Editor modernization are not complete.

- Native animation graph instances now observe the ResourceManager change counter
  before evaluating. Reload discards indexed active/previous states, in-flight
  blends and playback time while retaining per-instance bool/float parameters.
  Identical-content reloads also reset playback; assigning the same handle does
  not. Runtime instance code is separated from descriptor/resource serialization.
  CPU graph tests cover both instances, in-range stale indices, partial blends,
  time reset and explicit Reset semantics (`Workspace/editor-animation-graph-reload-tests.log`).
  The dependent integration target builds and all 26 Vulkan Editor integration
  subtests pass (`Workspace/editor-animation-graph-reload-build.log`,
  `Workspace/editor-animation-graph-reload-integration.log`).

- The FileSystemModel failure reproduces without animation tests in a fresh output
  directory (`Workspace/editor-filesystem-investigation-tests.log`). This workspace
  uses ReFS. Temporary raw watcher tracing shows case-only renames emit
  FILE_ACTION_RENAMED_OLD_NAME without NEW_NAME; folder notification completion
  waits for a subsequent operation. The mirror consequently retains the old
  spelling and asserts during deletion. Direct owned-directory probes reproduce
  this with MoveFileW, MoveFileExW and both native rename-information variants;
  ReadDirectoryChangesExW reports ERROR_INVALID_FUNCTION on this volume. This is
  an unresolved platform notification limitation, not evidence of correct external
  rename handling. Temporary tracing is removed; the test now avoids indexing
  missing event timestamps, retaining the actual event/count failures.


- Decal atlas loading now consumes four prepacked channel images with one shared
  UV layout. It uploads real pixel data and complete authored mip chains, releases
  CPU images after upload and rejects invalid metadata, channel formats and
  truncated payloads transactionally. The metadata-only runtime packing stub and
  its neutral-only GPU output are removed. Descriptor serialization, GPU upload
  and lookup implementations are separate files; normal asset files use the
  existing native descriptor loader. Per-decal asset authoring and the Editor atlas
  producer still require migration to this format.
- GraphicsCore and the dependent Editor integration target build. New atlas tests
  pass, including every file truncation, bounded DDS allocation, native file
  loading, four-channel GPU readback and reload retaining an old GPU snapshot
  (`Workspace/editor-decal-atlas-tests-build.log`, `Workspace/editor-decal-atlas-tests.log`).
  The full suite reaches 28 subtests but fails at the later ViewLifecycle Vulkan
  allocations (`Workspace/editor-decal-atlas-integration-tests.log`). Temporary
  fixture diagnostics show many device references remain after shutdown. Inspection
  identifies a strong ownership cycle between default buffer/texture views and
  their parent resources. Repair and regression coverage are in progress; the
  complete suite is not reported as passing. Temporary diagnostics are removed.

- The atlas suite's repeated-device failure is repaired at the ownership boundary.
  Default buffer/texture views are created lazily and cached without owning a
  reference; client views retain their parent resource. Cache acquisition pins
  only nonzero reference counts under the device mutex, so a concurrent final
  release cannot resurrect a destroyed view. Sampler cache acquisition uses the
  same rule. Backend view creation returns null after failed initialization.
- Independently reference-counted Vulkan/D3D12 GAL objects now use the device's
  parent allocator, consistently with reconstructed shared pointers. This
  allocator outlives device teardown; device-owned helpers retain their internal
  tracking allocator. The previous mismatch leaked proxy allocation records and
  could deallocate a final child through a proxy destroyed with its device.
- Command-list shader resource/UAV bindings retain views through the existing
  command data retirement path. Offscreen xiiView outputs retain their target
  views, and local renderer callers preserve shared ownership across use. No
  resource-to-default-view ownership cycle remains. The API documents lazy view
  identity and the requirement to retain borrowed views.
- Regression tests verify cache reuse and release, parent-resource retention,
  concurrent acquisition/final release, sampler release/recreation, fixture
  device reference balance, a final view outliving the device owner, and two
  Editor views retaining the same output after the caller releases it. All
  configured dependent targets build; all 40 GraphicsFoundation, 18 GraphicsCore
  and 28 Editor engine-process subtests pass, including the atlas reload/readback
  and formerly failing ViewLifecycle tests (`Workspace/editor-gal-ownership-build.log`,
  `Workspace/editor-gal-ownership-resources-tests.log`,
  `Workspace/editor-gal-ownership-graphics-tests.log`,
  `Workspace/editor-gal-ownership-core-tests.log`,
  `Workspace/editor-gal-ownership-editor-tests.log`). Windows Debug/Vulkan was
  exercised; D3D12 ownership code was updated consistently but is not enabled in
  this workspace and was not compiled or run. GUI workflows remain unverified.

- Per-decal resources now use checked native descriptor streams through the same
  ResourceManager file loader as their packed atlases. Serialization preserves
  the existing version-one binary field layout, reports failures with xiiResult,
  validates finite rendering parameters and checks atlas lookup identity. Reads
  publish only complete valid descriptors, with bounded UTF-8 IDs and exact
  resource types; invalid reloads clear stale atlas references. Atlas and decal
  descriptors share their private bounded metadata reader, with matching writer
  limits. Descriptor serialization and resource lifetime code are separate files.
- Tests cover all descriptor fields, optional source handles, mirrored/tiled UVs,
  every truncation, unsupported versions, wrong handle types, malformed lengths,
  invalid parameters and failed writes. Actual native decal asset files load and
  reload while retaining the packed atlas handle; malformed replacements become
  missing resources with cleared old metadata. Reload tests wait for existing
  loader task cleanup before requesting a subsequent reload, following the
  ResourceManager's existing lifecycle. All three decal/atlas subtests pass, as
  do all 29 Editor integration and 18 GraphicsCore subtests
  (`Workspace/editor-native-decal-verification-build.log`,
  `Workspace/editor-native-decal-reload-build.log`,
  `Workspace/editor-native-decal-reload-tests.log`,
  `Workspace/editor-native-decal-integration-tests.log`,
  `Workspace/editor-native-decal-core-tests.log`). This establishes the native
  resource boundary; the Qt decal atlas producer and individual asset transform
  still need migration and are not reported as complete.

- SharedPluginAssets now provides a Qt-free native decal atlas builder using the
  existing TexturePacker and image conversion utilities. Full IDs determine
  deterministic packing order; all channels share one layout, dimensions and
  authored mip count. It applies separate alpha masks, linearizes color inputs,
  converts ORM to native R/M/AO, reconstructs XY normals and safely normalizes
  degenerate normals, retains HDR emissive in finite half-float output and
  extrudes edges through padding corners. Missing optional channels use native
  neutral values. Sources and float atlas channels are processed incrementally.
- Builder limits are explicit: default 1024-pixel entries, 4096-pixel source and
  atlas extents, configurable source bounds up to 8192 and padding 1..64. It
  rejects invalid image dimensions/mips, incomplete channel inputs, non-finite
  pixels, HDR overflow, duplicate IDs and capacity exhaustion without replacing
  the previous descriptor. Empty projects produce valid neutral atlases. Output
  is uncompressed RGBA8 plus half-float emissive; format compression and multiple
  project atlases are not part of this stage. The current native renderer samples
  mip zero; filtered atlas tails must not be assumed to isolate neighboring tiles.
- Real image-file tests cover shared pixel coordinates, alpha masks, ORM channel
  order, XY normals, HDR, padding corners, neutral channels, aspect-preserving
  downscaling, deterministic order, complete mip chains, rejected inputs and
  empty projects. Generated output uploads through xiiDecalAtlasResource and
  Vulkan readback verifies the packed pixels. All 30 Editor integration subtests
  pass (`Workspace/editor-decal-builder-gpu-build.log`,
  `Workspace/editor-decal-builder-gpu-tests.log`,
  `Workspace/editor-decal-builder-integration-tests.log`). The Qt producer still
  needs to call this builder and write per-decal descriptors; that remaining
  migration is explicit and the overall Editor goal remains incomplete.


- Qt decal transforms now call the native shared atlas builder and serialize an
  individual native descriptor for every decal. Normal asset table entries map
  those descriptors to their files; the manager adds only the shared atlas alias.
  Atlas paths and build hashes respect the selected platform profile, full UUIDs
  and ordered dependency hashes. Collection releases the curator inventory lock
  before opening documents and closes documents it opened on failure or success.
- Generated atlases use deferred publication, native asset headers and the
  version-two packed descriptor; cache checks require matching native versions,
  dependency hash and a complete final marker. Regeneration explicitly reloads
  the active-profile atlas through existing engine-process resource messages.
  The document version invalidates old empty decal outputs. Legacy colorize
  settings remain authored but fail transformation with an explicit diagnostic:
  the current native decal renderer has no equivalent blending path.
- Both changed Qt translation units compile individually, and the broader
  EditorPluginAssets build compiles decal sources. That full plugin still fails
  on the removed GameEngine PropertyAnimResource dependency; no replacement
  runtime property-animation resource exists in the current Engine. Logs:
  `Workspace/editor-native-decal-plugin-build.log`,
  `Workspace/editor-native-decal-manager-compile.log` and
  `Workspace/editor-native-decal-document-compile.log`. The 30-subtest shared
  builder/resource/GPU suite reported above validates the native output boundary;
  project-wide Qt asset transformation and reload remain unverified until the
  full plugin is buildable and the Editor workflow is exercised.


- Scene geometry export now consumes native mesh components and resources through
  a Qt-free SharedPluginScene exporter. It exports LOD-zero sections, local
  instances and current skinning with corrected winding under mirrored object
  or coordinate transforms. GPU-only vertex/index buffers are read back through
  GAL commands and completed fences, without permanently retaining CPU geometry
  or constructing a second rendering pipeline. This is a synchronous tooling
  operation between frames; it is not placed in the rendering hot path.
- Collision export uses Core's static-mesh message, and navigation export uses
  its native geometry-module interface. Missing providers and unsupported
  selection queries return explicit errors. The native navigation interface has
  no object IDs, so per-object navigation export cannot be implemented through
  it. Nonzero morph weights are rejected because the native mesh exposes no CPU
  morph-delta interface. Output contains OBJ positions/faces, not material/UV/
  normal assets. Validation failures leave a previous output intact. Final I/O
  uses the existing deferred writer and reports failures; that writer does not
  provide atomic replacement on disk failure and buffers the output in memory.
- Export requests now receive an engine-result IPC message. The Editor reports
  success only after the engine has written the file, reports unavailable engine
  connections and displays export failures. The obsolete runtime exporter include
  and its integration are removed; scene export handling has its own source file.
- All 31 Editor engine-process integration subtests pass on Windows Debug/Vulkan,
  including real GPU-only readback, both index widths, selected sections, local
  instances, skinning, coordinate conversion, native collision/navigation test
  providers, invalid provider data, failed validation preserving previous output,
  deterministic output and result-message serialization. Build/test logs:
  `Workspace/editor-scene-geometry-build.log`,
  `Workspace/editor-scene-geometry-tests.log`,
  `Workspace/editor-scene-geometry-integration-tests.log`,
  `Workspace/editor-scene-geometry-final-build.log` and
  `Workspace/editor-scene-geometry-final-tests.log`.
  The scene document, export dialog and engine handler compile individually
  (`Workspace/editor-scene-geometry-document-compile.log`,
  `Workspace/editor-scene-geometry-dialog-compile.log`,
  `Workspace/editor-scene-geometry-handler-compile.log`). Selected-source checks
  use the exact backslash paths from the generated MSBuild projects and verify
  the requested translation unit appears in compiler output; earlier forward-
  slash selections compiled only the precompiled header. Both decal translation
  units were rechecked with exact selections and pass as recorded in their logs.
- The broader application build produces EditorEngineProcess and Player, but
  Editor/EditorProcessor remain blocked by PropertyAnimResource authoring and
  EnginePluginScene's removed shadow-pool, global render-world, sky-light and
  animation-message integrations. Scene view picking/selection still uses obsolete
  pass-property APIs and needs substantive migration. These failures are recorded
  in `Workspace/editor-modernization-current-build.log`. Integrated Qt workflows,
  D3D12, Linux and macOS remain unverified; the comprehensive goal is incomplete.


- Document-context allocation now lives in the engine-process framework and
  returns `xiiStatus`. Reflection registration must identify exactly one context
  for the requested type; missing/ambiguous registrations, unallocatable types,
  invalid custom allocator signatures and custom allocation rejection fail before
  registry mutation. A rejected custom allocation no longer falls back to default
  construction. Context initialization and reset also return `xiiStatus`, as
  described below, so allocation success alone is not reported as a loaded context.
- Document-open IPC now carries an explicit success flag and diagnostic. The
  Editor records a terminal failed state, reports its reason to the document UI
  and waiting transforms, and skips mirroring/selection/preview initialization on
  failure. Thumbnail requests use the same readiness check with their existing
  ten-second timeout. Normal engine shutdown now invalidates loaded document
  state, as crashes already did. Reopening/reconnecting can recover the document.
- Remote preview sends the document metadata and waits for the remote engine's
  actual acknowledgement before mirroring or activating a view. Rejection and
  timeout are reported without replacing the local document's state with remote
  failure. Mixed Editor/engine-process binaries must be rebuilt together for the
  updated internal message schema. A remote preview session itself remains
  unverified; no remote endpoint was exercised.
- `EditorFrameworkTest` is a new tooling-only test target for shared Editor
  infrastructure, independent of asset plugins and GPU backend setup. Its first
  test exercises the actual asset document's rejected-open state, diagnostic,
  sync-object behavior, recovery after restart, shutdown invalidation, missing
  connection and failed full-mirroring path without launching an engine process.
  Engine-process tests also cover reflected default/custom context allocation,
  exact alias matching, invalid requests, duplicate registrations, rejected and
  malformed allocators, allocation/registry/reset/destruction lifetime and both
  acknowledgement serialization outcomes.
- Windows Debug builds of EditorFramework, EditorEngineProcess,
  EditorEngineProcessFrameworkTest and EditorFrameworkTest pass. All 32 engine-
  process subtests pass on Vulkan, and EditorFrameworkTest passes headlessly.
  Logs: `Workspace/editor-document-context-build.log`,
  `Workspace/editor-document-context-tests.log`,
  `Workspace/editor-document-context-integration-tests.log`,
  `Workspace/editor-document-status-build.log` and
  `Workspace/editor-document-status-tests.log`. These results do not establish
  integrated Editor startup or GUI workflow correctness; the broader application
  and legacy plugin blockers recorded above still need migration.


- Context initialization, initialization hooks, registry insertion and reset now
  propagate `xiiStatus`. Failed initialization runs partial-state teardown, removes
  its temporary registry entry and leaves the allocation with its caller. The
  engine application releases failed allocations and destroys contexts whose
  reset failed before sending the failure acknowledgement. Duplicate/invalid
  registration and double initialization leave the existing context intact.
  Deinitialization is idempotent, including after failed initialization or reset.
- All asset context hooks use the result contract; the empty render-pipeline hook
  was removed. Scene layers reject missing/invalid parent-scene metadata and
  unavailable parent worlds before borrowing anything. Scene teardown clears its
  transient layer/context/tag lists, and layer removal invalidates visibility
  bookkeeping and falls back to the owning scene if it removed the active layer.
  The removed shadow pool's whitelist calls and includes are deleted; this does
  not establish native layer visibility or shadow filtering, whose renderer
  integration remains an outstanding scene migration requirement.
- Initialization tests exercise real world creation and destruction, partial-state
  cleanup, a second initialization attempt, duplicate registration, failed reset,
  registry ownership, repeated teardown and borrowed-world preservation. Live
  worlds are counted through occupied world slots because GetWorldCount includes
  vacant slots. The gizmo test fixture now follows the actual context initialization
  lifecycle before creating synchronized handles.
- EditorEngineProcessFrameworkTest, EditorFrameworkTest and EditorEngineProcess
  build on Windows Debug. All 33 engine-process subtests pass on Vulkan, and the
  Editor document lifecycle test still passes headlessly. The layer translation
  unit compiles with its real generated-project source selection. Logs:
  `Workspace/editor-document-initialization-build.log`,
  `Workspace/editor-document-initialization-tests.log`,
  `Workspace/editor-document-initialization-integration-tests.log`,
  `Workspace/editor-document-initialization-editor-tests.log` and
  `Workspace/editor-document-layer-compile.log`.
- A full EnginePluginScene build still fails on its removed extractor base,
  animation-message declarations and global render-world header. The scene
  context's changed teardown path is therefore not runtime-verified; the layer
  source check and generic context lifetime tests do not substitute for integrated
  scene/layer tests. Current diagnostics are recorded in
  `Workspace/editor-scene-context-current-build.log`. Full Editor/Processor builds,
  integrated GUI workflows and non-Vulkan/non-Windows configurations remain
  unverified or blocked as recorded above. The comprehensive goal remains active.

- Native view extraction now applies active-object and include/exclude tag filters
  before static-cache reuse. Include tags match any tag (an empty set includes all
  active objects); any exclusion wins. Tags are evaluated per object without
  inheriting parent tags, while activation follows the World hierarchy. This
  fixes the underlying rendering path required by Editor hidden objects, hidden
  layers and orthographic exclusions; the legacy scene plugin still needs its
  remaining rendering integrations migrated before those GUI workflows can be
  verified.
- A Vulkan integration test exercises two real world-local views through normal
  extraction and graph execution. It covers static-cache reuse, uncached packets,
  independent filters, include OR, exclusion precedence, filter changes, object
  tag changes, hide/unhide and hierarchy activation. The rendering graph uses a
  simple clear pass to isolate extraction from scene shading. Windows Debug builds
  of EditorEngineProcessFrameworkTest and EditorEngineProcess pass, and all 34
  engine-process subtests pass on Vulkan. Evidence:
  `Workspace/editor-scene-visibility-build.log` and
  `Workspace/editor-scene-visibility-tests.log`.

- Native camera managers now retain the view-event subscription ID. The event
  passes its value by copy while the callback takes a const reference; the
  converted delegate is not comparable and cannot be removed by reconstructing
  a member-function delegate. Teardown removes the original subscription and
  does not create a render module. A Vulkan regression test covers both module
  creation orders, explicit camera-manager deletion, view creation afterward,
  manager recreation and whole-world teardown. It passes together with the new
  camera-preview test in `Workspace/editor-camera-preview-tests.log`; the
  dependent Windows Debug engine-process build also passes.

- Textured screen rectangles accept the existing `xiiDebugRendererDepthMode`.
  The default remains depth-tested; explicit overlays use the native cached
  overlay depth state and normal debug upload/visualization passes. This is
  needed for camera previews over native scene depth, where a depth-tested
  rectangle at zero disappears. Vulkan pixel tests verify depth rejection,
  overlay visibility and one-frame consumption, and the camera-preview test
  verifies a partial rectangle composited into an actual viewport target.
  The three focused tests pass in `Workspace/editor-camera-preview-tests.log`;
  dependent engine-process targets build in
  `Workspace/editor-camera-preview-final-build.log`.

- Selected-camera previews now use `xiiSceneCameraPreview`, with a separate
  world-local view, camera, target and native pipeline instance per scene viewport.
  Pipeline definitions are referenced through the owning viewport's resource
  handle. Projection, exposure, render scale and camera tags are updated through
  current Engine APIs, including cameras whose usage hint is None. Camera changes
  invalidate temporal history. Failed updates suspend the preview; viewport
  redraw consumption suspends auxiliary views through the virtual view-context
  hook. Teardown destroys the preview view before its world. The old selected-
  objects extractor and globally shared `EditorCameraRT` resource are removed.
- Preview allocation initializes a black image, so composition never samples an
  uninitialized target. The fixed 256x144 overlay samples the most recently
  rendered preview. Stereo uses the previous mono perspective fallback rather
  than claiming to preview an HMD. Camera selection/visibility and suspension
  are isolated in `SceneViewCameraPreview.cpp`, which compiles against current
  headers. Source registration follows the repository's CMake source discovery.
- GPU tests cover independent targets/cameras with a shared pipeline resource,
  projection/exposure/scale/tags, actual resource-driven graph execution and
  readback, initialized contents, suspension/resumption, native viewport overlay
  composition and one-frame consumption, invalid settings, cross-world rejection,
  externally destroyed/recreated views and destruction. The test pipeline uses
  a reflected color-clear pass plus native debug passes to isolate preview
  ownership/composition from scene shading. Full scene shading and GUI camera
  selection are not established by these tests.
- Windows Debug builds of EditorEngineProcessFrameworkTest, EditorFrameworkTest
  and EditorEngineProcess pass. All 36 Vulkan engine-process subtests and the
  headless Editor document-lifecycle test pass. Logs:
  `Workspace/editor-camera-preview-integration-build.log`,
  `Workspace/editor-camera-preview-final-tests.log`,
  `Workspace/editor-camera-preview-editor-tests.log` and
  `Workspace/editor-scene-camera-preview-compile.log`.
- The full scene-plugin build still fails in its legacy shape-icon extractor,
  animation-message include and global render-world include. Picking, selection
  overlays, shape icons and the remaining scene animation/lighting integration
  still require migration; the new preview controller's isolated source compile
  does not substitute for a linked scene-plugin or integrated Editor test.
  Current diagnostics: `Workspace/editor-scene-camera-current-build.log`.
  The broader modernization goal remains active; GUI and other backend/platform
  configurations remain unverified.

- Shape icons now use the reflected `xiiSceneShapeIconPass` in SharedPluginScene,
  with separate collection, graph rendering and pipeline-definition sources.
  Its resource definition inserts into the native default pipeline before Debug;
  scene view creation selects this resource through the world-local render module.
  Thumbnail/preview views pass through without icons. The obsolete sprite-based
  shape-icon extractor and its SceneContext property are removed.
- Collection includes objects without render data, respects active state, view
  include/exclude tags, Editor/EditorHidden tags and HideShapeIcon, and emits one
  icon per object using the first active component with an available icon. Gamma
  color properties retain priority over linear properties and category colors.
  Reflection pointers exist only during collection, so plugin reload cannot leave
  a persistent RTTI/property cache dangling. ResourceManager owns icon textures.
- Projected-quad culling retains partially visible icons. Perspective and
  orthographic sizing preserve world size and the pixel cap. Camera-space depth
  orders far-to-near composition independently of projected depth encoding.
  Contiguous textures are drawn as instanced batches; per-resource import names
  prevent different icon textures from aliasing in the render graph. Graph-managed
  clears preserve the resource states declared to the graph on Vulkan.
- One shader writes display color and R32UInt picking IDs with identical alpha
  and scene-depth coverage, including depth textures at a different render
  resolution. Empty frames clear IDs; disabled/non-Editor views expose no picking
  texture. IDs index the pass's current-frame object/component handle records.
  Asynchronous consumers must capture those records and validate world handles;
  the legacy scene picking message/readback-property integration is not yet
  migrated. `xiiView` exposes read-only pipeline instance inspection at CPU frame
  boundaries, with explicit borrowed-state lifetime rules.
- Vulkan tests execute the real resource-defined icon pass against controlled
  color/depth source passes and read back both color and integer IDs. They verify
  alpha holes, reflected colors, scene-depth occlusion, overlapping textures,
  instance offsets, missing component icons, visibility changes and stale-ID
  clearing, perspective/orthographic sizing, partial-quad clipping, independent
  view instances, invalid settings, disabled/preview views and serialized native
  pipeline definitions. These tests do not establish full native scene shading or
  Editor GUI picking. The pass currently targets single-sample 2D mono Editor
  views and uses independent MRT blending; other configurations are not verified.
- Windows Debug builds of EditorEngineProcessFrameworkTest, EditorFrameworkTest
  and EditorEngineProcess pass. All 37 Vulkan engine-process subtests and the
  Editor document-lifecycle test pass. SceneViewRendering.cpp compiles against
  current Engine headers. Logs: `Workspace/editor-shape-icons-final-build.log`,
  `Workspace/editor-shape-icons-final-tests.log`,
  `Workspace/editor-shape-icons-editor-tests.log` and
  `Workspace/editor-scene-shape-rendering-compile.log`.
- The full EnginePluginScene target still fails at its removed animation-message
  and global render-world headers; shape-icon extraction is no longer a build
  blocker. Selection rendering, scene picking/IPC and remaining scene animation
  and lighting integration still need migration. Current diagnostics are in
  `Workspace/editor-shape-icons-scene-build.log`. The modernization goal remains
  active, and integrated GUI / other backend and platform verification remains
  outstanding.

- Scene viewport target setup and invisible-layer updates now use the owning
  World's view under its write marker instead of the removed global RenderWorld.
  Layer changes continue updating the same native view filters consumed by scene
  extraction and shape-icon collection. `SceneViewRendering.cpp` compiles with
  zero warnings/errors (`Workspace/editor-scene-layer-rendering-compile.log`).
  The full scene-plugin build and integrated GUI layer toggles remain unverified
  while the other legacy scene integrations are migrated.

- Shape-icon picking now exposes nonblocking point and marquee queries on the
  native per-view pass. Each of three bounded capture slots pairs its GPU ID image
  with compact generational object/component handles. Queries validate the World,
  view, target, camera, viewport, live identities, visibility and object position;
  unavailable or outdated images return failure, and background returns an empty
  result. Marquee results contain unique objects in first visible pixel order.
  `Editor.Picking` disables capture and invalidates pending results. Resizing drains
  old captures without waiting before replacing the staging pool, so repeated
  resizes cannot accumulate staging textures. Production code neither submits
  command lists nor waits on the GPU for these queries. Captures may lag a frame.
- The shared GAL readback helper now reports invalid requests and unavailable
  staging/fence resources with `xiiResult`; thumbnail capture propagates enqueue
  failure. Vulkan tests cover rejected regions/subresources followed by successful
  pixel readback and staging reuse. Shape-icon tests cover point/marquee coverage,
  independent views, camera/viewport/target changes, deleted/reused handles,
  visibility changes, capture disable/re-enable, repeated resizes and destruction
  with an outstanding capture. All 37 engine-process subtests and the focused GAL
  readback test pass. Dependent EditorFrameworkTest and EditorEngineProcess builds
  pass. Logs: `Workspace/editor-icon-picking-all-tests.log`,
  `Workspace/editor-icon-picking-readback-tests.log`,
  `Workspace/editor-icon-picking-build.log` and
  `Workspace/editor-icon-picking-dependent-build.log`.
- Scene picking IPC, mesh/transparent picking and selection overlays still need
  migration. These icon queries establish real GPU coverage and identity handling;
  they do not establish an integrated Editor selection workflow or GUI behavior.

- Scene culling uses the current `CullingOnly` camera usage and the Editor-owned
  `Editor.Scene.UseCullingCamera` CVar instead of the removed render-pipeline CVar.
  It copies validated component settings under World access and falls back to the
  rendering camera when the authored camera is missing, inactive, deleted or
  malformed. Camera pose/projection/exposure validation is shared with selected
  camera previews; no component pointers persist between updates. Native primary
  GPU visibility now uses `xiiView::ComputeCullingFrustum`, matching the indexed
  visibility path's support for an explicit culling camera. Hi-Z, LOD and cone
  facing retain their raster-camera inputs.
- Camera view creation now skips invalidated identities in compact component
  storage, which can contain deleted cameras until World cleanup. Creating a
  preview before that cleanup previously queued an invalid handle and asserted
  during the next camera-manager update. The regression test retains that exact
  deletion/view-creation sequence. Camera tests also verify culling enable/disable,
  missing/deactivated/deleted camera fallback, malformed settings, actual frustum
  inclusion/exclusion and render-camera isolation, alongside real preview GPU
  execution/readback. These checks do not establish a full native scene-shading
  image with an alternate culling camera or an integrated Editor GUI workflow.
- Windows Debug builds of EditorEngineProcessFrameworkTest, EditorFrameworkTest,
  EditorEngineProcess, GraphicsCoreTest and GraphicsFoundationTest pass. All 18
  GraphicsCore subtests, all 40 GraphicsFoundation Vulkan subtests, all 37 Vulkan
  engine-process subtests and the Editor document-lifecycle test pass.
  `SceneViewRendering.cpp` compiles with zero warnings/errors. Logs:
  `Workspace/editor-scene-camera-settings-build.log`,
  `Workspace/editor-scene-culling-core-build.log`,
  `Workspace/editor-scene-culling-core-tests.log`,
  `Workspace/editor-scene-culling-foundation-tests.log` and
  `Workspace/editor-scene-culling-camera-compile.log`; final engine-process and
  Editor logs are `Workspace/editor-scene-camera-settings-final-tests.log` and
  `Workspace/editor-scene-camera-settings-editor-tests.log`. The full scene-plugin
  migration and integrated GUI / other platform and backend verification remain
  outstanding.
- The current full scene-plugin build still stops at the removed
  `GraphicsCore/AnimationSystem/Declarations.h` and
  `GraphicsCore/RenderWorld/RenderWorld.h` includes
  (`Workspace/editor-scene-camera-settings-scene-build.log`). Culling no longer
  refers to the removed render-pipeline CVar; remaining scene picking/selection,
  animation and lighting integrations must be migrated before claiming a linked
  scene plugin or functional integrated Editor workflow.

## Native scene preview lighting and immediate export

- Scene editing and thumbnail lighting now use `xiiScenePreviewLighting`, backed
  by native `xiiSkyAtmosphereComponent` and `xiiDirectionalLightComponent` objects.
  The procedural atmosphere replaces the obsolete global cubemap skylight helper;
  it uses the lowest atmosphere priority, allowing authored skies to win through
  the native renderer's normal priority ordering. The directional fill retains
  the previous orientation and explicitly uses 10 lux. This does not reinterpret
  finite-volume reflection captures as global environment lighting.
- The helper owns only generational World object handles. Creation is deferred
  for empty scenes unless thumbnails force it; repeated creation updates export
  tags without duplicating helpers. Missing objects or components are repaired.
  Scene deinitialization destroys the controller before its World, and thumbnail
  cleanup deletes untagged helpers immediately. Scene-specific integration lives
  in `SceneContextLighting.cpp`; no Qt or renderer compatibility API was added.
- Immediate-export tests exposed a Core bug: hierarchy traversal visited deleted
  objects whose transformation storage had not yet been reclaimed. Both traversal
  orders now reject invalidated identities. Core regressions cover static and
  dynamic hierarchies, deleted roots and children, live inactive objects, and
  World serialization before any cleanup update. This prevents removed thumbnail
  lighting from appearing as empty objects in an immediate scene export.
- All 21 Core subtests, all 38 Vulkan engine-process subtests, all 18 GraphicsCore
  subtests and the EditorFramework document-lifecycle test pass. The lighting test
  exercises forced/deferred creation, tag changes, export exclusion, immediate
  deletion/export, external deletion repair, native render-data extraction,
  authored atmosphere priority, independent view filters, World clear and
  controller destruction. Its render graph uses a clear pass to isolate extraction
  and ownership; a full shaded-scene image and integrated GUI lighting toggle have
  not been verified.
- Windows Debug builds of CoreTest, EditorEngineProcessFrameworkTest,
  EditorEngineProcess, EditorFrameworkTest and GraphicsCoreTest pass.
  `SceneContextLighting.cpp` compiles with zero warnings/errors. Evidence:
  `Workspace/editor-preview-lighting-core-build.log`,
  `Workspace/editor-preview-lighting-core-tests.log`,
  `Workspace/editor-preview-lighting-all-tests.log`,
  `Workspace/editor-preview-lighting-dependent-build.log`,
  `Workspace/editor-preview-lighting-editor-tests.log`,
  `Workspace/editor-preview-lighting-graphics-tests.log` and
  `Workspace/editor-preview-lighting-scene-compile.log`.
- The complete scene plugin still requires native animation state pullback and
  scene picking/selection migration. Scene authoring's default sky creation and
  obsolete skylight property metadata also remain to be migrated; the preview
  helper does not convert existing scene assets. Other platforms/backends and
  integrated Editor GUI workflows remain unverified.
  `Workspace/editor-preview-lighting-scene-build.log` confirms that the full build
  still stops at the removed animation declarations and global RenderWorld headers.

## Native scene authoring, reflection volumes and visualizer lifetime

- New scene creation now uses `xiiCreateDefaultScene` through the document object
  accessor. The initial camera shortcut/thumbnail usage, sphere asset and position,
  sky tag and light orientation are retained. Native atmosphere and directional
  light components replace the removed skybox/skylight types; the light explicitly
  authors 10 lux. The procedural sky changes the new-scene default appearance.
  All edits form one undoable transaction and a rejected operation rolls back the
  complete setup, preserving any existing content. Setup failure propagates through
  the status-returning document factory, preventing registration or saving when
  initial content could not be created.
- EditorFramework tests execute this production setup, verify native properties,
  undo/redo, serialize and reload its document graph, and mirror it into a native
  World. Mirrored undo/redo removes/recreates the actual objects. An injected final
  light-property failure verifies rollback and preservation of existing content.
  These tests do not launch the integrated scene window or establish final shaded
  scene appearance.
- Reflection captures now expose a runtime reflection attribute describing their
  shape, half extents, radius and blend distance. The Editor-owned native adapter
  draws the box/sphere influence and inner full-weight boundary using regular
  synchronized visualizer handles. Dimensions match native local-space reflection
  sampling, including object transforms, inward blend distances and degenerate
  blend boundaries. Invalid active settings hide the visualization; unused shape
  settings do not hide a valid volume. No Qt types entered GraphicsCore.
- Shape-specific property visibility uses the native capture schema. Camera
  render-target rectangle visibility compares the current `RenderTarget` enum,
  fixing the obsolete numeric value that selected `CinematicView`. Native light
  metadata handles the actual `ShadowFadeOutRange` property instead of naming
  removed shadow/cascade settings. This does not restore the camera component's
  currently disabled render-to-texture implementation.
- Removed metadata/registration for absent skylight and greybox types and the
  unused camera-pipeline string enum, whose profile configuration and camera
  consumer no longer exist. The old box-probe adapter was replaced by the native
  capture adapter. Existing scene assets are not rewritten or automatically
  converted; legacy skybox/skylight/probe/greybox asset migration and greybox
  material-drag integration remain outstanding. Removing dead property handlers
  prevents their missing-type assertions on ordinary property-grid queries; it
  does not establish replacement runtime behavior for those legacy asset types.
- Visualizer registries retain the active selection and rebuild when factory
  creators change, respecting the factory's closest matching creator and fallback.
  Unregistering a creator destroys adapters and synchronized handles while plugin
  code is still loaded. Empty/closing selections remove cached document entries.
  Scene plugin unload now unregisters its point, spot and reflection creators.
  Tests cover late registration, derived creator replacement/fallback, independent
  documents, creator removal, synchronized-handle cleanup and cleared selections.
  Actual DLL unload/reload in the integrated GUI remains unverified.
- Windows Debug builds of EditorFrameworkTest, GraphicsCoreTest,
  EditorEngineProcessFrameworkTest and EditorEngineProcess pass. All four Editor
  authoring/lifecycle tests, all 18 GraphicsCore tests and all 38 Vulkan
  engine-process tests pass. Production `SceneDocumentManager.cpp` and
  `EditorPluginScene.cpp` compile with zero warnings/errors. The Editor tests also
  compile and exercise the production scene metadata and capture adapter units.
  Evidence: `Workspace/editor-reflection-authoring-final-build.log`,
  `Workspace/editor-reflection-authoring-final-editor-tests.log`,
  `Workspace/editor-reflection-authoring-graphics-tests.log`,
  `Workspace/editor-reflection-authoring-engine-tests.log`,
  `Workspace/editor-scene-creation-manager-compile.log` and
  `Workspace/editor-reflection-authoring-plugin-compile.log`.
- The full EditorPluginScene build remains blocked by EnginePluginScene's removed
  animation-declarations and global RenderWorld headers
  (`Workspace/editor-reflection-authoring-scene-build.log`). Native bone state
  pullback/editing, scene picking/selection,
  integrated GUI workflows and other platform/backend configurations still need
  implementation or verification. The modernization goal remains incomplete.

## Document startup and persistence failures

- `xiiDocumentManager` now accepts `xiiStatus` from document factories. All 22
  scene, asset and visual-script factory overrides use this contract. Native scene
  setup errors reach the existing caller error dialogs. Unsupported types, denied
  requests, missing files, null factory results, failed factories, failed loads and
  synchronous initial-save failures consistently return a null document. Partial
  documents are removed from the manager and destroyed through `BeforeClosing`;
  unsuccessful startup never requests a window or broadcasts `DocumentOpened`.
  Non-creatable document types may still be opened for reading.
- Partial asset documents can be destroyed without an initialized IPC mirror.
  Game-object documents initialize their subscription IDs and safely skip
  unsubscription before initialization. Scene documents use owned event
  subscriptions; layered scene teardown restores swapped storage only when that
  storage was installed. These changes support actual failure cleanup instead of
  invoking success initialization hooks just to make destruction possible.
- Document reading rejects missing headers, missing object roots and absent,
  mistyped or invalid document identities before native deserialization or clone
  processing can assert. Legacy documents with embedded headers and no `Types`
  block continue to use compiled-in RTTI. Reading such a document clears previous
  type-graph output rather than retaining another document's types. Tests load a
  legacy file, preserve identity and edited content, and save it in the current
  format. No document format version or valid asset identity was changed.
- Asynchronous initial creation continues to report scheduling success. If the
  eventual save fails, the document stays modified and the existing error log and
  document-status event report the failure. Tests exercise a real unwritable
  destination and immediate close, verifying that pending work completes and
  reports its result before document destruction.
- Windows Debug builds of ToolsFoundationTest, EditorFrameworkTest,
  EditorEngineProcessFrameworkTest and EditorEngineProcess pass. All seven
  ToolsFoundation `DocumentObject` tests and all five EditorFramework tests pass;
  the complete ToolsFoundation suite was not rerun in this checkpoint. All 38
  Vulkan engine-process tests pass after the startup/load-validation changes.
  The subsequent asynchronous-reporting addition rebuilt ToolsFoundationTest and
  EditorFrameworkTest and passed their seven/five tests again. Evidence:
  `Workspace/editor-document-validation-final-build.log`,
  `Workspace/editor-document-validation-engine-tests.log`,
  `Workspace/editor-document-async-failure-build.log`,
  `Workspace/editor-document-async-failure-tools-tests.log` and
  `Workspace/editor-document-async-failure-editor-tests.log`.
- Production scene manager, scene document and layered scene document units
  compile with zero warnings/errors. Nineteen of twenty asset factory units and
  the visual-script factory unit also compile. The property-animation factory
  remains blocked by its existing missing
  `GameEngine/Animation/PropertyAnimResource.h` dependency. Evidence is in
  `Workspace/editor-document-lifecycle-<UnitName>-compile.log`, including
  `SceneDocumentManager`, `SceneDocument`, `Scene2Document`,
  `VisualScriptClassAssetManager` and `PropertyAnimAssetManager`. These selected
  compiles do not establish a successful full plugin link or integrated GUI
  startup. Native animation/picking migration, complete plugin builds, actual
  scene-window failure handling and other platforms/backends remain outstanding.

## Native default scene selection

- Removed the bundled `Editor/DocumentTemplates/Default.xiiScene` override. It
  still authored removed skybox, skylight and sphere-probe components and obsolete
  camera properties, so the normal template-aware creation path would bypass the
  native scene factory. New scenes now use the native camera, mesh, atmosphere and
  directional-light recipe described above. Generic document-template support
  remains available for project configurations; existing authored scenes were not
  modified or automatically migrated.
- The scene-creation test mounts the shipped SDK data, exercises normal document
  creation without `EmptyDocument`, verifies that no template clone occurs, and
  saves/reloads the actual native content with stable identity and no unknown
  object types. This uses a headless document-manager fixture and the production
  scene-content helper, rather than claiming an integrated Scene2 window test.
- This test exposed the shared long-operation listener's null RTTI dereference
  when the optional scene plugin is absent. Opening and closing ordinary documents
  now checks for that plugin's scene type before testing inheritance. No removed
  scene type or compatibility alias was introduced.
- EditorFrameworkTest rebuilds and all five tests pass, including this new
  template-aware creation/load coverage. Evidence:
  `Workspace/editor-native-scene-default-build.log` and
  `Workspace/editor-native-scene-default-tests.log`. Full scene plugin linking,
  viewport appearance and integrated GUI workflows remain unverified.

## Native scene picking and selection composition

- Scene view composition now belongs to `xiiSceneRenderPipeline`, using the native
  pipeline resource descriptor. Interactive mesh queries, selected-only surfaces,
  selection color composition, shape icons and debug rendering remain separate
  reflected passes. Query passes share native frame geometry/material tables and
  own independent scene membership and visibility state. Selected-only surfaces
  require no CPU readback and do not change interactive click-through filtering.
- The selection compositor compares native reversed depth and renders visible and
  occluded outlines plus configurable interior tint. It supports independent scene
  depth/query resolutions and nonzero output viewport origins, preserves output
  alpha and bypasses itself when selection is disabled or empty.
- SceneView consumes native query results and pipeline parameters instead of
  removed RenderWorld pass properties. Full native owner/component identities and
  section indices reach the existing picking IPC, with GUID resolution across
  scene/layer contexts. Overlay gizmos retain priority; ordinary gizmos require
  available scene depth. Marquee queries deduplicate native owners and preserve
  the request action identifier. Thumbnail cameras consume authored native camera
  settings and disable interactive query work.
- EnginePluginScene, EditorEngineProcess and EditorEngineProcessFrameworkTest now
  build in Windows Debug. This supersedes earlier EnginePluginScene compile
  blockers recorded above. All 42 Vulkan engine-process tests and all 18
  GraphicsCore tests pass. GPU tests cover native identity and section 1024,
  viewport/camera/transform/deletion invalidation, per-view capture isolation,
  selected-object click-through, outline/interior/background pixels, visible and
  occluded selection, disabled composition and same-handle material reloads.
  Evidence: `Workspace/editor-scene-selection-build.log`,
  `Workspace/editor-scene-selection-tests.log` and
  `Workspace/editor-scene-selection-core-tests.log`.
- These tests exercise real GPU graph execution and shared scene passes. They do
  not establish integrated Qt scene-window IPC, layered editing, gizmo interaction
  or rendering parity for unsupported native deformation/indexed fallback paths.
  Same-handle mesh geometry reload handling and full Editor/plugin verification
  remain outstanding at this checkpoint. The overall modernization is incomplete.

## Native mesh reload lifetime and scene authoring

- Shared mesh residency now keys the current geometry by both mesh and buffer
  content revisions. Reloading publishes a replacement generation; a delayed view
  retains its exact previous geometry until it migrates. Releases name the acquired
  geometry rather than a resource handle that can now identify newer content.
  Last-use frames monotonically extend GPU retirement, and old LOD records reject
  uploads from a changed source buffer. Mesh-buffer changes invalidate query captures
  through the same resource epoch as mesh/material changes.
- GPU readback tests exposed a native meshlet layout mismatch: C++ bounds started
  at byte 24 while the portable shader float4 started at byte 32. Explicit alignment
  fields now match the 64-byte GPU record. The portable serialized payload remains
  56 bytes and resets transient padding when read. Degenerate meshoptimizer normal
  cones retain the disabled sentinel instead of rejecting closed clusters.
- The box drawing tool now authors native game objects and mesh components using
  the bundled unit Box mesh asset, full extents and center transforms. Optional
  materials use native `MaterialOverrides`. Creation is one undoable transaction;
  invalid dimensions and accessor failures roll back all partial content. This
  preserves the box drawing workflow without recreating the removed GreyBox
  component. Existing GreyBox assets have not yet been migrated.
- Material drag/drop uses native mesh components and override slots, rejects invalid
  targets and malformed asset MIME, commits only successfully applied targets and
  rolls cancelled previews back. Shared drag/drop identity decoding now handles
  missing, empty and truncated payloads. Newly constructed drag/drop info initializes
  its MIME pointer. A discovered command-history ownership bug is fixed: cancelled
  nested transactions are removed from their parent; temporary updates that reuse
  a transaction clear its work without destroying that shared transaction. Failed
  resize commands propagate their status and clean up rolled-back child commands.
- EditorFrameworkTest now links the complete production EditorPluginScene instead
  of compiling selected scene implementation copies. Windows Debug builds of
  EditorPluginScene, EditorFrameworkTest, ToolsFoundationTest, GraphicsCoreTest,
  EditorEngineProcessFrameworkTest and EditorEngineProcess pass. All seven
  EditorFramework tests, all seven ToolsFoundation DocumentObject tests, all 18
  GraphicsCore tests and all 42 Vulkan engine-process tests pass. Coverage includes
  native box asset identity and persistence, material drag undo/redo/cancel, nested
  transaction cancellation, CPU/GPU mesh metadata, actual meshlet/vertex uploads,
  same-handle mesh reloads and independent delayed-view geometry retirement.
  Evidence: `Workspace/editor-native-lifetime-build.log`,
  `Workspace/editor-native-lifetime-tools-tests.log`,
  `Workspace/editor-native-lifetime-repair-build.log`,
  `Workspace/editor-native-lifetime-repair-core-tests.log`,
  `Workspace/editor-native-lifetime-final-engine-tests.log`,
  `Workspace/editor-native-material-final-build.log` and
  `Workspace/editor-native-material-final-tests.log`.
- Full Editor/Assets/Processor integration and Qt scene interaction remain
  outstanding. Property-animation authoring still references a removed runtime
  resource, and no PropertyAnim engine document context is registered. The current
  skeletal clip resource is not a general reflected-property animation replacement.
  Native whole-mesh extraction also still needs section/material isolation before
  arbitrary multi-material picking and material drag can claim parity; the native
  box explicitly uses its single section and slot zero. Indexed fallback queries,
  native deformation, legacy scene migrations, other backends/platforms and GUI
  lifecycle workflows remain incomplete or unverified. Earlier compile blockers
  for EnginePluginScene and EditorPluginScene are superseded by the builds above;
  the comprehensive modernization goal remains active.

## Native section and material identity

- Whole-mesh components now emit ordinary render packets per nonempty section of
  the selected LOD. Each packet retains its owner, section primitive/meshlet range
  and material overrides. Explicit section selection uses the selected LOD's
  section list; an out-of-range section emits no geometry. Existing native scene
  synchronization and rasterization consume these packets without an Editor-only
  rendering path.
- Native source records distinguish mesh section identity from the section's
  material slot. Material selection and source metadata use the selected LOD and
  refresh the slot during synchronization. SceneView publishes the material slot
  through the existing picking part field consumed by material drag/drop, while
  native query clients can still inspect the separate full-width section index.
- Windows Debug dependent builds pass and all 42 Vulkan engine-process tests pass
  in `Workspace/editor-native-sections-final-tests.log`. The added test sends real
  extraction messages to a native mesh component and checks GPU hits on two
  spatially separate sections whose material slots are 5 and 2. It also verifies
  explicit section filtering and invalid-section rejection. All seven production
  EditorFramework tests pass in `Workspace/editor-native-sections-editor-tests.log`.
  Build evidence: `Workspace/editor-native-sections-final-build.log` and
  `Workspace/editor-native-sections-repair-build.log`. This supersedes the previous
  whole-mesh section/material isolation gap for the tested native meshlet path.
- The accompanying GraphicsCore run exposed an intermittent pre-existing custom
  loader reload race; its result is not recorded as green at this checkpoint.
  Cross-LOD residency/range interactions, indexed fallback and deformation paths,
  integrated Qt material dragging and the full Editor application remain outside
  this checkpoint's verified coverage.

## Busy resource reloads and failed command ownership

- Resource loading now consumes a pending custom payload after the current update
  and loader close finish. Requests arriving while the resource is in flight no
  longer remain silently stranded in the custom-loader table. The existing table
  coalesces requests to the latest payload; no second cache or reload queue was
  introduced. Shutdown prevents scheduling further reloads. Existing content may
  remain visible until the pending reload begins, so consumers continue to use
  the existing content-updated events and resource revisions.
- The pipeline test deterministically queues two replacement payloads from inside
  the previous loader's close callback and verifies that the latest definition is
  published. Shared signal ownership keeps the test safe even on a timeout. The
  existing shared-definition test waits for replacement publication before checking
  new instances and retained previous compilation snapshots. Both Core resource
  manager tests and all 18 GraphicsCore tests pass:
  `Workspace/editor-custom-reload-resources-tests.log` and
  `Workspace/editor-custom-reload-graphics-tests.log`.
- Failed composite commands clean up their successfully completed, rolled-back
  children without invoking the failed parent's completed-command cleanup hook.
  Tests verify one destruction for a created child and safe rejection of an invalid
  parent before any object exists. This corrects the failure-cleanup addition in
  the earlier command-history checkpoint. Extracted test packets are also released
  after GPU completion and before resource-manager shutdown.
- The final Windows Debug build passes for ToolsFoundationTest,
  EditorFrameworkTest (including the production Scene plugin),
  EditorEngineProcessFrameworkTest and EditorEngineProcess. All seven
  ToolsFoundation DocumentObject tests, all seven EditorFramework tests and all 42
  Vulkan engine-process tests pass. Evidence:
  `Workspace/editor-command-cleanup-final-build.log`,
  `Workspace/editor-command-cleanup-tools-tests.log`,
  `Workspace/editor-command-cleanup-editor-tests.log` and
  `Workspace/editor-command-cleanup-engine-tests.log`. This supersedes the
  GraphicsCore reload-race failure noted in the section checkpoint.
- The integrated Editor/Assets/Processor build still fails on the removed
  `GameEngine/Animation/PropertyAnimResource.h` include in property-animation
  authoring (`Workspace/editor-integrated-authoring-build.log`). No runtime
  replacement, successful application startup or GUI workflow is claimed.
  Other platforms/backends and the broader outstanding migrations above remain
  unverified or incomplete; this is continued progress toward the comprehensive
  modernization objective, not completion of that objective.

## Property-animation source preservation and integrated build

- Repository inspection confirms that property animation has no Engine resource,
  component, or EnginePluginAssets document context. Skeletal animation clips have
  different binding and sampling semantics and cannot represent arbitrary reflected
  property tracks. No shipped property-animation source documents were found; external
  project documents must still be preserved.
- Existing `.xiiPropertyAnimAsset` documents retain editable curves, gradients, event
  tracks, binding paths, FPS and duration. Their document UUIDs, track UUIDs and control
  point UUIDs survive save/reopen and undo/redo. The source editor uses the regular
  document window and local object mirror without a runtime connection. A visible
  notice explains the absent runtime export and 3D preview. Unsupported context
  keyframing, engine mirroring, viewports, gizmos and transform actions were removed.
- The production document manager opens existing files but rejects creation, removes
  runtime-compatible asset tags, disables transformation and publishes no asset-table
  resource redirection. It does not emit the obsolete binary format. Existing source
  files remain editable; this is not a replacement property-animation runtime.
- Editor-owned `xiiPropertyAnimationChannel` replaces the removed runtime enum. A
  track version patch preserves numeric channel values and migrates legacy named
  values while leaving bindings and child objects intact. The unused, unreflected
  playback-mode field was removed. Invalid legacy FPS values are bounded during
  scrubbing to avoid division by zero or a zero frame step.
- EditorFrameworkTest now links the production asset plugin and supplies the real
  application and Qt lifetime required by its startup services. Tests exercise all
  nine legacy channel names, numeric channel storage, disk persistence, stable
  identities, curve/gradient/event content, undo/redo, and production-manager source
  policy. All nine editor document subtests pass using Qt's offscreen platform:
  `Workspace/editor-property-source-final-build.log` and
  `Workspace/editor-property-source-final-tests.log`.
- The Windows Debug integrated build passes for Editor, EditorProcessor,
  EditorPluginAssets and EditorPluginVisualScript, including their scene and engine
  process dependencies: `Workspace/editor-property-source-integrated-build.log`.
  This supersedes the property-animation compile blocker recorded above. Integrated
  application startup, project/document windows, viewport interaction and other
  platforms remain separate verification work; these builds and source tests do not
  establish completion of the comprehensive Editor modernization goal.

## Native application, scene window and renderer lifecycle verification

- The production Editor startup test now opens an isolated project and real Qt
  document windows. It exercises the preserved property-animation source editor,
  queued selection after delete/undo/redo, window destruction, and project shutdown.
  Preference paths honor the existing `:appdata/` mount, so the startup override
  isolates document preferences as well as logs. Application data identity matches
  the Editor's tool-data directory. Optional plugin data mounts follow the runtime
  application convention.
- A production default scene is created with its native mesh components and
  pipeline resource. The native Qt test transforms the bundled Sphere mesh through
  the asset curator, writes asset tables, renders a 256-by-256 viewport through the
  Engine process, and verifies varied captured pixels. Picking requests traverse
  the real IPC connection and return the Sphere document UUID. The test drives
  the normal frame protocol and retries asynchronous picking within a bounded
  timeout while resource loading settles. Transform editing, undo/redo, save and
  reopen preserve the scene and object UUIDs and the final transform. Closing the
  window and project exercises destruction of both preview and scene worlds.
- Application world-update loops now skip vacant world-registry slots. Closing a
  preview world previously left a null slot that these loops dereferenced when
  the remaining scene continued rendering. The registry's documented count is
  its slot count; existing live-world identity and allocation semantics are retained.
- Native rendering exposed runtime defects beyond the Editor adapters. Reverse-Z
  swapchain clear depth zero is accepted. Required shader permutations block for
  their actual definitions, and invalid handles are rejected before acquisition.
  Concurrent pipeline-cache misses and shared material registration are serialized
  with their existing managers. World rendering shares one frame-completion clock
  and serializes manager preparation, recording and submission across worlds;
  extraction and GPU queues within a graph remain asynchronous. Emissive triangle
  extraction binds its required linear-wrap sampler and checks resource commits.
- Debug drawing resamples reduced-resolution scene depth to the display target
  through a depth-only render-graph pass. Matching attachments reuse the original
  depth. Intermediate coordinates start at zero even for offset presentation
  viewports. GPU tests verify visible, occluded and overlay triangles on an odd-sized
  display with both zero and nonzero viewport origins. Unsupported resampling
  dimensions and mismatched attachment sample counts fail preparation explicitly.
- Vulkan query pools grow in fixed-size blocks instead of exhausting at their
  initial capacities. Recorded and in-flight commands retain their query slots
  independently of public query objects. Retirement and readback use actual queue
  submission fences. Reusable lists reset slots before each execution through
  primary reset preambles, outside render passes. Secondary lists propagate those
  preambles and remain retained by submitted primaries until completion. Secondary
  commands declare render-pass continuation only when they inherit a render pass.
  Failed preamble recording returns `xiiResult` failure and releases the unbegun
  allocation. Completed command data is destroyed outside pool locks, and device
  shutdown drains secondary retirement before destroying query pools. Pool blocks
  remain allocated until device shutdown; reusable lists retain slots until reset
  or destruction.

Verification on Windows, VS2026 Debug, Qt 6.11 and Vulkan:

- Integrated builds of Editor, EditorProcessor, EditorPluginVisualScript and
  GraphicsFoundationTest pass, including the production Scene/Assets plugins and
  Engine process: `Workspace/editor-native-integrated-final-build.log`.
- All 40 GraphicsFoundation subtests pass, including 1537 duration queries over
  repeated allocation/reuse, release of public queries before submission, reusable
  primary and secondary query execution, and release of both command lists before
  GPU completion: `Workspace/editor-secondary-query-retirement-tests.log`.
- All 18 GraphicsCore subtests pass:
  `Workspace/editor-native-core-final-tests.log`.
- All 45 Vulkan engine-process subtests pass, including offset debug depth,
  concurrent cache/material acquisition, multiple world frame submission, native
  mesh section picking and resource reload:
  `Workspace/editor-secondary-final-engine-tests.log`.
- All ten offscreen Editor subtests pass:
  `Workspace/editor-offscreen-final-tests.log`. The native Qt Startup integration
  subtest also passes, including actual viewport capture and IPC picking:
  `Workspace/editor-native-scene-final-tests.log`.

These results supersede the earlier unverified application/window startup and
integrated build notes. Native Qt verification is automated through production
widgets and IPC; manual mouse-driven manipulation, material dragging, broad visual
quality comparisons and full workflow parity are not established. Vulkan is the
only compiled backend here; D3D12, other platforms and Release remain unverified.
Hardware without host query reset, allocation-failure injection and secondary
queries inside inherited render passes have not been exercised. Cross-LOD residency,
indexed/deformed native picking, legacy scene migrations and the ReFS case-only
notification limitation remain outstanding. The manual RenderGraphManager frame
timeline and shared temporal managers still require review against the common
world submission clock. The comprehensive modernization goal remains active.

## Current renderer and gameplay registration inventories

- GraphicsCore and GameEngine static-link inventories now follow StaticLinkUtil's
  current registration discovery. The previous inventories contained 112 and 40
  references, respectively, without a corresponding source registration marker.
  Removed pass, PropertyAnim, GreyBox, animation-node and former VisualScript
  registrations are gone. Current material managers, render graph/world services,
  particles, light/render components, gameplay volumes and resources are represented.
- Both inventories and their file markers match the existing generator's output:
  90 unique GraphicsCore references and 21 unique GameEngine references, with no
  unresolved points. Review verifies that implementation bodies are unchanged.
  Other libraries' static inventories have not been migrated in this checkpoint.
- The dependent Windows Debug DLL build passes for EditorFrameworkTest,
  EditorEngineProcessFrameworkTest, GraphicsCoreTest and EditorProcessor:
  `Workspace/editor-staticlink-inventory-build.log`. All ten native Qt Editor tests,
  all 45 Vulkan engine-process tests and all 18 GraphicsCore tests pass:
  `Workspace/editor-staticlink-native-tests.log`,
  `Workspace/editor-staticlink-engine-tests.log` and
  `Workspace/editor-staticlink-core-tests.log`. This is source registration audit
  and DLL integration evidence; a complete static-link configuration has not run.

## Texture conversion with disabled or partial mip chains

- Inspecting native-run assertion artifacts exposed a separate asset conversion
  defect. `ExtractLowerMipChain` used the theoretical maximum mip count for the
  dimensions instead of the image's stored count. The bundled Grid asset disables
  mip generation while requesting six low-resolution levels, causing access to
  nonexistent mip levels. The helper now clamps to stored levels. Invalid images
  and zero requested levels return `xiiResult` failure without changing output.
- Byte-level tests cover one-, two- and three-level compressed and uncompressed
  chains, requests larger than the stored chain, and failure preservation. All four
  ImageDimensions subtests and all five TextureConverterTool subtests pass:
  `Workspace/editor-stored-mip-chain-tests.log` and
  `Workspace/editor-stored-mip-tool-tests.log`. The exact Grid conversion options
  also succeed with isolated output paths, producing the main image, thumbnail and
  low-resolution image without a new assertion artifact:
  `Workspace/editor-stored-mip-converter-tests.log`.
- TextureTest, EditorFrameworkTest and EditorProcessor build successfully:
  `Workspace/editor-stored-mip-chain-final-build.log`. All ten native Editor tests
  pass again with `XII_SILENT_ASSERTS=1` inherited by child processes, so assertions
  crash instead of opening unattended dialogs. No assertion artifact is produced:
  `Workspace/editor-stored-mip-native-tests.log`. Earlier passing native runs did
  not expose this child-process failure through their test status; the clean
  conversion and native rerun above supersede that evidence gap.
- The final integrated Editor and EditorPluginVisualScript build also passes:
  `Workspace/editor-modernization-checkpoint-build.log`. No static-link,
  Release or additional platform/backend verification is implied by this build.


## Native scene LOD controls and GPU surface verification

- `xiiLODMeshComponent` already exposed forced LOD and bias, but native scene
  synchronization dropped both controls. The scene database now preserves them
  across creation, updates, compaction and slot reuse. Selection changes dirty
  the GPU upload; unchanged controls preserve revisions. Invalid handles,
  out-of-range forced indices and non-finite bias return `xiiResult` failure.
  Invalid extraction controls are checked before membership/resource mutation.
- GPU visibility honors forced LOD without applying coverage bias or hysteresis.
  A forced LOD absent from the definition or not resident emits no meshlets;
  existing streaming requests can make it resident without substituting a LOD
  with unrelated ranges. Automatic bias uses projected-coverage octaves, with
  positive values favoring finer detail. Only selection coverage changes, not
  physical frustum/cone bounds. GPU evaluation saturates bias to [-16, 16].
- Ordinary and temporal Hi-Z meshlet culling now use the existing shared shader
  implementation. Removing the duplicated ordinary implementation ensures both
  paths use identical selection, feedback and primitive-range rules. The GPU
  scene record grows from 240 to 256 bytes, with exact C++ size/offset assertions
  against the HLSL layout. CPU storage adds one index and one float per object.
  Reflection versions advance; mesh asset formats are unchanged.
- The dedicated `NativeSceneLod` Vulkan test runs eight actual visibility and
  surface-query frames: coarse/fine forced selection despite opposing biases,
  missing and nonexistent forced LODs, automatic positive/negative bias, and
  both ordinary and temporal Hi-Z paths. It stages the compact visibility stream
  and selected meshlet metadata, then reads nearest-surface identity and depth.
  Forced draws retain extracted primitive ranges and verify owner/material slots.
  CPU tests cover invalid inputs, mutation preservation, revisions and reuse.
  Native buffer readback is shared with the existing picking fixture.
- Windows Debug builds pass for GraphicsCoreTest, EditorEngineProcessFrameworkTest,
  Editor and EditorPluginVisualScript: `Workspace/editor-native-lod-final-build.log`.
  The final fixture rebuild passes: `Workspace/editor-native-lod-final-test-build.log`.
  All 46 engine-process subtests pass: `Workspace/editor-native-lod-integrated-tests.log`.
  The final absent-LOD shader guard and eight-frame fixture also pass independently:
  `Workspace/editor-native-lod-focused-final-tests.log`. All 18 GraphicsCore tests
  and all ten native Qt Editor tests pass: `Workspace/editor-native-lod-core-final-tests.log`
  and `Workspace/editor-native-lod-qt-tests.log`. Native Qt runs include viewport
  capture and real IPC picking. No assertion artifact is produced.
- The automatic-bias fixture at this checkpoint deliberately used a whole-LOD
  range; cross-LOD authored section mapping is verified in the follow-up below. Indexed/deformed
  native queries, legacy sky/probe/GreyBox migration, additional static-link
  inventories, the ReFS case-only watcher limitation, manual-manager frame
  timeline and shared temporal-manager isolation remain outstanding. Manual UI
  workflow parity, D3D12, Release and other platforms remain unverified. The
  comprehensive modernization goal remains active.

## Authored LOD sections, shared selection and picking invalidation

- Component extraction explicitly distinguishes whole meshes, authored section
  ordinals and custom primitive ranges. Native synchronization expands an authored
  mesh into its nonempty LOD/section candidates, using each LOD's own primitive
  range and material slot. Whole-mesh CPU section packets are deduplicated using
  reusable scratch storage. Ordinary indexed packets retain their CPU-selected
  ranges; a section absent from that CPU LOD emits no indexed work while remaining
  available to native GPU selection. Entirely empty CPU LODs retain an authoring
  packet for selection of other LODs. Custom packets preserve their prior exact
  primitive-range contract.
- Stable native keys and source metadata now include authored LOD identity.
  Geometry residency remains shared across candidates and views. Resource reload
  updates surviving ordinal keys and retires removed sections; a delayed scene
  context retains its own immutable geometry revision and material metadata until
  it synchronizes. No mesh asset format or Core graph dependency changes.
- A candidate's draw LOD restricts its range/material to the selected authored LOD.
  Section candidates share one generation-checked, non-owning LOD representative.
  Dense indices are resolved before GPU records are emitted, including when a
  representative follows its members or unrelated objects are removed. Destroyed
  representatives cannot resolve to reused objects. Instance culling updates LOD
  history once per instance before material routing, frustum or Hi-Z tests; meshlet
  culling consumes that result. Filtered opaque/translucent sections therefore retain
  coherent hysteresis. History uses native generation identities rather than
  application IDs. Streaming feedback applies the same active-instance bounds.
- Draw LOD and representative index reuse the GPU record's existing padding,
  preserving its 256-byte stride. CPU storage adds a draw index and representative
  handle per allocated scene object. Native candidate storage scales with authored
  LOD sections (at most eight LODs), while immutable mesh buffers remain shared.
  Emissive extraction admits only the finest resident authored LOD, or the explicitly
  forced LOD, preserving its view-independent policy without duplicating emitters
  from candidates whose primitive ranges overlap.
- Empty authored LODs translate to an empty source meshlet range at the source end;
  source count zero otherwise means "to end" and could incorrectly borrow the next
  LOD's meshlets. Native resource reload tests verify CPU and uploaded GPU counts,
  differing section layouts, retired IDs, and independently synchronized contexts.
- Picking fingerprints include packet range semantics and coverage bias. Captured
  owner validation also includes component section selection, forced LOD and bias,
  rejecting occupied pixels immediately after component changes before a new graph
  is built. Existing resource-event, transform, material and owner validation remains.
- All 48 Vulkan engine-process subtests pass, including three native LOD tests and
  the extended production picking fixture:
  `Workspace/editor-lod-sections-integrated-tests.log`. The section fixture executes
  15 frames with differing section counts, range/material orders, masked coverage,
  transparent filtering, shared hysteresis, stable identities and forced controls.
  All 18 GraphicsCore subtests pass:
  `Workspace/editor-lod-sections-core-tests.log`. The final shader bounds guard also
  passes all three native LOD tests: `Workspace/editor-lod-sections-final-tests.log`.
- Windows Debug builds pass for Editor, EditorPluginVisualScript,
  EditorFrameworkTest and EditorProcessor:
  `Workspace/editor-lod-sections-editor-build.log`. All ten native Qt Editor tests
  pass, including viewport capture and real engine-process IPC picking:
  `Workspace/editor-lod-sections-qt-tests.log`. No assertion artifact is produced.
  Indexed/deformed native picking, legacy sky/probe/GreyBox migrations, additional
  static-link inventories, case-only ReFS watching, manager frame timelines, shared
  temporal manager isolation and broader manual workflow parity remain outstanding.
  D3D12, Release, Linux and macOS remain unverified; this migration remains active.


### Native Indexed Surface Queries And Dispatch Synchronization (2026-10-12)

- Native surface queries now select mesh shading when available or a conventional vertex/pixel
  pipeline otherwise. `xiiSceneMeshPickingPass.UseMeshShaders` is a reflected pipeline-resource
  setting, defaulting to true; false explicitly selects vertex pulling. Views and Qt do not own
  a second renderer. The native GPU visibility service now initializes without mesh-shader
  features and omits mesh-dispatch command construction on those devices.
- Both routes consume the same compact visible meshlets, resident geometry, native identities,
  LOD selection, section ranges and canonical material coverage. Vertex attributes/transforms
  and query pixel output are shared shader headers. The indexed route pulls meshlet primitive
  indices and vertex remaps, preserving meshoptimizer's triangle ordering and culling bounds;
  drawing the original source-buffer ranges would be incorrect after meshlet optimization.
- A graph-owned compute pass emits one 16-byte DrawIndirect command per visible meshlet.
  The existing GPU visibility counter controls drawing without CPU readback. Command storage is
  16 bytes per visibility capacity slot and dispatch launches one thread per slot, rounded up
  to 64 threads. The graph cache reuses allocations. Adapter dispatch/draw limits and indirect
  counter capability are checked before adding passes; invalid descriptions clear the output
  and return failure. This path still requires compute, bindless resources and indirect drawing.
- Packed mesh GPU allocations are padded to complete four-byte words, covering both UInt16
  source-index tails and byte-packed meshlet primitive tails. CPU descriptors retain logical
  element counts. One-triangle UInt16/UInt32 fixtures verify allocations, owner identity, depth
  and normals on both routes. Indirect argument validation now accepts the correct 16-byte
  non-indexed draw record and measures the final record's extent rather than trailing stride.
- GPU command readback exposed a Vulkan dispatch bug: explicit render-graph barriers remained
  pending when shader resources were committed with Verify. Compute dispatch preparation now
  flushes those barriers, for both direct and indirect dispatch. The query uses graph-managed
  transitions and Verify bindings; it requires no manual barrier or queue-wait workaround.
- All 50 Vulkan engine-process subtests pass, including production picking, selection overlays,
  per-view capture isolation, owner/configuration invalidation, material and mesh reloads,
  authored LOD section/material routing and shared hysteresis. The two new subtests create
  devices with mesh shaders disabled; the LOD fixture additionally reads generated GPU commands,
  tests an exact single-command allocation and verifies invalid-input output clearing.
  Evidence: `Workspace/editor-indexed-picking-integrated-tests.log`.
- All 40 GraphicsFoundation, 18 GraphicsCore and ten native Qt Editor subtests pass:
  `Workspace/editor-indexed-picking-foundation-tests.log`,
  `Workspace/editor-indexed-picking-core-tests.log`, and
  `Workspace/editor-indexed-picking-qt-tests.log`. Qt verification includes real viewport capture
  and engine-process IPC picking. Editor, EditorPluginVisualScript, EditorProcessor and all three
  native test targets plus EditorFrameworkTest build in Windows VS2026 Debug x64:
  `Workspace/editor-indexed-picking-dependent-build.log`. Final test-only input/settings checks
  were built in `Workspace/editor-indexed-picking-validation-build.log`. No assertion artifact
  was produced. Vulkan reports existing nonfatal unused vertex/fragment interface warnings.
- Indexed native picking is now covered. Deformed geometry remains a distinct renderer gap:
  component skinning matrices/morph data are not yet connected to a shared execution path, so
  this change does not claim pose-correct queries. Legacy sky/probe/GreyBox migration, additional
  static-link inventories, case-only ReFS watching, manager frame timelines, shared temporal
  manager isolation and broader manual workflow parity remain active work. D3D12, Release,
  Linux and macOS remain unverified.


### Shared GPU Frame Timeline (2026-10-12)

- World rendering and registered render graphs now use one subsystem-owned frame counter,
  completion tracker and recording mutex through `xiiRenderGraphFrame`. Its result-returning
  Begin prepares global managers and waits for reusable slices. End captures all submitted
  queue fence values and releases the mutex; scope destruction does the same on early returns.
  Empty frames and frames that fail compilation retain valid retirement points. Extraction
  remains parallel, and render graphs retain their existing asynchronous queue scheduling.
- The former world-local tracker and caller-supplied manager frame counters are removed.
  The GPU-driven sample prepares its scene and executes registered graphs inside one scope.
  Registered graph frequency uses the common device frame index, including world submissions.
  Frame scopes require same-thread use and must end before renderer shutdown; nested scopes,
  repeated Begin and a zero submission window return failure. End is idempotent. Context
  creation/destruction and graph registration changes remain frame-boundary operations;
  concurrent registration or context destruction is unsupported.
- The requested submission window is bounded by configured material, geometry, shadow and
  ray-tracing rings and the smallest live visibility context ring. This prevents callers with
  a larger window from overwriting a shorter ring still consumed by the GPU. Visibility-ring
  inspection is linear in live contexts, as is the existing visibility frame preparation;
  the frame scope carries two 64-bit counters and no topology or per-view history.
- The native WorldRenderFrames regression interleaves two concurrently updated worlds,
  registered graphs and empty scopes for eight iterations. A registered pass uploads the
  exact frame identity and verifies it by GPU readback. A two-slot visibility context bounds
  a requested 64-frame window. A cyclic graph registered after the uploading graph fails
  compilation through `xiiResult`; recovery waits for the captured frame and executes again.
  Destroying a world or the shorter visibility context leaves the remaining consumers valid.
- This consolidates submission and retirement, not view-specific temporal ownership. Shared
  DDGI/radiance histories, deformed geometry, legacy sky/probe/GreyBox migration, additional
  static-link inventories, case-only ReFS watching and broader manual workflow parity remain
  active work. D3D12, Release, Linux and macOS remain unverified.

- Windows VS2026 Debug x64 builds pass for Editor, EditorPluginVisualScript, EditorProcessor,
  EditorFrameworkTest, EditorEngineProcessFrameworkTest, GraphicsCoreTest,
  GraphicsFoundationTest and GpuDrivenScene: `Workspace/editor-shared-frame-final-build.log`.
  All 50 Vulkan engine-process, 18 GraphicsCore, 40 GraphicsFoundation and ten native Qt Editor
  subtests pass: `Workspace/editor-shared-frame-integrated-tests.log`,
  `Workspace/editor-shared-frame-core-tests.log`,
  `Workspace/editor-shared-frame-foundation-tests.log` and
  `Workspace/editor-shared-frame-qt-tests.log`. These results include GPU readback,
  concurrent worlds, failed-frame recovery, actual viewport capture and IPC picking.

### Sample Integration And Vulkan Synchronization Diagnosis (2026-10-12)

- The GPU-driven sample shader still referenced the removed scene-instance ObjectIndex field.
  Its color variation now consumes application-owned UserData, preserving the sample's stable
  object labels independently of native generation identities. The sample validates its loaded
  draw shader before recording, reports shader/frame failures with a nonzero application result,
  and reports the completed count for bounded runs. Optional hardware ray tracing can explicitly
  be disabled through `-ray-tracing false` to exercise the supported raster-only device path;
  default behavior retains optional hardware ray tracing.
- Vulkan GPU-assisted and synchronization validation identified two actual hazards in the sample:
  AS-build input buffers require shader-read access at the AS-build stage, while the previous
  BuildASRead mapping included only acceleration-structure reads. The buffer mapping and
  BLAS/TLAS input verification now include the correct access. Acceleration-structure object
  barriers remain separate. Swap-chain acquisition waits now cover all commands in the small
  acquisition submission so its initial undefined-to-transfer image transition cannot precede
  acquisition. No CPU queue-idle workaround is introduced into frame execution.
- Visibility LOD feedback maps completed readbacks with DoNotWait, matching the existing queue
  completion check and Vulkan's explicit synchronization contract.
- Initial synchronization evidence: `Workspace/editor-shared-frame-sample-validation.log`
  reports AS-build input read-after-write and acquisition layout write-after-read hazards.
  After repair, `Workspace/editor-shared-frame-sample-synconly.log` and
  `Workspace/editor-shared-frame-sample-syncfixed-validation.log` report neither hazard.
  They still end in ErrorDeviceLost on larger/longer workloads, so they are not passing runs.
  The raster-only 64-frame comparison also loses the device:
  `Workspace/editor-shared-frame-sample-raster.log`; hardware ray tracing is not required.
- Small uninstrumented eight-frame (2x2) and sixteen-frame (8x8) sample runs exit normally:
  `Workspace/editor-shared-frame-sample-small.log` and
  `Workspace/editor-shared-frame-sample-syncfixed.log`. These successful bounded runs do not
  establish long-run stability or resolve the historically recorded device-loss problem.
  GPU-assisted safe-mode instrumentation changes workload and timing; its larger-run failure
  has not been attributed to instrumentation or a driver timeout. Further diagnosis remains
  active work alongside view-specific temporal history and the broader migration inventory.

- Final Windows Debug builds including the sample, Editor, VisualScript plugin, processor,
  native Qt tests and graphics consumers pass:
  `Workspace/editor-shared-frame-final-integration-build.log`. All 118 automated subtests pass
  again after the Vulkan repairs (50 engine-process, 40 Foundation, 18 Core, ten Qt):
  `Workspace/editor-shared-frame-final-integrated-tests.log`,
  `Workspace/editor-shared-frame-final-foundation-tests.log`,
  `Workspace/editor-shared-frame-final-core-tests.log` and
  `Workspace/editor-shared-frame-final-qt-tests.log`.
  The final 2x2 sample explicitly reports eight completed frames and exits normally with both
  GPU-assisted safe-mode and synchronization validation enabled and no validation errors:
  `Workspace/editor-shared-frame-sample-final-small-validation.log`.
  Existing nonfatal validation advisories about combined validation modes and instrumented
  shader compilation cost remain. No assertion artifact remains in the checkout; failed-run
  assertions are preserved alongside their Workspace diagnostic logs. Longer sample stability,
  view-specific temporal ownership and the comprehensive Editor migration remain unproven.

### Temporal History Across Interleaved Views

- Hi-Z history now imports the last pyramid recorded for its visibility context rather than
  the image at global frame N-1. Its destination rotates after a recorded build, independently
  of skipped global frames. Compilation without execution does not rotate or validate history.
  Camera-cut invalidation keeps rotation separate from validity; resize clears both. Imports
  reject the build's own frame and older indices. The existing caller-owned GPU lifetime and
  one-build-per-context-per-frame contract remains unchanged.
- Built-in AO, ray-traced shadows, ReSTIR DI/GI reservoirs, RT GI, RT reflections, SSR and fog
  now have independent per-view write indices. Each producer captures its selected slot in
  pass data and advances after recording its output, following the existing TAA pattern.
  Global frame indices remain available for shader sampling and resource lifetime tracking.
  DI history becomes valid in its spatial producer, rather than relying on the later emissive
  visibility consumer executing. A culled producer cannot advance its effect's rotation.
- Added the native `TemporalHistory` GPU test group. `InterleavedHiZ` alternates two contexts
  with two- and three-image rings, abandons compiled frames, invalidates a history, and reads
  every texel of every imported mip to verify the previous executed depth and view isolation.
  `InterleavedDefaultPipelines` shares one default pipeline resource across two worlds and
  checks nine actual previous/current GAL image bindings over six submissions, including TAA.
  It verifies that previous inputs match the same view's last output, never its destination,
  and that views own distinct history images. All producers are required to exist.
- The new rendering test also exposed an invalid offscreen output transition: the default
  final blit always exported shader-read state, even for render-only textures. It now exports
  shader-read state when the texture supports sampling and render-target state otherwise.
  The existing default-pipeline pixel-readback test now uses a render-only target to exercise
  this supported caller configuration without changing swap-chain presentation.
- Narrow Windows Debug/Vulkan verification passes:
  `Workspace/editor-history-lighting-tests.log` (both temporal subtests), and
  `Workspace/editor-history-output-tests.log` (default offscreen rendering and pixel readback).
  The Hi-Z-only checkpoint also passes in `Workspace/editor-history-hiz-tests.log`.
- Recording failure recovery also invalidates the view's built-in and resource-pass histories.
  Framebuffer shape validation reports xiiResult diagnostics without asserting. The executor
  closes its profiling scope and submits the valid command prefix before returning failure,
  preserving GAL's tracked states for imported images whose barriers were already recorded.
  Profiler `OnFrameAbort` discards incomplete timings and advances its query ring while keeping
  potentially submitted query resources alive. The public execution contract documents partial
  submission and the caller's completion/lifetime obligations on failure.
  `FailedRecordingInvalidatesHistory` intentionally combines differently sized attachments
  after a history-producing callback, reads back its persistent image, and renders subsequent
  frames through multiple profiler-ring rotations to exercise recovery.
- Synchronization validation exposed a further scheduling defect in the complete default
  pipeline: readers of a depth version can require different layouts. Waiting for the version's
  writer did not order a later reader against another reader's layout transition. Six view
  submissions each reported READ_RACING_WRITE in `Workspace/editor-history-recovery-sync-tests.log`.
  Barrier synthesis now tracks the pass establishing each state and the last access on each
  queue. Same-state readers wait for the establishing transition while remaining independent
  of one another; a state change or export joins all outstanding queues. A transition joining
  parallel readers is immediate, so a split begin cannot run before other readers finish.
  This bookkeeping is transient compile scratch; execution uses precomputed fence waits.
  `ReaderStateScheduling` verifies reader fan-out, joining readers, export transitions, and
  immediate barriers with split barriers explicitly enabled.
- After the dependency repair, all 122 automated subtests pass on Windows Debug: 53 native
  Editor engine-process and 41 graphics Foundation tests with Vulkan synchronization validation,
  18 graphics Core tests, and ten native Qt tests including production viewport/IPC workflows.
  Evidence: `Workspace/editor-history-final-integrated-tests.log`,
  `Workspace/editor-history-final-foundation-tests.log`, `Workspace/editor-history-final-core-tests.log`,
  and `Workspace/editor-history-final-qt-tests.log`. The targeted synchronization runs also pass
  in `Workspace/editor-history-reader-sync-tests.log` and
  `Workspace/editor-history-reader-scheduling-tests.log`.
- This closes per-view image selection and rotation, not the separate shared GI ownership
  issue. DDGI and sparse voxel radiance still use process-global CPU cells, camera origins,
  update suppression and persistent GPU storage; RenderWorldModule selects one camera per
  world update. Their context/lifetime migration remains active work. Deformed picking,
  legacy sky/probe/GreyBox migration, longer GPU sample device loss, and the remaining Editor
  inventory are also outstanding. D3D12, Release, Linux and macOS are unverified here.
- Final dependent Windows Debug targets (Editor, VisualScript plugin, EditorProcessor,
  native Qt/engine-process tests, both graphics suites and GpuDrivenScene) build successfully:
  `Workspace/editor-history-verified-integration-build.log` and its exit-code file (zero).
  A subsequent 64-frame 8x8 raster sample with Vulkan synchronization validation still fails
  with ErrorDeviceLost while waiting for fence 57 (completed value 56). No synchronization
  hazard precedes that failure in `Workspace/editor-history-sample-raster-sync.log`.
  Its assertion is preserved as `Workspace/editor-history-sample-raster-sync-assert.txt`;
  the recorded exit code is -2147483645. The reader-state repair does not establish longer
  sample stability, and the cause of this device loss remains unresolved.

### Per-View Global Illumination Ownership

- Replaced process-global DDGI and sparse voxel radiance schedulers with `xiiDDGIContext`
  and `xiiSparseVoxelRadianceContext`. Each existing lighting context owns its CPU cells,
  scrolling origin, update suppression, history flags, pipeline bindings and persistent
  GPU storage. The LightingManager's generation-checked context ownership releases this
  state during view disposal and high-level shutdown, before the GAL device is destroyed.
  The obsolete GI subsystem registrations and global manager APIs are removed; all callers
  use the new ownership model without compatibility aliases.
- Lighting preparation schedules both clipmaps from the current view's camera and shared
  64-bit render-frame token. RenderWorldModule no longer selects a first camera to overwrite
  global GI state. Camera cuts and failed recording invalidate only the owning view's GI
  history. Physical storage and residency remain allocated; subsequent updates use zero
  temporal weight until valid data is recorded again.
- Context initialization supports CPU-only scheduling with a null device. Supplying a device
  creates private GPU storage. Initialization failure preserves the previous configuration;
  invalid or repeated frame indices, non-finite/out-of-range camera positions and overflowing
  DDGI dimensions return failure without modifying existing state. Lighting-context creation
  propagates initialization failure, and lighting-stage construction fails when required GI
  resources or shader pipelines cannot be produced.
- Ownership makes the memory cost explicit: the default sparse pool is 32 MiB per lighting
  context (4096 bricks, 8x8x8 voxels, 16 bytes per voxel), plus its CPU scheduler and DDGI atlases.
  No additional per-frame persistent copies or synchronization primitives were introduced.
  Contexts are not thread-safe. Their public contract requires graphs to be discarded and GPU
  work completed before reinitialization, shutdown or destruction. Reflected settings/state
  schemas and existing asset IDs/formats are unchanged.
- CPU regressions retain toroidal probe scrolling, relocation and sparse residency coverage,
  and add independent contexts at widely separated camera origins, failed initialization,
  invalid frames/coordinates, view-local invalidation, repeated shutdown and reinitialization.
  The native default-pipeline regression now verifies separate persistent DDGI atlases and
  sparse pools, reads back actual uploaded probe positions and clipmap origins, and queues
  twelve additional view submissions without intervening WaitIdle calls. Eighteen total view
  submissions pass with Vulkan synchronization validation in the strengthened test.
- Windows Debug integration builds pass for Editor, VisualScript, EditorProcessor, native Qt
  and engine-process tests, graphics Core/Foundation tests, and GpuDrivenScene:
  `Workspace/editor-gi-context-integration-build.log` (exit zero). All 124 affected subtests pass:
  20 Core, 53 engine-process, 41 Foundation and ten Qt, including production viewport/IPC tests.
  Evidence: `Workspace/editor-gi-context-final-core-tests.log`,
  `Workspace/editor-gi-context-final-native-tests.log`,
  `Workspace/editor-gi-context-final-foundation-tests.log`, and
  `Workspace/editor-gi-context-final-qt-tests.log`. Both GPU suites use Vulkan synchronization
  validation. The subsequently strengthened readback and overlapping-frame regressions also
  pass: `Workspace/editor-gi-context-readback-tests.log` and
  `Workspace/editor-gi-context-overlap-tests.log` (three temporal subtests each).
- The separately reproduced longer GpuDrivenScene device loss remains unresolved; this GI
  migration does not establish sample stability. Deformed picking, legacy sky/probe/GreyBox
  migration and the remaining Editor inventory remain active work. D3D12, Release, Linux and
  macOS have not been validated here.


### Complete Meshlet Primitive Emission

- The remaining longer-sample device loss was reproduced before this repair: the
  64-frame 8x8 raster run failed waiting for fence 60 (completed 59), while 2x2
  completed. Evidence: `Workspace/editor-sample-device-loss-medium.log` and
  `Workspace/editor-sample-device-loss-medium-assert.txt`.
- All mesh shaders declared up to 126 output triangles but emitted only one per
  64-lane thread. Engine meshlets can contain 124 triangles, leaving output indices
  above 63 uninitialized. Depth, forward lighting, shared G-buffer/picking, shadow
  depth and the GPU-driven sample now stride primitive emission by the workgroup
  size. Vertex emission remains bounded by the 64-vertex meshlet limit.
- `MeshletRaster.DensePrimitiveCoverage` constructs a 98-triangle meshlet, verifies
  its retained mesh-buffer metadata, and reads back every central coverage pixel
  from both native identity and packed picking targets. The old shaders fail 160
  pixel assertions; the repaired shaders pass. Logs:
  `Workspace/editor-dense-meshlet-baseline-tests.log` and
  `Workspace/editor-dense-meshlet-fixed-tests.log`.
- After repair the same 64-frame 8x8 run completes with synchronization validation:
  `Workspace/editor-dense-meshlet-sample-medium.log`. The full default 24x16 scene
  also completes 64 frames with ray tracing enabled, GPU-assisted safe-mode and
  synchronization validation, without validation errors:
  `Workspace/editor-dense-meshlet-sample-full-gpuav.log`. These runs include fine-LOD
  streaming. This resolves the reproduced workloads; it does not establish all
  backend or arbitrary-duration stability.
- Windows Debug builds pass for engine-process and Qt tests. All 54 native
  engine-process, 41 graphics Foundation and 20 graphics Core subtests pass:
  `Workspace/editor-dense-meshlet-final-native-tests.log`,
  `Workspace/editor-dense-meshlet-final-foundation-tests.log`, and
  `Workspace/editor-dense-meshlet-final-core-tests.log`. Both GPU suites use Vulkan
  synchronization validation.
- The first Qt run captured a uniform startup image before a successful native
  pick; retry passed. The workflow test now confirms transformed-mesh picking
  before capturing the viewport instead of treating a startup delay as readiness.
  All ten Qt subtests pass after rebuilding:
  `Workspace/editor-scene-capture-readiness-build.log` and
  `Workspace/editor-scene-capture-readiness-tests.log`. The original failure remains
  recorded in `Workspace/editor-dense-meshlet-final-qt-tests.log`.
- Deformed geometry/picking, legacy sky/probe/GreyBox migrations, remaining
  registration inventories and broader Editor workflow parity remain active work.
  D3D12, Release, Linux and macOS remain unverified here.


## Static Registration And Plugin Lifecycle Verification

- Refreshed thirteen inventories with the repository's existing StaticLinkUtil:
  Foundation, Core, GraphicsFoundation, Vulkan, D3D12, Texture, Utilities,
  DearImgui, Fileserve, Inspector, VisualScript, RenderDoc and TestFramework.
  GraphicsCore and GameEngine were already current. Removed obsolete references
  and redundant file markers; added current graph reflection, shader compiler,
  pipeline-state and plugin implementation registrations. Canonical source-path
  names replace stale marker names without changing public type or asset identity.
- The final independent source audit compares discovery rules, group references
  and file definitions across all fifteen libraries: 312 references, no missing,
  unresolved or unexpected points. Evidence:
  `Workspace/editor-registration-generator-audit.json` and the local verification
  helper `Workspace/verify-static-link-inventory.py`. Outside the lifecycle fixes
  below, source review verifies registration-only edits with unchanged bodies.
  StaticLinkUtil returns 1 for successful modifications, as documented by its own
  return-code implementation; the regeneration logs contain no serious errors.
- An isolated Windows Debug static build exposed a pre-existing mismatch between
  the static plugin registrar's const-char declaration and string-view definition.
  The implementation now matches the declared interface. This build uses separate
  `Output/StaticLinkAudit` paths, Qt disabled and the UnitTestOnly filter, extended
  locally for a small RenderDoc registration probe. The standard Editor remains a
  separate DLL build.
- The probe first crashed before main while registering RenderDoc into an
  unconstructed global singleton map. SingletonRegistry now constructs its map on
  first use with XII's static allocator, preserving storage through Core restarts
  and ensuring registered global singletons are destroyed before their registry.
  Foundation's singleton test now includes registration during global construction.
  Public lookup/registration behavior is retained; concurrent mutation still needs
  external synchronization.
- A second startup trace found RenderDoc logging before Foundation's global log
  event was constructed. RenderDoc now creates its capture service during normal
  Core-system startup after Foundation/Core, and destroys it during Core-system
  shutdown. The global object is removed. The subsystem marker makes registration
  discoverable through StaticLinkUtil without a force marker or compatibility shim.
  This also lets normal command-line setup precede capture initialization.
- The final static probe explicitly retains Core and RenderDoc with the established
  `XII_STATICLINK_PLUGIN` macros, discovers the capture interface and both graph
  types without direct implementation-type references, and verifies release at
  shutdown. It passes in `Workspace/editor-registration-static-capture-tests.log`.
  Initial crash traces remain in `Workspace/editor-registration-static-probe-debug.log`
  and `Workspace/editor-registration-static-probe-lifetime-debug.log`.
- The final static build and tests pass: three Foundation configuration subtests,
  all 21 Core subtests and all 19 tooling subtests on NTFS. Evidence:
  `Workspace/editor-registration-static-final-build.log`,
  `Workspace/editor-registration-static-configuration-final-tests.log`,
  `Workspace/editor-registration-static-core-final2-tests.log`, and
  `Workspace/editor-registration-static-tools-final-tests.log`.
- The same tooling suite on the workspace ReFS volume still fails its case-only
  folder rename scenario, followed by deletion/cache assertions:
  `Workspace/editor-registration-static-tools-tests.log`. The fresh NTFS run passes;
  this isolates the known notification limitation and does not resolve ReFS support.
  Static graphics execution, D3D12 compilation/execution, Release, Linux and macOS
  remain unverified. Capturing an actual RenderDoc GPU frame was not tested.

- Final dependent Windows Debug DLL targets pass, including Editor,
  EditorProcessor, native Qt/engine-process tests, both graphics suites,
  GpuDrivenScene, FoundationTest and the affected runtime plugins:
  `Workspace/editor-registration-lifetime-integration-build.log` (exit zero).
  All 128 DLL subtests pass: 54 engine-process, 41 graphics Foundation, 20 graphics
  Core, three Foundation configuration and ten Qt production workflows. Logs:
  `Workspace/editor-registration-final-native-tests.log`,
  `Workspace/editor-registration-final-foundation-tests.log`,
  `Workspace/editor-registration-final-core-tests.log`,
  `Workspace/editor-registration-final-configuration-tests.log`, and
  `Workspace/editor-registration-final-qt-tests.log`. GPU suites use Vulkan
  synchronization validation; Qt exercises native scene capture, picking, resource
  transformation, document persistence and engine-process communication.
- The final full default 24x16 GpuDrivenScene also completes 64 frames with ray
  tracing, GPU-assisted safe-mode and synchronization validation, without validation
  errors: `Workspace/editor-registration-final-sample-gpuav.log` (exit zero).
  No new assertion artifact is produced. Deformed execution/picking, legacy
  sky/probe/GreyBox migration, ReFS notifications and broader manual Editor workflow
  parity remain outstanding; this checkpoint does not complete the modernization.


## ReFS Asset Watcher Recovery And Lifetime Verification

- Fresh native probes reproduced the earlier ReFS case-only rename failure:
  all five tested rename APIs changed the directory's stored spelling but did
  not signal the pending notification read until another filesystem operation.
  Only `OLD_NAME` arrived. Adding notification filter bits did not restore the
  missing pair. Evidence: `Workspace/editor-refs-rename-current-probe.log` and
  `Workspace/editor-refs-rename-filter-probe.log`. File casing changes and some
  removal completions also exhibited delayed delivery in the Foundation IO suite.
- The Windows watcher now reconciles existing cached paths on ReFS while polling.
  It checks at most 128 paths per batch, at most once per 100 ms; a failed portable
  stats query gets one additional native attributes query to distinguish a missing
  path from an inaccessible one. File contents are never scanned. Each batch
  walks the cached hierarchy in O(N); the metadata budget does not bound that CPU
  traversal. With a stable cache and sufficiently frequent polling, checking all
  N paths takes approximately ceil(N / 128) batches. The watcher retains an ordinal
  and temporary mutations, rather than another copy of filesystem topology.
  NTFS keeps native notifications without this metadata polling.
- Recovery repairs leaves before their ancestors, preserving cached subtrees when
  multiple file and directory names change before one poll. Delayed native names
  resolve against the existing mirror and do not duplicate recovered removals or
  rename pairs. A normal rename still follows the recovered case rename correctly.
  Nonrecursive watches inspect only immediate children. The multi-watcher API also
  performs recovery after a wait timeout and accepts an empty watcher list.
- Two existing mirror defects were exposed and fixed: enumeration did not select
  the nested directory iterator after descending, and directory type lookup omitted
  the trailing slash stored in directory keys. Mirror file moves preserve their
  associated value, including the watcher's pending-create state. Cached casing
  resolution works after the actual path has been deleted and rejects paths outside
  the mirror. A dedicated portable mirror regression verifies these behaviors.
- Rename sources now survive successive polls, including type inference for a
  `NEW_NAME` that arrives in a later native batch. ReFS's unpaired old-name records
  cannot become the source of a subsequent unrelated rename. Empty completions no
  longer discard already collected fallback notifications.
- Watcher shutdown cancels reads issued by any thread with `CancelIoEx`, waits for
  their completion, and only then releases the event, handle, mirror and buffer
  lifetime. This replaces `CancelIo`, which only cancelled reads issued by the
  calling thread. The event creation failure path now checks for null and releases
  the opened directory. Access to one watcher still requires external serialization;
  calls may move sequentially between threads.
- Five focused regressions cover nested and coalesced case changes, file/ancestor
  deletion before polling, more than two metadata batches, nonrecursive scope,
  late native records, normal rename pairing and worker-issued read cancellation
  across 16 close/reopen cycles. Existing watcher tests now await asynchronous
  delivery and do not impose an ordering across independent watchers. They still
  verify event identity, type, action and count. The OSFile iterator test now owns
  a deterministic fixture and checks recursive traversal and `SkipFolder`, rather
  than assuming the executable directory contains subdirectories.
- Windows Debug DLL integration builds pass for FoundationTest, ToolsFoundationTest,
  Editor, EditorProcessor, EditorFrameworkTest and EditorEngineProcessFrameworkTest:
  `Workspace/editor-directory-reconciliation-integration-build.log`. The isolated
  static build also passes in `Workspace/editor-directory-static-build.log` and
  `Workspace/editor-file-iterator-static-build.log`. The additional iterator fixture
  compiles in the DLL build in `Workspace/editor-file-iterator-fixture-build.log`.
- Final validation passes, with 327 test executions across the selected runs:
  - 120 DLL Foundation executions across three ReFS IO suite passes:
    `Workspace/editor-directory-io-repeated-tests.log`.
  - 15 focused ReFS executions across three passes, plus five NTFS executions:
    `Workspace/editor-directory-recovery-repeated-tests.log` and
    `Workspace/editor-directory-recovery-ntfs-final-tests.log`.
  - 40 DLL Foundation executions on NTFS, including its forced fallback watcher:
    `Workspace/editor-directory-io-ntfs-tests.log`.
  - All 19 DLL tooling subtests on ReFS:
    `Workspace/editor-tools-directory-final-tests.log`.
  - All 40 static Foundation IO, five static watcher recovery and 19 static tooling
    subtests on ReFS: `Workspace/editor-directory-static-io-final-tests.log`,
    `Workspace/editor-directory-static-recovery-final-tests.log`, and
    `Workspace/editor-directory-static-tools-tests.log`.
  - All ten production Qt Editor workflows and 54 native Editor subtests:
    `Workspace/editor-directory-qt-tests.log` and
    `Workspace/editor-directory-native-tests.log`. Vulkan synchronization validation
    reports no validation errors; no assertion artifact is produced.
- The earlier ReFS filesystem-model failure is resolved for these verified DLL and
  static runs. This is recovery of known cached identities, not a replacement for
  native discovery of newly created names or an unconditional full-disk rescan after
  notification overflow. Intermediate filesystem states can coalesce. Linux, macOS,
  Release and D3D12 configurations were not exercised in this checkpoint.
  Deformed geometry execution/picking, legacy sky/probe/GreyBox migration and broader
  manual Editor workflow parity remain outstanding. The modernization goal remains
  incomplete.

## Native Skinning Evaluation And Spatial Pose Lifetime

- The current renderer investigation confirms that extracted skinning matrices and
  morph weights do not yet reach native vertex execution. Geometry residency shares
  immutable mesh buffers; surface, depth, forward, shadow and emissive consumers
  still read bind-pose vertices. The existing `Skinning.xiiShader` has no C++ caller
  and describes a different vertex layout from `xiiMeshPackedVertex`. It cannot be
  treated as an implemented deformation path. Morph target metadata also lacks a
  dedicated retained delta interface; authoring and animation import need further
  investigation before claiming morph support.
- Added allocation-free `xiiMeshSkinning` operations in GraphicsCore, split into a
  public header and implementation. Native ushort4 joint indices and byte4 weights
  now have one CPU evaluator shared with scene geometry export. Positive influences
  are normalized by their sum; zero-weight vertices and empty palettes preserve
  bind pose. Invalid referenced joints, non-finite results and non-affine matrices
  fail with `xiiResult` without modifying output. No exceptions or termination APIs
  were introduced.
- Conservative pose bounds enclose the bind box and every palette-transformed box.
  Their union encloses positive normalized blends, including vertices without bone
  influences. Using transformed boxes avoids relying on a transformed sphere under
  shear. Work is O(palette size), with no CPU vertex retention or GPU readback;
  bounds can be loose for large rigs. This establishes spatial bounds, not GPU
  deformation, motion history or deformed meshlet cones.
- `xiiSkinnedMeshComponent` now refreshes local spatial bounds after direct palette
  updates, successful pose application, failed pose application and clearing. Its
  palette setter returns `xiiResult`, rejects invalid input without replacing the
  current pose, and accepts a subrange of its own palette without tail destruction
  corrupting the input. Animation preview propagates the result, and generated mesh
  palettes use the shared affine validation.
- Tests verify normalized and zero-weight skinning, ignored invalid zero-weight
  indices, invalid joints, non-finite and projective matrices, overflow, atomic
  failure and aliasing. Bounds tests check every box corner across 256 blends of
  sheared/reflected/singular and translated transforms. Native Editor tests verify
  world bounds while simulation is disabled, palette clearing, failed application
  and clip reload. Geometry export verifies matching CPU-retained and GPU-only
  readback results and preserves the published file after invalid influences.
- The first new export run exposed a test pointer retained across `world.Update()`;
  the fixture now reacquires the component through its handle. A later full run
  reported temporal allocation-continuity failures; the unmodified full rerun and
  isolated temporal group passed. Inspection found that the identity fixture did
  not enforce its stated fixed-resolution premise: the production PID can resize
  under load. The fixture now pins and restores both resolution bounds, verifies
  32-by-32 output, and keeps all nine identity checks. Its final focused run passes.
- Windows Debug DLL builds pass for GraphicsCoreTest, EditorEngineProcessFrameworkTest,
  Editor, EditorProcessor and EditorFrameworkTest. Evidence:
  `Workspace/editor-skinning-core-build.log`,
  `Workspace/editor-skinning-integration-build.log`, and
  `Workspace/editor-skinning-final-build.log`.
- Verified runs total 88 passing subtest executions: all 21 GraphicsCore tests,
  all 54 native Editor tests with Vulkan synchronization validation, all ten Qt
  Editor workflow tests, plus the three temporal tests after pinning the fixture.
  Logs are `Workspace/editor-skinning-core-tests.log`,
  `Workspace/editor-skinning-native-final-tests.log`,
  `Workspace/editor-skinning-qt-tests.log`, and
  `Workspace/editor-skinning-temporal-final-tests.log`. No Vulkan validation errors
  or assertion artifact were observed in these final runs. The earlier failed run
  remains recorded in `Workspace/editor-skinning-native-tests.log`.
- Shared runtime GPU deformation and native picking remain incomplete. The next
  integration must carry current and previous instance poses through graph-declared,
  frame-safe uploads, preserve independent view/context state, update all native
  vertex consumers, and provide posed bounds/cone behavior. CPU export is not an
  alternative renderer. Legacy sky/probe/GreyBox migration and broader manual
  workflow parity also remain outstanding. This checkpoint does not establish
  Release, static GraphicsCore, D3D12, Linux or macOS behavior; the full modernization
  goal remains active.

## Native GPU Scene Skinning, Picking And Pose History

- This checkpoint implements the shared GPU palette path left open above. Scene
  databases retain current/previous affine matrices only for posed instances, keyed
  by the full generation-checked scene identity. Commit packs an immutable snapshot;
  equal current/previous palettes share a range, and sections/LODs follow their
  representative's range. Clearing preserves the preceding pose for one committed
  interval, then retires its CPU state. Destruction and database clearing discard
  palette state; geometry reload resets history as each context adopts the revision.
- The GPU instance ABI grows from 256 to 288 bytes for palette ranges and native
  ushort4/normalized-byte4 stream offsets. Reflection and the shared shader record
  match. There is no new persistent asset format or ownership transfer. Purely
  static scenes allocate no per-object palette arrays; posed instances retain two
  arrays and the committed snapshot. Packing and graph upload cost O(palette data).
- Visibility uploads a graph-owned structured matrix buffer with a copied snapshot,
  using the existing resource cache and queue/fence lifetime. Every native vertex
  consumer declares its read: scene depth/G-buffer/Forward+, shadow raster, mesh and
  indexed surface queries, emissive triangle construction, and the GPU-driven sample.
  Static scenes bind one identity matrix. No separate Editor deformation renderer,
  resource cache, bindless ownership system or exception-based failure path is added.
- A shared shader evaluator normalizes positive byte weights by their sum, ignores
  invalid zero-weight indices, preserves zero-weight bind vertices, bounds-checks
  referenced joints/streams and evaluates both pose intervals. Surface normals use
  the inverse transpose of the blended linear transform; tangent frames follow the
  transformed surface and reflection sign. G-buffer velocity uses the previous pose
  as well as the previous object/view transforms. Unposed vertices take an early
  return without palette access or deformation math.
- Frustum culling uses conservative posed instance bounds for skinned meshlets.
  Bind-pose cones and temporal HiZ rejection are bypassed while either pose interval
  is skinned, preventing false rejection when a palette moves with a fixed object
  transform. This trades culling precision for correctness; it does not compute
  tight deformed meshlet bounds or cones. Bounds/stream preparation runs once per
  extracted mesh group rather than once per expanded section/LOD candidate.
- Reload testing found that a surviving representative could become a member of a
  new group while retaining its old private palette. Extraction now clears that
  obsolete history. The test follows differently timed adoption in two contexts,
  verifies their independent snapshots, and checks shared ranges across sections.
- New GPU readback tests exercise both mesh and indexed queries, palette-only motion
  outside bind-pose visibility, pose clearing, normalized multi-bone weights with a
  non-255 sum, ignored invalid zero-weight joints, sheared/nonuniform normals, pose
  velocity and zero velocity after stopping. Transfer/async-compute/graphics reads
  run through the real render graph. Core tests cover invalid input, stale handles,
  generation reuse, clearing, section sharing and one-interval history.
- Removed `Shaders/Pipeline/Skinning.xiiShader`: the repository had no caller or
  reference to it, it lacked the current shader section format, and its vertex ABI
  differed from the native packed mesh streams. Its unconnected morph code was not
  an implemented runtime facility. Native consumers now share `GpuSceneSkinning.h`.
- Windows Debug DLL builds pass for GraphicsCoreTest, EditorEngineProcessFrameworkTest,
  EditorFrameworkTest, Editor, EditorProcessor and GpuDrivenScene:
  `Workspace/editor-gpu-skinning-verification-build.log` (exit zero). All 21 Core
  subtests and all 57 native Editor subtests pass. After the final preparation
  optimization, all three skinning and four native LOD subtests pass again. Logs:
  `Workspace/editor-gpu-skinning-core-final-tests.log`,
  `Workspace/editor-gpu-skinning-verified-native-tests.log`,
  `Workspace/editor-gpu-skinning-focused-final-tests.log`, and
  `Workspace/editor-gpu-skinning-lod-final-tests.log`. Native GPU tests use Vulkan
  synchronization validation; all three skinning tests also pass with GPU-assisted
  safe-mode validation: `Workspace/editor-gpu-skinning-gpuav-tests.log`.
- Full-sample GPU-assisted validation exposed a regression: evaluating both palette
  intervals in the sample's normal-only mesh shader produced device loss at the first
  frame. It reproduced with and without ray tracing and outside the restricted
  process environment. A reversible comparison with the previous sample shader
  completed 64 frames using the same new scene ABI, isolating the shader path.
  The evaluator now separates current-surface evaluation from history; sample,
  Forward+ and both surface query shaders evaluate only the pose they consume.
  G-buffer history is evaluated after the current surface, avoiding two simultaneously
  live blended matrices. This preserves deformation rather than bypassing it.
- The repaired full 24-by-16 sample completes 64 frames with GPU-assisted safe-mode
  and synchronization validation both without ray tracing and with it enabled:
  `Workspace/editor-gpu-skinning-sample-current-surface.log` and
  `Workspace/editor-gpu-skinning-sample-final-gpuav.log` (exit zero). The full native
  suite passes again after this repair, and all three skinning readback tests pass
  with the final current-only query shaders:
  `Workspace/editor-gpu-skinning-current-query-tests.log`. No Vulkan validation errors
  were observed in these final runs. Earlier sample failures and their assertion
  output remain under `Workspace/editor-gpu-skinning-sample-gpuav*.log` and
  `Workspace/editor-gpu-skinning-device-loss-assert.txt` as diagnostic evidence.
- All ten Qt tests pass, including the integrated production viewport, asset
  transformation, picking, capture and shutdown:
  `Workspace/editor-gpu-skinning-qt-elevated-tests.log`. The preceding restricted
  process run passed nine document tests but timed out waiting for an engine-process
  picking reply. The rerun passed without changing product code or test timeouts;
  the failed run and its engine-process log remain recorded for investigation.
- Ray-tracing acceleration structures still reference immutable bind-pose geometry;
  this raster palette path does not provide per-instance deformed BLAS. Morph delta
  retention/import and runtime deformation also remain open. Posed shadow/emissive
  bindings compile and run in the integrated pipeline, but dedicated posed-pixel
  assertions for those paths remain to be added. Legacy sky/probe/GreyBox migration
  and broader manual Editor workflow parity remain active work. Release, static
  GraphicsCore, D3D12, Linux and macOS have not been verified at this checkpoint.

### Editor Launch GPU Memory Exhaustion

The reported launch failure is recorded in the production Editor engine-process
logs as Vulkan `ErrorOutOfDeviceMemory` from texture creation, followed by the
fatal transient-allocation assertion in `xiiRenderGraphResourceCache`. The cache
retired resources correctly behind GPU completion, but no renderer called its
explicit stale-resource cleanup. Dynamic resolution and viewport resizing could
therefore retain every previous texture size, and variable upload sizes could do
the same for buffers.

`BeginFrame()` now trims completed idle allocations after four unused frames and
removes empty description buckets. Trimming precedes promotion of newly completed
resources, giving those allocations a chance to be reused even when completion
takes longer than the retention window. Active and retired GPU-in-flight resources
remain protected. Cache shutdown also resets its frame bookkeeping.

Allocation failure no longer asserts and then continues with null resources. The
cache returns null; render-graph execution reports `XII_FAILURE` with the failing
resource name and allocation dimensions or buffer size. Persistent allocations and
command-list creation also propagate failure. If recording has already changed
GAL's tracked states, execution closes and submits the valid prefix, aborts the
profiler frame, and stops before issuing commands against the failed resource.
The existing view failure path invalidates temporal history.

Verification:

- The new `RenderGraphResourceCacheTest.cpp` exercises two independent graph
  contexts over 64 GPU frames with changing texture and buffer descriptions.
  Disabling automatic eviction reproduces growth to 62 idle GPU resources per
  view (`Workspace/editor-oom-baseline-tests.log`, expected failure). The repaired
  cache peaks at two idle resources per view. Additional tests verify completion
  before reuse or eviction, reuse after a long completion delay, and failure
  propagation followed by successful execution. The failure test uses an
  uninitialized cache to return null deterministically instead of exhausting the
  test machine's physical memory.
- All 60 native engine-process subtests pass with Vulkan synchronization
  validation: `Workspace/editor-oom-native-tests.log`. The three new resource
  regressions also pass with GPU-assisted validation:
  `Workspace/editor-oom-gpuav-tests.log`.
- All ten Qt subtests pass: `Workspace/editor-oom-qt-resize-tests.log`. The
  production Editor integration test now drives 160 frames, including 96 distinct
  native viewport sizes and 64 stable frames at 1280 by 720, and verifies a final
  capture at that size. The fixture explicitly resizes the native child window;
  changing the requested fixed resolution alone leaves the Windows surface at its
  original size. The final engine-process log has no allocation, device-loss or
  Vulkan validation errors. The capture is under
  `Workspace/TestOutput/editor-oom-qt-resize/SourceWindowProject/NativeViewportResized.png`.
- All 21 GraphicsCore subtests pass: `Workspace/editor-oom-core-tests.log`.
  Debug builds of Editor, EditorProcessor and both Editor test applications pass
  (`Workspace/editor-oom-fixed-build.log`, `Workspace/editor-oom-final-build.log`,
  `Workspace/editor-oom-qt-build.log`). `git diff --check` passes.

This verification uses an isolated production Editor test project. The existing
user project and its preferences were left untouched. Release, other graphics
backends and other operating systems have not been tested for this repair.

The repair was reverified against the current Debug binaries on October 13, 2026.
All three `RenderGraphResources` regressions pass with Vulkan GPU-assisted
validation (`Workspace/editor-oom-current-gpu-tests.log`); the changing-resolution
test retains at most two idle allocations per view. The production Qt
`EditorIntegration` test also passes (`Workspace/editor-oom-current-editor-tests.log`),
including 160 native viewport frames with 96 changing sizes, the final 1280 by 720
capture, and Play/close/reopen. Its child-engine logs contain no out-of-device-memory,
device-loss, allocation-failure or Vulkan validation errors. Both commands return
zero. No further implementation change was required for this verification.

### Scene Hierarchy Duplication

Duplicate Special's grouping option previously reached an unimplemented assertion,
and its selection-size spacing controls were hidden. Grouping now constructs one
ordinary reflected game object per copy using the existing serialized hierarchy,
GUID remapping, document ownership and command history. A shared parent is retained;
mixed-parent selections are grouped at the document root. Source world poses,
negative and nonuniform scales, nested children and selection order are preserved.
Each group rotates about the average source position.

Copy roots are ordered before insertion. Sorting the entire recorded copy list by
source selection order after insertion interleaved different copies and invalidated
the relationship between redo order and recorded insertion indices. The new tests
exposed this issue and verify copy-major ordering and stable identities through
three undo/redo cycles and redo-history destruction.

The dialog obtains hierarchy bounds through the existing engine selection-bounds
query, with a distinct purpose from focus-on-selection. Axis spacing cycles through
positive extent, negative extent and zero. Missing or degenerate bounds disable
only the affected spacing helpers. Fractional random translation/rotation ranges
now use a nonzero Gaussian distribution, and stationary revolve placement works
with a nonzero start angle and zero angle step. Invalid authored parameters,
malformed parent mappings and singular grouping parents return failure before
hierarchy mutation. Graph preparation, transforms and modal orchestration are split
into cohesive source files.

Verification: Debug builds of EditorFrameworkTest, Editor and dependent tooling pass
(`Workspace/editor-duplicate-final-build.log`). All eleven Qt subtests pass
(`Workspace/editor-duplicate-final-tests.log`), including document-level grouped and
ungrouped transformations, mixed parents, invalid inputs, dialog controls, and the
production native viewport's real modal action, engine-provided spacing, mirrored
group insertion/removal and undo/redo. The full run also repeats the 160-frame
viewport resize stress test. Its engine log contains no allocation, validation or
access-violation errors. The initial restricted rebuild failed while copying Qt
plugins; the native-access retry passed. An earlier full run reported success but
its child engine logged a late access violation after unexpectedly entering Play
mode and closing the scene. That evidence remains under
`Workspace/TestOutput/editor-duplicate-native/Logs`; scene closure during Play is
being investigated separately rather than treating the test runner as sufficient
proof of child-process lifecycle correctness.

### Closing A Scene During Play

Explicitly entering Play mode and immediately closing the document reproduces a
second, independent access violation. The game state borrows the scene context's
world and owns a main render view and windows. Context teardown destroyed the world
without deactivating that state; an asynchronous Stop Play request could not
establish the required lifetime boundary. The next game-state update or teardown
then accessed the deleted world. The deterministic baseline fails the integration
test and logs `c0000005` and a lost IPC connection:
`Workspace/editor-play-close-baseline-tests.log` and its corresponding engine log
under `Workspace/TestOutput/editor-play-close-baseline/Logs`.

The scene context now deactivates its linked game state before clearing the mirror,
render views and world, using the existing game-application lifecycle. This releases
the state's view and windows while their world is valid and clears the borrowed
world link. Other documents' game states are unaffected. The integration fixture
now starts Play explicitly, closes the scene, reopens it, and requires a fresh
selection-bounds reply after another update interval. A child process which is
still alive inside its crash handler cannot satisfy that check.

Debug builds of EditorFrameworkTest and EditorEngineProcess pass
(`Workspace/editor-play-close-fixed-build.log`). All eleven Qt subtests pass with
Vulkan synchronization validation (`Workspace/editor-play-close-fixed-tests.log`),
including the previously failing close-during-Play path and the fresh reply from the
reopened world. The final engine log records Play startup and contains no access
violation, allocation failure or Vulkan validation errors. This remains Windows
Debug/Vulkan verification; other configurations have not been exercised.

### Asset References In Maps And Exposed Parameters

The common asset document scanner checked the map's dictionary for string type
instead of examining its entries, so map-based resource and file dependencies
were silently omitted. Its string-only property filter also skipped explicitly
annotated variant collections and the native prefab/script parameter maps. This
could omit required transform inputs, thumbnail inputs and packaged resources.
The document regression fails against the old implementation:
`Workspace/editor-dependencies-baseline-tests.log` (exit one).

Dependency collection now has a dedicated `AssetDocumentDependencies.cpp`. One
collection path reads individual array, set and map elements through the existing
type accessor. Asset/file attributes determine dependency flags; nested variant
collections are scanned only when explicitly annotated. Empty strings, numeric
values, ordinary labels, temporary properties and temporary subtrees are excluded.
Existing prefab-default providers continue to exclude inherited references, whose
dependencies belong to the source prefab.

Authored exposed parameter overrides resolve their source asset through the
existing curator and use its saved `xiiExposedParameters` metadata. Per-parameter
asset/file flags are copied under the curator lock, without retaining metadata
pointers or introducing another cache. Unknown parameter names and unresolved
metadata do not become inferred resource references. Source resource dependencies
continue to be collected independently. Changes to source metadata affect the
next collection immediately.

Debug builds of EditorFrameworkTest, EditorProcessor and Editor pass
(`Workspace/editor-dependencies-final-build.log`). All twelve Qt subtests pass
(`Workspace/editor-dependencies-native-tests.log`), with a clean child-engine log.
The new tests cover all dependency flags, maps, arrays, sets, annotated variants,
nested collections, string views, duplicate references, temporary subtrees and
command undo/redo. Integrated coverage authors and saves a real prefab, exposes
native mesh/material properties, edits a native prefab-reference component's
parameters, instantiates the prefab through the production command, saves inherited
and overridden dependency metadata, and removes source parameter metadata. The
source prefab also completes its regular resource transform and thumbnail workflow.
The existing viewport resize and Play-close-reopen regressions run in the same
suite. Verification remains Windows Debug/Vulkan; broader asset compatibility and
other configurations remain part of the active modernization effort.

The final unresolved-source case also passes: the source resource remains a
dependency, and unresolvable parameter metadata adds no guessed references.
Undo restores collection from the current source metadata. The final integration
rerun is `Workspace/editor-dependencies-final-check-tests.log` (exit zero), with
the incremental test build in `Workspace/editor-dependencies-final-check-build.log`.

### Property Collection Failure Boundaries

The shared variant storage accessor accepted array removal at `index == count`,
including removal at zero from an empty array, and then called out-of-bounds
container removal. Direct conversion of numeric indices could also wrap oversized
integers or truncate fractional values into unrelated valid elements. Insertion
accepted negative values other than the documented append sentinel. These paths
underlie reflected document properties and exposed parameter editing.

One checked numeric-index conversion now rejects negative, fractional, non-finite
and narrowing-overflow inputs before accessing writable storage. Removal and reads
require an existing position; insertion and movement retain their existing end
position semantics, and insertion retains `-1` append. Failed operations preserve
values. Successful reads reset the supplied status, key queries replace their
output and clear it on failure, and dictionary move errors now name the property
without an invalid format argument. All failures use the established `xiiStatus`
wrapper around `xiiResult`.

The new ToolsFoundation reflection regression covers empty and nonempty bounds,
oversized integers, fractional values, NaN/infinity, append, end-position moves,
failure atomicity, dictionary collisions, repeated key queries and status reuse.
All twenty ToolsFoundation subtests pass:
`Workspace/editor-variant-accessor-tools-tests.log`. Debug builds of
ToolsFoundationTest, EditorFrameworkTest, EditorProcessor and Editor pass:
`Workspace/editor-variant-accessor-fixed-build.log`.

All twelve native Qt subtests also pass with the repaired shared accessor:
`Workspace/editor-variant-accessor-native-tests.log`. The child engine log has no
allocation, access-violation or Vulkan validation errors. This remains Windows
Debug/Vulkan verification. Older stored asset dependency headers are refreshed
when their documents are saved; this change does not silently rewrite every
existing project asset. Legacy scene migration and broad workflow parity remain
active work.

### Graph-Based Asset Resaving And Metadata Refresh

Bulk resaving previously used a repeated dependency scan with an arbitrary
dead-end fallback. It considered transform dependencies alone, treated dependencies
outside a selected subtree as unresolved ordering constraints, and retained asset
info pointers and the curator lock across document saves and progress callbacks.
Failures were only logged, so the processor's resave command could report success
after failed writes.

`AssetResave.cpp` now snapshots the selected asset documents into Core's directed
graph. Transform, thumbnail and package references resolve through the existing
curator, including paths and sub-assets mapped to their owning documents. Only
selected documents become vertices. Stable path/UUID input ordering makes plans
deterministic. Core topological sorting orders dependencies first; Core strongly
connected components identify actual cycle participants without confusing a
blocked downstream asset with a cycle member. Cycles fail before any document
write. Graph algorithms take O(V + E), with O(V log V) input sorting and the
normal curator reference-resolution cost.

The public read-only `GetAssetResaveOrder()` permits preflight. `ResaveAllAssets()`
returns `xiiStatus`, rejects reentry and non-main-thread saves, and reports
cancellation or aggregated write failures. Saving resolves each stable UUID again
after progress events, validates document identity, keeps already-open documents
open and closes documents opened by the batch. Read-only and partially understood
documents are rejected to avoid destructive resaves. Curator locking covers
snapshots and lookups, rather than document creation, saving or event pumping.
The asset browser handles the result and the processor sets a failing exit code.

The integration tests exposed stale dependency metadata after a cycle was removed
and saved rapidly. The file model relied on modification timestamps even for
explicit known-write notifications. It now supports forced file-change detection;
hash invalidation and notification selection occur together under its lock.
Explicit curator write notifications invalidate cached headers, and watcher
modification events use the same forced path. Header-cache reuse also requires a
nonzero matching content hash. Ordinary filesystem checks retain their timestamp
fast path. No second cache or asset format is introduced.

The native resave fixture uses real registered document managers, saved source
files, curator sub-assets and document save notifications. It covers all dependency
categories, external dependencies, parallel references by path and sub-asset UUID,
deterministic ordering, open/closed document ownership, empty scopes, cancellation,
reentry, read-only and unknown-type protection, self/multiple-asset cycles and
recovery. A worker snapshots the curator during a progress callback, proving that
the operation does not retain its global lock there. The Windows regression edits
dependency bytes while deliberately restoring the original file timestamp,
verifies both tracked and physical timestamps are unchanged, and checks immediate
cycle detection and recovery after explicit notification.

Deleting the current asset during a progress callback is reported as failure and
does not recreate the deleted source from an open document. The Editor's normal
removal handling can close that document, so the fixture restores source bytes and
reacquires the document by its preserved UUID. File mutation tests wait for short
background-reader locks; the timestamp test acquires an exclusive file handle
before locking the model, avoiding a reader blocked on that model lock.

Verification:

- Debug builds of EditorFrameworkTest, ToolsFoundationTest, EditorProcessor and
  Editor pass: `Workspace/editor-asset-resave-restored-build.log`. The final fixture
  build is `Workspace/editor-asset-resave-access-build.log`.
- All twenty ToolsFoundation subtests pass with the final shared library:
  `Workspace/editor-asset-resave-restored-tools-tests.log`.
- All twelve Qt subtests pass: `Workspace/editor-asset-resave-access-tests.log`.
  The child engine log has no allocation, access-violation or Vulkan validation
  errors. Native picking, 160 viewport resize/stable frames, capture,
  Play-close-reopen, prefab metadata and all new resave cases execute in that run.
- Disabling forced notification in a reversible comparison reproduces stale-cycle
  recovery and unchanged-timestamp failures:
  `Workspace/editor-asset-resave-timestamp-baseline-tests.log` (expected exit one).
  Forced notification is restored in both source and the final built library.
- Two intermediate runs exposed the viewport fixture aborting after a two-second
  IPC observation timeout while an initial native frame took about 8.5 seconds.
  Picking now polls the same outstanding request within its existing ten-second
  readiness deadline, retaining the exact expected-owner check. The final full run
  verifies this behavior. Diagnostic logs remain under
  `Workspace/editor-asset-resave-restored-tests.log` and
  `Workspace/editor-asset-resave-lifetime-tests.log`.

This verification remains Windows Debug/Vulkan. Processor failure routing is
implemented and built; an unrestricted processor resave against an existing user
project and manual browser-menu interaction were not run. Existing project assets
were not rewritten by these tests. Resaving refreshes compatible source metadata;
it does not convert obsolete scene component semantics. Legacy scene migration and
broader workflow parity remain active work.

### Package Dependencies, Dependency Inspection And Graph Export

The curator hashed package references but omitted them from reverse tracking. Native
mesh and material references marked solely `Package` were therefore absent from
usage searches and transitive invalidation. UUID tracking also looked only in the
main-asset table, leaving sub-asset dependencies unresolved indefinitely. Raw file
content contributed to thumbnail hashes but was omitted from package hashes.

The existing curator now tracks transform, thumbnail and package categories
independently, resolves sub-assets to their owning documents, and resolves pending
references after updating the owner's sub-asset list. Changes, additions, removals
and transform errors invalidate package consumers transitively. Adding a file can
repair a cached missing-dependency state. Reference removal handles targets that
have already disappeared; that uncommon case scans the category's reverse map to
discard the owner's remaining aliases. Normal untracking removes empty buckets.
Project shutdown clears all reverse trackers and unresolved-reference sets.

Package references participate in the curator's existing acyclic hashing
contract, with sub-asset identities normalized for cycle checks and property
selection validation. This restriction belongs to asset hashing; Core's generic
graph still permits cycles. Runtime file changes update package and thumbnail
hashes while leaving the containing asset's transform hash unchanged. Usage
searches traverse independently of prefilled output sets. The file-import queries
retain their documented transform-only meaning.

Dependency inspection and reverse queries now live in the cohesive
`AssetDependencyGraph.cpp`. Forward traversal resolves UUIDs and asset paths,
visits each owner once, preserves unresolved references as leaves, and continues
through references already present in the output. The asset processor's refresh
hull includes package dependencies. DGML export includes package edges, preserves
missing UUID nodes, and returns `xiiResult` to the existing status dialog.

The shared Foundation DGML writer also discarded direct-write failures and
buffered flush failures. It now checks both results before reporting success.
`DGMLWriterTest.cpp` mounts a test-only write-failing data directory to exercise
successful output, failed opening, failed buffered flushing and failed writes
larger than the file writer's cache. It does not exhaust disk space or alter file
permissions. The unchanged writer fails both injected write cases in
`Workspace/dgml-write-failure-baseline-tests.log`; the repaired Utility suite passes
all seven subtests in `Workspace/dgml-write-failure-fixed-tests.log`.

`AssetDependencyTrackingTest.cpp` exercises real saved test documents through the
production curator. Coverage includes consumer-first loading of sub-asset
references, direct/transitive usage, prefilled output, category-specific reverse
hulls, cached hash invalidation, runtime-file changes/removal/restoration, package
cycle detection/recovery, category overlap, reference removal, owner deletion and
restoration with stable identity, unresolved forward queries, cyclic traversal,
and real DGML contents and failed export. The saved test document and its manager
are shared with resave tests in `AssetTestDocument.h/.cpp`.

Verification on October 13, 2026:

- Debug builds of FoundationTest, EditorFrameworkTest, EditorProcessor and Editor
  pass: `Workspace/dgml-write-failure-fixed-build.log`.
- All twelve editor subtests pass against the final Foundation DLL:
  `Workspace/editor-dependency-final-tests.log`. This includes the native viewport
  stress, picking, Play/close/reopen, duplication, exposed dependencies and resave
  workflows. Child-engine logs contain no allocation, device-loss or Vulkan
  validation errors.
- Earlier stage builds and passing runs remain in
  `Workspace/editor-package-tracking-lifetime-*` and
  `Workspace/editor-dependency-graph-*`. The initial tracking run's only error was
  the deliberate missing-file diagnostic, which the fixture now explicitly expects.
- `git diff --check` passes. User project assets were not rewritten.

Verification remains Windows Debug/Vulkan. A dedicated external EditorProcessor
cache-refresh round trip, manual dependency-export dialog interaction, other
platforms and Release remain unverified. Legacy scene migration and broader
workflow parity remain active work.

### Production Asset Processor Requests And Failure Responses

The dedicated EditorProcessor round trip previously listed as unverified is now
covered by `AssetProcessorTest.cpp`, using the production executable, IPC channel,
document managers and resource serialization. Its native surface asset references
a prefab through the package-only `SlideReaction` property. No test-only asset
type or alternate processor implementation is loaded into the child process.

`xiiAssetProcessor::PrepareAssetRequest()` assembles the owner identity, document
path, three hashes, active profile name and all dependency categories through the
curator. Failed preparation leaves the output unchanged. The worker keeps that
complete message through process startup and sends it directly; it no longer
combines previously computed hashes with a profile name queried later at dispatch.
Request assembly lives in `AssetProcessing.cpp`. Separate hash/hull bookkeeping
and unused process-task connection/crash wrappers and a dead running flag were
removed. The removed crash wrapper incorrectly returned client-alive state.

An unknown profile previously logged an error but returned the response's default
successful `xiiTransformStatus`. The processor now returns a failure with the
profile name in its message. Failed host setup also sets exit code one, including
failure to parse the supplied host PID.

The native IPC fixture verifies:

- A warm request and package-only dependency membership through the same request
  preparation function used by the background worker.
- An invalid preparation preserving the existing request.
- A changed prefab updating package hashes without changing the surface's
  transform hash. On Windows the source's exact original native write timestamp
  is restored and read back before the second request. Valid processor replies
  contain no parent/child hash-disagreement warning.
- Unknown-profile and unknown-asset failures, followed by a successful valid job
  in the same processor.
- Deletion of the surface output followed by real child-process transformation.
  The generated file's asset header hash, changed restitution value and serialized
  prefab reference are read back and compared with the request and source values.

The test target now depends on EditorProcessor, which already depends on its
engine process, plugins and converter. A build of the test target therefore also
builds the production tools it launches. The Startup test's PCH remains the first
include in its own block.

Verification on October 13, 2026:

- The original processor fails exactly the two unknown-profile assertions in
  `Workspace/editor-processor-request-baseline-tests.log`. Other new IPC assertions
  pass in that run.
- All twelve editor subtests pass with the corrected processor in
  `Workspace/editor-processor-request-fixed-tests.log`, including the existing
  native viewport and document workflows.
- Debug builds of EditorFrameworkTest, its EditorProcessor dependency and Editor
  pass: `Workspace/editor-processor-request-fixed-build.log`.
- An explicit hidden process wait captures exit zero for invalid host PID input
  before the fix (`Workspace/editor-processor-host-baseline-waited.exit`) and exit
  one afterward (`Workspace/editor-processor-host-fixed.exit`). Both production
  logs record the expected host-connection failure. A direct PowerShell GUI launch
  without that wait did not provide a usable exit code and is not exit-code evidence.
- Engine-process logs contain no GPU allocation, device-loss or Vulkan validation
  failures. Invalid-profile/asset diagnostics in the processor are intentional.
  `git diff --check` passes; existing user project assets remain untouched.

Profile switching remains under review: `SetActiveAssetProfileByIndex()` scans the
filesystem but does not call `ComputeAllDocumentManagerAssetProfileHashes()`, while
document managers return cached profile hashes. Profile edits and background
request synchronization need dedicated regression coverage. This checkpoint does
not claim to resolve that separate issue, legacy scene migration, manual UI parity,
Release or other platforms.

### Profile-Aware Asset Evaluation And Header Read Failure Recovery

Document managers retained a single profile hash computed during curator startup.
Profile switching and edits did not refresh it. `IsAssetUpToDate()` also discarded
its profile argument, and output checks resolved the active profile's paths even
when a caller requested another profile. A valid existing active-profile output
could therefore suppress generation of an explicitly requested target.

The manager hash cache and its startup-only initialization API were removed.
`GetAssetProfileHash(profile)` calculates the existing type-specific transform hash
from the requested settings. Texture, cubemap and decal implementations already
derive these hashes from small configuration records. Dependency hashing forwards
the same profile throughout the query, and all output checks, including material
shader outputs, use that profile's paths. Non-active queries bypass the shared
active-profile cache and do not replace its state or hashes. Active results are
published only while the profile and asset revision still match the calculation.
These are Editor SDK signature/layout changes; repository consumers and plugins
were rebuilt, with no compatibility alias or new asset format.

Forced and non-active evaluations share transient results by owning asset UUID
within one query. Shared dependencies are not repeatedly recalculated through
diamond-shaped graphs. No per-profile persistent cache was added. Known cycles
return `CircularDependency` even for forced queries or another profile, instead
of bypassing the cached cycle result and recursing into it. Unknown identities
start with zero output hashes.

The first native test exposed another authoring defect: reflection initialized
new texture address modes to the native enum's `Unknown` value, despite the C++
members defaulting to `Wrap`. Explicit default attributes now match the C++ values
for U, V and W. This repairs new render-target transformation without coercing
persisted explicit invalid sampler settings.

`AssetProfileTest.cpp` verifies native render-target creation and file contents,
active-profile edits, distinct target hashes and serialized dimensions, package
hash propagation, active-cache isolation, switching in both directions, and forced
cycle rejection. Five fixture assets in a shared dependency graph each perform
exactly one output check per ordinary or forced non-active query. A pending
automatic transform may complete during the profile switch; the test accepts that
valid timing while retaining exact hash, header and dimension checks.

Verification also exposed a real file-removal race in header acquisition. The
curator asserted that header data existed even when `ReadDocument()` had failed
before invoking its callback. It now propagates `XII_FAILURE` and preserves the
known asset metadata. The file model treats a missing file as an ordinary failure
without logging it as locked/corrupt. Tools tests verify that no callback or model
entry is created. The resave fixture additionally holds a Windows read handle that
actually denies other readers, checks that a probe is blocked, expects the open
failure, and verifies metadata retention and recovery after unlocking.

Verification on October 13, 2026:

- Debug builds of ToolsFoundationTest, EditorFrameworkTest, EditorProcessor and
  Editor pass (`Workspace/editor-profile-evaluation-read-build.log` and the final
  fixture build `Workspace/editor-profile-evaluation-lock-build.log`).
- All twenty ToolsFoundation subtests pass:
  `Workspace/editor-profile-read-tools-tests.log`.
- All twelve editor subtests pass:
  `Workspace/editor-profile-lock-final-tests.log`. Native engine logs contain no
  GPU allocation, device-loss or Vulkan validation failures.
- Temporarily disabling only explicit-profile forwarding fails four checks,
  including package hashes and the requested target output:
  `Workspace/editor-profile-evaluation-control-tests.log`. The command restored
  the source byte-for-byte afterward, followed by final production builds/tests.
- The initial run records invalid new sampler defaults in
  `Workspace/editor-profile-evaluation-baseline-tests.log`. The first full memoized
  run records the deletion/read assertion in
  `Workspace/editor-profile-evaluation-final-tests.log`. The intermediate lock
  fixture used metadata-only access and did not deny reads; its corrected version
  explicitly verifies that precondition.
- `git diff --check` passes. Existing user assets were not rewritten.

This resolves profile hash evaluation, not the entire profile editing lifecycle.
Profile objects must remain alive and unchanged during an evaluation. Atomic live
editing, profile-definition reload in already-running processors and save-failure
handling in the profile dialog still need work. Release, other platforms, manual
UI parity and legacy scene migration remain unverified or incomplete.

### Saved Profile Reload In Running Asset Processors

Background processors previously loaded `Editor/AssetProfiles.ddl` only during
startup. Editing an existing profile left a running processor with stale settings;
adding a profile left it unknown. The processor could generate a render target with
the wrong dimensions and disagree with the editor's requested hashes. Curator
shutdown also wrote its profile objects back to disk, allowing a stale processor
or an initialization fallback to overwrite definitions it did not author.

`ReloadAssetProfiles()` is now the curator's single profile-loading path. It reads
and fingerprints the file contents, independently of cached timestamps. Unchanged
contents preserve the existing objects and avoid a curator rescan. Changed contents
are parsed into an owned temporary collection. Validation checks reflected roots,
owned configuration references and types, duplicate configuration types, empty
names and case-insensitive duplicate names before publishing anything. The default
profile is placed at index zero. Missing defaults are constructed with existing
configuration defaults. Reload preserves the active profile by name, falling back
to Default if it was removed.

On a successful change, background hashing is stopped before replacing objects.
Replacement occurs under the curator mutex, followed by active-state reevaluation
and the existing engine-process platform notification. The task wait occurs
outside the mutex. The API is for the main thread between asset operations; a
successful changed reload invalidates previously acquired profile pointers.
Rejected definitions leave the live collection, active selection and loaded-file
fingerprint unchanged. This is not a concurrent profile-editing API.

The processor reloads before selecting a job's named profile. Invalid definitions
produce an explicit failed response. Standalone transformation also validates
definitions and now exits with code one for an invalid file or unknown profile;
unknown profiles previously logged an error and exited zero. Profile persistence
is confined to explicit saves and initial default creation. Shutdown no longer
rewrites profiles. Startup retains an existing invalid file, and processors do not
create fallback definitions on disk. The reflected asset/profile formats are
unchanged, and no alternate configuration cache or compatibility path was added.

Verification on October 13, 2026:

- Debug builds of EditorFrameworkTest, EditorProcessor and Editor pass:
  `Workspace/editor-profile-reload-final-build.log`.
- All twelve editor subtests pass:
  `Workspace/editor-profile-reload-final-tests.log`. One running production
  processor generates native render targets at 640, 896 and 768 pixels with exact
  header hashes. The Windows fixture restores the profile file's exact modification
  timestamp between two distinct configurations. It exercises added and removed
  profiles, eight invalid-definition cases, metadata retention and recovery.
- Temporarily disabling only the processor's reload call causes nine regression
  failures, including wrong dimensions, mismatched hashes and rejection of the new
  profile: `Workspace/editor-profile-reload-control-tests.log`. The command restores
  the source automatically; final builds and tests use the production implementation.
- Real standalone processor launches verify error text and exit code one for an
  unknown profile and an invalid definition. The invalid input survives startup and
  shutdown unchanged. The pre-fix unknown-profile run exits zero:
  `Workspace/editor-profile-reload-cli-baseline.exit` and its stdout log.
- The first integrated run exposed a fixture reader retaining a generated surface
  output across a curator refresh. Closing it before the profile test fixes the
  pending transform's write failure; the failed run is retained in
  `Workspace/editor-profile-reload-tests.log`. Final native engine logs contain no
  GPU allocation, device-loss or Vulkan validation errors. `git diff --check` passes.

Profile-dialog edits still need an atomic publication/persistence boundary, correct
selection mapping after multiple insertions/deletions and surfaced save failures.
These changes do not establish arbitrary concurrent profile editing or resolve the
other outstanding editor workflows, legacy scene migrations and deformation paths
recorded above. Release, other backends and other operating systems remain untested
for this checkpoint. Existing user projects and assets were not rewritten.

### Staged Profile Editing And Persistence

The profile dialog previously applied UUID-sorted bindings directly to live native
objects before saving. Multiple additions could therefore disagree with the
selected document row. Deleting the active profile failed silently, property
updates could overlap background hashing, and failed writes still closed the
dialog. An unused property-change handler retained another path to those raw
native pointers.

The dialog now owns only its document draft and profile UUIDs. It derives the
complete proposed native collection in document order when committing. Selection
and the original active label follow document identities through insertion and
deletion. Removing the selected target falls back to Default. Cancellation does
not change native objects or files. The binding map and unused live-edit handler
were removed. Case-only renaming excludes the object being renamed from the
uniqueness check. The project action no longer performs a second reevaluation.

`ApplyAssetProfiles()` validates names, uniqueness, Default and the active index,
clones the caller-owned input, and writes fully flushed staged runtime files and
DDL definitions. It stops dispatch and gracefully joins outstanding processor
work, then stops background hashing outside the curator mutex. Files are installed
with originals retained as uniquely named backups; definitions are installed last.
Removed profiles' runtime files participate in the same rollback. Live definitions
are replaced under the mutex only after installation succeeds. The loaded-file
fingerprint is updated and the existing platform notification reevaluates assets
and updates the engine process. Previously running processing is restarted.

If installation fails, installed files are rolled back in reverse order. The
dialog reports the failing path and retains its draft for retry. Failed recovery
reports retained backup paths. Successful saves remove backups; staged-file cleanup
failures are logged. Runtime cleanup never derives a removal path from an invalid
old native profile name. The private persistence implementation is separated into
`AssetProfilePersistence.cpp`; no generic transaction framework was introduced.

`xiiPlatformProfile::SaveForRuntime()` previously ignored writes made by the chunk
writer and the file writer's destructor. It now serializes through existing memory
streams, then checks the actual file copy and final flush. The curator's runtime
save API returns `xiiResult`, and its callers check that result. Initial default
save failures are logged. Runtime chunk and reflected DDL formats are unchanged.

The integrated rollback test exposed a Windows watcher defect. Casing reconciliation
treated an unchanged leaf at a rapidly replaced path as proof of a case-only rename,
discarded its old-name event, then asserted on the new-name event. An unchanged
leaf now follows normal rename pairing. The dedicated replacement/rollback fixture
verifies four complete pairs and preservation of the original bytes, alongside
the existing case-change and forced non-NTFS coverage.

Verification on October 13, 2026:

- Debug FoundationTest, CoreTest, EditorFrameworkTest, EditorProcessor and Editor
  builds pass (`Workspace/editor-profile-publication-fixed-build.log` and the final
  `Workspace/editor-profile-publication-graceful-build.log`).
- All twelve editor subtests pass:
  `Workspace/editor-profile-publication-graceful-tests.log`. The native Qt dialog
  fixture adds two profiles, selects the second, deletes the original active
  profile, and verifies exact native order and selection after accepting.
- A Windows handle denies replacement of the last definitions file. The fixture
  verifies the error dialog, unchanged native pointers, exact original file bytes,
  restoration of the deleted runtime profile, removal of newly installed runtime
  files, retained draft edits and successful retry after unlocking. Cancellation
  and unchanged reload also preserve their required state. Idle processor stop,
  graceful join and restart are exercised before opening the test project.
- All forty Foundation subtests selected by the IO filter and all six
  DirectoryRecovery subtests pass (`Workspace/editor-profile-publication-io-tests.log`
  and `Workspace/editor-profile-publication-watcher-tests.log`). This includes
  ordinary and forced non-NTFS watchers.
- The Core Configuration regression passes:
  `Workspace/editor-profile-publication-core-final-tests.log`. A fault-injection
  mount verifies successful 18-byte output, open denial and buffered flush failure.
- The first integrated run records both the watcher assertions and a fixture
  confirmation that ended the message box without clicking its Yes button:
  `Workspace/editor-profile-publication-tests.log`. The fixture now clicks the
  actual standard button. Final logs contain no GPU allocation, device-loss or
  Vulkan validation errors. No staged files or backups remain, and `git diff --check`
  passes. Existing user projects and assets were not rewritten.

Publication is coordinated within the editor, not a crash-safe filesystem
transaction across several files or a concurrent multi-editor editing protocol.
Graceful publication can wait for active transforms. The controlled fixture does
not deliberately hold a long-running managed job open during commit. Release,
other backends and other operating systems remain untested. Broader workflow
verification, legacy scene migration and the deformation paths recorded above
remain outstanding; this checkpoint does not establish full goal completion.

### Temporal History Allocation And TAA Resize Retention

The follow-up allocation audit found a second unbounded retention path outside the
render-graph cache. TAA appended both replaced history textures to a per-view
`m_RetiredTAAHistoryBuffers` array on every resolution change and never released
them until the view was destroyed. The earlier graph-cache retention regression
did not cover this separately owned state. The array and its append path have now
been removed. Graph imports retain textures while required by the current graph;
GAL releases native images through its existing queue-completion-aware deferred
deletion. No additional retirement queue was introduced.

History texture allocation previously happened in void graph-setup callbacks.
The shared helper asserted on failure, and several direct allocation loops then
dereferenced the result without a failure path. Allocation is now prepared before
the relevant built-in stage registers callbacks. `BuiltinRenderPipelineMemory.cpp`
owns the stage-specific preparation for sixteen ping-pong texture pairs and the
exposure buffer. Screen-space reflection histories are prepared in their actual
producer stage, OpaqueComposition. The pass setup callbacks only import the
successfully prepared objects.

The small private memory helper reuses a pair only when both full descriptions
match. It allocates replacements locally and publishes them together after both
succeed. The internal factory parameter permits deterministic failure tests with
real GAL textures without exhausting physical GPU memory. On failure, stage
preparation releases its obsolete pair, invalidates that history, logs its name,
dimensions and format, and returns `XII_FAILURE`. The view invalidates all built-in
and resource-pass histories when synchronization or pipeline construction fails.
The existing render-world failure path stops before graph execution. Missing
devices are queried through `HasDefaultDevice()` before the asserting getter.

Verification on October 13, 2026:

- Debug builds of EditorEngineProcessFrameworkTest, GraphicsCoreTest,
  EditorFrameworkTest, EditorProcessor and Editor pass:
  `Workspace/editor-temporal-memory-final-build.log`.
- All 63 native engine-process subtests pass with Vulkan synchronization validation:
  `Workspace/editor-temporal-memory-native-tests.log`. The rendered history tests
  also pass with GPU-assisted validation in `Workspace/editor-temporal-memory-tests.log`;
  that earlier run subsequently fails the new missing-device fixture because its
  initial implementation called the asserting getter before checking availability.
- All three new TemporalMemory regressions pass with GPU-assisted validation on
  final production binaries: `Workspace/editor-temporal-memory-final-tests.log`.
  They check exact pair reuse without allocation, failures of either allocation,
  partial replacement release, resize and format changes, and recovery. A 64-size
  TAA fixture uses exported graph textures to verify the original allocation loses
  its runtime ownership. Resource-backed failure fixtures drive `world.Update()`
  and check history invalidation for Lighting, OpaqueComposition, AutoExposure and
  TAA, followed by a successful TAA build.
- Temporarily reinstating the old lifetime-long TAA retention produces 63 ownership
  failures, one per changed size:
  `Workspace/editor-temporal-memory-control-tests.log`. The command automatically
  restores both production source files byte-for-byte. The final build and focused
  tests use the restored implementation.
- All 21 GraphicsCore subtests pass:
  `Workspace/editor-temporal-memory-core-tests.log`.
- All twelve editor subtests pass:
  `Workspace/editor-temporal-memory-editor-tests.log`, including the production
  viewport's 160-frame resize stress, capture and Play/close/reopen. Its engine logs
  contain no allocation, device-loss or Vulkan validation errors. `git diff --check`
  passes. Existing user assets and preferences were not rewritten.

This corrects TAA retention and temporal allocation propagation, not every
persistent renderer allocation. The audit also identified lifetime-long retention
of replaced calibrated sensor outputs in `m_RetiredSensorOutputTextures`; that
separate path still needs migration and native coverage. Deliberate physical GPU
exhaustion was not performed. Pair-allocation null results and missing-device
failures are deterministic fixture inputs. Release, other backends and other
operating systems remain untested, and the broader editor modernization goal
remains incomplete.

### Calibrated Sensor Output Lifetime

The remaining pipeline-owned retirement array retained every replaced calibrated
sensor output until view destruction. Profile switching and sensor resolution
changes could therefore grow memory independently of the graph cache and TAA.
`m_RetiredSensorOutputTextures` and both append sites have been removed. A profile
change clears the view's current output. Existing graph imports and caller-owned
shared pointers retain images for their actual uses, with GAL deferring native
destruction behind submitted GPU work.

`PrepareSensorOutput()` now runs before ScreenEffects setup. It resolves the
immutable sensor profile and allocates the exact RGBA32Float output description,
reusing an existing matching texture. Invalid profiles or allocation failure
return `XII_FAILURE` before setup imports or execution. Allocation failures report
the requested dimensions and format. The old void callback could return silently
after failure and leave sensor execution absent from an otherwise successful
pipeline. The output accessor documents shared ownership and GPU synchronization
for asynchronous readback. No new cache, retirement queue or Qt dependency was
introduced.

The native regression uses the real default pipeline and two views sharing one
resource definition. It switches one view through 64 sensor sizes while preserving
the other view's output, and covers unchanged selection, disabling, retained
readback ownership and re-enabling. It explicitly seeds nonzero texture contents
before rendering the empty scene, then maps the GPU result to distinguish actual
sensor dispatch from zero-filled fresh memory. A resource-backed failure fixture
checks rejection before setup and subsequent successful pipeline execution.

The current folder-unity build also exposed an existing ModelImporter collision:
MeshImport.cpp and SkeletonImport.cpp both declared anonymous `PendingNode`
records with different layouts. The skeleton record now has the specific internal
name `PendingSkeletonNode`, preserving the algorithm and serialized formats.
This build integration fix is committed separately. The initial compiler evidence
is retained in `Workspace/editor-sensor-memory-build.log`; the later test-compile
run records explicit shared-pointer borrowing corrections in
`Workspace/editor-sensor-memory-unity-build.log`.

Verification on October 13, 2026:

- Debug EditorEngineProcessFrameworkTest, GraphicsCoreTest, EditorFrameworkTest,
  EditorProcessor and Editor builds pass with folder unity enabled:
  `Workspace/editor-sensor-memory-final-build.log`.
- Both SensorMemory regressions pass:
  `Workspace/editor-sensor-memory-tests.log`. GPU readback first verifies every
  channel of the seeded image is 0.75, then verifies the same allocation is fully
  overwritten with the empty scene's zero/invalid output. A retained output remains
  readable after disabling the sensor.
- Both regressions also pass with Vulkan GPU-assisted and synchronization
  validation: `Workspace/editor-sensor-memory-gpuav-tests.log`. The validation layer
  reports instrumentation performance warnings, with no allocation, device-loss
  or Vulkan validation errors.
- Temporarily restoring the original sensor retirement behavior causes 65
  ownership failures: 64 profile changes and disabling the current output.
  `Workspace/editor-sensor-memory-control-tests.log` retains that expected failure.
  The command restores both production files byte-for-byte before the final build.
- All 65 native engine-process subtests pass with Vulkan synchronization validation:
  `Workspace/editor-sensor-memory-native-tests.log`. This includes the ModelImporter
  skeleton-import regression and the existing graph-cache and temporal-memory tests.
- All 21 GraphicsCore subtests pass:
  `Workspace/editor-sensor-memory-core-tests.log`.
- All twelve editor subtests pass:
  `Workspace/editor-sensor-memory-editor-tests.log`, including the production
  viewport's resize stress, capture and Play/close/reopen. Its engine logs contain
  no allocation, device-loss or Vulkan validation errors. `git diff --check` passes.

Existing user projects, assets and preferences were not rewritten. This verifies
sensor-output lifetime and empty-scene dispatch/readback; it does not establish
physical sensor calibration accuracy across scene geometries or backends.
Deliberate physical GPU exhaustion was not performed. Release, other backends and
other operating systems remain untested. Broader editor workflow verification,
legacy scene migration and the deformation work recorded above remain outstanding.


### Shadow Atlas Allocation Follows Demand

A further audit of scene-independent GPU memory found that every view reserved
both a four-layer 4096 x 4096 D32 directional atlas and a 4096 x 4096 D32 local
atlas, even without any shadow-casting lights. Their uncompressed texture storage
alone totals 320 MiB per view, excluding backend allocation alignment and overhead.
The inspected user project at `D:/trmp/FeatureLab` has directional shadows disabled
in `Scenes/Main.xiiScene`; neither its source nor its configuration was changed.

Shadow allocation now runs after cascade setup and before graph callbacks import
or dereference the images. With no directional cascades, the array keeps its four
shader-compatible layers at 1 x 1. With no allocated local shadow faces, the local
atlas is also 1 x 1. The local decision uses allocated face statistics, since the
per-light descriptor array also contains lights without shadows. Active shadows
retain their existing 4096 resolution and configured local atlas size.

Matching textures are reused. Turning shadows off releases obsolete textures and
single-cascade views; graph imports and external shared owners retain only their
actual uses, while GAL handles deferred native destruction. Allocation and cascade
view failures return `XII_FAILURE` through Shadows stage construction rather than
asserting or dereferencing a null texture. The no-cascade execution path clears all
fallback array slices to reversed-Z far depth so shader reads are deterministic.

`ShadowMemoryTest.cpp` runs the resource-backed default pipeline and checks empty
views, non-shadowing lights, full allocation on demand, fallback reuse, and release
of both full atlases after shadows are disabled. It seeds fallback images with
nonzero depth before a second frame, then maps GPU readbacks of all five fallback
slices to verify that execution clears them to zero. A separate fixture rejects
allocation with the default device temporarily unavailable and subsequently builds
the normal pipeline successfully.

The two focused regressions pass with Vulkan synchronization validation in
`Workspace/editor-shadow-memory-final-tests.log`. Temporarily restoring the old
unconditional atlas sizes produces the expected allocation-size failure in
`Workspace/editor-shadow-memory-control-tests.log`; the command restores the fixed
source automatically. The final Debug build of Editor, its processor/player
consumers, GraphicsCoreTest, EditorFrameworkTest and the native engine-process
tests passes in `Workspace/editor-shadow-memory-restored-build.log`.


Final verification on October 13, 2026:

- All 67 native engine-process subtests pass with synchronization validation:
  `Workspace/editor-shadow-memory-native-final-tests.log`.
- All 21 GraphicsCore subtests pass: `Workspace/editor-shadow-memory-core-tests.log`.
- Both shadow-memory regressions pass with GPU-assisted and synchronization
  validation: `Workspace/editor-shadow-memory-gpuav-tests.log`. There are no
  Vulkan validation errors; instrumentation emits performance warnings.
- All twelve editor subtests pass: `Workspace/editor-shadow-memory-editor-tests.log`.
  The real engine process survives 160 viewport frames, including 96 size changes,
  captures the final 1280 x 720 output, and supports Play/close/reopen. Its engine
  logs contain no device-memory exhaustion, device-loss or Vulkan validation errors.
- The restored production translation unit was explicitly rebuilt after the
  negative-control run, before the final focused and full-suite runs.

These tests establish the removed unconditional storage cost and preserved active
shadow allocation, lifecycle and execution. They do not measure total renderer
residency in the user's RenderDoc capture or reproduce that exact live-project
crash. Physical GPU exhaustion, Release, other backends and operating systems were
not exercised. Other rendering resources retain their existing quality and budgets.


### Document Save Validation And Missing-Type Compatibility

The document loader can reconstruct absent plugin types from embedded phantom RTTI
schemas. Truly unknown objects without usable schemas are different: the converter
skips them and records their type names and instance count. The document retains no
editable object for those instances, so serializing its live tree would drop their
original payloads from disk. Previously, programmatic and forced saves could do
exactly that; successful completion then erased the unknown-type diagnostics.
The document-window and modified-documents dialog also returned success when a
user declined the warning about permanently dropping these objects.

`xiiDocument` now validates every actual synchronous or asynchronous save before
serialization, asset-metadata preparation or scene-layer switching. Read-only
and partially loaded documents return a failure. Force bypasses only the dirty
check. A rejected asynchronous request invokes its valid callback immediately,
returns an invalid task ID, and leaves any already active save untouched. It does
not overwrite the pending save result or clear the missing-type diagnostics.
The header documents rejection and no-work callback semantics.

The two Qt save paths rely on this one validation result instead of asking whether
to discard unknown data. Save-before-close reports the existing save error once
and refuses closure. Explicitly choosing the dialog's discard action remains a
separate user decision. The asset-curator resave and scene-export checks remain
valid domain-specific guards; this change closes direct callers' bypass.

Three ToolsFoundation persistence regressions use real document-manager ownership,
command transactions and serialized files. They cover sync/async and forced saves,
modified and unmodified documents, unchanged disk bytes, dirty state, unknown-type
diagnostics, absence of save events/hooks, immediate rejection callbacks, rejection
while an earlier valid save is active, read-only recovery, and reopening saved
properties. A separate roundtrip unregisters an embedded phantom type before
loading, verifies reconstruction of its schema and payload, edits it, saves it,
unregisters it again and verifies the edited property after reopening. Thus absent
native plugin code alone does not incorrectly forbid safely representable data.

The baseline run, `Workspace/editor-document-save-baseline-tests.log`, reproduces
real writes that remove skipped data and incorrectly allow read-only saves. After
the fix, all 23 ToolsFoundation subtests pass in
`Workspace/editor-document-save-all-tools-tests.log`.

The native Qt fixture creates a real docked document window and drives Save,
Save-before-close, and the modified-documents dialog. Initial execution exposed a
test build mismatch: `EditorFrameworkTest` compiled its derived widget without
C++ RTTI, while Qt Debug callbacks in GuiFoundation dynamically cast receivers.
The local stack trace resolves to Qt's receiver type assertion during a status-bar
signal. The test target now uses `xii_link_target_qt`, the same existing helper used
by production Qt modules, to obtain the required Debug RTTI settings. Temporary
Windows fault tracing was removed from the source before verification.


Final verification on October 13, 2026:

- ToolsFoundationTest, EditorFrameworkTest and Editor build successfully, including
  the editor plugins, engine process, processor and player dependencies:
  `Workspace/editor-document-save-integration-build.log`.
- The Qt test target rebuilds with the standard Debug RTTI settings:
  `Workspace/editor-document-save-qt-build.log` and
  `Workspace/editor-document-save-final-test-build.log`.
- All 23 ToolsFoundation subtests pass:
  `Workspace/editor-document-save-all-tools-tests.log`.
- All twelve editor subtests pass:
  `Workspace/editor-document-save-editor-final-tests.log`. The new native fixture
  verifies unchanged source bytes and dirty state after direct Save, Save-before-
  close and modified-documents save rejection; one error per failed operation;
  refused closure; explicit discard without writing; and successful save after
  clearing the simulated missing-type condition. The existing viewport resize,
  Play, close/reopen, asset processor and profile workflows still pass.
- `git diff --check` passes. No Qt dependency, exception header, termination call,
  compatibility alias or new serialization format was introduced into runtime.

The save guard preserves the original file; it does not reconstruct objects for
which no usable type schema exists. Users must restore type support and reload or
perform a faithful migration before writing those documents. The native UI fixture
simulates that missing-type state on a real asset document; the ToolsFoundation
fixture separately exercises actual skipped-object loading from DDL. Legacy scene
component migrations and the broader modernization objective remain incomplete.
Release, Linux and macOS configurations were not executed for this slice.


### Asynchronous Document Save Revisions And Completion Ownership

Accepted asynchronous saves had a separate dirty-state bug. Completion always
called `SetModified(false)`, even if a later transaction, undo or redo had changed
the document since its snapshot was serialized. The next normal Save could then
be a no-op and leave those newer edits absent from disk. Saved-event listeners'
edits were also cleared after notification. In addition, every completion
invalidated the document's current save handle, so finishing an older request
could hide a later queued save from close-time waiting.

Each document now advances a modification revision on every `SetModified(true)`,
including calls while it is already dirty. Save construction captures that revision
alongside its graphs. Successful completion clears dirty state only when the
captured revision is still current, before publishing DocumentSaved. A listener's
subsequent edit remains dirty. The existing event facility permits one nested
ModifiedChanged notification for this supported listener behavior; deeper recursive
notification remains guarded.

Each write task owns its path and result. The worker no longer reads or writes the
live document, and the completion task owns the finished write task until its result
has been consumed. The old shared `m_LastSaveResult` was removed. Failed-write
messages describe that request's captured path, even when the document path changes
before the worker runs. Existing dependencies still serialize queued saves.

Completion only clears the active handle when it still identifies that task, and
does so before invoking the callback. It does not access the document after the
callback. Close-time waiting drains successor saves queued by completion callbacks.
Synchronous Save waits on its own copied task ID, leaves newer handles intact, and
revalidates writability after waiting for an earlier callback. Resource documents
continue to use their existing metadata preparation and post-save hooks.

Document persistence fixtures were shared in `DocumentSaveTestUtils.h`, with the
new temporal and queue tests in `DocumentAsyncSaveTest.cpp`. The baseline at
`Workspace/editor-async-save-baseline-tests.log` reproduces stale dirty clearing,
lost close-time tracking, and saved-listener edits missing from subsequent writes.
The tests drain outstanding tasks explicitly on the broken baseline so failures
can be observed without destroying a document still referenced by its tasks.

The regressions use real graph serialization and command transactions, checking
captured and newer values separately on disk. They cover edit/undo/redo after an
async snapshot; document-local and global saved-event listeners; nested dirty
notifications; queued saves; a three-save callback chain drained during closing;
independent failure/success results and captured-path reporting; and writability
changes made by an earlier completion callback. The native asset-window fixture
also checks newer unsaved text after an accepted async save, Save-before-close of
that newer content, and manager-owned document/window destruction while a final
save is pending.


### Asset Removal Uses Document Identity

Repeated native project creation exposed another lifecycle error during integration
verification. An asset can be replaced with a new document UUID at the same path.
The curator correctly marks the old asset identity removed, but MainThreadTick
previously closed whichever open document occupied that path. During replacement
creation this was the new document, whose initial save completion was currently
running. Closing before its DocumentOpened event caused visualizer subscription
removal assertions, followed by waiting on the save task executing that very hook.

The retained assertion stack in
`Workspace/editor-async-save-startup-symbols.log` identifies the chain:
VisualizerManager unsubscribe, DocumentManager CloseDocument, curator file-removal
handling, AssetDocument InternalAfterSaveDocument, save completion, and synchronous
creation of the default scene. The reused-project failure is recorded in
`Workspace/editor-async-save-editor-retry-tests.log`; the stalled owned test was
stopped after diagnosis. The earlier fresh-output run reached the native save
regressions but timed out at the scene viewport's first picking reply, recorded in
`Workspace/editor-async-save-editor-final-tests.log`. Neither run is reported as
successful verification.

The curator now uses the removed asset GUID to find and close its matching document.
A replacement at the old path remains registered and usable. Existing main/subasset
removal and notification behavior is retained; no parallel identity or ownership
system was added. The temporary assertion tracing was removed before the final
build.

`AssetReplacementTest.cpp` exercises the actual registered asset manager and curator:
create an original asset, register its identity, close it, create a replacement at
the same path, verify removal of only the old identity, edit and save the replacement,
close/reopen it with the new identity and persisted property, then delete its file
and verify that the matching open document is closed exactly once. The same reused
project that reproduced the default-scene self-close is used for final verification.

Replacement also untracks the old identity's dependencies before removing it.
Removed records cannot be reintroduced into transform-state queues by invalidation
or an in-flight update; final removal purges those queues again. The regression
checks that a package dependency no longer reports the replaced identity as a user.

Checkpoint verification: all 28 ToolsFoundation subtests passed in
`Workspace/editor-async-save-all-tools-final-tests.log` (exit 0). The Debug Editor,
EditorFrameworkTest, plugins, engine process, processor, and player build passed in
`Workspace/editor-asset-wrap-build.log` (exit 0). The preceding native integration
run completed viewport resizing/capture and the replacement/save checks but exposed
stale dependency and transform-state records, prompting the additional cleanup.
The latest reused-project integration run (`Workspace/editor-asset-wrap-tests.log`)
stalled after engine reconnection, before the save fixture, and was interrupted.
Full native integration is therefore not claimed green for this checkpoint.
