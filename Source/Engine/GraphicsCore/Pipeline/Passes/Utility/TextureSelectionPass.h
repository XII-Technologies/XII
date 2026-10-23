/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Select a supplied texture or its required fallback without copying or submitting GPU work.
class XII_GRAPHICSCORE_DLL xiiTextureSelectionPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureSelectionPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputPrimary{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureInputPin  m_InputFallback;
  xiiRenderPipelineTextureOutputPin m_OutputTexture{xiiGALResourceFormat::Unknown, false, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};

  bool m_bPreferPrimary = true;

  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
