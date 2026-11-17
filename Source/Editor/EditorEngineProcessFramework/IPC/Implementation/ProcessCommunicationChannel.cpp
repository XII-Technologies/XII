/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcessMessage, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult RegisterEditorProcessCodecs(xiiNetworkTypeRegistry& registry);

namespace
{
  constexpr xiiUInt32 MaximumPacketBytes = 32 * 1024 * 1024;
  constexpr xiiUInt64 TypePrefix(xiiProcessMessageDomain domain)
  {
    return domain == xiiProcessMessageDomain::Engine ? 0x5849494500000000ULL : 0x5849494100000000ULL;
  }

  bool IsContract(const xiiRTTI* pType, xiiProcessMessageDomain domain)
  {
    const auto* identity = pType->GetAttributeByType<xiiNetworkIdentityAttribute>();
    return identity != nullptr && (identity->m_uiTypeId & 0xFFFFFFFFFFFF0000ULL) == TypePrefix(domain);
  }

  xiiResult Configure(xiiNetworkHostConfiguration& config, xiiProcessMessageDomain domain, bool bLocal)
  {
    config.m_Protocol                       = xiiUuid(0x584949454449544FULL, TypePrefix(domain));
    config.m_uiMaximumConnections           = 2;
    config.m_uiMaximumListeners             = 1;
    config.m_uiMaximumChannelsPerConnection = 2;
    config.m_uiMaximumMessageBytes          = MaximumPacketBytes;
    config.m_uiMaximumPeerSendBytes         = 64 * 1024 * 1024;
    config.m_uiMaximumPeerReceiveBytes      = 64 * 1024 * 1024;
    config.m_uiMaximumReceiveBytes          = 64 * 1024 * 1024;
    config.m_uiSendBurstBytes               = MaximumPacketBytes;
    config.m_uiPeerSendBurstBytes           = MaximumPacketBytes;
    config.m_IO.m_uiWorkerThreadCount       = 1;
    xiiNetworkReflectionEndpoint::EnableProtocol(config);
    if (bLocal)
    {
      config.m_Transport      = xiiNetworkTransportType::Local;
      config.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;
      return XII_SUCCESS;
    }

    const auto& command   = *xiiCommandLineUtils::GetGlobalInstance();
    const auto  transport = command.GetStringOption("-ipc_remote_transport", 0, "tls");
    if (transport == "quic")
      config.m_Transport = xiiNetworkTransportType::SecureDatagram;
    else if (transport != "tls")
      return XII_FAILURE;
    xiiNetworkSecurityConfig security;
    security.m_sCertificateChainFile = command.GetStringOption("-ipc_cert");
    security.m_sPrivateKeyFile       = command.GetStringOption("-ipc_key");
    security.m_sTrustAnchorsFile     = command.GetStringOption("-ipc_ca");
    security.m_sApplicationProtocol  = "xii-editor/1";
    security.m_bUseSystemTrust       = false;
    if (security.m_sCertificateChainFile.IsEmpty() || security.m_sPrivateKeyFile.IsEmpty() || security.m_sTrustAnchorsFile.IsEmpty())
    {
      xiiLog::Error("Remote Editor IPC requires -ipc_cert, -ipc_key, and -ipc_ca.");
      return XII_FAILURE;
    }
    config.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
    return config.m_pCredentials->Load(security).GetResult();
  }

  class ProcessMessageHandler final : public xiiNetworkMessageHandler
  {
  public:
    ProcessMessageHandler(const xiiRTTI* pType, xiiVariant& ref_message) :
      m_pType(pType), m_Message(ref_message)
    {
    }

    const xiiRTTI* GetType() const override { return m_pType; }

    bool Authorize(const xiiNetworkDispatchContext& context) const override
    {
      return context.m_pSession != nullptr && context.m_Channel == context.m_pSession->m_DefaultChannel &&
        context.m_pSession->m_ConnectionInfo.m_Capabilities.m_bAuthenticated;
    }

