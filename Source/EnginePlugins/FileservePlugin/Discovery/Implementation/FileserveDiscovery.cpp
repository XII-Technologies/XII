/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Discovery/FileserveDiscovery.h>
#include <Foundation/Network/Messaging/NetworkMessageHandler.h>
#include <Foundation/Network/Messaging/NetworkReflectionEndpoint.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Utilities/CommandLineUtils.h>

struct xiiFileserveDiscoveryAdvertisement
{
  xiiUuid                    m_Id;
  xiiDynamicArray<xiiString> m_Addresses;
  xiiUInt16                  m_uiPort = 0;
};

struct xiiFileserveDiscoveryAcknowledgement
{
  xiiUuid m_Id;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiFileserveDiscoveryAdvertisement);
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiFileserveDiscoveryAcknowledgement);

namespace
{
  xiiNetworkAttribute* AddressLimits()
  {
    auto* pLimits                   = new xiiNetworkAttribute(2);
    pLimits->m_uiMaximumElements    = 16;
    pLimits->m_uiMaximumStringBytes = 96;
    return pLimits;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFileserveDiscoveryAdvertisement, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiFileserveDiscoveryAdvertisement>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_ARRAY_MEMBER_PROPERTY("Addresses", m_Addresses)->AddAttributes(AddressLimits()),
    XII_MEMBER_PROPERTY("Port", m_uiPort)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFileserveDiscoveryAcknowledgement, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiFileserveDiscoveryAcknowledgement>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  using Advertisement   = xiiFileserveDiscoveryAdvertisement;
  using Acknowledgement = xiiFileserveDiscoveryAcknowledgement;

  bool IsValid(const Advertisement& announcement)
  {
    if (!announcement.m_Id.IsValid() || announcement.m_uiPort == 0 ||
        announcement.m_Addresses.IsEmpty() || announcement.m_Addresses.GetCount() > 16)
      return false;

    for (const auto& address : announcement.m_Addresses)
    {
      xiiNetworkAddress parsed;
      if (xiiNetworkAddress::Parse(address, parsed).Failed() || parsed.IsWildcard() || parsed.IsMulticast())
        return false;
    }
    return true;
  }

  template <typename T>
  class DiscoveryHandler final : public xiiNetworkTypedMessageHandler<T>
  {
  public:
    xiiDelegate<void(xiiNetworkChannel, const T&)> m_Deliver;

    bool Authorize(const xiiNetworkDispatchContext& context) const override
    {
      return context.m_pSession != nullptr && context.m_Channel == context.m_pSession->m_DefaultChannel &&
        context.m_pSession->m_ConnectionInfo.m_Capabilities.m_bAuthenticated;
    }

    void OnMessage(const xiiNetworkDispatchContext& context, const T& message) override
    {
      m_Deliver(context.m_Channel, message);
    }
  };

  xiiResult StartDiscovery(xiiNetworkHost& ref_host, xiiNetworkReflectionEndpoint& ref_reflection, bool bLocal)
  {
    xiiNetworkHostConfiguration config;
    config.m_Protocol                       = xiiUuid(0x5849494653444953ULL, 0x434F564552593031ULL);
    config.m_uiMaximumConnections           = 4;
    config.m_uiMaximumListeners             = 1;
    config.m_uiMaximumChannelsPerConnection = 2;
    config.m_uiMaximumMessageBytes          = 8192;
    config.m_IO.m_uiWorkerThreadCount       = 1;
    xiiNetworkReflectionEndpoint::EnableProtocol(config);

    if (bLocal)
    {
      config.m_Transport      = xiiNetworkTransportType::Local;
      config.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;
    }
    else
    {
      const auto& command   = *xiiCommandLineUtils::GetGlobalInstance();
      const auto  transport = command.GetStringOption("-fs_discovery_transport", 0, "tls");
      if (transport == "quic")
        config.m_Transport = xiiNetworkTransportType::SecureDatagram;
      else if (transport != "tls")
        return XII_FAILURE;

      xiiNetworkSecurityConfig security;
      security.m_sApplicationProtocol  = "xii-fileserve-discovery/1";
      security.m_bUseSystemTrust       = false;
      security.m_sCertificateChainFile = command.GetStringOption("-fs_cert");
      security.m_sPrivateKeyFile       = command.GetStringOption("-fs_key");
      security.m_sTrustAnchorsFile     = command.GetStringOption("-fs_ca");
      if (security.m_sCertificateChainFile.IsEmpty() || security.m_sPrivateKeyFile.IsEmpty() || security.m_sTrustAnchorsFile.IsEmpty())
      {
        xiiLog::Error("Remote FileServe discovery requires -fs_cert, -fs_key, and -fs_ca.");
        return XII_FAILURE;
      }
      config.m_pCredentials  = XII_DEFAULT_NEW(xiiNetworkCredentials);
      const auto credentials = config.m_pCredentials->Load(security);
      if (credentials.Failed())
      {
        xiiLog::Error("Could not load FileServe discovery credentials. {}", credentials.GetCodeName());
        return XII_FAILURE;
      }
    }

    xiiNetworkReflectionConfiguration reflection;
    reflection.m_uiMaximumPeers                        = config.m_uiMaximumConnections;
    reflection.m_uiMaximumHandlers                     = 1;
    reflection.m_uiMaximumSchemasPerPeer               = 2;
    reflection.m_Serialization.m_uiMaximumPayloadBytes = config.m_uiMaximumMessageBytes;
    XII_SUCCEED_OR_RETURN(ref_reflection.Start(reflection).GetResult());
    const auto started = ref_host.Start(config);
    if (started.Failed())
      xiiLog::Error("Could not start FileServe discovery. {}", started.GetCodeName());
    return started.GetResult();
  }

  template <typename T>
  xiiResult ProcessDiscoveryEvent(xiiNetworkReflectionEndpoint& ref_reflection, xiiNetworkEvent&& event)
  {
    switch (event.m_Type)
    {
      case xiiNetworkEventType::Connected:
      {
        const auto channel = event.m_pSessionInfo->m_DefaultChannel;
        XII_SUCCEED_OR_RETURN(ref_reflection.ProcessEvent(std::move(event)).GetResult());
        return ref_reflection.OfferSchema(channel, xiiGetStaticRTTI<T>()).GetResult();
      }
      case xiiNetworkEventType::Message:
        XII_SUCCEED_OR_RETURN(ref_reflection.ProcessEvent(std::move(event)).GetResult());
        return ref_reflection.Dispatch(0, 1).GetResult();
      case xiiNetworkEventType::Closed:
      case xiiNetworkEventType::ChannelClosed:
        return ref_reflection.ProcessEvent(std::move(event)).GetResult();
      default:
        return XII_SUCCESS;
    }
  }

  bool HasAgreement(const xiiNetworkReflectionEndpoint& reflection, xiiNetworkChannel channel, const xiiNetworkSchema& schema)
  {
    bool bCompact = false;
    return channel.IsValid() && reflection.GetSchemaAgreement(channel, schema.m_uiTypeId, bCompact).Succeeded();
  }

  xiiResult WaitForDiscoveryEvent(xiiNetworkHost& ref_host, xiiNetworkEvent& out_event, xiiTime deadline)
  {
    const auto remaining = deadline - xiiTime::Now();
    return remaining.IsPositive() ? ref_host.WaitForEvent(out_event, remaining).GetResult() : XII_FAILURE;
  }

  xiiUInt32 DiscoveryPort()
  {
    return xiiCommandLineUtils::GetGlobalInstance()->GetUIntOption("-fs_discovery_port", 2042);
  }
} // namespace

xiiResult xiiFileserveDiscovery::Send(xiiStringView sDevice, xiiUInt16 uiServerPort, xiiArrayPtr<const xiiStringBuilder> addresses, xiiTime timeout)
{
  if (addresses.GetCount() > 16 || sDevice.IsEmpty())
    return XII_FAILURE;

  Advertisement advertisement;
  advertisement.m_Id     = xiiUuid::MakeUuid();
  advertisement.m_uiPort = uiServerPort;
  for (const auto& address : addresses)
    advertisement.m_Addresses.PushBack(address.GetView());
  if (!IsValid(advertisement))
    return XII_FAILURE;

  xiiNetworkHost               host;
  xiiNetworkTypeRegistry       registry;
  xiiNetworkReflectionEndpoint reflection(host, registry);
  const bool                   bLocal = sDevice.StartsWith("local:");
  XII_SUCCEED_OR_RETURN(StartDiscovery(host, reflection, bLocal));

  bool                                            bAcknowledged = false;
  xiiNetworkChannel                               channel;
  xiiSharedPtr<DiscoveryHandler<Acknowledgement>> pHandler = XII_DEFAULT_NEW(DiscoveryHandler<Acknowledgement>);
  pHandler->m_Deliver                                      = [&](xiiNetworkChannel source, const Acknowledgement& acknowledgement) {
    bAcknowledged = source == channel && acknowledgement.m_Id == advertisement.m_Id;
  };
  XII_SUCCEED_OR_RETURN(reflection.RegisterMessageHandler(pHandler).GetResult());
  xiiSharedPtr<const xiiNetworkSchema> pSchema;
  XII_SUCCEED_OR_RETURN(registry.GetSchema(xiiGetStaticRTTI<Advertisement>(), pSchema).GetResult());

  xiiNetworkConnection connection;
  if (bLocal)
  {
    sDevice.Shrink(6, 0);
    xiiNetworkLocalEndpoint endpoint;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(sDevice, endpoint).GetResult());
    XII_SUCCEED_OR_RETURN(host.Connect(endpoint, {}, connection).GetResult());
  }
  else
  {
    const auto uiPort = DiscoveryPort();
    if (uiPort == 0 || uiPort > xiiMath::MaxValue<xiiUInt16>())
      return XII_FAILURE;
    xiiStringBuilder sPort;
    sPort.SetFormat("{}", uiPort);
    xiiNetworkConnectOptions options;
    options.m_sServerName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_discovery_name", 0, sDevice);
    XII_SUCCEED_OR_RETURN(host.Connect(sDevice, sPort, options, connection).GetResult());
  }

