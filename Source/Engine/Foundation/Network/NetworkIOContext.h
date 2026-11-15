/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkIOConfiguration.h>
#include <Foundation/Network/NetworkIOResult.h>
#include <Foundation/Network/NetworkIOStatistics.h>
#include <Foundation/Network/NetworkResolver.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Network/UdpSocket.h>

class xiiNetworkIORuntime;

enum class xiiNetworkIOContextState : xiiUInt8
{
  Created,
  Running,
  Stopping,
  Stopped
};

/// Independently owned native asynchronous runtime; no global networking singleton or application callbacks.
/// Start and Shutdown are mutually serialized. Other methods may be called concurrently after Start;
/// destruction must not race calls on the context itself. A successfully stopped context is not restarted.
/// Register transfers exclusive socket ownership. The source becomes closed, and no synchronous view
/// of the registered resource is exposed. Deregister cancels/drains native I/O then closes the socket.
/// Socket moves/destruction outside the context therefore cannot invalidate native operations.
class XII_FOUNDATION_DLL xiiNetworkIOContext
{
public:
  xiiNetworkIOContext();
  ~xiiNetworkIOContext();
  xiiNetworkIOContext(xiiNetworkIOContext&&)                          = delete;
  xiiNetworkIOContext& operator=(xiiNetworkIOContext&&)               = delete;
  xiiNetworkIOContext(const xiiNetworkIOContext&)                     = delete;
  xiiNetworkIOContext&          operator=(const xiiNetworkIOContext&) = delete;
  [[nodiscard]] xiiNetworkError Start(const xiiNetworkIOConfiguration& configuration = {}, xiiAllocator* pAllocator = nullptr);
  /// Stops admission, cancels operations, drains native ownership, joins every worker and closes sockets.
  /// Completion records remain drainable. Blocking OS DNS queries must finish before resolver workers
  /// can be joined; cancellation/deadlines abandon their results without accessing delivered records.
  void                                   Shutdown();
  [[nodiscard]] xiiNetworkIOContextState GetState() const;
  [[nodiscard]] static xiiUInt64         GetActiveContextCount();

  /// Ownership is preserved on failure; registration may normalize a socket to nonblocking mode.
  /// Listeners must already be listening. TCP/UDP sockets must be open or connected, without a pending
  /// synchronous connect. Configure options and bind endpoints before transferring ownership.
  [[nodiscard]] xiiNetworkError Register(xiiTcpSocket&& socket, xiiNetworkIORegistration& out_registration);
  [[nodiscard]] xiiNetworkError Register(xiiTcpListener&& listener, xiiNetworkIORegistration& out_registration);
  [[nodiscard]] xiiNetworkError Register(xiiUdpSocket&& socket, xiiNetworkIORegistration& out_registration);
  /// Requests removal, rejecting new work immediately. Existing operations still complete exactly once.
  [[nodiscard]] xiiNetworkError Deregister(xiiNetworkIORegistration registration);
  [[nodiscard]] xiiNetworkError GetLocalEndpoint(xiiNetworkIORegistration registration, xiiNetworkEndpoint& out_endpoint) const;
  [[nodiscard]] xiiNetworkError GetPeerEndpoint(xiiNetworkIORegistration registration, xiiNetworkEndpoint& out_endpoint) const;

  [[nodiscard]] xiiNetworkError AcquireBuffer(xiiUInt32 uiCount, xiiNetworkIOBuffer& out_buffer);
  /// Submission success means accepted, never I/O completion. On rejection buffers remain with the caller.
  /// Each accepted operation reserves completion capacity and retains buffers through native completion.
  [[nodiscard]] xiiNetworkError Accept(xiiNetworkIORegistration listener, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError Connect(xiiNetworkIORegistration socket, const xiiNetworkEndpoint& endpoint, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError Send(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError Receive(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError SendTo(xiiNetworkIORegistration socket, const xiiNetworkEndpoint& destination, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError ReceiveFrom(xiiNetworkIORegistration socket, xiiNetworkIOBuffers&& buffers, xiiNetworkIOOperation& out_operation, const xiiNetworkIOOptions& options = {});
  [[nodiscard]] xiiNetworkError Resolve(xiiStringView sHost, xiiStringView sService, xiiNetworkIOOperation& out_operation, xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Unspecified, xiiNetworkTransport transport = xiiNetworkTransport::Tcp, const xiiNetworkIOOptions& options = {});

  /// Cancellation wins only if no terminal result was committed. Already terminal/stale returns InvalidHandle.
  [[nodiscard]] xiiNetworkError Cancel(xiiNetworkIOOperation operation);
  [[nodiscard]] xiiNetworkError Cancel(xiiNetworkIORegistration registration);
  /// -1 removes the deadline; otherwise a finite nonnegative relative timeout. Wakes an idle worker.
  [[nodiscard]] xiiNetworkError SetDeadline(xiiNetworkIOOperation operation, xiiTime timeout);
  [[nodiscard]] bool            TryGetCompletion(xiiNetworkIOResult& out_result);
  /// Zero polls. Output unchanged on TimedOut or NotRunning (stopped and drained).
  [[nodiscard]] xiiNetworkError        WaitForCompletion(xiiNetworkIOResult& out_result, xiiTime timeout);
  [[nodiscard]] xiiNetworkIOStatistics GetStatistics() const;

private:
  xiiNetworkIORuntime* m_pRuntime = nullptr;
};
