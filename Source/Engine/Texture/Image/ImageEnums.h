/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>
#include <Texture/TextureDLL.h>

struct XII_TEXTURE_DLL xiiImageAddressMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Repeat,      ///< Repeats the texture on every integer junction.
    Clamp,       ///< Clamps the texture coordinates to the [0.0, 1.0] range.
    ClampBorder, ///< Uses an arbitrary border color for any texture coordinates outside the range [0.0, 1.0].
    Mirror,      ///< Mirrors the texture at every integer boundary.

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
