#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders into a depth target only.
class XII_GRAPHICSCORE_DLL xiiDepthOnlyPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthOnlyPass, xiiRenderPipelinePass);

public:
  xiiDepthOnlyPass(xiiStringView sName = "DepthOnlyPass");
  ~xiiDepthOnlyPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodePassThroughPin m_PinDepthStencil;

  xiiSharedPtr<xiiGALRenderPass>  m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;
};
