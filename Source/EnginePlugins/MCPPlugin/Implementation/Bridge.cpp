/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Bridge.h>
#include <MCPPlugin/Json.h>

class xiiMCPBridgeMethods
{
public:
  xiiVariantDictionary Query(const xiiVariantDictionary& request);

  xiiMCPRegistry* m_pRegistry = nullptr;
  xiiString       m_sInstance;
  xiiString       m_sApplication;
  xiiString       m_sIdentity;
  xiiTime         m_Deadline;
  xiiUInt8        m_uiPermissions = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMCPBridgeMethods);

namespace
{
  constexpr xiiUInt64 ServiceId      = 0x5849494D43503031ULL;
  constexpr xiiUInt64 QueryMethod    = 1;
  constexpr xiiUInt8  AllPermissions = 31;

  xiiNetworkRpcAttribute* MakeQueryAttribute()
  {
    auto* pAttribute                     = new xiiNetworkRpcAttribute(QueryMethod);
    pAttribute->m_bAllowFireAndForget    = false;
    pAttribute->m_uiMaximumArgumentBytes = xiiMCPJson::DefaultMaximumBytes;
    pAttribute->m_uiTimeoutMilliseconds  = 5000;
    return pAttribute;
  }

  xiiVariantDictionary BridgeError(xiiStringView sMessage)
  {
    xiiVariantDictionary response;
    response["error"] = xiiString(sMessage);
    return response;
  }

  class xiiMCPBridgeService final : public xiiNetworkRpcService
  {
  public:
    const xiiRTTI* GetType() const override { return xiiGetStaticRTTI<xiiMCPBridgeMethods>(); }
    void*          GetObject() override { return &m_Methods; }

    bool Authorize(const xiiNetworkDispatchContext& context) const override
    {
      if (!context.m_pSession || context.m_uiService != ServiceId || context.m_uiMethod != QueryMethod ||
          context.m_Channel != context.m_pSession->m_DefaultChannel)
      {
        return false;
      }

      const auto& info = context.m_pSession->m_ConnectionInfo;
      if (!info.m_Capabilities.m_bAuthenticated || info.m_sAuthenticatedIdentity.IsEmpty() || !info.m_bIncoming)
      {
        return false;
      }

      // Authorization and invocation run serially on the same executor.
      m_Methods.m_sIdentity = info.m_sAuthenticatedIdentity;
      m_Methods.m_Deadline  = context.m_Deadline;
      return true;
    }

