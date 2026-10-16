/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Retains supplied GPU buffer contents as an explicit previous-frame graph resource.
/// Writable storage is private to each pass occurrence; reuse waits for all frame consumers.
class XII_GRAPHICSCORE_DLL xiiBufferHistoryPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBufferHistoryPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Previous{true, xiiGALBindFlags::ShaderResource, xiiRenderPipelinePinAccess::Read, true};
  xiiBufferHistoryPass();
  ~xiiBufferHistoryPass();
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
