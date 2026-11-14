# Foundation Native Networking

Foundation's `Network/` API provides native IPv4/IPv6 addressing, OS name resolution, TCP streams and listeners, and UDP datagrams. It has no dependency on `Foundation/Communication`, ENet or higher engine modules. Include the individual public headers needed by the caller; they expose no OS socket types. Foundation defines its export macros in `Basics.h` and has no separate `DLL.h`.

## Organization and platform integration

- `xiiNetworkAddress` stores up to sixteen network-order address bytes, a numeric IPv6 scope and a family. Default construction is invalid; wildcard addresses are valid values. IPv4-mapped IPv6 addresses remain IPv6 values. Equality and hashes include the scope and exclude padding. Numeric formatting uses lowercase hexadecimal and the first longest compressible IPv6 zero run. Mapped addresses format in hexadecimal as well.
- `xiiNetworkEndpoint` adds a host-order `xiiUInt16` port. Port zero requests ephemeral binding. Text parsing accepts `127.0.0.1:8080` and `[2001:db8::1]:443`, including bracketed IPv6 scopes. It rejects ambiguous unbracketed IPv6 endpoint strings. Query `GetLocalEndpoint()` after binding to retrieve the assigned port. `GetPeerEndpoint()` retrieves the connected peer.
- `xiiNetworkResolver` uses `getaddrinfo` and `getnameinfo`. Forward lookup returns unique endpoints in OS preference order, with explicit family and TCP/UDP service selection. It applies no IPv4 preference or `AI_ADDRCONFIG` filtering. An empty hostname selects wildcard addresses. Reverse lookup requires a name. DNS and host database calls are synchronous and may block; there is no resolver timeout in this milestone. Resolver output is cleared on failure; other output values remain unchanged unless their operation documents partial data.
- `xiiNetworkSocket` owns the resource and supplies typed configuration and bounded readiness waits. Its destructor is protected: it is a shared implementation base, not a polymorphic deletion interface. `xiiTcpSocket`, `xiiTcpListener` and `xiiUdpSocket` are concrete move-only owners.

The existing Foundation source collection discovers all files. `Platform/Implementation/NetworkPlatform.cpp` selects private Windows or POSIX implementations at compile time. Shared native address conversion, resolver, socket, option and vector transfer code remains under `Platform/Implementation/`. Linux uses `socket`/`accept4` with atomic `SOCK_CLOEXEC` and optional `SOCK_NONBLOCK`, plus `MSG_NOSIGNAL`. macOS shares POSIX operations but uses `fcntl`, `accept` and `SO_NOSIGPIPE`, with BSD multicast option widths. Since macOS has no atomic close-on-exec creation/accept primitive, applications using concurrent fork/exec must serialize those operations with descriptor creation.

Windows uses overlapped-capable `WSASocketW` sockets with inheritance disabled, `ioctlsocket`, and Winsock vector I/O. Each live socket owns one successful `WSAStartup(2.2)` reference; each resolver call holds a scoped reference. Acceptance acquires its own reference before publishing the new socket. Moves transfer the existing reference and close releases it. Winsock's process reference counting synchronizes initialization and cleanup without a global engine I/O lock. Startup failure is a returned error. Foundation privately links `Ws2_32.lib` and `Iphlpapi.lib` (the latter provides interface-name conversion). The existing Windows PCH suppresses `winsock.h` through `IncludeWindows.h` before the private implementation includes `winsock2.h`.

## Ownership, concurrency and shutdown

Default-constructed and moved-from owners are closed. Copying and implicit native handle duplication are prohibited. Move construction transfers ownership. Move assignment transfers the source and closes the destination's previous resource; self-move is harmless. On an exceptional Windows native close failure, the source retains the displaced resource so ownership is not lost. Check explicit `Close()` when native close diagnostics matter.

`Close()` is idempotent. POSIX ownership is invalidated even on a close error; close is never retried after `EINTR`, avoiding a second close of a recycled descriptor. Windows retains ownership on a native close failure. No positive linger timer is exposed: ordinary close delegates pending output to the OS, while `SetLinger(true)` requests abortive close. Destructors perform best-effort cleanup without logging or unbounded engine waits. `Shutdown(Send)` performs a TCP half-close while retaining the receive side and ownership. Destruction does not guarantee delivery; complete the application protocol before closing when delivery matters.

