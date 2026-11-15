/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Network/Serialization/Implementation/NetworkSerializerInternal.h>
#include <Foundation/Threading/Mutex.h>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkMessagingInternal
{
  constexpr xiiUInt32 Magic = 0x584E5231; // XNR1, version one.
  enum class Kind : xiiUInt8
  {
    Offer     = 1,
    Agreement = 2,
    Message   = 3,
    Request   = 4,
    Response  = 5,
    Cancel    = 6
  };
  struct Handler : xiiRefCountingImpl
  {
    xiiSharedPtr<xiiNetworkMessageHandler> m_pOwner;
    const xiiRTTI*                         m_pType        = nullptr;
    xiiUInt64                              m_uiGeneration = 0, m_uiRegistration = 0;
    xiiUInt32                              m_uiExecutor = 0;
  };
  struct Method
  {
    const xiiAbstractFunctionProperty*                             m_pFunction      = nullptr;
    xiiUInt64                                                      m_uiId           = 0;
    xiiUInt32                                                      m_uiMaximumBytes = 0, m_uiTimeout = 0;
    bool                                                           m_bFireAndForget = false, m_bReturns = false;
    xiiHybridArray<xiiNetworkReflectionInternal::CompiledValue, 4> m_Arguments;
    xiiNetworkReflectionInternal::CompiledValue                    m_Return;
    xiiDynamicArray<xiiUInt8>                                      m_Descriptor;
  };
  struct Service : xiiRefCountingImpl
  {
    xiiSharedPtr<xiiNetworkRpcService> m_pOwner;
    const xiiRTTI*                     m_pType = nullptr;
    xiiUInt64                          m_uiId = 0, m_uiGeneration = 0, m_uiRegistration = 0;
    xiiUInt32                          m_uiExecutor = 0;
    xiiDynamicArray<Method>            m_Methods;
  };
  struct Agreement
  {
    xiiUInt64                            m_uiType = 0;
    xiiSharedPtr<const xiiNetworkSchema> m_pLocal;
    xiiDynamicArray<xiiUInt8>            m_Remote;
    bool                                 m_bIncoming = false, m_bOutgoing = false, m_bExact = false;
  };
  struct Peer : xiiRefCountingImpl
  {
    xiiSharedPtr<const xiiNetworkSessionInfo> m_pInfo;
    xiiDynamicArray<Agreement>                m_Agreements;
    xiiDynamicArray<xiiUInt64>                m_Replay;
    xiiUInt64                                 m_uiNextRequest = 1, m_uiHighestRequest = 0;
    xiiTime                                   m_LastRefill;
    double                                    m_fTokens = 0;
    bool                                      m_bAlive  = true;
  };
  struct Packet
  {
    xiiNetworkEvent m_Event;
    Kind            m_Kind           = Kind::Message;
    xiiUInt32       m_uiExecutor     = 0;
    xiiUInt64       m_uiRegistration = 0, m_uiService = 0, m_uiMethod = 0, m_uiRequest = 0;
    xiiTime         m_Queued, m_Deadline;
    bool            m_bFireAndForget = false;
  };
  struct Pending
  {
    xiiNetworkRpcRequest      m_Request;
    xiiUInt64                 m_uiService = 0, m_uiMethod = 0;
    xiiDynamicArray<xiiUInt8> m_Descriptor;
    xiiTime                   m_Deadline;
  };
  xiiNetworkDataError   Transport(xiiNetworkError error);
  void                  Envelope(xiiNetworkBinaryWriter& writer, Kind kind);
  xiiNetworkDataError   Compatible(xiiArrayPtr<const xiiUInt8> sender, xiiArrayPtr<const xiiUInt8> receiver, xiiUInt32 uiDepth = 0);
  xiiNetworkDataError   CompileService(xiiNetworkTypeRegistryState& registry, Service& service, const xiiNetworkReflectionConfiguration& config);
  const Method*         FindMethod(const Service& service, xiiUInt64 uiMethod);
  xiiNetworkFieldSchema RpcPolicy(const xiiNetworkReflectionInternal::CompiledValue& value, const xiiNetworkSerializationLimits& limits);
} // namespace xiiNetworkMessagingInternal

class xiiNetworkReflectionEndpointState
{
public:
  xiiNetworkReflectionEndpointState(xiiNetworkHost& host, xiiNetworkTypeRegistry& registry);
  ~xiiNetworkReflectionEndpointState();
  void                                            OnPluginEvent(const xiiPluginEvent& event);
  xiiSharedPtr<xiiNetworkMessagingInternal::Peer> FindPeer(xiiNetworkChannel channel);
  xiiNetworkDataError                             Send(xiiNetworkChannel channel, xiiArrayPtr<const xiiUInt8> bytes);
  xiiNetworkDataError                             Reliable(xiiNetworkChannel channel);
  void                                            Complete(xiiUInt32 uiIndex, xiiNetworkDataError error, xiiArrayPtr<const xiiUInt8> value = {});
  void                                            Stop(xiiNetworkDataError error);
  xiiNetworkDataError                             Register(xiiUInt64 uiService, const xiiRTTI* pType, xiiSharedPtr<xiiNetworkRpcService> pOwner, xiiUInt32 uiExecutor);
  xiiNetworkDataError                             Process(xiiNetworkEvent&& event);
  xiiNetworkDataError                             Dispatch(xiiUInt32 uiExecutor, xiiUInt32 uiMaximum);
  xiiNetworkDataError                             DispatchMessage(xiiNetworkMessagingInternal::Packet& packet);
  xiiNetworkDataError                             DispatchRpc(xiiNetworkMessagingInternal::Packet& packet);
  xiiNetworkDataError                             Reply(const xiiNetworkMessagingInternal::Packet& packet, xiiNetworkDataError error, xiiArrayPtr<const xiiUInt8> bytes = {});
  xiiNetworkDataError                             Call(xiiNetworkChannel channel, xiiUInt64 uiService, xiiUInt64 uiMethod, xiiArrayPtr<const xiiVariant> arguments,
                                                       xiiNetworkRpcRequest& out_request, xiiTime timeout, bool bFireAndForget);
  xiiNetworkDataError                             Decode(const xiiNetworkRpcResult& result, xiiVariant& out_value);

  xiiNetworkHost&                                                             m_Host;
  xiiNetworkTypeRegistry&                                                     m_Registry;
  xiiMutex                                                                    m_Mutex;
  bool                                                                        m_bRunning           = false;
  xiiUInt64                                                                   m_uiNextRegistration = 1;
  xiiNetworkReflectionConfiguration                                           m_Config;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiNetworkMessagingInternal::Handler>> m_Handlers;
  xiiHashTable<xiiUInt64, xiiSharedPtr<xiiNetworkMessagingInternal::Service>> m_Services;
  xiiDynamicArray<xiiSharedPtr<xiiNetworkMessagingInternal::Peer>>            m_Peers;
  xiiDynamicArray<xiiNetworkMessagingInternal::Packet>                        m_Queue;
  xiiDynamicArray<xiiNetworkMessagingInternal::Pending>                       m_Pending;
  xiiDeque<xiiNetworkRpcResult>                                               m_Results;
  xiiNetworkReflectionStatistics                                              m_Statistics;
};
