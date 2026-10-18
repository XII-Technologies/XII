/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Math/Size.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <Shaders/Pipeline/Passes/DDGI/DDGIConstants.h>
class xiiDDGIProbeSchedulerState;

/// Owned probe scheduling snapshot. Published as const data; contains no world/component pointers.
class XII_GRAPHICSCORE_DLL xiiDDGIProbeFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIProbeFrameData, xiiRenderPipelineFrameData);

public:
  xiiDDGIProbeFrameData(xiiUInt64 setup, xiiUInt64 frame, xiiSharedPtr<xiiDDGIProbeSchedulerState> scheduler);
  ~xiiDDGIProbeFrameData() override;
  xiiDDGIProbeConstants                                              m_Constants{};
  xiiDynamicArray<xiiGpuDDGIProbeState, xiiAlignedAllocatorWrapper>  m_Probes;
  xiiDynamicArray<xiiGpuDDGIProbeUpdate, xiiAlignedAllocatorWrapper> m_Updates;
  xiiSizeU32                                                         m_AtlasSize;
  bool                                                               m_bHistoryValid = false;
  /// Commit CPU scheduling feedback only after the corresponding GPU operation records.
  void CommitUpdates() const;

private:
  xiiUInt64                                m_Frame;
  xiiSharedPtr<xiiDDGIProbeSchedulerState> m_Scheduler;
};
