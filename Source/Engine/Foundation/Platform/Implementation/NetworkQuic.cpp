/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Network/Transport/Implementation/NetworkQuic.h>
#include <Foundation/Platform/Implementation/NetworkPlatform.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <openssl/err.h>
#include <openssl/pkcs7.h>

#define QUIC_API_ENABLE_PREVIEW_FEATURES
#include <msquic.h>
#undef QUIC_API_ENABLE_PREVIEW_FEATURES

#include <atomic>
#include <cstring>
#include <utility>

namespace
{
  constexpr xiiUInt32 QuicUnorderedLimit     = 16;
  constexpr xiiUInt32 QuicDatagramQueueLimit = 256;
  constexpr xiiUInt64 QuicApplicationError   = 0x584949;
  constexpr auto      QuicClosed             = xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed);
  constexpr auto      QuicPressure           = xiiNetworkError(xiiNetworkErrorCode::Backpressure);
  constexpr auto      QuicViolation          = xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation);

  xiiNetworkError MapQuicError(QUIC_STATUS status)
  {
    if (QUIC_SUCCEEDED(status))
    {
      return {};
    }
    auto code = xiiNetworkErrorCode::Unknown;
    if (status == QUIC_STATUS_CONNECTION_TIMEOUT || status == QUIC_STATUS_CONNECTION_IDLE)
    {
      code = xiiNetworkErrorCode::TimedOut;
    }
    else if (status == QUIC_STATUS_CONNECTION_REFUSED)
    {
      code = xiiNetworkErrorCode::ConnectionRefused;
    }
    else if (status == QUIC_STATUS_ABORTED)
    {
      code = xiiNetworkErrorCode::ConnectionAborted;
    }
    else if (status == QUIC_STATUS_UNREACHABLE)
    {
      code = xiiNetworkErrorCode::HostUnreachable;
    }
    else if (status == QUIC_STATUS_OUT_OF_MEMORY)
    {
      code = xiiNetworkErrorCode::ResourceExhausted;
    }
    else if (status == QUIC_STATUS_STREAM_LIMIT_REACHED)
    {
      code = xiiNetworkErrorCode::WouldBlock;
    }
    else if (status == QUIC_STATUS_INVALID_PARAMETER)
    {
      code = xiiNetworkErrorCode::InvalidArgument;
    }
    else if (status == QUIC_STATUS_INVALID_ADDRESS)
    {
      code = xiiNetworkErrorCode::InvalidAddress;
    }
    else if (status == QUIC_STATUS_ADDRESS_IN_USE)
    {
      code = xiiNetworkErrorCode::AddressInUse;
    }
    else if (status == QUIC_STATUS_NOT_SUPPORTED)
    {
      code = xiiNetworkErrorCode::UnsupportedCapability;
    }
    else if (status == QUIC_STATUS_BUFFER_TOO_SMALL)
    {
      code = xiiNetworkErrorCode::MessageTooLarge;
    }
    else if (status == QUIC_STATUS_ALPN_NEG_FAILURE)
    {
      code = xiiNetworkErrorCode::ProtocolMismatch;
    }
    else if (status == QUIC_STATUS_PROTOCOL_ERROR)
    {
      code = xiiNetworkErrorCode::ProtocolViolation;
    }
    else if (status == QUIC_STATUS_CERT_EXPIRED)
    {
      code = xiiNetworkErrorCode::CertificateExpired;
    }
    else if (status == QUIC_STATUS_BAD_CERTIFICATE || status == QUIC_STATUS_CERT_UNTRUSTED_ROOT || status == QUIC_STATUS_CERT_NO_CERT)
    {
      code = xiiNetworkErrorCode::CertificateUntrusted;
    }
    else if (status == QUIC_STATUS_HANDSHAKE_FAILURE)
    {
      code = xiiNetworkErrorCode::TlsError;
    }
    return xiiNetworkError(code, static_cast<xiiInt32>(status), xiiNetworkErrorDomain::Quic);
  }

  xiiMutex              s_QuicProviderMutex;
  const QUIC_API_TABLE* s_pQuicApi         = nullptr;
  xiiUInt32             s_uiQuicReferences = 0;

  xiiNetworkError AcquireQuicProvider(const QUIC_API_TABLE*& out_pApi)
  {
    XII_LOCK(s_QuicProviderMutex);
    if (s_uiQuicReferences == 0)
    {
      auto error = MapQuicError(MsQuicOpen2(&s_pQuicApi));
      if (error.Failed())
      {
        return error;
      }
      xiiUInt32 uiVersion[4]   = {};
      xiiUInt32 uiVersionBytes = sizeof(uiVersion);
      error                    = MapQuicError(s_pQuicApi->GetParam(nullptr, QUIC_PARAM_GLOBAL_LIBRARY_VERSION, &uiVersionBytes, uiVersion));
      if (error.Succeeded() && (uiVersion[0] != 2 || uiVersion[1] != 6 || uiVersion[2] != 4))
      {
        error = xiiNetworkError(xiiNetworkErrorCode::InitializationFailed);
      }
      if (error.Succeeded())
      {
        struct ExecutionConfiguration
        {
          QUIC_GLOBAL_EXECUTION_CONFIG_FLAGS m_Flags                = QUIC_GLOBAL_EXECUTION_CONFIG_FLAG_NO_IDEAL_PROC;
          xiiUInt32                          m_uiPollingIdleTimeout = 0;
          xiiUInt32                          m_uiProcessorCount     = 0;
          xiiUInt16                          m_uiProcessors[2]      = {};
        } configuration;
        static_assert(offsetof(ExecutionConfiguration, m_uiProcessors) == QUIC_GLOBAL_EXECUTION_CONFIG_MIN_SIZE);
        configuration.m_uiProcessorCount = xiiNetworkQuicProcessors(configuration.m_uiProcessors);
        error                            = configuration.m_uiProcessorCount == 0 ? xiiNetworkError(xiiNetworkErrorCode::InitializationFailed) :
                                                                                   MapQuicError(s_pQuicApi->SetParam(nullptr, QUIC_PARAM_GLOBAL_EXECUTION_CONFIG, sizeof(configuration), &configuration));
      }
      if (error.Failed())
      {
        MsQuicClose(s_pQuicApi);
        s_pQuicApi = nullptr;
        return error;
      }
    }
    ++s_uiQuicReferences;
    out_pApi = s_pQuicApi;
    return {};
  }

  void ReleaseQuicProvider()
  {
    XII_LOCK(s_QuicProviderMutex);
    if (--s_uiQuicReferences == 0)
    {
      MsQuicClose(s_pQuicApi);
      s_pQuicApi = nullptr;
    }
  }

  struct QuicConfiguration : xiiRefCountingImpl
  {
    ~QuicConfiguration()
    {
      if (m_Handle != nullptr)
      {
        m_pApi->ConfigurationClose(m_Handle);
      }
    }
    const QUIC_API_TABLE*               m_pApi   = nullptr;
    HQUIC                               m_Handle = nullptr;
    xiiSharedPtr<xiiNetworkCredentials> m_pCredentials;
  };

  struct QuicSend
  {
    QUIC_BUFFER m_Buffers[2] = {};
    void*       m_pOwner     = nullptr;
    bool        m_bReserved  = false;
  };
} // namespace

