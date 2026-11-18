# Telemetry and diagnostics

`xiiApplication` owns an optional `xiiTelemetryService`. The service owns one network host and a reflection endpoint, along with its sources, client subscriptions, and queues. `InspectorPlugin` registers the engine's diagnostic sources. CPU and GPU instrumentation, resource tracking, logs, and statistics continue to use their existing engine facilities. Network serialization and transport run through Foundation Network. Inspector owns a `xiiTelemetryClient` and pumps it on the Qt thread.

## Connect Inspector

Development game applications and the instrumented samples start a same-user native local listener by default. **Connect** in Inspector lists advertised processes. Select an instance, or enter `local:<name>` to connect to a known endpoint. Use `-DiagnosticsEndpoint <name>` on the application to choose a stable name and `-DiagnosticsConnect local:<name>` on Inspector to connect directly. By default the service generates a unique endpoint for each process. The user-local discovery directory holds small instance advertisements and open leases. Inspector ignores expired leases and verifies the selected instance ID after session negotiation. Discovery metadata provides a location and display name. The transport verifies peer identity.

For remote inspection, select `-DiagnosticsTransport tls` or `-DiagnosticsTransport quic` on the application and supply `-DiagnosticsListen <IP:port>`. Use `tls://<IP:port>` or `quic://<IP:port>` in Inspector. Both peers require `-DiagnosticsCert <certificate-chain.pem>`, `-DiagnosticsKey <private-key.pem>`, and `-DiagnosticsCA <trust-anchors.pem>` for certificate authentication. Inspector accepts `-DiagnosticsServerName <certificate-name>` to validate the server against a DNS name while connecting to a numeric address. The remote ALPN is `xii-diagnostics/2`. Remote sessions use configured trust anchors and require encrypted transport. Local sessions use OS same-user authentication and have no transport encryption.

The default policy grants **Read** and **Capture**. CVar edits require **Configure**, while console execution and completion require **Console**. `-DiagnosticsControl` grants both mutating permissions to authenticated peers. An embedding application can supply an authorization delegate to `Start` to narrow permissions by authenticated session identity. Use a trust root appropriate to the peers that should receive diagnostics. Console access grants the capabilities of the application's existing console interpreter.

## Register and collect a source

Register a stable positive source ID and its name, category, encoding version, minimum interval, permission, and delivery policy. Add an optional owner-thread collector for sampled data, or gate a producer callback with `IsCollecting` for events.

```cpp
xiiResult RegisterFrameCount(xiiTelemetryService& service)
{
  constexpr xiiUInt64 SourceId = 0x4652414D45434E54ULL;
  xiiTelemetrySourceInfo info;
  info.m_uiId = SourceId;
  info.m_sName = "Completed frames";
  info.m_sCategory = "Application";
  info.m_uiMinimumIntervalMilliseconds = 100;
  info.m_bSnapshot = true;
  info.m_bLatestOnly = true;
  return service.RegisterSource(info, [](xiiTelemetryService& owner, xiiUInt64 id) {
    xiiTelemetrySample sample;
    sample.m_uiSource = id;
    sample.m_Fields.Insert("count", ReadCompletedFrameCount());
    owner.Publish(std::move(sample)).IgnoreResult();
  });
}
```

`ReadCompletedFrameCount` represents the application's local counter. Field types belong to the source's encoding contract. Table sources use `m_Records`; profiling and captures use `m_Data` with source-specific validation. `Publish` moves a sample only on successful admission and reports rejected samples. `IsCollecting`, `Publish`, and `RecordDropped` can run on producer threads. Producers use bounded admission without waiting for network I/O. Application code owns lifecycle, source registration, collectors, and `Update`. Stop producers, remove callbacks and unregister sources before unloading their code.

Call `Update` from the application loop. It drives authenticated session and schema negotiation, invokes due collectors, and distributes samples. While a source has multiple consumers, scheduled collection uses the fastest requested cadence, clamped by the source minimum. Each client has independent delivery cadence and state. Call `Shutdown` before subsystem teardown and keep the facade alive until registered producers have removed their callbacks. Create a new service for a new host. The client creates a fresh host on each `Connect`. A plugin change invalidates the negotiated registry and closes affected sessions. Clients must reconnect and subscribe using the new source catalog.

## Protocol and delivery

Diagnostics use a dedicated session protocol identity and version 2. Stable reflected schemas cover the source catalog, subscription requests and responses, sample envelopes, and command results. The catalog includes a revision and instance ID. Each request identifies a source, request ID, subscription generation, cadence, and snapshot or enable state. Sample sequences advance per client subscription. A new session or generation discards stale samples. Unsupported schemas, unknown sources, unauthorized operations, and invalid generations are rejected. Scalar fields, tables, strings, and binary attachments have explicit limits. Each binary attachment is capped at 16 MiB before network framing. Source encodings version their payload independently.

