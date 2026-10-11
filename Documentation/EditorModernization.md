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
