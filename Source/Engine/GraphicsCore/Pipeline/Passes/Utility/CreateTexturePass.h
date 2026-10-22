/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Declares an uninitialized transient color texture. A connected writer must initialize it before it is read.
/// Texture creation is configured entirely through its reflected output pin.
class XII_GRAPHICSCORE_DLL xiiCreateTexturePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateTexturePass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_OutputTexture;

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
