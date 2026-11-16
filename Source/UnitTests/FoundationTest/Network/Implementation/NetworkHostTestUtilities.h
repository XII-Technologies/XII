/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkHost.h>
#include <TestFramework/Framework/TestFramework.h>

namespace xiiNetworkHostTestUtilities
{
  inline bool HostTestEvent(xiiNetworkHost& host, xiiNetworkEventType type, xiiNetworkEvent& out_event)
  {
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    for (xiiUInt32 uiCount = 0; uiCount < 1000; ++uiCount)
    {
      const auto remaining = deadline - xiiTime::Now();
      if (remaining.IsNegative())
      {
        break;
      }
      const auto error = host.WaitForEvent(out_event, remaining);
      if (!XII_TEST_BOOL_MSG(error.Succeeded(), "Wait %u: %s", static_cast<unsigned>(type), error.GetCodeName()))
      {
        return false;
      }
      if (out_event.m_Type == type)
      {
        return true;
      }
      if (out_event.m_Type == xiiNetworkEventType::Closed)
      {
        XII_TEST_BOOL_MSG(false, "Unexpected close: %s (%d)", out_event.m_Error.GetCodeName(), out_event.m_Error.m_iNativeCode);
        return false;
      }
    }
    return XII_TEST_BOOL(false);
  }

  inline xiiNetworkHostConfiguration HostTestConfiguration(xiiNetworkTransportType type)
  {
    xiiNetworkHostConfiguration configuration;
    configuration.m_Transport                      = type;
    configuration.m_uiMaximumConnections           = 8;
    configuration.m_uiMaximumChannelsPerConnection = 8;
    configuration.m_uiMaximumQueuedMessages        = 64;
    configuration.m_uiMaximumQueuedEvents          = 128;
    configuration.m_uiMaximumSessions              = 16;
    configuration.m_IO.m_uiWorkerThreadCount       = 1;
    configuration.m_IO.m_uiResolverThreadCount     = 1;
    configuration.m_HandshakeTimeout               = xiiTime::MakeFromSeconds(3);
    configuration.m_CloseTimeout                   = xiiTime::MakeFromMilliseconds(500);
    if (type == xiiNetworkTransportType::Tcp || type == xiiNetworkTransportType::NativeUdp)
    {
      configuration.m_Authentication            = xiiNetworkAuthenticationPolicy::AnonymousDevelopment;
      configuration.m_bAllowInsecureDevelopment = true;
    }
    return configuration;
  }
} // namespace xiiNetworkHostTestUtilities
