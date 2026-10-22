/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Generates missing-LOD demand from query candidates, including forced LODs without resident draws.
/// InitializedRequests must be a fresh initialized table. No scene ownership, streaming or readback.
class XII_GRAPHICSCORE_DLL xiiGeometryLodFeedbackPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGeometryLodFeedbackPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin              m_Before;
  xiiRenderPipelineOrderOutputPin             m_After;
  xiiRenderPipelineBufferInputPin             m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferInputPin             m_Initial{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Requests{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiUInt32                                   m_uiPriorityTier = 3;
  xiiUInt32                                   m_uiPurposeMask  = 1;
  bool                                        m_bAsyncCompute  = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
