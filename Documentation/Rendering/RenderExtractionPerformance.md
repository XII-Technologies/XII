# Optimized world extraction and upload measurements

## Configuration and comparison

Measured on 2026-10-15 on Windows, Intel Core i7-12700H (14 cores, 20 logical
processors), NVIDIA GeForce RTX 3070 Ti Laptop GPU, NVIDIA driver 617.42, Vulkan.
The fixture prints the actual GAL adapter selected for each run.

Both builds use Visual Studio 2026, MSVC 19.52.37025.0, x64, C++ latest, AVX2,
precise floating point and the engine's **Shipping** configuration. Generated
GraphicsCore projects specify full optimization, suitable-function inlining and
intrinsics (`/Ox /Ob2 /Oi`); Shipping disables device validation. The available
configurations are Debug, Dev and Shipping. D3D12 remains disabled.

The baseline is `1d03d6a68149aa961b22df594f2f4d3de7489e3e`, immediately before
this animated-ray-geometry follow-up. The scene database migration was already
merged in that revision. This comparison checks the follow-up's effect on existing
world-driven paths; it does not reconstruct the deleted scene database.
The updated production source revision is `fc06dc2f9`; the benchmark fixture was
committed in `6f886d716`. Subsequent report changes do not alter either workload.

The same `GraphicsFoundationTest/Pipeline/WorldExtractionBenchmark.cpp` is built
in both source trees. Its SHA256 is
`79B1D9F7C4454AC93A10AC88DAA25D79C8F22D7702425F99735510C923CAD5AD`.
Baseline production sources remain unchanged. Compiler processes and other test
runs finish before timing. Seven baseline/updated pairs run sequentially, with
the baseline first in each pair. CPU/GPU clocks are not pinned.

## Workloads

The fixture creates 4096 real dynamic world objects with existing static mesh
components referencing one box mesh. Handles are resolved after world updates;
the benchmark does not retain pointers into movable component storage.

* Unchanged packing: 16 warm-up updates, then 11 batches of 32 updates. A batch's
  duration is divided by 32. The 11-sample p95 is the largest batch average.
* Incremental changes: move one authoritative object, update the world and refresh
  its component packet; separately time packing all 4096 candidates.
* All-moving changes: move all 4096 objects, update the world and refresh their
  component packets; separately time packing the candidates.
* GPU uploads: execute two independent camera graphs against the same world GPU
  buffers. Warm every frame slot, then collect 35 samples from 40 iterations per
  change workload. Existing graph duration queries measure the actual upload pass.

Assertions run outside the timed CPU intervals. Every measured changed frame
checks one coalesced range and one upload pass across both cameras: **288 bytes**
for one changed instance or **1,179,648 bytes** for all 4096. Warmed unchanged
frames check zero uploaded bytes. The world/packet interval includes transform
mutation, `world.Update()`, component extraction messages and packet sorting;
packing excludes that interval. GPU recording and idle waits are outside CPU
timing. The fixture waits for the device between iterations to isolate duration
measurements; it is not a throughput benchmark for overlapped simulation/rendering.

## Results

All 14 final runs passed the exact upload assertions. Values below are
**median / p95 in milliseconds**. Each column takes the median of seven per-run
statistics, rather than pooling batch averages with individual frame samples.
Delta compares the two medians.

| Workload | Baseline median / p95 | Updated median / p95 | Median delta |
| --- | ---: | ---: | ---: |
| 4096 unchanged candidates, packing | 1.2200 / 1.3171 | 1.2529 / 1.3328 | +2.7% |
| One changed / 4096 candidates, packing | 1.2863 / 1.4693 | 1.2863 / 1.4205 | 0.0% |
| 4096 changed candidates, packing | 1.4027 / 1.5720 | 1.4164 / 1.6168 | +1.0% |
| One changed object, world update / packet refresh | 0.0812 / 0.1164 | 0.0754 / 0.1172 | -7.1% |
| 4096 changed objects, world update / packet refresh | 1.2444 / 2.3357 | 1.1789 / 2.3269 | -5.3% |
| Shared 288-byte GPU upload | 0.002048 / 0.002368 | 0.002048 / 0.002432 | 0.0% |
| Shared 1,179,648-byte GPU upload | 0.093504 / 0.094528 | 0.093504 / 0.094464 | 0.0% |

