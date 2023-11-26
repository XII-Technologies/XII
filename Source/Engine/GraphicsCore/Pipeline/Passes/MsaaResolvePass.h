#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiMsaaResolvePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMsaaResolvePass, xiiRenderPipelinePass);

public:
  xiiMsaaResolvePass();
  ~xiiMsaaResolvePass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  bool                       m_bIsDepth        = false;
  xiiEnum<xiiGALSampleCount> m_MsaaSampleCount = xiiGALSampleCount::OneSample;
  xiiShaderResourceHandle    m_hDepthResolveShader;
};
