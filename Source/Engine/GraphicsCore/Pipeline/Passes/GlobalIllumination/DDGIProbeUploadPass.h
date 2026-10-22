/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
class XII_GRAPHICSCORE_DLL xiiDDGIProbeUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIProbeUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin   m_Before;
  xiiRenderPipelineOrderOutputPin  m_After;
  xiiRenderPipelineDataInputPin    m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>()};
  xiiRenderPipelineBufferOutputPin m_States{false, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin m_Updates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
