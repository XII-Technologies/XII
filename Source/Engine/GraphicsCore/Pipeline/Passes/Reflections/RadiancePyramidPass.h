/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Produces a linear HDR mip chain with area-weighted reduction, including odd edges.
class XII_GRAPHICSCORE_DLL xiiRadiancePyramidPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRadiancePyramidPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_Color{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  /// Optional setup-time activation token; its texels are never sampled.
  xiiRenderPipelineTextureInputPin            m_Activation{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin           m_Pyramid{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1};
  bool                                        m_bRequireActivation = false;
  bool                                        m_bAsyncCompute      = true;
  void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }

private:
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
