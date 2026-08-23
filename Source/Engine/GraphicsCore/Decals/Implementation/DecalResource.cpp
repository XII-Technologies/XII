/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Device/Device.h>

namespace
{
  static xiiUInt16 ResolveTextureWidth(const xiiTexture2DResourceHandle& hTexture, xiiUInt16 uiFallback)
  {
    if (!hTexture.IsValid())
      return xiiMath::Max<xiiUInt16>(uiFallback, static_cast<xiiUInt16>(1U));

    xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!pTexture)
      return xiiMath::Max<xiiUInt16>(uiFallback, static_cast<xiiUInt16>(1U));

    return static_cast<xiiUInt16>(xiiMath::Clamp<xiiUInt32>(pTexture->GetWidth(), 1U, xiiMath::MaxValue<xiiUInt16>()));
  }

  static xiiUInt16 ResolveTextureHeight(const xiiTexture2DResourceHandle& hTexture, xiiUInt16 uiFallback)
  {
    if (!hTexture.IsValid())
      return xiiMath::Max<xiiUInt16>(uiFallback, static_cast<xiiUInt16>(1U));

    xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!pTexture)
      return xiiMath::Max<xiiUInt16>(uiFallback, static_cast<xiiUInt16>(1U));

    return static_cast<xiiUInt16>(xiiMath::Clamp<xiiUInt32>(pTexture->GetHeight(), 1U, xiiMath::MaxValue<xiiUInt16>()));
  }

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAtlasResource, 1, xiiRTTIDefaultAllocator<xiiDecalAtlasResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDecalAtlasResource);

xiiDecalAtlasResource::xiiDecalAtlasResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor)
{
  m_Descriptor = descriptor;

  PackAtlas();
  RebuildLookup();
  CreateGPUAtlases();

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Loaded;
  return description;
}

bool xiiDecalAtlasResource::TryGetAtlasEntry(const xiiTempHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const
{
  out_pEntry = nullptr;

  xiiUInt32 uiEntryIndex = xiiInvalidIndex;
  if (!m_IdToEntryIndex.TryGetValue(sDecalId, uiEntryIndex) || uiEntryIndex >= m_Descriptor.m_Entries.GetCount())
    return false;

  out_pEntry = &m_Descriptor.m_Entries[uiEntryIndex];
  return true;
}

bool xiiDecalAtlasResource::TryGetAtlasEntry(const xiiHashedString& sDecalId, const xiiDecalAtlasEntry*& out_pEntry) const
{
  return TryGetAtlasEntry(xiiTempHashedString(sDecalId), out_pEntry);
}

xiiResourceLoadDescription xiiDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  m_Descriptor.m_Entries.Clear();
  m_IdToEntryIndex.Clear();
  m_pAlbedoAtlasTexture.Clear();
  m_pNormalAtlasTexture.Clear();
  m_pMaterialAtlasTexture.Clear();
  m_pEmissiveAtlasTexture.Clear();
  m_pAtlasSampler.Clear();
  m_uiMemoryGPU = 0U;

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Unloaded;
  return description;
}

xiiResourceLoadDescription xiiDecalAtlasResource::UpdateContent(xiiStreamReader* pStream)
{
  XII_LOG_BLOCK("xiiDecalAtlasResource::UpdateContent", GetResourceIdOrDescription());

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

  PackAtlas();
  RebuildLookup();
  CreateGPUAtlases();

  xiiResourceLoadDescription description;
  description.m_uiQualityLevelsDiscardable = 0U;
  description.m_uiQualityLevelsLoadable    = 0U;
  description.m_State                      = xiiResourceState::Loaded;
  return description;
}

void xiiDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this) + static_cast<xiiUInt32>(m_Descriptor.m_Entries.GetHeapMemoryUsage()) + static_cast<xiiUInt32>(m_IdToEntryIndex.GetHeapMemoryUsage());
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU;
}

