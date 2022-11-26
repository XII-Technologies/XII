#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCollectionResource, 1, xiiRTTIDefaultAllocator<xiiCollectionResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiCollectionResource);

xiiCollectionResource::xiiCollectionResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

void xiiCollectionResource::PreloadResources()
{
  XII_LOCK(m_PreloadMutex);
  XII_PROFILE_SCOPE("Inject Resources to Preload");

  if (!m_PreloadedResources.IsEmpty())
  {
    // PreloadResources has already been called so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  for (const auto& e : m_Collection.m_Resources)
  {
    xiiTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const xiiRTTI* pRtti = xiiResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = xiiResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        xiiLog::Error("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register "
                      "the resource type with the xiiResourceManager?",
                      e.m_sAssetTypeName, xiiArgSensitive(e.m_sResourceID, "ResourceID"));
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
}

bool xiiCollectionResource::IsLoadingFinished(float* out_progress) const
{
  XII_LOCK(m_PreloadMutex);

  XII_ASSERT_DEBUG(m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount(), "Collection size mismatch. PreloadResources not called?");

  xiiUInt64 loadedWeight = 0;
  xiiUInt64 totalWeight  = 0;

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

    if (state != xiiResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
  }

  if (out_progress != nullptr)
  {
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_progress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight);
    }
    else
    {
      *out_progress = 1.f;
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

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDesc xiiCollectionResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  {
    UnregisterNames();
    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    //XII_LOCK(m_preloadMutex);
    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

xiiResourceLoadDesc xiiCollectionResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiCollectionResource::UpdateContent", GetResourceDescription().GetData());

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
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

void xiiCollectionResourceDescriptor::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8  uiVersion      = 3;
  const xiiUInt8  uiIdentifier   = 0xC0;
  const xiiUInt32 uiNumResources = m_Resources.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumResources;

  for (xiiUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream << m_Resources[i].m_sAssetTypeName;
    stream << m_Resources[i].m_sOptionalNiceLookupName;
    stream << m_Resources[i].m_sResourceID;
    stream << m_Resources[i].m_uiFileSize;
  }
}

void xiiCollectionResourceDescriptor::Load(xiiStreamReader& stream)
{
  xiiUInt8  uiVersion      = 0;
  xiiUInt8  uiIdentifier   = 0;
  xiiUInt32 uiNumResources = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    xiiUInt16 uiNumResourcesShort;
    stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    stream >> uiNumResources;
  }

  XII_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid xiiCollectionResourceDescriptor");
  XII_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (xiiUInt32 i = 0; i < uiNumResources; ++i)
  {
    stream >> m_Resources[i].m_sAssetTypeName;
    stream >> m_Resources[i].m_sOptionalNiceLookupName;
    stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);
