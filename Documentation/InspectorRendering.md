# Inspector performance analysis

Connect `xiiInspector` to a standalone application or editor engine process running `xiiInspectorPlugin`. **Panels → Profiler**, **Rendering Profiler**, and **Render Graph** separate application performance, GPU execution, and structural diagnostics. The default workspace gives analysis the central area; other existing panels remain available through Panels and can be docked or floated normally.

## Profiler

Live monitoring receives completed CPU frames and scopes from the existing profiling system. The application discard threshold still applies. Frame history shows CPU wall time, FPS, average, P95, maximum, and budget overruns. Select a frame or a start/end frame range to inspect nested thread scopes, inclusive/self time, calls, mean, minimum, P95, and maximum. Scope intervals are clipped to the selected CPU frame range. Self time subtracts recorded child interval coverage on the same thread before filtering; discarded instrumentation cannot be reconstructed.

Drag timelines to pan, use Ctrl+wheel to zoom, double-click a scope to focus its interval, and hover for operation/function details. Search scopes/functions, select a thread, or double-click a hotspot to focus its name. Scope and table selection use thread/scope identities. Timeline and individual-scope rows are limited to 20,000 matches; hotspot aggregation retains all matches in the selected interval.

## Rendering Profiler

Resolved render-graph pass and submission measurements retain graph instance, registered pass/submission index, actual queue, GPU recording frame, duration availability, and CPU recording time. CPU recording time is capture-membership metadata, not GPU clock calibration. Select an operation to see its history; filter by graph/view, pass, queue, individual GPU frame, or GPU frame range. Duplicate graph names receive contextual ordinals and duplicate pass names include pass indices. Technical identities are available in tooltips and the copy-identity context menu.

Unsupported queues, aborted/unresolved queries, and unavailable measurements display **Unavailable**. Timing history leaves gaps for unavailable samples. GPU duration queries provide no absolute timeline placement, and submissions on different queues can overlap: no sum is presented as GPU frame wall time. The budget line is a reference for the selected operation. Existing timestamped GPU scopes remain available in their own GPU timeline lanes, separated from CPU clocks; their producer does not supply application frame identities.

**Locate selected pass in Render Graph** selects the corresponding graph instance and registered pass. When no structural capture is cached, it explicitly requests one. Structural snapshots are never used as the timing update mechanism. A newly captured graph can represent a later topology than an older timing sample.

## Live monitoring and recording

**Pause** freezes the inspected live data; **Resume live** returns to the rolling history. Hidden or paused profilers stop their stream subscription unless recording. A recording continues when its docks are hidden. Histories and producer queues remain bounded, and neither pause nor capture waits for the application or GPU.

Choose **Until stopped**, **Frame count**, or **Duration**, configure the limits, then **Start recording**. Manual recordings also have 36,000-frame and ten-minute ceilings. The configurable decoded-data budget is 1–256 MiB. Recordings stop when the selected limit or memory/sample ceiling is reached. On normal stop, a bounded 500 ms tail accepts delayed scopes and GPU measurements belonging to the recording interval, then freezes the capture. Results arriving after this tail are absent; no timing is invented for them. Memory-limited captures stop immediately and are marked truncated.

Completed captures are selectable in both profilers. Up to eight captures share a 256 MiB estimated decoded-data history budget; starting a new recording evicts old captures as needed to reserve its limit. Live retention is separate: 1,200 CPU frames, 100,000 CPU scopes, 60,000 timestamped GPU scopes, and 60,000 render-graph measurements, with a 64 MiB estimated sample/metadata budget. Individual recordings also cap CPU scopes at 500,000, timestamped GPU scopes at 100,000, and render-graph samples at 500,000. Limits include conservative per-sample string estimates; Qt widgets, the transport string dictionary, and JSON serialization require additional bounded storage.

**Save** writes the inspected data; **Open** imports a previous capture without replacing the current capture on validation failure. Files retain Chrome trace events and use the optional version-2 `xiiProfiler` extension for exact frame/scope identities, GPU measurements, graph names, dropped counters, and truncation status. Old Chrome complete (`X`) and named begin/end (`B`/`E`) events and `xiiGpuPassCosts` extensions remain readable. Files are limited to 256 MiB. JSON serialization occurs only for file operations.

