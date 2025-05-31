#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiBloomPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBloomPass, xiiRenderPipelinePass);

public:
  xiiBloomPass();
  ~xiiBloomPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void      ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  void UpdateConstantBuffer(xiiVec2 pixelSize, const xiiColor& tintColor);

  xiiRenderPipelineNodeInputPin  m_PinInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  float                          m_fRadius        = 0.2f;
  float                          m_fThreshold     = 1.0f;
  float                          m_fIntensity     = 0.3f;
  xiiColorGammaUB                m_InnerTintColor = xiiColor::White;
  xiiColorGammaUB                m_MidTintColor   = xiiColor::White;
  xiiColorGammaUB                m_OuterTintColor = xiiColor::White;
  xiiShaderResourceHandle        m_hShader;

  xiiSharedPtr<xiiGALRenderPass> m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;

  xiiSharedPtr<xiiGALBuffer> m_pBloomConstantBuffer;
};
