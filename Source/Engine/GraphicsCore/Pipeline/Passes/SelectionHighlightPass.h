#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiSelectionHighlightPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSelectionHighlightPass, xiiGraphicsPipelinePass);

public:
  xiiSelectionHighlightPass(xiiStringView sName = "SelectionHighlightPass");

  virtual ~xiiSelectionHighlightPass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodePassThroughColourAttachmentPin m_PinColour;
  xiiRenderPipelineNodeInputDepthAttachmentPin        m_PinDepthStencil;

  xiiShaderResourceHandle    m_hShader;
  xiiSharedPtr<xiiGALBuffer> m_pConstantBuffer;

  xiiColor m_HighlightColour  = xiiColorScheme::LightUI(xiiColorScheme::Yellow);
  float    m_fOverlayOpacity = 0.1f;
};