    mutable xiiMCPBridgeMethods m_Methods;
  };
} // namespace

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMCPBridgeMethods, xiiNoBase, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_FUNCTIONS
    {
      XII_FUNCTION_PROPERTY(Query)->AddAttributes(MakeQueryAttribute()),
    } XII_END_FUNCTIONS;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiNetworkIdentityAttribute(0x5849494D43500101ULL)} XII_END_ATTRIBUTES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiVariantDictionary xiiMCPBridgeMethods::Query(const xiiVariantDictionary& request)
{
  xiiUInt64   uiPermissions = 0;
  const auto* pPermissions  = request.GetValue("permissions");
  if (pPermissions == nullptr || !xiiMCPJson::ReadUInt(*pPermissions, uiPermissions) || uiPermissions > AllPermissions)
  {
    return BridgeError("Invalid permission mask.");
  }

  xiiMCPCallContext context;
  context.m_uiPermissions      = m_uiPermissions & static_cast<xiiUInt8>(uiPermissions);
  context.m_sIdentity          = m_sIdentity;
  context.m_Deadline           = m_Deadline;
  const auto           sAction = xiiMCPJson::GetString(request, "action");
  xiiVariantDictionary response;
  if (sAction == "identity")
  {
    response["instance"]    = m_sInstance;
    response["application"] = m_sApplication;
    response["processId"]   = static_cast<xiiUInt32>(xiiProcess::GetCurrentProcessID());
    response["revision"]    = m_pRegistry->GetRevision();
    return response;
  }

  if (xiiMCPJson::GetString(request, "instance") != m_sInstance)
  {
    return BridgeError("The target instance changed. Rediscover it before sending another operation.");
  }

  xiiUInt64   uiKind = 0;
  const auto* pKind  = request.GetValue("kind");
  if (pKind == nullptr || !xiiMCPJson::ReadUInt(*pKind, uiKind) || uiKind >= static_cast<xiiUInt8>(xiiMCPCapability::Count))
  {
    return BridgeError("Invalid capability kind.");
  }

  const auto kind = static_cast<xiiMCPCapability>(uiKind);
  if (sAction == "list")
  {
    xiiUInt64   uiOffset = 0;
    const auto* pOffset  = request.GetValue("offset");
    if (pOffset != nullptr && !xiiMCPJson::ReadUInt(*pOffset, uiOffset))
    {
      return BridgeError("Invalid capability offset.");
    }

    xiiVariantArray entries;
    m_pRegistry->List(kind, context, entries);
    if (uiOffset > entries.GetCount())
    {
      return BridgeError("Capability list changed. Restart discovery.");
    }

    xiiVariantArray page;
    const auto      uiEnd = xiiMath::Min(entries.GetCount(), static_cast<xiiUInt32>(uiOffset) + 8);
    for (xiiUInt32 uiIndex = static_cast<xiiUInt32>(uiOffset); uiIndex < uiEnd; ++uiIndex)
    {
      page.PushBack(entries[uiIndex]);
    }
    response["entries"]  = std::move(page);
    response["revision"] = m_pRegistry->GetRevision();
    if (uiEnd < entries.GetCount())
    {
      response["nextOffset"] = uiEnd;
    }
    return response;
  }

  if (sAction != "invoke")
  {
    return BridgeError("Unknown bridge operation.");
  }

  const auto* pArguments = xiiMCPJson::GetObject(request, "arguments");
  if (pArguments == nullptr)
  {
    return BridgeError("Operation arguments must be an object.");
  }

  xiiVariantDictionary result;
  const auto           sKey   = xiiMCPJson::GetString(request, "key");
  const auto           status = kind == xiiMCPCapability::Resource || kind == xiiMCPCapability::ResourceTemplate ?
    m_pRegistry->ReadResource(sKey, context, result) :
    m_pRegistry->Invoke(kind, sKey, context, *pArguments, result);
  if (status.Failed())
  {
    return BridgeError(status.GetMessageString());
  }
  response["value"] = std::move(result);
  return response;
}

class xiiMCPBridgeState
{
public:
  xiiMCPBridgeState() :
    m_Reflection(m_Host, m_TypeRegistry)
  {
  }

  ~xiiMCPBridgeState()
  {
    m_Discovery.Reset();
    m_Reflection.Shutdown();
    m_Host.Shutdown();
  }

  xiiStatus Start()
  {
    xiiNetworkHostConfiguration config;
    config.m_Transport                      = xiiNetworkTransportType::Local;
    config.m_Authentication                 = xiiNetworkAuthenticationPolicy::LocalUser;
    config.m_Protocol                       = xiiUuid(ServiceId, 1);
    config.m_uiMaximumConnections           = 32;
    config.m_uiMaximumChannelsPerConnection = 2;
    config.m_uiMaximumMessageBytes          = 512 * 1024;
    xiiNetworkReflectionEndpoint::EnableProtocol(config);
    const auto error = m_Host.Start(config);
    if (error.Failed())
    {
      return xiiStatus(xiiFmt("Could not start Local MCP transport ({0}).", error.GetCodeName()));
    }

    xiiNetworkReflectionConfiguration reflection;
    reflection.m_uiMaximumPeers         = config.m_uiMaximumConnections;
    reflection.m_uiMaximumPendingCalls  = 32;
    reflection.m_uiMaximumQueuedPackets = 64;
    reflection.m_uiMaximumQueuedBytes   = 4 * 1024 * 1024;
    reflection.m_uiRequestsPerSecond    = 32;
    reflection.m_uiRequestBurst         = 8;
    reflection.m_MaximumCallTimeout     = xiiTime::MakeFromSeconds(10);
    if (m_Reflection.Start(reflection).Failed() || RegisterMethods().Failed())
    {
      return xiiStatus("Could not register the reflected MCP protocol.");
    }

    m_uiTypeGeneration = m_TypeRegistry.GetStatistics().m_uiGeneration;
    return XII_SUCCESS;
  }

