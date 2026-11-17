# FileServe transfers and reverse discovery

The Editor and standalone FileServe tool can send their service address to a waiting development device. Reverse discovery uses a separate versioned network protocol. The host sends an address advertisement with a request ID and waits for the device to acknowledge it. The device then checks the advertised FileServe service within its original deadline.

## Reverse discovery configuration

- For processes on the same machine, launch the waiting device with `-fs_discovery_local fileserve-device`. Enter `local:fileserve-device` in **Connect to Device**. The native Local transport authenticates the OS account.
- For a remote device, install separate certificates and keys for both processes. Pass `-fs_cert <chain.pem> -fs_key <key.pem> -fs_ca <trusted-ca.pem>` to each. The device's server certificate must match the address or DNS name entered in the Editor. Use `-fs_discovery_name` on the announcing host when connecting by IP to a device certificate issued for a DNS name.
- Both processes use mutually authenticated TLS/TCP by default. Select QUIC with `-fs_discovery_transport quic` on both ends where the QUIC provider is available. The device listens on port 2042 unless both ends use `-fs_discovery_port`. Use `-fs_discovery_bind` to select a numeric device bind address.

Discovery protects the address advertisement. The subsequent file service connection, including shader commands, still uses the legacy ENet transport and requires a trusted development network. Direct connection configuration remains `-fs_server <address:port>` or `xiiFileserve.txt`. `-fs_no_save` disables automatic persistence of the last discovered address.

## Mounts, transfers, and cache

- The server resolves client mount requests through special-directory aliases. It permits `sdk` and `project` by default. Pass `-fs_read_roots <alias> ...` to specify a different read allowlist. Pass `-fs_write_roots <alias> ...` to enable uploads and deletes under selected readable aliases. An alias must also be configured on the server.
- Relative paths stay under the mounted directory after canonicalization. File operations reject traversal, symlinks encountered in the requested path, and non-regular files. Mount paths are controlled by the server.
- Downloads and uploads stream through staged files. The receiver checks the byte count and hash before atomic installation. Uploads have one active record per legacy client ID, with a global limit of 64 records. An upload acknowledgement identifies the request and indicates whether the server committed the file.
- Cache data and metadata are keyed by server address and mount. The client checks cached content against its stored hash before reusing it after startup. Set `-fs_cache_dir <absolute-path>` to select a cache directory. Resource reload invalidates the cache check timestamps before notifying the resource manager.
- File and shader operations use bounded synchronous waits through the legacy transport. Update both the server and client together to use the correlated upload and shader message formats.

## Shader compilation

The standalone FileServe tool accepts shader requests through its shader-specific capability. Set `-fs_shader_platforms <platform> ...` to enable the listed targets. The shader source must be an `.xiiShader` under a mounted readable root. The client also needs one server-authorized writable mount named `shadercache` for compiler output. Shader compilation returns a correlated result and compiler log. A successful compile invalidates cached file checks on the requesting client.

## Measurements

The Windows x64 Debug `FileserveTest -fs_measure` workload used a separate client process and the legacy loopback file transfer connection after Local discovery. One run measured a 3.839 ms empty-file open, 27.9 MiB/s for a 64 MiB download, and 3.8 MiB/s for a 32 MiB upload through server commit. One thousand cached opens averaged 0.024 ms. These figures include VFS and disk work. Comparative baselines, latency percentiles, CPU and memory measurements, and concurrent scaling remain outstanding.
