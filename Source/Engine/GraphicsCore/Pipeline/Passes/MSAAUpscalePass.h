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

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;
  xiiRenderPipelineNodeInputBufferPin            m_PinFrameConstants;

  xiiEnum<xiiGALMSAASampleCount> m_SampleCount = xiiGALMSAASampleCount::OneSample;
  xiiShaderResourceHandle        m_hShader;
};
