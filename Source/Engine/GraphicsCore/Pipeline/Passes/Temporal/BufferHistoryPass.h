/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Retains supplied GPU buffer contents as an explicit previous-frame graph resource.
/// Writable storage is private to each pass occurrence; reuse waits for all frame consumers.
class XII_GRAPHICSCORE_DLL xiiBufferHistoryPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBufferHistoryPass, xiiRenderPipelinePass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiBufferHistoryPass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Previous{true, xiiGALBindFlags::ShaderResource, xiiRenderPipelinePinAccess::Read, true};
  bool                             m_bAsyncCopy = true;
  xiiBufferHistoryPass();
  ~xiiBufferHistoryPass();
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

private:
  struct State;
  xiiSharedPtr<State> m_pState;
};