struct xiiNetworkQuicStream
{
  xiiNetworkQuicConnection* m_pConnection = nullptr;
  HQUIC                     m_Handle      = nullptr;
  xiiNetworkReceiveState    m_Receive;
  xiiUInt32                 m_uiChannel  = 0;
  bool                      m_bIncoming  = false;
  bool                      m_bControl   = false;
  bool                      m_bUnordered = false;
};

struct xiiNetworkQuicConnection
{
  xiiNetworkQuicState*                         m_pState = nullptr;
  const QUIC_API_TABLE*                        m_pApi   = nullptr;
  HQUIC                                        m_Handle = nullptr;
  xiiNetworkConnection                         m_Owner;
  xiiSharedPtr<QuicConfiguration>              m_pConfiguration;
  xiiString                                    m_sServerName;
  xiiMutex                                     m_StreamsMutex;
  xiiDeque<xiiUniquePtr<xiiNetworkQuicStream>> m_Streams;
  xiiDeque<QuicSend>                           m_Sends;
  xiiNetworkQuicStream*                        m_pControl = nullptr;
  std::atomic<xiiUInt32>                       m_uiDatagramBytes{0};
  std::atomic<xiiUInt64>                       m_uiDropped{0};
  std::atomic<bool>                            m_bQueueFailed{false};
  std::atomic<bool>                            m_bQuiescing{false};
  bool                                         m_bServer            = false;
  bool                                         m_bShutdownRequested = false;
  bool                                         m_bValidated         = false;
};

struct xiiNetworkQuicListener
{
  xiiNetworkQuicState* m_pState = nullptr;
  HQUIC                m_Handle = nullptr;
  xiiNetworkListener   m_Owner;
};

class xiiNetworkQuicState
{
public:
  ~xiiNetworkQuicState()
  {
    m_pClient.Clear();
    m_pServer.Clear();
    if (m_Registration != nullptr)
    {
      m_pApi->RegistrationClose(m_Registration);
    }
    if (m_pApi != nullptr)
    {
      ReleaseQuicProvider();
    }
  }

  bool Push(xiiNetworkQuicEvent&& event)
  {
    auto& connection = *event.m_pConnection;
    if (connection.m_bQuiescing.load())
    {
      return false;
    }
    {
      XII_LOCK(m_QueueMutex);
      if (connection.m_bQuiescing.load())
      {
        return false;
      }
      if (event.m_Type == xiiNetworkQuicEventType::Datagram && m_uiDatagrams >= QuicDatagramQueueLimit)
      {
        ++connection.m_uiDropped;
        return false;
      }
      if (m_Events.GetCount() >= m_uiQueueLimit)
      {
        connection.m_bQueueFailed.store(true);
        m_pSignal->RaiseSignal();
        return false;
      }
      if (event.m_Type == xiiNetworkQuicEventType::Datagram)
      {
        ++m_uiDatagrams;
      }
      m_Events.PushBack(std::move(event));
    }
    m_pSignal->RaiseSignal();
    return true;
  }

  void Destroy(xiiNetworkQuicConnection* pConnection)
  {
    XII_DEFAULT_DELETE(pConnection);
    --m_uiConnections;
  }

  xiiNetworkQuicConnection* Create(bool bServer)
  {
    auto uiCount = m_uiConnections.load();
    do
    {
      if (uiCount >= m_uiConnectionLimit)
      {
        return nullptr;
      }
    } while (!m_uiConnections.compare_exchange_weak(uiCount, uiCount + 1));
    xiiNetworkQuicConnection* pConnection = XII_DEFAULT_NEW(xiiNetworkQuicConnection);
    pConnection->m_pState                 = this;
    pConnection->m_pApi                   = m_pApi;
    pConnection->m_bServer                = bServer;
    return pConnection;
  }

  xiiNetworkError             Configuration(xiiSharedPtr<xiiNetworkCredentials> pCredentials, bool bServer, xiiSharedPtr<QuicConfiguration>& out_pConfiguration);
  static QUIC_STATUS QUIC_API ListenerCallback(HQUIC, void* pContext, QUIC_LISTENER_EVENT* pEvent);
  static QUIC_STATUS QUIC_API ConnectionCallback(HQUIC, void* pContext, QUIC_CONNECTION_EVENT* pEvent);
  static QUIC_STATUS QUIC_API StreamCallback(HQUIC, void* pContext, QUIC_STREAM_EVENT* pEvent);

  const QUIC_API_TABLE*           m_pApi         = nullptr;
  HQUIC                           m_Registration = nullptr;
  xiiNetworkIOContext*            m_pIO          = nullptr;
  xiiThreadSignal*                m_pSignal      = nullptr;
  xiiString                       m_sAlpn;
  xiiMutex                        m_QueueMutex;
  xiiDeque<xiiNetworkQuicEvent>   m_Events;
  xiiSharedPtr<QuicConfiguration> m_pClient;
  xiiSharedPtr<QuicConfiguration> m_pServer;
  std::atomic<xiiUInt32>          m_uiConnections{0};
  xiiUInt32                       m_uiConnectionLimit       = 0;
  xiiUInt32                       m_uiQueueLimit            = 0;
  xiiUInt32                       m_uiStreamLimit           = 0;
  xiiUInt32                       m_uiSendLimit             = 0;
  xiiUInt32                       m_uiDatagrams             = 0;
  xiiUInt32                       m_uiFlowWindow            = 0;
  xiiUInt64                       m_uiHandshakeMilliseconds = 0;
  xiiUInt64                       m_uiIdleMilliseconds      = 0;
};

