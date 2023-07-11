#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

xiiTypelessResourceHandle xiiResourceManager::LoadResourceByType(const xiiRTTI* pResourceType, xiiStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  XII_LOCK(s_ResourceMutex);
  return xiiTypelessResourceHandle(GetResource(pResourceType, sResourceID, true));
}

void xiiResourceManager::InternalPreloadResource(xiiResource* pResource, bool bHighestPriority)
{
  if (s_pState->m_bShutdown)
    return;

  XII_PROFILE_SCOPE("InternalPreloadResource");

  XII_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == xiiResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    // XII_ASSERT_DEV(!IsQueuedForLoading(pResource), "Invalid flag on resource type '{0}'",
    // pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  XII_ASSERT_DEV(!s_pState->m_bExportMode, "Resources should not be loaded in export mode");

  // if we are already loading this resource, early out
  if (IsQueuedForLoading(pResource))
  {
    // however, if it now has highest priority and is still in the loading queue (so not yet started)
    // move it to the front of the queue
    if (bHighestPriority)
    {
      // if it is not in the queue anymore, it has already been started by some thread
      if (RemoveFromLoadingQueue(pResource).Succeeded())
      {
        AddToLoadingQueue(pResource, bHighestPriority);
      }
    }

    return;
  }
  else
  {
    AddToLoadingQueue(pResource, bHighestPriority);

    if (bHighestPriority && xiiTaskSystem::GetCurrentThreadWorkerType() == xiiWorkerThreadType::FileAccess)
    {
      xiiResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    }

    RunWorkerTask(pResource);
  }
}

void xiiResourceManager::SetupWorkerTasks()
{
  if (!s_pState->m_bTaskNamesInitialized)
  {
    s_pState->m_bTaskNamesInitialized = true;
    xiiStringBuilder s;

    {
      static const xiiUInt32 InitialDataLoadTasks = 4;

      for (xiiUInt32 i = 0; i < InitialDataLoadTasks; ++i)
      {
        s.Format("Resource Data Loader {0}", i);
        auto& data   = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
        data.m_pTask = XII_DEFAULT_NEW(xiiResourceManagerWorkerDataLoad);
        data.m_pTask->ConfigureTask(s, xiiTaskNesting::Maybe);
      }
    }

    {
      static const xiiUInt32 InitialUpdateContentTasks = 16;

      for (xiiUInt32 i = 0; i < InitialUpdateContentTasks; ++i)
      {
        s.Format("Resource Content Updater {0}", i);
        auto& data   = s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
        data.m_pTask = XII_DEFAULT_NEW(xiiResourceManagerWorkerUpdateContent);
        data.m_pTask->ConfigureTask(s, xiiTaskNesting::Maybe);
      }
    }
  }
}

void xiiResourceManager::RunWorkerTask(xiiResource* pResource)
{
  if (s_pState->m_bShutdown)
    return;

  XII_ASSERT_DEV(s_ResourceMutex.IsLocked(), "");

  SetupWorkerTasks();

  if (s_pState->m_bAllowLaunchDataLoadTask && !s_pState->m_LoadingQueue.IsEmpty())
  {
    s_pState->m_bAllowLaunchDataLoadTask = false;

    for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
    {
      if (s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
      {
        s_pState->m_WorkerTasksDataLoad[i].m_GroupId =
          xiiTaskSystem::StartSingleTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask, xiiTaskPriority::FileAccess);
        return;
      }
    }

    // could not find any unused task -> need to create a new one
    {
      xiiStringBuilder s;
      s.Format("Resource Data Loader {0}", s_pState->m_WorkerTasksDataLoad.GetCount());
      auto& data   = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
      data.m_pTask = XII_DEFAULT_NEW(xiiResourceManagerWorkerDataLoad);
      data.m_pTask->ConfigureTask(s, xiiTaskNesting::Maybe);
      data.m_GroupId = xiiTaskSystem::StartSingleTask(data.m_pTask, xiiTaskPriority::FileAccess);
    }
  }
}

void xiiResourceManager::ReverseBubbleSortStep(xiiDeque<LoadingInfo>& data)
{
  // Yep, it's really bubble sort!
  // This will move the entry with the smallest value to the front and move all other values closer to their correct position,
  // which is exactly what we need for the priority queue.
  // We do this once a frame, which gives us nice iterative sorting, with relatively deterministic performance characteristics.

  XII_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  const xiiUInt32 uiCount = data.GetCount();

  for (xiiUInt32 i = uiCount; i > 1; --i)
  {
    const xiiUInt32 idx2 = i - 1;
    const xiiUInt32 idx1 = i - 2;

    if (data[idx1].m_fPriority > data[idx2].m_fPriority)
    {
      xiiMath::Swap(data[idx1], data[idx2]);
    }
  }
}

