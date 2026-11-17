# Message transports and sessions

For schema negotiation, typed messages, and reflected RPC over these sessions, see [Reflection networking](NetworkReflection.md).

For native same-user streams, see [Native local transport](NetworkLocalTransport.md).

`xiiNetworkHost` owns a protocol driver and Milestone 2 native I/O context. Select transport independently for each host. `Tcp` and `TlsTcp` carry framed reliable ordered messages; `NativeUdp` carries raw connectionless datagrams. Capability queries describe the selected transport. Unsupported delivery guarantees are rejected before admission. Plain TCP, UDP and anonymous authentication require explicit insecure-development configuration. TLS uses immutable credential snapshots described in [NetworkProviders.md](NetworkProviders.md).

Host startup/shutdown are serialized. Submissions, queries and event dequeues may be concurrent after startup; destruction must not race facade access. There are no application callbacks on native worker threads. `WaitForEvent` and `TryGetEvent` transfer move-only events to their caller. Application authorization uses `AuthenticationRequired` followed by `Authorize`; optional channel authorization uses `ChannelRequested` followed by `AuthorizeChannel`. Authentication data is retained in a bounded immutable object that cleanses its secret storage at final release.

Connections, listeners and messages use host-scoped generation handles. Channels additionally carry a session generation, so a channel retained across reconnection cannot address the replacement session. Native resources and slots remain retained until outstanding operations quiesce and terminal events are consumed. Owned message buffers must come from the sending host. Rejected sends preserve ownership. Accepted sends reserve completion capacity, retain their leases through native completion, and publish exactly one `MessageSent` outcome. This outcome reports transport release, never application delivery. No accepted message is automatically replayed after failure.

## TCP and TLS example

```cpp
xiiSharedPtr<xiiNetworkCredentials> credentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
xiiNetworkSecurityConfig security;
security.m_sCertificateChainFile = "server.pem";
security.m_sPrivateKeyFile = "server.key";
security.m_sTrustAnchorsFile = "development-ca.pem";
security.m_bUseSystemTrust = false;
auto error = credentials->Load(security);
if (error.Failed()) { return error; }

xiiNetworkHostConfiguration configuration;
configuration.m_pCredentials = credentials; // TLS and certificate authentication are defaults.
xiiNetworkHost server;
error = server.Start(configuration);
if (error.Failed()) { return error; }
xiiNetworkListener listener;
error = server.Listen({xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv6), 0}, listener);
if (error.Failed()) { return error; }
xiiNetworkEndpoint assigned;
error = server.GetLocalEndpoint(listener, assigned);
if (error.Failed()) { return error; }

xiiNetworkEvent event;
error = server.WaitForEvent(event, xiiTime::MakeFromSeconds(5));
if (error.Failed()) { return error; }
if (event.m_Type == xiiNetworkEventType::Connected)
{
  xiiNetworkChannel channel;
  error = server.GetDefaultChannel(event.m_Connection, channel);
  if (error.Failed()) { return error; }
  xiiNetworkIOBuffer payload;
  error = server.AcquireBuffer(4, payload);
  if (error.Failed()) { return error; }
  payload.GetBytes()[0] = 1;
  payload.GetBytes()[1] = 2;
  payload.GetBytes()[2] = 3;
  payload.GetBytes()[3] = 4;
  xiiNetworkMessage message;
  error = server.Send(channel, std::move(payload), message);
  if (error.Failed()) { return error; }
}
```

A client creates its own host with client identity/trust credentials, sets `xiiNetworkConnectOptions::m_sServerName` to the DNS/IP certificate SAN, and calls `Connect(assigned, options, connection)`. The hostname/service overload resolves asynchronously through M2 and attempts returned endpoints in native resolver order, bounded to 32 candidates. A connection's phase deadline bounds resolution and connection attempts. IPv6 listeners default to IPv6-only; dual-stack is explicit. Native socket options are established before registration.