Every operation on the **same socket object** must be externally synchronized, including send/receive, configuration, endpoint queries, moves, native borrowing and destruction. Different socket objects can operate concurrently. Closing a socket on another thread while an operation is active is outside the contract. `GetState()` reports local lifecycle state, not remote health; an orderly remote half-close is reported by a nonempty `Receive()` as `ConnectionClosed`. The opposite direction may remain usable. Reset is a separate error category.

## Results and nonblocking I/O

`xiiNetworkError` is an allocation-free, nodiscard result with a stable `m_Code`, `m_Domain` and preserved `m_iNativeCode`. `Succeeded()`, `Failed()`, `GetResult()` and `GetCodeName()` support normal XII error handling without parsing strings. Runtime errors are returned, without automatic logging. Resolver failures use their own native domain; POSIX `EAI_SYSTEM` preserves the underlying socket error. Scope syntax failures report `InvalidAddress`.

`xiiNetworkOperationResult` adds `m_uiBytesTransferred`. A successful TCP transfer may be partial. It never implies an entire application message has transferred. Empty TCP buffers succeed with zero bytes on a connected socket; nonempty TCP receive with orderly EOF returns `ConnectionClosed`. A zero-byte UDP transfer is a successful empty datagram. A truncated datagram returns `Truncated`, its copied byte count and sender; its remaining bytes are discarded. Receiving UDP into an empty span consumes one datagram and reports truncation when it contained data.

`Open()` defaults to nonblocking mode. `WouldBlock` means retry after readiness. `Interrupted` is returned to let the caller enforce cancellation/deadline policy; transfers and waits never loop internally over interrupted syscalls. A connect interrupted after initiation is reported as `InProgress`, with its native error retained. `Connect()` returning `InProgress` is expected; call `FinishConnect()` to poll completion. Completion first checks readiness, then consumes `SO_ERROR`, and retains a final failure so another completion check cannot mistake a cleared error for success. Close/open a failed TCP socket before another connection attempt. Resolution returns endpoints usable directly with `Connect()`; address fallback policy belongs to the caller.

`Wait(Readable/Writable, timeout)` waits for one socket, including hangup or error notification. Zero polls and returns `TimedOut` when nothing is ready. Readiness is a hint; handle the next operation's result. Negative, nonfinite or greater-than-`INT_MAX` millisecond timeouts are rejected; positive fractions round up. This is a bounded native readiness primitive, not a scheduler.

## TCP server and client

These examples use nonblocking sockets and bounded waits. They return explicit errors. The helper functions below are complete and can be used together in one translation unit.

