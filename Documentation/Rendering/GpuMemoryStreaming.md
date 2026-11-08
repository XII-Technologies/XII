# GPU memory and streaming

The Vulkan backend reports each heap's capacity, process budget and usage, allocator block bytes, and occupied suballocations. The native budget can change when other applications use the device. If the extension is unavailable, VMA estimates the budget at 80 percent of heap capacity and reports its committed blocks as usage. Device-local and host-visible describe heap properties. A heap can have both properties, including on integrated hardware.

## Reading the counters

- **Usage bytes** includes driver-reported allocations outside VMA where native reporting is available.
- **Committed bytes** counts Vulkan memory blocks VMA owns. Several buffers and images can share one block.
- **Occupied bytes** counts VMA suballocations. This includes resources pending GPU retirement. Committed bytes can stay high after occupied bytes fall because VMA can reuse a block.
- **Occupied arena bytes** counts geometry metadata ranges assigned to active records. Geometry eviction makes these ranges reusable.
- **Resident topology bytes** counts the padded remap and primitive-index payload once per live meshlet range within each mesh revision. **Retired topology bytes** counts payloads passed to deferred retirement during the frame. The Vulkan occupied count includes them until the bindless views and their buffers are released.

Profiling builds publish the default Vulkan device's heap counters under **Renderer / GPU memory** in Inspector. Geometry arena occupancy and reclamation appear under **Renderer / Geometry**. When checking scene unload, sample after the relevant queue fences have completed. Compare both occupied and committed bytes. An unchanged occupied count points to a remaining allocation owner, in-flight work, or another workload. A drop in occupied bytes with unchanged committed bytes indicates space available within existing allocator blocks, subject to alignment and fragmentation.

## Streaming controls

| CVar | Default | Effect |
| --- | ---: | --- |
| `Streaming.GpuBudgetMiB` | 0 | Caps optional admission against the primary device-local heap. Zero derives the limit from the native or estimated budget. |
| `Streaming.GpuHeadroomMiB` | 256 | Reserves space below the native budget for required work. |
| `Streaming.GpuPressureHighPercent` | 95 | Enters pressure at this fraction of the usable budget. |
| `Streaming.GpuPressureLowPercent` | 85 | Resumes optional streaming below this fraction. |
| `Streaming.TextureUploadMiB` | 64 | Limits admitted texture refinement payloads per device frame. Zero pauses them. |

Geometry, render targets, materials, acceleration structures, and other Vulkan allocations contribute to the reported heap usage. Optional texture refinements and optional geometry topology upgrades reserve bytes against the same device-local admission limit. Each keeps its own transfer allowance. Texture reservations cover the complete replacement while the current image remains live. Vulkan 1.3 supplies image memory requirements before creation. Older Vulkan devices use the mip payload size for this preflight. VMA also checks the selected heap's budget if it needs a new memory block. Under pressure the renderer trims completed idle graph allocations sooner and periodically runs an incremental sweep of unused GPU-backed resources.

Texture resources continue sampling the coarse image when refinement is deferred. Successful promotion releases the resource's coarse reference, and previous bindless bindings and submitted work retain the old image until safe retirement. Texture creation still uploads a complete replacement image. Canonical mesh vertex and index buffers and other required GPU resources still allocate independently. Initial topology residency, pinned topology, and topology needed by an exact forced draw use required allocation. Hierarchical geometry pages, independent mip and tile transfers, and subsystem memory attribution remain separate work.

## Geometry topology residency

The geometry manager packs each admitted LOD’s meshlet remaps and primitive indices into one GPU allocation and one bindless descriptor. It validates the range against the compiled CPU streams and uploads the smallest covering span from each stream. The geometry record identifies the byte where primitive indices start. Meshlet offsets are relative to their respective spans. Primitive identities and the compiled asset data retain their original values.

Identical ranges within a mesh revision share topology bindings and transfer charges. Distinct ranges share the canonical vertex and index bindings. Overlapping ranges can duplicate topology bytes, and a range with gaps retains the bytes between its first and last reference. The metadata graph waits on the graphics queue signal before exposing new bindings to its consumers. Vulkan checks the packed buffer’s native allocation requirements at admission, so required memory allocated after the frame’s pressure sample can defer an upgrade. Once a fallback is resident, optional LODs commit individually. A deferral keeps the current LOD mask and returns any tentative metadata arena range.

Eviction and unregister release a record's leases after its last referenced frame completes. A range retires its descriptors and buffers when its final lease ends, even if another range of the same mesh stays resident. The mesh resource retains its canonical vertex and index buffers for debug rendering, ray tracing, and export. A later request can upload the topology again from the same loaded mesh.

While the device reports pressure, the manager checks every eight frames for unpinned records idle for at least 120 frames. It shares a limit of four retirements per check between whole records and fine LODs. For active records it inspects up to 128 records in rotation. A fine LOD can leave an active record after 120 frames without demand when a coarser resident LOD has recent demand. Requests from different views contribute to the same history. Recent feedback overflow or dropped readbacks suspend active-record trimming.

Visibility reports resident demand for streamable meshes with multiple LODs. Ordinary instances use the CPU preference when recent GPU demand is unavailable. A trimmed LOD disappears from new GPU metadata immediately, and its arena range and topology lease stay occupied until older frames complete. Delayed feedback from before the trim cannot re-admit it. Renewed optional fine demand waits under pressure while a coarse LOD is available. Forced draws can request their exact authored LOD under pressure.
