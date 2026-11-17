/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Implementation/FileserveTransfer.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace xiiFileserveTransfer
{
  namespace
  {
    xiiStringView Transport()
    {
      const auto& command = *xiiCommandLineUtils::GetGlobalInstance();
      return command.GetStringOption("-fs_transport", 0, command.HasOption("-fs_cert") ? "tls" : "local");
    }

    xiiNetworkError MakeLocalEndpoint(xiiUInt16 uiPort, xiiNetworkLocalEndpoint& out_endpoint)
    {
      xiiStringBuilder name;
      name.SetFormat("xii-fileserve-{}", uiPort);
      return xiiNetworkLocalEndpoint::Make(name, out_endpoint);
    }
  } // namespace

  Request::Request(Operation operation) :
    m_Writer(&m_Storage)
  {
    m_Writer << static_cast<xiiUInt8>(operation);
  }

  xiiNetworkError Request::Send(xiiNetworkHost& host, xiiNetworkChannel channel) const
  {
    xiiNetworkMessage message;
    return host.Send(channel, m_Storage.GetContiguousMemoryRange(0), message);
  }

  xiiResult Start(xiiNetworkHost& host, bool bServer)
  {
    xiiNetworkHostConfiguration config;
    config.m_Protocol                       = xiiUuid(0x58494946494C4553ULL, 0x4552564554523031ULL);
    config.m_uiMaximumConnections           = bServer ? 64 : 1;
    config.m_uiMaximumListeners             = 1;
    config.m_uiMaximumChannelsPerConnection = 2;
    config.m_uiMaximumMessageBytes          = PacketBytes;
    config.m_uiMaximumPeerSendBytes         = 512 * 1024;
    config.m_uiMaximumPeerReceiveBytes      = 1024 * 1024;
    config.m_IO.m_uiWorkerThreadCount       = 1;

    const auto transport = Transport();
    if (transport == "local")
    {
      config.m_Transport      = xiiNetworkTransportType::Local;
      config.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;
    }
    else
    {
      if (transport == "quic")
        config.m_Transport = xiiNetworkTransportType::SecureDatagram;
      else if (transport != "tls")
        return XII_FAILURE;

      const auto&              command = *xiiCommandLineUtils::GetGlobalInstance();
      xiiNetworkSecurityConfig security;
      security.m_sApplicationProtocol  = "xii-fileserve/1";
      security.m_bUseSystemTrust       = false;
      security.m_sCertificateChainFile = command.GetStringOption("-fs_cert");
      security.m_sPrivateKeyFile       = command.GetStringOption("-fs_key");
      security.m_sTrustAnchorsFile     = command.GetStringOption("-fs_ca");
      if (security.m_sCertificateChainFile.IsEmpty() || security.m_sPrivateKeyFile.IsEmpty() || security.m_sTrustAnchorsFile.IsEmpty())
      {
        xiiLog::Error("Remote FileServe requires -fs_cert, -fs_key, and -fs_ca.");
        return XII_FAILURE;
      }
      config.m_pCredentials = XII_DEFAULT_NEW(xiiNetworkCredentials);
      XII_SUCCEED_OR_RETURN(config.m_pCredentials->Load(security).GetResult());
    }
    const auto error = host.Start(config);
    if (error.Failed())
      xiiLog::Error("Could not start FileServe networking. {}", error.GetCodeName());
    return error.GetResult();
  }

  xiiNetworkError Listen(xiiNetworkHost& host, xiiUInt16 uiPort, xiiNetworkListener& out_listener)
  {
    if (Transport() == "local")
    {
      xiiNetworkLocalEndpoint endpoint;
      const auto              error = MakeLocalEndpoint(uiPort, endpoint);
      return error.Failed() ? error : host.Listen(endpoint, out_listener);
    }
    xiiNetworkAddress address;
    const auto        error = xiiNetworkAddress::Parse(xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_bind", 0, "0.0.0.0"), address);
    return error.Failed() ? error : host.Listen(xiiNetworkEndpoint(address, uiPort), out_listener);
  }

  xiiNetworkError Connect(xiiNetworkHost& host, xiiStringView sAddress, xiiNetworkConnection& out_connection)
  {
    xiiStringBuilder   numericName, numericService;
    xiiStringView      name = sAddress, service = "1042";
    xiiNetworkEndpoint numeric;
    if (xiiNetworkEndpoint::Parse(sAddress, numeric).Succeeded())
    {
      numeric.GetAddress().ToString(numericName);
      numericService.SetFormat("{}", numeric.GetPort());
      name    = numericName;
      service = numericService;
    }
    else if (const char* separator = sAddress.FindSubString(":"))
    {
      name    = xiiStringView(sAddress.GetStartPointer(), separator);
      service = xiiStringView(separator + 1, sAddress.GetEndPointer());
    }
    xiiUInt32   uiPort = 0;
    const char* end    = nullptr;
    if (name.IsEmpty() || xiiConversionUtils::StringToUInt(service, uiPort, &end).Failed() ||
        end != service.GetEndPointer() || uiPort == 0 || uiPort > 65535)
      return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);

    if (Transport() == "local")
    {
      xiiNetworkAddress address;
      if (!name.IsEqual_NoCase("localhost") && (xiiNetworkAddress::Parse(name, address).Failed() || !address.IsLoopback()))
        return xiiNetworkError(xiiNetworkErrorCode::InvalidAddress);
      xiiNetworkLocalEndpoint endpoint;
      const auto              error = MakeLocalEndpoint(static_cast<xiiUInt16>(uiPort), endpoint);
      return error.Failed() ? error : host.Connect(endpoint, {}, out_connection);
    }

    xiiNetworkConnectOptions options;
    options.m_sServerName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server_name", 0, name);
    return numeric.IsValid() ? host.Connect(numeric, options, out_connection) : host.Connect(name, service, options, out_connection);
  }

  xiiResult ReadOperation(xiiRawMemoryStreamReader& reader, Operation& out_operation)
  {
    xiiUInt8 uiOperation = 0;
    if (reader.ReadBytes(&uiOperation, 1) != 1 || uiOperation < static_cast<xiiUInt8>(Operation::Mount) ||
        uiOperation > static_cast<xiiUInt8>(Operation::ShaderResult))
      return XII_FAILURE;
    out_operation = static_cast<Operation>(uiOperation);
    return XII_SUCCESS;
  }
} // namespace xiiFileserveTransfer

XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Implementation_FileserveTransfer);