void xiiDecalAtlasResource::PackAtlas()
{
  const xiiUInt32 uiAtlasWidth  = xiiMath::Max<xiiUInt32>(m_Descriptor.m_AtlasSize.width, 1U);
  const xiiUInt32 uiAtlasHeight = xiiMath::Max<xiiUInt32>(m_Descriptor.m_AtlasSize.height, 1U);
  const xiiUInt32 uiPadding     = m_Descriptor.m_uiPadding;

  xiiUInt32 uiCursorX   = uiPadding;
  xiiUInt32 uiCursorY   = uiPadding;
  xiiUInt32 uiRowHeight = 0U;

  for (xiiDecalAtlasEntry& entry : m_Descriptor.m_Entries)
  {
    const xiiTexture2DResourceHandle hSizeTexture = entry.m_hAlbedo.IsValid() ? entry.m_hAlbedo : (entry.m_hNormal.IsValid() ? entry.m_hNormal : entry.m_hMaterial);

    xiiUInt32 uiWidth  = ResolveTextureWidth(hSizeTexture, entry.m_Size.width);
    xiiUInt32 uiHeight = ResolveTextureHeight(hSizeTexture, entry.m_Size.height);

    const xiiUInt32 uiEntryPadding = xiiMath::Max<xiiUInt32>(entry.m_uiPadding, uiPadding);
    uiWidth                        = xiiMath::Min(uiWidth, xiiMath::Max<xiiUInt32>(1U, uiAtlasWidth - xiiMath::Min(uiAtlasWidth, uiEntryPadding * 2U)));
    uiHeight                       = xiiMath::Min(uiHeight, xiiMath::Max<xiiUInt32>(1U, uiAtlasHeight - xiiMath::Min(uiAtlasHeight, uiEntryPadding * 2U)));

    if (uiCursorX + uiWidth + uiEntryPadding > uiAtlasWidth)
    {
      uiCursorX = uiPadding;
      uiCursorY += uiRowHeight + uiPadding;
      uiRowHeight = 0U;
    }

    if (uiCursorY + uiHeight + uiEntryPadding > uiAtlasHeight)
    {
      entry.m_vUVRect         = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);
      entry.m_vTextureMetrics = xiiVec4(1.0f / static_cast<float>(uiAtlasWidth), 1.0f / static_cast<float>(uiAtlasHeight), 1.0f, 1.0f);
      continue;
    }

    entry.m_Size.width      = static_cast<xiiUInt16>(uiWidth);
    entry.m_Size.height     = static_cast<xiiUInt16>(uiHeight);
    entry.m_vUVRect         = xiiVec4(static_cast<float>(uiCursorX) / static_cast<float>(uiAtlasWidth), static_cast<float>(uiCursorY) / static_cast<float>(uiAtlasHeight), static_cast<float>(uiWidth) / static_cast<float>(uiAtlasWidth), static_cast<float>(uiHeight) / static_cast<float>(uiAtlasHeight));
    entry.m_vTextureMetrics = xiiVec4(1.0f / static_cast<float>(uiWidth), 1.0f / static_cast<float>(uiHeight), static_cast<float>(uiWidth), static_cast<float>(uiHeight));

    uiCursorX += uiWidth + uiEntryPadding;
    uiRowHeight = xiiMath::Max(uiRowHeight, uiHeight);
  }
}

