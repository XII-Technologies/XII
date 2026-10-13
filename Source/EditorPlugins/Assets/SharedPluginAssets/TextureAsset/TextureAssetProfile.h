/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

struct xiiTexture2DResolution
{
  using StorageType = xiiInt8;
  enum Enum
  {
    Fixed64x64,
    Fixed128x128,
    Fixed256x256,
    Fixed512x512,
    Fixed1024x1024,
    Fixed2048x2048,
    ProfileResolution1,
    ProfileResolution2,
    Default = Fixed256x256
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiTexture2DResolution);

/// Texture asset transformation settings. Profile dimensions are baked into native target definitions.
class XII_SHAREDPLUGINASSETS_DLL xiiTextureAssetProfileConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureAssetProfileConfig, xiiProfileConfigData);

public:
  xiiUInt16 m_uiMaxResolution           = 1024 * 16;
  xiiUInt16 m_uiRenderTargetResolution1 = 256;
  xiiUInt16 m_uiRenderTargetResolution2 = 512;

  /// Fixed resolutions ignore scale; profile resolutions retain the old 8-pixel quantization and 8..4096 limits.
  xiiResult GetRenderTargetResolution(xiiTexture2DResolution::Enum mode, float fScale, xiiUInt32& out_uiResolution) const;
  xiiUInt64 GetTransformHash() const;
};
