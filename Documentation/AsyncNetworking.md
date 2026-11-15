# Foundation Asynchronous Networking

`xiiNetworkIOContext` runs native asynchronous TCP, UDP and hostname-resolution operations on the [native networking foundation](Networking.md). Applications own independent contexts, choose their limits and drain completion records on their own threads. There is no dependency on `Foundation/Communication`, no application callback on an I/O worker, and no session, message framing, security or reliability protocol.

## Public contracts

- `NetworkIOContext.h`: lifecycle, exclusive socket registration, operation submission, cancellation, endpoint queries and completion delivery.
- `NetworkIOOperation.h`: context-scoped generation handles, operation kinds, relative deadlines, user data and stream transfer mode.
- `NetworkIOResult.h`: move-only terminal result, byte count, returned buffers, accepted socket, sender/peer endpoint or resolved endpoint list.
- `NetworkIOBuffer.h` and `NetworkBufferPool.h`: allocator-aware move-only memory leases and bounded, thread-safe size-class reuse.
- `NetworkIOConfiguration.h` and `NetworkIOStatistics.h`: admission/fairness limits and diagnostic snapshots.

A context moves from Created to Running to Stopping to Stopped. `Start()` validates configuration and creates all workers and native resources before accepting work. Initialization failure cleans up resources already acquired. Invalid configuration may be corrected before retrying; after worker initialization fails or shutdown begins the context cannot restart. `Start()` and `Shutdown()` are mutually serialized. Other methods may be used concurrently after `Start()` returns. Context destruction must not race any call on that context.

`Shutdown()` stops admission, cancels pending work, drains native ownership, closes registered sockets and joins every worker. It is idempotent. Already accepted completion records remain available until dequeued or context destruction. Destruction discards unconsumed records only after all native access has ended. There are no detached threads. A blocking OS name lookup can delay joining the resolver executor; see DNS below.

## Ownership and identity

`Register(std::move(socket), registration)` transfers exclusive ownership of an open TCP/UDP socket or a listening TCP listener. Configure options and bind before registration. A pending synchronous connect cannot be transferred. Failure preserves ownership, although registration may already have enabled nonblocking mode. Success leaves the source closed. A second registration of that moved-from socket fails with `NotOpen`.

The context owns the original Milestone 1 socket object. There is no second native socket wrapper and no synchronous socket view while registered. Registered handles are non-owning identities, not descriptors: they include a context nonce, worker, slot and generation. Recycled slots, stale handles and foreign contexts are rejected. Readiness events carry separate monotonically issued registration tokens, so descriptor reuse cannot dispatch an old event to a new socket. Windows overlapped state occupies stable operation storage until the completion packet is consumed internally.

`Deregister()` rejects subsequent submissions, cancels pending operations and closes the resource after native operations drain. It requests removal; it does not wait for application completion consumption. Every accepted operation still delivers one result. Accepted TCP sockets are owned by the accept result and may be configured, used synchronously, or moved into **any** context. An unregistered accepted socket closes when its result is destroyed.

## Submission, ordering and completion

Submission returns `xiiNetworkError`. Success means accepted, not transferred. Failure produces no completion, leaves output handles unchanged and leaves submitted buffers with the caller. Runtime pressure returns `Backpressure`; stop races can return `NotRunning`, `ShuttingDown` or a stale `InvalidHandle` after removal. Do not spin on these errors. Drain completions, release buffers, or defer admission to the application's scheduling policy.

Every accepted operation has exactly one terminal record. `TryGetCompletion()` polls; `WaitForCompletion()` blocks the **calling** thread for a finite nonnegative duration. Zero polls. A successful dequeue returns a record even when its operation failed: inspect `result.m_Error`. A wait returns `TimedOut` when the wait expired and `NotRunning` when a created/stopped context is drained; output remains unchanged in both cases. Multiple consumer threads may dequeue, but application dispatch order then depends on those consumers. `m_uiUserData` is copied into the result for routing; the runtime never dereferences it.

