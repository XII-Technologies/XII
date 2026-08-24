/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/AssetFileHeader.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionResource, 1, xiiRTTIDefaultAllocator<xiiCollectionResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiCollectionResource);

xiiCollectionResource::xiiCollectionResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

// UnloadData() already makes sure to call UnregisterNames();
xiiCollectionResource::~xiiCollectionResource() = default;

bool xiiCollectionResource::PreloadResources(xiiUInt32 uiNumResourcesToPreload)
{
  XII_LOCK(m_PreloadMutex);
  XII_PROFILE_SCOPE("Inject Resources to Preload");

  if (m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount())
  {
    // All resources have already been queued so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return false;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  const xiiUInt32 remainingResources = m_Collection.m_Resources.GetCount() - m_PreloadedResources.GetCount();
  const xiiUInt32 end                = xiiMath::Min(remainingResources, uiNumResourcesToPreload) + m_PreloadedResources.GetCount();
  for (xiiUInt32 i = m_PreloadedResources.GetCount(); i < end; ++i)
  {
    const xiiCollectionEntry& e = m_Collection.m_Resources[i];
    xiiTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const xiiRTTI* pRtti = xiiResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = xiiResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        xiiLog::Warning("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register the resource type with the xiiResourceManager?", e.m_sAssetTypeName, xiiArgSensitive(e.m_sResourceID, "ResourceID"));
      }
    }
    else
    {
      xiiLog::Error("Asset '{}' had an empty asset type name. Cannot pre-load it.", xiiArgSensitive(e.m_sResourceID, "ResourceID"));
    }

    m_PreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      xiiResourceManager::PreloadResource(hTypeless);
    }
  }

  return m_PreloadedResources.GetCount() < m_Collection.m_Resources.GetCount();
}

bool xiiCollectionResource::IsLoadingFinished(float* out_pProgress) const
{
  XII_LOCK(m_PreloadMutex);

  xiiUInt64 loadedWeight = 0;
  xiiUInt64 totalWeight  = 0;

  xiiUInt32 uiPoked = 0;

  for (xiiUInt32 i = 0; i < m_PreloadedResources.GetCount(); i++)
  {
    const xiiTypelessResourceHandle& hResource = m_PreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const xiiCollectionEntry& entry      = m_Collection.m_Resources[i];
    xiiUInt64                 thisWeight = xiiMath::Max(entry.m_uiFileSize, 1ull); // if file sizes are not specified, we weight by 1
    xiiResourceState          state      = xiiResourceManager::GetLoadingState(hResource);

    if (state == xiiResourceState::Loaded || state == xiiResourceState::LoadedResourceMissing)
    {
      loadedWeight += thisWeight;
    }
    else if (state != xiiResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
    else
    {
      if (uiPoked < 3)
      {
        // there's a bug or race condition somewhere when unloading resources, which means resources that should be queued
        // for preloading don't get preloaded and then the entire preloading system gets stuck
        // to prevent this, we'll make sure that the next few unloaded resources do get requeued for preload

        ++uiPoked;
        xiiResourceManager::PreloadResource(hResource);
      }
    }
  }

  if (out_pProgress != nullptr)
  {
    const float maxLoadedFraction = m_Collection.m_Resources.GetCount() == 0 ? 1.f : (float)m_PreloadedResources.GetCount() / m_Collection.m_Resources.GetCount();
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_pProgress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight) * maxLoadedFraction;
    }
    else
    {
      *out_pProgress = maxLoadedFraction;
    }
  }

  if (totalWeight == 0 || totalWeight == loadedWeight)
  {
    return true;
  }

  return false;
}

const xiiCollectionResourceDescriptor& xiiCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiCollectionResource, xiiCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDescription xiiCollectionResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  {
    UnregisterNames();

    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    // XII_LOCK(m_preloadMutex);

    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

xiiResourceLoadDescription xiiCollectionResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiCollectionResource::UpdateContent", GetResourceIdOrDescription());

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // The standard file reader writes the absolute file path into the stream.
  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Skip the asset file header at the start of the file.
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Collection.Load(*Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  XII_LOCK(m_PreloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<xiiUInt32>(m_PreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
}

void xiiCollectionResource::RegisterNames()
{
  if (m_bRegistered)
    return;

  m_bRegistered = true;

  XII_LOCK(xiiResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      xiiResourceManager::RegisterNamedResource(entry.m_sOptionalNiceLookupName, entry.m_sResourceID);
    }
  }
}

void xiiCollectionResource::UnregisterNames()
{
  if (!m_bRegistered)
    return;

  m_bRegistered = false;

  XII_LOCK(xiiResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      xiiResourceManager::UnregisterNamedResource(entry.m_sOptionalNiceLookupName);
    }
  }
}

void xiiCollectionResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8  uiVersion      = 3;
  const xiiUInt8  uiIdentifier   = 0xC0;
  const xiiUInt32 uiNumResources = m_Resources.GetCount();

  ref_stream << uiVersion;
  ref_stream << uiIdentifier;
  ref_stream << uiNumResources;

  for (xiiUInt32 i = 0; i < uiNumResources; ++i)
  {
    ref_stream << m_Resources[i].m_sAssetTypeName;
    ref_stream << m_Resources[i].m_sOptionalNiceLookupName;
    ref_stream << m_Resources[i].m_sResourceID;
    ref_stream << m_Resources[i].m_uiFileSize;
  }
}

void xiiCollectionResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8  uiVersion      = 0;
  xiiUInt8  uiIdentifier   = 0;
  xiiUInt32 uiNumResources = 0;

  ref_stream >> uiVersion;
  ref_stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    xiiUInt16 uiNumResourcesShort;
    ref_stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    ref_stream >> uiNumResources;
  }

  XII_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid xiiCollectionResourceDescriptor");
  XII_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (xiiUInt32 i = 0; i < uiNumResources; ++i)
  {
    ref_stream >> m_Resources[i].m_sAssetTypeName;
    ref_stream >> m_Resources[i].m_sOptionalNiceLookupName;
    ref_stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      ref_stream >> m_Resources[i].m_uiFileSize;
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);
