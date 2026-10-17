/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Retains supplied GPU texture contents as an explicit previous-frame graph resource.
/// Writable storage is private to each pass occurrence; reuse waits for all frame consumers.
class XII_GRAPHICSCORE_DLL xiiTextureHistoryPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureHistoryPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Undefined, 1};
  xiiRenderPipelineTextureOutputPin m_Previous{xiiGALResourceFormat::Unknown, true, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Undefined, 1, xiiRenderPipelinePinAccess::Read, true};
  xiiTextureHistoryPass();
  ~xiiTextureHistoryPass();
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override;
  void      InvalidateTemporalHistory() override;

private:
  struct State;
  xiiUniquePtr<State> m_pState;
};
