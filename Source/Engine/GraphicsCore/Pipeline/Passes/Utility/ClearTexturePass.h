/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Produces a transient cleared image, optionally sized from another image.
class XII_GRAPHICSCORE_DLL xiiClearTexturePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClearTexturePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputReference{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_OutputTexture;
  xiiUInt32                         m_uiWidth = 0, m_uiHeight = 0;
  float                             m_fResolutionScale = 1.0f;
  xiiEnum<xiiGALResourceFormat>     m_Format           = xiiGALResourceFormat::RGBA16Float;
  xiiColor                          m_Color            = xiiColor::MakeZero();
  xiiResult                         Configure(const xiiVariantDictionary& properties) override;
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};
