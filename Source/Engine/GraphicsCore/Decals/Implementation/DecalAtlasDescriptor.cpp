/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Decals/Implementation/DecalSerialization.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

xiiResult xiiDecalAtlasResourceDescriptor::Validate() const
{
  if (!m_AtlasSize.HasNonZeroArea() || m_AtlasSize.width > 16384 || m_AtlasSize.height > 16384 || m_Entries.GetCount() > 65535)
    return XII_FAILURE;
  const auto mipCount     = m_LayerImages[0].GetMipLevelCount();
  xiiUInt32  expectedMips = 1;
  if (m_bGenerateMipMaps)
    for (xiiUInt32 size = xiiMath::Max(m_AtlasSize.width, m_AtlasSize.height); size > 1; size >>= 1)
      ++expectedMips;
  if (mipCount != expectedMips)
    return XII_FAILURE;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_LayerImages); ++i)
  {
    const auto& image = m_LayerImages[i];
    if (!image.IsValid() || image.GetWidth() != m_AtlasSize.width || image.GetHeight() != m_AtlasSize.height ||
        image.GetDepth() != 1 || image.GetNumFaces() != 1 || image.GetNumArrayIndices() != 1 ||
        image.GetMipLevelCount() != mipCount || image.GetByteBlobPtr().GetCount() > 512U * 1024U * 1024U)
      return XII_FAILURE;
    // Enforce the channel interpretation expected by the built-in decal shader.
    if (i == 0 ? !xiiGALResourceFormat::IsSrgb(image.GetImageFormat()) : xiiGALResourceFormat::IsSrgb(image.GetImageFormat()))
      return XII_FAILURE;
    const auto& format = xiiGALTextureUtilities::GetResourceFormatProperties(image.GetImageFormat());
    if (xiiGALResourceFormat::IsTypeless(image.GetImageFormat()) || xiiGALResourceFormat::IsMultiplanar(image.GetImageFormat()) ||
        format.m_ComponentType == xiiGALResourceFormatComponentType::Undefined ||
        format.m_ComponentType == xiiGALResourceFormatComponentType::Depth || format.m_ComponentType == xiiGALResourceFormatComponentType::DepthStencil ||
        format.m_ComponentType == xiiGALResourceFormatComponentType::SignedInteger || format.m_ComponentType == xiiGALResourceFormatComponentType::UnsignedInteger)
      return XII_FAILURE;
    // Texture2DResource rounds compressed uploads to complete blocks. Reject a
    // changed logical extent, which would shift every normalized entry rectangle.
    if (format.IsCompressed() && (m_AtlasSize.width % format.m_uiBlockWidth != 0 || m_AtlasSize.height % format.m_uiBlockHeight != 0))
      return XII_FAILURE;
  }
  xiiHashSet<xiiHashedString> names;
  for (const auto& entry : m_Entries)
  {
    const auto& uv = entry.m_vUVRect;
    if (entry.m_sDecalId.IsEmpty() || entry.m_sDecalId.GetString().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
        entry.m_hAlbedo.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
        entry.m_hNormal.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
        entry.m_hMaterial.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
        entry.m_hEmissive.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes || names.Insert(entry.m_sDecalId) || !entry.m_Size.HasNonZeroArea() ||
        !uv.IsValid() || uv.x < 0 || uv.y < 0 || uv.z <= 0 || uv.w <= 0 || uv.x + uv.z > 1 || uv.y + uv.w > 1 ||
        !entry.m_vTextureMetrics.IsValid() || entry.m_vTextureMetrics.x <= 0 || entry.m_vTextureMetrics.y <= 0 ||
        entry.m_vTextureMetrics.z <= 0 || entry.m_vTextureMetrics.w <= 0 ||
        (entry.m_ChannelMask.GetValue() & ~static_cast<xiiUInt16>(xiiDecalChannelMask::All)) != 0)
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiDecalAtlasResourceDescriptor::Serialize(xiiStreamWriter& stream) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  // Build in memory so every external write failure propagates through CopyToStream.
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  writer.WriteVersion(2);
  writer << m_AtlasSize;
  writer << m_uiPadding;
  writer << m_bGenerateMipMaps;
  writer << m_Entries.GetCount();
  for (const auto& entry : m_Entries)
  {
    writer << entry.m_sDecalId;
    writer << entry.m_hAlbedo;
    writer << entry.m_hNormal;
    writer << entry.m_hMaterial;
    writer << entry.m_hEmissive;
    writer << entry.m_Size;
    writer << entry.m_uiPadding;
    writer << entry.m_uiPriority;
    writer << entry.m_ChannelMask;
    writer << entry.m_vUVRect;
    writer << entry.m_vTextureMetrics;
  }
  xiiDdsFileFormat dds;
  for (const auto& image : m_LayerImages)
  {
    xiiDefaultMemoryStreamStorage imageStorage;
    xiiMemoryStreamWriter         imageWriter(&imageStorage);
    XII_SUCCEED_OR_RETURN(dds.WriteImage(imageWriter, image, "dds"));
    writer << imageStorage.GetStorageSize32();
    XII_SUCCEED_OR_RETURN(imageStorage.CopyToStream(writer));
  }
  writer << static_cast<xiiUInt32>(0xDECADA7A);
  return storage.CopyToStream(stream);
}