  xiiResult RegisterMethods()
  {
    if (!m_pService)
    {
      return m_Reflection.DeclareRemoteService(ServiceId, xiiGetStaticRTTI<xiiMCPBridgeMethods>()).GetResult();
    }
    xiiNetworkRpcServiceLimits limits;
    limits.m_uiRequestsPerSecond         = 32;
    limits.m_uiBurst                     = 8;
    limits.m_uiMaximumConcurrentRequests = 8;
    limits.m_uiMaximumResponseBytes      = 256 * 1024;
    return m_Reflection.RegisterService(ServiceId, m_pService, 0, limits).GetResult();
  }

  void Pump()
  {
    const auto uiGeneration = m_TypeRegistry.GetStatistics().m_uiGeneration;
    if (uiGeneration != m_uiTypeGeneration)
    {
      // Startup can register the service before the registry processes AfterPluginChanges.
      // Replace the descriptor even when the endpoint missed the preceding invalidation event.
      m_Reflection.RemoveService(ServiceId).IgnoreResult();
      if (RegisterMethods().Failed())
      {
        return;
      }
      m_uiTypeGeneration = uiGeneration;
    }

    xiiNetworkEvent event;
    for (xiiUInt32 uiCount = 0; uiCount < 64 && m_Host.TryGetEvent(event); ++uiCount)
    {
      const auto hConnection = event.m_Connection;
      if (event.m_Type == xiiNetworkEventType::Connected || event.m_Type == xiiNetworkEventType::Message ||
          event.m_Type == xiiNetworkEventType::ChannelClosed || event.m_Type == xiiNetworkEventType::Closed)
      {
        if (m_Reflection.ProcessEvent(std::move(event)).Failed())
        {
          m_Host.Disconnect(hConnection, false).IgnoreResult();
        }
      }
    }

    m_Reflection.Dispatch(0, 4).IgnoreResult();
    m_Reflection.Update();
    xiiNetworkRpcResult result;
    while (m_Reflection.TryGetRpcResult(result))
    {
      if (m_Pending.Contains(result.m_Request))
      {
        m_Completed.PushBack(std::move(result));
      }
    }
  }

  xiiTelemetryDiscovery                   m_Discovery;
  xiiNetworkHost                          m_Host;
  xiiNetworkTypeRegistry                  m_TypeRegistry;
  xiiNetworkReflectionEndpoint            m_Reflection;
  xiiSharedPtr<xiiMCPBridgeService>       m_pService;
  xiiHybridArray<xiiNetworkRpcRequest, 8> m_Pending;
  xiiHybridArray<xiiNetworkRpcResult, 8>  m_Completed;
  xiiUInt64                               m_uiTypeGeneration = 0;
};

xiiMCPBridge::xiiMCPBridge()  = default;
xiiMCPBridge::~xiiMCPBridge() = default;

xiiStatus xiiMCPBridge::StartClient()
{
  if (m_pState)
  {
    return xiiStatus("The native MCP bridge is already running.");
  }

  xiiUniquePtr<xiiMCPBridgeState> pState = XII_DEFAULT_NEW(xiiMCPBridgeState);
  const auto                      status = pState->Start();
  if (status.Succeeded())
  {
    m_pState = std::move(pState);
  }
  return status;
}

