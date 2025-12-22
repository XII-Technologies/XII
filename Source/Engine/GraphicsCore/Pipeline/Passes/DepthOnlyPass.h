#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiDepthOnlyPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthOnlyPass, xiiGraphicsPipelinePass);

public:
  xiiDepthOnlyPass(xiiStringView sName = "DepthOnlyPass");

  virtual ~xiiDepthOnlyPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodePassThroughDepthAttachmentPin m_PinDepthStencil;
  xiiRenderPipelineNodeInputBufferPin                m_PinFrameConstants;

  bool m_bRenderStaticObjects      = true;
  bool m_bRenderDynamicObjects     = true;
  bool m_bRenderTransparentObjects = false;

  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 2U> m_FramebufferCache;
};