void xiiResourceManager::UpdateLoadingDeadlines()
{
  if (s_pState->m_LoadingQueue.IsEmpty())
    return;

  XII_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  XII_PROFILE_SCOPE("UpdateLoadingDeadlines");

  const xiiUInt32 uiCount                     = s_pState->m_LoadingQueue.GetCount();
  s_pState->m_uiLastResourcePriorityUpdateIdx = xiiMath::Min(s_pState->m_uiLastResourcePriorityUpdateIdx, uiCount);

  xiiUInt32 uiUpdateCount = xiiMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);

  if (uiUpdateCount == 0)
  {
    s_pState->m_uiLastResourcePriorityUpdateIdx = 0;
    uiUpdateCount                               = xiiMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);
  }

  if (uiUpdateCount > 0)
  {
    {
      XII_PROFILE_SCOPE("EvalLoadingDeadlines");

      const xiiTime tNow = xiiTime::Now();

      for (xiiUInt32 i = 0; i < uiUpdateCount; ++i)
      {
        auto& element       = s_pState->m_LoadingQueue[s_pState->m_uiLastResourcePriorityUpdateIdx];
        element.m_fPriority = element.m_pResource->GetLoadingPriority(tNow);
        ++s_pState->m_uiLastResourcePriorityUpdateIdx;
      }
    }

    {
      XII_PROFILE_SCOPE("SortLoadingDeadlines");
      ReverseBubbleSortStep(s_pState->m_LoadingQueue);
    }
  }
}

void xiiResourceManager::PreloadResource(xiiResource* pResource)
{
  InternalPreloadResource(pResource, false);
}

void xiiResourceManager::PreloadResource(const xiiTypelessResourceHandle& hResource)
{
  XII_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  xiiResource* pResource = hResource.m_pResource;
  PreloadResource(pResource);
}

xiiResourceState xiiResourceManager::GetLoadingState(const xiiTypelessResourceHandle& hResource)
{
  if (hResource.m_pResource == nullptr)
    return xiiResourceState::Invalid;

  return hResource.m_pResource->GetLoadingState();
}

