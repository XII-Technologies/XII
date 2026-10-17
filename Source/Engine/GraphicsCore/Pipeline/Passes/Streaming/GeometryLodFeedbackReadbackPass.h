/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackTypes.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Completion-checked asynchronous request consumption into existing device geometry residency.
/// Staging slots belong to this pass occurrence. No world objects or geometry assets are owned here.
class XII_GRAPHICSCORE_DLL xiiGeometryLodFeedbackReadbackPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGeometryLodFeedbackReadbackPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Requests{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                       m_uiMaxPendingReadbacks = 4;
  xiiGeometryLodFeedbackReadbackPass();
  ~xiiGeometryLodFeedbackReadbackPass();
  xiiResult                   Configure(const xiiVariantDictionary& properties) override;
  xiiResult                   Build(xiiRenderPipelinePassContext& context) override;
  xiiGeometryLodFeedbackStats GetStats() const;

private:
  struct State;
  xiiSharedPtr<State> m_State;
};
