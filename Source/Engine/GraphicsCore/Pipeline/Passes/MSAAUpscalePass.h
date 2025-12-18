#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiMSAAUpscalePass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMSAAUpscalePass, xiiGraphicsPipelinePass);

public:
  xiiMSAAUpscalePass(xiiStringView sName = "MSAAUpscalePass");

  virtual ~xiiMSAAUpscalePass();

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;

  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual xiiResult InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiSharedPtr<xiiGALGraphicsPipelineState> CreatePipelineState(const xiiRenderViewContext& renderViewContext) const;

protected:
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;
  xiiRenderPipelineNodeInputBufferPin            m_PinFrameConstants;

  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 2U> m_FramebufferCache;

  xiiEnum<xiiGALMSAASampleCount> m_MsaaMode = xiiGALMSAASampleCount::OneSample;
  xiiShaderResourceHandle        m_hShader;
};