```cpp
#include <Foundation/Network/NetworkResolver.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Network/UdpSocket.h>
#include <Foundation/Strings/StringBuilder.h>

xiiNetworkError ConnectBounded(xiiTcpSocket& socket, const xiiNetworkEndpoint& endpoint, xiiTime timeout)
{
  xiiNetworkError error = socket.Open(endpoint.GetAddress().GetFamily());
  if (error.Failed())
    return error;

  error = socket.Connect(endpoint);
  if (error.m_Code != xiiNetworkErrorCode::InProgress)
    return error;

  error = socket.Wait(xiiNetworkSocketEvent::Writable, timeout);
  return error.Failed() ? error : socket.FinishConnect();
}

// Advance by actual progress. The single deadline bounds the entire operation, including retries.
xiiNetworkError SendAll(xiiTcpSocket& socket, xiiArrayPtr<const xiiUInt8> bytes, xiiTime timeout)
{
  const xiiTime deadline = xiiTime::Now() + timeout;
  while (!bytes.IsEmpty())
  {
    if (xiiTime::Now() >= deadline)
      return xiiNetworkError(xiiNetworkErrorCode::TimedOut);

    const auto result = socket.Send(bytes);
    if (result.Succeeded())
    {
      bytes = bytes.GetSubArray(result.m_uiBytesTransferred);
      continue;
    }
    if (result.m_Error.m_Code == xiiNetworkErrorCode::Interrupted)
      continue;
    if (result.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
      return result.m_Error;

    const xiiTime remaining = deadline - xiiTime::Now();
    if (remaining.IsZeroOrNegative())
      return xiiNetworkError(xiiNetworkErrorCode::TimedOut);
    const auto ready = socket.Wait(xiiNetworkSocketEvent::Writable, remaining);
    if (ready.Failed() && ready.m_Code != xiiNetworkErrorCode::Interrupted)
      return ready;
  }
  return {};
}

// Accept one connection and send a greeting. A real server retains the listener and accepts repeatedly.
xiiNetworkError ServeGreeting(const xiiNetworkEndpoint& local, xiiTime timeout)
{
  xiiTcpListener listener;
  xiiNetworkError error = listener.Open(local.GetAddress().GetFamily());
  if (error.Failed())
    return error;
  error = listener.Bind(local);
  if (error.Failed())
    return error;
  error = listener.Listen(128);
  if (error.Failed())
    return error;
  error = listener.Wait(xiiNetworkSocketEvent::Readable, timeout);
  if (error.Failed())
    return error;

  xiiTcpSocket client;
  error = listener.Accept(client);
  if (error.Failed())
    return error;
  const xiiUInt8 greeting[] = {'h', 'e', 'l', 'l', 'o'};
  error = SendAll(client, greeting, timeout);
  return error.Failed() ? error : client.Shutdown(xiiNetworkShutdown::Send);
}

// Send a request, then receive until the server's orderly half-close. No stream message boundary is assumed.
xiiNetworkError Exchange(const xiiNetworkEndpoint& remote, xiiArrayPtr<const xiiUInt8> request, xiiTime timeout)
{
  xiiTcpSocket socket;
  xiiNetworkError error = ConnectBounded(socket, remote, timeout);
  if (error.Failed())
    return error;
  error = SendAll(socket, request, timeout);
  if (error.Failed())
    return error;
  error = socket.Shutdown(xiiNetworkShutdown::Send);
  if (error.Failed())
    return error;

  const xiiTime deadline = xiiTime::Now() + timeout;
  xiiUInt8 buffer[4096];
  while (xiiTime::Now() < deadline)
  {
    const auto result = socket.Receive(buffer);
    if (result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed)
      return {};
    if (result.Succeeded())
    {
      // Consume exactly result.m_uiBytesTransferred bytes here before reusing buffer.
      continue;
    }
    if (result.m_Error.m_Code == xiiNetworkErrorCode::Interrupted)
      continue;
    if (result.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
      return result.m_Error;
    const xiiTime remaining = deadline - xiiTime::Now();
    if (remaining.IsZeroOrNegative())
      break;
    const auto ready = socket.Wait(xiiNetworkSocketEvent::Readable, remaining);
    if (ready.Failed() && ready.m_Code != xiiNetworkErrorCode::Interrupted)
      return ready;
  }
  return xiiNetworkError(xiiNetworkErrorCode::TimedOut);
}
```

Call `ServeGreeting()` with `xiiNetworkEndpoint(xiiNetworkAddress::MakeLoopback(xiiNetworkAddressFamily::Ipv4), 8080)` and `Exchange()` from another execution context. Choose a timeout appropriate to the application. For a server assigned an ephemeral port, keep the listener alive and publish `GetLocalEndpoint()` to the client instead of guessing a port. There is no limit on accepted connections imposed by this API; application resource limits and the OS apply.

To resolve a hostname, pass `"localhost"` and `"8080"` to `xiiNetworkResolver::Resolve()`, inspect every returned endpoint, and open a fresh socket for each failed attempt. For IPv6, use `xiiNetworkAddressFamily::Ipv6` and `::1`, or parse `[::1]:8080` with `xiiNetworkEndpoint::Parse()`. IPv6 sockets start IPv6-only. Call `SetIpv6Only(false)` before binding for explicit dual-stack service where the OS supports it. A dual-stack listener returns IPv4 peers as mapped IPv6 endpoints; send/connect endpoints must match the socket family, so construct mapped IPv6 values when using an IPv6 socket with an IPv4 peer. Link-local IPv6 literals accept `%numeric-index` and `%interface-name`; formatting normalizes the scope to its numeric index.

## UDP and vector transfers

```cpp
xiiNetworkError DatagramLoopback(xiiNetworkAddressFamily family, xiiTime timeout)
{
  xiiUdpSocket sender;
  xiiUdpSocket receiver;
  auto error = sender.Open(family);
  if (error.Failed())
    return error;
  error = receiver.Open(family);
  if (error.Failed())
    return error;
  error = receiver.Bind(xiiNetworkEndpoint(xiiNetworkAddress::MakeLoopback(family), 0));
  if (error.Failed())
    return error;
  xiiNetworkEndpoint destination;
  error = receiver.GetLocalEndpoint(destination);
  if (error.Failed())
    return error;

  const xiiUInt8 header[] = {1, 2};
  const xiiUInt8 payload[] = {3, 4, 5};
  const xiiArrayPtr<const xiiUInt8> parts[] = {header, payload};
  const auto sent = sender.SendToGather(destination, xiiMakeArrayPtr(parts));
  if (sent.Failed())
    return sent.m_Error;
  error = receiver.Wait(xiiNetworkSocketEvent::Readable, timeout);
  if (error.Failed())
    return error;
  xiiUInt8 buffer[32];
  xiiNetworkEndpoint source;
  const auto received = receiver.ReceiveFrom(buffer, source);
  return received.m_Error;
}
```

