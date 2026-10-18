# Rendering refactor handoff - 18 October 2026

The active goal was closed at the user's request after the atmosphere lookup pass
refactor. **The complete rendering-stack migration remains unfinished.** This is
the single future-work handoff; detailed implementation history and previous
verification are in [NativePassInventory.md](NativePassInventory.md).

## Current boundary

There are 102 concrete reflected operation classes under
`Source/Engine/GraphicsCore/Pipeline/Passes` and 90 editable pipeline asset/binary
pairs under `Data/Base/RenderPipelines`. These are real operations and compositions,
not evidence that every rendering technique has been converted. Pins are concrete
members exposed with `XII_MEMBER_PROPERTY`; properties use the existing reflection,
attributes, document commands and serialization infrastructure.

The final phase adds independent `AtmosphereTransmittancePass` and
`AtmosphereMultiScatterPass`, plus AtmosphereConstants, AtmosphereTransmittance,
AtmosphereMultiScatter, AtmosphereLookup and WorldAtmosphereView resources.
`xiiAtmosphereManager`, its unused LUT registry/handles, global default GPU
allocations and revision/commit API are deleted. `xiiAtmosphereSettings`, validation
and its narrowly targeted version-3-to-4 asset repair remain. Sky atmosphere
components are authoritative; each pass consumes an immutable extraction snapshot
or its authored fallback settings. The lookup assets own graph-derived images.

## Highest-priority remaining work

1. **Finish resource-only application entry points.**
   `Source/Samples/GpuDrivenScene/GpuDrivenScene.cpp`, especially
   `BuildRenderGraph`, still registers a custom C++ graph and coordinates multiple
   pipeline fragments. Convert its supported visibility, shadow/VSM, depth,
   directional, query and output wiring into authored top-level resources. Preserve
   existing GPU behavior and overflow/readback policies while deleting the custom
   orchestration. Do not replace it with a giant reflected pass.

2. **Supply and select complete supported scene graphs.**
   `Source/Engine/GraphicsCore/Pipeline/RenderPipelineProfileConfig.h` and its
   implementation intentionally have empty Game, EditorScene and AssetPreview
   references after old defaults were removed. Rebuild supported full scene assets,
   configure explicit project/profile references, and exercise game, editor scene,
   asset preview and scene-camera preview consumers. `DeferredOpaque` is a supported
   geometry/lighting composition, not a complete replacement for every old scene
   pipeline. Missing/invalid references must keep producing diagnostics; do not add
   hidden fallback construction or restore deleted Deferred/EditorScene assets.

3. **Complete real algorithm decomposition.**
   The responsibility table in NativePassInventory is the starting audit. Remaining
   work includes shadow atlas/raster/filtering and ray shadows; geometry-dependent
   DDGI/sparse-voxel updates; ReSTIR direct/indirect reuse and denoising;
   reflection-probe preparation/selection and ray reflections; fog/volume stages;
   decal preparation/rendering; transparent/particle raster; camera/jitter history
   and velocity dilation. Split meaningful multi-operation boundaries still inside
   existing passes, including `Passes/Output/BloomPass.cpp`. Do not count deleted
   operations as converted or create artificial wrappers to reach hundreds of nodes.

4. **Finish environment integration and safe LUT content caching.**
   Atmosphere lookup outputs currently regenerate whenever the corresponding graph
   operations execute. Existing graph resource caching reuses physical allocations;
   it does not cache LUT contents by atmosphere parameters. Implement any content
   cache against explicit parameter revisions/content identities, with completion
   tracking, per-instance ownership and safe retirement. Graph handles alone are
   not content identities. Preserve the current shader's approximate multiple
   scattering model; improve it separately with numerical/physical validation.
   Sky irradiance convolution, environment/probe resources and full scene consumers
   remain open. Do not recreate a global atmosphere object registry.

5. **Validate the complete editor workflow.**
   Automated editor tests cover reflected node/pin inspection, attributes, property
   edits, undo/redo, serialization, nested overrides and deterministic library
   export. This does not establish a complete interactive Windows editor workflow.
   Exercise new-project profile selection, graph creation, nesting/referencing,
   pin selection, property editing, save/reload, asset transformation, hot reload
   and live scene rendering. Relevant code is under
   `Source/EditorPlugins/Assets/EditorPluginAssets/RenderPipelineAsset`,
   `Source/Tools/Libs/GuiFoundation/NodeEditor` and the existing property grid.