xiiStatus xiiMCPBridge::StartServer(xiiMCPRegistry& registry, xiiStringView sApplication, xiiStringView sEndpoint, xiiUInt8 uiPermissions, bool bAdvertise)
{
  if (m_pState)
  {
    return xiiStatus("The native MCP bridge is already running.");
  }

  xiiTelemetryInstance instance;
  instance.m_Instance     = xiiUuid::MakeUuid();
  instance.m_sApplication = sApplication;
  instance.m_uiProcessId  = xiiProcess::GetCurrentProcessID();
  xiiStringBuilder sInstance;
  xiiConversionUtils::ToString(instance.m_Instance, sInstance);
  if (sEndpoint.IsEmpty())
  {
    xiiStringBuilder sAutomatic;
    sAutomatic.SetFormat("xii-mcp-{}-{}", instance.m_uiProcessId, sInstance.GetView().GetSubString(2, 8));
    sAutomatic.ToLower();
    instance.m_sEndpoint = sAutomatic;
  }
  else
  {
    instance.m_sEndpoint = sEndpoint;
  }

  xiiNetworkLocalEndpoint endpoint;
  if (xiiNetworkLocalEndpoint::Make(instance.m_sEndpoint, endpoint).Failed())
  {
    return xiiStatus("Invalid Local MCP endpoint name.");
  }

  xiiUniquePtr<xiiMCPBridgeState> pState        = XII_DEFAULT_NEW(xiiMCPBridgeState);
  pState->m_pService                            = XII_DEFAULT_NEW(xiiMCPBridgeService);
  pState->m_pService->m_Methods.m_pRegistry     = &registry;
  pState->m_pService->m_Methods.m_sInstance     = sInstance;
  pState->m_pService->m_Methods.m_sApplication  = sApplication;
  pState->m_pService->m_Methods.m_uiPermissions = uiPermissions & AllPermissions;
  const auto status                             = pState->Start();
  if (status.Failed())
  {
    return status;
  }

  xiiNetworkListener hListener;
  const auto         error = pState->m_Host.Listen(endpoint, hListener);
  if (error.Failed())
  {
    return xiiStatus(xiiFmt("Could not listen on Local MCP endpoint '{0}' ({1}).", instance.m_sEndpoint, error.GetCodeName()));
  }
  if (bAdvertise && pState->m_Discovery.Publish(instance, xiiTelemetryDiscoveryScope::ModelContextProtocol).Failed())
  {
    return xiiStatus("Could not publish the local MCP advertisement.");
  }

  m_pState = std::move(pState);
  return XII_SUCCESS;
}

void xiiMCPBridge::Stop()
{
  m_pState.Clear();
}

void xiiMCPBridge::Update()
{
  if (m_pState)
  {
    m_pState->Pump();
  }
}

xiiStatus xiiMCPBridge::Connect(xiiStringView sEndpoint, xiiNetworkConnection& out_hConnection)
{
  xiiNetworkLocalEndpoint endpoint;
  if (!m_pState || xiiNetworkLocalEndpoint::Make(sEndpoint, endpoint).Failed())
  {
    return xiiStatus("Start the bridge and provide a valid Local endpoint.");
  }

  const auto error = m_pState->m_Host.Connect(endpoint, {}, out_hConnection);
  return error.Succeeded() ? xiiStatus(XII_SUCCESS) : xiiStatus(xiiFmt("Connection submission failed ({0}).", error.GetCodeName()));
}

void xiiMCPBridge::Disconnect(xiiNetworkConnection hConnection)
{
  if (m_pState)
  {
    m_pState->m_Host.Disconnect(hConnection).IgnoreResult();
  }
}

bool xiiMCPBridge::IsConnected(xiiNetworkConnection hConnection) const
{
  xiiNetworkConnectionInfo info;
  return m_pState && m_pState->m_Host.GetConnectionInfo(hConnection, info).Succeeded() && info.m_State == xiiNetworkConnectionState::Connected;
}

void xiiMCPBridge::SetServerPermissions(xiiUInt8 uiPermissions)
{
  uiPermissions &= AllPermissions;
  if (m_pState && m_pState->m_pService && m_pState->m_pService->m_Methods.m_uiPermissions != uiPermissions)
  {
    auto& service           = m_pState->m_pService->m_Methods;
    service.m_uiPermissions = uiPermissions;
    service.m_pRegistry->NotifyAvailabilityChanged();
  }
}

