/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// An optional interface buffer connection without GPU work or ownership.
class XII_GRAPHICSCORE_DLL xiiBufferForwardPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBufferForwardPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{true};
  xiiRenderPipelineBufferOutputPin m_Output{true, {}, xiiRenderPipelinePinAccess::Read};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