6. **Measure and optimize compiled execution.**
   Many reflected operations need not imply many GPU submissions. Keep dependencies
   and queue selection authoritative; investigate compatible scheduling/batching
   without changing semantics. A previous DeferredOpaque fixture had 37 operations
   and 12 submissions with its async policy; it is not a controlled benchmark.
   Measure CPU compilation/extraction, GPU uploads, queue transitions, memory and
   frame latency on representative worlds/views before claiming a speedup. LUT
   regeneration has no measured performance comparison with the deleted unused
   cache. Default RGBA32Float LUTs use 272 KiB per live occurrence before allocator
   overhead, twice the former unused RGBA16Float pair; assess precision/footprint
   with representative scenes. D3D12 remains explicitly outside this task's
   verification scope.

## Invariants to preserve

- `xiiWorld`, objects and components own scene state, transforms and lifetimes.
  GPU data remains derived; do not add a scene database, mirror hierarchy or public
  GPU-table scene-manipulation API.
- Core graph composition stays domain-neutral. Graph-owned handles are not
  persistent serialized identities; preserve generation checks, remapping,
  provenance, mutation complexity and documented ownership semantics.
- Passes are independent units with concrete reflected members. No stage checks,
  Builtin rendering files, manual pin-type tables or compatibility runtimes.
- Use ordinary resources and reflected graph assets. Definition sharing must not
  share writable view/instance state. Temporal dependencies remain explicit.
- Aggregate demand across active views. Unused features must not initialize mesh,
  material, lighting or other services; execution must not access mutable component
  storage. Keep completion-safe resources across frames in flight.
- Classify presentation, runtime parameters, compile-time properties, topology and
  definition changes before invalidating compiled plans or allocations.

## Verification at handoff

All five affected Debug targets build: GraphicsCoreTest, GraphicsFoundationTest,
EditorFrameworkTest, EditorEngineProcessFrameworkTest and GpuDrivenScene, with their
dependent engine/editor targets (`Workspace/native-atmosphere-final-build.log`).

The final sequential regression run passes all 77 GraphicsCore subtests, 39 native
Vulkan subtests, 14 PipelineResources, three PipelineExtraction, three shader
failure/recovery, one temporal-history, three temporal-memory, one native
depth-boundary, 16 EditorDocuments and 71 authoring subtests. The library test
compares packaged/exported/repeated bytes for all 90 asset pairs. Logs are
`Workspace/native-atmosphere-cpu-final.log` and
`Workspace/native-atmosphere-final-*.log`.

The new NativeAtmosphere test verifies numerical lookup readbacks, two worlds with
two independent views each, component mutation/removal, authored fallback and
recording execution after instance/world retirement. Atmosphere-only extraction
does not initialize mesh/material residency. Its fixture records 12 operations,
four submissions and 14 barriers per view. The initial focused fixture run failed
on an incorrect uniform/SRV sink contract; the corrected focused run and final
broad run pass. Exact history is retained in NativePassInventory.md.

GpuDrivenScene completes 300 Vulkan frames with exit code zero
(`Workspace/native-atmosphere-sample.log` and `native-atmosphere-sample-results.json`).
Its advanced custom graph still registers 124 operations, compiles 116, culls eight,
and emits one submission with 190 barriers. This does not validate its unfinished
resource-only entry-point migration. No controlled benchmark was run. Other
backends and interactive full-editor workflows are not claimed verified.

Source/Data searches find no production scene database or atmosphere manager and
no old LUT-handle/cache-statistics consumers. No rendering Builtin file was restored.
The broader completion items below remain open.

Reproduce Debug builds with the existing `Workspace/vs2026x64` configuration:

```powershell
cmake -S . -B Workspace/vs2026x64
cmake --build Workspace/vs2026x64 --config Debug --target GraphicsCoreTest GraphicsFoundationTest EditorFrameworkTest EditorEngineProcessFrameworkTest GpuDrivenScene -- /m:1 /p:CL_MPcount=1
```

Run binaries in `Output/Bin/WinVs2026Debug64` with `-nogui -close -all`.
`-filter` selects test groups; GPU runs here use `-renderer Vulkan`.
Authoring tests should receive a writable `-outputDir`; keep builds and test runs
sequential to avoid output/PDB contention. Workspace logs/scripts are local evidence
and are not tracked deliverables.

## Completion still requiring evidence

- [ ] All normal production entry points use explicit authored pipeline resources.
- [ ] No large application graph function dictates an entire rendering stack.
- [ ] Existing supported techniques have complete resource-authored replacements.
- [ ] Interactive editor/project/preview workflows work end to end.
- [ ] Representative performance and safe multi-frame resource lifetimes are measured.
- [ ] Available backends beyond the Vulkan coverage below are separately verified.

Do not treat closing this goal as satisfying these remaining architecture criteria.
