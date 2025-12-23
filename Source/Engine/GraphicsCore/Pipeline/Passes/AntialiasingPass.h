#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiAntialiasingPass : public xiiGraphicsPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAntialiasingPass, xiiGraphicsPipelinePass);

public:
  xiiAntialiasingPass(xiiStringView sName = "AntialiasingPass");

  virtual ~xiiAntialiasingPass();

  virtual xiiResult GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;

protected:
  xiiRenderPipelineNodeInputColourAttachmentPin  m_PinInput;
  xiiRenderPipelineNodeOutputColourAttachmentPin m_PinOutput;

  xiiHashedString         m_SampleCount;
  xiiShaderResourceHandle m_hShader;
};