TCP send and receive queues are independent. Each direction processes operations FIFO in submission acceptance order. `All` (default) continues partial transfers until the selected byte count is reached or a terminal error occurs. `Some` completes after one successful native transfer and may return fewer bytes than selected. Neither mode supplies message framing. On EOF, a nonempty receive returns `ConnectionClosed`, including any bytes accumulated by `All` before EOF. Reset and other failures remain distinct. An empty TCP operation succeeds with zero bytes and does not probe peer liveness.

UDP `SendTo`/`ReceiveFrom` each transfer one datagram, including empty datagrams. Up to 64 buffer leases form one scatter/gather operation; common vectors use inline descriptor storage. Receive leases select capacities with `SetCount()`, and only `m_uiBytesTransferred` bytes across those spans contain received data. Datagram truncation reports `Truncated`, the prefix actually copied and the sender; the remainder is discarded. Empty receive capacity still consumes one datagram. Aggregate transfers are limited to `INT_MAX`; UDP sends enforce the native IPv4/IPv6 payload limits. The TCP per-attempt byte budget never splits a UDP datagram.

## Buffers and resource limits

Acquire leases through the destination context. Successful submission moves their descriptors and shared pool-state references without copying payloads. Borrowed spans must not be accessed after submission. Cancellation and timeout do not return buffers until native socket I/O has stopped using them. The completion record then owns them. Leases and dequeued records can outlive the context; any supplied allocator must outlive all its leases. Buffers from another pool/context are rejected rather than escaping context memory accounting.

The pool has up to eight strictly increasing size classes, terminated by zero entries, and a hard committed-memory budget. Free blocks store an intrusive next pointer; ordinary reuse allocates no metadata. Requests above the largest class use uncached allocations. Cached and leased bytes both count against the budget. The standalone pool offers `Trim()`; a context retains its cache until destruction. Pool exhaustion preserves the output lease. Backing allocations follow Foundation allocator behavior; the runtime does not replace the engine's general out-of-memory policy.

Operation slots, registered sockets, per-socket operations, queued submissions, queued send bytes, receive requests, resolver requests, accept requests and completion capacity all have explicit limits. An accepted operation reserves completion capacity, so a slow consumer causes admission backpressure instead of dropped results or unbounded queues. Slots remain retained through completion consumption and native quiescence. Canceled blocking DNS can retain a slot after its logical completion has been consumed. Buffer memory is bounded independently, including leases retained by the application.

Defaults use two I/O workers and two resolver workers. Limits and processing budgets are configured before `Start()`. Increasing capacity reserves storage; size the limits for the workload rather than starting a context per connection. Accept concurrency limits the number of native AcceptEx requests issued per listener; queue capacity separately bounds accepted sockets waiting in completion records. Applications replenish their accept submissions after consuming results.

## Cancellation, deadlines and errors

`Cancel(operation)` and `Cancel(registration)` request individual or socket-wide cancellation. The latter leaves a usable connected TCP/UDP registration open; `Deregister()` also closes it. A canceled connect retires its registration because an interrupted connection is not a reusable stream. Repeated cancellation before terminal commit is harmless. A terminal or stale operation returns `InvalidHandle`.

The first terminal decision under the owning worker lock wins. Cancellation may win while the OS has already transferred bytes, and a deadline may expire before a queued native completion is processed. A canceled send can have wire effects. The returned byte count describes known native progress, not a rollback guarantee. Protocols must account for this. A native cancellation completion is drained before operation storage or payloads are reused.

Options use monotonic relative timeouts: `-2` selects the context default, `-1` means infinite, and finite nonnegative values establish a deadline from submission. `SetDeadline(operation, timeout)` changes or removes a deadline and wakes the owning worker. An indexed min-heap schedules the next wake; idle workers do not periodically poll. Expiration requests safe cancellation and returns `TimedOut` after native buffer access ends. Application completion-queue delay does not change an already committed result.

Errors extend Milestone 1 with `Cancelled`, `Backpressure`, `InvalidHandle`, `ShuttingDown` and `NotRunning`. Existing codes retain their numeric values. `m_iNativeCode` and `m_Domain` preserve diagnostic provenance, including resolver errors and the Win32 IOCP `NativeCompletion` domain. Expected failures return values without forced logging. A worker wait/interest failure stops admission and requests shutdown across shards; callers should call `Shutdown()` to join and drain after observing Stopping.

## Native execution and synchronization

