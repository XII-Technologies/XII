#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders into a depth target only.
class XII_RENDERERCORE_DLL xiiDepthOnlyPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthOnlyPass, xiiRenderPipelinePass);

public:
  xiiDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~xiiDepthOnlyPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodePassThrougPin m_PinDepthStencil;
};
