/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>
#include <Foundation/Time/Clock.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

/// \todo Do not unload resources while they are acquired
/// \todo Resource Type Memory Thresholds
/// \todo Preload does not load all quality levels

/// Infos to Display:
///   Ref Count (max)
///   Fallback: Type / Instance
///   Loading Time

/// Resource Flags:
/// Category / Group (Texture Sets)

/// Resource Loader
///   Requires No File Access -> on non-File Thread

xiiUniquePtr<xiiResourceManagerState> xiiResourceManager::s_pState;
xiiMutex                              xiiResourceManager::s_ResourceMutex;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, ResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::OnCoreStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::OnCoreShutdown();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiResourceManager::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


xiiResourceTypeLoader* xiiResourceManager::GetResourceTypeLoader(const xiiRTTI* pRTTI)
{
  return s_pState->m_ResourceTypeLoader[pRTTI];
}

xiiMap<const xiiRTTI*, xiiResourceTypeLoader*>& xiiResourceManager::GetResourceTypeLoaders()
{
  return s_pState->m_ResourceTypeLoader;
}

void xiiResourceManager::AddResourceCleanupCallback(ResourceCleanupCB cb)
{
  XII_ASSERT_DEV(cb.IsComparable(), "Delegates with captures are not allowed");

  for (xiiUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
      return;
  }

  s_pState->m_ResourceCleanupCallbacks.PushBack(cb);
}

void xiiResourceManager::ClearResourceCleanupCallback(ResourceCleanupCB cb)
{
  for (xiiUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
    {
      s_pState->m_ResourceCleanupCallbacks.RemoveAtAndSwap(i);
      return;
    }
  }
}

void xiiResourceManager::ExecuteAllResourceCleanupCallbacks()
{
  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, nothing to do
    return;
  }

  xiiDynamicArray<ResourceCleanupCB> callbacks = s_pState->m_ResourceCleanupCallbacks;
  s_pState->m_ResourceCleanupCallbacks.Clear();

  for (auto& cb : callbacks)
  {
    cb();
  }

  XII_ASSERT_DEV(s_pState->m_ResourceCleanupCallbacks.IsEmpty(), "During resource cleanup, new resource cleanup callbacks were registered.");
}

xiiMap<const xiiRTTI*, xiiResourcePriority>& xiiResourceManager::GetResourceTypePriorities()
{
  return s_pState->m_ResourceTypePriorities;
}

void xiiResourceManager::BroadcastResourceEvent(const xiiResourceEvent& e)
{
  XII_LOCK(s_ResourceMutex);

  // broadcast it through the resource to everyone directly interested in that specific resource
  e.m_pResource->m_ResourceEvents.Broadcast(e);

  // and then broadcast it to everyone else through the general event
  s_pState->m_ResourceEvents.Broadcast(e);
}

void xiiResourceManager::RegisterResourceForAssetType(xiiStringView sAssetTypeName, const xiiRTTI* pResourceType)
{
  xiiStringBuilder s = sAssetTypeName;
  s.ToLower();

  s_pState->m_AssetToResourceType[s] = pResourceType;
}

const xiiRTTI* xiiResourceManager::FindResourceForAssetType(xiiStringView sAssetTypeName)
{
  xiiStringBuilder s = sAssetTypeName;
  s.ToLower();

  return s_pState->m_AssetToResourceType.GetValueOrDefault(s, nullptr);
}

void xiiResourceManager::ForceNoFallbackAcquisition(xiiUInt32 uiNumFrames /*= 0xFFFFFFFF*/)
{
  s_pState->m_uiForceNoFallbackAcquisition = xiiMath::Max(s_pState->m_uiForceNoFallbackAcquisition, uiNumFrames);
}

