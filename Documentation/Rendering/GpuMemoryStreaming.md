# GPU memory and streaming

The Vulkan device reports heap capacity, native process budget and usage, allocator block capacity, and occupied suballocations. The native budget can change as other applications use the GPU. Without `VK_EXT_memory_budget`, VMA estimates the budget at 80 percent of heap capacity and reports committed blocks as usage. Device-local and host-visible properties can overlap, including on unified-memory and BAR-visible heaps.

## Ownership and reclamation

Every VMA buffer and image allocation has one owner category, native allocation size, memory type, and copied debug name. Views and bindless registrations do not add an allocation charge. The charge survives resource removal, descriptor retirement, GAL object destruction, and deferred native destruction. Only the allocator's actual destroy operation removes it. Streaming admission uses this outstanding ownership rather than subtracting bytes when a page mapping is withdrawn.

Inspector publishes the default device's counters under **Renderer / GPU memory**:

- **Usage bytes** is native process usage, including driver allocations outside VMA where available.
- **Committed bytes** is Vulkan memory block capacity owned by VMA. Multiple resources can share a block.
- **Occupied bytes** is occupied VMA suballocation memory, including deferred destruction.
- **Unused allocator bytes** is unoccupied block capacity. Reuse still depends on alignment, memory type, and fragmentation.
- **Owners / category / Allocated bytes** counts native suballocation ownership once, including retirement.
- **Owners / category / Retiring bytes** identifies allocations queued for native destruction.
- **Owners / category / Allocation count** identifies owners retaining numerous small allocations.

Categories attribute geometry, geometry pages, textures, texture pages, streaming metadata, materials, acceleration structures, render graph resources, upload pools, readback resources, and other allocations. Host-visible ownership can overlap device-local ownership. Borrowed native objects and imported external memory are outside the VMA ownership ledger; native heap usage remains the device-wide pressure source.

Set `Streaming.DumpGpuAllocations` to dump outstanding allocation names, categories, sizes, memory types, and retirement state once. Resource names are copied and remain valid after their GAL owners have disappeared. Allocator shutdown diagnoses any remaining owners.

A scene unload must be evaluated after queue completion and retirement processing. Falling occupied bytes with unchanged committed bytes means allocator capacity became reusable. An unchanged occupied count points to live owners, outstanding work, or another workload. A residency-table counter alone does not demonstrate physical reclamation.

## Shared admission and scheduling

| CVar | Default | Effect |
| --- | ---: | --- |
| `Streaming.GpuBudgetMiB` | 0 | Caps optional admission on the primary device-local heap; zero uses the native or estimated budget. |
| `Streaming.GpuHeadroomMiB` | 256 | Reserves native budget headroom for required rendering work. |
| `Streaming.GpuPressureHighPercent` | 95 | Enters memory pressure at this fraction of the usable budget. |
| `Streaming.GpuPressureLowPercent` | 85 | Resumes optional streaming below this fraction. |
| `Streaming.GeometryCacheMiB` | 256 | Limits optional native geometry page allocations, including retiring backing. |
| `Streaming.TextureCacheMiB` | 512 | Limits optional native texture tile allocations, including retiring backing. |
| `Streaming.TextureUploadMiB` | 64 | Limits admitted optional texture upload payload per device frame. Zero pauses optional uploads. |

Cache limits are elastic admission ceilings, not preallocated device partitions. All allocations contribute to native device pressure. Optional geometry and texture pages reserve against the same native budget and retain independent transfer allowances. Vulkan 1.2 and newer query exact buffer and image requirements before optional creation. VMA also checks the selected heap budget when committing new blocks. Required coarse representations retain admission priority.

The shared scheduler deduplicates requests across views, orders priority and age deterministically, and bounds request examination, IO, ready payload memory, and eviction work. GPU feedback uses frame-indexed atomic request bitsets and asynchronous, fence-checked readbacks. Queue completion markers protect feedback capture without a device-idle wait or blocking readback. Physical geometry slots use 4-128 KiB size classes; physical texture slots are shared across assets of the same color space. Withdrawal queues a slot until its last-use frame completes. Its resource and descriptor then become reusable without allocation or descriptor churn. Free slots expire after 120 frames and release sooner under memory pressure or reduced cache ceilings. Allocation ownership remains charged during reuse and native retirement. Inspector separately reports backing, reusable, retiring, active payload, active padding, allocation, reuse, and release counters.

Cold pages expire; memory pressure can accelerate eviction after a minimum residence interval. Delayed feedback predating eviction cannot immediately resurrect the same page.

Render graph allocations participate in native pressure and ownership attribution. Completed idle graph allocations and unused GPU-backed resources are reclaimed sooner under pressure.

## Compiled geometry

