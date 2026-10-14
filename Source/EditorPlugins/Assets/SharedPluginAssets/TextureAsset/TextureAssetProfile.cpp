/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <SharedPluginAssets/TextureAsset/TextureAssetProfile.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexture2DResolution, 2)
  XII_ENUM_CONSTANTS(xiiTexture2DResolution::Fixed64x64, xiiTexture2DResolution::Fixed128x128, xiiTexture2DResolution::Fixed256x256, xiiTexture2DResolution::Fixed512x512, xiiTexture2DResolution::Fixed1024x1024, xiiTexture2DResolution::Fixed2048x2048)
  XII_ENUM_CONSTANTS(xiiTexture2DResolution::ProfileResolution1, xiiTexture2DResolution::ProfileResolution2)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureAssetProfileConfig, 2, xiiRTTIDefaultAllocator<xiiTextureAssetProfileConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new xiiDefaultValueAttribute(16 * 1024)),
    XII_MEMBER_PROPERTY("RenderTargetResolution1", m_uiRenderTargetResolution1)->AddAttributes(new xiiDefaultValueAttribute(256), new xiiClampValueAttribute(8, 4096)),
    XII_MEMBER_PROPERTY("RenderTargetResolution2", m_uiRenderTargetResolution2)->AddAttributes(new xiiDefaultValueAttribute(512), new xiiClampValueAttribute(8, 4096)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiTextureAssetProfileConfig::GetRenderTargetResolution(xiiTexture2DResolution::Enum mode, float fScale, xiiUInt32& out_uiResolution) const
{
  out_uiResolution = 0;
  if (mode >= xiiTexture2DResolution::Fixed64x64 && mode <= xiiTexture2DResolution::Fixed2048x2048)
  {
    out_uiResolution = 64U << mode;
    return XII_SUCCESS;
  }
  if ((mode != xiiTexture2DResolution::ProfileResolution1 && mode != xiiTexture2DResolution::ProfileResolution2) || !xiiMath::IsFinite(fScale) || fScale <= 0)
    return XII_FAILURE;
  const xiiUInt32 base = mode == xiiTexture2DResolution::ProfileResolution1 ? m_uiRenderTargetResolution1 : m_uiRenderTargetResolution2;
  if (base == 0)
    return XII_FAILURE;
  const float resolution = xiiMath::Clamp(base * fScale, 8.0f, 4096.0f);
  out_uiResolution       = static_cast<xiiUInt32>(xiiMath::Floor((resolution + 7.9f) / 8.0f)) * 8U;
  return XII_SUCCESS;
}

xiiUInt64 xiiTextureAssetProfileConfig::GetTransformHash() const
{
  const xiiUInt32 values[] = {m_uiMaxResolution, m_uiRenderTargetResolution1, m_uiRenderTargetResolution2};
  return xiiHashingUtils::xxHash64(values, sizeof(values));
}
