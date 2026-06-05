/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct xiiTexConvChannelMapping
{
  xiiInt8                      m_iInputImageIndex = -1;
  xiiTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct xiiTexConvSliceChannelMapping
{
  xiiTexConvChannelMapping m_Channel[4] = {
    xiiTexConvChannelMapping{-1, xiiTexConvChannelValue::Red},
    xiiTexConvChannelMapping{-1, xiiTexConvChannelValue::Green},
    xiiTexConvChannelMapping{-1, xiiTexConvChannelValue::Blue},
    xiiTexConvChannelMapping{-1, xiiTexConvChannelValue::Alpha},
  };
};

class XII_TEXTURE_DLL xiiTexConvDesc
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTexConvDesc);

public:
  xiiTexConvDesc() = default;

  xiiHybridArray<xiiString, 4> m_InputFiles;
  xiiDynamicArray<xiiImage>    m_InputImages;

  xiiHybridArray<xiiTexConvSliceChannelMapping, 6> m_ChannelMappings;

  // output type / platform
  xiiEnum<xiiTexConvOutputType>     m_OutputType;
  xiiEnum<xiiTexConvTargetPlatform> m_TargetPlatform;

  // low resolution output
  xiiUInt32 m_uiLowResMipmaps = 0;

  // thumbnail output
  xiiUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  xiiEnum<xiiTexConvUsage>           m_Usage;
  xiiEnum<xiiTexConvCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  xiiUInt32 m_uiMinResolution  = 16;
  xiiUInt32 m_uiMaxResolution  = 1024 * 8;
  xiiUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  xiiEnum<xiiTexConvMipmapMode>    m_MipmapMode;
  xiiEnum<xiiTextureFilterSetting> m_FilterMode; // only used when writing to XII specific formats
  xiiEnum<xiiImageAddressMode>     m_AddressModeU;
  xiiEnum<xiiImageAddressMode>     m_AddressModeV;
  xiiEnum<xiiImageAddressMode>     m_AddressModeW;
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
  xiiEnum<xiiTexConvBumpMapFilter> m_BumpMapFilter;
};