xiiUInt32 xiiResourceManager::FreeAllUnusedResources()
{
  XII_LOG_BLOCK("xiiResourceManager::FreeAllUnusedResources");

  XII_PROFILE_SCOPE("FreeAllUnusedResources");

  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, no resources to unload
    return 0;
  }

  const bool bFreeAllUnused = true;

  xiiUInt32 uiUnloaded   = 0;
  bool      bUnloadedAny = false;
  bool      bAnyFailed   = false;

  do
  {
    {
      XII_LOCK(s_ResourceMutex);

      bUnloadedAny = false;

      for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
      {
        LoadedResources& lr = itType.Value();

        for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); /* empty */)
        {
          xiiResource* pReference = it.Value();

          if (pReference->m_iReferenceCount == 0)
          {
            bUnloadedAny = true; // make sure to try again, even if DeallocateResource() fails; need to release our lock for that to prevent dead-locks

            if (DeallocateResource(pReference).Succeeded())
            {
              ++uiUnloaded;

              it = lr.m_Resources.Remove(it);
              continue;
            }
            else
            {
              bAnyFailed = true;
            }
          }

          ++it;
        }
      }
    }

    if (bAnyFailed)
    {
      // When this happens, it is possible that the resource that failed to be deleted
      // is dependent on a task that needs to be executed on THIS thread (main thread).
      // Therefore, help executing some tasks here, to unblock the task system.

      bAnyFailed = false;

      xiiInt32 iHelpExecTasksRounds = 1;
      xiiTaskSystem::WaitForCondition([&iHelpExecTasksRounds]() { return iHelpExecTasksRounds-- <= 0; });
    }

  } while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
}

xiiUInt32 xiiResourceManager::FreeUnusedResources(xiiTime timeout, xiiTime lastAcquireThreshold)
{
  if (timeout.IsZeroOrNegative())
    return 0;

  XII_LOCK(s_ResourceMutex);
  XII_LOG_BLOCK("xiiResourceManager::FreeUnusedResources");
  XII_PROFILE_SCOPE("FreeUnusedResources");

  auto itResourceType = s_pState->m_LoadedResources.Find(s_pState->m_pFreeUnusedLastType);
  if (!itResourceType.IsValid())
  {
    itResourceType = s_pState->m_LoadedResources.GetIterator();
  }

  if (!itResourceType.IsValid())
    return 0;

  auto itResourceID = itResourceType.Value().m_Resources.Find(s_pState->m_sFreeUnusedLastResourceID);
  if (!itResourceID.IsValid())
  {
    itResourceID = itResourceType.Value().m_Resources.GetIterator();
  }

  const xiiTime tStart = xiiTime::Now();

  xiiUInt32 uiDeallocatedCount = 0;

  xiiStringBuilder sResourceName;

  const xiiRTTI* pLastTypeCheck = nullptr;

  // stop once we wasted enough time
  while (xiiTime::Now() - tStart < timeout)
  {
    if (!itResourceID.IsValid())
    {
      // reached the end of this resource type
      // advance to the next resource type
      ++itResourceType;

      if (!itResourceType.IsValid())
      {
        // if we reached the end, reset everything and stop

        s_pState->m_pFreeUnusedLastType       = nullptr;
        s_pState->m_sFreeUnusedLastResourceID = xiiTempHashedString();
        return uiDeallocatedCount;
      }


      // reset resource ID to the beginning of this type and start over
      itResourceID = itResourceType.Value().m_Resources.GetIterator();
      continue;
    }

    s_pState->m_pFreeUnusedLastType       = itResourceType.Key();
    s_pState->m_sFreeUnusedLastResourceID = itResourceID.Key();

    if (pLastTypeCheck != itResourceType.Key())
    {
      pLastTypeCheck = itResourceType.Key();

      if (GetResourceTypeInfo(pLastTypeCheck).m_bIncrementalUnload == false)
      {
        itResourceID = itResourceType.Value().m_Resources.GetEndIterator();
        continue;
      }
    }

    xiiResource* pResource = itResourceID.Value();

    if ((pResource->GetReferenceCount() == 0) && (tStart - pResource->GetLastAcquireTime() > lastAcquireThreshold))
    {
      sResourceName = pResource->GetResourceID();

      if (DeallocateResource(pResource).Succeeded())
      {
        xiiLog::Debug("Freed '{}'", xiiArgSensitive(sResourceName, "ResourceID"));

        ++uiDeallocatedCount;
        itResourceID = itResourceType.Value().m_Resources.Remove(itResourceID);
        continue;
      }
    }

    ++itResourceID;
  }

  return uiDeallocatedCount;
}

void xiiResourceManager::SetAutoFreeUnused(xiiTime timeout, xiiTime lastAcquireThreshold)
{
  s_pState->m_AutoFreeUnusedTimeout   = timeout;
  s_pState->m_AutoFreeUnusedThreshold = lastAcquireThreshold;
}

void xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent(const xiiRTTI* pTypeBeingUpdated, const xiiRTTI* pTypeItWantsToAcquire)
{
  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  XII_ASSERT_DEV(info.m_bAllowNestedAcquireCached == false, "AllowResourceTypeAcquireDuringUpdateContent for type '{}' must be called before the resource info has been requested.", pTypeBeingUpdated->GetTypeName());

  if (info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) == xiiInvalidIndex)
  {
    info.m_NestedTypes.PushBack(pTypeItWantsToAcquire);
  }
}

bool xiiResourceManager::IsResourceTypeAcquireDuringUpdateContentAllowed(const xiiRTTI* pTypeBeingUpdated, const xiiRTTI* pTypeItWantsToAcquire)
{
  XII_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "");

  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  if (!info.m_bAllowNestedAcquireCached)
  {
    info.m_bAllowNestedAcquireCached = true;

    xiiSet<const xiiRTTI*> visited;
    xiiSet<const xiiRTTI*> todo;
    xiiSet<const xiiRTTI*> deps;

    for (const xiiRTTI* pRtti : info.m_NestedTypes)
    {
      xiiRTTI::ForEachDerivedType(pRtti, [&](const xiiRTTI* pDerived) { todo.Insert(pDerived); });
    }

    while (!todo.IsEmpty())
    {
      auto           it    = todo.GetIterator();
      const xiiRTTI* pRtti = it.Key();
      todo.Remove(it);

      if (visited.Contains(pRtti))
        continue;

      visited.Insert(pRtti);
      deps.Insert(pRtti);

      for (const xiiRTTI* pNestedRtti : s_pState->m_TypeInfo[pRtti].m_NestedTypes)
      {
        if (!visited.Contains(pNestedRtti))
        {
          xiiRTTI::ForEachDerivedType(pNestedRtti, [&](const xiiRTTI* pDerived) { todo.Insert(pDerived); });
        }
      }
    }

    info.m_NestedTypes.Clear();
    for (const xiiRTTI* pRtti : deps)
    {
      info.m_NestedTypes.PushBack(pRtti);
    }
    info.m_NestedTypes.Sort();
  }

  return info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) != xiiInvalidIndex;
}

xiiResult xiiResourceManager::DeallocateResource(xiiResource* pResource)
{
  // XII_ASSERT_DEBUG(pResource->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.", pResource->GetResourceID());

  if (RemoveFromLoadingQueue(pResource).Failed())
  {
    // cannot deallocate resources that are currently queued for loading,
    // especially when they are already picked up by a task
    return XII_FAILURE;
  }

  pResource->CallUnloadData(xiiResource::Unload::AllQualityLevels);

  XII_ASSERT_DEBUG(pResource->GetLoadingState() <= xiiResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  // broadcast that we are going to delete the resource
  {
    xiiResourceEvent e;
    e.m_pResource = pResource;
    e.m_Type      = xiiResourceEvent::Type::ResourceDeleted;
    xiiResourceManager::BroadcastResourceEvent(e);
  }

  XII_ASSERT_DEV(pResource->GetReferenceCount() == 0, "The resource '{}' ({}) is being deallocated, you just stored a handle to it, which won't work! If you are listening to xiiResourceEvent::Type::ResourceContentUnloading then additionally listen to xiiResourceEvent::Type::ResourceDeleted to clean up handles to dead resources.", pResource->GetResourceID(), pResource->GetResourceDescription());

  // delete the resource via the RTTI provided allocator
  pResource->GetDynamicRTTI()->GetAllocator()->Deallocate(pResource);

  return XII_SUCCESS;
}

// To allow triggering this event without a link dependency
// Used by Fileserve, to trigger this event, even though Fileserve should not have a link dependency on Core
XII_ON_GLOBAL_EVENT(xiiResourceManager_ReloadAllResources)
{
  XII_IGNORE_UNUSED(param0);
  XII_IGNORE_UNUSED(param1);
  XII_IGNORE_UNUSED(param2);
  XII_IGNORE_UNUSED(param3);

  xiiResourceManager::ReloadAllResources(false);
}
void xiiResourceManager::ResetAllResources()
{
  XII_LOCK(s_ResourceMutex);
  XII_LOG_BLOCK("xiiResourceManager::ReloadAllResources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      xiiResource* pResource = it.Value();
      pResource->ResetResource();
    }
  }
}

