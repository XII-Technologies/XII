/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Decals/Implementation/DecalSerialization.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

xiiResult xiiDecalResourceDescriptor::Validate() const
{
  const auto unit = [](float value) {
    return xiiMath::IsFinite(value) && value >= 0 && value <= 1;
  };
  if ((m_hAtlas.IsValid() && m_sDecalId.IsEmpty()) || m_sDecalId.GetString().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
      !m_vUVOffset.IsValid() || !m_vUVScale.IsValid() || m_vUVScale.x == 0 || m_vUVScale.y == 0 || !m_Tint.IsValid() ||
      !unit(m_fOpacity) || !unit(m_fNormalBlend) || !unit(m_fRoughness) || !unit(m_fMetallic) ||
      !xiiMath::IsFinite(m_fEmissive) || m_fEmissive < 0 ||
      (m_ChannelMask.GetValue() & ~static_cast<xiiUInt16>(xiiDecalChannelMask::All)) != 0)
    return XII_FAILURE;
  if (m_hAtlas.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
      m_hAlbedo.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
      m_hNormal.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
      m_hMaterial.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes ||
      m_hEmissive.GetResourceID().GetElementCount() > xiiDecalSerialization::MaxStringBytes)
    return XII_FAILURE;
  return XII_SUCCESS;
}

xiiResult xiiDecalResourceDescriptor::Serialize(xiiStreamWriter& stream) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  // Keep the existing version-one field layout while propagating external write
  // failures. The memory writer cannot fail; checked readers reject truncation.
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  writer.WriteVersion(1);
  writer << m_sDecalId;
  writer << m_hAtlas;
  writer << m_hAlbedo;
  writer << m_hNormal;
  writer << m_hMaterial;
  writer << m_hEmissive;
  writer << m_vUVOffset;
  writer << m_vUVScale;
  writer << m_Tint;
  writer << m_ChannelMask;
  writer << m_fOpacity;
  writer << m_fNormalBlend;
  writer << m_fRoughness;
  writer << m_fMetallic;
  writer << m_fEmissive;
  writer << m_uiPriority;
  return storage.CopyToStream(stream);
}

xiiResult xiiDecalResourceDescriptor::Deserialize(xiiStreamReader& stream)
{
  xiiTypeVersion version = 0;
  if (stream.ReadWordValue(&version).Failed() || version != 1)
    return XII_FAILURE;
  xiiDecalResourceDescriptor descriptor;
  xiiStringBuilder           id;
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadString(stream, id));
  descriptor.m_sDecalId.Assign(id);
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, descriptor.m_hAtlas));
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, descriptor.m_hAlbedo));
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, descriptor.m_hNormal));
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, descriptor.m_hMaterial));
  XII_SUCCEED_OR_RETURN(xiiDecalSerialization::ReadResourceHandle(stream, descriptor.m_hEmissive));
  xiiUInt16 channelMask = 0;
  if (stream.ReadBytes(&descriptor.m_vUVOffset, sizeof(descriptor.m_vUVOffset)) != sizeof(descriptor.m_vUVOffset) ||
      stream.ReadBytes(&descriptor.m_vUVScale, sizeof(descriptor.m_vUVScale)) != sizeof(descriptor.m_vUVScale) ||
      stream.ReadBytes(&descriptor.m_Tint, sizeof(descriptor.m_Tint)) != sizeof(descriptor.m_Tint) ||
      stream.ReadWordValue(&channelMask).Failed() ||
      stream.ReadDWordValue(&descriptor.m_fOpacity).Failed() ||
      stream.ReadDWordValue(&descriptor.m_fNormalBlend).Failed() ||
      stream.ReadDWordValue(&descriptor.m_fRoughness).Failed() ||
      stream.ReadDWordValue(&descriptor.m_fMetallic).Failed() ||
      stream.ReadDWordValue(&descriptor.m_fEmissive).Failed() ||
      stream.ReadBytes(&descriptor.m_uiPriority, sizeof(descriptor.m_uiPriority)) != sizeof(descriptor.m_uiPriority))
    return XII_FAILURE;
  descriptor.m_ChannelMask.SetValue(channelMask);
  XII_SUCCEED_OR_RETURN(descriptor.Validate());
  *this = std::move(descriptor);
  return XII_SUCCESS;
}
