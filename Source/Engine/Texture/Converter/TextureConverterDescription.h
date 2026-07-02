/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Converter/TextureConverterEnums.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>


struct xiiTextureConverterChannelMapping
{
  xiiInt8                      m_iInputImageIndex = -1;
  xiiTextureConverterChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct xiiTextureConverterSliceChannelMapping
{
  xiiTextureConverterChannelMapping m_Channel[4] = {
    xiiTextureConverterChannelMapping{-1, xiiTextureConverterChannelValue::Red},
    xiiTextureConverterChannelMapping{-1, xiiTextureConverterChannelValue::Green},
    xiiTextureConverterChannelMapping{-1, xiiTextureConverterChannelValue::Blue},
    xiiTextureConverterChannelMapping{-1, xiiTextureConverterChannelValue::Alpha},
  };
};

class XII_TEXTURE_DLL xiiTextureConverterDescription
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTextureConverterDescription);

public:
  xiiTextureConverterDescription() = default;

  xiiHybridArray<xiiString, 4> m_InputFiles;
  xiiDynamicArray<xiiImage>    m_InputImages;

  xiiHybridArray<xiiTextureConverterSliceChannelMapping, 6> m_ChannelMappings;

  // output type / platform
  xiiEnum<xiiTextureConverterOutputType>     m_OutputType;
  xiiEnum<xiiTextureConverterTargetPlatform> m_TargetPlatform;

  // low resolution output
  xiiUInt32 m_uiLowResMipmaps = 0;

  // thumbnail output
  xiiUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  xiiEnum<xiiTextureConverterUsage>           m_Usage;
  xiiEnum<xiiTextureConverterCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  xiiUInt32 m_uiMinResolution  = 16;
  xiiUInt32 m_uiMaxResolution  = 1024 * 8;
  xiiUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  xiiEnum<xiiTextureConverterMipmapMode> m_MipmapMode;
  xiiEnum<xiiGALFilterType>              m_FilterMode; // only used when writing to XII specific formats
  xiiEnum<xiiGALTextureAddressMode>      m_AddressModeU;
  xiiEnum<xiiGALTextureAddressMode>      m_AddressModeV;
  xiiEnum<xiiGALTextureAddressMode>      m_AddressModeW;
  bool                             m_bPreserveMipmapCoverage = false;
  float                            m_fMipmapAlphaThreshold   = 0.5f;

  // Misc options
  xiiUInt8 m_uiDilateColor     = 0;
  bool     m_bFlipHorizontal   = false;
  bool     m_bPremultiplyAlpha = false;
  float    m_fHdrExposureBias  = 0.0f;
  float    m_fMaxValue         = 64000.f;

  // XII specific
  xiiUInt64 m_uiAssetHash    = 0;
  xiiUInt16 m_uiAssetVersion = 0;

  // Texture Atlas
  xiiString m_sTextureAtlasDescFile;

  // Bump map filter
  xiiEnum<xiiTextureConverterBumpMapFilter> m_BumpMapFilter;
};