    void Handle(const xiiNetworkDispatchContext& context, const void* pMessage) override
    {
      if (m_pType == xiiGetStaticRTTI<xiiViewFrameMsgToEditor>())
      {
        const auto& memory          = static_cast<const xiiViewFrameMsgToEditor*>(pMessage)->m_Memory;
        const auto  expectedProcess = context.m_pSession->m_ConnectionInfo.m_uiLocalPeerProcessId;
        if ((memory.m_uiNativeHandle != 0 || memory.m_uiNativeSemaphoreHandle != 0) &&
            (expectedProcess == 0 || memory.m_uiProcessId != expectedProcess))
        {
          xiiLog::Warning("Editor IPC rejected a shared texture from outside the authenticated process.");
          return;
        }
      }
      // The decoded object belongs to the endpoint. Application handlers may load plugins and
      // therefore run after Dispatch releases its registry read scope.
      m_Message.MoveTypedObject(xiiReflectionSerializer::Clone(pMessage, m_pType), m_pType);
    }

  private:
    const xiiRTTI* m_pType;
    xiiVariant&    m_Message;
  };
} // namespace

class xiiEditorProcessTransport
{
public:
  xiiEditorProcessTransport() : m_Reflection(m_Host, m_Registry) {}
  ~xiiEditorProcessTransport()
  {
    m_Reflection.Shutdown();
    m_Host.Shutdown();
  }

  xiiResult Start(xiiProcessMessageDomain domain, bool bLocal)
  {
    m_Domain = domain;
    m_bLocal = bLocal;
    xiiNetworkHostConfiguration config;
    XII_SUCCEED_OR_RETURN(Configure(config, domain, bLocal));
    xiiNetworkReflectionConfiguration reflection;
    reflection.m_uiMaximumPeers                          = config.m_uiMaximumConnections;
    reflection.m_uiMaximumQueuedBytes                    = 64 * 1024 * 1024;
    reflection.m_Serialization.m_uiMaximumPayloadBytes   = MaximumPacketBytes;
    reflection.m_Serialization.m_uiMaximumArrayElements  = 16384;
    reflection.m_Serialization.m_uiMaximumObjectBytes    = MaximumPacketBytes;
    reflection.m_Serialization.m_uiMaximumDecodedBytes   = 192 * 1024 * 1024;
    reflection.m_Serialization.m_uiMaximumTemporaryBytes = 192 * 1024 * 1024;
    if (domain == xiiProcessMessageDomain::Engine)
      XII_SUCCEED_OR_RETURN(RegisterEditorProcessCodecs(m_Registry));
    XII_SUCCEED_OR_RETURN(m_Reflection.Start(reflection).GetResult());
    XII_SUCCEED_OR_RETURN(RegisterContracts());
    return m_Host.Start(config).GetResult();
  }

  xiiResult RegisterContracts()
  {
    m_Types.Clear();
    xiiRTTI::ForEachType([&](const xiiRTTI* pType) {
      if (pType->IsDerivedFrom<xiiProcessMessage>() && IsContract(pType, m_Domain))
        m_Types.PushBack(pType);
    });
    if (m_Types.IsEmpty())
      return XII_FAILURE;
    bool bRegistered = true;
    for (const auto* pType : m_Types)
    {
      xiiSharedPtr<const xiiNetworkSchema> schema;
      const auto                           schemaError = m_Registry.GetSchema(pType, schema);
      if (schemaError.Failed())
      {
        bRegistered = false;
        xiiLog::Error("Invalid Editor IPC schema '{}'. {} (field {}).", pType->GetTypeName(), schemaError.GetCodeName(), schemaError.m_uiField);
        continue;
      }
      xiiSharedPtr<xiiNetworkMessageHandler> handler = XII_DEFAULT_NEW(ProcessMessageHandler, pType, m_Received);
      const auto                             error   = m_Reflection.RegisterMessageHandler(std::move(handler));
      if (error.Failed())
      {
        xiiLog::Error("Could not register Editor IPC message '{}'. {}", pType->GetTypeName(), error.GetCodeName());
        bRegistered = false;
      }
    }
    m_uiRegistryGeneration = m_Registry.GetStatistics().m_uiGeneration;
    return bRegistered ? XII_SUCCESS : XII_FAILURE;
  }