`DatagramLoopback(Ipv4, timeout)` and `DatagramLoopback(Ipv6, timeout)` use the same native API. Handle unavailable IPv6 explicitly. `Connect()` on UDP selects a default peer and filters incoming datagrams; `Send()` then sends to that peer. If connection establishment reports `InProgress`, use `FinishConnect()`. Close/open to disconnect portably. UDP retains datagram boundaries and adds no reliability or ordering.

TCP `SendGather()`/`ReceiveScatter()` and UDP `SendToGather()`/`ReceiveFromScatter()` accept XII arrays of byte spans. Each call supports up to 64 spans and at most `INT_MAX` bytes in aggregate. Invalid vector counts or overflow are returned before a syscall. Empty spans are skipped; an entirely empty UDP vector still represents one empty datagram. Synchronous native calls borrow descriptors and data only until return. The ordinary transfer path uses stack descriptors with no persistent socket buffers or heap allocation. TCP partial vector transfers advance through the logical concatenation; callers must adjust both the span index and offset before retrying. One UDP gather call creates one datagram and one scatter receive consumes one datagram. UDP send rejects payloads above 65507 bytes for IPv4 and 65527 bytes for IPv6; path MTU and OS limits may impose smaller limits. IPv6 jumbograms are not supported.

## Configuration

Use named methods for nonblocking mode, address reuse, IPv6-only mode, send/receive buffers, blocking send/receive timeouts, TCP no-delay, keepalive, keepalive timing and abortive linger. Buffer sizes are requests; retrieve actual OS values through `GetBufferSizes()`. Keepalive timing accepts positive whole seconds and a positive probe count; unsupported backends return `UnsupportedOperation`. Multiple native keepalive options are applied in order; a later failure can leave earlier values configured. Blocking I/O timeouts do not bound DNS, connect or readiness waits. Native timeout errors are preserved (`WouldBlock` on some POSIX paths, `TimedOut` on Windows).

Address reuse must be selected before bind. Windows `SO_REUSEADDR` is not exclusive port ownership. The API defaults to reuse disabled; reuse is not enabled implicitly for listeners. Applications requiring strict Windows exclusivity must account for native system semantics rather than assume POSIX reuse behavior.

UDP adds broadcast permission, IPv4/IPv6 multicast join/leave, outgoing interface, hop limit and multicast loopback. IPv4 interfaces are local IPv4 addresses; IPv6 interfaces are indices. A scoped IPv6 group can supply the membership index; conflicting explicit and scoped indices are rejected. Close drops all memberships in the kernel, including those transferred by a move. Network policy and interface capabilities may reject multicast even when the API is available.

## Milestone 2 boundary

The private `Platform/Implementation/NetworkSocketAccess.h` provides borrowed native handle access to Foundation implementation code only. It grants no ownership or duplication rights. Windows sockets are overlapped-capable; all platforms support nonblocking operations, pending connection state and vector buffers. No scheduler, IOCP/epoll/kqueue integration, worker pool, public async placeholder or buffering queue is included.

An I/O context must keep the owner immobile and alive while registered, serialize socket access, associate a generation/token with its own registrations (native handle numbers are recycled), and keep all operation buffers alive until completion. Native borrowers must not close handles, change blocking mode behind the owner's back or consume `SO_ERROR` independently of connection completion. Deregistration/cancellation must precede close/move, and the context must drain completions before releasing the resource or its buffers. Cancellation alone is not a lifetime fence. Thread handoff requires external synchronization and exclusive access. These are low-level ownership constraints; the later runtime owns their enforcement.

## Focused validation

Build `FoundationTest`, then run it with `-noGui -all -noSave -filter Network`. Tests use loopback and ephemeral ports, with bounded I/O waits and transfer deadlines. They cover address values, canonical text, scopes, endpoint parsing, resolver filtering/cleanup, TCP connection/error/half-close behavior, partial and vector transfers, UDP empty/truncated datagrams, moves, repeated close and socket configuration. IPv6 and multicast capability checks report explicit skips when unavailable. No Internet service is required.
