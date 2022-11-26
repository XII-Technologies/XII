#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>
#include <Texture/TextureDLL.h>

struct XII_TEXTURE_DLL xiiImageAddressMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Repeat,
    Clamp,
    ClampBorder,
    Mirror,

    ENUM_COUNT,

    Default = Repeat
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiImageAddressMode);

//////////////////////////////////////////////////////////////////////////
// xiiTextureFilterSetting
//////////////////////////////////////////////////////////////////////////

struct XII_TEXTURE_DLL xiiTextureFilterSetting
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    FixedNearest,
    FixedBilinear,
    FixedTrilinear,
    FixedAnisotropic2x,
    FixedAnisotropic4x,
    FixedAnisotropic8x,
    FixedAnisotropic16x,

    LowestQuality,
    LowQuality,
    DefaultQuality,
    HighQuality,
    HighestQuality,

    Default = DefaultQuality
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiTextureFilterSetting);
