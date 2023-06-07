#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalResource.h>

static xiiDecalResourceLoader s_DecalResourceLoader;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::SetResourceTypeLoader<xiiDecalResource>(&s_DecalResourceLoader);

    xiiDecalResourceDescriptor desc;
    xiiDecalResourceHandle hFallback = xiiResourceManager::CreateResource<xiiDecalResource>("Fallback Decal", std::move(desc), "Empty Decal for loading and missing decals");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiDecalResource>(hFallback);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiDecalResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeLoader<xiiDecalResource>(nullptr);

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiDecalResource>(xiiDecalResourceHandle());
    xiiResourceManager::SetResourceTypeMissingFallback<xiiDecalResource>(xiiDecalResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalResource, 1, xiiRTTIDefaultAllocator<xiiDecalResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDecalResource);
// clang-format on

xiiDecalResource::xiiDecalResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiResourceLoadDesc xiiDecalResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiDecalResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

void xiiDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiDecalResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDecalResource, xiiDecalResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;
  ret.m_State                      = xiiResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

xiiResourceLoadData xiiDecalResourceLoader::OpenDataStream(const xiiResource* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  xiiResourceLoadData res;
  return res;
}

void xiiDecalResourceLoader::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData)
{
  // nothing to do
}

bool xiiDecalResourceLoader::IsResourceOutdated(const xiiResource* pResource) const
{
  // decals are never outdated
  return false;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalResource);