xiiNetworkError xiiNetworkQuicState::Configuration(xiiSharedPtr<xiiNetworkCredentials> pCredentials, bool bServer, xiiSharedPtr<QuicConfiguration>& out_pConfiguration)
{
  auto& pCached = bServer ? m_pServer : m_pClient;
  if (pCached != nullptr && pCached->m_pCredentials == pCredentials)
  {
    out_pConfiguration = pCached;
    return {};
  }
  const auto& security = pCredentials->GetConfiguration();
  if (security.m_sApplicationProtocol != m_sAlpn || security.m_bEnableTransportResumption || (bServer && !pCredentials->HasIdentity()))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiSharedPtr<QuicConfiguration> pConfiguration = XII_DEFAULT_NEW(QuicConfiguration);
  pConfiguration->m_pApi                         = m_pApi;
  pConfiguration->m_pCredentials                 = pCredentials;
  QUIC_SETTINGS settings                         = {};
  settings.IsSet.PeerBidiStreamCount             = true;
  settings.PeerBidiStreamCount                   = 1;
  settings.IsSet.PeerUnidiStreamCount            = true;
  settings.PeerUnidiStreamCount                  = static_cast<uint16_t>(m_uiStreamLimit);
  settings.IsSet.DatagramReceiveEnabled          = true;
  settings.DatagramReceiveEnabled                = true;
  settings.IsSet.SendBufferingEnabled            = true;
  settings.SendBufferingEnabled                  = false;
  settings.IsSet.PacingEnabled                   = true;
  settings.PacingEnabled                         = true;
  settings.IsSet.MigrationEnabled                = true;
  settings.MigrationEnabled                      = false;
  settings.IsSet.ServerResumptionLevel           = true;
  settings.ServerResumptionLevel                 = QUIC_SERVER_NO_RESUME;
  settings.IsSet.StreamRecvWindowDefault         = true;
  settings.StreamRecvWindowDefault               = 65536;
  settings.IsSet.ConnFlowControlWindow           = true;
  settings.ConnFlowControlWindow                 = m_uiFlowWindow;
  settings.IsSet.HandshakeIdleTimeoutMs          = true;
  settings.HandshakeIdleTimeoutMs                = m_uiHandshakeMilliseconds;
  settings.IsSet.IdleTimeoutMs                   = true;
  settings.IdleTimeoutMs                         = m_uiIdleMilliseconds;
  QUIC_BUFFER alpn{m_sAlpn.GetElementCount(), reinterpret_cast<xiiUInt8*>(const_cast<char*>(m_sAlpn.GetData()))};
  auto        error = MapQuicError(m_pApi->ConfigurationOpen(m_Registration, &alpn, 1, &settings, sizeof(settings), nullptr, &pConfiguration->m_Handle));
  if (error.Failed())
  {
    return error;
  }

  const auto&             identity = xiiNetworkSecurityAccess::Get(*pCredentials)->m_Pkcs12;
  QUIC_CERTIFICATE_PKCS12 pkcs12{identity.GetData(), identity.GetCount(), ""};
  QUIC_CREDENTIAL_CONFIG  credentials = {};
  credentials.Type                    = identity.IsEmpty() ? QUIC_CREDENTIAL_TYPE_NONE : QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
  credentials.CertificatePkcs12       = identity.IsEmpty() ? nullptr : &pkcs12;
  credentials.Flags                   = QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED | QUIC_CREDENTIAL_FLAG_DEFER_CERTIFICATE_VALIDATION | QUIC_CREDENTIAL_FLAG_USE_PORTABLE_CERTIFICATES;
  if (!bServer)
  {
    credentials.Flags |= QUIC_CREDENTIAL_FLAG_CLIENT;
  }
  else if (security.m_bRequireClientCertificate)
  {
    credentials.Flags |= QUIC_CREDENTIAL_FLAG_REQUIRE_CLIENT_AUTHENTICATION;
  }
  error = MapQuicError(m_pApi->ConfigurationLoadCredential(pConfiguration->m_Handle, &credentials));
  if (error.Succeeded())
  {
    pCached            = pConfiguration;
    out_pConfiguration = pConfiguration;
  }
  return error;
}

QUIC_STATUS QUIC_API xiiNetworkQuicState::ListenerCallback(HQUIC, void* pContext, QUIC_LISTENER_EVENT* pEvent)
{
  auto& listener = *static_cast<xiiNetworkQuicListener*>(pContext);
  auto& state    = *listener.m_pState;
  if (pEvent->Type != QUIC_LISTENER_EVENT_NEW_CONNECTION)
  {
    return QUIC_STATUS_SUCCESS;
  }
  auto* pConnection = state.Create(true);
  if (pConnection == nullptr)
  {
    return QUIC_STATUS_CONNECTION_REFUSED;
  }
  pConnection->m_Handle = pEvent->NEW_CONNECTION.Connection;
  state.m_pApi->SetCallbackHandler(pConnection->m_Handle, reinterpret_cast<void*>(ConnectionCallback), pConnection);
  xiiNetworkQuicEvent event;
  event.m_Type        = xiiNetworkQuicEventType::Incoming;
  event.m_pConnection = pConnection;
  event.m_Listener    = listener.m_Owner;
  if (!state.Push(std::move(event)))
  {
    state.m_pApi->SetCallbackHandler(pConnection->m_Handle, reinterpret_cast<void*>(ConnectionCallback), nullptr);
    state.Destroy(pConnection);
    return QUIC_STATUS_CONNECTION_REFUSED;
  }
  return QUIC_STATUS_SUCCESS;
}

namespace
{
  bool CopyQuicBytes(xiiNetworkQuicState& state, const QUIC_BUFFER* pInput, xiiUInt32 uiLimit, xiiNetworkIOBuffer& out_buffer)
  {
    if (pInput == nullptr || pInput->Length == 0)
    {
      return true;
    }
    if (pInput->Length > uiLimit || state.m_pIO->AcquireBuffer(pInput->Length, out_buffer).Failed())
    {
      return false;
    }
    std::memcpy(out_buffer.GetBytes().GetPtr(), pInput->Buffer, pInput->Length);
    return true;
  }