Each socket has affinity to one configured worker. Independent shard locks protect operation state; short pool/queue locks reserve capacity and deliver records. Native waits, resolver calls and thread joins run without shard locks. No global lock encloses independent socket I/O. Intrusive ready queues rotate sockets and unlink closing registrations immediately; native control wakeups are coalesced. Alternating direction priority, per-socket attempt/byte/time budgets and a per-iteration socket budget prevent one ready connection from monopolizing the worker. Budget exhaustion reschedules work; would-block waits for a new native notification.

- **Windows:** a bounded number of IOCP affinity shards, one port per worker and many sockets per port. `CreateIoCompletionPort`, batched `GetQueuedCompletionStatus` drains and `PostQueuedCompletionStatus` provide association, completion and wakeup. Operations use overlapped WSA transfers, provider-cached `AcceptEx`/`ConnectEx`, pre-created accept sockets and the required `SO_UPDATE_*_CONTEXT` updates. Immediate success still waits for its IOCP packet. `CancelIoEx` requests cancellation; a provider cancellation failure aborts the socket while retaining storage and the worker's Winsock lease until packets drain. Ports close after workers join. Native extension failures produce ordinary results. A call that fails without initiating an overlapped request publishes its error; it never waits for a completion packet that will not arrive. [Microsoft's overlapped receive contract](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-WSARecvFrom) and [cancellation contract](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex) define these native lifetimes.
- **Linux:** `epoll_create1`, `EPOLLONESHOT`, explicit read/write interests and `eventfd` wakeup. Interest exists only while queued operations need it and is rearmed after processing. Error/hangup notifications enter the same nonblocking operation path to retrieve precise errors or drain data before EOF. Event descriptors use close-on-exec.
- **macOS:** `kqueue` with `EV_DISPATCH` read/write filters, explicit reenabling/deletion and `EVFILT_USER` wakeup. `EV_EOF` still allows reading queued data. `EV_ERROR` is checked. Queue descriptors use close-on-exec; socket creation inherits Milestone 1's BSD descriptor/fork restrictions.
- **Shared POSIX:** operation execution reuses Milestone 1's `accept4`/BSD acceptance, vector transfers, SIGPIPE suppression, connection completion and centralized native error translation. No duplicate socket implementation is introduced.

Native thread creation returns recoverable initialization errors and deterministic joins. Foundation mutexes, condition variables, containers, allocators and `xiiTime` support the runtime. Native workers optionally register with Foundation profiling and emit a ready-processing scope, without importing `xiiThread`'s Communication event dependency. Foundation's existing CMake file collection and platform guards select these files; no new library or additional OS linkage is needed beyond the native networking libraries.

## Asynchronous DNS

`Resolve(host, service, ...)` copies bounded query text and submits Milestone 1's OS resolver to the separate bounded resolver executor. It returns multiple unique endpoints in OS preference order with explicit family and transport filtering. Empty host selects wildcard addresses; empty service selects port zero. Host/service text is bounded to 1024/256 bytes and rejects embedded NUL characters.

Canceling or timing out DNS logically abandons the result. An OS `getaddrinfo` already executing may continue; its executor pin prevents slot reuse, and it never accesses a delivered record. Queued abandoned work is skipped. Context shutdown joins outstanding resolver calls rather than detaching them, so it cannot promise a bounded shutdown duration if the host resolver stalls. I/O workers remain available while DNS blocks.

## Examples

The following complete functions compile together against `NetworkIOContext.h`. The TCP example places both ends in one context for a self-contained loopback exchange. A service normally keeps its listener, replenishes accepts and routes results by operation handle or user data. Select `Ipv6` for an IPv6-only loopback exchange.

