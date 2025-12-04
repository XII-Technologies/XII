#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiSimpleRenderPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleRenderPass, xiiGraphicsPipelinePass);

public:
  xiiSimpleRenderPass(xiiStringView sName = "SimpleRenderPass");

  virtual ~xiiSimpleRenderPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

private:
  xiiRenderPipelineNodePassThroughColourAttachmentPin m_PinColour;
  xiiRenderPipelineNodePassThroughDepthAttachmentPin  m_PinDepthStencil;
  xiiRenderPipelineNodeInputBufferPin                 m_PinFrameConstants;

  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 2U> m_FramebufferCache;

  xiiString m_sMessage;
};
