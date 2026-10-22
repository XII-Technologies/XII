/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Roots a shader-readable buffer for an external consumer of this graph.
/// This declares liveness and final state; it performs no upload, copy or dispatch.
class XII_GRAPHICSCORE_DLL xiiBufferExportPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBufferExportPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin   m_Before;
  xiiRenderPipelineOrderOutputPin  m_After;
  xiiRenderPipelineBufferInputPin  m_Input{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Output{false, xiiGALBindFlags::ShaderResource, xiiRenderPipelinePinAccess::Read};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
