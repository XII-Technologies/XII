#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiMSAAResolvePass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMSAAResolvePass, xiiGraphicsPipelinePass);

public:
  xiiMSAAResolvePass(xiiStringView sName = "MSAAResolvePass");

  virtual ~xiiMSAAResolvePass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;
  xiiRenderPipelineNodeInputBufferPin            m_PinFrameConstants;

  xiiEnum<xiiGALMSAASampleCount> m_SampleCount     = xiiGALMSAASampleCount::OneSample;
  bool                           m_bIsDepthResolve = false;
  xiiShaderResourceHandle        m_hDepthResolveShader;
};