xiiResult xiiResourceManager::RemoveFromLoadingQueue(xiiResource* pResource)
{
  XII_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");

  if (!IsQueuedForLoading(pResource))
    return XII_SUCCESS;

  LoadingInfo li;
  li.m_pResource = pResource;

  if (s_pState->m_LoadingQueue.RemoveAndSwap(li))
  {
    pResource->m_Flags.Remove(xiiResourceFlags::IsQueuedForLoading);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiResourceManager::AddToLoadingQueue(xiiResource* pResource, bool bHighestPriority)
{
  XII_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");
  XII_ASSERT_DEV(IsQueuedForLoading(pResource) == false, "Resource is already in the loading queue");

  pResource->m_Flags.Add(xiiResourceFlags::IsQueuedForLoading);

  LoadingInfo li;
  li.m_pResource = pResource;

  if (bHighestPriority)
  {
    pResource->SetPriority(xiiResourcePriority::Critical);
    li.m_fPriority = 0.0f;
    s_pState->m_LoadingQueue.PushFront(li);
  }
  else
  {
    li.m_fPriority = pResource->GetLoadingPriority(s_pState->m_LastFrameUpdate);
    s_pState->m_LoadingQueue.PushBack(li);
  }
}

bool xiiResourceManager::ReloadResource(xiiResource* pResource, bool bForce)
{
  XII_LOCK(s_ResourceMutex);

  if (!pResource->m_Flags.IsAnySet(xiiResourceFlags::IsReloadable))
    return false;

  if (!bForce && pResource->m_Flags.IsAnySet(xiiResourceFlags::PreventFileReload))
    return false;

  xiiResourceTypeLoader* pLoader = xiiResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return false;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == xiiResourceState::Unloaded)
    return false;

  bool bAllowPreloading = true;

  // if the resource is already in the loading queue we can just keep it there
  if (IsQueuedForLoading(pResource))
  {
    bAllowPreloading = false;

    LoadingInfo li;
    li.m_pResource = pResource;

    if (s_pState->m_LoadingQueue.IndexOf(li) == xiiInvalidIndex)
    {
      // the resource is marked as 'loading' but it is not in the queue anymore
      // that means some task is already working on loading it
      // therefore we should not touch it (especially unload it), it might end up in an inconsistent state

      xiiLog::Dev(
        "Resource '{0}' is not being reloaded, because it is currently being loaded", xiiArgSensitive(pResource->GetResourceID(), "ResourceID"));
      return false;
    }
  }

  // if bForce, skip the outdated check
  if (!bForce)
  {
    if (!pLoader->IsResourceOutdated(pResource))
      return false;

    if (pResource->GetLoadingState() == xiiResourceState::LoadedResourceMissing)
    {
      xiiLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", xiiArgSensitive(pResource->GetResourceID(), "ResourceID"),
                  xiiArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
    else
    {
      xiiLog::Dev("Resource '{0}' is outdated and will be reloaded ('{1}')", xiiArgSensitive(pResource->GetResourceID(), "ResourceID"),
                  xiiArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
  }

  if (pResource->GetBaseResourceFlags().IsSet(xiiResourceFlags::UpdateOnMainThread) == false || xiiThreadUtils::IsMainThread())
  {
    // make sure existing data is purged
    pResource->CallUnloadData(xiiResource::Unload::AllQualityLevels);

    XII_ASSERT_DEV(pResource->GetLoadingState() <= xiiResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.",
                   pResource->GetResourceID());
  }
  else
  {
    s_pState->m_ResourcesToUnloadOnMainThread.Insert(xiiTempHashedString(pResource->GetResourceID().GetData()), pResource->GetDynamicRTTI());
  }

  if (bAllowPreloading)
  {
    const xiiTime tNow = s_pState->m_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - xiiTime::Seconds(30.0))
    {
      PreloadResource(pResource);
    }
  }

  return true;
}

xiiUInt32 xiiResourceManager::ReloadResourcesOfType(const xiiRTTI* pType, bool bForce)
{
  XII_LOCK(s_ResourceMutex);
  XII_LOG_BLOCK("xiiResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  xiiUInt32 count = 0;

  LoadedResources& lr = s_pState->m_LoadedResources[pType];

  for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    if (ReloadResource(it.Value(), bForce))
      ++count;
  }

  return count;
}

xiiUInt32 xiiResourceManager::ReloadAllResources(bool bForce)
{
  XII_PROFILE_SCOPE("ReloadAllResources");

  XII_LOCK(s_ResourceMutex);
  XII_LOG_BLOCK("xiiResourceManager::ReloadAllResources");

  xiiUInt32 count = 0;

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      if (ReloadResource(it.Value(), bForce))
        ++count;
    }
  }

  if (count > 0)
  {
    xiiResourceManagerEvent e;
    e.m_Type = xiiResourceManagerEvent::Type::ReloadAllResources;

    s_pState->m_ManagerEvents.Broadcast(e);
  }

  return count;
}

void xiiResourceManager::UpdateResourceWithCustomLoader(const xiiTypelessResourceHandle& hResource, xiiUniquePtr<xiiResourceTypeLoader>&& pLoader)
{
  XII_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(xiiResourceFlags::HasCustomDataLoader);
  s_pState->m_CustomLoaders[hResource.m_pResource] = std::move(pLoader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

void xiiResourceManager::EnsureResourceLoadingState(xiiResource* pResourceToLoad, const xiiResourceState RequestedState)
{
  const xiiRTTI* pOwnRtti = pResourceToLoad->GetDynamicRTTI();

  // help loading until the requested resource is available
  while ((xiiInt32)pResourceToLoad->GetLoadingState() < (xiiInt32)RequestedState &&
         (pResourceToLoad->GetLoadingState() != xiiResourceState::LoadedResourceMissing))
  {
    xiiTaskGroupID tgid;

    {
      XII_LOCK(s_ResourceMutex);

      for (xiiUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
      {
        const xiiResource* pQueuedResource = s_pState->m_WorkerTasksUpdateContent[i].m_pTask->m_pResourceToLoad;

        if (pQueuedResource != nullptr && pQueuedResource != pResourceToLoad && !s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
        {
          if (!IsResourceTypeAcquireDuringUpdateContentAllowed(pQueuedResource->GetDynamicRTTI(), pOwnRtti))
          {
            tgid = s_pState->m_WorkerTasksUpdateContent[i].m_GroupId;
            break;
          }
        }
      }
    }

    if (tgid.IsValid())
    {
      xiiTaskSystem::WaitForGroup(tgid);
    }
    else
    {
      // do not use xiiThreadUtils::YieldTimeSlice here, otherwise the thread is not tagged as 'blocked' in the TaskSystem
      xiiTaskSystem::WaitForCondition([=]() -> bool {
        return (xiiInt32)pResourceToLoad->GetLoadingState() >= (xiiInt32)RequestedState ||
          (pResourceToLoad->GetLoadingState() == xiiResourceState::LoadedResourceMissing);
      });
    }
  }
}


XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceLoading);
