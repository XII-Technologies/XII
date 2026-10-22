/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// An optional interface texture connection without GPU work or ownership.
class XII_GRAPHICSCORE_DLL xiiTextureForwardPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureForwardPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin    m_Before;
  xiiRenderPipelineOrderOutputPin   m_After;
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};
