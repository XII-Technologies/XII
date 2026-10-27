/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes the device-shared immutable GGX split-sum lookup, generating it on demand.
class XII_GRAPHICSCORE_DLL xiiBRDFLutPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBRDFLutPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Activation{xiiGALResourceFormat::Unknown, true};
  bool                              m_bRequireActivation = false;
  void                              GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RG16Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiBRDFLutPass();
  ~xiiBRDFLutPass();
  xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const override { return xiiRenderPipelineDeviceFeature::Compute; }
  xiiResult                                   Build(xiiRenderPipelinePassContext& context) override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
