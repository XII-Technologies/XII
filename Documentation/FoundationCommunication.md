# Foundation communication ownership

`Foundation/Events` owns synchronous in-process notifications. Use a typed event on the object or subsystem that owns the notification. A default event with a mutex serializes its callbacks. Use copy-on-broadcast for callbacks that need to run outside the event's lock. Before destroying a receiver or unloading a plugin, remove its subscriptions and allow any in-flight snapshots to finish.

`Foundation/Messaging` owns reflected local messages, handler dispatch and queues. World posting still clones through RTTI and chooses the message allocator based on the delivery time. The queue borrows pointers from the caller. Dispatch IDs are process-local. `Foundation/Network` owns cross-process schemas and remote delivery.

`xiiGlobalEvent` remains under Events for existing startup, shutdown, plugin and application notifications. Inspector uses the names and registration counts of these events in its telemetry view. Replacing the named registry requires migrating both the lifecycle publishers and Inspector's diagnostics together.

## Process and service boundaries

Foundation Network owns the native I/O workers, cancellation, framing, buffer leases, sessions, and channels. The Editor and AssetProcessor use the reflected endpoint for their process protocols. FileServe owns file authorization, transfer state, and shader request handling. Diagnostics owns subscriber demand, publication policy, and diagnostic commands. Applications pump these services on their owning threads and quiesce them before unloading dependent modules.

- [Native networking and transport documentation](Networking.md)
- [Editor process communication](EditorProcessCommunication.md)
- [FileServe operations and security](Fileserve.md)
- [Telemetry and diagnostics](TelemetryDiagnostics.md)