  xiiNetworkQuicStream* AddQuicStream(xiiNetworkQuicConnection& connection, HQUIC hStream, bool bIncoming, bool bControl, bool bUnordered)
  {
    auto& state = *connection.m_pState;
    XII_LOCK(connection.m_StreamsMutex);
    if (bControl && connection.m_pControl != nullptr)
    {
      return nullptr;
    }
    if (connection.m_Streams.GetCount() >= 2 * state.m_uiStreamLimit + 1)
    {
      return nullptr;
    }
    if (bUnordered)
    {
      xiiUInt32 uiUnordered = 0;
      for (const auto& pExisting : connection.m_Streams)
      {
        uiUnordered += pExisting->m_bUnordered ? 1U : 0U;
      }
      if (uiUnordered >= QuicUnorderedLimit)
      {
        return nullptr;
      }
    }
    auto pStream                               = xiiUniquePtr<xiiNetworkQuicStream>(XII_DEFAULT_NEW(xiiNetworkQuicStream));
    pStream->m_pConnection                     = &connection;
    pStream->m_Handle                          = hStream;
    pStream->m_bIncoming                       = bIncoming;
    pStream->m_bControl                        = bControl;
    pStream->m_bUnordered                      = bUnordered;
    pStream->m_Receive.m_bRequireControlFrames = bControl;
    pStream->m_Receive.m_bRequireDataFrames    = !bControl;
    auto* pResult                              = pStream.Borrow();
    connection.m_Streams.PushBack(std::move(pStream));
    if (bControl)
    {
      connection.m_pControl = pResult;
    }
    return pResult;
  }
} // namespace

