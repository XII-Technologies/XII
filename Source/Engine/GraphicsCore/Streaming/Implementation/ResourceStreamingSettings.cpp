/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Streaming/ResourceStreamingSettings.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiResourceStreamingSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiResourceStreamingSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("TextureUploadMiB", m_iTextureUploadMiB)->AddAttributes(new xiiClampValueAttribute(0, 1048576)),
    XII_MEMBER_PROPERTY("TextureMipBias", m_fTextureMipBias)->AddAttributes(new xiiClampValueAttribute(-16, 16)),
    XII_MEMBER_PROPERTY("TextureMinimumMip", m_iTextureMinimumMip)->AddAttributes(new xiiClampValueAttribute(0, 31)),
    XII_MEMBER_PROPERTY("TextureCacheMiB", m_iTextureCacheMiB)->AddAttributes(new xiiClampValueAttribute(0, 1048576)),
    XII_MEMBER_PROPERTY("GeometryCacheMiB", m_iGeometryCacheMiB)->AddAttributes(new xiiClampValueAttribute(0, 1048576)),
    XII_MEMBER_PROPERTY("PressureTrimInterval", m_uiPressureTrimInterval)->AddAttributes(new xiiClampValueAttribute(1, 1000000)),
    XII_MEMBER_PROPERTY("PressureMinimumIdleFrames", m_uiPressureMinimumIdleFrames)->AddAttributes(new xiiClampValueAttribute(0, 1000000)),
    XII_MEMBER_PROPERTY("PressureMaxEvictions", m_uiPressureMaxEvictions)->AddAttributes(new xiiClampValueAttribute(1, 1000000)),
  } XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiResourceStreamingSettings::Validate() const
{
  return xiiMath::IsFinite(m_fTextureMipBias) && m_fTextureMipBias >= -16 && m_fTextureMipBias <= 16 && m_iTextureUploadMiB >= 0 && m_iTextureUploadMiB <= 1048576 && m_iTextureMinimumMip >= 0 && m_iTextureMinimumMip <= 31 && m_iTextureCacheMiB >= 0 && m_iTextureCacheMiB <= 1048576 && m_iGeometryCacheMiB >= 0 && m_iGeometryCacheMiB <= 1048576 && m_uiPressureTrimInterval > 0 && m_uiPressureTrimInterval <= 1000000U && m_uiPressureMinimumIdleFrames <= 1000000U && m_uiPressureMaxEvictions > 0 && m_uiPressureMaxEvictions <= 1000000U ? XII_SUCCESS : XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Streaming_Implementation_ResourceStreamingSettings);
