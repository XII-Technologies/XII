#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Blurs input and writes it to an output buffer of the same format.
class XII_RENDERERCORE_DLL xiiBlurPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlurPass, xiiRenderPipelinePass);

public:
  xiiBlurPass();
  ~xiiBlurPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  void     SetRadius(xiiInt32 iRadius);
  xiiInt32 GetRadius() const;

protected:
  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiInt32                       m_iRadius = 15;
  xiiConstantBufferStorageHandle m_hBlurCB;
  xiiShaderResourceHandle        m_hShader;
};
