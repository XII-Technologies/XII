/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Network/Protocol/Implementation/NetworkReceiveState.h>

XII_FOUNDATION_INTERNAL_HEADER

class xiiNetworkIOContext;
class xiiThreadSignal;
struct xiiNetworkQuicConnection;
struct xiiNetworkQuicListener;
struct xiiNetworkQuicStream;
class xiiNetworkQuicState;

enum class xiiNetworkQuicEventType : xiiUInt8
{
  Incoming,
  Certificate,
  Connected,
  Receive,
  Datagram,
  Send,
  StreamClosed,
  Shutdown
};

struct xiiNetworkQuicEvent
{
  xiiNetworkQuicEventType                        m_Type        = xiiNetworkQuicEventType::Shutdown;
  xiiNetworkQuicConnection*                      m_pConnection = nullptr;
  xiiNetworkQuicStream*                          m_pStream     = nullptr;
  xiiNetworkListener                             m_Listener;
  xiiNetworkError                                m_Error;
  xiiNetworkFrameHeader                          m_FrameHeader;
  xiiNetworkIOBuffer                             m_Bytes;
  xiiNetworkIOBuffer                             m_Chain;
  xiiHybridArray<xiiArrayPtr<const xiiUInt8>, 4> m_Spans;
  void*                                          m_pSend          = nullptr;
  xiiUInt32                                      m_uiSpanIndex    = 0;
  xiiUInt64                                      m_uiReceiveBytes = 0;
  bool                                           m_bFin           = false;
};

/// Driver-owned native handles with a bounded callback queue. Close quiesces provider callbacks
/// and discards queued records before releasing their contexts. Receive spans are borrowed from
/// MsQuic until CompleteReceive or connection closure. Sends borrow through the terminal Send event.
class xiiNetworkQuic
{
public:
  xiiNetworkQuic() = default;
  ~xiiNetworkQuic();
  xiiNetworkQuic(const xiiNetworkQuic&)                    = delete;
  xiiNetworkQuic&         operator=(const xiiNetworkQuic&) = delete;
  xiiNetworkError         Start(const xiiNetworkHostConfiguration& configuration, xiiNetworkIOContext& io, xiiThreadSignal& signal);
  xiiNetworkError         Listen(const xiiNetworkEndpoint& endpoint, xiiNetworkListener owner, xiiNetworkQuicListener*& out_pListener, xiiNetworkEndpoint& out_endpoint);
  void                    CloseListener(xiiNetworkQuicListener* pListener);
  xiiNetworkError         Connect(const xiiNetworkEndpoint& endpoint, xiiStringView sServerName, xiiSharedPtr<xiiNetworkCredentials> pCredentials, xiiNetworkConnection owner, xiiNetworkQuicConnection*& out_pConnection);
  xiiNetworkError         Accept(xiiNetworkQuicConnection& connection, xiiSharedPtr<xiiNetworkCredentials> pCredentials, xiiNetworkConnection owner);
  xiiNetworkConnection    GetOwner(const xiiNetworkQuicConnection& connection) const;
  xiiNetworkError         GetFailure(const xiiNetworkQuicConnection& connection) const;
  xiiNetworkError         VerifyCertificate(xiiNetworkQuicConnection& connection, const xiiNetworkQuicEvent& event, xiiNetworkCertificate& out_certificate);
  xiiNetworkError         GetEndpoints(xiiNetworkQuicConnection& connection, xiiNetworkEndpoint& out_local, xiiNetworkEndpoint& out_remote);
  bool                    Poll(xiiNetworkQuicEvent& out_event);
  xiiUInt32               GetMaximumDatagramBytes(const xiiNetworkQuicConnection& connection) const;
  void                    UpdateStatistics(const xiiNetworkQuicConnection& connection, xiiNetworkConnectionStatistics& ref_statistics);
  xiiNetworkError         Send(xiiNetworkQuicConnection& connection, xiiUInt32 uiChannel, xiiNetworkDeliveryMode mode, xiiArrayPtr<const xiiUInt8> header, xiiArrayPtr<const xiiUInt8> payload, void* pSend);
  void                    FinishChannel(xiiNetworkQuicConnection& connection, xiiUInt32 uiChannel);
  xiiNetworkReceiveState& GetReceiveState(xiiNetworkQuicStream& stream);
  void                    CompleteReceive(xiiNetworkQuicStream& stream, xiiUInt64 uiBytes);
  void                    CloseStream(xiiNetworkQuicStream* pStream, xiiUInt64& ref_uiBytes, xiiUInt32& ref_uiEvents);
  void                    Shutdown(xiiNetworkQuicConnection& connection);
  void                    Close(xiiNetworkQuicConnection* pConnection, xiiUInt64& ref_uiBytes, xiiUInt32& ref_uiEvents);

private:
  xiiNetworkQuicState* m_pState = nullptr;
};

/// Select up to two provider processor indices supported by the current native platform.
xiiUInt32 xiiNetworkQuicProcessors(xiiUInt16 (&out_uiProcessors)[2]);