Unchanged packing run medians ranged from 1.200-1.403 ms for baseline and
1.201-1.307 ms for updated. One-change packing ranged from 1.170-1.488 ms and
1.249-1.509 ms; all-changing packing ranged from 1.339-1.653 ms and
1.309-1.527 ms. These overlapping ranges are larger than the final median
differences. The measurements establish a reproducible optimized baseline, not
a statistically demonstrated overall speedup. The unchanged median increase is
0.033 ms; the updated extraction also retains native-buffer revision leases and
morph inputs needed for correct animated ray geometry. These samples do not
isolate the contribution of that additional data from clock/scheduler variation.

An initial three-pair run exposed avoidable mesh-handle reference-count churn.
Extraction now retains unchanged mesh/native-buffer leases and replaces them
only when their handles change. Hot-reload and lifetime tests pass after this
optimization. Initial measurements remain in
`Workspace/extraction-before-lease-cache-shipping-{1,2,3}.log`; the table above
uses only the seven final pairs after the fix.

## Reproduction

Build the updated checkout with the existing Vulkan/Qt/SDL configuration:

```powershell
cmake --build Workspace/vs2026x64 --config Shipping --target GraphicsFoundationTest -- /m:1 /p:CL_MPcount=1
Output/Bin/WinVs2026Shipping64/GraphicsFoundationTest.exe -renderer Vulkan -filter RenderingBenchmarks -benchmark-render-extraction -noGui -all -close
```

For the baseline, archive the exact revision into a separate ignored directory,
materialize its SDL submodule at `ca551ec3bfa45f12505885fe327854f99ac82a50` and
provide the same `Data/Tools/Precompiled` dependencies. Copy only the benchmark
fixture above into its unit-test directory before CMake generation. The local
comparison uses `Workspace/RenderingBaseline-1d03d6a68`:

```powershell
cmake -S Workspace/RenderingBaseline-1d03d6a68 -B Workspace/RenderingBaseline-1d03d6a68/Workspace/vs2026x64 -G "Visual Studio 18 2026" -A x64 -DXII_BUILD_D3D12=OFF -DXII_BUILD_VULKAN=ON -DXII_ENABLE_QT_SUPPORT=ON -DXII_3RDPARTY_SDL_SUPPORT=ON
cmake --build Workspace/RenderingBaseline-1d03d6a68/Workspace/vs2026x64 --config Shipping --target GraphicsFoundationTest -- /m:1 /p:CL_MPcount=1
Workspace/RenderingBaseline-1d03d6a68/Output/Bin/WinVs2026Shipping64/GraphicsFoundationTest.exe -renderer Vulkan -filter RenderingBenchmarks -benchmark-render-extraction -noGui -all -close
```

Default suite runs skip timing workloads unless `-benchmark-render-extraction`
is supplied. Local raw runs are retained in ignored
`Workspace/extraction-{baseline,updated}-shipping-{1..7}.log` files.

## Scope and correctness verification

These measurements cover production world updates, component packet refresh,
GPU packing and actual incremental GPU uploads for a controlled static-mesh
workload. They do not measure an entire application frame, asset import, material
streaming or animated BLAS throughput. The baseline has no correct animated
skinning/morph BLAS path, so comparing its ray-tracing cost to the new functionality
would compare different results.

Animated correctness is verified independently by hardware ray-hit readback in
`PipelineExtraction/RayTracingDeformation`, including two cameras, independent
component poses, morph-before-skinning order, reflected/nonuniform normals,
hot reload, delayed captures across frame-ring reuse and authoritative object
removal/replacement. Both Debug with Vulkan validation and optimized Shipping
run the full 47-subtest GraphicsFoundationTest suite. The separate Core,
GraphicsCore and Vulkan editor suites pass 24, 24 and 73 subtests respectively.
The GPU-driven sample completes 120 ray-enabled frames and 30 raster-only frames.
