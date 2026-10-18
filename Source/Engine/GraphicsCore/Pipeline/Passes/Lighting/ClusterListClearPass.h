/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>
/// Initializes private fixed-offset lists, invalid entries and an explicit overflow flag.
class XII_GRAPHICSCORE_DLL xiiClusterListClearPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusterListClearPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin               m_Layout{xiiGetStaticRTTI<xiiLightClusterFrameData>()};
  xiiRenderPipelineBufferOutputPin            m_Grid{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Indices{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Overflow{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
