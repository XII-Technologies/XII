/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
/// Creates one transient, fully initialized D32 depth image. Rasterization is a separate operation.
class XII_GRAPHICSCORE_DLL xiiDepthTargetPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthTargetPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Reference{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::D32Float, false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::DepthStencil, xiiGALResourceDimension::Texture2D, 1};
  float                             m_fDepth = 0;

  xiiResult Configure(const xiiVariantDictionary& properties) override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