xiiResult xiiDecalAtlasResourceDescriptor::Deserialize(xiiStreamReader& stream)
{
  xiiTypeVersion version = 0;
  if (stream.ReadWordValue(&version).Failed() || version != 2)
    return XII_FAILURE;
  xiiDecalAtlasResourceDescriptor descriptor;
  xiiUInt8                        mipMaps = 0;
  if (stream.ReadBytes(&descriptor.m_AtlasSize, sizeof(descriptor.m_AtlasSize)) != sizeof(descriptor.m_AtlasSize) ||
      stream.ReadBytes(&descriptor.m_uiPadding, sizeof(descriptor.m_uiPadding)) != sizeof(descriptor.m_uiPadding) ||
      stream.ReadBytes(&mipMaps, sizeof(mipMaps)) != sizeof(mipMaps) || mipMaps > 1)
    return XII_FAILURE;
  descriptor.m_bGenerateMipMaps = mipMaps != 0;
  xiiUInt32 count               = 0;
  if (stream.ReadDWordValue(&count).Failed() || count > 65535 || !descriptor.m_AtlasSize.HasNonZeroArea() ||
      descriptor.m_AtlasSize.width > 16384 || descriptor.m_AtlasSize.height > 16384)
    return XII_FAILURE;
  descriptor.m_Entries.SetCount(count);
  for (auto& entry : descriptor.m_Entries)
  {
    xiiStringBuilder id;
    XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadString(stream, id));
    entry.m_sDecalId.Assign(id);
    XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, entry.m_hAlbedo));
    XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, entry.m_hNormal));
    XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, entry.m_hMaterial));
    XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, entry.m_hEmissive));
    xiiUInt16 channelMask = 0;
    if (stream.ReadBytes(&entry.m_Size, sizeof(entry.m_Size)) != sizeof(entry.m_Size) ||
        stream.ReadBytes(&entry.m_uiPadding, sizeof(entry.m_uiPadding)) != sizeof(entry.m_uiPadding) ||
        stream.ReadBytes(&entry.m_uiPriority, sizeof(entry.m_uiPriority)) != sizeof(entry.m_uiPriority) ||
        stream.ReadWordValue(&channelMask).Failed() ||
        stream.ReadBytes(&entry.m_vUVRect, sizeof(entry.m_vUVRect)) != sizeof(entry.m_vUVRect) ||
        stream.ReadBytes(&entry.m_vTextureMetrics, sizeof(entry.m_vTextureMetrics)) != sizeof(entry.m_vTextureMetrics))
      return XII_FAILURE;
    entry.m_ChannelMask.SetValue(channelMask);
  }
  xiiDdsFileFormat dds;
  for (auto& image : descriptor.m_LayerImages)
  {
    xiiUInt32 size = 0;
    if (stream.ReadDWordValue(&size).Failed() || size < 128 || size > 512U * 1024U * 1024U)
      return XII_FAILURE;
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(size);
    if (stream.ReadBytes(bytes.GetData(), size) != size)
      return XII_FAILURE;
    xiiRawMemoryStreamReader         reader(bytes.GetData(), size);
    xiiGALTextureCreationDescription description;
    XII_SUCCEED_OR_RETURN(dds.ReadImageDescription(reader, description, "dds"));
    if (description.m_Type != xiiGALResourceDimension::Texture2D || description.m_uiArraySizeOrDepth != 1 ||
        description.m_Size.width != descriptor.m_AtlasSize.width || description.m_Size.height != descriptor.m_AtlasSize.height ||
        description.m_uiMipLevels > 15 || description.m_uiMipLevels == 0)
      return XII_FAILURE;
    // Check the complete byte budget before DDS allocates storage from its header.
    // A small or truncated file must not cause a much larger image allocation.
    xiiUInt64 imageBytes = 0;
    for (xiiUInt32 mip = 0; mip < description.m_uiMipLevels; ++mip)
      imageBytes += xiiGALTextureUtilities::GetMipLevelProperties(description, mip).m_uiMipSize;
    if (imageBytes == 0 || imageBytes != size - reader.GetReadPosition())
      return XII_FAILURE;
    reader.SetReadPosition(0);
    XII_SUCCEED_OR_RETURN(dds.ReadImage(reader, image, "dds"));
    if (reader.GetReadPosition() != size)
      return XII_FAILURE;
  }
  xiiUInt32 marker = 0;
  if (stream.ReadDWordValue(&marker).Failed() || marker != 0xDECADA7A || descriptor.Validate().Failed())
    return XII_FAILURE;
  *this = std::move(descriptor);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Decals_Implementation_DecalAtlasDescriptor);
