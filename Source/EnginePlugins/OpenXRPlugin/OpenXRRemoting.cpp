#include <OpenXRPlugin/OpenXRPluginPCH.h>

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Reflection/ReflectionUtils.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <OpenXRPlugin/OpenXRDeclarations.h>
#  include <OpenXRPlugin/OpenXRRemoting.h>
#  include <OpenXRPlugin/OpenXRSingleton.h>

XII_IMPLEMENT_SINGLETON(xiiOpenXRRemoting);

xiiOpenXRRemoting::xiiOpenXRRemoting(xiiOpenXR* pOpenXR) :
  m_SingletonRegistrar(this), m_pOpenXR(pOpenXR)
{
}

xiiOpenXRRemoting::~xiiOpenXRRemoting()
{
}

xiiResult xiiOpenXRRemoting::Initialize()
{
  if (m_pOpenXR->IsInitialized())
  {
    return XII_FAILURE;
  }

  xiiStringBuilder sRemotingJson = xiiOSFile::GetApplicationDirectory();
  sRemotingJson.AppendPath("RemotingXR.json");

  if (xiiOSFile::ExistsFile(sRemotingJson))
  {
    m_sPreviousRuntime = xiiEnvironmentVariableUtils::GetValueString("XR_RUNTIME_JSON");
    if (xiiEnvironmentVariableUtils::SetValueString("XR_RUNTIME_JSON", sRemotingJson).Failed())
    {
      xiiLog::Error("Failed to set environment variable XR_RUNTIME_JSON.");
      return XII_FAILURE;
    }

    m_bInitialized = true;
    return XII_SUCCESS;
  }
  else
  {
    xiiLog::Error("XR_RUNTIME_JSON not found: {}", sRemotingJson);
    return XII_FAILURE;
  }
}

xiiResult xiiOpenXRRemoting::Deinitialize()
{
  if (!m_bInitialized)
  {
    return XII_SUCCESS;
  }

  if (m_pOpenXR->IsInitialized())
    return XII_FAILURE;

  m_bInitialized = false;
  SetEnvironmentVariableW(L"XR_RUNTIME_JSON", xiiStringWChar(m_sPreviousRuntime));
  m_sPreviousRuntime.Clear();
  return XII_SUCCESS;
}

bool xiiOpenXRRemoting::IsInitialized() const
{
  return m_bInitialized;
}

xiiResult xiiOpenXRRemoting::Connect(const char* remoteHostName, uint16_t remotePort, bool enableAudio, int maxBitrateKbps)
{
  XII_ASSERT_DEV(IsInitialized(), "Need to call 'xiiXRRemotingInterface::Initialize' first.");
  XII_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'xiiXRInterface::Initialize' first.");

  XrRemotingRemoteContextPropertiesMSFT contextProperties;
  contextProperties                             = XrRemotingRemoteContextPropertiesMSFT{static_cast<XrStructureType>(XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT)};
  contextProperties.enableAudio                 = enableAudio;
  contextProperties.maxBitrateKbps              = maxBitrateKbps;
  contextProperties.videoCodec                  = XR_REMOTING_VIDEO_CODEC_ANY_MSFT;
  contextProperties.depthBufferStreamResolution = XR_REMOTING_DEPTH_BUFFER_STREAM_RESOLUTION_HALF_MSFT;
  XrResult res                                  = m_pOpenXR->m_extensions.pfn_xrRemotingSetContextPropertiesMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &contextProperties);
  if (res != XrResult::XR_SUCCESS)
  {
    XR_LOG_ERROR(res);
    return XII_FAILURE;
  }

  XrRemotingConnectInfoMSFT connectInfo{static_cast<XrStructureType>(XR_TYPE_REMOTING_CONNECT_INFO_MSFT)};
  connectInfo.remoteHostName   = remoteHostName;
  connectInfo.remotePort       = remotePort;
  connectInfo.secureConnection = false;
  res                          = m_pOpenXR->m_extensions.pfn_xrRemotingConnectMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &connectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return XII_FAILURE;
  }
  {
    xiiXRRemotingConnectionEventData data;
    data.m_connectionState  = xiiXRRemotingConnectionState::Connecting;
    data.m_disconnectReason = xiiXRRemotingDisconnectReason::None;
    m_event.Broadcast(data);
  }
  return XII_SUCCESS;
}

xiiResult xiiOpenXRRemoting::Disconnect()
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return XII_SUCCESS;

  XrRemotingDisconnectInfoMSFT disconnectInfo;
  XrResult                     res = m_pOpenXR->m_extensions.pfn_xrRemotingDisconnectMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &disconnectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiEnum<xiiXRRemotingConnectionState> xiiOpenXRRemoting::GetConnectionState() const
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return xiiXRRemotingConnectionState::Disconnected;

  XrRemotingConnectionStateMSFT connectionState;
  XrResult                      res = m_pOpenXR->m_extensions.pfn_xrRemotingGetConnectionStateMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &connectionState, nullptr);
  if (res != XrResult::XR_SUCCESS)
  {
    return xiiXRRemotingConnectionState::Disconnected;
  }
  switch (connectionState)
  {
    case XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT:
      return xiiXRRemotingConnectionState::Disconnected;
    case XR_REMOTING_CONNECTION_STATE_CONNECTING_MSFT:
      return xiiXRRemotingConnectionState::Connecting;
    case XR_REMOTING_CONNECTION_STATE_CONNECTED_MSFT:
      return xiiXRRemotingConnectionState::Connected;
    default:
      XII_REPORT_FAILURE("Unknown enum value");
      break;
  }

  return xiiXRRemotingConnectionState::Disconnected;
}

xiiXRRemotingConnectionEvent& xiiOpenXRRemoting::GetConnectionEvent()
{
  return m_event;
}

void xiiOpenXRRemoting::HandleEvent(const XrEventDataBuffer& event)
{
  if (!m_bInitialized)
    return;

  switch (event.type)
  {
    case XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT:
    {
      xiiXRRemotingConnectionEventData data;
      data.m_connectionState  = xiiXRRemotingConnectionState::Connected;
      data.m_disconnectReason = xiiXRRemotingDisconnectReason::None;

      xiiLog::Info("XR Remoting connected.");
      m_event.Broadcast(data);
    }
    break;
    case XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT:
    {
      xiiXRRemotingConnectionEventData data;
      data.m_connectionState                = xiiXRRemotingConnectionState::Disconnected;
      XrRemotingDisconnectReasonMSFT reason = reinterpret_cast<const XrRemotingEventDataDisconnectedMSFT*>(&event)->disconnectReason;
      data.m_disconnectReason               = static_cast<xiiXRRemotingDisconnectReason::Enum>(reason);

      xiiStringBuilder sTemp;
      xiiReflectionUtils::EnumerationToString(data.m_disconnectReason, sTemp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
      xiiLog::Info("XR Remoting disconnected with reason: {}", sTemp);

      m_event.Broadcast(data);
    }
    break;
  }
}

#endif
