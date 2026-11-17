# Foundation communication ownership

`Foundation/Events` owns synchronous in-process notifications. Use a typed event on the object or subsystem that owns the notification. A default event with a mutex serializes its callbacks. Use copy-on-broadcast for callbacks that need to run outside the event's lock. Before destroying a receiver or unloading a plugin, remove its subscriptions and allow any in-flight snapshots to finish.

`Foundation/Messaging` owns reflected local messages, handler dispatch and queues. World posting still clones through RTTI and chooses the message allocator based on the delivery time. The queue borrows pointers from the caller. Dispatch IDs are process-local. `Foundation/Network` owns cross-process schemas and remote delivery.

`xiiGlobalEvent` remains under Events for existing startup, shutdown, plugin and application notifications. Inspector uses the names and registration counts of these events in its telemetry view. Replacing the named registry requires migrating both the lifecycle publishers and Inspector's diagnostics together.

## Legacy Communication

| Component | Migration |
| --- | --- |
| `IpcChannel` and platform implementations, including ENet | Phases 2 and 3. Keep the current channels available while clients move. |
| `IpcProcessMessageProtocol` | Phase 3. Replace the reflected process-message protocol with the Editor, EngineProcess and Player migration. |
| `MessageLoop` | Retire after the legacy channels no longer need it. |
| `RemoteInterface` and `RemoteMessage` | Retire after the services using the legacy remote protocol migrate. |
| `RemoteInterfaceEnet` | Retire with ENet after the last dependent service moves. |
| `Telemetry` and `TelemetryMessage` | Phase 5. Replace with the diagnostics transport used by Inspector and other telemetry consumers. |
| `DataTransfer` | Phase 5. Move to diagnostics and tooling. |

Phase 2 can introduce the new transport while the current channel and protocol continue serving existing process connections. The transport can use the network layer's identity and serialization contracts. Local queue entries and message IDs stay process-local throughout that transition.
