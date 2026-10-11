/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAtlasResource, 2, xiiRTTIDefaultAllocator<xiiDecalAtlasResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDecalAtlasResource);

xiiDecalAtlasResource::xiiDecalAtlasResource() : xiiResource(DoUpdate::OnAnyThread, 0U) {}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor)
{
  UnloadData(Unload::AllQualityLevels);
  if (descriptor.Validate().Failed())
  {
    xiiLog::Error("Decal atlas '{}' requires four valid prepacked channel images and unique, valid entries.", GetResourceIdOrDescription());
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  m_Descriptor = std::move(descriptor);
  if (CreateGPUAtlases().Failed())
  {
    UnloadData(Unload::AllQualityLevels);
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  RebuildLookup();
  // GPU textures own the uploaded data. Keep only metadata in the resource.
  for (auto& image : m_Descriptor.m_LayerImages)
    image.Clear();
  return {xiiResourceState::Loaded, 0, 0};
}

bool xiiDecalAtlasResource::TryGetAtlasEntry(const xiiTempHashedString& id, const xiiDecalAtlasEntry*& out_pEntry) const
{
  xiiUInt32 index;
  if (!m_IdToEntryIndex.TryGetValue(id, index))
  {
    out_pEntry = nullptr;
    return false;
  }
  out_pEntry = &m_Descriptor.m_Entries[index];
  return true;
}

bool xiiDecalAtlasResource::TryGetAtlasEntry(const xiiHashedString& id, const xiiDecalAtlasEntry*& out_pEntry) const
{
  return TryGetAtlasEntry(xiiTempHashedString(id), out_pEntry);
}

xiiResourceLoadDescription xiiDecalAtlasResource::UnloadData(Unload)
{
  m_Descriptor = {};
  m_IdToEntryIndex.Clear();
  m_pAlbedoAtlasTexture.Clear();
  m_pNormalAtlasTexture.Clear();
  m_pMaterialAtlasTexture.Clear();
  m_pEmissiveAtlasTexture.Clear();
  m_pAtlasSampler.Clear();
  m_uiMemoryGPU = 0;
  return {xiiResourceState::Unloaded, 0, 0};
}

xiiResourceLoadDescription xiiDecalAtlasResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiDecalAtlasResourceDescriptor descriptor;
  if (pStream == nullptr || descriptor.Deserialize(*pStream).Failed())
  {
    UnloadData(Unload::AllQualityLevels);
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  return CreateResource(std::move(descriptor));
}

void xiiDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_usage)
{
  out_usage.m_uiMemoryCPU = sizeof(*this) + static_cast<xiiUInt32>(m_Descriptor.m_Entries.GetHeapMemoryUsage() + m_IdToEntryIndex.GetHeapMemoryUsage());
  out_usage.m_uiMemoryGPU = m_uiMemoryGPU;
}

xiiResult xiiDecalAtlasResource::CreateGPUAtlases()
{
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (device == nullptr)
    return XII_FAILURE;
  xiiSharedPtr<xiiGALTexture>  textures[4];
  static constexpr const char* names[] = {"Albedo", "Normal", "Material", "Emissive"};
  xiiUInt32                    memory  = 0;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(textures); ++i)
  {
    const auto&                                      image = m_Descriptor.m_LayerImages[i];
    xiiTexture2DResourceDescriptor                   description;
    xiiHybridArray<xiiGALTextureSubResourceData, 32> initialData;
    xiiUInt32                                        layerMemory;
    xiiTexture2DResource::FillOutDescriptor(description, &image, image.GetMipLevelCount(), layerMemory, initialData);
    xiiGALTextureData data(initialData);
    textures[i] = device->CreateTexture(description.m_TextureDescription, &data);
    if (textures[i] == nullptr)
      return XII_FAILURE;
    xiiStringBuilder name(GetResourceIdOrDescription(), "::", names[i]);
    textures[i]->SetDebugName(name);
    memory += layerMemory;
  }
  auto description       = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
  description.m_AddressU = xiiGALTextureAddressMode::Clamp;
  description.m_AddressV = xiiGALTextureAddressMode::Clamp;
  description.m_AddressW = xiiGALTextureAddressMode::Clamp;
  auto sampler           = device->CreateSampler(description);
  if (sampler == nullptr)
    return XII_FAILURE;
  m_pAlbedoAtlasTexture   = std::move(textures[0]);
  m_pNormalAtlasTexture   = std::move(textures[1]);
  m_pMaterialAtlasTexture = std::move(textures[2]);
  m_pEmissiveAtlasTexture = std::move(textures[3]);
  m_pAtlasSampler         = std::move(sampler);
  m_uiMemoryGPU           = memory;
  return XII_SUCCESS;
}

void xiiDecalAtlasResource::RebuildLookup()
{
  m_IdToEntryIndex.Clear();
  m_IdToEntryIndex.Reserve(m_Descriptor.m_Entries.GetCount());
  for (xiiUInt32 i = 0; i < m_Descriptor.m_Entries.GetCount(); ++i)
    m_IdToEntryIndex.Insert(m_Descriptor.m_Entries[i].m_sDecalId, i);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Decals_Implementation_DecalAtlasResource);
