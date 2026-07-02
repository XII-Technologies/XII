/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/Converter/TextureConverterDescription.h>

struct xiiTextureAtlasCreationDescription;

class XII_TEXTURE_DLL xiiTextureConverterProcessor
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTextureConverterProcessor);

public:
  xiiTextureConverterProcessor();

  xiiTextureConverterDescription m_Descriptor;

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
  xiiResult ConvertAndScaleInputImages(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTextureConverterUsage> usage);
  xiiResult ConvertToNormalMap(xiiImage& bumpMap) const;
  xiiResult ConvertToNormalMap(xiiArrayPtr<xiiImage> bumpMap) const;
  xiiResult ClampInputValues(xiiArrayPtr<xiiImage> images, float maxValue) const;
  xiiResult ClampInputValues(xiiImage& image, float maxValue) const;
  xiiResult DetectNumChannels(xiiArrayPtr<const xiiTextureConverterSliceChannelMapping> channelMapping, xiiUInt32& uiNumChannels);
  xiiResult InvertNormalMap(xiiImage& img);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  enum class MipmapChannelMode
  {
    AllChannels,
    SingleChannel
  };

  xiiResult ChooseOutputFormat(xiiEnum<xiiGALResourceFormat>& out_Format, xiiEnum<xiiTextureConverterUsage> usage, xiiUInt32 uiNumChannels) const;
  xiiResult DetermineTargetResolution(const xiiImage& image, xiiEnum<xiiGALResourceFormat> OutputImageFormat, xiiUInt32& out_uiTargetResolutionX, xiiUInt32& out_uiTargetResolutionY) const;
  xiiResult Assemble2DTexture(const xiiGALTextureCreationDescription& refImg, xiiImage& dst) const;
  xiiResult AssembleCubemap(xiiImage& dst) const;
  xiiResult Assemble3DTexture(xiiImage& dst) const;
  xiiResult AdjustHdrExposure(xiiImage& img) const;
  xiiResult PremultiplyAlpha(xiiImage& image) const;
  xiiResult DilateColor2D(xiiImage& img) const;
  xiiResult Assemble2DSlice(const xiiTextureConverterSliceChannelMapping& mapping, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiColor* pPixelOut) const;
  xiiResult GenerateMipmaps(xiiImage& img, xiiUInt32 uiNumMips /* =0 */, MipmapChannelMode channelMode = MipmapChannelMode::AllChannels) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional
  static xiiResult AdjustUsage(xiiStringView sFilename, const xiiImage& srcImg, xiiEnum<xiiTextureConverterUsage>& inout_Usage);
  static xiiResult ConvertAndScaleImage(xiiStringView sImageName, xiiImage& inout_Image, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiEnum<xiiTextureConverterUsage> usage);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static xiiResult GenerateOutput(xiiImage&& src, xiiImage& dst, xiiEnum<xiiGALResourceFormat> format);
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

  xiiResult LoadAtlasInputs(const xiiTextureAtlasCreationDescription& atlasDesc, xiiDynamicArray<TextureAtlasItem>& items) const;
  xiiResult CreateAtlasLayerTexture(const xiiTextureAtlasCreationDescription& atlasDesc, xiiDynamicArray<TextureAtlasItem>& atlasItems, xiiInt32 layer, xiiImage& dstImg);

  static xiiResult WriteTextureAtlasInfo(const xiiDynamicArray<TextureAtlasItem>& atlasItems, xiiUInt32 uiNumLayers, xiiStreamWriter& stream);
  static xiiResult TrySortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiInt32 layer);
  static xiiResult SortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32& out_ResX, xiiUInt32& out_ResY, xiiInt32 layer);
  static xiiResult CreateAtlasTexture(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiResX, xiiUInt32 uiResY, xiiImage& atlas, xiiInt32 layer);
  static xiiResult FillAtlasBorders(xiiDynamicArray<TextureAtlasItem>& items, xiiImage& atlas, xiiInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  xiiResult GenerateTextureAtlas(xiiMemoryStreamWriter& stream);
};
