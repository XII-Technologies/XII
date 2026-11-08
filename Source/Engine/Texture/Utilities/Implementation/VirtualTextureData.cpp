/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Texture/TexturePCH.h>

#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Utilities/VirtualTextureData.h>

xiiResult xiiVirtualTextureTile::Serialize(xiiStreamWriter& stream) const
{
  stream << m_uiMip << m_uiX << m_uiY << m_uiOffset;
  return XII_SUCCESS;
}

xiiResult xiiVirtualTextureTile::Deserialize(xiiStreamReader& stream)
{
  stream >> m_uiMip >> m_uiX >> m_uiY >> m_uiOffset;
  return XII_SUCCESS;
}

xiiResult xiiVirtualTextureData::Build(const xiiImageView& image, const xiiGALSamplerCreationDescription& sampler)
{
  if (!image.IsValid() || image.GetNumFaces() != 1U || image.GetNumArrayIndices() != 1U || image.GetDepth() != 1U)
    return XII_FAILURE;
  xiiImage   converted;
  const auto format = xiiGALResourceFormat::IsSrgb(image.GetImageFormat()) ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : xiiGALResourceFormat::RGBA8UNormalized;
  XII_SUCCEED_OR_RETURN(xiiImageConversion::Convert(image, converted, format));
  const auto mode = [](xiiGALTextureAddressMode::Enum value) {
    return value == xiiGALTextureAddressMode::Clamp ? 1U : value == xiiGALTextureAddressMode::Mirror ? 2U :
                                                                                                       0U;
  };
  m_uiAddressFlags   = mode(sampler.m_AddressU) | (mode(sampler.m_AddressV) << 2U);
  const auto address = [](xiiInt32 value, xiiUInt32 size, xiiUInt32 mode) {
    if (mode == 1U)
      return static_cast<xiiUInt32>(xiiMath::Clamp(value, 0, static_cast<xiiInt32>(size) - 1));
    const auto period  = static_cast<xiiInt32>(mode == 2U ? size * 2U : size);
    const auto wrapped = (value % period + period) % period;
    return static_cast<xiiUInt32>(mode == 2U && wrapped >= static_cast<xiiInt32>(size) ? period - wrapped - 1 : wrapped);
  };
  m_Description = converted.GetDescription();
  m_Tiles.Clear();
  m_Data.Clear();
  for (xiiUInt32 mip = 0U; mip < converted.GetMipLevelCount(); ++mip)
  {
    const auto width  = converted.GetWidth(mip);
    const auto height = converted.GetHeight(mip);
    for (xiiUInt32 y = 0U; y < (height + s_uiTileSize - 1U) / s_uiTileSize; ++y)
      for (xiiUInt32 x = 0U; x < (width + s_uiTileSize - 1U) / s_uiTileSize; ++x)
      {
        if (m_Data.GetCount() > xiiMath::MaxValue<xiiUInt32>() - s_uiTileBytes)
          return XII_FAILURE;
        const auto offset = m_Data.GetCount();
        m_Tiles.PushBack({mip, x, y, offset});
        m_Data.SetCountUninitialized(offset + s_uiTileBytes);
        for (xiiUInt32 py = 0U; py < s_uiPhysicalSize; ++py)
          for (xiiUInt32 px = 0U; px < s_uiPhysicalSize; ++px)
          {
            const auto  sx    = address(static_cast<xiiInt32>(x * s_uiTileSize + px) - s_uiBorder, width, m_uiAddressFlags & 3U);
            const auto  sy    = address(static_cast<xiiInt32>(y * s_uiTileSize + py) - s_uiBorder, height, (m_uiAddressFlags >> 2U) & 3U);
            const auto* pixel = converted.GetPixelPointer<xiiUInt8>(mip, 0U, 0U, sx, sy);
            xiiMemoryUtils::Copy(m_Data.GetData() + offset + (py * s_uiPhysicalSize + px) * 4U, pixel, 4U);
          }
      }
  }
  return XII_SUCCESS;
}

xiiResult xiiVirtualTextureData::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1U);
  stream << m_Description << m_uiAddressFlags;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_Tiles));
  return stream.WriteArray(m_Data);
}

xiiResult xiiVirtualTextureData::Deserialize(xiiStreamReader& stream)
{
  stream.ReadVersion(1U);
  stream >> m_Description >> m_uiAddressFlags;
  if (m_Description.m_Type != xiiGALResourceDimension::Texture2D || m_Description.m_uiMipLevels == 0U || m_Description.m_uiMipLevels > 32U ||
      !m_Description.m_Size.HasNonZeroArea() || (m_Description.m_Format != xiiGALResourceFormat::RGBA8UNormalized && m_Description.m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Tiles));
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Data));
  xiiUInt32 expected = 0U;
  for (xiiUInt32 mip = 0U; mip < m_Description.m_uiMipLevels; ++mip)
  {
    const auto width  = xiiMath::Max(1U, m_Description.m_Size.width >> mip);
    const auto height = xiiMath::Max(1U, m_Description.m_Size.height >> mip);
    for (xiiUInt32 y = 0U; y < (height + 127U) / 128U; ++y)
      for (xiiUInt32 x = 0U; x < (width + 127U) / 128U; ++x)
      {
        if (expected >= m_Tiles.GetCount())
          return XII_FAILURE;
        const auto& tile = m_Tiles[expected++];
        if (tile.m_uiMip != mip || tile.m_uiX != x || tile.m_uiY != y || tile.m_uiOffset > m_Data.GetCount() || s_uiTileBytes > m_Data.GetCount() - tile.m_uiOffset)
          return XII_FAILURE;
      }
  }
  return expected == m_Tiles.GetCount() ? XII_SUCCESS : XII_FAILURE;
}
