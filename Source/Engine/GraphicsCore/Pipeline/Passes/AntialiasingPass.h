#pragma once

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiAntialiasingPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAntialiasingPass, xiiRenderPipelinePass);

public:
  xiiAntialiasingPass();
  ~xiiAntialiasingPass();

  virtual bool      GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiHashedString         m_sMsaaSampleCount;
  xiiShaderResourceHandle m_hShader;
};
