/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Select a supplied texture or its fallback without copying or submitting GPU work.
/// The fallback is required unless absent output is explicitly permitted.
class XII_GRAPHICSCORE_DLL xiiTextureSelectionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureSelectionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputPrimary{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureInputPin  m_InputFallback;
  xiiRenderPipelineTextureOutputPin m_OutputTexture{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};

  bool m_bPreferPrimary       = true;
  bool m_bAllowAbsentFallback = false;
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;

  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
