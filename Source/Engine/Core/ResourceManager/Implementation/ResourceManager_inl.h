/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Logging/Log.h>

template <typename ResourceType>
XII_FORCE_INLINE ResourceType* xiiResourceManager::GetResource(xiiStringView sResourceID, bool bIsReloadable)
{
  return static_cast<ResourceType*>(GetResource(xiiGetStaticRTTI<ResourceType>(), sResourceID, bIsReloadable));
}

template <typename ResourceType>
XII_FORCE_INLINE xiiTypedResourceHandle<ResourceType> xiiResourceManager::LoadResource(xiiStringView sResourceID)
{
  // The mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle.
  XII_LOCK(s_ResourceMutex);
  return xiiTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
}

template <typename ResourceType>
xiiTypedResourceHandle<ResourceType> xiiResourceManager::LoadResource(xiiStringView sResourceID, xiiTypedResourceHandle<ResourceType> hLoadingFallback)
{
  xiiTypedResourceHandle<ResourceType> hResource;
  {
    // The mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle.
    XII_LOCK(s_ResourceMutex);
    hResource = xiiTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
  }

  if (hLoadingFallback.IsValid())
  {
    hResource.m_pResource->SetLoadingFallbackResource(hLoadingFallback);
  }

  return hResource;
}

template <typename ResourceType>
xiiTypedResourceHandle<ResourceType> xiiResourceManager::GetExistingResource(xiiStringView sResourceID)
{
  xiiResource* pResource = nullptr;

  const xiiTempHashedString sResourceHash(sResourceID);

  XII_LOCK(s_ResourceMutex);

  const xiiRTTI* pRtti = FindResourceTypeOverride(xiiGetStaticRTTI<ResourceType>(), sResourceID);

  if (GetLoadedResources()[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return xiiTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return xiiTypedResourceHandle<ResourceType>();
}

template <typename ResourceType, typename DescriptorType>
xiiTypedResourceHandle<ResourceType> xiiResourceManager::CreateResource(xiiStringView sResourceID, DescriptorType&& descriptor, xiiStringView sResourceDescription)
{
  static_assert(std::is_rvalue_reference<DescriptorType&&>::value, "Please std::move the descriptor into this function");

  XII_LOG_BLOCK("xiiResourceManager::CreateResource", sResourceID);

  XII_LOCK(s_ResourceMutex);

  xiiTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(sResourceID, false));

  ResourceType* pResource = BeginAcquireResource(hResource, xiiResourceAcquireMode::PointerOnly);
  pResource->SetResourceDescription(sResourceDescription);
  pResource->m_Flags.Add(xiiResourceFlags::IsCreatedResource);

  XII_ASSERT_DEV(pResource->GetLoadingState() == xiiResourceState::Unloaded, "CreateResource was called on a resource that is already created");

  // If this does not compile, you either passed in the wrong descriptor type for the given resource type
  // or you forgot to std::move the descriptor when calling CreateResource
  {
    auto                       localDescriptor = std::move(descriptor);
    xiiResourceLoadDescription ld              = pResource->CreateResource(std::move(localDescriptor));
    pResource->VerifyAfterCreateResource(ld);
  }

  XII_ASSERT_DEV(pResource->GetLoadingState() != xiiResourceState::Unloaded, "CreateResource did not set the loading state properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType, typename DescriptorType>
xiiTypedResourceHandle<ResourceType>
xiiResourceManager::GetOrCreateResource(xiiStringView sResourceID, DescriptorType&& descriptor, xiiStringView sResourceDescription)
{
  XII_LOCK(s_ResourceMutex);
  xiiTypedResourceHandle<ResourceType> hResource = GetExistingResource<ResourceType>(sResourceID);
  if (!hResource.IsValid())
  {
    hResource = CreateResource<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription);
  }

  return hResource;
}

XII_FORCE_INLINE xiiResource* xiiResourceManager::BeginAcquireResourcePointer(const xiiRTTI* pType, const xiiTypelessResourceHandle& hResource)
{
  XII_IGNORE_UNUSED(pType);
  XII_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  xiiResource* pResource = (xiiResource*)hResource.m_pResource;

  XII_ASSERT_DEBUG(pResource->GetDynamicRTTI()->IsDerivedFrom(pType), "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(), pType->GetTypeName());

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
ResourceType* xiiResourceManager::BeginAcquireResource(const xiiTypedResourceHandle<ResourceType>& hResource, xiiResourceAcquireMode mode, const xiiTypedResourceHandle<ResourceType>& hFallbackResource, xiiResourceAcquireResult* out_pAcquireResult /*= nullptr*/)
{
  XII_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiResource* pCurrentlyUpdatingContent = xiiResource::GetCurrentlyUpdatingContent();
  if (pCurrentlyUpdatingContent != nullptr)
  {
    XII_LOCK(s_ResourceMutex);
    XII_ASSERT_DEV(mode == xiiResourceAcquireMode::PointerOnly || IsResourceTypeAcquireDuringUpdateContentAllowed(pCurrentlyUpdatingContent->GetDynamicRTTI(), xiiGetStaticRTTI<ResourceType>()),
                   "Trying to acquire a resource of type '{0}' during '{1}::UpdateContent()'. This has to be enabled by calling "
                   "xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<{1}, {0}>(); at engine startup, for example in "
                   "xiiGameApplication::Init_SetupDefaultResources().",
                   xiiGetStaticRTTI<ResourceType>()->GetTypeName(), pCurrentlyUpdatingContent->GetDynamicRTTI()->GetTypeName());
  }
#endif

  ResourceType* pResource = (ResourceType*)hResource.m_hTypeless.m_pResource;

  // XII_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  XII_ASSERT_DEBUG(pResource->GetDynamicRTTI()->template IsDerivedFrom<ResourceType>(), "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(), xiiGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == xiiResourceAcquireMode::AllowLoadingFallback && GetForceNoFallbackAcquisition() > 0)
  {
    mode = xiiResourceAcquireMode::BlockTillLoaded;
  }

  if (mode == xiiResourceAcquireMode::PointerOnly)
  {
    if (out_pAcquireResult)
      *out_pAcquireResult = xiiResourceAcquireResult::Final;

    // pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used
  // productively
  pResource->m_LastAcquire = GetLastFrameUpdate();

  if (pResource->GetLoadingState() != xiiResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != xiiResourceState::Loaded)
    {
      // if BlockTillLoaded is specified, it will prepended to the preload array, thus will be loaded immediately
      InternalPreloadResource(pResource, mode >= xiiResourceAcquireMode::BlockTillLoaded);

      if (mode == xiiResourceAcquireMode::AllowLoadingFallback && (pResource->m_hLoadingFallback.IsValid() || hFallbackResource.IsValid() || GetResourceTypeLoadingFallback<ResourceType>().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_pAcquireResult)
          *out_pAcquireResult = xiiResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hLoadingFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hLoadingFallback, xiiResourceAcquireMode::BlockTillLoaded);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, xiiResourceAcquireMode::BlockTillLoaded);
        else
          return (ResourceType*)BeginAcquireResource(GetResourceTypeLoadingFallback<ResourceType>(), xiiResourceAcquireMode::BlockTillLoaded);
      }

      EnsureResourceLoadingState(pResource, xiiResourceState::Loaded);
    }
    else
    {
      // as long as there are more quality levels available, schedule the resource for more loading
      // accessing IsQueuedForLoading without a lock here is save because InternalPreloadResource() will lock and early out if necessary
      // and accidentally skipping InternalPreloadResource() is no problem
      if (IsQueuedForLoading(pResource) == false && pResource->GetNumQualityLevelsLoadable() > 0)
        InternalPreloadResource(pResource, false);
    }
  }

  if (pResource->GetLoadingState() == xiiResourceState::LoadedResourceMissing)
  {
    // When you get a crash with a stack overflow in this code path, then the resource to be used as the
    // 'missing resource' replacement might be missing itself.

    if (xiiResourceManager::GetResourceTypeMissingFallback<ResourceType>().IsValid())
    {
      if (out_pAcquireResult)
        *out_pAcquireResult = xiiResourceAcquireResult::MissingFallback;

      return (ResourceType*)BeginAcquireResource(xiiResourceManager::GetResourceTypeMissingFallback<ResourceType>(), xiiResourceAcquireMode::BlockTillLoaded);
    }

    if (mode != xiiResourceAcquireMode::AllowLoadingFallback_NeverFail && mode != xiiResourceAcquireMode::BlockTillLoaded_NeverFail)
    {
      XII_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(), xiiGetStaticRTTI<ResourceType>()->GetTypeName());
    }

    if (out_pAcquireResult)
      *out_pAcquireResult = xiiResourceAcquireResult::None;

    return nullptr;
  }

  if (out_pAcquireResult)
    *out_pAcquireResult = xiiResourceAcquireResult::Final;

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
void xiiResourceManager::EndAcquireResource(ResourceType* pResource)
{
  XII_IGNORE_UNUSED(pResource);

  // XII_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (xiiInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

XII_FORCE_INLINE void xiiResourceManager::EndAcquireResourcePointer(xiiResource* pResource)
{
  XII_IGNORE_UNUSED(pResource);

  // XII_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (xiiInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

template <typename ResourceType>
xiiLockedObject<xiiMutex, xiiDynamicArray<xiiResource*>> xiiResourceManager::GetAllResourcesOfType()
{
  const xiiRTTI* pBaseType = xiiGetStaticRTTI<ResourceType>();

  auto& container = GetLoadedResourceOfTypeTempContainer();

  // We use a static container here to ensure its life-time is extended beyond
  // calls to this function as the locked object does not own the passed-in object
  // and thus does not extend the data life-time. It is safe to do this, as the
  // locked object holding the container ensures the container will not be
  // accessed concurrently.
  xiiLockedObject<xiiMutex, xiiDynamicArray<xiiResource*>> loadedResourcesLock(s_ResourceMutex, &container);

  container.Clear();

  for (auto itType = GetLoadedResources().GetIterator(); itType.IsValid(); itType.Next())
  {
    const xiiRTTI* pDerivedType = itType.Key();

    if (pDerivedType->IsDerivedFrom(pBaseType))
    {
      const LoadedResources& lr = GetLoadedResources()[pDerivedType];

      container.Reserve(container.GetCount() + lr.m_Resources.GetCount());

      for (auto itResource : lr.m_Resources)
      {
        container.PushBack(itResource.Value());
      }
    }
  }

  return loadedResourcesLock;
}

template <typename ResourceType>
bool xiiResourceManager::ReloadResource(const xiiTypedResourceHandle<ResourceType>& hResource, bool bForce)
{
  ResourceType* pResource = BeginAcquireResource(hResource, xiiResourceAcquireMode::PointerOnly);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResource(pResource);

  return res;
}

XII_FORCE_INLINE bool xiiResourceManager::ReloadResource(const xiiRTTI* pType, const xiiTypelessResourceHandle& hResource, bool bForce)
{
  xiiResource* pResource = BeginAcquireResourcePointer(pType, hResource);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResourcePointer(pResource);

  return res;
}

template <typename ResourceType>
xiiUInt32 xiiResourceManager::ReloadResourcesOfType(bool bForce)
{
  return ReloadResourcesOfType(xiiGetStaticRTTI<ResourceType>(), bForce);
}

template <typename ResourceType>
void xiiResourceManager::SetResourceTypeLoader(xiiResourceTypeLoader* pCreator)
{
  XII_LOCK(s_ResourceMutex);

  GetResourceTypeLoaders()[xiiGetStaticRTTI<ResourceType>()] = pCreator;
}

template <typename ResourceType>
xiiTypedResourceHandle<ResourceType> xiiResourceManager::GetResourceHandleForExport(xiiStringView sResourceID)
{
  XII_ASSERT_DEV(IsExportModeEnabled(), "Export mode needs to be enabled");

  return LoadResource<ResourceType>(sResourceID);
}

template <typename ResourceType>
void xiiResourceManager::SetIncrementalUnloadForResourceType(bool bActive)
{
  GetResourceTypeInfo(xiiGetStaticRTTI<ResourceType>()).m_bIncrementalUnload = bActive;
}
