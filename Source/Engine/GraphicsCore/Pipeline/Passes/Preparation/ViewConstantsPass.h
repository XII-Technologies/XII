/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Camera and frame-time upload without lighting, geometry or world extraction.
class XII_GRAPHICSCORE_DLL xiiViewConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>(), true};
  xiiRenderPipelineBufferOutputPin m_OutputPerFrameCamera{false, xiiGALBindFlags::UniformBuffer};
  xiiRenderPipelineBufferOutputPin m_OutputPerFrameGlobal{false, xiiGALBindFlags::UniformBuffer};


  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
