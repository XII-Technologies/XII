/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Publishes the device-owned geometry metadata frame slice and pending meshlet uploads.
class XII_GRAPHICSCORE_DLL xiiGeometryUploadPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGeometryUploadPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineOrderInputPin   m_Before;
  xiiRenderPipelineOrderOutputPin  m_After;
  xiiRenderPipelineBufferOutputPin m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
