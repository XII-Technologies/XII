/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <Shaders/Pipeline/Passes/SparseVoxelRadiance/SparseVoxelRadianceConstants.h>
class xiiSparseVoxelSchedulerState;
/// Owned CPU clipmap scheduling snapshot, published as const data and retained by recording.
class XII_GRAPHICSCORE_DLL xiiSparseVoxelFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVoxelFrameData, xiiRenderPipelineFrameData);

public:
  xiiSparseVoxelFrameData(xiiUInt64 setup, xiiUInt64 frame, xiiSharedPtr<xiiSparseVoxelSchedulerState> scheduler);
  ~xiiSparseVoxelFrameData() override;
  xiiSparseVoxelRadianceConstants                                           m_Constants{};
  xiiDynamicArray<xiiUInt32>                                                m_Pages;
  xiiDynamicArray<xiiGpuSparseVoxelLevel, xiiAlignedAllocatorWrapper>       m_Levels;
  xiiDynamicArray<xiiGpuSparseVoxelBrickUpdate, xiiAlignedAllocatorWrapper> m_Updates;
  xiiUInt32                                                                 m_PoolCount     = 0;
  bool                                                                      m_bHistoryValid = false;
  void                                                                      CommitUpdates() const;

private:
  xiiUInt64                                  m_Frame;
  xiiSharedPtr<xiiSparseVoxelSchedulerState> m_Scheduler;
};