xiiStatus xiiMCPBridge::BeginCall(xiiNetworkConnection hConnection, const xiiVariantDictionary& request, xiiTime deadline, xiiNetworkRpcRequest& out_request)
{
  if (!m_pState || !IsConnected(hConnection))
  {
    return xiiStatus("The selected XII target is disconnected.");
  }

  if (m_pState->m_Pending.GetCount() >= 32)
  {
    return xiiStatus("The native MCP request queue is full.");
  }

  Update();
  xiiNetworkChannel channel;
  if (m_pState->m_Host.GetDefaultChannel(hConnection, channel).Failed())
  {
    return xiiStatus("The selected XII target has no active channel.");
  }

  const auto remaining = deadline - xiiTime::Now();
  if (!remaining.IsPositive())
  {
    return xiiStatus("The request deadline expired.");
  }
  const xiiVariant     arguments[] = {request};
  xiiNetworkRpcRequest pending;
  const auto           admission = m_pState->m_Reflection.CallRpc(channel, ServiceId, QueryMethod, arguments, pending, remaining);
  if (admission.Failed())
  {
    return xiiStatus(xiiFmt("Could not send the operation to the selected XII target ({0}).", admission.GetCodeName()));
  }

  m_pState->m_Pending.PushBack(pending);
  out_request = pending;
  return XII_SUCCESS;
}

bool xiiMCPBridge::PollCall(xiiNetworkRpcRequest request, xiiStatus& out_status, xiiVariantDictionary& out_response)
{
  if (!m_pState || !m_pState->m_Pending.Contains(request))
  {
    out_status = xiiStatus("The native MCP request is no longer active.");
    return true;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < m_pState->m_Completed.GetCount(); ++uiIndex)
  {
    if (m_pState->m_Completed[uiIndex].m_Request != request)
    {
      continue;
    }
    auto result = std::move(m_pState->m_Completed[uiIndex]);
    m_pState->m_Completed.RemoveAtAndSwap(uiIndex);
    m_pState->m_Pending.RemoveAndSwap(request);
    if (result.m_Error.Failed())
    {
      out_status = xiiStatus(xiiFmt("The XII target did not complete the operation ({0}).", result.m_Error.GetCodeName()));
      return true;
    }
    const auto decodeError = m_pState->m_Reflection.DecodeRpcResult(result, out_response);
    if (decodeError.Failed())
    {
      out_status = xiiStatus(xiiFmt("Could not decode the XII target response ({0}).", decodeError.GetCodeName()));
      return true;
    }
    const auto sError = xiiMCPJson::GetString(out_response, "error");
    out_status        = sError.IsEmpty() ? xiiStatus(XII_SUCCESS) : xiiStatus(sError);
    return true;
  }
  return false;
}

void xiiMCPBridge::CancelCall(xiiNetworkRpcRequest request)
{
  if (!m_pState || !m_pState->m_Pending.RemoveAndSwap(request))
  {
    return;
  }
  for (xiiUInt32 uiIndex = 0; uiIndex < m_pState->m_Completed.GetCount(); ++uiIndex)
  {
    if (m_pState->m_Completed[uiIndex].m_Request == request)
    {
      m_pState->m_Completed.RemoveAtAndSwap(uiIndex);
      return;
    }
  }
  m_pState->m_Reflection.CancelRpc(request).IgnoreResult();
}

xiiStatus xiiMCPBridge::Call(xiiNetworkConnection hConnection, const xiiVariantDictionary& request, xiiTime deadline, xiiVariantDictionary& out_response, const xiiAtomicBool* pCanceled)
{
  if (pCanceled != nullptr && *pCanceled)
  {
    return xiiStatus("The operation was canceled before submission.");
  }
  xiiNetworkRpcRequest pending;
  XII_SUCCEED_OR_RETURN(BeginCall(hConnection, request, deadline, pending));
  do
  {
    Update();
    xiiStatus status(XII_SUCCESS);
    if (PollCall(pending, status, out_response))
    {
      return status;
    }
    if (pCanceled != nullptr && *pCanceled)
    {
      CancelCall(pending);
      return xiiStatus("Cancellation was requested. The target may already have completed the operation.");
    }
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  } while (xiiTime::Now() < deadline);

  CancelCall(pending);
  return xiiStatus("The XII target did not respond before the deadline.");
}

xiiResult xiiMCPBridge::Discover(xiiDynamicArray<xiiTelemetryInstance>& out_instances)
{
  return xiiTelemetryDiscovery::Enumerate(out_instances, xiiTelemetryDiscoveryScope::ModelContextProtocol);
}

XII_STATICLINK_FILE(MCPPlugin, MCPPlugin_Implementation_Bridge);
