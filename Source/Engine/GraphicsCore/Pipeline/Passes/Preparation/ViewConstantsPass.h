/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Camera and frame-time upload without lighting, geometry or world extraction.
class XII_GRAPHICSCORE_DLL xiiViewConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_OutputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_OutputPerFrameGlobal{false, xiiGALBindFlags::UniformBuffer};


  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
