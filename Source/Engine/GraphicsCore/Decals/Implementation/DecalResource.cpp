/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Device/Device.h>

namespace
{
  static void SkipResourceFileHeader(xiiStreamReader& ref_stream)
  {
    xiiStringBuilder sAbsoluteFilePath;
    ref_stream >> sAbsoluteFilePath;

    xiiAssetFileHeader assetHeader;
    assetHeader.Read(ref_stream).IgnoreResult();
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiDecalChannelMask, 1)
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::None),
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::Albedo),
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::Normal),
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::Material),
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::Emissive),
  XII_BITFLAGS_CONSTANT(xiiDecalChannelMask::All),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDecalProjectionMode, 1)
  XII_ENUM_CONSTANT(xiiDecalProjectionMode::Projected),
  XII_ENUM_CONSTANT(xiiDecalProjectionMode::Mesh),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalResource, 1, xiiRTTIDefaultAllocator<xiiDecalResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDecalResource);

xiiDecalResource::xiiDecalResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDecalResource, xiiDecalResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Loaded;
  return description;
}

xiiResourceLoadDescription xiiDecalResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  m_Descriptor = {};

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Unloaded;
  return description;
}

xiiResourceLoadDescription xiiDecalResource::UpdateContent(xiiStreamReader* pStream)
{
  XII_LOG_BLOCK("xiiDecalResource::UpdateContent", GetResourceIdOrDescription());

  if (pStream == nullptr)
  {
    xiiResourceLoadDescription description;
    description.m_uiQualityLevelsDiscardable = 0U;
    description.m_uiQualityLevelsLoadable    = 0U;
    description.m_State                      = xiiResourceState::LoadedResourceMissing;
    return description;
  }

  SkipResourceFileHeader(*pStream);
  m_Descriptor.Load(*pStream);

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Loaded;
  return description;
}

void xiiDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

void xiiDecalResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1U);

  ref_stream << m_sDecalId;
  ref_stream << m_hAtlas;
  ref_stream << m_hAlbedo;
  ref_stream << m_hNormal;
  ref_stream << m_hMaterial;
  ref_stream << m_hEmissive;
  ref_stream << m_vUVOffset;
  ref_stream << m_vUVScale;
  ref_stream << m_Tint;
  ref_stream << m_ChannelMask;
  ref_stream << m_fOpacity;
  ref_stream << m_fNormalBlend;
  ref_stream << m_fRoughness;
  ref_stream << m_fMetallic;
  ref_stream << m_fEmissive;
  ref_stream << m_uiPriority;
}

void xiiDecalResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  ref_stream.ReadVersion(1U);

  ref_stream >> m_sDecalId;
  ref_stream >> m_hAtlas;
  ref_stream >> m_hAlbedo;
  ref_stream >> m_hNormal;
  ref_stream >> m_hMaterial;
  ref_stream >> m_hEmissive;
  ref_stream >> m_vUVOffset;
  ref_stream >> m_vUVScale;
  ref_stream >> m_Tint;
  ref_stream >> m_ChannelMask;
  ref_stream >> m_fOpacity;
  ref_stream >> m_fNormalBlend;
  ref_stream >> m_fRoughness;
  ref_stream >> m_fMetallic;
  ref_stream >> m_fEmissive;
  ref_stream >> m_uiPriority;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Decals_Implementation_DecalResource);
