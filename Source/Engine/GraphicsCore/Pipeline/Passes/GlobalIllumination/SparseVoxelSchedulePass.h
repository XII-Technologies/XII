/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
class XII_GRAPHICSCORE_DLL xiiSparseVoxelSchedulePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVoxelSchedulePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_PreviousPool{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineDataOutputPin  m_Schedule{xiiGetStaticRTTI<xiiSparseVoxelFrameData>()};
  xiiUInt32                       m_Levels = 5, m_Grid = 8, m_Voxels = 8, m_Resident = 4096, m_Budget = 64, m_Refresh = 120;
  xiiUInt32                       m_MaximumPoolBytes = 536870912;
  float                           m_fVoxelSize = 0.5f, m_fHysteresis = 0.95f;
  bool                            m_bUseViewCamera = true;
  xiiVec3                         m_CameraPosition = xiiVec3::MakeZero();
  xiiSparseVoxelSchedulePass();
  ~xiiSparseVoxelSchedulePass();
  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

private:
  xiiSharedPtr<xiiSparseVoxelSchedulerState> m_State;
};