QUIC_STATUS QUIC_API xiiNetworkQuicState::ConnectionCallback(HQUIC, void* pContext, QUIC_CONNECTION_EVENT* pEvent)
{
  auto&               connection = *static_cast<xiiNetworkQuicConnection*>(pContext);
  auto&               state      = *connection.m_pState;
  xiiNetworkQuicEvent event;
  event.m_pConnection = &connection;
  switch (pEvent->Type)
  {
    case QUIC_CONNECTION_EVENT_CONNECTED:
      event.m_Type = xiiNetworkQuicEventType::Connected;
      break;
    case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
    {
      const auto& certificate = pEvent->PEER_CERTIFICATE_RECEIVED;
      event.m_Type            = xiiNetworkQuicEventType::Certificate;
      const auto uiLimit      = connection.m_pConfiguration->m_pCredentials->GetConfiguration().m_uiMaximumCertificateBytes;
      if (!CopyQuicBytes(state, static_cast<const QUIC_BUFFER*>(certificate.Certificate), uiLimit, event.m_Bytes) ||
          !CopyQuicBytes(state, static_cast<const QUIC_BUFFER*>(certificate.Chain), uiLimit + 4096, event.m_Chain))
      {
        event.m_Error = xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
      }
      return state.Push(std::move(event)) ? QUIC_STATUS_PENDING : QUIC_STATUS_BAD_CERTIFICATE;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
      event.m_Error = MapQuicError(pEvent->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
      break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
      event.m_Error = QuicClosed;
      break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
      event.m_bFin = true;
      break;
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
    {
      const bool bControl = (pEvent->PEER_STREAM_STARTED.Flags & QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL) == 0;
      const auto hStream  = pEvent->PEER_STREAM_STARTED.Stream;
      auto*      pStream  = (!bControl || connection.m_bServer) ? AddQuicStream(connection, hStream, true, bControl, false) : nullptr;
      if (pStream == nullptr)
      {
        connection.m_bQueueFailed.store(true);
        state.m_pSignal->RaiseSignal();
        connection.m_pApi->SetCallbackHandler(hStream, reinterpret_cast<void*>(StreamCallback), nullptr);
        connection.m_pApi->StreamClose(hStream);
      }
      else
      {
        state.m_pApi->SetCallbackHandler(hStream, reinterpret_cast<void*>(StreamCallback), pStream);
      }
      return QUIC_STATUS_SUCCESS;
    }
    case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
      connection.m_uiDatagramBytes.store(pEvent->DATAGRAM_STATE_CHANGED.SendEnabled ? pEvent->DATAGRAM_STATE_CHANGED.MaxSendLength : 0);
      state.m_pSignal->RaiseSignal();
      return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE:
      state.m_pSignal->RaiseSignal();
      return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
    {
      event.m_Type                                = xiiNetworkQuicEventType::Datagram;
      const auto*                       pDatagram = pEvent->DATAGRAM_RECEIVED.Buffer;
      const xiiArrayPtr<const xiiUInt8> bytes(pDatagram->Buffer, pDatagram->Length);
      xiiNetworkFrameDecoder            decoder(65535);
      xiiUInt32                         uiHeaderBytes = 0;
      const auto                        parsed        = decoder.Consume(bytes, uiHeaderBytes);
      const auto                        decodeState   = decoder.GetState();
      if (parsed.Failed() || (decodeState != xiiNetworkFrameDecodeState::NeedBuffer && decodeState != xiiNetworkFrameDecodeState::Ready) ||
          decoder.GetHeader().m_Type != xiiNetworkFrameType::Data || decoder.GetHeader().m_uiPayloadBytes != bytes.GetCount() - uiHeaderBytes)
      {
        ++connection.m_uiDropped;
        return QUIC_STATUS_SUCCESS;
      }
      event.m_FrameHeader = decoder.GetHeader();
      const auto  payload = bytes.GetSubArray(uiHeaderBytes);
      QUIC_BUFFER nativePayload{payload.GetCount(), const_cast<xiiUInt8*>(payload.GetPtr())};
      if (!CopyQuicBytes(state, &nativePayload, 65535, event.m_Bytes))
      {
        ++connection.m_uiDropped;
        return QUIC_STATUS_SUCCESS;
      }
      break;
    }
    case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
    {
      const auto& completion = pEvent->DATAGRAM_SEND_STATE_CHANGED;
      if (!QUIC_DATAGRAM_SEND_STATE_IS_FINAL(completion.State))
      {
        return QUIC_STATUS_SUCCESS;
      }
      event.m_Type  = xiiNetworkQuicEventType::Send;
      event.m_pSend = completion.ClientContext;
      if (completion.State == QUIC_DATAGRAM_SEND_CANCELED)
      {
        event.m_Error = xiiNetworkError(xiiNetworkErrorCode::Cancelled);
      }
      if (completion.State == QUIC_DATAGRAM_SEND_LOST_DISCARDED)
      {
        event.m_Error = xiiNetworkError(xiiNetworkErrorCode::DatagramLost, static_cast<xiiInt32>(completion.State), xiiNetworkErrorDomain::Quic);
      }
      break;
    }
    default:
      return QUIC_STATUS_SUCCESS;
  }
  state.Push(std::move(event));
  return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS QUIC_API xiiNetworkQuicState::StreamCallback(HQUIC, void* pContext, QUIC_STREAM_EVENT* pEvent)
{
  if (pContext == nullptr)
  {
    return QUIC_STATUS_SUCCESS;
  }
  auto&               stream     = *static_cast<xiiNetworkQuicStream*>(pContext);
  auto&               connection = *stream.m_pConnection;
  auto&               state      = *connection.m_pState;
  xiiNetworkQuicEvent event;
  event.m_pConnection = &connection;
  event.m_pStream     = &stream;
  switch (pEvent->Type)
  {
    case QUIC_STREAM_EVENT_RECEIVE:
    {
      const auto& receive = pEvent->RECEIVE;
      if (receive.BufferCount > 64)
      {
        connection.m_bQueueFailed.store(true);
        state.m_pSignal->RaiseSignal();
        return QUIC_STATUS_SUCCESS;
      }
      event.m_Type           = xiiNetworkQuicEventType::Receive;
      event.m_uiReceiveBytes = receive.TotalBufferLength;
      event.m_bFin           = (receive.Flags & QUIC_RECEIVE_FLAG_FIN) != 0;
      for (xiiUInt32 uiIndex = 0; uiIndex < receive.BufferCount; ++uiIndex)
      {
        const auto& buffer = receive.Buffers[uiIndex];
        event.m_Spans.PushBack({buffer.Buffer, buffer.Length});
      }
      return state.Push(std::move(event)) ? QUIC_STATUS_PENDING : QUIC_STATUS_SUCCESS;
    }
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
      event.m_Type  = xiiNetworkQuicEventType::Send;
      event.m_pSend = pEvent->SEND_COMPLETE.ClientContext;
      if (pEvent->SEND_COMPLETE.Canceled)
      {
        event.m_Error = xiiNetworkError(xiiNetworkErrorCode::Cancelled);
      }
      break;
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
      event.m_Type = xiiNetworkQuicEventType::StreamClosed;
      break;
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
      event.m_Error = QuicViolation;
      break;
    case QUIC_STREAM_EVENT_START_COMPLETE:
      event.m_Error = MapQuicError(pEvent->START_COMPLETE.Status);
      if (event.m_Error.Succeeded())
      {
        return QUIC_STATUS_SUCCESS;
      }
      break;
    default:
      return QUIC_STATUS_SUCCESS;
  }
  state.Push(std::move(event));
  return QUIC_STATUS_SUCCESS;
}

namespace
{
  xiiNetworkError QuicAddress(const xiiNetworkEndpoint& endpoint, QUIC_ADDR& out_address)
  {
    xiiNetworkInternal::NativeAddress address;
    auto                              error = xiiNetworkInternal::ToNative(endpoint, address);
    if (error.Succeeded())
    {
      XII_ASSERT_DEV(static_cast<size_t>(address.m_uiLength) <= sizeof(out_address), "Native address size must fit the QUIC address container.");
      std::memcpy(&out_address, address.Get(), address.m_uiLength);
    }
    return error;
  }

  xiiNetworkError ReadQuicAddress(const QUIC_API_TABLE& api, HQUIC handle, xiiUInt32 uiParameter, xiiNetworkEndpoint& out_endpoint)
  {
    QUIC_ADDR  address = {};
    xiiUInt32  uiBytes = sizeof(address);
    const auto error   = MapQuicError(api.GetParam(handle, uiParameter, &uiBytes, &address));
    return error.Failed() ? error : xiiNetworkInternal::FromNative(reinterpret_cast<const sockaddr*>(&address), static_cast<xiiNetworkInternal::NativeLength>(uiBytes), out_endpoint);
  }

  void ReleaseQuicReceive(xiiNetworkQuicStream& stream, xiiUInt64& ref_uiBytes, xiiUInt32& ref_uiEvents)
  {
    ref_uiBytes += stream.m_Receive.m_uiReservedBytes;
    ref_uiEvents += stream.m_Receive.m_bEventReserved ? 1U : 0U;
  }

  xiiNetworkError CheckQuicCertificate(xiiNetworkCredentials& credentials, bool bServer, xiiStringView sServerName, const xiiNetworkQuicEvent& event, xiiNetworkCertificate& out_certificate)
  {
    if (event.m_Error.Failed())
    {
      return event.m_Error;
    }
    const auto leaf = event.m_Bytes.GetBytes();
    if (leaf.IsEmpty())
    {
      return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    ERR_clear_error();
    const unsigned char* pLeafBytes = leaf.GetPtr();
    X509*                pLeaf      = d2i_X509(nullptr, &pLeafBytes, leaf.GetCount());
    if (pLeaf == nullptr || pLeafBytes != leaf.GetEndPtr())
    {
      X509_free(pLeaf);
      return xiiNetworkSecurityInternal::LastError(xiiNetworkErrorCode::CertificateUntrusted);
    }
    const auto chain              = event.m_Chain.GetBytes();
    PKCS7*     pChain             = nullptr;
    STACK_OF(X509)* pCertificates = nullptr;
    xiiNetworkError error;
    if (!chain.IsEmpty())
    {
      const unsigned char* pChainBytes = chain.GetPtr();
      pChain                           = d2i_PKCS7(nullptr, &pChainBytes, chain.GetCount());
      if (pChain == nullptr || pChainBytes != chain.GetEndPtr() || !PKCS7_type_is_signed(pChain) || pChain->d.sign == nullptr)
      {
        error = xiiNetworkSecurityInternal::LastError(xiiNetworkErrorCode::CertificateUntrusted);
      }
      else
      {
        pCertificates = pChain->d.sign->cert;
      }
    }
    if (error.Succeeded())
    {
      error = xiiNetworkSecurityInternal::VerifyPeer(credentials, pLeaf, pCertificates, sServerName, !bServer, out_certificate);
    }
    PKCS7_free(pChain);
    X509_free(pLeaf);
    return error;
  }
} // namespace

xiiNetworkQuic::~xiiNetworkQuic()
{
  if (m_pState != nullptr)
  {
    XII_ASSERT_DEV(m_pState->m_uiConnections.load() == 0, "Close all native connections before releasing the QUIC provider.");
    XII_DEFAULT_DELETE(m_pState);
  }
}

xiiNetworkError xiiNetworkQuic::Start(const xiiNetworkHostConfiguration& configuration, xiiNetworkIOContext& io, xiiThreadSignal& signal)
{
  if (configuration.m_uiMaximumChannelsPerConnection > 1024 || configuration.m_pCredentials->GetConfiguration().m_bEnableTransportResumption)
  {
    return xiiNetworkError(xiiNetworkErrorCode::UnsupportedCapability);
  }
  xiiUniquePtr<xiiNetworkQuicState> pState = XII_DEFAULT_NEW(xiiNetworkQuicState);
  auto                              error  = AcquireQuicProvider(pState->m_pApi);
  if (error.Failed())
  {
    return error;
  }
  const QUIC_REGISTRATION_CONFIG registration{"XII Foundation", QUIC_EXECUTION_PROFILE_TYPE_SCAVENGER};
  error = MapQuicError(pState->m_pApi->RegistrationOpen(&registration, &pState->m_Registration));
  if (error.Failed())
  {
    return error;
  }
  pState->m_pIO                     = &io;
  pState->m_pSignal                 = &signal;
  pState->m_sAlpn                   = configuration.m_pCredentials->GetConfiguration().m_sApplicationProtocol;
  pState->m_uiConnectionLimit       = configuration.m_uiMaximumConnections;
  pState->m_uiStreamLimit           = configuration.m_uiMaximumChannelsPerConnection + QuicUnorderedLimit;
  pState->m_uiQueueLimit            = configuration.m_uiMaximumQueuedMessages + QuicDatagramQueueLimit + 2 * configuration.m_uiMaximumConnections * (2 * pState->m_uiStreamLimit + 8);
  pState->m_uiSendLimit             = xiiMath::Min(configuration.m_uiMaximumQueuedMessages, pState->m_uiStreamLimit + QuicDatagramQueueLimit) + 1;
  pState->m_uiFlowWindow            = static_cast<xiiUInt32>(xiiMath::Clamp(configuration.m_uiMaximumPeerReceiveBytes, xiiUInt64{65536}, xiiUInt64{64 * 1024 * 1024}));
  pState->m_uiHandshakeMilliseconds = static_cast<xiiUInt64>(xiiMath::Clamp(configuration.m_HandshakeTimeout.GetMilliseconds(), 1.0, 86400000.0));
  pState->m_uiIdleMilliseconds      = static_cast<xiiUInt64>(xiiMath::Clamp(configuration.m_IdleTimeout.GetMilliseconds(), 1.0, 86400000.0));
  m_pState                          = pState.Release();
  return {};
}

xiiNetworkError xiiNetworkQuic::Listen(const xiiNetworkEndpoint& endpoint, xiiNetworkListener owner, xiiNetworkQuicListener*& out_pListener, xiiNetworkEndpoint& out_endpoint)
{
  QUIC_ADDR address = {};
  auto      error   = QuicAddress(endpoint, address);
  if (error.Failed())
  {
    return error;
  }
  xiiUniquePtr<xiiNetworkQuicListener> pListener = XII_DEFAULT_NEW(xiiNetworkQuicListener);
  pListener->m_pState                            = m_pState;
  pListener->m_Owner                             = owner;
  const auto& api                                = *m_pState->m_pApi;
  error                                          = MapQuicError(api.ListenerOpen(m_pState->m_Registration, xiiNetworkQuicState::ListenerCallback, pListener.Borrow(), &pListener->m_Handle));
  if (error.Failed())
  {
    return error;
  }
  const auto& sAlpn = m_pState->m_sAlpn;
  QUIC_BUFFER alpn{sAlpn.GetElementCount(), reinterpret_cast<xiiUInt8*>(const_cast<char*>(sAlpn.GetData()))};
  error = MapQuicError(api.ListenerStart(pListener->m_Handle, &alpn, 1, &address));
  if (error.Succeeded())
  {
    error = ReadQuicAddress(api, pListener->m_Handle, QUIC_PARAM_LISTENER_LOCAL_ADDRESS, out_endpoint);
  }
  if (error.Failed())
  {
    api.ListenerClose(pListener->m_Handle);
    return error;
  }
  out_pListener = pListener.Release();
  return {};
}

void xiiNetworkQuic::CloseListener(xiiNetworkQuicListener* pListener)
{
  m_pState->m_pApi->ListenerClose(pListener->m_Handle);
  XII_DEFAULT_DELETE(pListener);
}

xiiNetworkError xiiNetworkQuic::Connect(const xiiNetworkEndpoint& endpoint, xiiStringView sServerName, xiiSharedPtr<xiiNetworkCredentials> pCredentials, xiiNetworkConnection owner, xiiNetworkQuicConnection*& out_pConnection)
{
  QUIC_ADDR address = {};
  auto      error   = QuicAddress(endpoint, address);
  if (error.Failed())
  {
    return error;
  }
  xiiSharedPtr<QuicConfiguration> pConfiguration;
  error = m_pState->Configuration(pCredentials, false, pConfiguration);
  if (error.Failed())
  {
    return error;
  }
  auto* pConnection = m_pState->Create(false);
  if (pConnection == nullptr)
  {
    return QuicPressure;
  }
  pConnection->m_Owner          = owner;
  pConnection->m_sServerName    = sServerName;
  pConnection->m_pConfiguration = pConfiguration;
  const auto& api               = *m_pState->m_pApi;
  error                         = MapQuicError(api.ConnectionOpen(m_pState->m_Registration, xiiNetworkQuicState::ConnectionCallback, pConnection, &pConnection->m_Handle));
  if (error.Succeeded())
  {
    error = MapQuicError(api.SetParam(pConnection->m_Handle, QUIC_PARAM_CONN_REMOTE_ADDRESS, sizeof(address), &address));
  }
  if (error.Succeeded())
  {
    error = MapQuicError(api.ConnectionStart(pConnection->m_Handle, pConfiguration->m_Handle, QuicAddrGetFamily(&address), pConnection->m_sServerName.GetData(), endpoint.GetPort()));
  }
  if (error.Failed())
  {
    xiiUInt64 uiBytes  = 0;
    xiiUInt32 uiEvents = 0;
    Close(pConnection, uiBytes, uiEvents);
    return error;
  }
  out_pConnection = pConnection;
  return {};
}

xiiNetworkError xiiNetworkQuic::Accept(xiiNetworkQuicConnection& connection, xiiSharedPtr<xiiNetworkCredentials> pCredentials, xiiNetworkConnection owner)
{
  connection.m_Owner = owner;
  const auto error   = m_pState->Configuration(pCredentials, true, connection.m_pConfiguration);
  return error.Failed() ? error : MapQuicError(connection.m_pApi->ConnectionSetConfiguration(connection.m_Handle, connection.m_pConfiguration->m_Handle));
}

xiiNetworkConnection xiiNetworkQuic::GetOwner(const xiiNetworkQuicConnection& connection) const { return connection.m_Owner; }
xiiNetworkError      xiiNetworkQuic::GetFailure(const xiiNetworkQuicConnection& connection) const { return connection.m_bQueueFailed.load() ? QuicPressure : xiiNetworkError(); }
xiiUInt32            xiiNetworkQuic::GetMaximumDatagramBytes(const xiiNetworkQuicConnection& connection) const { return connection.m_uiDatagramBytes.load(); }

xiiNetworkError xiiNetworkQuic::VerifyCertificate(xiiNetworkQuicConnection& connection, const xiiNetworkQuicEvent& event, xiiNetworkCertificate& out_certificate)
{
  const auto error        = CheckQuicCertificate(*connection.m_pConfiguration->m_pCredentials, connection.m_bServer, connection.m_sServerName, event, out_certificate);
  connection.m_bValidated = error.Succeeded();
  const auto notified     = MapQuicError(connection.m_pApi->ConnectionCertificateValidationComplete(connection.m_Handle, connection.m_bValidated, QUIC_TLS_ALERT_CODE_BAD_CERTIFICATE));
  return error.Failed() ? error : notified;
}

xiiNetworkError xiiNetworkQuic::GetEndpoints(xiiNetworkQuicConnection& connection, xiiNetworkEndpoint& out_local, xiiNetworkEndpoint& out_remote)
{
  if (!connection.m_bValidated && (!connection.m_bServer || connection.m_pConfiguration->m_pCredentials->GetConfiguration().m_bRequireClientCertificate))
  {
    return xiiNetworkError(xiiNetworkErrorCode::CertificateUntrusted);
  }
  const auto error = ReadQuicAddress(*connection.m_pApi, connection.m_Handle, QUIC_PARAM_CONN_LOCAL_ADDRESS, out_local);
  return error.Failed() ? error : ReadQuicAddress(*connection.m_pApi, connection.m_Handle, QUIC_PARAM_CONN_REMOTE_ADDRESS, out_remote);
}

bool xiiNetworkQuic::Poll(xiiNetworkQuicEvent& out_event)
{
  {
    XII_LOCK(m_pState->m_QueueMutex);
    if (m_pState->m_Events.IsEmpty())
    {
      return false;
    }
    out_event = std::move(m_pState->m_Events.PeekFront());
    m_pState->m_Events.PopFront();
    if (out_event.m_Type == xiiNetworkQuicEventType::Datagram)
    {
      --m_pState->m_uiDatagrams;
    }
  }
  if (out_event.m_Type == xiiNetworkQuicEventType::Send)
  {
    auto* pSend = static_cast<QuicSend*>(out_event.m_pSend);
    XII_ASSERT_DEV(pSend != nullptr && pSend->m_bReserved, "Each provider completion retires a retained send descriptor.");
    out_event.m_pSend  = pSend->m_pOwner;
    pSend->m_bReserved = false;
  }
  return true;
}

void xiiNetworkQuic::UpdateStatistics(const xiiNetworkQuicConnection& connection, xiiNetworkConnectionStatistics& ref_statistics)
{
  QUIC_STATISTICS_V2 statistics                = {};
  xiiUInt32          uiBytes                   = sizeof(statistics);
  ref_statistics.m_uiProviderReceiveQueueDrops = connection.m_uiDropped.load();
  if (QUIC_SUCCEEDED(connection.m_pApi->GetParam(connection.m_Handle, QUIC_PARAM_CONN_STATISTICS_V2, &uiBytes, &statistics)))
  {
    ref_statistics.m_bHasNativeCongestionStatistics = true;
    ref_statistics.m_NativeSmoothedRoundTrip        = xiiTime::MakeFromMicroseconds(statistics.Rtt);
    ref_statistics.m_uiNativeLostPackets            = statistics.SendSuspectedLostPackets - xiiMath::Min(statistics.SendSuspectedLostPackets, statistics.SendSpuriousLostPackets);
    ref_statistics.m_uiNativeCongestionWindowBytes  = statistics.SendCongestionWindow;
    ref_statistics.m_uiNativeReorderedPackets       = statistics.RecvReorderedPackets;
    ref_statistics.m_uiWireBytesSent                = statistics.SendTotalBytes;
    ref_statistics.m_uiWireBytesReceived            = statistics.RecvTotalBytes;
  }
}

xiiNetworkError xiiNetworkQuic::Send(xiiNetworkQuicConnection& connection, xiiUInt32 uiChannel, xiiNetworkDeliveryMode mode, xiiArrayPtr<const xiiUInt8> header, xiiArrayPtr<const xiiUInt8> payload, void* pOwner)
{
  auto&      api        = *connection.m_pApi;
  const bool bDatagram  = mode == xiiNetworkDeliveryMode::UnreliableSequenced || mode == xiiNetworkDeliveryMode::UnreliableUnsequenced;
  const bool bUnordered = mode == xiiNetworkDeliveryMode::ReliableUnordered;
  QuicSend*  pSend      = nullptr;
  for (auto& send : connection.m_Sends)
  {
    if (!send.m_bReserved)
    {
      pSend = &send;
      break;
    }
  }
  if (pSend == nullptr)
  {
    if (connection.m_Sends.GetCount() >= m_pState->m_uiSendLimit)
    {
      return xiiNetworkError(xiiNetworkErrorCode::WouldBlock);
    }
    pSend = &connection.m_Sends.ExpandAndGetRef();
  }
  pSend->m_Buffers[0]     = {header.GetCount(), const_cast<xiiUInt8*>(header.GetPtr())};
  pSend->m_Buffers[1]     = {payload.GetCount(), const_cast<xiiUInt8*>(payload.GetPtr())};
  pSend->m_pOwner         = pOwner;
  pSend->m_bReserved      = true;
  const xiiUInt32 uiCount = payload.IsEmpty() ? 1 : 2;
  xiiNetworkError error;
  if (bDatagram)
  {
    const auto uiLimit = GetMaximumDatagramBytes(connection);
    if (uiLimit == 0)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::UnsupportedCapability);
    }
    else if (header.GetCount() + payload.GetCount() > uiLimit)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
    }
    else
    {
      error = MapQuicError(api.DatagramSend(connection.m_Handle, pSend->m_Buffers, uiCount, QUIC_SEND_FLAG_NONE, pSend));
    }
  }
  else
  {
    xiiNetworkQuicStream* pStream = nullptr;
    {
      XII_LOCK(connection.m_StreamsMutex);
      if (uiChannel == 0)
      {
        pStream = connection.m_pControl;
      }
      else if (!bUnordered)
      {
        for (const auto& pExisting : connection.m_Streams)
        {
          if (!pExisting->m_bIncoming && pExisting->m_uiChannel == uiChannel)
          {
            pStream = pExisting.Borrow();
            break;
          }
        }
      }
    }
    if (pStream == nullptr && uiChannel == 0 && connection.m_bServer)
    {
      error = xiiNetworkError(xiiNetworkErrorCode::WouldBlock);
    }
    else if (pStream == nullptr)
    {
      pStream = AddQuicStream(connection, nullptr, false, uiChannel == 0, bUnordered);
      if (pStream == nullptr)
      {
        error = xiiNetworkError(xiiNetworkErrorCode::WouldBlock);
      }
      else
      {
        pStream->m_uiChannel = uiChannel;
        error                = MapQuicError(api.StreamOpen(connection.m_Handle, uiChannel == 0 ? QUIC_STREAM_OPEN_FLAG_NONE : QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL, xiiNetworkQuicState::StreamCallback, pStream, &pStream->m_Handle));
        if (error.Succeeded())
        {
          error = MapQuicError(api.StreamStart(pStream->m_Handle, QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL));
        }
        if (error.Failed())
        {
          xiiUInt64 uiBytes  = 0;
          xiiUInt32 uiEvents = 0;
          CloseStream(pStream, uiBytes, uiEvents);
        }
      }
    }
    if (error.Succeeded())
    {
      error = MapQuicError(api.StreamSend(pStream->m_Handle, pSend->m_Buffers, uiCount, bUnordered ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE, pSend));
    }
  }
  if (error.Failed())
  {
    pSend->m_bReserved = false;
  }
  return error;
}

void xiiNetworkQuic::FinishChannel(xiiNetworkQuicConnection& connection, xiiUInt32 uiChannel)
{
  XII_LOCK(connection.m_StreamsMutex);
  for (const auto& pStream : connection.m_Streams)
  {
    if (!pStream->m_bIncoming && !pStream->m_bUnordered && pStream->m_uiChannel == uiChannel)
    {
      connection.m_pApi->StreamShutdown(pStream->m_Handle, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
      break;
    }
  }
}

xiiNetworkReceiveState& xiiNetworkQuic::GetReceiveState(xiiNetworkQuicStream& stream) { return stream.m_Receive; }
void                    xiiNetworkQuic::CompleteReceive(xiiNetworkQuicStream& stream, xiiUInt64 uiBytes) { stream.m_pConnection->m_pApi->StreamReceiveComplete(stream.m_Handle, uiBytes); }

void xiiNetworkQuic::CloseStream(xiiNetworkQuicStream* pStream, xiiUInt64& ref_uiBytes, xiiUInt32& ref_uiEvents)
{
  auto& connection = *pStream->m_pConnection;
  if (pStream->m_Handle != nullptr)
  {
    connection.m_pApi->StreamClose(pStream->m_Handle);
  }
  XII_LOCK(connection.m_StreamsMutex);
  ReleaseQuicReceive(*pStream, ref_uiBytes, ref_uiEvents);
  if (connection.m_pControl == pStream)
  {
    connection.m_pControl = nullptr;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < connection.m_Streams.GetCount(); ++uiIndex)
  {
    if (connection.m_Streams[uiIndex].Borrow() == pStream)
    {
      if (uiIndex + 1 != connection.m_Streams.GetCount())
      {
        connection.m_Streams[uiIndex] = std::move(connection.m_Streams.PeekBack());
      }
      connection.m_Streams.PopBack();
      return;
    }
  }
}

void xiiNetworkQuic::Shutdown(xiiNetworkQuicConnection& connection)
{
  if (!connection.m_bShutdownRequested)
  {
    connection.m_bShutdownRequested = true;
    connection.m_pApi->ConnectionShutdown(connection.m_Handle, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, QuicApplicationError);
  }
}

void xiiNetworkQuic::Close(xiiNetworkQuicConnection* pConnection, xiiUInt64& ref_uiBytes, xiiUInt32& ref_uiEvents)
{
  pConnection->m_bQuiescing.store(true);
  if (pConnection->m_Handle != nullptr)
  {
    pConnection->m_pApi->ConnectionClose(pConnection->m_Handle);
    for (const auto& pStream : pConnection->m_Streams)
    {
      pConnection->m_pApi->StreamClose(pStream->m_Handle);
      ReleaseQuicReceive(*pStream, ref_uiBytes, ref_uiEvents);
    }
  }
  {
    XII_LOCK(m_pState->m_QueueMutex);
    xiiUInt32 uiRetained = 0;
    for (xiiUInt32 uiIndex = 0; uiIndex < m_pState->m_Events.GetCount(); ++uiIndex)
    {
      auto& event = m_pState->m_Events[uiIndex];
      if (event.m_pConnection == pConnection)
      {
        if (event.m_Type == xiiNetworkQuicEventType::Datagram)
        {
          --m_pState->m_uiDatagrams;
        }
        continue;
      }
      if (uiRetained != uiIndex)
      {
        m_pState->m_Events[uiRetained] = std::move(event);
      }
      ++uiRetained;
    }
    while (m_pState->m_Events.GetCount() > uiRetained)
    {
      m_pState->m_Events.PopBack();
    }
  }
  m_pState->Destroy(pConnection);
}