void xiiResourceManager::PerFrameUpdate()
{
  XII_PROFILE_SCOPE("xiiResourceManagerUpdate");

  s_pState->m_LastFrameUpdate = xiiClock::GetGlobalClock()->GetLastUpdateTime();

  if (s_pState->m_bBroadcastExistsEvent)
  {
    XII_LOCK(s_ResourceMutex);

    s_pState->m_bBroadcastExistsEvent = false;

    for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
    {
      for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        xiiResourceEvent e;
        e.m_Type      = xiiResourceEvent::Type::ResourceExists;
        e.m_pResource = it.Value();

        xiiResourceManager::BroadcastResourceEvent(e);
      }
    }
  }

  {
    XII_LOCK(s_ResourceMutex);

    for (auto it = s_pState->m_ResourcesToUnloadOnMainThread.GetIterator(); it.IsValid(); it.Next())
    {
      // Identify the container of loaded resource for the type of resource we want to unload.
      LoadedResources loadedResourcesForType;
      if (s_pState->m_LoadedResources.TryGetValue(it.Value(), loadedResourcesForType) == false)
      {
        continue;
      }

      // See, if the resource we want to unload still exists.
      xiiResource* resourceToUnload = nullptr;

      if (loadedResourcesForType.m_Resources.TryGetValue(it.Key(), resourceToUnload) == false)
      {
        continue;
      }

      XII_ASSERT_DEV(resourceToUnload != nullptr, "Found a resource above, should not be nullptr.");

      // If the resource was still loaded, we are going to unload it now.
      resourceToUnload->CallUnloadData(xiiResource::Unload::AllQualityLevels);

      XII_ASSERT_DEV(resourceToUnload->GetLoadingState() <= xiiResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", resourceToUnload->GetResourceID());
    }

    s_pState->m_ResourcesToUnloadOnMainThread.Clear();
  }

  if (s_pState->m_AutoFreeUnusedTimeout.IsPositive())
  {
    FreeUnusedResources(s_pState->m_AutoFreeUnusedTimeout, s_pState->m_AutoFreeUnusedThreshold);
  }
}

const xiiEvent<const xiiResourceEvent&, xiiMutex>& xiiResourceManager::GetResourceEvents()
{
  return s_pState->m_ResourceEvents;
}

const xiiEvent<const xiiResourceManagerEvent&, xiiMutex>& xiiResourceManager::GetManagerEvents()
{
  return s_pState->m_ManagerEvents;
}

void xiiResourceManager::BroadcastExistsEvent()
{
  s_pState->m_bBroadcastExistsEvent = true;
}

void xiiResourceManager::PluginEventHandler(const xiiPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiPluginEvent::AfterStartupShutdown:
    {
      // unload all resources until there are no more that can be unloaded
      // this is to prevent having resources allocated that came from a dynamic plugin
      FreeAllUnusedResources();
    }
    break;

    default:
      break;
  }
}

void xiiResourceManager::OnCoreStartup()
{
  s_pState = XII_DEFAULT_NEW(xiiResourceManagerState);

  XII_LOCK(s_ResourceMutex);
  s_pState->m_bAllowLaunchDataLoadTask = true;
  s_pState->m_bShutdown                = false;

  xiiPlugin::Events().AddEventHandler(PluginEventHandler);
}

