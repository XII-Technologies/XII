/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <GameEngine/GameApplication/PlayerSessionProtocol.h>

namespace
{
  template <typename T>
  class PlayerMessageHandler final : public xiiNetworkTypedMessageHandler<T>
  {
  public:
    explicit PlayerMessageHandler(xiiPlayerSessionProtocol::Handler<T> handler) : m_Handler(std::move(handler)) {}

    bool Authorize(const xiiNetworkDispatchContext& context) const override
    {
      return context.m_pSession != nullptr && context.m_Channel == context.m_pSession->m_DefaultChannel &&
        context.m_pSession->m_ConnectionInfo.m_uiLocalPeerProcessId != 0;
    }

    void OnMessage(const xiiNetworkDispatchContext& context, const T& message) override
    {
      m_Handler(context.m_Channel, message);
    }

  private:
    xiiPlayerSessionProtocol::Handler<T> m_Handler;
  };
} // namespace

xiiPlayerSessionProtocol::xiiPlayerSessionProtocol() :
  m_Reflection(m_Host, m_Registry)
{
}

xiiPlayerSessionProtocol::~xiiPlayerSessionProtocol()
{
  m_Reflection.Shutdown();
  m_Host.Shutdown();
}

xiiResult xiiPlayerSessionProtocol::Start(bool bEditor)
{
  m_bEditor = bEditor;
  xiiNetworkHostConfiguration config;
  config.m_Transport                      = xiiNetworkTransportType::Local;
  config.m_Authentication                 = xiiNetworkAuthenticationPolicy::LocalUser;
  config.m_Protocol                       = xiiUuid(0x584949506C617965ULL, 0x72436F6E74726F6CULL);
  config.m_uiMaximumListeners             = bEditor ? 64 : 1;
  config.m_uiMaximumConnections           = bEditor ? 64 : 1;
  config.m_uiMaximumChannelsPerConnection = 2;
  config.m_uiMaximumMessageBytes          = 128 * 1024;
  config.m_uiMaximumPeerSendBytes         = 256 * 1024;
  config.m_uiMaximumPeerReceiveBytes      = 256 * 1024;
  config.m_uiMaximumReceiveBytes          = 16 * 1024 * 1024;
  config.m_IO.m_uiWorkerThreadCount       = bEditor ? 2 : 1;
  xiiNetworkReflectionEndpoint::EnableProtocol(config);

  xiiNetworkReflectionConfiguration reflection;
  reflection.m_uiMaximumPeers                        = config.m_uiMaximumConnections;
  reflection.m_uiMaximumHandlers                     = 3;
  reflection.m_uiMaximumSchemasPerPeer               = 3;
  reflection.m_Serialization.m_uiMaximumPayloadBytes = config.m_uiMaximumMessageBytes;
  XII_SUCCEED_OR_RETURN(m_Reflection.Start(reflection).GetResult());
  XII_SUCCEED_OR_RETURN(RegisterHandlers());
  m_uiRegistryGeneration = m_Registry.GetStatistics().m_uiGeneration;
  return m_Host.Start(config).GetResult();
}

xiiResult xiiPlayerSessionProtocol::StartEditor(Handler<xiiPlayerStatusMessage> status)
{
  m_Status = std::move(status);
  return Start(true);
}

xiiResult xiiPlayerSessionProtocol::StartPlayer(Handler<xiiPlayerSceneMessage> scene, Handler<xiiPlayerStopMessage> stop)
{
  m_Scene = std::move(scene);
  m_Stop  = std::move(stop);
  return Start(false);
}

xiiResult xiiPlayerSessionProtocol::RegisterHandlers()
{
  if (m_bEditor)
  {
    xiiSharedPtr<xiiNetworkMessageHandler> pStatus = XII_DEFAULT_NEW(PlayerMessageHandler<xiiPlayerStatusMessage>, m_Status);
    return m_Reflection.RegisterMessageHandler(std::move(pStatus)).GetResult();
  }
  xiiSharedPtr<xiiNetworkMessageHandler> pScene = XII_DEFAULT_NEW(PlayerMessageHandler<xiiPlayerSceneMessage>, m_Scene);
  xiiSharedPtr<xiiNetworkMessageHandler> pStop  = XII_DEFAULT_NEW(PlayerMessageHandler<xiiPlayerStopMessage>, m_Stop);
  XII_SUCCEED_OR_RETURN(m_Reflection.RegisterMessageHandler(std::move(pScene)).GetResult());
  return m_Reflection.RegisterMessageHandler(std::move(pStop)).GetResult();
}

