#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>

class XII_RENDERERCORE_DLL xiiTonemapPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTonemapPass, xiiRenderPipelinePass);

public:
  xiiTonemapPass();
  ~xiiTonemapPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

protected:
  xiiRenderPipelineNodeInputPin  m_PinColorInput;
  xiiRenderPipelineNodeInputPin  m_PinBloomInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  void        SetVignettingTextureFile(const char* szFile);
  const char* GetVignettingTextureFile() const;

  void        SetLUT1TextureFile(const char* szFile);
  const char* GetLUT1TextureFile() const;

  void        SetLUT2TextureFile(const char* szFile);
  const char* GetLUT2TextureFile() const;

  xiiTexture2DResourceHandle m_hVignettingTexture;
  xiiTexture2DResourceHandle m_hNoiseTexture;
  xiiTexture3DResourceHandle m_hLUT1;
  xiiTexture3DResourceHandle m_hLUT2;

  xiiColor m_MoodColor;
  float    m_fMoodStrength;
  float    m_fSaturation;
  float    m_fContrast;
  float    m_fLut1Strength;
  float    m_fLut2Strength;

  xiiConstantBufferStorageHandle m_hConstantBuffer;
  xiiShaderResourceHandle        m_hShader;
};
