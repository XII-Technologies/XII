# FileServe transfers and reverse discovery

The Editor and standalone FileServe tool can send their service address to a waiting development device. Reverse discovery uses a separate versioned network protocol. The host sends an address advertisement with a request ID and waits for the device to acknowledge it. The device then checks the advertised FileServe service within its original deadline.

## Reverse discovery configuration

- For processes on the same machine, launch the waiting device with `-fs_discovery_local fileserve-device`. Enter `local:fileserve-device` in **Connect to Device**. The native Local transport authenticates the OS account.
- For a remote device, install separate certificates and keys for both processes. Pass `-fs_cert <chain.pem> -fs_key <key.pem> -fs_ca <trusted-ca.pem>` to each. The device's server certificate must match the address or DNS name entered in the Editor. Use `-fs_discovery_name` on the announcing host when connecting by IP to a device certificate issued for a DNS name.
- Both processes use mutually authenticated TLS/TCP by default. Select QUIC with `-fs_discovery_transport quic` on both ends where the QUIC provider is available. The device listens on port 2042 unless both ends use `-fs_discovery_port`. Use `-fs_discovery_bind` to select a numeric device bind address.

The transfer service has its own protocol identity and authenticates independently of discovery. With no certificate configured, the default is native Local. Use `-fs_server localhost:<port>` to connect on the same machine. The port selects the name `xii-fileserve-<port>` for the native endpoint. For a remote service, configure separate peer certificates and select `-fs_transport tls` or `-fs_transport quic` on both sides. `-fs_bind` selects the server's numeric bind address, and `-fs_server_name` can override the certificate name when connecting by IP. Transfers use the existing reliable session channel. Direct address configuration via `xiiFileserve.txt` and opt-out via `-fs_no_save` remain available.

## Mounts, transfers, and cache

- The server resolves client mount requests through special-directory aliases. It permits `sdk` and `project` by default. Pass `-fs_read_roots <alias> ...` to specify a different read allowlist. Pass `-fs_write_roots <alias> ...` to enable uploads and deletes under selected readable aliases. An alias must also be configured on the server.
- Relative paths stay under the mounted directory after canonicalization. File operations reject traversal, symlinks encountered in the requested path, and non-regular files. Mount paths are controlled by the server.
- Downloads and uploads stream through staged files. The receiver checks the byte count and hash before atomic installation. Each authenticated session can own one upload and one download, with a server limit of 64 sessions. The server advances downloads in bounded turns and retains the next chunk under backpressure. An upload acknowledgement identifies the request and indicates whether the server committed the file.
- Cache data and metadata are keyed by server address and mount. The client checks cached content against its stored hash before reusing it after startup. Set `-fs_cache_dir <absolute-path>` to select a cache directory. Resource reload invalidates the cache check timestamps before notifying the resource manager.
- The synchronous file and shader interfaces wait on host events with finite deadlines. Disconnect releases the server's per-session mount and staging state. A reconnect establishes a fresh session and restores active mounts. Update both the server and client together when moving to the new transfer protocol.

## Shader compilation

The standalone FileServe tool accepts shader requests through its shader-specific capability. Set `-fs_shader_platforms <platform> ...` to enable the listed targets. The shader source must be an `.xiiShader` under a mounted readable root. The client also needs one server-authorized writable mount named `shadercache` for compiler output. Shader compilation returns a correlated result and compiler log. A successful compile invalidates cached file checks on the requesting client.

## Measurements

The Windows x64 Debug `FileserveTest -fs_measure` workload used a separate client process and the legacy loopback file transfer connection after Local discovery. One run measured a 3.839 ms empty-file open, 27.9 MiB/s for a 64 MiB download, and 3.8 MiB/s for a 32 MiB upload through server commit. One thousand cached opens averaged 0.024 ms. These figures include VFS and disk work. Comparative baselines, latency percentiles, CPU and memory measurements, and concurrent scaling remain outstanding.
