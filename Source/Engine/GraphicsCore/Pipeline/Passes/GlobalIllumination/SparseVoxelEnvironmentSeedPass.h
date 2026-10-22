/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Explicit isotropic environment seeding; not geometry voxelization or an implicit RT fallback.
class XII_GRAPHICSCORE_DLL xiiSparseVoxelEnvironmentSeedPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVoxelEnvironmentSeedPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineDataInputPin               m_Schedule{xiiGetStaticRTTI<xiiSparseVoxelFrameData>()};
  xiiRenderPipelineBufferInputPin             m_Environment{false, xiiGALBindFlags::UniformBuffer}, m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Levels{false, xiiGALBindFlags::ShaderResource}, m_Updates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_InitialPages{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess}, m_InitialPool{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Pages{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess}, m_Pool{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  bool                                        m_bAsyncCompute = false;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
