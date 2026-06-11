/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Converter/TextureConverterProcessor.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/Utilities/TextureAtlasDescription.h>
#include <Texture/Utilities/TexturePacker.h>

xiiResult xiiTextureConverterProcessor::GenerateTextureAtlas(xiiMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != xiiTextureConverterOutputType::Atlas)
    return XII_SUCCESS;

  if (m_Descriptor.m_sTextureAtlasDescFile.IsEmpty())
  {
    xiiLog::Error("Texture atlas description file is not specified.");
    return XII_FAILURE;
  }

  xiiTextureAtlasCreationDescription       atlasDesc;
  xiiDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sTextureAtlasDescFile).Failed())
  {
    xiiLog::Error("Failed to load texture atlas description '{0}'", xiiArgSensitive(m_Descriptor.m_sTextureAtlasDescFile, "File"));
    return XII_FAILURE;
  }

  m_Descriptor.m_uiMinResolution = xiiMath::Max(32u, m_Descriptor.m_uiMinResolution);

  XII_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const xiiUInt8 uiVersion = 3;
  stream << uiVersion;

  xiiDdsFileFormat ddsWriter;
  xiiImage         atlasImg;

  for (xiiUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    XII_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg));

    if (ddsWriter.WriteImage(stream, atlasImg, "dds").Failed())
    {
      xiiLog::Error("Failed to write DDS image to texture atlas file.");
      return XII_FAILURE;
    }

    // debug: write out atlas slices as pure DDS
    if (false)
    {
      xiiStringBuilder sOut;
      sOut.SetFormat("D:/atlas_{}.dds", layerIdx);

      xiiFileWriter fOut;
      if (fOut.Open(sOut).Succeeded())
      {
        XII_SUCCEED_OR_RETURN(ddsWriter.WriteImage(fOut, atlasImg, "dds"));
      }
    }
  }

  XII_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, atlasDesc.m_Layers.GetCount(), stream));

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::LoadAtlasInputs(const xiiTextureAtlasCreationDescription& atlasDesc, xiiDynamicArray<TextureAtlasItem>& items) const
{
  items.Clear();

  for (const auto& srcItem : atlasDesc.m_Items)
  {
    auto& item        = items.ExpandAndGetRef();
    item.m_uiUniqueID = srcItem.m_uiUniqueID;
    item.m_uiFlags    = srcItem.m_uiFlags;

    for (xiiUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          xiiLog::Error("Failed to load texture atlas texture '{0}'", xiiArgSensitive(srcItem.m_sLayerInput[layer], "File"));
          return XII_FAILURE;
        }

        if (atlasDesc.m_Layers[layer].m_Usage == xiiTextureConverterUsage::Color)
        {
          // enforce sRGB format for all color textures
          item.m_InputImage[layer].ReinterpretAs(xiiImageFormat::AsSrgb(item.m_InputImage[layer].GetImageFormat()));
        }

        xiiUInt32 uiResX = 0, uiResY = 0;
        XII_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], xiiImageFormat::UNKNOWN, uiResX, uiResY));

        XII_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, atlasDesc.m_Layers[layer].m_Usage));
      }
    }


    if (!srcItem.m_sAlphaInput.IsEmpty())
    {
      xiiImage alphaImg;

      if (alphaImg.LoadFrom(srcItem.m_sAlphaInput).Failed())
      {
        xiiLog::Error("Failed to load texture atlas alpha mask '{0}'", srcItem.m_sAlphaInput);
        return XII_FAILURE;
      }

      xiiUInt32 uiResX = 0, uiResY = 0;
      XII_SUCCEED_OR_RETURN(DetermineTargetResolution(alphaImg, xiiImageFormat::UNKNOWN, uiResX, uiResY));

      XII_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sAlphaInput, alphaImg, uiResX, uiResY, xiiTextureConverterUsage::Linear));

      // layer 0 must have the exact same size as the alpha texture
      XII_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[0], item.m_InputImage[0], uiResX, uiResY, xiiTextureConverterUsage::Linear));

      // copy alpha channel into layer 0
      XII_SUCCEED_OR_RETURN(xiiImageUtils::CopyChannel(item.m_InputImage[0], 3, alphaImg, 0));

      // rescale all layers to be no larger than the alpha mask texture
      for (xiiUInt32 layer = 1; layer < atlasDesc.m_Layers.GetCount(); ++layer)
      {
        if (item.m_InputImage[layer].GetWidth() <= uiResX && item.m_InputImage[layer].GetHeight() <= uiResY)
          continue;

        XII_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, xiiTextureConverterUsage::Linear));
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::WriteTextureAtlasInfo(const xiiDynamicArray<TextureAtlasItem>& atlasItems, xiiUInt32 uiNumLayers, xiiStreamWriter& stream)
{
  xiiTextureAtlasRuntimeDescription runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e     = runtimeAtlas.m_Items[item.m_uiUniqueID];
    e.m_uiFlags = item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

constexpr xiiUInt32 uiAtlasCellSize = 32;

xiiResult xiiTextureConverterProcessor::TrySortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiInt32 layer)
{
  xiiTexturePacker packer;

  // TODO: review, currently the texture packer only works on 32 sized cells
  xiiUInt32 uiPixelAlign = uiAtlasCellSize;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign, (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  XII_SUCCEED_OR_RETURN(packer.PackTextures());

  xiiUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x      = tex.m_Position.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].y      = tex.m_Position.y * uiAtlasCellSize;
      item.m_AtlasRect[layer].width  = tex.m_Size.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].height = tex.m_Size.y * uiAtlasCellSize;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::SortItemsIntoAtlas(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32& out_ResX, xiiUInt32& out_ResY, xiiInt32 layer)
{
  for (xiiUInt32 power = 8; power < 14; ++power)
  {
    const xiiUInt32 halfRes            = 1 << (power - 1);
    const xiiUInt32 resolution         = 1 << power;
    const xiiUInt32 resDivCellSize     = resolution / uiAtlasCellSize;
    const xiiUInt32 halfResDivCellSize = halfRes / uiAtlasCellSize;

    if (TrySortItemsIntoAtlas(items, resDivCellSize, halfResDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return XII_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return XII_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return XII_SUCCESS;
    }
  }

  xiiLog::Error("Could not sort items into texture atlas. Too many too large textures.");
  return XII_FAILURE;
}

xiiResult xiiTextureConverterProcessor::CreateAtlasTexture(xiiDynamicArray<TextureAtlasItem>& items, xiiUInt32 uiResX, xiiUInt32 uiResY, xiiImage& atlas, xiiInt32 layer)
{
  xiiImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(xiiImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<xiiUInt8>();
    xiiMemoryUtils::ZeroFill(pixelData.GetPtr(), static_cast<size_t>(pixelData.GetCount()));
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      xiiImage& itemImage = item.m_InputImage[layer];

      xiiRectU32 r;
      r.x      = 0;
      r.y      = 0;
      r.width  = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      XII_SUCCEED_OR_RETURN(xiiImageUtils::Copy(itemImage, r, atlas, xiiVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0)));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::FillAtlasBorders(xiiDynamicArray<TextureAtlasItem>& items, xiiImage& atlas, xiiInt32 layer)
{
  const xiiUInt32 uiBorderPixels = 2;

  const xiiUInt32 uiNumMipmaps = atlas.GetHeader().GetNumMipLevels();
  for (xiiUInt32 uiMipLevel = 0; uiMipLevel < uiNumMipmaps; ++uiMipLevel)
  {
    for (auto& item : items)
    {
      if (!item.m_InputImage[layer].IsValid())
        continue;

      xiiRectU32&     itemRect = item.m_AtlasRect[layer];
      const xiiUInt32 uiRectX  = itemRect.x >> uiMipLevel;
      const xiiUInt32 uiRectY  = itemRect.y >> uiMipLevel;
      const xiiUInt32 uiWidth  = xiiMath::Max(1u, itemRect.width >> uiMipLevel);
      const xiiUInt32 uiHeight = xiiMath::Max(1u, itemRect.height >> uiMipLevel);

      // fill the border of the item rect with alpha 0 to prevent bleeding into other decals in the atlas
      if (uiWidth <= 2 * uiBorderPixels || uiHeight <= 2 * uiBorderPixels)
      {
        for (xiiUInt32 y = 0; y < uiHeight; ++y)
        {
          for (xiiUInt32 x = 0; x < uiWidth; ++x)
          {
            const xiiUInt32 xClamped                                                 = xiiMath::Min(uiRectX + x, atlas.GetWidth(uiMipLevel));
            const xiiUInt32 yClamped                                                 = xiiMath::Min(uiRectY + y, atlas.GetHeight(uiMipLevel));
            atlas.GetPixelPointer<xiiColor>(uiMipLevel, 0, 0, xClamped, yClamped)->a = 0.0f;
          }
        }
      }
      else
      {
        for (xiiUInt32 i = 0; i < uiBorderPixels; ++i)
        {
          for (xiiUInt32 y = 0; y < uiHeight; ++y)
          {
            atlas.GetPixelPointer<xiiColor>(uiMipLevel, 0, 0, uiRectX + i, uiRectY + y)->a               = 0.0f;
            atlas.GetPixelPointer<xiiColor>(uiMipLevel, 0, 0, uiRectX + uiWidth - 1 - i, uiRectY + y)->a = 0.0f;
          }

          for (xiiUInt32 x = 0; x < uiWidth; ++x)
          {
            atlas.GetPixelPointer<xiiColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + i)->a                = 0.0f;
            atlas.GetPixelPointer<xiiColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + uiHeight - 1 - i)->a = 0.0f;
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::CreateAtlasLayerTexture(const xiiTextureAtlasCreationDescription& atlasDesc, xiiDynamicArray<TextureAtlasItem>& atlasItems, xiiInt32 layer, xiiImage& dstImg)
{
  xiiUInt32 uiTexWidth, uiTexHeight;
  XII_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer));

  xiiLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  xiiImage atlasImg;
  XII_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  xiiUInt32 uiNumMipmaps = atlasImg.GetHeader().ComputeNumberOfMipMaps();
  XII_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  if (atlasDesc.m_Layers[layer].m_uiNumChannels == 4)
  {
    XII_SUCCEED_OR_RETURN(FillAtlasBorders(atlasItems, atlasImg, layer));
  }

  xiiEnum<xiiImageFormat> OutputImageFormat;

  XII_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  XII_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return XII_SUCCESS;
}
