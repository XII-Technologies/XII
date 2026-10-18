/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Allocates one graph-owned append stream and initializes its header.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowFeedbackResetPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowFeedbackResetPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Requests{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiUInt32                        m_uiCapacity = 8192;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
