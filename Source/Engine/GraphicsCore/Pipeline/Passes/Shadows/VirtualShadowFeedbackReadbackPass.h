/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackTypes.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Completion-checked request consumption into existing device page residency.
/// Staging/fences are private to this occurrence and allocated only when consumed.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowFeedbackReadbackPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowFeedbackReadbackPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Requests{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                       m_uiMaxPendingReadbacks = 4;
  xiiVirtualShadowFeedbackReadbackPass();
  ~xiiVirtualShadowFeedbackReadbackPass();
  xiiResult                     Configure(const xiiVariantDictionary& properties) override;
  xiiResult                     Build(xiiRenderPipelinePassContext& context) override;
  xiiVirtualShadowFeedbackStats GetStats() const;

private:
  struct State;
  xiiSharedPtr<State> m_State;
};