```cpp
#include <Foundation/Network/NetworkIOContext.h>

#include <utility>

xiiNetworkError AsyncTcpExchange(xiiNetworkAddressFamily family)
{
  xiiNetworkIOContext       context;
  xiiNetworkIOConfiguration configuration;
  configuration.m_uiWorkerThreadCount = 2;
  configuration.m_DefaultTimeout      = xiiTime::MakeFromSeconds(5);
  auto error                          = context.Start(configuration);
  if (error.Failed())
  {
    return error;
  }

  xiiTcpListener listener;
  error = listener.Open(family);
  if (error.Failed())
  {
    return error;
  }
  error = listener.Bind({xiiNetworkAddress::MakeLoopback(family), 0});
  if (error.Failed())
  {
    return error;
  }
  error = listener.Listen();
  if (error.Failed())
  {
    return error;
  }
  xiiNetworkEndpoint endpoint;
  error = listener.GetLocalEndpoint(endpoint);
  if (error.Failed())
  {
    return error;
  }
  xiiNetworkIORegistration listening;
  error = context.Register(std::move(listener), listening);
  if (error.Failed())
  {
    return error;
  }

  xiiTcpSocket clientSocket;
  error = clientSocket.Open(family);
  if (error.Failed())
  {
    return error;
  }
  error = clientSocket.SetNoDelay(true);
  if (error.Failed())
  {
    return error;
  }
  xiiNetworkIORegistration client;
  error = context.Register(std::move(clientSocket), client);
  if (error.Failed())
  {
    return error;
  }
  xiiNetworkIOOperation accept;
  xiiNetworkIOOperation connect;
  error = context.Accept(listening, accept);
  if (error.Failed())
  {
    return error;
  }
  error = context.Connect(client, endpoint, connect);
  if (error.Failed())
  {
    return error;
  }
  xiiNetworkIORegistration server;
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    error = context.WaitForCompletion(result, xiiTime::MakeFromSeconds(5));
    if (error.Failed())
    {
      return error;
    }
    if (result.Failed())
    {
      return result.m_Error;
    }
    if (result.m_Operation == accept)
    {
      error = context.Register(std::move(result.m_AcceptedSocket), server);
      if (error.Failed())
      {
        return error;
      }
    }
  }

  xiiNetworkIOBuffer sendBuffer;
  xiiNetworkIOBuffer receiveBuffer;
  error = context.AcquireBuffer(1, sendBuffer);
  if (error.Failed())
  {
    return error;
  }
  error = context.AcquireBuffer(1, receiveBuffer);
  if (error.Failed())
  {
    return error;
  }
  sendBuffer.GetBytes()[0] = 42;
  xiiNetworkIOBuffers send;
  xiiNetworkIOBuffers receive;
  send.PushBack(std::move(sendBuffer));
  receive.PushBack(std::move(receiveBuffer));
  xiiNetworkIOOperation sent;
  xiiNetworkIOOperation received;
  error = context.Receive(server, std::move(receive), received);
  if (error.Failed())
  {
    return error;
  }
  error = context.Send(client, std::move(send), sent);
  if (error.Failed())
  {
    return error;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < 2; ++uiIndex)
  {
    xiiNetworkIOResult result;
    error = context.WaitForCompletion(result, xiiTime::MakeFromSeconds(5));
    if (error.Failed())
    {
      return error;
    }
    // All mode completes the entire byte count. On disconnection/error the result
    // still owns the buffers and reports any bytes transferred before the failure.
    if (result.Failed())
    {
      return result.m_Error;
    }
    if (result.m_Operation == received && result.m_Buffers[0].GetBytes()[0] != 42)
    {
      return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
    }
  }
  context.Shutdown();
  return {};
}

xiiNetworkError AsyncDatagram(xiiNetworkIOContext&      context,
                              xiiNetworkIORegistration  socket,
                              const xiiNetworkEndpoint& destination,
                              xiiNetworkIOOperation&    out_operation)
{
  xiiNetworkIOBuffer buffer;
  auto               error = context.AcquireBuffer(3, buffer);
  if (error.Failed())
  {
    return error;
  }
  buffer.GetBytes()[0] = 1;
  buffer.GetBytes()[1] = 2;
  buffer.GetBytes()[2] = 3;
  xiiNetworkIOBuffers buffers;
  buffers.PushBack(std::move(buffer));
  // socket is a registered xiiUdpSocket. Keep/drain its completion like any other operation.
  return context.SendTo(socket, destination, std::move(buffers), out_operation);
}

xiiNetworkError ResolveLocalhost(xiiNetworkIOContext&   context,
                                 xiiNetworkIOOperation& out_operation)
{
  xiiNetworkIOOptions options;
  options.m_Timeout    = xiiTime::MakeFromSeconds(2);
  options.m_uiUserData = 100;
  return context.Resolve("localhost", "443", out_operation,
                         xiiNetworkAddressFamily::Unspecified, xiiNetworkTransport::Tcp, options);
}

xiiNetworkError CancelOrShorten(xiiNetworkIOContext&  context,
                                xiiNetworkIOOperation operation, bool bCancel)
{
  return bCancel ? context.Cancel(operation) :
                   context.SetDeadline(operation, xiiTime::MakeFromMilliseconds(20));
}
```

