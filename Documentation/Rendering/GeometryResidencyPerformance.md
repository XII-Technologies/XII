# Geometry residency and large-scene synchronization

Geometry-dependent preparation belongs to a resource revision or an admitted residency transaction. Routine extraction updates retain that preparation and update instance state independently. The streaming allowance belongs to the shared device frame, including calls made before and after world synchronization.

## Findings and execution path

`xiiRenderWorldModule::ExecuteRenderGraphs()` collects extracted packets from prepared views and calls `xiiGpuInstanceData::UpdateFromExtraction()`. Whole-mesh packets become draw candidates for authored LOD sections. These candidates share generation-checked geometry records through `AcquireMeshGeometry()`. Residency runs both in `xiiRenderGraphManager::BeginFrame()` and after world synchronization, allowing newly discovered geometry to enter the same frame. Graph upload passes publish geometry and meshlet metadata before visibility and draw generation consume it.

Source inspection identified the following unnecessary work:

- Primitive-range coverage scanned every meshlet for every packed section on every update, including instances sharing unchanged geometry.
- Geometry preparation allocated arena ranges and copied metadata before checking the remaining allowance. Both streaming entry points received independent allowances.
- Streaming reconstructed its queue by scanning the configured slot capacity. Upload setup also scanned unused capacity and copied pending meshlet payloads for each graph. Pending-transfer retirement used nested searches.
- Instance packing reacquired geometry references, resolved repeated material flags, and recomputed normal matrices and bounds even when their inputs were unchanged.
- Native mesh-resource creation rebuilt meshlets even when the compiled descriptor already contained them. Loading a compiled asset therefore repeated clustering and optimization.
- GPU culling visited unrelated sections' meshlets, launched a rectangular grid based on the largest candidate, and reserved every visible meshlet with a separate global compare/exchange loop. GPU timestamps on Emerald Square confirmed meshlet culling remained a dominant workload at the CPU-refactor checkpoint: two culling operations averaged 159.043 ms and 166.136 ms. Bounded dispatch and range indexing alone did not improve those timings; group-level reservation batching then reduced them to 12.470 ms and 38.182 ms. This isolates global reservation contention as a material part of the observed GPU cost.

The CPU timings below establish the coverage cost directly. They do not establish a complete historical editor FPS baseline. Previously nonresident geometry becoming visible also creates legitimate raster, picking, shadow, and lighting work; this is separate from repeated preparation and empty dispatch work.

## Implementation and ownership

### Immutable geometry and dynamic instances

Each geometry slot lazily owns an interval index per authored LOD. Sorted primitive starts and ends give the exact half-open overlap count through two binary searches, including boundary meshlets, overlapping ranges, and unordered input. Zero primitive count still means the complete LOD. Source-buffer revisions invalidate raw-geometry indices. Native mesh revisions receive new geometry handles; old references retain their exact revision until retirement. Slot retirement clears the index before a generation can be reused.

Instance entries retain the geometry lease, source revisions, primitive-range coverage, and local bounds. Coverage changes only with geometry or range identity. Transform changes trigger the normal-matrix calculation; transform or local-bounds changes trigger world-bounds propagation. Material flags are resolved once per unique material per extraction, so hot reload remains observable. Scratch membership sets retain capacity. Pose, morph, previous-transform, instance topology, and view-selection updates retain their existing dynamic behavior.

Compiled resources validate supplied meshlet bounds, source ranges, remaps, local indices, LODs, and sections instead of reclustering them. Descriptors without clusters retain the builder path. Buffer-level procedural clustering may precede section/LOD authoring: after payload validation, nonempty sections or LODs with missing draw ranges use the existing descriptor builder before publication. Fully prepared compiled descriptors retain their clusters, including authored bounds. Explicitly empty LODs do not force a rebuild. Procedural callers changing raw data must explicitly rebuild clusters. Asset serialization and GPU record sizes are unchanged.

### Admission, commit, and scheduling

Admission holds source-resource locks, resolves missing LODs, checks revisions and ranges, and calculates logical residency bytes and arena requirements. Budget and capacity deferrals do not allocate metadata payloads or mutate the arena. First-fit planning uses at most eight local reservations. Commit allocates admitted ranges, prepares payloads, registers bindless resources, and publishes the complete record. Genuine failures roll back allocations and registrations.

The first streaming call in a device frame establishes its allowance. Later calls share its remainder. An unused positive allowance permits one indivisible oversized admission; the remaining allowance saturates at zero, while admitted bytes retain the complete cost. Zero pauses admissions for the entire frame. Logical admitted bytes retain the existing vertex/index/metadata accounting; actual metadata transfer bytes are reported separately.

