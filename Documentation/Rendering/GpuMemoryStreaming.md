# GPU memory and streaming

The Vulkan backend reports each heap's capacity, process budget and usage, allocator block bytes, and occupied suballocations. The native budget can change when other applications use the device. If the extension is unavailable, VMA estimates the budget at 80 percent of heap capacity and reports its committed blocks as usage. Device-local and host-visible describe heap properties. A heap can have both properties, including on integrated hardware.

## Reading the counters

- **Usage bytes** includes driver-reported allocations outside VMA where native reporting is available.
- **Committed bytes** counts Vulkan memory blocks VMA owns. Several buffers and images can share one block.
- **Occupied bytes** counts VMA suballocations. This includes resources pending GPU retirement. Committed bytes can stay high after occupied bytes fall because VMA can reuse a block.
- **Occupied arena bytes** counts geometry metadata ranges assigned to active records. Geometry eviction makes these ranges reusable.
- **Resident topology bytes** counts the padded remap and primitive-index payload once per live mesh revision. **Retired topology bytes** counts payloads passed to deferred retirement during the frame. The Vulkan occupied count includes them until the bindless views and their buffers are released.

Profiling builds publish the default Vulkan device's heap counters under **Renderer / GPU memory** in Inspector. Geometry arena occupancy and reclamation appear under **Renderer / Geometry**. When checking scene unload, sample after the relevant queue fences have completed. Compare both occupied and committed bytes. An unchanged occupied count points to a remaining allocation owner, in-flight work, or another workload. A drop in occupied bytes with unchanged committed bytes indicates space available within existing allocator blocks, subject to alignment and fragmentation.

## Streaming controls

| CVar | Default | Effect |
| --- | ---: | --- |
| `Streaming.GpuBudgetMiB` | 0 | Caps optional admission against the primary device-local heap. Zero derives the limit from the native or estimated budget. |
| `Streaming.GpuHeadroomMiB` | 256 | Reserves space below the native budget for required work. |
| `Streaming.GpuPressureHighPercent` | 95 | Enters pressure at this fraction of the usable budget. |
| `Streaming.GpuPressureLowPercent` | 85 | Resumes optional streaming below this fraction. |
| `Streaming.TextureUploadMiB` | 64 | Limits admitted texture refinement payloads per device frame. Zero pauses them. |

Geometry, render targets, materials, acceleration structures, and other Vulkan allocations contribute to the reported heap usage. Optional texture refinements are admitted against that usage and the separate transfer allowance. The reservation covers the complete replacement while the current image remains live. Vulkan 1.3 supplies image memory requirements before creation. Older Vulkan devices use the mip payload size for this preflight. VMA also checks the selected heap's budget if it needs a new memory block. Under pressure the renderer trims completed idle graph allocations sooner and periodically runs an incremental sweep of unused GPU-backed resources.

Texture resources continue sampling the coarse image when refinement is deferred. Successful promotion releases the resource's coarse reference, and previous bindless bindings and submitted work retain the old image until safe retirement. Texture creation still uploads a complete replacement image. Canonical mesh vertex and index buffers and other required GPU resources still allocate independently, so the device admission cap applies to optional texture refinements. Hierarchical geometry pages, independent mip and tile transfers, and subsystem memory attribution remain separate work.

## Geometry topology residency

The geometry manager creates GPU remap and primitive-index buffers on first admission, using the compiled CPU topology retained by the mesh resource. Records and authored LODs for the same mesh revision share their buffer bindings. A new upload consumes the geometry transfer allowance once. The metadata graph waits on the graphics queue signal before exposing the new bindings to its consumers.

Eviction and unregister release a record's lease after its last referenced frame completes. The final lease retires the shared descriptors and their backing buffers through the existing bindless and GAL deletion queues. The mesh resource retains its canonical vertex and index buffers for debug rendering, ray tracing, and export. A later request can upload the topology again from the same loaded mesh.

While the device reports pressure, the manager checks every eight frames for unpinned records idle for at least 120 frames. It retires up to four completed records per check. A fine LOD that needs fresh topology waits under pressure if a coarser LOD is already resident. Sharing and reclamation are at mesh-revision granularity.
