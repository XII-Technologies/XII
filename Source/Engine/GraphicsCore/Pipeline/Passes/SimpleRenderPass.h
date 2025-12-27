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

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

public:
  /// \brief String property to hold a debug message that is printed when this pass is executed.
  XII_ALWAYS_INLINE void SetMessage(xiiStringView sMessage) { m_sMessage = sMessage; }

private:
  xiiRenderPipelineNodePassThroughColourAttachmentPin m_PinColour;
  xiiRenderPipelineNodePassThroughDepthAttachmentPin  m_PinDepthStencil;

  xiiString m_sMessage;
};