For caller-controlled dispatch, poll `TryGetCompletion()` from the engine update/job that owns the application state. Apply a per-update result budget there. A `Some` completion advances the protocol cursor by `m_uiBytesTransferred`; select the remaining range/count before submitting another lease. Do not resubmit the already transferred prefix. An `All` result with EOF/error must likewise process any partial bytes before deciding how to close the application protocol. Maintain bounded application queues when submission returns `Backpressure`.

## Validation and measured workloads

`FoundationTest -noGui -all -noSave -assert 0 -filter Network` selects native networking, pool and asynchronous tests. They use loopback, ephemeral ports and bounded completion waits; unavailable IPv6 is reported explicitly. Concurrent producers, cancellation/completion races, deadline-heap changes, FIFO stream payloads, cross-context accepted ownership, generation reuse, pending shutdown, churn and capacity rejection are covered.

Stress intensity is configurable: `-networkStressConnections` (default 64, maximum 4096), `-networkBenchmarkMessages` (default 1000, maximum 100000), and `-networkStreamBytes` (default 262144, maximum 16777216). Connection tests report a resource-limit skip when the requested client socket count cannot be allocated. Each successful connection run verifies both ends and all transfer bytes before teardown. Benchmarks run in the same test harness and include submission/dispatch and payload validation overhead; they are not isolated kernel throughput measurements.

Statistics expose pending/retained/completed work, queue depths, byte counts, cancellation/timeouts/failures, native errors, backpressure, wakeups, completion-latency sum/max and pool allocation/reuse. Tracked internal memory covers primary runtime containers and allocated native operation/event state; it excludes allocator metadata, incidental query/vector allocations, kernel resources and thread stacks, and is not process RSS. Active/wait durations describe worker wall time. Native worker CPU time is collected on worker exit (zero before exit), excludes resolver/application CPU, and is available after `Shutdown()`. Snapshots are thread-safe, not globally transactional; latency is submission-to-terminal-commit, excluding application queue residence. These definitions matter when comparing workloads.

One Linux x86-64 **Clang Debug** run with two I/O workers completed 2000 concurrent loopback connections in 126.127 ms, then 230507 transfer operations/s for one 64-byte send/receive per connection. The runtime tracked 3985112 bytes of internal storage and 1024000 committed pool bytes; mean completion latency across connection and transfer operations was 32.529 microseconds. The worker lifetime used 85.570 ms of native CPU time. A separate 10000-message, 64-byte FIFO run measured 153388 messages/s (9.362 MiB/s), 33 pool backing allocations and 9968 reuses. An 8 MiB gather-send/receive with full byte validation measured 187.071 MiB/s. Socket-wide cancellation of 64 queued UDP receives took 9.385 microseconds to request and 57.470 microseconds through final dequeue. These are individual local observations with scheduling and Debug/harness overhead, not cross-platform guarantees or comparative performance claims.

Linux AddressSanitizer/LeakSanitizer validation passed. The combined ASan/UBSan run completed the networking assertions but reported existing zero-length null memory calls during Foundation allocator/container initialization. ThreadSanitizer completed the asynchronous assertions and reported existing task-system thread-status/shutdown races; it reported no networking-runtime races in that run. These full-harness UBSan/TSan runs are not clean. Configured Clang-Tidy analysis also reports include-cycle/include-cleaner and const-correctness diagnostics, including surrounding Foundation headers and macro expansions; no warning suppression is added.

Future secure transports, sessions and message protocols can consume this operation/lease model without borrowing descriptors or running application work inside native locks. Their protocol state, cancellation policy and dispatch scheduling remain above Foundation's raw I/O runtime.
