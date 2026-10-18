# Render graph and rendering profiling inspectors

Start `xiiInspector`, connect to the application running `xiiInspectorPlugin`, and open **Panels → Render Graph** or **Panels → Rendering Profiler**. **Capture** requests a snapshot. **Live** requests at most one capture per second while the panel is visible. **Open** and **Save** support offline JSON captures. Drag diagrams to pan, use Ctrl+wheel to zoom, and use **Fit** to frame the data.

## Render Graph

The graph selector retains captures from up to 64 graph instances, labeled with the view name when available. Process-local instance IDs distinguish views which share the same stable graph name. Each graph is captured on its next execution within a two-second request window; captures from different graphs may have different frame numbers. Compilation failures also produce a diagnostic capture. A new server identity clears the previous application's captures.

- **Dependencies** displays directed pass dependencies in graphics, compute, and transfer lanes. Select a node to inspect its stable identity, registered index, dependencies, acquire/release records, and barrier placement. The selected pass survives live refreshes when its stable ID still exists.
- **Passes** lists compiled order, culling, queue, merge group, side effects, managed native scopes, and resolved GPU cost.
- **Lifetimes** shows logical resource lifetimes in compiled pass order. **Resources** includes external/transient/persistent flags, alias groups, texture dimensions/formats/mips, and buffer sizes. These describe logical resources, not a measurement of physical VRAM usage.
- **Barriers** shows state names, transition type, subresource ranges, and queue ownership transfers. **Versions** shows producer identity, parent versions, required/current states, and exports.
- **Queue schedule** lists pass order, producer submission waits, and fence values. Merge membership and framebuffer attachment indices are also retained in the capture.

The name filter dims unmatched nodes and filters table rows. Tables support numeric sorting. Graph captures preserve 64-bit stable IDs and frame identities as decimal strings, including values larger than JavaScript's exact integer range.

## Rendering Profiler

The profiler requests the application's most recent second of completed CPU/GPU scopes without clearing the application's profiling buffers. Enable `XII_USE_PROFILING` to record CPU scopes. The application's discard threshold still applies.

- **Timeline** shows nested scopes in separate thread/GPU lanes, with duration and function tooltips. Select a frame or lane to narrow the window. Imported traces may include GPU timestamp lanes if the producer recorded them.
- **Frame history** shows wall time against an adjustable frame budget. Click a frame bar to inspect that frame. The summary reports frame P50, P95, maximum, and over-budget count.
- **Hotspots** aggregates calls, inclusive/self time, mean, P95, and maximum per scope and lane. **Scopes** shows individual calls. Self time subtracts child interval coverage on the same lane, before applying the name filter. Frame selection clips scope intervals to the selected window.
- **GPU pass costs** uses the render graph's asynchronous duration queries, separated by graph and registered pass identity. Both the measured frame and capture frame are shown. Disabled profiling, unsupported queues, and warmup are displayed as unavailable rather than zero. Query support and `m_bEnableGPUProfiling` are controlled by the application; the Inspector does not change render settings.

GPU duration queries have no calibrated absolute timestamps, so pass costs are not placed at invented positions on the CPU timeline. Queue work can overlap, and inclusive nested scope totals can overlap. Neither sum is a frame wall-time measurement.

Up to 30 profiling captures are retained, with a 64 MiB encoded history budget. Saved profiling files use the engine's Chrome trace-event format and retain available graph costs in the optional `xiiGpuPassCosts` extension. Exported costs are bounded by the 16 MiB file limit; `xiiGpuPassCostsTruncated` marks omitted costs. The importer accepts complete (`X`) events and the engine's named begin/end (`B`/`E`) pairs. Incomplete pairs are omitted.

## Capture protocol and bounds

`InspectorPlugin/Rendering.cpp` handles request-only telemetry. Systems `RGRF` and `PROF` accept `CAPT`; responses use `DATA` or `ERRO`, followed by a 32-bit byte count and UTF-8 payload. Graph JSON schema version is 1. Capture serialization is capped at 16 MiB per response. No reliable captures are queued for disconnected clients.

Graph execution callbacks run synchronously on the executing thread. The producer serializes owned metadata during that callback and retains no graph, view, command-list, or GPU-resource pointers. Capture never waits for the GPU or creates timing queries. The request expires after two seconds or disconnect. The client validates schema/version, array limits, dependency order, resource lifetimes, and barrier references before replacing displayed captures. Invalid files leave the previous capture visible.

The UI limits graph captures to 4,096 passes, 16,384 resources, and 65,536 dependency edges. Graph retention has a 64 MiB encoded budget. Profiling imports permit 200,000 trace events; the timeline draws at most 20,000 bars while tables retain all decoded scopes. Limits are explicit to keep live tools responsive.

## Validation

```powershell
cmake -S . -B Workspace/vs2026x64
cmake --build Workspace/vs2026x64 --config Debug --target Inspector InspectorPlugin InspectorTest GraphicsCoreTest -- /m:1
$env:QT_QPA_PLATFORM = 'offscreen'
Output/Bin/WinVs2026Debug64/InspectorTest.exe -nosave -nogui -all
Output/Bin/WinVs2026Debug64/GraphicsCoreTest.exe -nosave -nogui -all
```

`InspectorTest` compiles the production dock implementations. It covers compiled graph serialization, exact frame IDs, offline loading, selection/filtering/reset, malformed captures, scope nesting/self time, unavailable timings, a separate-process telemetry round trip through the actual InspectorPlugin, and real GPU uploads/query readback with duplicate pass names in different graphs. The GPU fixture uses the configured renderer. UI tests render previews into `Workspace/inspector-graph-preview.png` and `Workspace/inspector-profile-preview.png`.

Validated on 2026-10-18 in Debug with Vulkan: Inspector and InspectorPlugin built successfully; all five Inspector subtests and all 77 GraphicsCore subtests passed. Both rendered dock previews were inspected.