Active, pending, and retiring slots have persistent lists. Only pending requests are sorted. Authored priority, visibility priority, bounded request aging, recency, and deterministic slot tie-breaking remain intact. Preparation stops when the allowance is exhausted. Results distinguish budget, readiness, arena, and soft-memory deferrals from failures. Arena planning avoids allocation/rollback cycles for records that cannot fit.

Memory pressure and arena pressure share a frame-local list of completed cold eviction candidates. Pinned assets, current-frame use, and current-frame admissions are protected. The candidate list is refreshed when the completed-frame frontier advances. A request exceeding total arena capacity is rejected before evicting unrelated assets.

### Uploads and GPU work

Graphs share reference-counted pending payloads rather than copying their contents. Graph setup never consumes a transfer. Recording stages each payload once, releases its CPU data, and marks shared completion state. Abandoned or failed compilation leaves the original payload available. The executor submits a valid recorded prefix on subsequent recording failure. Retirement uses linear compaction. Canceled transfers and retained subsystem epochs prevent stale callbacks from resurrecting released allocations. Dirty frame slices clear only when the recorded snapshot still matches the CPU record; delayed older snapshots cannot overwrite newer publication.

Adjacent dirty geometry records are transferred in contiguous batches. Clean records do not generate updates. Meshlet payloads remain separate transfers because arena fragmentation and independent cancellation make unconditional concatenation inappropriate.

Residency sorts an unordered admitted arena copy by primitive start and stores prefix-maximum primitive ends in existing nonserialized GPU padding. Source buffers, section identities, primitive/remap offsets, and authored LOD spans remain intact. Visibility bounds, meshlet culling, and emissive extraction binary-search a conservative candidate span, including nested overlapping intervals; exact primitive tests still determine contribution. Externally supplied metadata without this index retains full-range fallback behavior. Meshlet culling uses a bounded scheduling width and grid-stride iteration, visiting every relevant meshlet without launching the largest section's complete grid for every candidate. Each uniform tile compacts visible lanes in group memory and reserves one contiguous global output range. Partial grants saturate the count at capacity and report overflow. This uses ordinary compute-group synchronization without requiring optional wave operations. Dispatch arguments are explicitly synchronized for both indirect and shader reads. Capacity validation, frustum/cone tests, and overflow reporting remain enabled.

## Complexity

Let `M` be meshlets, `S` section draw entries, `A` active geometry records, `Q` pending requests, `D` dirty records, and `U` pending transfers.

| Operation | Previous cost | Revised cost |
| --- | --- | --- |
| Primitive coverage across updates | `O(S * M)` each update | `O(M log M)` once per LOD revision, `O(log M)` per changed range; unchanged entries reuse counts |
| Stationary instance synchronization | Coverage scans, reference churn, matrices and bounds | `O(S)` membership/state work without meshlet traversal or unchanged matrix/bounds calculation |
| Streaming queue | Scan configured slot capacity and sort requests | Maintain pending list; `O(Q log Q)` sorting while allowance remains |
| Metadata setup | Scan configured capacity and copy pending payload bytes per graph | `O(A + D log D + U)` with shared payload ownership |
| Transfer retirement | Nested pending/snapshot searches | `O(U)` compaction |
| Indexed GPU section coverage | Complete LOD traversal per candidate | Binary-search conservative span plus exact per-meshlet overlap tests |
| Meshlet launch grid | Largest range multiplied by candidates | Bounded scheduling groups per candidate with complete grid-stride traversal |
| Visible-meshlet reservation | One contended global CAS loop per visible meshlet | One global reservation per nonempty 64-lane tile |

The pending list remains sorted on each eligible invocation. A more complicated heap with continuously changing visibility/age priorities was not introduced. Fragmented arena planning remains proportional to the free-range count with a fixed eight-LOD bound.

## Measurements and reproduction

Measurements use Windows/MSVC Debug and Vulkan on an Intel Core i7-12700H system with an NVIDIA GeForce RTX 3070 Ti Laptop GPU. They are individual diagnostic runs, sensitive to clocks, memory pressure, cache state, and Debug instrumentation.

| CPU workload | Result |
| --- | --- |
| Synthetic coverage: 16,384 meshlets, 512 ranges, 16 updates | 185.683 ms before indexing; 5.611 ms after indexing, including preparation |
| 1,024 unchanged world components, 20 updates | 30.249 ms total, 1.512 ms/update |
| Compiled Emerald Square Day | 12,107,607 vertices, 10,046,405 triangles, 197,456 meshlets, 2,479 sections |
| Emerald Square, 128 sampled ranges | Scan oracle 310.175 ms; indexed queries including preparation 25.439 ms; identical overlap checksum |
| Emerald Square GPU scene, 2,479 draw entries | Cold packing 21.802 ms; stationary 7.155 ms/update over 20 updates; no additional coverage queries and no dirty instance ranges |

