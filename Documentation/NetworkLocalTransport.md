# Native local transport

`xiiNetworkTransportType::Local` uses the existing host, framed protocol, session negotiation, channels, and buffer pool. Local provides reliable, ordered delivery over one byte stream. The transport reports OS user authentication and has no encryption or certificate recovery tickets. The host reconnect policy can establish a fresh session.

## Listen and exchange a message

```cpp
#include <Foundation/Network/NetworkHost.h>

bool SendLocalMessage()
{
  xiiNetworkLocalEndpoint endpoint;
  if (xiiNetworkLocalEndpoint::Make("xii-editor-local", endpoint).Failed())
  {
    return false;
  }

  xiiNetworkHostConfiguration configuration;
  configuration.m_Transport = xiiNetworkTransportType::Local;
  configuration.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;

  xiiNetworkHost server;
  xiiNetworkHost client;
  xiiNetworkListener listener;
  xiiNetworkConnection connection;
  if (server.Start(configuration).Failed() || server.Listen(endpoint, listener).Failed() ||
      client.Start(configuration).Failed() || client.Connect(endpoint, {}, connection).Failed())
  {
    return false;
  }

  const auto timeout = xiiTime::MakeFromSeconds(5);
  xiiNetworkEvent event;
  if (client.WaitForEvent(event, timeout).Failed() || event.m_Type != xiiNetworkEventType::Connected)
  {
    return false;
  }

  xiiNetworkChannel channel;
  const xiiUInt8 uiPayload[] = {1, 2, 3};
  xiiNetworkMessage message;
  if (client.GetDefaultChannel(connection, channel).Failed() || client.Send(channel, uiPayload, message).Failed())
  {
    return false;
  }

  if (server.WaitForEvent(event, timeout).Failed() || event.m_Type != xiiNetworkEventType::Connected ||
      server.WaitForEvent(event, timeout).Failed() || event.m_Type != xiiNetworkEventType::Message)
  {
    return false;
  }
  return event.m_Payload.GetBytes() == xiiArrayPtr<const xiiUInt8>(uiPayload);
}
```

The server and client can run in separate processes under the same user. Names contain 1–48 lowercase ASCII letters, digits, hyphens or underscores. A listener query, each related connection, and its immutable session metadata expose the service name. The client's end is unnamed. Local has no IP address, port, or DNS lookup.

## Authentication and ownership

- Windows uses overlapped named pipes and the existing IOCP workers. A protected DACL grants access to the current user SID and the pipe rejects remote clients. The client verifies the pipe owner's SID. The server verifies the security context of the first read before publishing received bytes. The listener creates the next instance before transferring the accepted handle. Accepted handles retain their original IOCP shard and context. Open accepted instances reserve the name after listener closure. A busy endpoint rejects the native connection attempt, which the host reconnect policy can retry.
- Linux and macOS use nonblocking Unix-domain sockets with epoll and kqueue, respectively. The listener exclusively creates a directory under `/tmp` with owner-only access. Linux checks `SO_PEERCRED` and macOS checks `getpeereid`. Both require the effective user to match. Cleanup checks the open directory and bound entry identities before removal. A conflicting or orphaned path remains for the owner to inspect and remove.
- The authenticated identity represents an OS account. All processes authorized as that user are within the trust boundary. Connection events and session metadata preserve the verified identity. Application messages become available after session activation.

For direct I/O, use `ListenLocal` or `OpenLocal`, then submit `Accept` or `Connect` on the existing context. The accepted stream is in `m_AcceptedLocalSocket`. Move it into `Register` on the accepting context or let it close at destruction. On a Windows server, `GetLocalPeerIdentity` returns `InProgress` until the first successful read verifies the caller.

Successful submission reserves exactly one terminal completion. Rejection preserves caller-owned leases. Deadlines, cancellation and deregistration retire native I/O before publishing returned leases. Unix sockets use scatter/gather. Named pipes issue one span per overlapped operation and advance through the remaining spans using the same transfer-progress state. Host disconnection drains through ordered Disconnect frames within the configured close timeout. Direct Local I/O closes through deregistration.

## Benchmarks

Run the `NetworkTransport / HostTransportBenchmark` subtest with `-networkLocalBenchmark -networkBenchmarkConnections 1` or `32`. It compares Local with TCP loopback using 16 KiB messages, 128 samples of 64-byte echo RTT, sequential connection establishment, and one-channel and distributed workloads.

Windows x64 MSVC Debug results from one run per workload. Each throughput column uses 64 MiB. The distributed workload uses four channels at one client and the default channel of each peer at 32 clients.

| Transport | Clients | Mean establish, µs | 64-byte RTT p50 / p99, µs | One channel / distributed, MiB/s | Worker CPU / wall, s | Server pool allocations / reuses |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| TCP loopback | 1 | 3,084 | 80 / 131 | 451 / 453 | 0.250 / 0.297 | 9 / 20,938 |
| Local named pipe | 1 | 923 | 90 / 121 | 592 / 614 | 0.203 / 0.226 | 8 / 17,608 |
| TCP loopback | 4 | 916 | 138 / 241 | 163 / 306 | 0.719 / 0.624 | 17 / 23,992 |
| Local named pipe | 4 | 624 | 129 / 234 | 431 / 919 | 0.234 / 0.239 | 18 / 19,458 |
| TCP loopback | 32 | 657 | 114 / 187 | 372 / 618 | 0.375 / 0.315 | 66 / 23,234 |
| Local named pipe | 32 | 412 | 139 / 218 | 470 / 905 | 0.156 / 0.240 | 76 / 21,842 |

Both host transports recorded zero buffer-pool backpressure. Worker CPU time sums the native I/O threads and excludes the host protocol driver and benchmark caller. These runs include first-connection warm-up and Windows scheduler granularity.
