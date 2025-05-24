#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

/// \brief Blurs input and writes it to an output buffer of the same format.
class XII_GRAPHICSCORE_DLL xiiBlurPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlurPass, xiiRenderPipelinePass);

public:
  xiiBlurPass();
  ~xiiBlurPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void     SetRadius(xiiInt32 iRadius);
  xiiInt32 GetRadius() const;

protected:
  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiInt32                   m_iRadius = 15;
  xiiSharedPtr<xiiGALBuffer> m_pBlurConstantBuffer;
  xiiShaderResourceHandle    m_hShader;
};
