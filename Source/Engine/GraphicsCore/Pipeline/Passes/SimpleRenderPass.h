#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// \brief A very basic render pass that renders into the color target.
///
/// Can either works as passthrough or if no input is present creates
/// output targets matching the view's render target.
/// Needs to be connected to a xiiTargetPass to function.
class XII_RENDERERCORE_DLL xiiSimpleRenderPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleRenderPass, xiiRenderPipelinePass);

public:
  xiiSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~xiiSimpleRenderPass();

  virtual bool      GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void SetMessage(const char* szMessage);

protected:
  xiiRenderPipelineNodePassThrougPin m_PinColor;
  xiiRenderPipelineNodePassThrougPin m_PinDepthStencil;

  xiiString m_sMessage;
};