void xiiResourceManager::EngineAboutToShutdown()
{
  {
    XII_LOCK(s_ResourceMutex);

    if (s_pState == nullptr)
    {
      // In case resource manager wasn't initialized, nothing to do
      return;
    }

    s_pState->m_bAllowLaunchDataLoadTask = false; // prevent a new one from starting
    s_pState->m_bShutdown                = true;
  }

  for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    xiiTaskSystem::CancelTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask).IgnoreResult();
  }

  for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    xiiTaskSystem::CancelTask(s_pState->m_WorkerTasksUpdateContent[i].m_pTask).IgnoreResult();
  }

  {
    XII_LOCK(s_ResourceMutex);

    for (auto entry : s_pState->m_LoadingQueue)
    {
      entry.m_pResource->m_Flags.Remove(xiiResourceFlags::IsQueuedForLoading);
    }

    s_pState->m_LoadingQueue.Clear();

    // Since we just canceled all loading tasks above and cleared the loading queue,
    // some resources may still be flagged as 'loading', but can never get loaded.
    // That can deadlock the 'FreeAllUnused' function, because it won't delete 'loading' resources.
    // Therefore we need to make sure no resource has the IsQueuedForLoading flag set anymore.
    for (auto itTypes : s_pState->m_LoadedResources)
    {
      for (auto itRes : itTypes.Value().m_Resources)
      {
        xiiResource* pRes = itRes.Value();

        if (pRes->GetBaseResourceFlags().IsSet(xiiResourceFlags::IsQueuedForLoading))
        {
          pRes->m_Flags.Remove(xiiResourceFlags::IsQueuedForLoading);
        }
      }
    }
  }
}

bool xiiResourceManager::IsAnyLoadingInProgress()
{
  XII_LOCK(s_ResourceMutex);

  if (s_pState->m_LoadingQueue.GetCount() > 0)
  {
    return true;
  }

  for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }

  for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }
  return false;
}

void xiiResourceManager::OnEngineShutdown()
{
  xiiResourceManagerEvent e;
  e.m_Type = xiiResourceManagerEvent::Type::ManagerShuttingDown;

  // In case of a crash inside the event broadcast or ExecuteAllResourceCleanupCallbacks():
  // - You might have a resource type added through a dynamic plugin that has already been unloaded, but the event handler is still referenced.
  // - To fix this, call xiiResource::CleanupDynamicPluginReferences() on that resource type during engine shutdown (see xiiStartup).
  s_pState->m_ManagerEvents.Broadcast(e);

  ExecuteAllResourceCleanupCallbacks();

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeAllUnusedResources();
}

void xiiResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  XII_LOG_BLOCK("Referenced Resources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    const xiiRTTI*   pRtti = itType.Key();
    LoadedResources& lr    = itType.Value();

    if (!lr.m_Resources.IsEmpty())
    {
      XII_LOG_BLOCK("Type", pRtti->GetTypeName());

      xiiLog::Error("{0} resource of type '{1}' are still referenced.", lr.m_Resources.GetCount(), pRtti->GetTypeName());

      for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        xiiResource* pReference = it.Value();

        xiiLog::Info("RC = {0}, ID = '{1}'", pReference->GetReferenceCount(), xiiArgSensitive(pReference->GetResourceID(), "ResourceID"));

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
        pReference->PrintHandleStackTraces();
#endif
      }
    }
  }

  xiiPlugin::Events().RemoveEventHandler(PluginEventHandler);

  s_pState.Clear();
}

xiiResource* xiiResourceManager::GetResource(const xiiRTTI* pRtti, xiiStringView sResourceID, bool bIsReloadable)
{
  if (sResourceID.IsEmpty())
    return nullptr;

  XII_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Calling code must lock the mutex until the resource pointer is stored in a handle");

  // redirect requested type to override type, if available
  pRtti = FindResourceTypeOverride(pRtti, sResourceID);

  XII_ASSERT_DEBUG(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", XII_PP_STRINGIFY(ResourceType));

  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
  {
    // this can happen for assets that use a resource type override (such as scripts) shortly after they have been created
    return nullptr;
  }

  XII_ASSERT_DEBUG(pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate(), "There is no RTTI allocator available for the given resource type '{0}'", XII_PP_STRINGIFY(ResourceType));

  xiiTempHashedString sHashedResourceID(sResourceID);

  xiiHashedString* pRedirection;
  if (s_pState->m_NamedResources.TryGetValue(sHashedResourceID, pRedirection))
  {
    sHashedResourceID = *pRedirection;
    sResourceID       = pRedirection->GetView();
  }

  LoadedResources& lr = s_pState->m_LoadedResources[pRtti];

  xiiResource*& pResource = lr.m_Resources[sHashedResourceID];
  if (pResource != nullptr)
    return pResource;

  pResource             = pRtti->GetAllocator()->Allocate<xiiResource>();
  pResource->m_Priority = s_pState->m_ResourceTypePriorities.GetValueOrDefault(pRtti, xiiResourcePriority::Medium);
  pResource->SetUniqueID(sResourceID, bIsReloadable);
  pResource->m_Flags.AddOrRemove(xiiResourceFlags::ResourceHasTypeFallback, pResource->HasResourceTypeLoadingFallback());

  return pResource;
}

void xiiResourceManager::RegisterResourceOverrideType(const xiiRTTI* pDerivedTypeToUse, xiiDelegate<bool(const xiiStringBuilder&)> overrideDecider)
{
  const xiiRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != xiiGetStaticRTTI<xiiResource>())
  {
    auto& info          = s_pState->m_DerivedTypeInfos[pParentType].ExpandAndGetRef();
    info.m_pDerivedType = pDerivedTypeToUse;
    info.m_Decider      = overrideDecider;

    pParentType = pParentType->GetParentType();
  }
}

