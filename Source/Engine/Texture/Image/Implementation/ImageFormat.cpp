#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Texture/Image/ImageFormat.h>

namespace
{
  struct xiiImageFormatMetaData
  {
    xiiImageFormatMetaData()
    {
      xiiMemoryUtils::ZeroFillArray(m_uiBitsPerChannel);
      xiiMemoryUtils::ZeroFillArray(m_uiChannelMasks);

      m_planeData.SetCount(1);
    }

    const char* m_szName{nullptr};

    struct PlaneData
    {
      xiiUInt8             m_uiBitsPerBlock{0}; ///< Bits per block for compressed formats; for uncompressed formats (which always have a block size of 1x1x1), this is equal to bits per pixel.
      xiiUInt8             m_uiBlockWidth{1};
      xiiUInt8             m_uiBlockHeight{1};
      xiiUInt8             m_uiBlockDepth{1};
      xiiImageFormat::Enum m_subFormat{xiiImageFormat::UNKNOWN}; ///< Subformats when viewing only a subslice of the data.
    };

    xiiStaticArray<PlaneData, 2> m_planeData;

    xiiUInt8 m_uiNumChannels{0};

    xiiUInt8  m_uiBitsPerChannel[xiiImageFormatChannel::COUNT];
    xiiUInt32 m_uiChannelMasks[xiiImageFormatChannel::COUNT];


    bool m_requireFirstLevelBlockAligned{false}; ///< Only for compressed formats: If true, the first level's dimensions must be a multiple of the
                                                 ///< block size; if false, padding can be applied for compressing the first mip level, too.
    bool m_isDepth{false};
    bool m_isStencil{false};

    xiiImageFormatDataType::Enum m_dataType{xiiImageFormatDataType::NONE};
    xiiImageFormatType::Enum     m_formatType{xiiImageFormatType::UNKNOWN};

    xiiImageFormat::Enum m_asLinear{xiiImageFormat::UNKNOWN};
    xiiImageFormat::Enum m_asSrgb{xiiImageFormat::UNKNOWN};

    xiiUInt32 getNumBlocksX(xiiUInt32 uiWidth, xiiUInt32 uiPlaneIndex) const
    {
      return (uiWidth - 1) / m_planeData[uiPlaneIndex].m_uiBlockWidth + 1;
    }

    xiiUInt32 getNumBlocksY(xiiUInt32 uiHeight, xiiUInt32 uiPlaneIndex) const
    {
      return (uiHeight - 1) / m_planeData[uiPlaneIndex].m_uiBlockHeight + 1;
    }

    xiiUInt32 getNumBlocksZ(xiiUInt32 uiDepth, xiiUInt32 uiPlaneIndex) const
    {
      return (uiDepth - 1) / m_planeData[uiPlaneIndex].m_uiBlockDepth + 1;
    }

    xiiUInt32 getRowPitch(xiiUInt32 uiWidth, xiiUInt32 uiPlaneIndex) const
    {
      return getNumBlocksX(uiWidth, uiPlaneIndex) * m_planeData[uiPlaneIndex].m_uiBitsPerBlock / 8;
    }
  };

  xiiStaticArray<xiiImageFormatMetaData, xiiImageFormat::NUM_FORMATS> s_formatMetaData;

  void InitFormatLinear(xiiImageFormat::Enum format, const char* szName, xiiImageFormatDataType::Enum dataType, xiiUInt8 uiBitsPerPixel, xiiUInt8 uiBitsR, xiiUInt8 uiBitsG, xiiUInt8 uiBitsB, xiiUInt8 uiBitsA, xiiUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType                      = dataType;
    s_formatMetaData[format].m_formatType                    = xiiImageFormatType::LINEAR;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::R] = uiBitsR;
    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::G] = uiBitsG;
    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::B] = uiBitsB;
    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::A] = uiBitsA;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb   = format;
  }

