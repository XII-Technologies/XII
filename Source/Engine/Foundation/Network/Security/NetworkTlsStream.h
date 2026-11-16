/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkError.h>
#include <Foundation/Network/Security/NetworkCredentials.h>
#include <Foundation/Types/SharedPtr.h>

class xiiNetworkTlsStreamState;

/// Bounded TLS 1.3 record engine, independent of socket scheduling. One serialized owner drives all
/// methods. FeedCiphertext and DrainCiphertext connect it to native asynchronous buffer operations;
/// neither performs socket I/O. WouldBlock requires feeding input or draining output, never spinning.
/// Plaintext operations may partially transfer. Empty buffers succeed with zero bytes. A received
/// close_notify reports ConnectionClosed; raw EOF without close_notify reports TlsError.
class XII_FOUNDATION_DLL xiiNetworkTlsStream final
{
public:
  xiiNetworkTlsStream();
  ~xiiNetworkTlsStream();
  xiiNetworkTlsStream(const xiiNetworkTlsStream&)            = delete;
  xiiNetworkTlsStream& operator=(const xiiNetworkTlsStream&) = delete;
  xiiNetworkTlsStream(xiiNetworkTlsStream&&)                 = delete;
  xiiNetworkTlsStream& operator=(xiiNetworkTlsStream&&)      = delete;

  /// Client serverName is mandatory and validated against SANs. Origin port scopes the optional
  /// credential snapshot's bounded session cache. Servers require a loaded certificate/key identity.
  [[nodiscard]] xiiNetworkError           Initialize(xiiSharedPtr<xiiNetworkCredentials> pCredentials, bool bServer, xiiStringView sServerName = {}, xiiUInt16 uiOriginPort = 0);
  [[nodiscard]] xiiNetworkError           Handshake();
  [[nodiscard]] xiiNetworkOperationResult FeedCiphertext(xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiNetworkOperationResult DrainCiphertext(xiiArrayPtr<xiiUInt8> bytes);
  /// On WouldBlock, retry the same remaining plaintext before starting another write. Retain its
  /// contents until accepted; the stream owns no application send queue.
  [[nodiscard]] xiiNetworkOperationResult Write(xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiNetworkOperationResult Read(xiiArrayPtr<xiiUInt8> bytes);
  /// Signal native EOF only after feeding all previously received ciphertext.
  void NotifyEndOfInput();
  /// Send close_notify and wait for the peer's close_notify. WouldBlock still requires output drain.
  [[nodiscard]] xiiNetworkError Shutdown();
  [[nodiscard]] bool            IsEstablished() const;
  [[nodiscard]] bool            WasSessionResumed() const;
  [[nodiscard]] xiiNetworkError GetPeerCertificate(xiiNetworkCertificate& out_certificate) const;
  [[nodiscard]] xiiUInt32       GetPendingCiphertextBytes() const;

private:
  xiiNetworkTlsStreamState* m_pState = nullptr;
};
