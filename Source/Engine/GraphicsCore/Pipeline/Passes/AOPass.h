#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiAOPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAOPass, xiiRenderPipelinePass);

public:
  xiiAOPass();
  ~xiiAOPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> pInputs, xiiArrayPtr<xiiGALTextureCreationDescription> pOutputs) override;
  virtual void      InitializeRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual void      ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  void  SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void  SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSampler();

  xiiRenderPipelineNodeInputPin  m_PinDepthInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius               = 1.0f;
  float m_fMaxScreenSpaceRadius = 1.0f;
  float m_fContrast             = 2.0f;
  float m_fIntensity            = 0.7f;

  float m_fFadeOutStart = 80.0f;
  float m_fFadeOutEnd   = 100.0f;

  float m_fPositionBias       = 5.0f;
  float m_fMipLevelScale      = 10.0f;
  float m_fDepthBlurThreshold = 2.0f;

  xiiSharedPtr<xiiGALRenderPass> m_pRenderPass;
  xiiSharedPtr<xiiGALFramebuffer> m_pFramebuffer;

  xiiSharedPtr<xiiGALBuffer> m_pDownscaleConstantBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pSSAOConstantBuffer;

  xiiTexture2DResourceHandle m_hNoiseTexture;

  xiiSharedPtr<xiiGALSampler> m_pSSAOSampler;

  xiiShaderResourceHandle m_hDownscaleShader;
  xiiShaderResourceHandle m_hSSAOShader;
  xiiShaderResourceHandle m_hBlurShader;
};
