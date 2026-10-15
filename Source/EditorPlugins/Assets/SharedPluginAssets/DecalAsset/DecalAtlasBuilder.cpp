/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <SharedPluginAssets/DecalAsset/DecalAtlasBuilder.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/Utilities/TexturePacker.h>

namespace
{
  xiiStatus ReadDescription(xiiStringView path, const xiiDecalAtlasBuildSettings& settings, xiiGALTextureCreationDescription& description)
  {
    if (path.IsEmpty() || xiiImageFileFormat::ReadImageDescription(path, description).Failed())
      return xiiStatus(xiiFmt("Cannot read decal source image '{}'.", path));
    if (description.m_Type != xiiGALResourceDimension::Texture2D || description.m_uiArraySizeOrDepth != 1 || !description.m_Size.HasNonZeroArea() || description.m_Size.width > settings.m_uiMaxSourceSize || description.m_Size.height > settings.m_uiMaxSourceSize)
      return xiiStatus(xiiFmt("Decal source '{}' must be a 2D image no larger than {} pixels per axis.", path, settings.m_uiMaxSourceSize));
    xiiUInt32 maxMips = 1;
    for (auto axis = xiiMath::Max(description.m_Size.width, description.m_Size.height); axis > 1; axis >>= 1)
      ++maxMips;
    if (description.m_uiMipLevels == 0 || description.m_uiMipLevels > maxMips)
      return xiiStatus(xiiFmt("Decal source '{}' declares an invalid mip count.", path));
    return XII_SUCCESS;
  }

  void AllocateFloatImage(xiiImage& image, xiiUInt32 width, xiiUInt32 height, const xiiColor& value)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type   = xiiGALResourceDimension::Texture2D;
    description.m_Size   = {width, height};
    description.m_Format = xiiGALResourceFormat::RGBA32Float;
    image.ResetAndAlloc(description);
    for (auto& pixel : image.GetBlobPtr<xiiColor>())
      pixel = value;
  }

  xiiStatus ReadScaledImage(xiiStringView path, bool color, xiiUInt32 width, xiiUInt32 height, const xiiDecalAtlasBuildSettings& settings, xiiImage& result, bool normal = false)
  {
    xiiGALTextureCreationDescription description;
    XII_SUCCEED_OR_RETURN(ReadDescription(path, settings, description));
    xiiImage image;
    if (image.LoadFrom(path).Failed())
      return xiiStatus(xiiFmt("Cannot load decal source '{}'.", path));
    // Recheck in case the file changed between metadata and pixel reads.
    if (image.GetWidth() != description.m_Size.width || image.GetHeight() != description.m_Size.height || image.GetDepth() != 1 || image.GetNumFaces() != 1 || image.GetNumArrayIndices() != 1)
      return xiiStatus(xiiFmt("Decal source '{}' changed while being transformed.", path));
    image.ReinterpretAs(color ? xiiGALResourceFormat::AsSrgb(image.GetImageFormat()) : xiiGALResourceFormat::AsLinear(image.GetImageFormat()));
    if (image.Convert(xiiGALResourceFormat::RGBA32Float).Failed())
      return xiiStatus(xiiFmt("Cannot convert decal source '{}' to linear pixels.", path));
    if (xiiImageUtils::Scale(image.GetSubImageView(0), result, width, height).Failed())
      return xiiStatus(xiiFmt("Cannot resize decal source '{}'.", path));
    for (const auto& pixel : result.GetBlobPtr<xiiColor>())
      if (!pixel.IsValid())
        return xiiStatus(xiiFmt("Decal source '{}' contains non-finite pixels.", path));
    if (normal && xiiGALTextureUtilities::GetComponentCount(description.m_Format) == 2)
      xiiImageUtils::ReconstructNormalZ(result);
    return XII_SUCCESS;
  }

  void NormalizeNormals(xiiImage& image)
  {
    for (auto& pixel : image.GetBlobPtr<xiiColor>())
    {
      xiiVec3 normal(pixel.r * 2 - 1, pixel.g * 2 - 1, pixel.b * 2 - 1);
      normal.NormalizeIfNotZero(xiiVec3(0, 0, 1)).IgnoreResult();
      pixel.r = normal.x * 0.5f + 0.5f;
      pixel.g = normal.y * 0.5f + 0.5f;
      pixel.b = normal.z * 0.5f + 0.5f;
    }
  }

  xiiStatus PrepareTile(const xiiDecalAtlasSource& source, xiiUInt32 layer, const xiiDecalAtlasEntry& entry, const xiiDecalAtlasBuildSettings& settings, const xiiColor& neutral, xiiImage& tile)
  {
    const xiiString* paths[] = {&source.m_sAlbedo, &source.m_sNormal, &source.m_sORM, &source.m_sEmissive};
    if (paths[layer]->IsEmpty())
      AllocateFloatImage(tile, entry.m_Size.width, entry.m_Size.height, neutral);
    else
      XII_SUCCEED_OR_RETURN(ReadScaledImage(*paths[layer], layer == 0 || layer == 3, entry.m_Size.width, entry.m_Size.height, settings, tile, layer == 1));
    if (layer == 0 && !source.m_sAlphaMask.IsEmpty())
    {
      xiiImage mask;
      XII_SUCCEED_OR_RETURN(ReadScaledImage(source.m_sAlphaMask, false, entry.m_Size.width, entry.m_Size.height, settings, mask));
      XII_SUCCEED_OR_RETURN(xiiImageUtils::CopyChannel(tile, 3, mask, 0));
    }
    if (layer == 1)
    {
      NormalizeNormals(tile);
    }
    else if (layer == 2 && !source.m_sORM.IsEmpty())
    {
      for (auto& pixel : tile.GetBlobPtr<xiiColor>())
      {
        const auto orm = pixel;
        pixel          = xiiColor(orm.g, orm.b, orm.r, orm.a);
      }
    }
    else if (layer == 3)
    {
      for (const auto& pixel : tile.GetBlobPtr<xiiColor>())
        if (xiiMath::Abs(pixel.r) > 65504 || xiiMath::Abs(pixel.g) > 65504 || xiiMath::Abs(pixel.b) > 65504 || xiiMath::Abs(pixel.a) > 65504)
          return xiiStatus(xiiFmt("Decal emissive image '{}' exceeds the finite half-float output range.", source.m_sEmissive));
    }
    return XII_SUCCESS;
  }
} // namespace

