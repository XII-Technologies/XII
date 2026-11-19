/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Session/NetworkSession.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

/// Stable delivery context captured at session establishment, never obtained from a recycled slot.
struct XII_FOUNDATION_DLL xiiNetworkDispatchContext
{
  xiiSharedPtr<const xiiNetworkSessionInfo> m_pSession;
  xiiNetworkChannel                         m_Channel;
  xiiUInt64                                 m_uiService = 0;
  xiiUInt64                                 m_uiMethod  = 0;
  xiiUInt64                                 m_uiRequest = 0;
  xiiTime                                   m_Deadline; ///< Local deadline of a correlated RPC. Zero for messages without a deadline.
};

/// Explicit incoming-message allowlist entry. The endpoint owns this handler until removal; active
/// dispatch retains it through the callback. GetType must remain constant. Callbacks may reenter the
/// endpoint, but must not unload plugins, mutate the registry, or destroy the endpoint.
class XII_FOUNDATION_DLL xiiNetworkMessageHandler : public xiiRefCounted
{
public:
  [[nodiscard]] virtual const xiiRTTI* GetType() const                                                        = 0;
  [[nodiscard]] virtual bool           Authorize(const xiiNetworkDispatchContext& context) const              = 0;
  virtual void                         Handle(const xiiNetworkDispatchContext& context, const void* pMessage) = 0;
};

/// Typed handler specialization; the callback receives the registered value type directly.
template <typename T>
class xiiNetworkTypedMessageHandler : public xiiNetworkMessageHandler
{
public:
  const xiiRTTI* GetType() const final { return xiiGetStaticRTTI<T>(); }
  void           Handle(const xiiNetworkDispatchContext& context, const void* pMessage) final
  {
    OnMessage(context, *static_cast<const T*>(pMessage));
  }
  virtual void OnMessage(const xiiNetworkDispatchContext& context, const T& message) = 0;
};
