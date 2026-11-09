/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/StreamWithStats.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Compression.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Utilities/VirtualTextureData.h>

xiiResult xiiVirtualTextureTile::Serialize(xiiStreamWriter& stream) const
{
  stream << m_uiMip << m_uiX << m_uiY << m_uiOffset << m_uiStoredSize;
  return XII_SUCCESS;
}

xiiResult xiiVirtualTextureTile::Deserialize(xiiStreamReader& stream)
{
  stream >> m_uiMip >> m_uiX >> m_uiY >> m_uiOffset >> m_uiStoredSize;
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
  m_Description        = converted.GetDescription();
  m_Description.m_Type = xiiGALResourceDimension::Texture2D;
  m_sFile.Clear();
  m_uiFileOffset = m_uiFileHash = m_uiPayloadBytes = 0U;
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
        const auto                offset = m_Data.GetCount();
        xiiDynamicArray<xiiUInt8> tileData;
        tileData.SetCountUninitialized(s_uiTileBytes);
        for (xiiUInt32 py = 0U; py < s_uiPhysicalSize; ++py)
          for (xiiUInt32 px = 0U; px < s_uiPhysicalSize; ++px)
          {
            const auto  sx    = address(static_cast<xiiInt32>(x * s_uiTileSize + px) - s_uiBorder, width, m_uiAddressFlags & 3U);
            const auto  sy    = address(static_cast<xiiInt32>(y * s_uiTileSize + py) - s_uiBorder, height, (m_uiAddressFlags >> 2U) & 3U);
            const auto* pixel = converted.GetPixelPointer<xiiUInt8>(mip, 0U, 0U, sx, sy);
            xiiMemoryUtils::Copy(tileData.GetData() + (py * s_uiPhysicalSize + px) * 4U, pixel, 4U);
          }
        xiiDynamicArray<xiiUInt8> compressed;
        XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Compress(tileData, xiiCompressionMethod::ZStd, compressed));
        m_Tiles.PushBack({mip, x, y, offset, compressed.GetCount()});
        m_Data.PushBackRange(compressed);
      }
  }
  m_uiPayloadBytes = m_Data.GetCount();
  return XII_SUCCESS;
}

xiiResult xiiVirtualTextureData::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(2U);
  stream << m_Description << m_uiAddressFlags;
  if (m_sFile.IsEmpty())
  {
    XII_SUCCEED_OR_RETURN(stream.WriteArray(m_Tiles));
    return stream.WriteArray(m_Data);
  }
  auto                      tiles = m_Tiles;
  xiiDynamicArray<xiiUInt8> payload, decoded, encoded;
  for (xiiUInt32 tile = 0U; tile < tiles.GetCount(); ++tile)
  {
    XII_SUCCEED_OR_RETURN(ReadTile(tile, decoded));
    XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Compress(decoded, xiiCompressionMethod::ZStd, encoded));
    if (encoded.GetCount() > xiiMath::MaxValue<xiiUInt32>() - payload.GetCount())
      return XII_FAILURE;
    tiles[tile].m_uiOffset     = payload.GetCount();
    tiles[tile].m_uiStoredSize = encoded.GetCount();
    payload.PushBackRange(encoded);
  }
  XII_SUCCEED_OR_RETURN(stream.WriteArray(tiles));
  return stream.WriteArray(payload);
}

