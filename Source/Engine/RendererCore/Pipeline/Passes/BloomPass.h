#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class XII_RENDERERCORE_DLL xiiBloomPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBloomPass, xiiRenderPipelinePass);

public:
  xiiBloomPass();
  ~xiiBloomPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer(xiiVec2 pixelSize, const xiiColor& tintColor);

  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  float                          m_fRadius;
  float                          m_fThreshold;
  float                          m_fIntensity;
  xiiColorGammaUB                m_InnerTintColor;
  xiiColorGammaUB                m_MidTintColor;
  xiiColorGammaUB                m_OuterTintColor;
  xiiConstantBufferStorageHandle m_hConstantBuffer;
  xiiShaderResourceHandle        m_hShader;
};