  const auto      deadline = xiiTime::Now() + timeout;
  bool            bSent    = false;
  xiiNetworkEvent event;
  while (WaitForDiscoveryEvent(host, event, deadline).Succeeded())
  {
    if (event.m_Connection != connection)
      continue;
    if (event.m_Type == xiiNetworkEventType::Closed)
      return XII_FAILURE;
    if (event.m_Type == xiiNetworkEventType::Connected)
      channel = event.m_pSessionInfo->m_DefaultChannel;
    XII_SUCCEED_OR_RETURN(ProcessDiscoveryEvent<Advertisement>(reflection, std::move(event)));
    if (bSent && bAcknowledged)
      return XII_SUCCESS;
    if (!bSent && HasAgreement(reflection, channel, *pSchema))
    {
      XII_SUCCEED_OR_RETURN(reflection.SendMessage(channel, advertisement).GetResult());
      bSent = true;
    }
  }
  return XII_FAILURE;
}

xiiResult xiiFileserveDiscovery::Receive(xiiUInt16& out_uiServerPort, xiiDynamicArray<xiiString>& out_addresses, xiiTime timeout)
{
  const auto&                  command    = *xiiCommandLineUtils::GetGlobalInstance();
  const auto                   sLocalName = command.GetStringOption("-fs_discovery_local");
  xiiNetworkHost               host;
  xiiNetworkTypeRegistry       registry;
  xiiNetworkReflectionEndpoint reflection(host, registry);
  XII_SUCCEED_OR_RETURN(StartDiscovery(host, reflection, !sLocalName.IsEmpty()));

  Advertisement                                 advertisement;
  xiiNetworkChannel                             channel;
  xiiSharedPtr<DiscoveryHandler<Advertisement>> pHandler = XII_DEFAULT_NEW(DiscoveryHandler<Advertisement>);
  pHandler->m_Deliver                                    = [&](xiiNetworkChannel source, const Advertisement& message) {
    if (!channel.IsValid() && IsValid(message))
    {
      advertisement = message;
      channel       = source;
    }
  };
  XII_SUCCEED_OR_RETURN(reflection.RegisterMessageHandler(pHandler).GetResult());
  xiiSharedPtr<const xiiNetworkSchema> pSchema;
  XII_SUCCEED_OR_RETURN(registry.GetSchema(xiiGetStaticRTTI<Acknowledgement>(), pSchema).GetResult());

  xiiNetworkListener listener;
  if (!sLocalName.IsEmpty())
  {
    xiiNetworkLocalEndpoint endpoint;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(sLocalName, endpoint).GetResult());
    XII_SUCCEED_OR_RETURN(host.Listen(endpoint, listener).GetResult());
  }
  else
  {
    const auto        uiPort = DiscoveryPort();
    xiiNetworkAddress address;
    if (uiPort == 0 || uiPort > xiiMath::MaxValue<xiiUInt16>())
      return XII_FAILURE;
    XII_SUCCEED_OR_RETURN(xiiNetworkAddress::Parse(command.GetStringOption("-fs_discovery_bind", 0, "0.0.0.0"), address).GetResult());
    XII_SUCCEED_OR_RETURN(host.Listen(xiiNetworkEndpoint(address, static_cast<xiiUInt16>(uiPort)), listener).GetResult());
  }

  const auto        deadline = xiiTime::Now() + timeout;
  xiiNetworkMessage acknowledgement;
  xiiNetworkEvent   event;
  while (WaitForDiscoveryEvent(host, event, deadline).Succeeded())
  {
    if (acknowledgement.IsValid() && event.m_Type == xiiNetworkEventType::MessageSent && event.m_Message == acknowledgement)
    {
      XII_SUCCEED_OR_RETURN(event.m_Error.GetResult());
      out_uiServerPort = advertisement.m_uiPort;
      out_addresses    = std::move(advertisement.m_Addresses);
      return XII_SUCCESS;
    }

    const auto connection = event.m_Connection;
    const bool bClosed    = event.m_Type == xiiNetworkEventType::Closed;
    if (ProcessDiscoveryEvent<Acknowledgement>(reflection, std::move(event)).Failed() || bClosed)
    {
      if (connection.IsValid() && !bClosed)
        host.Disconnect(connection, false).IgnoreResult();
      if (channel.IsValid() && channel.m_Connection == connection)
      {
        channel         = {};
        acknowledgement = {};
      }
      continue;
    }
    if (!acknowledgement.IsValid() && HasAgreement(reflection, channel, *pSchema))
    {
      const Acknowledgement response{advertisement.m_Id};
      XII_SUCCEED_OR_RETURN(reflection.SendMessage(channel, response, xiiNetworkEncoding::Tagged, &acknowledgement).GetResult());
    }
  }
  return XII_FAILURE;
}

XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Discovery_Implementation_FileserveDiscovery);
