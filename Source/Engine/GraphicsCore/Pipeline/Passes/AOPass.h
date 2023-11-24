#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiAOPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAOPass, xiiRenderPipelinePass);

public:
  xiiAOPass();
  ~xiiAOPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void      ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
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

  xiiConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  xiiConstantBufferStorageHandle m_hSSAOConstantBuffer;

  xiiTexture2DResourceHandle m_hNoiseTexture;

  xiiGALSamplerHandle m_hSSAOSampler;

  xiiShaderResourceHandle m_hDownscaleShader;
  xiiShaderResourceHandle m_hSSAOShader;
  xiiShaderResourceHandle m_hBlurShader;
};