Messages have canonical unsigned LEB128 body lengths followed by a one-byte kind, big-endian channel identifier and 64-bit sequence. Parsing validates size/type before reserving memory. Header fragments, payload fragments and multiple coalesced frames are supported. Plain TCP submits header/payload leases with scatter/gather; TLS moves ciphertext through bounded record BIOs. M2 retains partial native transfers until completion. Applications receive whole messages and never need to reconstruct a TCP stream.

`Broadcast` takes an explicit array of recipient channels and an equally sized result array. It copies the caller's payload into each admitted send lease. Admission is independent and follows recipient order; a full queue or invalid recipient does not suppress later recipients. Inspect every result for its message handle or admission error, then consume the ordinary `MessageSent` completions. A mismatched result count rejects the entire call before submission.

Protocol negotiation exchanges a protocol UUID, version range, required/supported features, delivery capabilities and size/channel limits. Incompatible requirements fail closed. Authentication precedes session activation. The Hello/Welcome/Ready/Activated barrier prevents application dispatch before both peers agree. Certificate policy authenticates both peers. Application policy authenticates the secure server, then delegates client authorization to the event consumer. Authorization controls client sending, channel creation and recovery eligibility.

## Wire protocol

Version 1 uses the same framing for TCP, decrypted TLS records, QUIC streams and QUIC datagrams. Native UDP remains application-defined. Each frame contains a canonical unsigned LEB128 body length (at most five bytes), a one-byte type, a big-endian `u32` channel, a big-endian `u64` sequence, and the payload. The body length includes the 13 metadata bytes. Overlong encodings, unknown tags, arithmetic overflow and payloads exceeding the negotiated limit are rejected. Control payloads are limited to 8,192 bytes.

All integers below are unsigned and in network byte order. `uuid` occupies 16 bytes: the high 64-bit value followed by the low value. `text` consists of a `u32` byte count and that many bytes without an embedded NUL. There is no structure padding or implicit terminator. Fields appear in the order listed, and trailing bytes are rejected.

| Tag | Payload fields |
| --- | --- |
| 1, Hello | `u32` magic `0x58494933`; protocol `uuid`; `u32` minimum and maximum versions; `u64` supported and required feature masks; `u32` delivery mask, maximum message bytes and maximum channels; `u8` authentication policy and flags; requested session `uuid`; 32-byte recovery token; `u32` authentication byte count followed by at most 4,096 credential bytes |
| 2, Welcome | protocol `uuid`; `u32` selected version; `u64` features; `u32` delivery mask, maximum message bytes and maximum channels; session `uuid`; `u8` resumed indicator and permission mask; identity `text`; `u64` remaining recovery lifetime in milliseconds; 32-byte recovery token |
| 3, Ready; 4, Activated | Empty |
| 5, Data | Opaque application bytes |
| 6, OpenChannel | `u8` delivery mode and scheduling weight; `u32` queued-message limit; `u64` queued-byte limit; `u32` application service identifier |
| 7, ChannelAccepted | `u32` admission error code, zero for acceptance |
| 8, CloseChannel | `u8` acknowledgement indicator; `u64` final sequence sent by this side |
| 9, Disconnect | `u32` networking error code, zero for ordinary closure |
| 10, Ping; 11, Pong | Empty; the frame sequence carries the probe identifier |

Authentication policy values are certificate `0`, application `1` and anonymous development `2`. Hello flag bits permit a fresh session (`1`) and request recovery (`2`). Welcome permission bits allow client messages (`1`), client channel creation (`2`) and recovery (`4`). Delivery mode values are reliable ordered `0`, reliable unordered `1`, unreliable sequenced `2` and unreliable unsequenced `3`; delivery masks use the corresponding bit positions. Control error values follow the stable `xiiNetworkErrorCode` numbering; native error values are never transmitted.

