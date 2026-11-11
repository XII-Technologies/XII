/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Configuration/PlatformProfile.h>
#include <GraphicsCore/Streaming/GpuResidencyManager.h>
#include <GraphicsFoundation/Device/StreamingMemorySettings.h>

/// Project/platform ownership of shared device policies. No view or pipeline holds a copy.
class XII_GRAPHICSCORE_DLL xiiRenderDeviceProfileConfig final : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderDeviceProfileConfig, xiiProfileConfigData);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderDeviceSettings);

public:
  xiiGALStreamingMemorySettings m_Memory;
  xiiGpuResidencyDescription    m_Residency;

  xiiResult Validate() const override;
  void      SaveRuntimeData(xiiChunkStreamWriter& stream) const override;
  void      LoadRuntimeData(xiiChunkStreamReader& stream) override;

private:
  static void Startup();
  static void Shutdown();
  class State;
  static xiiUniquePtr<State> s_pState;
};
