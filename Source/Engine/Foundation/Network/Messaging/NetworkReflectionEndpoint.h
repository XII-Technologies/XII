/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Network/Rpc/NetworkRpc.h>
#include <Foundation/Network/Serialization/NetworkSerializer.h>
#include <Foundation/Time/Timestamp.h>
#include <type_traits>
#include <utility>

struct XII_FOUNDATION_DLL xiiNetworkReflectionConfiguration
{
  xiiNetworkSerializationLimits m_Serialization;
  xiiUInt32                     m_uiMaximumPeers             = 128;
  xiiUInt32                     m_uiMaximumHandlers          = 128;
  xiiUInt32                     m_uiMaximumServices          = 128;
  xiiUInt32                     m_uiMaximumMethodsPerService = 128;
  xiiUInt32                     m_uiMaximumQueuedPackets     = 1024;
  xiiUInt64                     m_uiMaximumQueuedBytes       = 16 * 1024 * 1024;
  /// Slots are reserved at admission, including completed results not yet dequeued.
  xiiUInt32 m_uiMaximumPendingCalls     = 256;
  xiiUInt32 m_uiMaximumSchemasPerPeer   = 128;
  xiiUInt64 m_uiMaximumSchemaCacheBytes = 8 * 1024 * 1024;
  xiiUInt32 m_uiMaximumSchemaBytes      = 262144;
  /// Finite out-of-order replay window. Older requests are rejected, never automatically retried.
  xiiUInt32 m_uiReplayWindow = 1024;
  /// Per-peer admission rate, shared across services. Both must be nonzero.
  xiiUInt32 m_uiRequestsPerSecond = 1000;
  xiiUInt32 m_uiRequestBurst      = 128;
  xiiTime   m_MaximumCallTimeout  = xiiTime::MakeFromSeconds(30);
};

struct XII_FOUNDATION_DLL xiiNetworkReflectionStatistics
{
  xiiUInt64 m_uiMessagesSent = 0, m_uiMessagesDispatched = 0, m_uiRejectedPackets = 0;
  xiiUInt64 m_uiRpcRequests = 0, m_uiRpcExecuted = 0, m_uiRpcCompleted = 0;
  xiiUInt64 m_uiUnauthorized = 0, m_uiTimeouts = 0, m_uiCancellations = 0;
  xiiUInt32 m_uiQueuedPackets = 0, m_uiPendingCalls = 0;
  xiiUInt64 m_uiQueuedBytes = 0;
  xiiTime   m_DispatchDelay, m_RpcExecutionTime;
};

class xiiNetworkReflectionEndpointState;

