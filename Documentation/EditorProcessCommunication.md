# Editor process communication

The Editor opens a unique native Local endpoint before launching each EngineProcess or AssetProcessor child. The child receives the endpoint name and parent PID as launch arguments. Both peers check the PID reported by the operating system in the authenticated session. Each launched child owns its connection and schema agreements. The Editor can close or restart one process independently of the others.

EngineProcess and AssetProcessor have separate protocol identities and explicit reflected message contracts. Readiness requires agreement on those contracts as well as an authenticated connection. Project readiness follows EngineProcess applying project configuration and completing its application startup. Document and view operations use an ordered channel. Asset workers keep one request in flight per child. Existing synchronous callers use finite waits and correlate operations that can overlap by request ID or document and view identity.

Network workers publish events. The owning Editor, asset worker, or EngineProcess thread pumps them and runs application handlers. Worker-originated log and long-operation submissions use the negotiated channel without pumping receive callbacks or waiting for capacity. During interactive startup, the Qt timer pumps events and keeps repaint and shutdown notifications active. The Editor excludes ordinary editing input during that wait and retains the last 64 KiB of child diagnostic output. Closing the process connection rejects further submissions and retires outstanding native work.

## Remote EngineProcess

Run the engine with `-remote -ipc_remote_listen <IP:port>`. Both peers must provide `-ipc_cert <chain.pem> -ipc_key <key.pem> -ipc_ca <trust.pem>`. Choose `-ipc_remote_transport tls` or `-ipc_remote_transport quic` on both peers. The Editor accepts the service address and supports `-ipc_server_name <certificate-name>` when connecting by numeric IP to a certificate issued for a DNS name. The protocol uses the `xii-editor/1` ALPN. Issue certificates only to peers authorized to perform Editor operations on the selected engine process.

## Measurements

On Windows x64 Debug, the separate-process `EngineProcessLifecycle` benchmark reached schema readiness in 90.3 ms after launch and measured 227/326 µs p50/p99 across 128 reflected redraw synchronization round trips. The exchange ran before project setup, so these timings exclude rendering and document synchronization.