Only Data and channel-management frames carry a nonzero channel. Channel IDs combine a 16-bit generation above a 16-bit slot. Slot 1, generation 1 is the default channel; clients allocate additional odd slots and servers allocate even slots. Data sequences start at 1 independently for each sending direction and channel. Other control sequences are zero except heartbeat probes. Unknown identifiers, invalid generations and out-of-phase handshakes terminate the connection. The handshake order is client Hello, server Welcome, client Ready and server Activated. Recovery tokens are only exchanged after authenticated encryption is established and are cleared from temporary control storage.

## Queues, channels and closure

The default reliable ordered channel becomes available on `Connected`. `OpenChannel` negotiates additional generation-scoped channels with an application service identifier, delivery policy, scheduling weight and queue limits. TCP channels share stream head-of-line blocking. Scheduling weights control fair message service opportunities; they are distinct from congestion control. Per-channel/per-peer byte and message budgets reject admission with `Backpressure`. Host/peer token buckets bound offered payload rates; controls remain bounded separately so draining can make progress. Receive memory and event reservations are bounded; reliable receive exhaustion explicitly terminates the connection rather than silently dropping an accepted message.

`CloseChannel` drains accepted sends before its close acknowledgement. `Disconnect(connection, true)` drains output, exchanges a disconnect control frame, performs TLS close notification where applicable and closes within the configured deadline. Abortive disconnect cancels pending work. Peer EOF, reset, TLS authentication failure, malformed frames and timeouts remain explicit error results. A `Closed` event follows prior received messages and accepted send completions. Listener closure similarly publishes `ListenerClosed` after native work drains. Shutdown stops listeners/admission first and retains terminal events for dequeue. Underlying blocking OS DNS work can delay the final M2 join after its cancelled result is abandoned.

## Recovery

Recovery is opt-in on both hosts. It requires an authenticated encrypted transport. Server session IDs and 256-bit capabilities use cryptographic randomness. Server records retain only capability hashes and bind recovery to the authenticated identity and certificate. A client ticket binds the secret to server name, port, protocol and server certificate before transmitting it. Recovery reauthenticates the peer and rotates the capability. Active-session replay, revoked capabilities and expired server records fail. Server restart invalidates its records. Graceful logout revokes recovery; unexpected transport failure detaches a record for the configured recovery window, bounded by an absolute session lifetime. `IsValid` on a ticket checks its local absolute expiry; the server may reject it sooner.

Reconnection defaults to disabled. Fixed/exponential policies have bounded attempts, maximum delays and optional jitter. Infinite retries require explicit opt-in. The logical connection handle survives automatic reconnection, while its transport/session/channel generations change. Pending application sends are cancelled and never replayed. Authentication/protocol failures do not trigger ordinary transport retries. `CancelReconnect` stops recovery attempts. TLS ticket resumption and application session recovery are separate mechanisms.

## UDP

A `NativeUdp` host uses `Listen` to bind a datagram socket, then `SendDatagram` with a destination endpoint and an owned host buffer. `Datagram` events contain the sender endpoint, payload and native status. Empty datagrams are valid messages. Truncation is reported explicitly. `Connect`, application sessions and logical channels are unsupported for this transport. Native datagram boundaries are preserved.

Connection statistics expose actual payload/wire counts, queue pressure and measured application heartbeat RTT. Native congestion statistics are explicitly marked unavailable when the provider cannot supply them. No synthetic TCP packet-loss or retransmission estimates are reported.

## Secure datagrams

Select `xiiNetworkTransportType::SecureDatagram` for QUIC over UDP. Foundation uses MsQuic with TLS 1.3 and its native flow control, pacing, retransmission and congestion control. The protocol driver applies the same explicit credential trust, SAN and certificate-purpose checks as TLS over TCP. Peer verification completes before the connection is admitted to the application protocol. Certificate-policy servers require mutual authentication. QUIC early data, native ticket resumption and migration are disabled. Setting the native ticket option for this transport returns `UnsupportedCapability`. The application session recovery protocol is available independently.

