/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Generates bounded screen-depth demand into an explicitly initialized request stream.
/// Consumes immutable snapshots; no world access, residency, readback or scene ownership.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowFeedbackPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowFeedbackPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin            m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineBufferInputPin             m_Cascades{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferInputPin             m_Initial{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin            m_Requests{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiRenderPipelinePinAccess::ReadWrite};
  xiiMat4                                     m_InverseViewProjection = xiiMat4::MakeIdentity();
  xiiUInt32                                   m_uiVirtualResolution = 16384, m_uiPageSize = 128, m_uiLightId = 1;
  float                                       m_fNearPlane    = 0.1f;
  bool                                        m_bAsyncCompute = true;
  xiiResult                                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
