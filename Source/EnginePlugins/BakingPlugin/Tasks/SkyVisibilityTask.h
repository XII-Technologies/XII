#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

struct xiiBakingSettings;
class xiiTracerInterface;

namespace xiiBakingInternal
{
  class XII_BAKINGPLUGIN_DLL SkyVisibilityTask : public xiiTask
  {
  public:
    SkyVisibilityTask(const xiiBakingSettings& settings, xiiTracerInterface& tracer, xiiArrayPtr<const xiiVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    xiiArrayPtr<const xiiCompressedSkyVisibility> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    const xiiBakingSettings& m_Settings;

    xiiTracerInterface&        m_Tracer;
    xiiArrayPtr<const xiiVec3> m_ProbePositions;

    xiiDynamicArray<xiiCompressedSkyVisibility> m_SkyVisibility;
  };
} // namespace xiiBakingInternal
