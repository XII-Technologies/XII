/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Declares an uninitialized transient color texture. A connected writer must initialize it before it is read.
/// Zero width or height uses the current view's render resolution for that dimension.
class XII_GRAPHICSCORE_DLL xiiCreateTexturePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateTexturePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_OutputTexture{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiUInt32                         m_uiWidth  = 0;
  xiiUInt32                         m_uiHeight = 0;
  xiiEnum<xiiGALResourceFormat>     m_Format   = xiiGALResourceFormat::RGBA16Float;

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
