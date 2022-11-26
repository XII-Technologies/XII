#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class XII_RENDERERCORE_DLL xiiAOPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAOPass, xiiRenderPipelinePass);

public:
  xiiAOPass();
  ~xiiAOPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  void  SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void  SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  xiiRenderPipelineNodeInputPin  m_PinDepthInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius;
  float m_fMaxScreenSpaceRadius;
  float m_fContrast;
  float m_fIntensity;

  float m_fFadeOutStart;
  float m_fFadeOutEnd;

  float m_fPositionBias;
  float m_fMipLevelScale;
  float m_fDepthBlurThreshold;

  xiiConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  xiiConstantBufferStorageHandle m_hSSAOConstantBuffer;

  xiiTexture2DResourceHandle m_hNoiseTexture;

  xiiGALSamplerStateHandle m_hSSAOSamplerState;

  xiiShaderResourceHandle m_hDownscaleShader;
  xiiShaderResourceHandle m_hSSAOShader;
  xiiShaderResourceHandle m_hBlurShader;
};