#define INIT_FORMAT_LINEAR(format, dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels) \
  InitFormatLinear(xiiImageFormat::format, #format, xiiImageFormatDataType::dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels)

  void InitFormatCompressed(xiiImageFormat::Enum format, const char* szName, xiiImageFormatDataType::Enum dataType, xiiUInt8 uiBitsPerBlock, xiiUInt8 uiBlockWidth, xiiUInt8 uiBlockHeight, xiiUInt8 uiBlockDepth, bool bRequireFirstLevelBlockAligned, xiiUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerBlock;
    s_formatMetaData[format].m_planeData[0].m_uiBlockWidth   = uiBlockWidth;
    s_formatMetaData[format].m_planeData[0].m_uiBlockHeight  = uiBlockHeight;
    s_formatMetaData[format].m_planeData[0].m_uiBlockDepth   = uiBlockDepth;
    s_formatMetaData[format].m_dataType                      = dataType;
    s_formatMetaData[format].m_formatType                    = xiiImageFormatType::BLOCK_COMPRESSED;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_requireFirstLevelBlockAligned = bRequireFirstLevelBlockAligned;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb   = format;
  }

#define INIT_FORMAT_COMPRESSED(                                                                                                                      \
  format, dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, requireFirstLevelBlockAligned, uiNumChannels)                         \
  InitFormatCompressed(xiiImageFormat::format, #format, xiiImageFormatDataType::dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, \
                       requireFirstLevelBlockAligned, uiNumChannels)

  void InitFormatDepth(xiiImageFormat::Enum format, const char* szName, xiiImageFormatDataType::Enum dataType, xiiUInt8 uiBitsPerPixel, bool bIsStencil, xiiUInt8 uiBitsD, xiiUInt8 uiBitsS)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType                      = dataType;
    s_formatMetaData[format].m_formatType                    = xiiImageFormatType::LINEAR;

    s_formatMetaData[format].m_isDepth   = true;
    s_formatMetaData[format].m_isStencil = bIsStencil;

    s_formatMetaData[format].m_uiNumChannels = bIsStencil ? 2 : 1;

    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::D] = uiBitsD;
    s_formatMetaData[format].m_uiBitsPerChannel[xiiImageFormatChannel::S] = uiBitsS;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb   = format;
  }

#define INIT_FORMAT_DEPTH(format, dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS) \
  InitFormatDepth(xiiImageFormat::format, #format, xiiImageFormatDataType::dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS);

  void SetupSrgbPair(xiiImageFormat::Enum linearFormat, xiiImageFormat::Enum srgbFormat)
  {
    s_formatMetaData[linearFormat].m_asLinear = linearFormat;
    s_formatMetaData[linearFormat].m_asSrgb   = srgbFormat;

    s_formatMetaData[srgbFormat].m_asLinear = linearFormat;
    s_formatMetaData[srgbFormat].m_asSrgb   = srgbFormat;
  }

} // namespace

