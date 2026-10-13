/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Decals/DecalResource.h>

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
  if (descriptor.Validate().Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  m_Descriptor = std::move(descriptor);

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

  xiiDecalResourceDescriptor descriptor;
  if (pStream == nullptr || descriptor.Deserialize(*pStream).Failed())
  {
    UnloadData(Unload::AllQualityLevels);
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  return CreateResource(std::move(descriptor));
}

void xiiDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Decals_Implementation_DecalResource);
