/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Types/Delegate.h>
#include <GameEngine/GameApplication/PlayerMessages.h>

/// Managed Player's three-message protocol. The owner pumps it on the Editor or game application thread.
/// An Editor instance can own multiple listeners on the same host.
class XII_GAMEENGINE_DLL xiiPlayerSessionProtocol final
{
public:
  template <typename T>
  using Handler = xiiDelegate<void(xiiNetworkChannel, const T&)>;

  xiiPlayerSessionProtocol();
  ~xiiPlayerSessionProtocol();

  xiiResult StartEditor(Handler<xiiPlayerStatusMessage> status);
  xiiResult StartPlayer(Handler<xiiPlayerSceneMessage> scene, Handler<xiiPlayerStopMessage> stop);

  xiiNetworkHost& GetHost() { return m_Host; }

  /// Re-register the built-in handlers and renew agreements after a plugin registry refresh.
  void Update();

  /// Pass Connected after checking the native PID against the expected process.
  /// Deliver subsequent events in host order. Runs any admitted handler before returning.
  xiiResult ProcessEvent(xiiNetworkEvent&& event);
  bool      IsReady(xiiNetworkChannel channel) const;

  template <typename T>
  xiiNetworkDataError Send(xiiNetworkChannel channel, const T& message, xiiNetworkMessage* out_pSubmission = nullptr)
  {
    return m_Reflection.SendMessage(channel, message, xiiNetworkEncoding::Tagged, out_pSubmission);
  }

  /// For Player teardown only. Waits for transport completion of the final status without invoking callbacks.
  xiiResult WaitForFinalStatus(xiiNetworkMessage submission, xiiTime timeout);

private:
  xiiResult Start(bool bEditor);
  xiiResult RegisterHandlers();
  xiiResult OfferSchemas(xiiNetworkChannel channel);
  bool      HasAgreement(xiiNetworkChannel channel, const xiiRTTI* pType) const;

  xiiNetworkHost                        m_Host;
  xiiNetworkTypeRegistry                m_Registry;
  xiiNetworkReflectionEndpoint          m_Reflection;
  Handler<xiiPlayerStatusMessage>       m_Status;
  Handler<xiiPlayerSceneMessage>        m_Scene;
  Handler<xiiPlayerStopMessage>         m_Stop;
  xiiHybridArray<xiiNetworkChannel, 16> m_Channels;
  xiiUInt64                             m_uiRegistryGeneration = 0;
  bool                                  m_bEditor              = false;
};