The compiled-asset CPU benchmark excludes texture/material IO, FBX import, and GPU rendering. The Editor preview benchmark uses the canonical Editor pipeline at 64 by 64 pixels with one test material across the original sections. It validates actual rendering through readback before and after deactivation and collects asynchronous GPU timestamp samples. It does not reproduce the full authored scene's texture, light, camera, shadow, or GI workload.

| GPU operation in Emerald Square preview | CPU-refactor checkpoint | Final range/grid/batch implementation |
| --- | --- | --- |
| Culling operation `{50b147ff-d428-4a6d-af70-87e9635bb443}` | 159.043 ms | 12.470 ms |
| Nested culling operation ending `{42a45e6c-497a-4098-a147-6bb5ac1d332f}` | 166.136 ms | 38.182 ms |

Each GPU value averages five available timestamp samples. These are pass durations from separate runs, not full-frame wall time or an original-PR baseline. The final settled streaming sample was approximately 0.003 ms with no records examined/admitted or meshlets prepared. Remaining culling time is material and warrants profiling in a full authored scene; residency completion does not eliminate the actual rendering workload.

Use focused test-order files with `-noGui -run -close -noSave -order <file> -outputDir <directory>`:

- `GraphicsFoundationTest`: `PipelineResources/GeometryCoverageScaling`, `GeometryAdmissionAndUploadOwnership`, `CompiledMeshletPreparation`, `OversizedGeometryResidency`; `PipelineExtraction/WorldGpuInstances`; `NativeGeometryFeedback/AuthoredAsynchronousResidency`.
- `GraphicsFoundationTest`: `PipelineResources/CompiledGeometryScaling -geometryModel <absolute .xiiBinMesh>` for the compiled-asset CPU benchmark.
- `GraphicsFoundationTest`: `NativeVisibility/InstanceCullingAndExplicitHistory`, `AuthoredWorldVisibilityInstances`, `AuthoredDepthOnlyInstances`, `AuthoredGBufferInstances`.
- `EditorEngineProcessFrameworkTest`: `EditorViews/MeshPreviewEditorPipeline -compiledMesh <absolute .xiiBinMesh>` for geometry-focused GPU timings; `-importModel <source model>` retains the import path.
- `EditorEngineProcessFrameworkTest`: `NativeSceneLod/Sections`, `SectionsWithoutMeshShaders`, `Reload`; `SkinningRaster/ReloadAndContextIsolation`.

## Diagnostics and validation boundaries

Existing CPU profiling scopes identify geometry coverage, streaming, admission/commit, compiled validation, resource preparation, and GPU scene synchronization. `Renderer/Geometry/*` statistics expose streaming time, examined/admitted/deferred/failed attempts, logical admitted bytes, actual metadata upload bytes, prepared meshlets, arena occupancy, resident/pending records, memory, and coverage queries/index hits/builds. `Renderer/GPU scene/*` exposes synchronization time and instance count. Statistic publication is compiled out when profiling is disabled. Coverage counters are cumulative; streaming counters accumulate across invocations of one device frame. Use the existing rendering profiler for individual GPU passes and queue timelines; overlapping queue durations must not be summed into frame wall time.

Focused regressions cover shared frame allowances, unused-positive oversized admission, zero-budget pause, partial allowance, no preparation on deferral, out-of-order and repeated graphs, delayed pre-admission snapshots, completed-frame retirement, cold arena eviction, pinned/current-frame protection, unordered primitive ranges across authored LODs and handle generations, compiled metadata reuse and malformed remaps, unchanged instance uploads, authored sections/LODs, hot reload, skinning, culling completeness across more than two grid strides, and explicit overflow.

Full Emerald Square editor navigation, authored materials/lights, repeated interactive scene loading/unloading, Shipping performance, other graphics backends, and other operating systems remain acceptance work. The asset was available and exercised as compiled geometry; no full authored-scene FPS claim is made. Larger follow-up work should measure compact per-candidate GPU task streams, incremental emissive-light extraction, diagnostic-mode reservation batching, and transient capacity sizing against those workloads before adding infrastructure. Emissive extraction still launches its existing conservative global rectangular grid, although its range lookup skips unrelated metadata. Highly overlapping intervals can widen conservative GPU spans; the legacy logical byte accounting can count shared source buffers per LOD.
