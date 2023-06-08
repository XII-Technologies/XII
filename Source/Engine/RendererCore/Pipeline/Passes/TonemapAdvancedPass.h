#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

/// \brief Specifies in which mode the tonemap is configured.
struct XII_RENDERERCORE_DLL xiiTonemapMode
{
  using StorageType = xiiInt8;

  enum Enum
  {
    Exponential = 0,
    Reinhard,
    ReinhardModified,
    Uncharted2,
    FilmicALU,
    Logarithmic,
    AdaptiveLogarithmic,
    Lottes,
    Uchimura,
    Unreal,
    Aces,

    ENUM_COUNT,

    Default = Aces
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiTonemapMode);

class XII_RENDERERCORE_DLL xiiTonemapAdvancedPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTonemapAdvancedPass, xiiRenderPipelinePass);

public:
  xiiTonemapAdvancedPass();
  ~xiiTonemapAdvancedPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

protected:
  xiiRenderPipelineNodeInputPin  m_PinColorInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiEnum<xiiTonemapMode> m_TonemapMode;
  bool                    m_bAutoExposure;
  float                   m_fMiddleGray;
  bool                    m_bLightAdaptation;
  float                   m_fWhitePoint;
  float                   m_fLuminanceSaturation;
  float                   m_fAverageLogLum;

  xiiConstantBufferStorageHandle m_hConstantBuffer;
  xiiShaderResourceHandle        m_hShader;
};
