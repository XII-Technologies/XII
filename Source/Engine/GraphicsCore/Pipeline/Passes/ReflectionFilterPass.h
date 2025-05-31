#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class XII_GRAPHICSCORE_DLL xiiReflectionFilterPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionFilterPass, xiiRenderPipelinePass);

public:
  xiiReflectionFilterPass();
  ~xiiReflectionFilterPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiUInt32 GetInputCubemap() const;
  void      SetInputCubemap(xiiUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(xiiUInt32 uiMipMapIndex, xiiUInt32 uiNumMipMaps);
  void UpdateIrradianceConstantBuffer();

  xiiRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  xiiRenderPipelineNodeOutputPin m_PinAvgLuminance;
  xiiRenderPipelineNodeOutputPin m_PinIrradianceData;

  float     m_fIntensity              = 1.0f;
  float     m_fSaturation             = 1.0f;
  xiiUInt32 m_uiSpecularOutputIndex   = 0;
  xiiUInt32 m_uiIrradianceOutputIndex = 0;

  xiiSharedPtr<xiiGALTexture> m_pInputCubemap;

  xiiSharedPtr<xiiGALBuffer> m_pFilteredSpecularConstantBuffer;
  xiiShaderResourceHandle    m_hFilteredSpecularShader;

  xiiSharedPtr<xiiGALBuffer> m_pIrradianceConstantBuffer;
  xiiShaderResourceHandle    m_hIrradianceShader;
};