void xiiPlayerSessionProtocol::Update()
{
  const auto uiGeneration = m_Registry.GetStatistics().m_uiGeneration;
  if (uiGeneration == m_uiRegistryGeneration || RegisterHandlers().Failed())
    return;
  m_uiRegistryGeneration = uiGeneration;
  for (const auto channel : m_Channels)
  {
    if (OfferSchemas(channel).Failed())
      m_Host.Disconnect(channel.m_Connection, false).IgnoreResult();
  }
}

xiiResult xiiPlayerSessionProtocol::OfferSchemas(xiiNetworkChannel channel)
{
  // Each peer advertises all contracts so a peer that refreshed its RTTI can renew both directions.
  XII_SUCCEED_OR_RETURN(m_Reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiPlayerSceneMessage>()).GetResult());
  XII_SUCCEED_OR_RETURN(m_Reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiPlayerStopMessage>()).GetResult());
  return m_Reflection.OfferSchema(channel, xiiGetStaticRTTI<xiiPlayerStatusMessage>()).GetResult();
}

bool xiiPlayerSessionProtocol::HasAgreement(xiiNetworkChannel channel, const xiiRTTI* pType) const
{
  xiiSharedPtr<const xiiNetworkSchema> pSchema;
  bool                                 bCompact;
  return m_Registry.GetSchema(pType, pSchema).Succeeded() &&
    m_Reflection.GetSchemaAgreement(channel, pSchema->m_uiTypeId, bCompact).Succeeded();
}

bool xiiPlayerSessionProtocol::IsReady(xiiNetworkChannel channel) const
{
  if (!channel.IsValid())
    return false;
  return HasAgreement(channel, xiiGetStaticRTTI<xiiPlayerSceneMessage>()) &&
    HasAgreement(channel, xiiGetStaticRTTI<xiiPlayerStopMessage>()) &&
    HasAgreement(channel, xiiGetStaticRTTI<xiiPlayerStatusMessage>());
}

xiiResult xiiPlayerSessionProtocol::ProcessEvent(xiiNetworkEvent&& event)
{
  switch (event.m_Type)
  {
    case xiiNetworkEventType::Connected:
    {
      if (event.m_pSessionInfo == nullptr)
        return XII_FAILURE;
      const auto channel = event.m_pSessionInfo->m_DefaultChannel;
      XII_SUCCEED_OR_RETURN(m_Reflection.ProcessEvent(std::move(event)).GetResult());
      m_Channels.PushBack(channel);
      return OfferSchemas(channel);
    }
    case xiiNetworkEventType::Message:
    {
      const auto error = m_Reflection.ProcessEvent(std::move(event));
      if (error.m_Code == xiiNetworkDataErrorCode::NotNegotiated || error.m_Code == xiiNetworkDataErrorCode::RegistryUnavailable)
        return XII_SUCCESS;
      XII_SUCCEED_OR_RETURN(error.GetResult());
      return m_Reflection.Dispatch(0, 1).GetResult();
    }
    case xiiNetworkEventType::Closed:
      for (xiiUInt32 i = 0; i < m_Channels.GetCount(); ++i)
        if (m_Channels[i].m_Connection == event.m_Connection)
        {
          m_Channels.RemoveAtAndSwap(i);
          break;
        }
      [[fallthrough]];
    case xiiNetworkEventType::ChannelClosed:
      return m_Reflection.ProcessEvent(std::move(event)).GetResult();
    default:
      return XII_SUCCESS;
  }
}

xiiResult xiiPlayerSessionProtocol::WaitForFinalStatus(xiiNetworkMessage submission, xiiTime timeout)
{
  const auto      deadline = xiiTime::Now() + timeout;
  xiiNetworkEvent event;
  while (true)
  {
    const auto remaining = deadline - xiiTime::Now();
    if (!remaining.IsPositive() || m_Host.WaitForEvent(event, remaining).Failed())
      return XII_FAILURE;
    if (event.m_Type == xiiNetworkEventType::MessageSent && event.m_Message == submission)
      return event.m_Error.GetResult();
    if (event.m_Type == xiiNetworkEventType::Closed)
      return XII_FAILURE;
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_PlayerSessionProtocol);
