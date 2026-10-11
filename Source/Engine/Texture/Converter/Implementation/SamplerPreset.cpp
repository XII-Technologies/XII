/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <GraphicsFoundation/Resources/Sampler.h>
#include <Texture/Converter/TextureConverterEnums.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTextureConverterSamplerPreset, 1)
  XII_ENUM_CONSTANTS(xiiTextureConverterSamplerPreset::Nearest, xiiTextureConverterSamplerPreset::Linear, xiiTextureConverterSamplerPreset::Trilinear)
  XII_ENUM_CONSTANTS(xiiTextureConverterSamplerPreset::Aniso2x, xiiTextureConverterSamplerPreset::Aniso4x, xiiTextureConverterSamplerPreset::Aniso8x, xiiTextureConverterSamplerPreset::Aniso16x)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiResult xiiTextureConverterSamplerPreset::ConfigureSampler(Enum preset, xiiGALSamplerCreationDescription& inout_description)
{
  if (preset < Nearest || preset > Aniso16x)
    return XII_FAILURE;

  inout_description.m_MinFilter = preset == Nearest ? xiiGALFilterType::Point : (preset >= Aniso2x ? xiiGALFilterType::Anisotropic : xiiGALFilterType::Linear);
  inout_description.m_MagFilter = inout_description.m_MinFilter;
  inout_description.m_MipFilter = inout_description.m_MinFilter;
  if (preset <= Linear)
    inout_description.m_MipFilter = xiiGALFilterType::Point;
  inout_description.m_uiMaxAnisotropy = preset >= Aniso2x ? 1U << (preset - 2) : 0U;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_Converter_Implementation_SamplerPreset);
