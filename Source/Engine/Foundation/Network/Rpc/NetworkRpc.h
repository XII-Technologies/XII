/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <Foundation/Network/Serialization/NetworkDataError.h>

/// Own the native target through this object. Only attributed reflected member functions can execute.
/// Authorization runs before argument decoding on the selected executor and must explicitly allow
/// each invocation. GetType/GetObject must stay constant; the owner must serialize target mutation.
class XII_FOUNDATION_DLL xiiNetworkRpcService : public xiiRefCounted
{
public:
  [[nodiscard]] virtual const xiiRTTI* GetType() const                                           = 0;
  [[nodiscard]] virtual void*          GetObject()                                               = 0;
  [[nodiscard]] virtual bool           Authorize(const xiiNetworkDispatchContext& context) const = 0;
};

struct XII_FOUNDATION_DLL xiiNetworkRpcServiceLimits
{
  xiiUInt32 m_uiRequestsPerSecond         = 1000;
  xiiUInt32 m_uiBurst                     = 128;
  xiiUInt32 m_uiMaximumConcurrentRequests = 128;
  xiiUInt32 m_uiMaximumResponseBytes      = 1024 * 1024;
};

struct XII_FOUNDATION_DLL xiiNetworkRpcRequest
{
  xiiNetworkChannel  m_Channel;
  xiiUInt64          m_uiId = 0;
  [[nodiscard]] bool IsValid() const { return m_Channel.IsValid() && m_uiId != 0; }
  bool               operator==(const xiiNetworkRpcRequest&) const = default;
};

/// Encoded results contain no plugin-owned native objects. Decode explicitly against a local,
/// declared service signature. Exactly one result is retained for every admitted correlated call.
struct XII_FOUNDATION_DLL xiiNetworkRpcResult
{
  xiiNetworkRpcRequest      m_Request;
  xiiUInt64                 m_uiService = 0;
  xiiUInt64                 m_uiMethod  = 0;
  xiiNetworkDataError       m_Error;
  xiiDynamicArray<xiiUInt8> m_Value;
  xiiDynamicArray<xiiUInt8> m_Signature;
};