Mesh document version 17 and mesh-buffer descriptor version 5 contain compiled meshlet hierarchies and independently compressed geometry pages. Animated mesh documents use version 11. Eligible immutable static triangle meshes allocate page payloads after admission, without full canonical GPU vertex, index, or cluster-metadata buffers. Deformable and mutable geometry retain their direct rendering representation. Unsupported older compiled page formats require asset transformation.

Pages contain compact vertices, remaps, primitive indices, and cluster records, targeting 64 KiB with a 128 KiB maximum including metadata. Spatial groups simplify into parents with accumulated geometric error and locked borders. Material sections, primitive ranges, and authored LODs remain distinct. Small persistent hierarchy metadata is separate from payload residency. Native loading retains the directory and streams compressed pages directly from the compiled file. Visible cluster keys identify a physical buffer descriptor, metadata offset, and authored LOD; GPU-safe pool retirement protects those keys. Static geometry does not consume the direct meshlet arena. DDGI and emissive preparation enumerate a compact root directory rather than scanning nonresident fine clusters.

GPU traversal uses conservative visibility and projected screen error. Visible missing children generate requests, while an admitted parent remains drawable until all required visible children are ready. Mesh shaders and vertex pulling resolve the same page mapping. Ray tracing and emissive preparation consume coarse geometry. Canonical CPU mesh data remains available for specialized consumers.

Authored LOD selection metadata is admitted together. LODs no longer own competing topology caches or streaming feedback systems. Deformable ranges share one direct topology allocation per source revision. Geometry handles and IO completions are generation checked. Page-table replacement, bindless retirement, and native destruction preserve references used by submitted work.

## Virtual textures

Eligible ordinary 2D material textures compile into independently compressed 128 by 128 tiles with a two-texel border and resident coarse fallback. HDR, signed formats, cube/3D textures, small textures, and incomplete mip chains currently use ordinary single-allocation resources. The old low/full replacement texture model has been removed.

GPU material sampling derives wanted mip from derivatives or explicit level, records missing and resident demand, and walks coarse ancestors until a resident tile is found. Adjacent resident mips blend, and a footprint crossing a tile boundary predicts one neighboring request. `Streaming.TextureMipBias` and `Streaming.TextureMinimumMip` control demand. Named material texture bindings, editor 2D previews, and the standard surface, DDGI, and ray tracing paths use the shared mapping. IO and bounded decompression run asynchronously, with source hash/version checks before publication.

Texture document version 12 removes the obsolete LOWRES output and converter options. Texture imports produce one compiled asset containing its virtual pages. Mesh transforms propagate serialization failures, and texture resource replacement clears obsolete virtual data when switching to conventional content.

The final 1x1 mip is stored as its exact RGBA8 color in the GPU page table, with sRGB decoding where required. It consumes no physical tile, upload allowance, or feedback identity. Native-view consumers retain their 1x1 resource. Particle materials resolve virtual textures through the same residency service and bind the tile, table, and feedback resources.

The software cache pools individual physical tile images. Native allocation padding is charged, rather than the smaller decompressed payload. Reused slots consume the same scheduler upload allowance as newly allocated slots. Native sparse mapping, feedback capability fallback, broader texture formats, view-based prediction, and anisotropic filtering refinement remain migration work.

## Focused validation

A Vulkan 360-frame near/far camera tour using a compiled 69-page sphere and a 2048-square virtual texture completed with mesh shaders enabled. With 1 MiB geometry and 2 MiB texture cache ceilings, native geometry page ownership converged at 1,045,344 bytes near the scene and fell to 54,256 bytes for two coarse pages. Native texture tile ownership fell from 2,064,384 to 737,280 bytes as the camera moved away. Queued requests and pending IO converged to zero. These measurements establish native suballocation reclamation for this scene; they do not establish acceptance for datasets larger than VRAM or repeated multi-scene transitions.

With physical pooling enabled, a 720-frame vertex-pulling run using three 240-frame camera cycles completed with 54 slot reuses and 26 native page allocations. Geometry ownership remained at or below 1,024,000 bytes and texture ownership at or below 2,064,384 bytes. A slower 900-frame mesh-shader tour released 2,277,376 bytes of cold pool ownership, with native geometry ownership falling to 73,728 bytes and texture ownership to 737,280 bytes before demand returned. Pool release and native destruction are separate stages.

An 1,800-frame mesh-shader run alternating populated and empty worlds every 360 frames released all native geometry and texture page allocations during both empty intervals. Feedback identity allocation held at a high-water mark of 417, with 834 reuses. A separate 720-frame vertex-pulling run replacing the world every two frames completed with 12,282 request identity reuses and a high-water mark of 486. Fixed-capacity material and geometry tables enforce the index bound independently of their generation table's reserved capacity.
