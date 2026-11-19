/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCP/Gateway.h>
#include <MCPPlugin/Json.h>

namespace
{
  constexpr xiiUInt32 MaximumTargets               = 24;
  constexpr xiiUInt32 MaximumCapabilitiesPerTarget = 64;

  xiiString MakeTargetNamespace(xiiStringView sInstance)
  {
    if (sInstance.IsEmpty())
    {
      return {};
    }

    xiiStringBuilder sNamespace("xii-");
    for (const char* pCharacter = sInstance.GetStartPointer(); pCharacter < sInstance.GetEndPointer(); ++pCharacter)
    {
      if (xiiStringUtils::IsHexDigit(static_cast<xiiUInt8>(*pCharacter)))
      {
        sNamespace.Append(*pCharacter);
      }
    }
    return xiiString(sNamespace);
  }

  xiiVariantDictionary CreateBridgeRequest(xiiStringView sAction, xiiUInt8 uiPermissions, xiiStringView sInstance = {})
  {
    xiiVariantDictionary request;
    request["action"]      = xiiString(sAction);
    request["permissions"] = uiPermissions;
    request["instance"]    = xiiString(sInstance);
    return request;
  }

  const char* GetCapabilityField(xiiMCPCapability kind)
  {
    switch (kind)
    {
      case xiiMCPCapability::Resource:
        return "uri";
      case xiiMCPCapability::ResourceTemplate:
        return "uriTemplate";
      default:
        return "name";
    }
  }
} // namespace

xiiMCPGateway::xiiMCPGateway() :
  m_Protocol(m_Registry),
  m_Http(m_Protocol)
{
}

xiiStatus xiiMCPGateway::Start(const xiiMCPGatewayConfiguration& configuration)
{
  m_Configuration = configuration;
  for (const auto& sEndpoint : configuration.m_Endpoints)
  {
    xiiNetworkLocalEndpoint endpoint;
    if (xiiNetworkLocalEndpoint::Make(sEndpoint, endpoint).Failed())
    {
      return xiiStatus(xiiFmt("Invalid Local MCP endpoint '{0}'.", sEndpoint));
    }
    AddTarget(sEndpoint, {}, {}, true);
  }

  if (m_Registry.RegisterTool("xii.targets", "List configured and discovered XII instances, their connection health, and their published capabilities.",
                              R"({"type":"object","additionalProperties":false})", xiiMCPPermission::Read, xiiMakeDelegate(&xiiMCPGateway::ListTargets, this))
        .Failed())
  {
    return xiiStatus("Could not register the gateway target directory.");
  }

  const auto status = m_Bridge.StartClient();
  if (status.Failed())
  {
    return status;
  }
  if (configuration.m_Http.m_Endpoint.GetAddress().IsValid())
  {
    return m_Http.Start(configuration.m_Http);
  }
  return XII_SUCCESS;
}

void xiiMCPGateway::AddTarget(xiiStringView sEndpoint, xiiStringView sInstance, xiiStringView sApplication, bool bExplicit)
{
  if (m_Targets.GetCount() >= MaximumTargets || m_Targets.Contains(sEndpoint) || m_BlockedTargets.Contains(sEndpoint))
  {
    return;
  }
  Target target;
  target.m_sEndpoint         = sEndpoint;
  target.m_sExpectedInstance = sInstance;
  target.m_sApplication      = sApplication;
  target.m_bExplicit         = bExplicit;
  m_Targets.Insert(xiiString(sEndpoint), std::move(target));
}

xiiStatus xiiMCPGateway::SetTargetEnabled(xiiStringView sEndpoint, bool bEnabled)
{
  xiiNetworkLocalEndpoint endpoint;
  if (xiiNetworkLocalEndpoint::Make(sEndpoint, endpoint).Failed())
  {
    return xiiStatus("Enter a valid Local endpoint name.");
  }
  if (bEnabled)
  {
    if (!m_Targets.Contains(sEndpoint) && m_Targets.GetCount() >= MaximumTargets)
    {
      return xiiStatus("The gateway target limit has been reached.");
    }
    m_BlockedTargets.Remove(sEndpoint);
    AddTarget(sEndpoint, {}, {}, true);
  }
  else if (auto target = m_Targets.Find(sEndpoint); target.IsValid())
  {
    if (m_BlockedTargets.GetCount() == 128)
    {
      return xiiStatus("The disconnected target list is full. Reconnect an endpoint first.");
    }
    m_Bridge.CancelCall(target.Value().m_PendingDiscovery);
    RemoveCapabilities(target.Value());
    m_Bridge.Disconnect(target.Value().m_hConnection);
    m_Targets.Remove(target);
    m_BlockedTargets.Insert(xiiString(sEndpoint));
  }
  return XII_SUCCESS;
}