xiiStatus xiiDecalAtlasBuilder::Build(xiiArrayPtr<const xiiDecalAtlasSource> sources, const xiiDecalAtlasBuildSettings& settings, xiiDecalAtlasResourceDescriptor& output)
{
  if (sources.GetCount() > 65535 || settings.m_uiMaxEntrySize == 0 || settings.m_uiMaxEntrySize > settings.m_uiMaxSourceSize || settings.m_uiMaxSourceSize > 8192 || settings.m_uiMaxAtlasSize < 4 || settings.m_uiMaxAtlasSize > 4096 || !xiiMath::IsPowerOf2(settings.m_uiMaxAtlasSize) || settings.m_uiPadding == 0 || settings.m_uiPadding > 64)
    return xiiStatus("Invalid decal atlas packing limits.");
  xiiDynamicArray<xiiUInt32> order;
  for (xiiUInt32 i = 0; i < sources.GetCount(); ++i)
    order.PushBack(i);
  order.Sort([&](xiiUInt32 a, xiiUInt32 b) { return sources[a].m_sDecalId.GetString().Compare(sources[b].m_sDecalId.GetString()) < 0; });
  xiiDecalAtlasResourceDescriptor atlas;
  atlas.m_uiPadding        = settings.m_uiPadding;
  atlas.m_bGenerateMipMaps = settings.m_bGenerateMipMaps;
  for (xiiUInt32 i = 0; i < order.GetCount(); ++i)
  {
    const auto& source = sources[order[i]];
    if (source.m_sDecalId.IsEmpty() || (i > 0 && source.m_sDecalId == sources[order[i - 1]].m_sDecalId) || (source.m_ChannelMask.GetValue() & ~static_cast<xiiUInt16>(xiiDecalChannelMask::All)) != 0 || (source.m_ChannelMask.IsSet(xiiDecalChannelMask::Normal) && source.m_sNormal.IsEmpty()) || (source.m_ChannelMask.IsSet(xiiDecalChannelMask::Material) && source.m_sORM.IsEmpty()) || (source.m_ChannelMask.IsSet(xiiDecalChannelMask::Emissive) && source.m_sEmissive.IsEmpty()))
      return xiiStatus(xiiFmt("Decal '{}' has a missing channel source, invalid mask or duplicate identity.", source.m_sDecalId.GetString()));
    xiiGALTextureCreationDescription description;
    XII_SUCCEED_OR_RETURN(ReadDescription(source.m_sAlbedo, settings, description));
    const auto maxAxis  = xiiMath::Max(description.m_Size.width, description.m_Size.height);
    auto&      entry    = atlas.m_Entries.ExpandAndGetRef();
    entry.m_sDecalId    = source.m_sDecalId;
    entry.m_Size        = {static_cast<xiiUInt16>(xiiMath::Max(1U, description.m_Size.width * xiiMath::Min(maxAxis, settings.m_uiMaxEntrySize) / maxAxis)),
                           static_cast<xiiUInt16>(xiiMath::Max(1U, description.m_Size.height * xiiMath::Min(maxAxis, settings.m_uiMaxEntrySize) / maxAxis))};
    entry.m_uiPadding   = settings.m_uiPadding;
    entry.m_uiPriority  = source.m_uiPriority;
    entry.m_ChannelMask = source.m_ChannelMask;
  }
  xiiTexturePacker packer;
  xiiUInt32        extent = 4;
  for (;; extent *= 2)
  {
    if (extent > settings.m_uiMaxAtlasSize)
      return xiiStatus(xiiFmt("Decal images do not fit within the {} pixel project atlas. Reduce source sizes or increase the packing limit.", settings.m_uiMaxAtlasSize));
    packer.SetTextureSize(extent, extent, atlas.m_Entries.GetCount());
    for (const auto& entry : atlas.m_Entries)
      packer.AddTexture(entry.m_Size.width + 2U * settings.m_uiPadding, entry.m_Size.height + 2U * settings.m_uiPadding);
    if (packer.PackTextures().Succeeded())
      break;
  }
  atlas.m_AtlasSize = {static_cast<xiiUInt16>(extent), static_cast<xiiUInt16>(extent)};
  for (xiiUInt32 i = 0; i < atlas.m_Entries.GetCount(); ++i)
  {
    auto&       entry       = atlas.m_Entries[i];
    const auto& packed      = packer.GetTextures()[i];
    entry.m_vUVRect         = {float(packed.m_Position.x + settings.m_uiPadding) / extent, float(packed.m_Position.y + settings.m_uiPadding) / extent,
                               float(entry.m_Size.width) / extent, float(entry.m_Size.height) / extent};
    entry.m_vTextureMetrics = {1.0f / entry.m_Size.width, 1.0f / entry.m_Size.height, float(packed.m_Size.x), float(packed.m_Size.y)};
  }
  // Empty projects still have a valid neutral atlas. The material neutral is
  // already native R/M/AO; PrepareTile swizzles only authored ORM sources.
  const xiiColor neutral[] = {xiiColor(1, 1, 1, 0), xiiColor(0.5f, 0.5f, 1, 1), xiiColor(0.5f, 0, 1, 1), xiiColor(0, 0, 0, 1)};
  for (xiiUInt32 layer = 0; layer < 4; ++layer)
  {
    xiiImage image;
    AllocateFloatImage(image, extent, extent, neutral[layer]);
    for (xiiUInt32 i = 0; i < order.GetCount(); ++i)
    {
      const auto& entry  = atlas.m_Entries[i];
      const auto& packed = packer.GetTextures()[i];
      xiiImage    tile;
      XII_SUCCEED_OR_RETURN(PrepareTile(sources[order[i]], layer, entry, settings, neutral[layer], tile));
      // Extrude the source edge through the entire padding ring, including corners.
      for (xiiUInt32 y = 0; y < packed.m_Size.y; ++y)
        for (xiiUInt32 x = 0; x < packed.m_Size.x; ++x)
        {
          const auto sx                                                                               = xiiMath::Clamp<xiiInt32>(xiiInt32(x) - settings.m_uiPadding, 0, entry.m_Size.width - 1);
          const auto sy                                                                               = xiiMath::Clamp<xiiInt32>(xiiInt32(y) - settings.m_uiPadding, 0, entry.m_Size.height - 1);
          *image.GetPixelPointer<xiiColor>(0, 0, 0, packed.m_Position.x + x, packed.m_Position.y + y) = *tile.GetPixelPointer<xiiColor>(0, 0, 0, sx, sy);
        }
    }
    auto& result = atlas.m_LayerImages[layer];
    if (settings.m_bGenerateMipMaps)
    {
      xiiImageUtils::MipMapOptions options;
      xiiImageUtils::GenerateMipMaps(image, result, options);
    }
    else
      result = std::move(image);
    if (layer == 1)
      NormalizeNormals(result);
    if (result.Convert(layer == 0 ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : layer == 3 ? xiiGALResourceFormat::RGBA16Float :
                                                                                              xiiGALResourceFormat::RGBA8UNormalized)
          .Failed())
      return xiiStatus("Failed to encode a packed decal channel.");
  }
  XII_SUCCEED_OR_RETURN(atlas.Validate());
  output = std::move(atlas);
  return XII_SUCCESS;
}