  xiiResult OfferContracts()
  {
    for (const auto* pType : m_Types)
    {
      const auto error = m_Reflection.OfferSchema(m_Channel, pType);
      if (error.Failed())
      {
        xiiLog::Error("Could not offer Editor IPC schema '{}'. {}", pType->GetTypeName(), error.GetCodeName());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  bool RefreshContracts()
  {
    if (m_uiRegistryGeneration == m_Registry.GetStatistics().m_uiGeneration)
      return true;
    m_bReady = false;
    return RegisterContracts().Succeeded() && (!m_Channel.IsValid() || OfferContracts().Succeeded());
  }

  void CheckReadiness()
  {
    if (m_bReady || !m_Channel.IsValid())
      return;
    for (const auto* pType : m_Types)
    {
      bool bCompact = false;
      if (m_Reflection.GetSchemaAgreement(m_Channel, pType->GetAttributeByType<xiiNetworkIdentityAttribute>()->m_uiTypeId, bCompact).Failed())
        return;
    }
    m_bReady = true;
  }

  void ProcessEvent(xiiNetworkEvent&& event)
  {
    const auto connection = event.m_Connection;
    if (event.m_Type == xiiNetworkEventType::Connected && event.m_pSessionInfo)
    {
      const auto& info = event.m_pSessionInfo->m_ConnectionInfo;
      if (m_Channel.IsValid() || !info.m_Capabilities.m_bAuthenticated ||
          (m_bLocal && (m_uiExpectedProcessId == 0 || info.m_uiLocalPeerProcessId != m_uiExpectedProcessId)))
      {
        m_Host.Disconnect(connection, false).IgnoreResult();
        return;
      }
      m_Channel    = event.m_pSessionInfo->m_DefaultChannel;
      m_Connection = connection;
      if (m_Reflection.ProcessEvent(std::move(event)).Failed() || OfferContracts().Failed())
        m_Host.Disconnect(connection, false).IgnoreResult();
      return;
    }
    if (connection != m_Connection)
      return;
    if (event.m_Type == xiiNetworkEventType::Closed)
    {
      m_Reflection.ProcessEvent(std::move(event)).IgnoreResult();
      m_Channel    = {};
      m_Connection = {};
      m_bReady     = false;
    }
    else if (event.m_Type == xiiNetworkEventType::Message || event.m_Type == xiiNetworkEventType::ChannelClosed)
    {
      const auto error = m_Reflection.ProcessEvent(std::move(event));
      if (error.Failed() && error.m_Code != xiiNetworkDataErrorCode::NotNegotiated && error.m_Code != xiiNetworkDataErrorCode::RegistryUnavailable)
      {
        xiiLog::Warning("Editor IPC protocol error. {}", error.GetCodeName());
        m_Host.Disconnect(connection, false).IgnoreResult();
      }
      CheckReadiness();
    }
  }

  xiiNetworkHost                     m_Host;
  xiiNetworkTypeRegistry             m_Registry;
  xiiNetworkReflectionEndpoint       m_Reflection;
  xiiHybridArray<const xiiRTTI*, 64> m_Types;
  xiiVariant                         m_Received;
  xiiNetworkConnection               m_Connection;
  xiiNetworkListener                 m_Listener;
  xiiNetworkChannel                  m_Channel;
  xiiProcessMessageDomain            m_Domain               = xiiProcessMessageDomain::Engine;
  xiiUInt64                          m_uiRegistryGeneration = 0;
  xiiUInt32                          m_uiExpectedProcessId  = 0;
  bool                               m_bLocal               = false;
  bool                               m_bReady               = false;
  bool                               m_bMayWaitToSend       = false;
};

xiiProcessCommunicationChannel::xiiProcessCommunicationChannel()  = default;
xiiProcessCommunicationChannel::~xiiProcessCommunicationChannel() = default;

xiiResult xiiProcessCommunicationChannel::StartLocalListener(const xiiNetworkLocalEndpoint& endpoint, xiiProcessMessageDomain domain)
{
  CloseTransport();
  xiiUniquePtr<xiiEditorProcessTransport> transport = XII_DEFAULT_NEW(xiiEditorProcessTransport);
  XII_SUCCEED_OR_RETURN(transport->Start(domain, true));
  XII_SUCCEED_OR_RETURN(transport->m_Host.Listen(endpoint, transport->m_Listener).GetResult());
  m_pTransport = std::move(transport);
  return XII_SUCCESS;
}

xiiResult xiiProcessCommunicationChannel::ConnectLocal(const xiiNetworkLocalEndpoint& endpoint, xiiProcessMessageDomain domain, xiiUInt32 uiExpectedProcessId)
{
  CloseTransport();
  xiiUniquePtr<xiiEditorProcessTransport> transport = XII_DEFAULT_NEW(xiiEditorProcessTransport);
  XII_SUCCEED_OR_RETURN(transport->Start(domain, true));
  transport->m_uiExpectedProcessId = uiExpectedProcessId;
  transport->m_bMayWaitToSend      = true;
  XII_SUCCEED_OR_RETURN(transport->m_Host.Connect(endpoint, {}, transport->m_Connection).GetResult());
  m_pTransport = std::move(transport);
  return XII_SUCCESS;
}

xiiResult xiiProcessCommunicationChannel::StartRemoteListener(xiiStringView sAddress)
{
  CloseTransport();
  xiiNetworkEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkEndpoint::Parse(sAddress, endpoint).GetResult());
  xiiUniquePtr<xiiEditorProcessTransport> transport = XII_DEFAULT_NEW(xiiEditorProcessTransport);
  XII_SUCCEED_OR_RETURN(transport->Start(xiiProcessMessageDomain::Engine, false));
  transport->m_bMayWaitToSend = true;
  XII_SUCCEED_OR_RETURN(transport->m_Host.Listen(endpoint, transport->m_Listener).GetResult());
  m_pTransport = std::move(transport);
  return XII_SUCCESS;
}

xiiResult xiiProcessCommunicationChannel::ConnectRemote(xiiStringView sAddress)
{
  CloseTransport();
  xiiStringView      sHost = sAddress;
  xiiStringView      sPort = "1050";
  xiiStringBuilder   numericHost;
  xiiNetworkEndpoint endpoint;
  if (xiiNetworkEndpoint::Parse(sAddress, endpoint).Succeeded())
  {
    endpoint.GetAddress().ToString(numericHost);
    sHost = numericHost;
  }
  else if (const char* pSeparator = sAddress.FindSubString(":"))
  {
    sHost = xiiStringView(sAddress.GetStartPointer(), pSeparator);
    sPort = xiiStringView(pSeparator + 1, sAddress.GetEndPointer());
  }
  xiiUniquePtr<xiiEditorProcessTransport> transport = XII_DEFAULT_NEW(xiiEditorProcessTransport);
  XII_SUCCEED_OR_RETURN(transport->Start(xiiProcessMessageDomain::Engine, false));
  xiiNetworkConnectOptions options;
  options.m_sServerName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-ipc_server_name", 0, sHost);
  const auto error      = endpoint.IsValid() ? transport->m_Host.Connect(endpoint, options, transport->m_Connection) :
                                               transport->m_Host.Connect(sHost, sPort, options, transport->m_Connection);
  XII_SUCCEED_OR_RETURN(error.GetResult());
  m_pTransport = std::move(transport);
  return XII_SUCCESS;
}

void xiiProcessCommunicationChannel::SetExpectedProcessId(xiiUInt32 uiProcessId)
{
  m_pTransport->m_uiExpectedProcessId = uiProcessId;
}

void xiiProcessCommunicationChannel::CloseTransport()
{
  m_pTransport.Clear();
  m_pWaitForMessageType = nullptr;
}

bool xiiProcessCommunicationChannel::HasTransport() const { return m_pTransport != nullptr; }
bool xiiProcessCommunicationChannel::HasPendingConnection() const { return m_pTransport && m_pTransport->m_Connection.IsValid(); }
bool xiiProcessCommunicationChannel::IsConnected() const { return m_pTransport && m_pTransport->m_bReady; }

bool xiiProcessCommunicationChannel::SendMessage(xiiProcessMessage* pMessage)
{
  if (!m_pTransport || !m_pTransport->m_Channel.IsValid() || !m_pTransport->RefreshContracts())
    return false;

  const auto* pType = pMessage->GetDynamicRTTI();
  if (!IsContract(pType, m_pTransport->m_Domain) || (m_pFirstAllowedMessageType && !pType->IsDerivedFrom(m_pFirstAllowedMessageType)))
    return false;

  auto&      transport = *m_pTransport;
  const auto deadline  = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
  do
  {
    auto error = transport.m_Reflection.SendMessage(transport.m_Channel, pType, pMessage);
    if (error.Succeeded())
    {
      m_pFirstAllowedMessageType = nullptr;
      return true;
    }
    if (!transport.m_bMayWaitToSend || (error.m_Code != xiiNetworkDataErrorCode::NotNegotiated && error.m_Code != xiiNetworkDataErrorCode::Backpressure))
    {
      xiiLog::Warning("Could not send Editor IPC message '{}'. {}", pType->GetTypeName(), error.GetCodeName());
      return false;
    }
    // Startup can load plugins and publish reflection metadata in a message callback. Pump
    // schema and transfer completions here, deferring application messages to the owner loop.
    xiiNetworkEvent event;
    const auto      remaining = deadline - xiiTime::Now();
    if (!remaining.IsPositive() || transport.m_Host.WaitForEvent(event, remaining).Failed())
      return false;
    transport.ProcessEvent(std::move(event));
  } while (transport.m_Channel.IsValid());
  return false;
}

bool xiiProcessCommunicationChannel::Pump(xiiTime wait)
{
  if (!m_pTransport || m_bDispatchingMessage || !m_pTransport->RefreshContracts())
    return false;

  m_bInterrupted  = false;
  bool bProcessed = false;
  for (xiiUInt32 i = 0; i < 128 && m_pTransport && !m_bInterrupted; ++i)
  {
    auto&      transport = *m_pTransport;
    const bool bWasReady = transport.m_bReady;
    if (bWasReady && transport.m_Reflection.GetStatistics().m_uiQueuedPackets != 0)
    {
      const auto error = transport.m_Reflection.Dispatch(0, 1);
      if (error.Failed() && error.m_Code != xiiNetworkDataErrorCode::HandlerRemoved && error.m_Code != xiiNetworkDataErrorCode::Disconnected)
      {
        xiiLog::Warning("Rejected Editor IPC message. {}", error.GetCodeName());
        transport.m_Host.Disconnect(transport.m_Channel.m_Connection, false).IgnoreResult();
      }
      auto message = std::move(transport.m_Received);
      if (message.IsValid())
      {
        bProcessed = true;
        DispatchMessage(*static_cast<const xiiProcessMessage*>(message.GetData()));
      }
      continue;
    }
    xiiNetworkEvent event;
    const bool      bEvent = i == 0 && wait.IsPositive() ? transport.m_Host.WaitForEvent(event, wait).Succeeded() : transport.m_Host.TryGetEvent(event);
    if (!bEvent)
      break;
    transport.ProcessEvent(std::move(event));
    // Let startup callers observe schema readiness before dispatching application messages.
    if (!bWasReady && transport.m_bReady)
      break;
  }
  return bProcessed;
}

bool xiiProcessCommunicationChannel::ProcessMessages() { return Pump({}); }
void xiiProcessCommunicationChannel::WaitForMessages() { Pump(xiiTime::MakeFromMilliseconds(100)); }

void xiiProcessCommunicationChannel::DispatchMessage(const xiiProcessMessage& message)
{
  m_bDispatchingMessage = true;
  XII_SCOPE_EXIT(m_bDispatchingMessage = false);
  if (m_pWaitForMessageType && message.GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType) &&
      (!m_WaitForMessageCallback.IsValid() || m_WaitForMessageCallback(const_cast<xiiProcessMessage*>(&message))))
  {
    m_pWaitForMessageType = nullptr;
  }
  Event event{&message};
  m_Events.Broadcast(event);
  m_bInterrupted = event.m_bInterruptMessageProcessing;
}

xiiResult xiiProcessCommunicationChannel::WaitForMessage(const xiiRTTI* pMessageType, xiiTime timeout, WaitForMessageCallback* pMessageCallback)
{
  if (!IsConnected() || pMessageType == nullptr || !timeout.IsPositive() || m_bDispatchingMessage || m_pWaitForMessageType != nullptr)
    return XII_FAILURE;
  m_pWaitForMessageType    = pMessageType;
  m_WaitForMessageCallback = pMessageCallback ? *pMessageCallback : WaitForMessageCallback();
  XII_SCOPE_EXIT(m_pWaitForMessageType = nullptr; m_WaitForMessageCallback = {});

  const auto deadline = xiiTime::Now() + timeout;
  while (m_pWaitForMessageType)
  {
    const auto remaining = deadline - xiiTime::Now();
    if (!remaining.IsPositive() || !m_pTransport || !m_pTransport->m_Channel.IsValid())
      return XII_FAILURE;
    Pump(remaining);
  }
  return m_pTransport && m_pTransport->m_Channel.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiProcessCommunicationChannel::WaitForConnection(xiiTime timeout)
{
  if (!m_pTransport || !timeout.IsPositive() || m_bDispatchingMessage)
    return XII_FAILURE;
  const auto deadline = xiiTime::Now() + timeout;
  while (!IsConnected())
  {
    const auto remaining = deadline - xiiTime::Now();
    if (!m_pTransport || !remaining.IsPositive() || (!m_pTransport->m_Listener.IsValid() && !HasPendingConnection()))
      return XII_FAILURE;
    Pump(remaining);
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(EditorEngineProcessFramework, EditorEngineProcessFramework_IPC_Implementation_ProcessCommunicationChannel);
