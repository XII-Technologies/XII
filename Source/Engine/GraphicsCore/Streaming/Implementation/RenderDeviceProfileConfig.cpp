/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Streaming/RenderDeviceProfileConfig.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderDeviceProfileConfig, 1, xiiRTTIDefaultAllocator<xiiRenderDeviceProfileConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Memory", m_Memory),
    XII_MEMBER_PROPERTY("Residency", m_Residency),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Device and Streaming"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderDeviceSettings)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GpuResidencyManager"
  END_SUBSYSTEM_DEPENDENCIES
  ON_HIGHLEVELSYSTEMS_STARTUP { xiiRenderDeviceProfileConfig::Startup(); }
  ON_HIGHLEVELSYSTEMS_SHUTDOWN { xiiRenderDeviceProfileConfig::Shutdown(); }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiResult xiiRenderDeviceProfileConfig::Validate() const
{
  XII_SUCCEED_OR_RETURN(SUPER::Validate());
  XII_SUCCEED_OR_RETURN(m_Memory.Validate());
  XII_SUCCEED_OR_RETURN(m_Residency.m_Streaming.Validate());
  return m_Residency.m_uiMaxGeometries > 0 && m_Residency.m_uiMaxGeometries <= 1048576U && m_Residency.m_uiFramesInFlight > 0 &&
      m_Residency.m_uiFramesInFlight <= 64 && m_Residency.m_uiMaxMeshlets > 0 && m_Residency.m_uiMaxMeshlets <= 64U * 1024U * 1024U ?
    XII_SUCCESS :
    XII_FAILURE;
}

void xiiRenderDeviceProfileConfig::SaveRuntimeData(xiiChunkStreamWriter& stream) const
{
  if (Validate().Failed())
  {
    xiiLog::Error("Cannot export invalid rendering device settings.");
    return;
  }
  stream.BeginChunk("RenderDeviceSettings", 1);
  xiiReflectionSerializer::WriteObjectToDDL(stream, GetDynamicRTTI(), this);
  stream.EndChunk();
}

void xiiRenderDeviceProfileConfig::LoadRuntimeData(xiiChunkStreamReader& stream)
{
  if (stream.GetCurrentChunk().m_sChunkName != "RenderDeviceSettings")
    return;
  if (stream.GetCurrentChunk().m_uiChunkVersion != 1)
  {
    xiiLog::Error("Unsupported rendering device settings version {}.", stream.GetCurrentChunk().m_uiChunkVersion);
    return;
  }
  if (ReadFromDDL(stream).Failed())
    xiiLog::Error("Invalid rendering device settings. Keeping the preceding policy.");
}

class xiiRenderDeviceProfileConfig::State
{
public:
  xiiCopyOnBroadcastEvent<const xiiGameApplicationExecutionEvent&>::Unsubscriber m_Subscription;
  xiiGALStreamingMemorySettings                                                  m_LastMemory;
  xiiGpuResidencyDescription                                                     m_LastResidency;
  xiiString                                                                      m_sError;
  const xiiGALDevice*                                                            m_pDevice  = nullptr;
  bool                                                                           m_bApplied = false;

  void Apply(const xiiGameApplicationExecutionEvent& event)
  {
    if (event.m_Type != xiiGameApplicationExecutionEvent::Type::BeforeWorldUpdates)
      return;
    const auto* application = xiiGameApplicationBase::GetGameApplicationBaseInstance();
    const auto* policy      = application ? application->GetPlatformProfile().GetTypeConfig<xiiRenderDeviceProfileConfig>() : nullptr;
    if (!policy || !xiiGALDevice::HasDefaultDevice())
      return;
    const auto  device    = xiiGALDevice::GetDefaultDevice();
    const auto& memory    = policy->m_Memory;
    const auto& residency = policy->m_Residency;
    if (m_bApplied && m_pDevice == device.Borrow() && m_LastMemory == memory && m_LastResidency == residency)
      return;
    // Remember failed requests too. Retrying the same incompatible startup policy every frame is not useful.
    m_LastMemory    = memory;
    m_LastResidency = residency;
    m_pDevice       = device.Borrow();
    m_bApplied      = true;
    xiiStringView error;
    if (policy->Validate().Failed())
      error = "Rendering device settings are invalid. Correct the active platform profile.";
    else if (device->SetStreamingMemorySettings(memory).Failed())
      error = "Could not publish the rendering device memory policy.";
    else if (xiiGpuResidencyManager::Configure(residency).Failed())
      error = "Rendering residency capacity changes require restarting the running scene. Streaming policy changes were applied.";
    if (m_sError != error)
    {
      m_sError = error;
      if (!error.IsEmpty())
        xiiLog::Error("{}", error);
    }
  }
};

xiiUniquePtr<xiiRenderDeviceProfileConfig::State> xiiRenderDeviceProfileConfig::s_pState;
void                                              xiiRenderDeviceProfileConfig::Startup()
{
  s_pState = XII_DEFAULT_NEW(State);
  if (auto* application = xiiGameApplicationBase::GetGameApplicationBaseInstance())
    application->m_ExecutionEvents.AddEventHandler(xiiMakeDelegate(&State::Apply, s_pState.Borrow()), s_pState->m_Subscription);
}
void xiiRenderDeviceProfileConfig::Shutdown()
{
  s_pState = nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Streaming_Implementation_RenderDeviceProfileConfig);
