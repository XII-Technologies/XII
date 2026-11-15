# Message transports and sessions

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

Protocol negotiation exchanges a protocol UUID, version range, required/supported features, delivery capabilities and size/channel limits. Incompatible requirements fail closed. Authentication precedes session activation. The Hello/Welcome/Ready/Activated barrier prevents application dispatch before both peers agree. Certificate policy authenticates both peers. Application policy authenticates the secure server, then delegates client authorization to the event consumer. Authorization controls client sending, channel creation and recovery eligibility.

## Queues, channels and closure

The default reliable ordered channel becomes available on `Connected`. `OpenChannel` negotiates additional generation-scoped channels with an application service identifier, delivery policy, scheduling weight and queue limits. TCP channels share stream head-of-line blocking. Scheduling weights control fair message service opportunities; they are distinct from congestion control. Per-channel/per-peer byte and message budgets reject admission with `Backpressure`. Host/peer token buckets bound offered payload rates; controls remain bounded separately so draining can make progress. Receive memory and event reservations are bounded; reliable receive exhaustion explicitly terminates the connection rather than silently dropping an accepted message.

`CloseChannel` drains accepted sends before its close acknowledgement. `Disconnect(connection, true)` drains output, exchanges a disconnect control frame, performs TLS close notification where applicable and closes within the configured deadline. Abortive disconnect cancels pending work. Peer EOF, reset, TLS authentication failure, malformed frames and timeouts remain explicit error results. A `Closed` event follows prior received messages and accepted send completions. Listener closure similarly publishes `ListenerClosed` after native work drains. Shutdown stops listeners/admission first and retains terminal events for dequeue. Underlying blocking OS DNS work can delay the final M2 join after its cancelled result is abandoned.

## Recovery

Recovery is opt-in on both hosts. It requires an authenticated encrypted transport. Server session IDs and 256-bit capabilities use cryptographic randomness. Server records retain only capability hashes and bind recovery to the authenticated identity and certificate. A client ticket binds the secret to server name, port, protocol and server certificate before transmitting it. Recovery reauthenticates the peer and rotates the capability. Active-session replay, revoked capabilities and expired server records fail. Server restart invalidates its records. Graceful logout revokes recovery; unexpected transport failure detaches a record for the configured recovery window, bounded by an absolute session lifetime. `IsValid` on a ticket checks its local absolute expiry; the server may reject it sooner.

Reconnection defaults to disabled. Fixed/exponential policies have bounded attempts, maximum delays and optional jitter. Infinite retries require explicit opt-in. The logical connection handle survives automatic reconnection, while its transport/session/channel generations change. Pending application sends are cancelled and never replayed. Authentication/protocol failures do not trigger ordinary transport retries. `CancelReconnect` stops recovery attempts. TLS ticket resumption and application session recovery are separate mechanisms.

## UDP

A `NativeUdp` host uses `Listen` to bind a datagram socket, then `SendDatagram` with a destination endpoint and an owned host buffer. `Datagram` events contain the sender endpoint, payload and native status. Empty datagrams are valid messages. Truncation is reported explicitly. `Connect`, application sessions and logical channels are unsupported for this transport. Native datagram boundaries are preserved.

Connection statistics expose actual payload/wire counts, queue pressure and measured application heartbeat RTT. Native congestion statistics are explicitly marked unavailable when the provider cannot supply them. No synthetic TCP packet-loss or retransmission estimates are reported.