void xiiDecalAtlasResource::CreateGPUAtlases()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return;

  const xiiUInt32 uiAtlasWidth  = xiiMath::Max<xiiUInt32>(m_Descriptor.m_AtlasSize.width, 1U);
  const xiiUInt32 uiAtlasHeight = xiiMath::Max<xiiUInt32>(m_Descriptor.m_AtlasSize.height, 1U);

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  textureDescription.m_Size.width  = uiAtlasWidth;
  textureDescription.m_Size.height = uiAtlasHeight;
  textureDescription.m_uiMipLevels = 1U;
  textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage       = xiiGALResourceUsage::Mutable;

  auto CreateNeutralAtlas = [&](xiiUInt32 uiClearValue, xiiStringView sSuffix) -> xiiSharedPtr<xiiGALTexture> {
    xiiTemporaryArray<xiiUInt32> neutralPixels;
    neutralPixels.SetCount(uiAtlasWidth * uiAtlasHeight);
    for (xiiUInt32& uiPixel : neutralPixels)
    {
      uiPixel = uiClearValue;
    }

    xiiTemporaryHybridArray<xiiGALTextureSubResourceData, 1U> initData;
    xiiGALTextureSubResourceData&                             subResourceData = initData.ExpandAndGetRef();
    subResourceData.m_pData                                                   = neutralPixels.GetByteArrayPtr();
    subResourceData.m_uiStride                                                = uiAtlasWidth * sizeof(xiiUInt32);
    subResourceData.m_uiDepthStride                                           = uiAtlasWidth * uiAtlasHeight * sizeof(xiiUInt32);

    xiiGALTextureData           textureData(initData);
    xiiSharedPtr<xiiGALTexture> pTexture = pDevice->CreateTexture(textureDescription, &textureData);
    if (pTexture)
    {
      xiiStringBuilder sDebugName = GetResourceDescription();
      sDebugName.Append("::", sSuffix);
      pTexture->SetDebugName(sDebugName);
    }

    return pTexture;
  };

  // Albedo = white, normal = neutral tangent-space normal, material = roughness 0.5 / metallic 0 / ao 1, emissive = black.
  m_pAlbedoAtlasTexture   = CreateNeutralAtlas(0xFFFFFFFFU, "Albedo");
  m_pNormalAtlasTexture   = CreateNeutralAtlas(0xFFFF8080U, "Normal");
  m_pMaterialAtlasTexture = CreateNeutralAtlas(0x00FF0080U, "Material");
  m_pEmissiveAtlasTexture = CreateNeutralAtlas(0x00000000U, "Emissive");

  xiiGALSamplerCreationDescription samplerDescription = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
  samplerDescription.m_AddressU                       = xiiGALTextureAddressMode::Clamp;
  samplerDescription.m_AddressV                       = xiiGALTextureAddressMode::Clamp;
  samplerDescription.m_AddressW                       = xiiGALTextureAddressMode::Clamp;
  m_pAtlasSampler                                     = pDevice->CreateSampler(samplerDescription);

  m_uiMemoryGPU = uiAtlasWidth * uiAtlasHeight * sizeof(xiiUInt32) * 4U;
}

void xiiDecalAtlasResource::RebuildLookup()
{
  m_IdToEntryIndex.Clear();
  m_IdToEntryIndex.Reserve(m_Descriptor.m_Entries.GetCount());

  for (xiiUInt32 i = 0; i < m_Descriptor.m_Entries.GetCount(); ++i)
  {
    const xiiHashedString& sDecalId = m_Descriptor.m_Entries[i].m_sDecalId;

    if (!sDecalId.IsEmpty())
    {
      m_IdToEntryIndex.Insert(sDecalId, i);
    }
  }
}

void xiiDecalAtlasResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1U);

  ref_stream << m_AtlasSize;
  ref_stream << m_uiPadding;
  ref_stream << m_bGenerateMipMaps;

  ref_stream << m_Entries.GetCount();
  for (const xiiDecalAtlasEntry& entry : m_Entries)
  {
    ref_stream << entry.m_sDecalId;
    ref_stream << entry.m_hAlbedo;
    ref_stream << entry.m_hNormal;
    ref_stream << entry.m_hMaterial;
    ref_stream << entry.m_hEmissive;
    ref_stream << entry.m_Size;
    ref_stream << entry.m_uiPadding;
    ref_stream << entry.m_uiPriority;
    ref_stream << entry.m_ChannelMask;
  }
}

void xiiDecalAtlasResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  ref_stream.ReadVersion(1U);

  ref_stream >> m_AtlasSize;
  ref_stream >> m_uiPadding;
  ref_stream >> m_bGenerateMipMaps;

  xiiUInt32 uiEntryCount = 0U;
  ref_stream >> uiEntryCount;
  m_Entries.SetCount(uiEntryCount);

  for (xiiDecalAtlasEntry& entry : m_Entries)
  {
    ref_stream >> entry.m_sDecalId;
    ref_stream >> entry.m_hAlbedo;
    ref_stream >> entry.m_hNormal;
    ref_stream >> entry.m_hMaterial;
    ref_stream >> entry.m_hEmissive;
    ref_stream >> entry.m_Size;
    ref_stream >> entry.m_uiPadding;
    ref_stream >> entry.m_uiPriority;
    ref_stream >> entry.m_ChannelMask;
  }
}

//////////////////////////////////////////////////////////////////////////

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
