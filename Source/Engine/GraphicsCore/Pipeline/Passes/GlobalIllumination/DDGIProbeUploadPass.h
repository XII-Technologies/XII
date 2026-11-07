/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
class XII_GRAPHICSCORE_DLL xiiDDGIProbeUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDDGIProbeUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_Schedule{xiiGetStaticRTTI<xiiDDGIProbeFrameData>(), true};
  xiiRenderPipelineBufferOutputPin m_States{true, xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess};
  xiiRenderPipelineBufferOutputPin m_Updates{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Constants{true, xiiGALBindFlags::UniformBuffer};
  bool                             m_bAllowAbsentSchedule = false;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