## Render Graph

Explicit **Capture** retains the existing dependencies, execution order, culling, compilation statistics, resource versions, logical lifetimes, alias groups, barriers, merge groups, submissions, waits, and signals. A request covers up to 64 graph instances during a two-second window. Graph captures retain schema version 1 and the existing 16 MiB per-file limit. Added resource-state names are optional metadata.

Names and context lead the dependency, resource, barrier, version, and queue views. Resource dimensions and memory sizes use readable units. Opaque identifiers remain in tooltips, copied identifiers, hidden technical columns, or the selected node's technical details. The existing Resources panel also leads with descriptions and exposes identifiers through its context menu. A missing description uses a type-based unnamed-resource label.

The Data Transfer dock, menu action, widget/UI files, and Inspector `TRAN` subscription are removed. Foundation `xiiDataTransfer`, its callers, and ordinary profiling captures remain intact.

## Streaming protocol and lifetime

`InspectorPlugin/Profiling.cpp` handles version-1 binary `PSTR` telemetry. `SUBS` identifies a consumer session, `BTCH` carries an incremental batch, `ACKN` acknowledges it, and `STOP` unsubscribes. A three-second lease bounds abandoned subscriptions. CPU/GPU instrumentation remains opt-in to this side channel; ordinary capture buffers are never drained by live monitoring.

Batches are emitted at most every 33 ms from application telemetry updates. One acknowledged reliable batch is allowed in flight, bounding network backlog without stalling the engine. Producer retention is limited to 8,192 CPU scopes, 256 completed CPU frames, 2,048 timestamped GPU scopes, and 4,096 retired graph query measurements. CPU scope writers and GPU event callbacks use non-blocking queue acquisition; overflow/contention counters are transmitted. Thread metadata and the thread selector are capped at 1,024 entries. Live graph-name metadata is capped at 256 graphs and recording graph metadata at 4,096; metadata contributes to the decoded-data budget. A session string dictionary has 32,768 entries and names are bounded to 512 UTF-8 characters; empty or exhausted names use type-based fallbacks. Unchanged names and graph/view metadata are not repeated.

Frame sequence numbers and original CPU timestamps survive batching. GPU samples retain recording-frame identities through delayed query retirement. The receiver validates counts, names, intervals, session/sequence, and packet footer before appending. Disconnection stops producer work; reconnection starts a fresh dictionary/session and rejects stale batches. Completed owned captures remain inspectable.

Telemetry callbacks append to the GUI-owned session. Plot painting runs on a separate 33 ms UI timer; aggregate tables update at most every 100 ms and reuse rows by identity. The Inspector network pump runs every 16 ms. No full Chrome document is constructed for live telemetry and no full graph snapshot is sent to refresh timing data.

## Verification

```powershell
cmake --build Workspace/vs2026x64 --config Debug --target Inspector InspectorPlugin InspectorTest FoundationTest GraphicsCoreTest -- /m:2
$env:QT_QPA_PLATFORM = 'offscreen'
Output/Bin/WinVs2026Debug64/InspectorTest.exe -nosave -nogui -all
Output/Bin/WinVs2026Debug64/FoundationTest.exe -nosave -nogui -all -filter Profiling
Output/Bin/WinVs2026Debug64/GraphicsCoreTest.exe -nosave -nogui -all
```

Coverage includes bounded incremental producer queues, non-destructive ordinary captures, binary validation, 64-bit CPU/GPU frame attribution, late results, capture limits/history, file-format round trips, pause/resume, real separate-process plugin streaming and reconnection, native Qt controls/rendered previews, duplicate graph/pass names, and Vulkan query resolution. Runtime verification is Windows/MSVC Debug with Vulkan. D3D12 remains disabled; other operating systems, Shipping, mixed-DPI/multi-monitor use, and a full editor-host profiling session require separate acceptance.
