/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Produces a transient cleared image, optionally sized from another image.
class XII_GRAPHICSCORE_DLL xiiClearTexturePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClearTexturePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputReference{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_OutputTexture{xiiGALResourceFormat::RGBA16Float};
  xiiColor                          m_Color = xiiColor::MakeZero();

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
