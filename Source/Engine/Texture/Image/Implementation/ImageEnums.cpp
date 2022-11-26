#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiImageAddressMode, 1)
  XII_ENUM_CONSTANT(xiiImageAddressMode::Repeat),
  XII_ENUM_CONSTANT(xiiImageAddressMode::Clamp),
  XII_ENUM_CONSTANT(xiiImageAddressMode::ClampBorder),
  XII_ENUM_CONSTANT(xiiImageAddressMode::Mirror),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTextureFilterSetting, 1)
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedNearest),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedBilinear),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedTrilinear),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedAnisotropic2x),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedAnisotropic4x),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedAnisotropic8x),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::FixedAnisotropic16x),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::LowestQuality),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::LowQuality),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::DefaultQuality),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::HighQuality),
  XII_ENUM_CONSTANT(xiiTextureFilterSetting::HighestQuality),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on


XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageEnums);