void xiiResourceManager::UnregisterResourceOverrideType(const xiiRTTI* pDerivedTypeToUse)
{
  const xiiRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != xiiGetStaticRTTI<xiiResource>())
  {
    auto it     = s_pState->m_DerivedTypeInfos.Find(pParentType);
    pParentType = pParentType->GetParentType();

    if (!it.IsValid())
      break;

    auto& infos = it.Value();

    for (xiiUInt32 i = infos.GetCount(); i > 0; --i)
    {
      if (infos[i - 1].m_pDerivedType == pDerivedTypeToUse)
      {
        infos.RemoveAtAndSwap(i - 1);
      }
    }
  }
}

const xiiRTTI* xiiResourceManager::FindResourceTypeOverride(const xiiRTTI* pRtti, xiiStringView sResourceID)
{
  auto it = s_pState->m_DerivedTypeInfos.Find(pRtti);

  if (!it.IsValid())
    return pRtti;

  xiiStringBuilder sRedirectedPath;
  xiiFileSystem::ResolveAssetRedirection(sResourceID, sRedirectedPath);

  while (it.IsValid())
  {
    for (const auto& info : it.Value())
    {
      if (info.m_Decider(sRedirectedPath))
      {
        pRtti = info.m_pDerivedType;
        it    = s_pState->m_DerivedTypeInfos.Find(pRtti);
        continue;
      }
    }

    break;
  }

  return pRtti;
}

xiiString xiiResourceManager::GenerateUniqueResourceID(xiiStringView sResourceIDPrefix)
{
  xiiStringBuilder sResourceID;
  sResourceID.SetFormat("{}-{}", sResourceIDPrefix, s_pState->m_uiNextResourceID++);
  return sResourceID;
}

