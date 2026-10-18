/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes pending uploads to device-owned material resources.
class XII_GRAPHICSCORE_DLL xiiMaterialUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Materials{false, xiiGALBindFlags::ShaderResource};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