| Channel policy | QUIC mapping |
| --- | --- |
| Reliable ordered | One persistent unidirectional stream per sending direction and channel |
| Reliable unordered | One unidirectional stream per message, terminated with FIN |
| Unreliable sequenced | QUIC DATAGRAM with a sequence check that drops stale messages |
| Unreliable unsequenced | QUIC DATAGRAM with no ordering guarantee |

Control messages use a separate bidirectional stream. Reliable receives defer native completion long enough to parse retained provider buffers directly into owned payload leases. Data received ahead of activation or channel acceptance stays pending on its own stream. Each unreliable message fits in one provider datagram. Query `GetConnectionInfo` for the current path payload limit and handle `MessageTooLarge` if it changes. A datagram send completion can report `DatagramLost` when the provider stops tracking an unacknowledged datagram. Native acknowledgements describe transport progress. Application acknowledgement is a higher-level protocol responsibility.

QUIC channel closure exchanges final send sequences to drain reliable traffic across separate streams. The host enforces the connection close deadline, including when a remote peer stops participating. Outstanding provider handles, retained receives and send descriptors are retired before connection slots are reused. The registration and configuration objects are private to each host. The shared provider lifetime bounds its execution partitions to at most two processors, with a single partition on Darwin.

The QUIC host supports IPv4 and IPv6 listeners with separate explicit binds. It reports `UnsupportedCapability` for a requested IPv6 dual-stack listener. QUIC hosts support at most 1,024 configured channels per connection. The provider uses a bounded number of unordered stream slots and bounded receive windows. Hosts can coexist with different transport types, protocol identifiers, credentials and resource limits. Credential rotation affects new connections, and a running QUIC host keeps its initial ALPN.

`GetStatistics` reports occupancy, active listeners, connections, sessions and channels, completed traffic, failure categories, receive drops and M2 buffer pool metrics. `GetConnectionInfo` exposes establishment time, terminal error and per-peer traffic. Native QUIC samples report congestion window, RTT, packet loss and reordering. Use `GetPeerCertificate` for the verified identity snapshot. `m_Tcp` controls the TCP socket options, and the host and peer token buckets can also limit native UDP sends.

## Focused validation and benchmarks

Build `FoundationTest` with the Clang Debug preset. Run the networking suites with `FoundationTest -noGui -all -noSave -assert 0 -filter Network`. The `NetworkTransport` suite covers incremental framing, a deterministic malformed frame corpus, trust and identity failures, TLS tickets, session recovery and revocation, reconnects, queue exhaustion, concurrent same-channel sends, shutdown racing with submissions, all QUIC delivery modes and IPv6 negotiation. The QUIC relay test drops every thirteenth client packet, reorders selected packets and duplicates every seventeenth forwarded packet in its fault phase. It checks the complete payload and provider loss and congestion metrics.

Enable `-networkTransportBenchmark` for the bounded loopback workload. An x86-64 Fedora Linux 45 run on an Intel Core i7-12700H with Clang 23.1.5 Debug used two hosts, 32 simultaneous peers, one M2 worker and one resolver per host. Each throughput sample moved 4 MiB in 16 KiB messages with payload validation and batches of 32. The four-channel sample alternated messages across independent logical channels. The TLS and QUIC samples used mutual certificate authentication.

| Transport | Establishments/s | One channel MiB/s | Four channels MiB/s | Mean 64-byte echo RTT | Closures/s | Incremental server buffer pool bytes/peer | Process CPU and elapsed time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| TCP | 10,632 | 785 | 881 | 85 µs | 27,734 | 16,512 | 0.040 s and 0.017 s |
| TLS | 1,327 | 195 | 248 | 122 µs | 17,335 | 23,144 | 0.153 s and 0.067 s |
| QUIC | 720 | 93 | 589 | 59 µs | 9,237 | 488 | 0.089 s and 0.100 s |

These are single debug loopback samples. Pool bytes measure the M2 allocator delta and exclude provider, process and kernel allocations. The benchmark checks host capacity at 32 peers. The backpressure regression separately verifies send-slot exhaustion and ownership preservation. WAN capacity, tail latency and cross-OS peer interoperability require separate measurements.