xiiTypelessResourceHandle xiiResourceManager::GetExistingResourceByType(const xiiRTTI* pResourceType, xiiStringView sResourceID)
{
  xiiResource* pResource = nullptr;

  const xiiTempHashedString sResourceHash(sResourceID);

  XII_LOCK(s_ResourceMutex);

  const xiiRTTI* pRtti = FindResourceTypeOverride(pResourceType, sResourceID);

  if (s_pState->m_LoadedResources[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return xiiTypelessResourceHandle(pResource);

  return xiiTypelessResourceHandle();
}

xiiTypelessResourceHandle xiiResourceManager::GetExistingResourceOrCreateAsync(const xiiRTTI* pResourceType, xiiStringView sResourceID, xiiUniquePtr<xiiResourceTypeLoader>&& pLoader)
{
  XII_LOCK(s_ResourceMutex);

  xiiTypelessResourceHandle hResource = GetExistingResourceByType(pResourceType, sResourceID);

  if (hResource.IsValid())
    return hResource;

  hResource              = GetResource(pResourceType, sResourceID, false);
  xiiResource* pResource = hResource.m_pResource;

  pResource->m_Flags.Add(xiiResourceFlags::HasCustomDataLoader | xiiResourceFlags::IsCreatedResource);
  s_pState->m_CustomLoaders[pResource] = std::move(pLoader);

  return hResource;
}

void xiiResourceManager::ForceLoadResourceNow(const xiiTypelessResourceHandle& hResource)
{
  XII_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  xiiResource* pResource = hResource.m_pResource;

  if (pResource->GetLoadingState() != xiiResourceState::LoadedResourceMissing && pResource->GetLoadingState() != xiiResourceState::Loaded)
  {
    InternalPreloadResource(pResource, true);

    EnsureResourceLoadingState(hResource.m_pResource, xiiResourceState::Loaded);
  }
}

void xiiResourceManager::RegisterNamedResource(xiiStringView sLookupName, xiiStringView sRedirectionResource)
{
  XII_LOCK(s_ResourceMutex);

  xiiTempHashedString sLookup(sLookupName);

  xiiHashedString sRedirection;
  sRedirection.Assign(sRedirectionResource);

  s_pState->m_NamedResources[sLookup] = sRedirection;
}

void xiiResourceManager::UnregisterNamedResource(xiiStringView sLookupName)
{
  XII_LOCK(s_ResourceMutex);

  xiiTempHashedString sHash(sLookupName);
  s_pState->m_NamedResources.Remove(sHash);
}

void xiiResourceManager::SetResourceLowResData(const xiiTypelessResourceHandle& hResource, xiiStreamReader* pStream)
{
  xiiResource* pResource = hResource.m_pResource;

  if (pResource->GetBaseResourceFlags().IsSet(xiiResourceFlags::HasLowResData))
    return;

  if (!pResource->GetBaseResourceFlags().IsSet(xiiResourceFlags::IsReloadable))
    return;

  XII_LOCK(s_ResourceMutex);

  // set this, even if we don't end up using the data (because some thread is already loading the full thing)
  pResource->m_Flags.Add(xiiResourceFlags::HasLowResData);

  if (IsQueuedForLoading(pResource))
  {
    // if we cannot find it in the queue anymore, some thread already started loading it
    // in this case, do not try to modify it
    if (RemoveFromLoadingQueue(pResource).Failed())
      return;
  }

  pResource->CallUpdateContent(pStream);

  XII_ASSERT_DEV(pResource->GetLoadingState() != xiiResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    xiiResource::MemoryUsage memoryUsage;
    memoryUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    memoryUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    pResource->UpdateMemoryUsage(memoryUsage);

    XII_ASSERT_DEV(memoryUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResource->GetResourceID());
    XII_ASSERT_DEV(memoryUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResource->GetResourceID());

    pResource->m_MemoryUsage = memoryUsage;
  }
}

xiiResourceTypeLoader* xiiResourceManager::GetDefaultResourceLoader()
{
  return s_pState->m_pDefaultResourceLoader;
}

void xiiResourceManager::EnableExportMode(bool bEnable)
{
  XII_ASSERT_DEV(s_pState != nullptr, "xiiStartup::StartupCoreSystems() must be called before using the xiiResourceManager.");

  s_pState->m_bExportMode = bEnable;
}

bool xiiResourceManager::IsExportModeEnabled()
{
  XII_ASSERT_DEV(s_pState != nullptr, "xiiStartup::StartupCoreSystems() must be called before using the xiiResourceManager.");

  return s_pState->m_bExportMode;
}

void xiiResourceManager::RestoreResource(const xiiTypelessResourceHandle& hResource)
{
  XII_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  xiiResource* pResource = hResource.m_pResource;
  pResource->m_Flags.Remove(xiiResourceFlags::PreventFileReload);

  ReloadResource(pResource, true);
}

xiiUInt32 xiiResourceManager::GetForceNoFallbackAcquisition()
{
  return s_pState->m_uiForceNoFallbackAcquisition;
}

xiiTime xiiResourceManager::GetLastFrameUpdate()
{
  return s_pState->m_LastFrameUpdate;
}

xiiHashTable<const xiiRTTI*, xiiResourceManager::LoadedResources>& xiiResourceManager::GetLoadedResources()
{
  return s_pState->m_LoadedResources;
}

xiiDynamicArray<xiiResource*>& xiiResourceManager::GetLoadedResourceOfTypeTempContainer()
{
  return s_pState->m_LoadedResourceOfTypeTempContainer;
}

void xiiResourceManager::SetDefaultResourceLoader(xiiResourceTypeLoader* pDefaultLoader)
{
  XII_LOCK(s_ResourceMutex);

  s_pState->m_pDefaultResourceLoader = pDefaultLoader;
}

xiiResourceManager::ResourceTypeInfo& xiiResourceManager::GetResourceTypeInfo(const xiiRTTI* pRtti)
{
  return s_pState->m_TypeInfo[pRtti];
}

XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);