| Source | Delivery |
| --- | --- |
| Counters, memory, time, plugins, global events, input, subsystems, reflection, resources, CVars | Latest snapshots, with obsolete pending samples coalesced |
| Logs, file operations, assertions, incremental profiling | Ordered delivery through a bounded per-client queue |
| Profiling history, render graphs | Explicit snapshots or bounded capture requests |

Defaults allow eight clients, a shared queue of 1,024 samples and 32 MiB of accounted data, and 32 MiB of pending data per client. Each update attempts at most 64 publications and 32 MiB of accounted publication work, rotating which peer goes first. Ordered queue exhaustion closes the affected client to preserve stream integrity. The service exposes queue occupancy, publication bytes, drops, backpressure, time spent collecting and publishing, and maximum publication delay. The Counters source presents these statistics in Inspector. A transport send means that the network host accepted the message for delivery.

Inspector subscribes according to visible panels, with counters retained for plots and profiling retained for active recordings. Its network pump dispatches on the Qt thread. CVar edits and console operations use the existing network RPC system with permission checks, correlated results, finite deadlines, and cancellation. Cancellation ends the local wait and cannot undo a command already executed by the application.

## Offline diagnostics and known limits

`Capture(sourceId, callback)` invokes a registered collector on the owner thread and borrows each generated sample during the callback. It can run before a network host is started. Existing live deliveries remain queued. Ordinary Chrome profiling exports still work offline, and Inspector can open completed profiler files without a connection. Live CPU and GPU batches are collected separately from historical capture buffers. See [Inspector performance analysis](InspectorRendering.md) for profiler recording limits, history capture, and render graph behavior.

Each service selects one transport. Local advertisements are per user and machine. Remote endpoints are configured explicitly. Inspector presents one active session per window. An engine can serve multiple independent Inspector sessions.

## Windows performance measurements

The Foundation test uses native local peers in a single process, pumps continuously, and sends 128 64-byte snapshot requests followed by 128 256-KiB publications in groups of four. Every subscriber receives the bulk workload. Snapshot latency includes the configured 1 ms sampling interval. CPU percentage is process CPU time divided by elapsed time, expressed in units of one logical core. Pool bytes report server committed and leased memory at the end of the workload.

| MSVC x64 build | Clients | Ready (ms) | Snapshot p50 / p99 (ms) | Aggregate MiB/s | CPU | Default allocator allocations/delivery | Pool committed / leased (KiB) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Debug | 1 | 13.0 | 1.00 / 1.08 | 235 | 138% | 24.3 | 279 / 272 |
| Debug | 4 | 35.8 | 1.00 / 1.09 | 525 | 154% | 22.6 | 339 / 320 |
| Shipping | 1 | 7.8 | 1.00 / 1.10 | 477 | 210% | unavailable | 275 / 272 |
| Shipping | 4 | 15.3 | 1.00 / 1.11 | 1,212 | 296% | unavailable | 335 / 320 |

No drops were reported. Debug collection, serialization and total publication time across both phases were 18.8, 29.0 and 38.8 ms for one client and 14.6, 93.4 and 102.7 ms for four. The ordered-backpressure test bounds each peer to two queued samples and verifies that the fast peer continues receiving ordered samples after the slower peer exhausts its queue.

The Inspector benchmark runs 30 frames per configuration with 512 CPU scope additions and 256 simulated GPU timing retirement events per frame. The first client decodes the batches and renders the offscreen profiler. It exercises the publication and GUI work following query retirement.

| Local clients | CPU scopes/s | GPU event producer ns/event | Payload delivered (MiB) | Allocations | Collection / serialization (ms) | Largest GUI refresh and event pass (ms) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 1.00 M | 194 | 0 | 1 | 0.26 / 0.00 | 24 |
| 1 | 2.03 M | 104 | 0.93 | 1,182 | 41.66 / 2.01 | 188 |
| 4 | 2.39 M | 104 | 3.73 | 3,710 | 122.27 / 6.10 | 194 |

The Debug run reported no publication drops. These are absolute timings from successive runs in one process, with startup and warmup effects. The GUI passes reached about 194 ms as the live history grew, so heavy Debug captures can visibly delay updates. Physical GPU query costs and full Editor workflows were outside this benchmark.

The one-client transport benchmark measured 84.7/124.8 µs p50/p99 and 649 MiB/s on native Local, with 76.7/184.2 µs and 506 MiB/s on loopback TCP. Those are transport-level echo and throughput workloads. The diagnostics results above include schema negotiation, collection and publication.

Run the opt-in benchmarks from the checkout root. Use the corresponding output directory to benchmark a Shipping build.

```powershell
Output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -run -close -filter DiagnosticsBenchmark -diagnosticsBenchmark -diagnosticsBenchmarkClients 4
Output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -run -close -filter NetworkTransport -networkLocalBenchmark -networkBenchmarkConnections 1
$env:QT_QPA_PLATFORM = 'offscreen'
Output/Bin/WinVs2026Debug64/InspectorTest.exe -noGui -run -close -filter InspectorDiagnosticsBenchmark -inspectorDiagnosticsBenchmark
```
