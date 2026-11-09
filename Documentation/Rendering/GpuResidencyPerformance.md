# GPU residency performance

The default-device xiiGpuResidencyManager owns geometry pages, virtual texture tiles, feedback identities, physical pools, and shared page scheduling. See [GPU memory and streaming](GpuMemoryStreaming.md) for ownership counters, native Vulkan budgeting, asset formats, and current validation boundaries.

## Current execution and lifetime

World extraction retains resource-revision geometry leases, primitive-range coverage, transforms, bounds, and material state. Unchanged instances reuse preparation. Coverage indices are built once per revision; changed ranges use binary searches instead of full meshlet scans. Native assets load compiled clusters without rebuilding them.

All authored LOD selection metadata is admitted together. Static detail streams in geometry pages; deformable authored ranges share direct topology. GPU visibility traverses compiled page hierarchies using projected error and keeps a valid parent when visible child pages are missing. Vertex pulling and mesh shaders resolve the same physical payloads. Geometry page metadata retains source ordering; the direct deformable path retains primitive-range indexing and group reservation batching.

The first streaming invocation establishes the frame's geometry transfer allowance. Subsequent views share the remainder. Texture uploads have an independent allowance in the same scheduler, including reuse of existing physical images. Pending queues are bounded and sorted by priority, age, and deterministic tie breaks. IO retains its source revision and cannot publish into a replacement source.

Page withdrawal, physical-slot retirement, reuse, descriptor retirement, and native destruction are separate transitions. Native allocation ownership remains charged throughout. Resident-ring entries are removed in constant time when their source disappears. Feedback request identities enter a quarantine until every frame slot has captured, cleared, and consumed feedback after withdrawal. Queued requests retain their creation frame so recycled identities cannot inherit stale queued work. This bounds identity growth across repeated scene transitions without blocking GPU readbacks.

Graph metadata transfers retain shared upload payloads. Frame snapshots, subsystem epochs, and deferred retirement preserve ownership across abandoned graphs, delayed recordings, hot reload, and cancellation. Reconfiguration rejects active geometry or virtual textures; an identical configuration is a no-op.

## Complexity and diagnostics

- Changed primitive ranges query revision-owned interval indices in O(log(meshlets)); unchanged entries reuse counts.
- Instance synchronization performs membership/state work without repeated meshlet traversal or unchanged matrix calculations.
- Missing-page scheduling examines at most 4096 queued requests per frame. IO plus ready payloads is limited to 32 jobs.
- Resident-page eviction examines at most 512 ring entries per frame. Physical retirement examines at most 512 entries; free-pool trimming examines at most 32 slots per size/color class.
- Native category queries read cached totals under the allocator ledger lock. Detailed allocation enumeration runs only on an explicit diagnostic request.

Inspector exposes streaming latency, admitted/deferred requests, uploads, logical pages, feedback ID high water/reuse, pool backing/reuse/retirement/padding, native allocation owners, and heap usage. Keep active payload, reusable storage, native suballocation ownership, and committed heap capacity distinct when evaluating unload behavior.

Static cluster metadata streams inside geometry payloads. The direct metadata arena is allocated only for mutable or deformable geometry. Root-cluster enumeration uses cumulative counts and binary search for coarse consumers. Source bindings include the mesh-buffer revision, preventing in-place editor reloads from reusing the previous page directory. Full authored-scene navigation, datasets larger than physical VRAM, and final hot-reload/device-loss acceptance remain in progress.

## Historical CPU and direct-culling measurements

The following measurements precede hierarchical page streaming and physical pool reuse. They establish the original coverage, extraction, and global reservation costs; they are not performance measurements of the current page traversal or virtual texture implementation.

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