xiiResult xiiVirtualTextureData::Deserialize(xiiStreamReader& source, xiiStringView file, xiiUInt64 fileOffset)
{
  m_sFile.Clear();
  m_Data.Clear();
  xiiStreamReaderWithStats stream(&source);
  if (stream.ReadVersion(2U) != 2U)
    return XII_FAILURE;
  stream >> m_Description >> m_uiAddressFlags;
  if (m_Description.m_Type != xiiGALResourceDimension::Texture2D || m_Description.m_uiMipLevels == 0U || m_Description.m_uiMipLevels > 32U ||
      !m_Description.m_Size.HasNonZeroArea() || (m_Description.m_Format != xiiGALResourceFormat::RGBA8UNormalized && m_Description.m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB))
    return XII_FAILURE;
  // The last mip is the single tile guaranteed resident by the runtime.
  if ((xiiMath::Max(m_Description.m_Size.width, m_Description.m_Size.height) >> (m_Description.m_uiMipLevels - 1U)) != 1U)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Tiles));
  stream >> m_uiPayloadBytes;
  if (m_uiPayloadBytes > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;
  if (file.IsEmpty())
  {
    m_Data.SetCountUninitialized(static_cast<xiiUInt32>(m_uiPayloadBytes));
    if (stream.ReadBytes(m_Data.GetData(), m_uiPayloadBytes) != m_uiPayloadBytes)
      return XII_FAILURE;
  }
  else
  {
    m_sFile        = file;
    m_uiFileOffset = fileOffset + stream.m_uiBytesRead;
    if (stream.SkipBytes(m_uiPayloadBytes) != m_uiPayloadBytes)
      return XII_FAILURE;
  }
  xiiUInt32 expected = 0U;
  for (xiiUInt32 mip = 0U; mip < m_Description.m_uiMipLevels; ++mip)
  {
    const auto width  = xiiMath::Max(1U, m_Description.m_Size.width >> mip);
    const auto height = xiiMath::Max(1U, m_Description.m_Size.height >> mip);
    for (xiiUInt32 y = 0U; y <= (height - 1U) / s_uiTileSize; ++y)
      for (xiiUInt32 x = 0U; x <= (width - 1U) / s_uiTileSize; ++x)
      {
        if (expected >= m_Tiles.GetCount())
          return XII_FAILURE;
        const auto& tile = m_Tiles[expected++];
        if (tile.m_uiMip != mip || tile.m_uiX != x || tile.m_uiY != y || tile.m_uiOffset > m_uiPayloadBytes || tile.m_uiStoredSize == 0U || tile.m_uiStoredSize > m_uiPayloadBytes - tile.m_uiOffset)
          return XII_FAILURE;
      }
  }
  return expected == m_Tiles.GetCount() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiVirtualTextureData::ReadTile(xiiUInt32 index, xiiDynamicArray<xiiUInt8>& out_data) const
{
  if (index >= m_Tiles.GetCount())
    return XII_FAILURE;
  const auto& tile = m_Tiles[index];
  if (tile.m_uiStoredSize == 0U || tile.m_uiStoredSize > s_uiTileBytes + 1024U)
    return XII_FAILURE;
  xiiDynamicArray<xiiUInt8> encoded;
  if (m_sFile.IsEmpty())
  {
    if (tile.m_uiOffset > m_Data.GetCount() || tile.m_uiStoredSize > m_Data.GetCount() - tile.m_uiOffset)
      return XII_FAILURE;
    encoded = m_Data.GetArrayPtr().GetSubArray(tile.m_uiOffset, tile.m_uiStoredSize);
  }
  else
  {
    xiiFileReader file;
    XII_SUCCEED_OR_RETURN(file.Open(m_sFile));
    xiiStreamReaderWithStats stream(&file);
    xiiAssetFileHeader       header;
    XII_SUCCEED_OR_RETURN(header.Read(stream));
    if (!header.IsFileUpToDate(m_uiFileHash, m_uiAssetVersion) || m_uiFileOffset + tile.m_uiOffset < stream.m_uiBytesRead)
      return XII_FAILURE;
    const auto skip = m_uiFileOffset + tile.m_uiOffset - stream.m_uiBytesRead;
    if (stream.SkipBytes(skip) != skip)
      return XII_FAILURE;
    encoded.SetCountUninitialized(tile.m_uiStoredSize);
    if (stream.ReadBytes(encoded.GetData(), encoded.GetCount()) != encoded.GetCount())
      return XII_FAILURE;
  }
  XII_SUCCEED_OR_RETURN(xiiCompressionUtils::Decompress(encoded, xiiCompressionMethod::ZStd, s_uiTileBytes, out_data));
  return out_data.GetCount() == s_uiTileBytes ? XII_SUCCESS : XII_FAILURE;
}
