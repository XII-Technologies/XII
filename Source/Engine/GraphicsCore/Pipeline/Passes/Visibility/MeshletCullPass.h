/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Compacts supplied meshlet work using LOD routing, primitive ranges, frustum and cone tests.
/// InitialCount/InitialOverflow must be zero. Consumers must handle VisibleMeshletOverflow.
class XII_GRAPHICSCORE_DLL xiiMeshletCullPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletCullPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferInputPin             m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Visible{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Count{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Lod{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Arguments{false, xiiGALBindFlags::IndirectDrawArguments};
  xiiRenderPipelineBufferInputPin             m_InitialCount{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferInputPin             m_InitialOverflow{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Output{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_OutputCount{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiRenderPipelineBufferOutputPin            m_Overflow{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiUInt32                                   m_uiMaxVisibleMeshlets = 1024U * 1024U;
  bool                                        m_bAsyncCompute        = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
