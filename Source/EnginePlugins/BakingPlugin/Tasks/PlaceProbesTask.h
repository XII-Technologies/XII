#pragma once

#include <BakingPlugin/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>

struct xiiBakingSettings;

namespace xiiBakingInternal
{
  class XII_BAKINGPLUGIN_DLL PlaceProbesTask : public xiiTask
  {
  public:
    PlaceProbesTask(const xiiBakingSettings& settings, const xiiBoundingBox& bounds, xiiArrayPtr<const Volume> volumes);
    ~PlaceProbesTask();

    virtual void Execute() override;

    xiiArrayPtr<const xiiVec3> GetProbePositions() const { return m_ProbePositions; }
    const xiiVec3&             GetGridOrigin() const { return m_vGridOrigin; }
    const xiiVec3U32&          GetProbeCount() const { return m_vProbeCount; }

  private:
    const xiiBakingSettings& m_Settings;

    xiiBoundingBox            m_Bounds;
    xiiArrayPtr<const Volume> m_Volumes;

    xiiVec3                  m_vGridOrigin = xiiVec3::ZeroVector();
    xiiVec3U32               m_vProbeCount = xiiVec3U32::ZeroVector();
    xiiDynamicArray<xiiVec3> m_ProbePositions;
  };
} // namespace xiiBakingInternal
