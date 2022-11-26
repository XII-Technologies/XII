#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

struct xiiTextureAtlasCreationDesc;

class XII_TEXTURE_DLL xiiTexConvProcessor
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTexConvProcessor);

public:
  xiiTexConvProcessor();

  xiiTexConvDesc m_Descriptor;

  xiiResult Process();

  xiiImage                      m_OutputImage;
  xiiImage                      m_LowResOutputImage;
  xiiImage                      m_ThumbnailOutputImage;
  xiiDefaultMemoryStreamStorage m_TextureAtlas;

private:
  //////////////////////////////////////////////////////////////////////////
  // Modifying the Descriptor

  xiiResult LoadInputImages();
  xiiResult ForceSRGBFormats();
  xiiResult ConvertAndScaleInputImages(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTexConvUsage> usage);
  xiiResult ConvertToNormalMap(xiiImage& bumpMap) const;
  xiiResult ConvertToNormalMap(xiiArrayPtr<xiiImage> bumpMap) const;
  xiiResult ClampInputValues(xiiArrayPtr<xiiImage> images, float maxValue) const;
  xiiResult ClampInputValues(xiiImage& image, float maxValue) const;
  xiiResult DetectNumChannels(xiiArrayPtr<const xiiTexConvSliceChannelMapping> channelMapping, xiiUInt32& uiNumChannels);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  enum class MipmapChannelMode
  {
    AllChannels,
    SingleChannel
  };

  xiiResult ChooseOutputFormat(xiiEnum<xiiImageFormat>& out_Format, xiiEnum<xiiTexConvUsage> usage, xiiUInt32 uiNumChannels) const;
  xiiResult DetermineTargetResolution(
    const xiiImage&         image,
    xiiEnum<xiiImageFormat> OutputImageFormat,
    xiiUInt32&              out_uiTargetResolutionX,
    xiiUInt32&              out_uiTargetResolutionY) const;
  xiiResult Assemble2DTexture(const xiiImageHeader& refImg, xiiImage& dst) const;
  xiiResult AssembleCubemap(xiiImage& dst) const;
  xiiResult Assemble3DTexture(xiiImage& dst) const;
  xiiResult AdjustHdrExposure(xiiImage& img) const;
  xiiResult PremultiplyAlpha(xiiImage& image) const;
  xiiResult DilateColor2D(xiiImage& img) const;
  xiiResult Assemble2DSlice(const xiiTexConvSliceChannelMapping& mapping, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiColor* pPixelOut) const;
  xiiResult GenerateMipmaps(xiiImage& img, xiiUInt32 uiNumMips /* =0 */, MipmapChannelMode channelMode = MipmapChannelMode::AllChannels) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional
  static xiiResult AdjustUsage(const char* szFilename, const xiiImage& srcImg, xiiEnum<xiiTexConvUsage>& inout_Usage);
  static xiiResult ConvertAndScaleImage(const char* szImageName, xiiImage& inout_Image, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTexConvUsage> usage);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static xiiResult GenerateOutput(xiiImage&& src, xiiImage& dst, xiiEnum<xiiImageFormat> format);
  static xiiResult GenerateThumbnailOutput(const xiiImage& srcImg, xiiImage& dstImg, xiiUInt32 uiTargetRes);
  static xiiResult GenerateLowResOutput(const xiiImage& srcImg, xiiImage& dstImg, xiiUInt32 uiLowResMip);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  struct TextureAtlasItem
  {
    xiiUInt32  m_uiUniqueID = 0;
    xiiUInt32  m_uiFlags    = 0;
    xiiImage   m_InputImage[4];
    xiiRectU32 m_AtlasRect[4];
  };

  xiiResult LoadAtlasInputs(const xiiTextureAtlasCreationDesc& atlasDesc, xiiDynamicArray<TextureAtlasItem>& items) const;
  xiiResult CreateAtlasLayerTexture(
    const xiiTextureAtlasCreationDesc& atlasDesc,
    xiiDynamicArray<TextureAtlasItem>& atlasItems,
    xiiInt32                           layer,
    xiiImage&                          dstImg);

  static xiiResult WriteTextureAtlasInfo(const xiiDynamicArray<TextureAtlasItem>& atlasItems, xiiUInt32 uiNumLayers, xiiStreamWriter& stream);
  static xiiResult TrySortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiInt32 layer);
  static xiiResult SortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32& out_ResX, xiiUInt32& out_ResY, xiiInt32 layer);
  static xiiResult CreateAtlasTexture(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiResX, xiiUInt32 uiResY, xiiImage& atlas, xiiInt32 layer);
  static xiiResult FillAtlasBorders(xiiDynamicArray<TextureAtlasItem>& items, xiiImage& atlas, xiiInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  xiiResult GenerateTextureAtlas(xiiMemoryStreamWriter& stream);
};