static void SetupImageFormatTable()
{
  if (!s_formatMetaData.IsEmpty())
    return;

  s_formatMetaData.SetCount(xiiImageFormat::NUM_FORMATS);

  s_formatMetaData[xiiImageFormat::UNKNOWN].m_szName = "UNKNOWN";

  INIT_FORMAT_LINEAR(R32G32B32A32_FLOAT, FLOAT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_UINT, UINT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_SINT, SINT, 128, 32, 32, 32, 32, 4);

  INIT_FORMAT_LINEAR(R32G32B32_FLOAT, FLOAT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_UINT, UINT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_SINT, SINT, 96, 32, 32, 32, 0, 3);

  INIT_FORMAT_LINEAR(R32G32_FLOAT, FLOAT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_UINT, UINT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_SINT, SINT, 64, 32, 32, 0, 0, 2);

  INIT_FORMAT_LINEAR(R32_FLOAT, FLOAT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_UINT, UINT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_SINT, SINT, 32, 32, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R16G16B16A16_FLOAT, FLOAT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UINT, UINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SINT, SINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UNORM, UNORM, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SNORM, SNORM, 64, 16, 16, 16, 16, 4);

  INIT_FORMAT_LINEAR(R16G16B16_UNORM, UNORM, 48, 16, 16, 16, 0, 3);

  INIT_FORMAT_LINEAR(R16G16_FLOAT, FLOAT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UINT, UINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SINT, SINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UNORM, UNORM, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SNORM, SNORM, 32, 16, 16, 0, 0, 2);

  INIT_FORMAT_LINEAR(R16_FLOAT, FLOAT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UINT, UINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SINT, SINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UNORM, UNORM, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SNORM, SNORM, 16, 16, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8G8B8A8_UINT, UINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SINT, SINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SNORM, SNORM, 32, 8, 8, 8, 8, 4);

  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(xiiImageFormat::R8G8B8A8_UNORM, xiiImageFormat::R8G8B8A8_UNORM_SRGB);

  s_formatMetaData[xiiImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x000000FF;
  s_formatMetaData[xiiImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[xiiImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x00FF0000;
  s_formatMetaData[xiiImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(R8G8B8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(R8G8B8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(xiiImageFormat::R8G8B8_UNORM, xiiImageFormat::R8G8B8_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(xiiImageFormat::B8G8R8A8_UNORM, xiiImageFormat::B8G8R8A8_UNORM_SRGB);

  s_formatMetaData[xiiImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[xiiImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[xiiImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[xiiImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM, UNORM, 32, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 0, 3);
  SetupSrgbPair(xiiImageFormat::B8G8R8X8_UNORM, xiiImageFormat::B8G8R8X8_UNORM_SRGB);

  s_formatMetaData[xiiImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[xiiImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[xiiImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[xiiImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(B8G8R8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(xiiImageFormat::B8G8R8_UNORM, xiiImageFormat::B8G8R8_UNORM_SRGB);

  s_formatMetaData[xiiImageFormat::B8G8R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[xiiImageFormat::B8G8R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[xiiImageFormat::B8G8R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[xiiImageFormat::B8G8R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(R8G8_UINT, UINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SINT, SINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_UNORM, UNORM, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SNORM, SNORM, 16, 8, 8, 0, 0, 2);

  INIT_FORMAT_LINEAR(R8_UINT, UINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SINT, SINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SNORM, SNORM, 8, 8, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8_UNORM, UNORM, 8, 8, 0, 0, 0, 1);
  s_formatMetaData[xiiImageFormat::R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0xFF;
  s_formatMetaData[xiiImageFormat::R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x00;
  s_formatMetaData[xiiImageFormat::R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x00;
  s_formatMetaData[xiiImageFormat::R8_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x00;

  INIT_FORMAT_COMPRESSED(BC1_UNORM, UNORM, 64, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC1_UNORM_SRGB, UNORM, 64, 4, 4, 1, true, 4);
  SetupSrgbPair(xiiImageFormat::BC1_UNORM, xiiImageFormat::BC1_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC2_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC2_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(xiiImageFormat::BC2_UNORM, xiiImageFormat::BC2_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC3_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC3_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(xiiImageFormat::BC3_UNORM, xiiImageFormat::BC3_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC4_UNORM, UNORM, 64, 4, 4, 1, true, 1);
  INIT_FORMAT_COMPRESSED(BC4_SNORM, SNORM, 64, 4, 4, 1, true, 1);

  INIT_FORMAT_COMPRESSED(BC5_UNORM, UNORM, 128, 4, 4, 1, true, 2);
  INIT_FORMAT_COMPRESSED(BC5_SNORM, SNORM, 128, 4, 4, 1, true, 2);

  INIT_FORMAT_COMPRESSED(BC6H_UF16, FLOAT, 128, 4, 4, 1, true, 3);
  INIT_FORMAT_COMPRESSED(BC6H_SF16, FLOAT, 128, 4, 4, 1, true, 3);

  INIT_FORMAT_COMPRESSED(BC7_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC7_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(xiiImageFormat::BC7_UNORM, xiiImageFormat::BC7_UNORM_SRGB);



  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 4);
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 4);
  SetupSrgbPair(xiiImageFormat::B5G5R5A1_UNORM, xiiImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[xiiImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x0F00;
  s_formatMetaData[xiiImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x00F0;
  s_formatMetaData[xiiImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x000F;
  s_formatMetaData[xiiImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0xF000;
  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(xiiImageFormat::B4G4R4A4_UNORM, xiiImageFormat::B4G4R4A4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[xiiImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0xF000;
  s_formatMetaData[xiiImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x0F00;
  s_formatMetaData[xiiImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x00F0;
  s_formatMetaData[xiiImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x000F;
  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(xiiImageFormat::A4B4G4R4_UNORM, xiiImageFormat::A4B4G4R4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G6R5_UNORM, UNORM, 16, 5, 6, 5, 0, 3);
  s_formatMetaData[xiiImageFormat::B5G6R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0xF800;
  s_formatMetaData[xiiImageFormat::B5G6R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x07E0;
  s_formatMetaData[xiiImageFormat::B5G6R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x001F;
  s_formatMetaData[xiiImageFormat::B5G6R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G6R5_UNORM_SRGB, UNORM, 16, 5, 6, 5, 0, 3);
  SetupSrgbPair(xiiImageFormat::B5G6R5_UNORM, xiiImageFormat::B5G6R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[xiiImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[xiiImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[xiiImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x001F;
  s_formatMetaData[xiiImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(xiiImageFormat::B5G5R5A1_UNORM, xiiImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM, UNORM, 16, 5, 5, 5, 0, 3);
  s_formatMetaData[xiiImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[xiiImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[xiiImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x001F;
  s_formatMetaData[xiiImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 0, 3);
  SetupSrgbPair(xiiImageFormat::B5G5R5X1_UNORM, xiiImageFormat::B5G5R5X1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[xiiImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[xiiImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[xiiImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x001F;
  s_formatMetaData[xiiImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(xiiImageFormat::A1B5G5R5_UNORM, xiiImageFormat::A1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[xiiImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[xiiImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[xiiImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x001F;
  s_formatMetaData[xiiImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(xiiImageFormat::X1B5G5R5_UNORM, xiiImageFormat::X1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(R11G11B10_FLOAT, FLOAT, 32, 11, 11, 10, 0, 3);
  INIT_FORMAT_LINEAR(R10G10B10A2_UINT, UINT, 32, 10, 10, 10, 2, 4);
  INIT_FORMAT_LINEAR(R10G10B10A2_UNORM, UNORM, 32, 10, 10, 10, 2, 4);

  // msdn.microsoft.com/library/windows/desktop/bb943991(v=vs.85).aspx documents R10G10B10A2 as having an alpha mask of 0
  s_formatMetaData[xiiImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[xiiImageFormatChannel::R] = 0x000003FF;
  s_formatMetaData[xiiImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[xiiImageFormatChannel::G] = 0x000FFC00;
  s_formatMetaData[xiiImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[xiiImageFormatChannel::B] = 0x3FF00000;
  s_formatMetaData[xiiImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[xiiImageFormatChannel::A] = 0;

  INIT_FORMAT_DEPTH(D32_FLOAT, DEPTH_STENCIL, 32, false, 32, 0);
  INIT_FORMAT_DEPTH(D32_FLOAT_S8X24_UINT, DEPTH_STENCIL, 64, true, 32, 8);
  INIT_FORMAT_DEPTH(D24_UNORM_S8_UINT, DEPTH_STENCIL, 32, true, 24, 8);
  INIT_FORMAT_DEPTH(D16_UNORM, DEPTH_STENCIL, 16, false, 16, 0);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM, UNORM, 128, 12, 12, 1, false, 4);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM_SRGB, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM_SRGB, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM_SRGB, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM_SRGB, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM_SRGB, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM_SRGB, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM_SRGB, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM_SRGB, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM_SRGB, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM_SRGB, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM_SRGB, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM_SRGB, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM_SRGB, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM_SRGB, UNORM, 128, 12, 12, 1, false, 4);

  SetupSrgbPair(xiiImageFormat::ASTC_4x4_UNORM, xiiImageFormat::ASTC_4x4_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_5x4_UNORM, xiiImageFormat::ASTC_5x4_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_5x5_UNORM, xiiImageFormat::ASTC_5x5_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_6x5_UNORM, xiiImageFormat::ASTC_6x5_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_6x6_UNORM, xiiImageFormat::ASTC_6x6_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_8x5_UNORM, xiiImageFormat::ASTC_8x5_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_8x6_UNORM, xiiImageFormat::ASTC_8x6_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_10x5_UNORM, xiiImageFormat::ASTC_10x5_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_10x6_UNORM, xiiImageFormat::ASTC_10x6_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_8x8_UNORM, xiiImageFormat::ASTC_8x8_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_10x8_UNORM, xiiImageFormat::ASTC_10x8_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_10x10_UNORM, xiiImageFormat::ASTC_10x10_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_12x10_UNORM, xiiImageFormat::ASTC_12x10_UNORM_SRGB);
  SetupSrgbPair(xiiImageFormat::ASTC_12x12_UNORM, xiiImageFormat::ASTC_12x12_UNORM_SRGB);

  s_formatMetaData[xiiImageFormat::NV12].m_szName        = "NV12";
  s_formatMetaData[xiiImageFormat::NV12].m_formatType    = xiiImageFormatType::PLANAR;
  s_formatMetaData[xiiImageFormat::NV12].m_uiNumChannels = 3;

  s_formatMetaData[xiiImageFormat::NV12].m_planeData.SetCount(2);

  s_formatMetaData[xiiImageFormat::NV12].m_planeData[0].m_uiBitsPerBlock = 8;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[0].m_uiBlockWidth   = 1;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[0].m_uiBlockHeight  = 1;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[0].m_uiBlockDepth   = 1;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[0].m_subFormat      = xiiImageFormat::R8_UNORM;

  s_formatMetaData[xiiImageFormat::NV12].m_planeData[1].m_uiBitsPerBlock = 16;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[1].m_uiBlockWidth   = 2;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[1].m_uiBlockHeight  = 2;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[1].m_uiBlockDepth   = 1;
  s_formatMetaData[xiiImageFormat::NV12].m_planeData[1].m_subFormat      = xiiImageFormat::R8G8_UNORM;
}

static const XII_ALWAYS_INLINE xiiImageFormatMetaData& GetImageFormatMetaData(xiiImageFormat::Enum format)
{
  if (s_formatMetaData.IsEmpty())
  {
    SetupImageFormatTable();
  }

  return s_formatMetaData[format];
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Image, ImageFormats)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    SetupImageFormatTable();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiUInt32 xiiImageFormat::GetBitsPerPixel(Enum format, xiiUInt32 uiPlaneIndex)
{
  const xiiImageFormatMetaData& metaData       = GetImageFormatMetaData(format);
  auto                          pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return (metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock + pixelsPerBlock - 1) / pixelsPerBlock; // Return rounded-up value
}


float xiiImageFormat::GetExactBitsPerPixel(Enum format, xiiUInt32 uiPlaneIndex)
{
  const xiiImageFormatMetaData& metaData       = GetImageFormatMetaData(format);
  auto                          pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return static_cast<float>(metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock) / pixelsPerBlock;
}


xiiUInt32 xiiImageFormat::GetBitsPerBlock(Enum format, xiiUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBitsPerBlock;
}


xiiUInt32 xiiImageFormat::GetNumChannels(Enum format)
{
  return GetImageFormatMetaData(format).m_uiNumChannels;
}

xiiImageFormat::Enum xiiImageFormat::FromPixelMask(
  xiiUInt32 uiRedMask,
  xiiUInt32 uiGreenMask,
  xiiUInt32 uiBlueMask,
  xiiUInt32 uiAlphaMask,
  xiiUInt32 uiBitsPerPixel)
{
  // Some DDS files in the wild are encoded as this
  if (uiBitsPerPixel == 8 && uiRedMask == 0xff && uiGreenMask == 0xff && uiBlueMask == 0xff)
  {
    return R8_UNORM;
  }

  for (xiiUInt32 index = 0; index < NUM_FORMATS; index++)
  {
    Enum format = static_cast<Enum>(index);
    if (GetChannelMask(format, xiiImageFormatChannel::R) == uiRedMask && GetChannelMask(format, xiiImageFormatChannel::G) == uiGreenMask &&
        GetChannelMask(format, xiiImageFormatChannel::B) == uiBlueMask && GetChannelMask(format, xiiImageFormatChannel::A) == uiAlphaMask &&
        GetBitsPerPixel(format) == uiBitsPerPixel && GetDataType(format) == xiiImageFormatDataType::UNORM && !IsCompressed(format))
    {
      return format;
    }
  }

  return UNKNOWN;
}


xiiImageFormat::Enum xiiImageFormat::GetPlaneSubFormat(Enum format, xiiUInt32 uiPlaneIndex)
{
  const auto& metadata = GetImageFormatMetaData(format);

  if (metadata.m_formatType == xiiImageFormatType::PLANAR)
  {
    return metadata.m_planeData[uiPlaneIndex].m_subFormat;
  }
  else
  {
    XII_ASSERT_DEV(uiPlaneIndex == 0, "Invalid plane index {0} for format {0}", uiPlaneIndex, xiiImageFormat::GetName(format));
    return format;
  }
}

bool xiiImageFormat::IsCompatible(Enum left, Enum right)
{
  if (left == right)
  {
    return true;
  }
  switch (left)
  {
    case xiiImageFormat::R32G32B32A32_FLOAT:
    case xiiImageFormat::R32G32B32A32_UINT:
    case xiiImageFormat::R32G32B32A32_SINT:
      return (right == xiiImageFormat::R32G32B32A32_FLOAT || right == xiiImageFormat::R32G32B32A32_UINT || right == xiiImageFormat::R32G32B32A32_SINT);
    case xiiImageFormat::R32G32B32_FLOAT:
    case xiiImageFormat::R32G32B32_UINT:
    case xiiImageFormat::R32G32B32_SINT:
      return (right == xiiImageFormat::R32G32B32_FLOAT || right == xiiImageFormat::R32G32B32_UINT || right == xiiImageFormat::R32G32B32_SINT);
    case xiiImageFormat::R32G32_FLOAT:
    case xiiImageFormat::R32G32_UINT:
    case xiiImageFormat::R32G32_SINT:
      return (right == xiiImageFormat::R32G32_FLOAT || right == xiiImageFormat::R32G32_UINT || right == xiiImageFormat::R32G32_SINT);
    case xiiImageFormat::R32_FLOAT:
    case xiiImageFormat::R32_UINT:
    case xiiImageFormat::R32_SINT:
      return (right == xiiImageFormat::R32_FLOAT || right == xiiImageFormat::R32_UINT || right == xiiImageFormat::R32_SINT);
    case xiiImageFormat::R16G16B16A16_FLOAT:
    case xiiImageFormat::R16G16B16A16_UINT:
    case xiiImageFormat::R16G16B16A16_SINT:
    case xiiImageFormat::R16G16B16A16_UNORM:
    case xiiImageFormat::R16G16B16A16_SNORM:
      return (right == xiiImageFormat::R16G16B16A16_FLOAT || right == xiiImageFormat::R16G16B16A16_UINT || right == xiiImageFormat::R16G16B16A16_SINT ||
              right == xiiImageFormat::R16G16B16A16_UNORM || right == xiiImageFormat::R16G16B16A16_SNORM);
    case xiiImageFormat::R16G16_FLOAT:
    case xiiImageFormat::R16G16_UINT:
    case xiiImageFormat::R16G16_SINT:
    case xiiImageFormat::R16G16_UNORM:
    case xiiImageFormat::R16G16_SNORM:
      return (right == xiiImageFormat::R16G16_FLOAT || right == xiiImageFormat::R16G16_UINT || right == xiiImageFormat::R16G16_SINT ||
              right == xiiImageFormat::R16G16_UNORM || right == xiiImageFormat::R16G16_SNORM);
    case xiiImageFormat::R8G8B8A8_UINT:
    case xiiImageFormat::R8G8B8A8_SINT:
    case xiiImageFormat::R8G8B8A8_UNORM:
    case xiiImageFormat::R8G8B8A8_SNORM:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
      return (right == xiiImageFormat::R8G8B8A8_UINT || right == xiiImageFormat::R8G8B8A8_SINT || right == xiiImageFormat::R8G8B8A8_UNORM ||
              right == xiiImageFormat::R8G8B8A8_SNORM || right == xiiImageFormat::R8G8B8A8_UNORM_SRGB);
    case xiiImageFormat::B8G8R8A8_UNORM:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
      return (right == xiiImageFormat::B8G8R8A8_UNORM || right == xiiImageFormat::B8G8R8A8_UNORM_SRGB);
    case xiiImageFormat::B8G8R8X8_UNORM:
    case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
      return (right == xiiImageFormat::B8G8R8X8_UNORM || right == xiiImageFormat::B8G8R8X8_UNORM_SRGB);
    case xiiImageFormat::B8G8R8_UNORM:
    case xiiImageFormat::B8G8R8_UNORM_SRGB:
      return (right == xiiImageFormat::B8G8R8_UNORM || right == xiiImageFormat::B8G8R8_UNORM_SRGB);
    case xiiImageFormat::R8G8_UINT:
    case xiiImageFormat::R8G8_SINT:
    case xiiImageFormat::R8G8_UNORM:
    case xiiImageFormat::R8G8_SNORM:
      return (right == xiiImageFormat::R8G8_UINT || right == xiiImageFormat::R8G8_SINT || right == xiiImageFormat::R8G8_UNORM ||
              right == xiiImageFormat::R8G8_SNORM);
    case xiiImageFormat::R8_UINT:
    case xiiImageFormat::R8_SINT:
    case xiiImageFormat::R8_UNORM:
    case xiiImageFormat::R8_SNORM:
      return (
        right == xiiImageFormat::R8_UINT || right == xiiImageFormat::R8_SINT || right == xiiImageFormat::R8_UNORM || right == xiiImageFormat::R8_SNORM);
    case xiiImageFormat::BC1_UNORM:
    case xiiImageFormat::BC1_UNORM_SRGB:
      return (right == xiiImageFormat::BC1_UNORM || right == xiiImageFormat::BC1_UNORM_SRGB);
    case xiiImageFormat::BC2_UNORM:
    case xiiImageFormat::BC2_UNORM_SRGB:
      return (right == xiiImageFormat::BC2_UNORM || right == xiiImageFormat::BC2_UNORM_SRGB);
    case xiiImageFormat::BC3_UNORM:
    case xiiImageFormat::BC3_UNORM_SRGB:
      return (right == xiiImageFormat::BC3_UNORM || right == xiiImageFormat::BC3_UNORM_SRGB);
    case xiiImageFormat::BC4_UNORM:
    case xiiImageFormat::BC4_SNORM:
      return (right == xiiImageFormat::BC4_UNORM || right == xiiImageFormat::BC4_SNORM);
    case xiiImageFormat::BC5_UNORM:
    case xiiImageFormat::BC5_SNORM:
      return (right == xiiImageFormat::BC5_UNORM || right == xiiImageFormat::BC5_SNORM);
    case xiiImageFormat::BC6H_UF16:
    case xiiImageFormat::BC6H_SF16:
      return (right == xiiImageFormat::BC6H_UF16 || right == xiiImageFormat::BC6H_SF16);
    case xiiImageFormat::BC7_UNORM:
    case xiiImageFormat::BC7_UNORM_SRGB:
      return (right == xiiImageFormat::BC7_UNORM || right == xiiImageFormat::BC7_UNORM_SRGB);
    case xiiImageFormat::R10G10B10A2_UINT:
    case xiiImageFormat::R10G10B10A2_UNORM:
      return (right == xiiImageFormat::R10G10B10A2_UINT || right == xiiImageFormat::R10G10B10A2_UNORM);
    case xiiImageFormat::ASTC_4x4_UNORM:
    case xiiImageFormat::ASTC_4x4_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_4x4_UNORM || right == xiiImageFormat::ASTC_4x4_UNORM_SRGB);
    case xiiImageFormat::ASTC_5x4_UNORM:
    case xiiImageFormat::ASTC_5x4_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_5x4_UNORM || right == xiiImageFormat::ASTC_5x4_UNORM_SRGB);
    case xiiImageFormat::ASTC_5x5_UNORM:
    case xiiImageFormat::ASTC_5x5_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_5x5_UNORM || right == xiiImageFormat::ASTC_5x5_UNORM_SRGB);
    case xiiImageFormat::ASTC_6x5_UNORM:
    case xiiImageFormat::ASTC_6x5_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_6x5_UNORM || right == xiiImageFormat::ASTC_6x5_UNORM_SRGB);
    case xiiImageFormat::ASTC_6x6_UNORM:
    case xiiImageFormat::ASTC_6x6_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_6x6_UNORM || right == xiiImageFormat::ASTC_6x6_UNORM_SRGB);
    case xiiImageFormat::ASTC_8x5_UNORM:
    case xiiImageFormat::ASTC_8x5_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_8x5_UNORM || right == xiiImageFormat::ASTC_8x5_UNORM_SRGB);
    case xiiImageFormat::ASTC_8x6_UNORM:
    case xiiImageFormat::ASTC_8x6_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_8x6_UNORM || right == xiiImageFormat::ASTC_8x6_UNORM_SRGB);
    case xiiImageFormat::ASTC_10x5_UNORM:
    case xiiImageFormat::ASTC_10x5_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_10x5_UNORM || right == xiiImageFormat::ASTC_10x5_UNORM_SRGB);
    case xiiImageFormat::ASTC_10x6_UNORM:
    case xiiImageFormat::ASTC_10x6_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_10x6_UNORM || right == xiiImageFormat::ASTC_10x6_UNORM_SRGB);
    case xiiImageFormat::ASTC_8x8_UNORM:
    case xiiImageFormat::ASTC_8x8_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_8x8_UNORM || right == xiiImageFormat::ASTC_8x8_UNORM_SRGB);
    case xiiImageFormat::ASTC_10x8_UNORM:
    case xiiImageFormat::ASTC_10x8_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_10x8_UNORM || right == xiiImageFormat::ASTC_10x8_UNORM_SRGB);
    case xiiImageFormat::ASTC_10x10_UNORM:
    case xiiImageFormat::ASTC_10x10_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_10x10_UNORM || right == xiiImageFormat::ASTC_10x10_UNORM_SRGB);
    case xiiImageFormat::ASTC_12x10_UNORM:
    case xiiImageFormat::ASTC_12x10_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_12x10_UNORM || right == xiiImageFormat::ASTC_12x10_UNORM_SRGB);
    case xiiImageFormat::ASTC_12x12_UNORM:
    case xiiImageFormat::ASTC_12x12_UNORM_SRGB:
      return (right == xiiImageFormat::ASTC_12x12_UNORM || right == xiiImageFormat::ASTC_12x12_UNORM_SRGB);
    default:
      XII_ASSERT_DEV(false, "Encountered unhandled format: {0}", xiiImageFormat::GetName(left));
      return false;
  }
}


bool xiiImageFormat::RequiresFirstLevelBlockAlignment(Enum format)
{
  return GetImageFormatMetaData(format).m_requireFirstLevelBlockAligned;
}

const char* xiiImageFormat::GetName(Enum format)
{
  return GetImageFormatMetaData(format).m_szName;
}

xiiUInt32 xiiImageFormat::GetPlaneCount(Enum format)
{
  return GetImageFormatMetaData(format).m_planeData.GetCount();
}

xiiUInt32 xiiImageFormat::GetChannelMask(Enum format, xiiImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[c];
}

xiiUInt32 xiiImageFormat::GetBitsPerChannel(Enum format, xiiImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiBitsPerChannel[c];
}

xiiUInt32 xiiImageFormat::GetRedMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[xiiImageFormatChannel::R];
}

xiiUInt32 xiiImageFormat::GetGreenMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[xiiImageFormatChannel::G];
}

xiiUInt32 xiiImageFormat::GetBlueMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[xiiImageFormatChannel::B];
}

xiiUInt32 xiiImageFormat::GetAlphaMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[xiiImageFormatChannel::A];
}

xiiUInt32 xiiImageFormat::GetBlockWidth(Enum format, xiiUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockWidth;
}

xiiUInt32 xiiImageFormat::GetBlockHeight(Enum format, xiiUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockHeight;
}

xiiUInt32 xiiImageFormat::GetBlockDepth(Enum format, xiiUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockDepth;
}

xiiImageFormatDataType::Enum xiiImageFormat::GetDataType(Enum format)
{
  return GetImageFormatMetaData(format).m_dataType;
}

bool xiiImageFormat::IsCompressed(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType == xiiImageFormatType::BLOCK_COMPRESSED;
}

bool xiiImageFormat::IsDepth(Enum format)
{
  return GetImageFormatMetaData(format).m_isDepth;
}

bool xiiImageFormat::IsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear != format;
}

bool xiiImageFormat::IsStencil(Enum format)
{
  return GetImageFormatMetaData(format).m_isStencil;
}

xiiImageFormat::Enum xiiImageFormat::AsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asSrgb;
}

xiiImageFormat::Enum xiiImageFormat::AsLinear(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear;
}

xiiUInt32 xiiImageFormat::GetNumBlocksX(Enum format, xiiUInt32 uiWidth, xiiUInt32 uiPlaneIndex)
{
  return (uiWidth - 1) / GetBlockWidth(format, uiPlaneIndex) + 1;
}

xiiUInt32 xiiImageFormat::GetNumBlocksY(Enum format, xiiUInt32 uiHeight, xiiUInt32 uiPlaneIndex)
{
  return (uiHeight - 1) / GetBlockHeight(format, uiPlaneIndex) + 1;
}

xiiUInt32 xiiImageFormat::GetNumBlocksZ(Enum format, xiiUInt32 uiDepth, xiiUInt32 uiPlaneIndex)
{
  return (uiDepth - 1) / GetBlockDepth(format, uiPlaneIndex) + 1;
}

xiiUInt64 xiiImageFormat::GetRowPitch(Enum format, xiiUInt32 uiWidth, xiiUInt32 uiPlaneIndex)
{
  return static_cast<xiiUInt64>(GetNumBlocksX(format, uiWidth, uiPlaneIndex)) * GetBitsPerBlock(format, uiPlaneIndex) / 8;
}

xiiUInt64 xiiImageFormat::GetDepthPitch(Enum format, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiPlaneIndex)
{
  return static_cast<xiiUInt64>(GetNumBlocksY(format, uiHeight, uiPlaneIndex)) * static_cast<xiiUInt64>(GetRowPitch(format, uiWidth, uiPlaneIndex));
}

xiiImageFormatType::Enum xiiImageFormat::GetType(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFormat);
