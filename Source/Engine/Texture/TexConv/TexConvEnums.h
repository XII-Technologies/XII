#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct xiiTexConvOutputType
{
  enum Enum
  {
    None,
    Texture2D,
    Volume,
    Cubemap,
    Atlas,

    Default = Texture2D
  };

  using StorageType = xiiUInt8;
};

struct xiiTexConvCompressionMode
{
  enum Enum
  {
    // Note: order of enum values matters
    None   = 0, // Uncompressed
    Medium = 1, // Compressed with high quality, if possible
    High   = 2, // Strongest compression, if possible

    Default = Medium,
  };

  using StorageType = xiiUInt8;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiTexConvCompressionMode);

struct xiiTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Exact format will be decided together with xiiTexConvCompressionMode

    Color,
    Linear,
    Hdr,

    NormalMap,
    NormalMap_Inverted,

    BumpMap,

    Default = Auto
  };

  using StorageType = xiiUInt8;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiTexConvUsage);

struct xiiTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Kaiser
  };

  using StorageType = xiiUInt8;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TEXTURE_DLL, xiiTexConvMipmapMode);

struct xiiTexConvTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  using StorageType = xiiUInt8;
};

/// \brief Defines which channel of another texture to read to get a value
struct xiiTexConvChannelValue
{
  enum Enum
  {
    Red,   ///< read the RED channel
    Green, ///< read the GREEN channel
    Blue,  ///< read the BLUE channel
    Alpha, ///< read the ALPHA channel

    Black, ///< don't read any channel, just take the constant value 0
    White, ///< don't read any channel, just take the constant value 0xFF / 1.0f
  };
};

/// \brief Defines which filter kernel is used to approximate the x/y bump map gradients
struct xiiTexConvBumpMapFilter
{
  enum Enum
  {
    Finite, ///< Simple finite differences in a 4-Neighborhood
    Sobel,  ///< Sobel kernel (8-Neighborhood)
    Scharr, ///< Scharr kernel (8-Neighborhood)

    Default = Finite
  };

  using StorageType = xiiUInt8;
};
