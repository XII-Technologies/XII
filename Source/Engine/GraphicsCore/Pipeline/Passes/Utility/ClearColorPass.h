/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Color.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Initializes the entire supplied color texture and publishes its written version.
class XII_GRAPHICSCORE_DLL xiiClearColorPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClearColorPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputTexture{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputTexture{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget, xiiGALResourceDimension::Texture2D, 1};
  xiiColor                          m_Color = xiiColor::Black;

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