/// M4 protocol layered on M3 logical channels. Feed Connected/Message/Closed events in publication
/// order through ProcessEvent. Other host events remain the application's responsibility. Configure
/// both hosts with EnableProtocol before Start. Raw NativeUdp has no authenticated sessions and is
/// intentionally outside this endpoint; QUIC unreliable channels support typed datagrams.
///
/// Submissions/polling may be concurrent. ProcessEvent calls are serialized by the caller, preserving
/// host event order. Dispatch(executor) is called by the application on that executor's thread; only
/// one Dispatch per executor may run at a time. Different executors may run concurrently. Callbacks
/// never run under endpoint locks. Lifecycle Start/Shutdown/destruction require caller quiescence;
/// Shutdown completes pending calls and drops queued work. Host and registry outlive this facade.
class XII_FOUNDATION_DLL xiiNetworkReflectionEndpoint final
{
public:
  static constexpr xiiUInt64 ProtocolFeature = xiiUInt64(1) << 63;
  static void                EnableProtocol(xiiNetworkHostConfiguration& configuration);
  xiiNetworkReflectionEndpoint(xiiNetworkHost& host, xiiNetworkTypeRegistry& registry);
  ~xiiNetworkReflectionEndpoint();
  xiiNetworkReflectionEndpoint(const xiiNetworkReflectionEndpoint&)                = delete;
  xiiNetworkReflectionEndpoint&     operator=(const xiiNetworkReflectionEndpoint&) = delete;
  [[nodiscard]] xiiNetworkDataError Start(const xiiNetworkReflectionConfiguration& configuration = {});
  void                              Shutdown();
  [[nodiscard]] xiiNetworkDataError RegisterMessageHandler(xiiSharedPtr<xiiNetworkMessageHandler> pHandler, xiiUInt32 uiExecutor = 0);
  [[nodiscard]] xiiNetworkDataError RemoveMessageHandler(xiiUInt64 uiTypeId);
  /// Declares the local signature of a remote service without exposing a local target.
  [[nodiscard]] xiiNetworkDataError DeclareRemoteService(xiiUInt64 uiService, const xiiRTTI* pType);
  [[nodiscard]] xiiNetworkDataError RegisterService(xiiUInt64 uiService, xiiSharedPtr<xiiNetworkRpcService> pService, xiiUInt32 uiExecutor = 0, const xiiNetworkRpcServiceLimits& limits = {});
  [[nodiscard]] xiiNetworkDataError RemoveService(xiiUInt64 uiService);
  /// Offers a bounded recursive descriptor. A matching acknowledgment enables compact encoding;
  /// compatible evolved descriptors permit tagged encoding. No application object is constructed.
  [[nodiscard]] xiiNetworkDataError OfferSchema(xiiNetworkChannel channel, const xiiRTTI* pType);
  [[nodiscard]] xiiNetworkDataError SendMessage(xiiNetworkChannel channel, const xiiRTTI* pType, const void* pMessage, xiiNetworkEncoding encoding = xiiNetworkEncoding::Tagged, xiiNetworkMessage* out_pSubmission = nullptr, xiiUInt64 uiUserData = 0);
  [[nodiscard]] xiiNetworkDataError ProcessEvent(xiiNetworkEvent&& event);
  /// Runs at most uiMaximum callbacks on the selected executor. Rejections count as dispatched work.
  [[nodiscard]] xiiNetworkDataError Dispatch(xiiUInt32 uiExecutor, xiiUInt32 uiMaximum = 64);
  /// Must be pumped regularly by the application; no hidden timer/worker threads.
  void Update();
  /// Arguments match the declared reflected signature exactly. No automatic retry on any failure.
  /// Correlated calls require a reliable channel. Zero timeout selects the method's default.
  [[nodiscard]] xiiNetworkDataError CallRpc(xiiNetworkChannel channel, xiiUInt64 uiService, xiiUInt64 uiMethod,
                                            xiiArrayPtr<const xiiVariant> arguments, xiiNetworkRpcRequest& out_request, xiiTime timeout = xiiTime::MakeZero(), bool bFireAndForget = false);
  /// Local terminal cancellation; remote cancellation is best effort. Already executing functions
  /// cannot be interrupted and their side effects cannot be undone.
  [[nodiscard]] xiiNetworkDataError            CancelRpc(xiiNetworkRpcRequest request);
  [[nodiscard]] bool                           TryGetRpcResult(xiiNetworkRpcResult& out_result);
  [[nodiscard]] xiiNetworkDataError            DecodeRpcResult(const xiiNetworkRpcResult& result, xiiVariant& out_value);
  [[nodiscard]] xiiNetworkReflectionStatistics GetStatistics() const;
  template <typename T>
  [[nodiscard]] xiiNetworkDataError SendMessage(xiiNetworkChannel channel, const T& message, xiiNetworkEncoding encoding = xiiNetworkEncoding::Tagged, xiiNetworkMessage* out_pSubmission = nullptr, xiiUInt64 uiUserData = 0)
  {
    return SendMessage(channel, xiiGetStaticRTTI<T>(), &message, encoding, out_pSubmission, uiUserData);
  }

  /// Typed convenience. Timestamp results use microseconds in the variant API; this overload restores
  /// the native timestamp. Reflected return objects must have ordinary copy assignment semantics.
  template <typename T>
  [[nodiscard]] xiiNetworkDataError DecodeRpcResult(const xiiNetworkRpcResult& result, T& out_value)
  {
    xiiVariant value;
    auto       error = DecodeRpcResult(result, value);
    if (error.Failed())
      return error;
    if constexpr (std::is_same_v<T, xiiTimestamp>)
    {
      if (!value.IsA<xiiInt64>())
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidFieldType);
      out_value = xiiTimestamp::MakeFromInt(value.Get<xiiInt64>(), xiiSIUnitOfTime::Microsecond);
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
      out_value = std::move(value);
    else
    {
      bool matches = value.GetReflectedType() == xiiGetStaticRTTI<T>();
      if constexpr (std::is_same_v<T, xiiVariantArray>)
        matches = value.IsA<xiiVariantArray>();
      if constexpr (std::is_same_v<T, xiiVariantDictionary>)
        matches = value.IsA<xiiVariantDictionary>();
      if (!matches || value.GetData() == nullptr)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidFieldType);
      out_value = *static_cast<const T*>(value.GetData());
    }
    return {};
  }

private:
  xiiNetworkReflectionEndpointState* m_pState;
};