void xiiMCPGateway::RefreshTargets()
{
  xiiDynamicArray<xiiTelemetryInstance> live;
  if (xiiMCPBridge::Discover(live).Failed())
  {
    return;
  }

  for (auto it = m_Targets.GetIterator(); it.IsValid();)
  {
    auto& target      = it.Value();
    bool  bAdvertised = false;
    for (const auto& instance : live)
    {
      if (instance.m_sEndpoint == target.m_sEndpoint)
      {
        xiiStringBuilder sId;
        xiiConversionUtils::ToString(instance.m_Instance, sId);
        bAdvertised = target.m_sExpectedInstance == sId;
        break;
      }
    }

    if (!target.m_bExplicit && !bAdvertised)
    {
      m_Bridge.CancelCall(target.m_PendingDiscovery);
      RemoveCapabilities(target);
      if (target.m_hConnection.IsValid())
      {
        m_Bridge.Disconnect(target.m_hConnection);
      }
      it = m_Targets.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  for (const auto& instance : live)
  {
    xiiStringBuilder sId;
    xiiConversionUtils::ToString(instance.m_Instance, sId);
    AddTarget(instance.m_sEndpoint, sId, instance.m_sApplication, false);
  }
}

void xiiMCPGateway::Update()
{
  m_Bridge.Update();
  const auto now = xiiTime::Now();
  if (m_Configuration.m_bDiscovery && now >= m_NextDiscovery)
  {
    RefreshTargets();
    m_NextDiscovery = now + xiiTime::MakeFromSeconds(2);
  }

  if (!m_Targets.IsEmpty())
  {
    const xiiUInt32 uiIndex = m_uiNextTarget++ % m_Targets.GetCount();
    auto            it      = m_Targets.GetIterator();
    for (xiiUInt32 uiSkip = 0; uiSkip < uiIndex; ++uiSkip)
    {
      ++it;
    }
    UpdateTarget(it.Value(), now);
  }

  m_Http.Update();
}

void xiiMCPGateway::UpdateTarget(Target& ref_target, xiiTime now)
{
  if (ref_target.m_PendingDiscovery.IsValid())
  {
    xiiStatus            status(XII_SUCCESS);
    xiiVariantDictionary response;
    if (!m_Bridge.PollCall(ref_target.m_PendingDiscovery, status, response))
    {
      if (now < ref_target.m_DiscoveryDeadline && m_Bridge.IsConnected(ref_target.m_hConnection))
      {
        return;
      }
      m_Bridge.CancelCall(ref_target.m_PendingDiscovery);
      status = xiiStatus("Target discovery expired or disconnected.");
    }
    ref_target.m_PendingDiscovery = {};
    if (status.Failed())
    {
      ref_target.m_sConnectionStatus = status.GetMessageString();
    }
    else if (ref_target.m_DiscoveryKind == xiiMCPCapability::Count ? AcceptIdentity(ref_target, response) : AcceptCapabilityPage(ref_target, response))
    {
      return;
    }
    else
    {
      ref_target.m_sConnectionStatus = "Target capabilities changed during discovery or could not be published.";
    }

    ref_target.m_Draft.Clear();
    ref_target.m_DiscoveryKind = xiiMCPCapability::Count;
    RemoveCapabilities(ref_target);
    return;
  }
  if (now < ref_target.m_NextAttempt)
  {
    return;
  }

  if (!m_Bridge.IsConnected(ref_target.m_hConnection))
  {
    RemoveCapabilities(ref_target);
    if (ref_target.m_hConnection.IsValid())
    {
      m_Bridge.Disconnect(ref_target.m_hConnection);
      ref_target.m_hConnection = {};
    }
    const auto status              = m_Bridge.Connect(ref_target.m_sEndpoint, ref_target.m_hConnection);
    ref_target.m_sConnectionStatus = status.Succeeded() ? xiiString("Connecting") : status.GetMessageString();
    ref_target.m_NextAttempt       = now + xiiTime::MakeFromSeconds(status.Succeeded() ? 1 : 3);
    return;
  }

  ref_target.m_NextAttempt       = now + xiiTime::MakeFromSeconds(2);
  ref_target.m_DiscoveryDeadline = now + xiiTime::MakeFromMilliseconds(600);
  const auto status              = StartDiscoveryCall(ref_target);
  if (status.Failed())
  {
    ref_target.m_sConnectionStatus = status.GetMessageString();
    RemoveCapabilities(ref_target);
  }
}

xiiStatus xiiMCPGateway::StartDiscoveryCall(Target& ref_target)
{
  const bool bIdentity = ref_target.m_DiscoveryKind == xiiMCPCapability::Count;
  auto       request   = CreateBridgeRequest(bIdentity ? "identity" : "list", m_Configuration.m_uiPermissions,
                                             bIdentity ? xiiStringView() : ref_target.m_sInstance.GetView());
  if (!bIdentity)
  {
    request["kind"]   = static_cast<xiiUInt8>(ref_target.m_DiscoveryKind);
    request["offset"] = ref_target.m_uiDiscoveryOffset;
  }
  return m_Bridge.BeginCall(ref_target.m_hConnection, request, ref_target.m_DiscoveryDeadline, ref_target.m_PendingDiscovery);
}

bool xiiMCPGateway::AcceptIdentity(Target& ref_target, const xiiVariantDictionary& identity)
{
  const auto sInstance = xiiMCPJson::GetString(identity, "instance");
  if (!xiiConversionUtils::IsStringUuid(sInstance) || (!ref_target.m_sExpectedInstance.IsEmpty() && sInstance != ref_target.m_sExpectedInstance))
  {
    m_Bridge.Disconnect(ref_target.m_hConnection);
    return false;
  }
  if (ref_target.m_sExpectedInstance.IsEmpty())
  {
    ref_target.m_sExpectedInstance = sInstance;
  }

  ref_target.m_sInstance    = sInstance;
  ref_target.m_sApplication = xiiMCPJson::GetString(identity, "application");
  if (const auto* pId = identity.GetValue("processId"); pId != nullptr && pId->IsNumber())
  {
    ref_target.m_uiProcessId = pId->ConvertTo<xiiUInt32>();
  }
  const auto* pRevision  = identity.GetValue("revision");
  xiiUInt64   uiRevision = 0;
  if (pRevision == nullptr || !xiiMCPJson::ReadUInt(*pRevision, uiRevision))
  {
    return false;
  }
  if (ref_target.m_bPublished && uiRevision == ref_target.m_uiRevision)
  {
    return true;
  }

  ref_target.m_DiscoveryKind       = xiiMCPCapability::Tool;
  ref_target.m_uiDiscoveryRevision = uiRevision;
  ref_target.m_uiDiscoveryOffset   = 0;
  ref_target.m_DiscoveryDeadline   = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  ref_target.m_Draft.Clear();
  return StartDiscoveryCall(ref_target).Succeeded();
}

bool xiiMCPGateway::AcceptCapabilityPage(Target& ref_target, const xiiVariantDictionary& response)
{
  const auto* pEntries   = response.GetValue("entries");
  const auto* pRevision  = response.GetValue("revision");
  xiiUInt64   uiRevision = 0;
  if (pEntries == nullptr || !pEntries->IsA<xiiVariantArray>() || pRevision == nullptr ||
      !xiiMCPJson::ReadUInt(*pRevision, uiRevision) || uiRevision != ref_target.m_uiDiscoveryRevision)
  {
    return false;
  }

  for (const auto& entry : pEntries->Get<xiiVariantArray>())
  {
    if (!entry.IsA<xiiVariantDictionary>() || ref_target.m_Draft.GetCount() >= MaximumCapabilitiesPerTarget)
    {
      return false;
    }
    ref_target.m_Draft.PushBack(RemoteCapability{ref_target.m_DiscoveryKind, entry.Get<xiiVariantDictionary>()});
  }

  if (const auto* pOffset = response.GetValue("nextOffset"))
  {
    xiiUInt64 uiOffset = 0;
    if (!xiiMCPJson::ReadUInt(*pOffset, uiOffset) || uiOffset <= ref_target.m_uiDiscoveryOffset || uiOffset >= MaximumCapabilitiesPerTarget)
    {
      return false;
    }
    ref_target.m_uiDiscoveryOffset = static_cast<xiiUInt32>(uiOffset);
  }
  else
  {
    ref_target.m_uiDiscoveryOffset = 0;
    ref_target.m_DiscoveryKind     = static_cast<xiiMCPCapability>(static_cast<xiiUInt8>(ref_target.m_DiscoveryKind) + 1U);
  }
  if (ref_target.m_DiscoveryKind != xiiMCPCapability::Count)
  {
    return StartDiscoveryCall(ref_target).Succeeded();
  }

  RemoveCapabilities(ref_target);
  ref_target.m_bPublished = PublishCapabilities(ref_target);
  ref_target.m_Draft.Clear();
  if (!ref_target.m_bPublished)
  {
    return false;
  }
  ref_target.m_uiRevision        = ref_target.m_uiDiscoveryRevision;
  ref_target.m_sConnectionStatus = "Ready";
  return true;
}

void xiiMCPGateway::RemoveCapabilities(Target& ref_target)
{
  for (const auto& capability : ref_target.m_Capabilities)
  {
    m_Registry.Remove(capability.m_Kind, capability.m_sKey);
  }
  ref_target.m_Capabilities.Clear();
  ref_target.m_bPublished = false;
}

bool xiiMCPGateway::PublishCapabilities(Target& ref_target)
{
  const auto sPrefix = MakeTargetNamespace(ref_target.m_sInstance);
  for (auto& entry : ref_target.m_Draft)
  {
    const auto kind       = entry.m_Kind;
    auto&      descriptor = entry.m_Descriptor;
    const auto sRemoteKey = xiiString(xiiMCPJson::GetString(descriptor, GetCapabilityField(kind)));
    if (sRemoteKey.IsEmpty())
    {
      return false;
    }
    const bool      bResource  = kind == xiiMCPCapability::Resource || kind == xiiMCPCapability::ResourceTemplate;
    const xiiString sPublicKey = bResource ? xiiString(xiiStringBuilder("xii-mcp://", sPrefix, "/", sRemoteKey)) :
                                             xiiString(xiiStringBuilder(sPrefix, ".", sRemoteKey));
    auto            forward    = [this, sEndpoint = ref_target.m_sEndpoint, sInstance = ref_target.m_sInstance, kind, sRemoteKey, sPublicKey](
                                   const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result) {
      return Forward(sEndpoint, sInstance, kind, sRemoteKey, sPublicKey, context, arguments, out_result);
    };
    auto permission = xiiMCPPermission::Read;
    if (const auto* pMetadata = xiiMCPJson::GetObject(descriptor, "_meta"))
    {
      const auto* pPermission  = pMetadata->GetValue("xii/permission");
      xiiUInt64   uiPermission = 0;
      if (pPermission == nullptr || !xiiMCPJson::ReadUInt(*pPermission, uiPermission) ||
          uiPermission == 0 || uiPermission > static_cast<xiiUInt8>(xiiMCPPermission::Administration) ||
          (uiPermission & (uiPermission - 1)) != 0)
      {
        return false;
      }
      permission = static_cast<xiiMCPPermission>(uiPermission);
    }
    if (m_Registry.Register(kind, sPublicKey, std::move(descriptor), permission, std::move(forward)).Failed())
    {
      return false;
    }
    ref_target.m_Capabilities.PushBack(Capability{kind, sPublicKey});
  }
  return true;
}

xiiStatus xiiMCPGateway::Forward(xiiStringView sEndpoint, xiiStringView sInstance, xiiMCPCapability kind, xiiStringView sRemoteKey, xiiStringView sPublicKey,
                                 const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  const auto it = m_Targets.Find(sEndpoint);
  if (!it.IsValid() || !it.Value().m_bPublished || it.Value().m_sInstance != sInstance)
  {
    return xiiStatus("The selected instance is disconnected. Rediscover capabilities before issuing another request.");
  }

  const auto start   = xiiTime::Now();
  auto       request = CreateBridgeRequest("invoke", context.m_uiPermissions & m_Configuration.m_uiPermissions, sInstance);
  request["kind"]    = static_cast<xiiUInt8>(kind);
  const auto sUri    = xiiMCPJson::GetString(arguments, "uri");
  if (kind == xiiMCPCapability::ResourceTemplate)
  {
    const auto uiPrefixBytes = sPublicKey.GetElementCount() - sRemoteKey.GetElementCount();
    if (sUri.GetElementCount() <= uiPrefixBytes)
    {
      return xiiStatus("The resource template needs a concrete URI.");
    }
    sRemoteKey = xiiStringView(sUri.GetStartPointer() + uiPrefixBytes, sUri.GetEndPointer());
  }
  request["key"]                = xiiString(sRemoteKey);
  request["arguments"]          = arguments;
  const auto           deadline = context.m_Deadline.IsPositive() ? context.m_Deadline : start + xiiTime::MakeFromSeconds(5);
  xiiVariantDictionary response;
  const auto           status = m_Bridge.Call(it.Value().m_hConnection, request, deadline, response, context.m_pCanceled);

  xiiVariantDictionary activity;
  activity["capability"] = xiiString(sRemoteKey);
  activity["target"]     = xiiString(sEndpoint);
  activity["elapsedMs"]  = (xiiTime::Now() - start).GetMilliseconds();
  activity["status"]     = xiiString(status.Succeeded() ? "Completed" : "Failed");
  if (status.Failed())
  {
    activity["error"] = xiiString(status.GetMessageString().GetView().GetSubString(0, 256));
  }
  if (m_Activity.GetCount() >= 100)
  {
    m_Activity.RemoveAtAndCopy(0);
  }
  m_Activity.PushBack(std::move(activity));

  const auto* pValue = xiiMCPJson::GetObject(response, "value");
  if (status.Failed() || pValue == nullptr)
  {
    return status.Failed() ? status : xiiStatus("The target returned an invalid MCP result.");
  }

  out_result = *pValue;
  if (kind == xiiMCPCapability::Resource || kind == xiiMCPCapability::ResourceTemplate)
  {
    const auto* pContents = pValue->GetValue("contents");
    if (pContents != nullptr && pContents->IsA<xiiVariantArray>())
    {
      xiiVariantArray rewritten;
      for (const auto& item : pContents->Get<xiiVariantArray>())
      {
        if (item.IsA<xiiVariantDictionary>())
        {
          auto content   = item.Get<xiiVariantDictionary>();
          content["uri"] = xiiString(sUri);
          rewritten.PushBack(std::move(content));
        }
      }
      out_result["contents"] = std::move(rewritten);
    }
  }
  return XII_SUCCESS;
}

void xiiMCPGateway::Process(xiiStringView sRequest, xiiMCPSession& ref_session, xiiStringBuilder& out_sReply, const xiiAtomicBool* pCanceled)
{
  xiiMCPCallContext context;
  context.m_sIdentity     = "local-client";
  context.m_uiPermissions = m_Configuration.m_uiPermissions;
  context.m_Deadline      = xiiTime::Now() + m_Configuration.m_Http.m_RequestTimeout;
  context.m_pCanceled     = pCanceled;
  m_Protocol.Process(sRequest, ref_session, context, out_sReply);
}

xiiStatus xiiMCPGateway::ListTargets(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_reply)
{
  xiiVariantArray targets;
  for (auto it = m_Targets.GetIterator(); it.IsValid(); ++it)
  {
    const auto&          target = it.Value();
    xiiVariantDictionary row;
    row["endpoint"]        = target.m_sEndpoint;
    row["instance"]        = target.m_sInstance;
    row["processId"]       = target.m_uiProcessId;
    row["namespace"]       = MakeTargetNamespace(target.m_sInstance);
    row["application"]     = target.m_sApplication;
    row["connected"]       = m_Bridge.IsConnected(target.m_hConnection);
    row["capabilityCount"] = target.m_Capabilities.GetCount();
    row["status"]          = target.m_sConnectionStatus;
    targets.PushBack(std::move(row));
  }
  out_reply["targets"] = std::move(targets);
  return XII_SUCCESS;
}

xiiVariantDictionary xiiMCPGateway::GetSnapshot() const
{
  xiiVariantDictionary snapshot;
  const_cast<xiiMCPGateway*>(this)->ListTargets({}, {}, snapshot).IgnoreResult();
  xiiMCPCallContext context;
  context.m_uiPermissions = m_Configuration.m_uiPermissions;
  xiiVariantArray tools, resources, prompts;
  m_Registry.List(xiiMCPCapability::Tool, context, tools);
  m_Registry.List(xiiMCPCapability::Resource, context, resources);
  m_Registry.List(xiiMCPCapability::Prompt, context, prompts);
  xiiVariantArray templates;
  m_Registry.List(xiiMCPCapability::ResourceTemplate, context, templates);
  snapshot["resourceTemplates"] = templates;
  snapshot["tools"]             = std::move(tools);
  snapshot["resources"]         = std::move(resources);
  snapshot["prompts"]           = std::move(prompts);
  snapshot["activity"]          = m_Activity;
  snapshot["sessions"]          = m_Http.GetSessionCount();
  xiiVariantArray clients;
  m_Http.ListSessions(clients);
  snapshot["clients"] = clients;
  xiiStringBuilder sEndpoint;
  const auto       endpoint = GetEndpoint();
  if (endpoint.GetAddress().IsValid())
  {
    endpoint.ToString(sEndpoint);
  }
  snapshot["endpoint"] = xiiString(sEndpoint);
  return snapshot;
}
