/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

xiiResourceManagerWorkerDataLoad::xiiResourceManagerWorkerDataLoad()  = default;
xiiResourceManagerWorkerDataLoad::~xiiResourceManagerWorkerDataLoad() = default;

void xiiResourceManagerWorkerDataLoad::Execute()
{
  XII_PROFILE_SCOPE("LoadResourceFromDisk");

  xiiResource*                        pResourceToLoad = nullptr;
  xiiResourceTypeLoader*              pLoader         = nullptr;
  xiiUniquePtr<xiiResourceTypeLoader> pCustomLoader;

  {
    XII_LOCK(xiiResourceManager::s_ResourceMutex);

    if (xiiResourceManager::s_pState->m_LoadingQueue.IsEmpty())
    {
      xiiResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
      return;
    }

    xiiResourceManager::UpdateLoadingDeadlines();

    auto it         = xiiResourceManager::s_pState->m_LoadingQueue.PeekFront();
    pResourceToLoad = it.m_pResource;
    xiiResourceManager::s_pState->m_LoadingQueue.PopFront();

    if (pResourceToLoad->m_Flags.IsSet(xiiResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(xiiResourceManager::s_pState->m_CustomLoaders[pResourceToLoad]);
      pLoader       = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(xiiResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(xiiResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = xiiResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  XII_ASSERT_DEV(pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  xiiResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(xiiResourceFlags::UpdateOnMainThread);

  xiiSharedPtr<xiiResourceManagerWorkerUpdateContent> pUpdateContentTask;
  xiiTaskGroupID*                                     pUpdateContentGroup = nullptr;

  XII_LOCK(xiiResourceManager::s_ResourceMutex);

  // try to find an update content task that has finished and can be reused
  for (xiiUInt32 i = 0; i < xiiResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    auto& td = xiiResourceManager::s_pState->m_WorkerTasksUpdateContent[i];

    if (xiiTaskSystem::IsTaskGroupFinished(td.m_GroupId))
    {
      pUpdateContentTask  = td.m_pTask;
      pUpdateContentGroup = &td.m_GroupId;
      break;
    }
  }

  // if no such task could be found, we must allocate a new one
  if (pUpdateContentTask == nullptr)
  {
    xiiStringBuilder s;
    s.SetFormat("Resource Content Updater {0}", xiiResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount());

    auto& td   = xiiResourceManager::s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
    td.m_pTask = XII_DEFAULT_NEW(xiiResourceManagerWorkerUpdateContent);
    td.m_pTask->ConfigureTask(s, xiiTaskNesting::Maybe);

    pUpdateContentTask  = td.m_pTask;
    pUpdateContentGroup = &td.m_GroupId;
  }

  // always updated together with pUpdateContentTask
  XII_MSVC_ANALYSIS_ASSUME(pUpdateContentGroup != nullptr);

  // set up the data load task and launch it
  {
    pUpdateContentTask->m_LoaderData      = LoaderData;
    pUpdateContentTask->m_pLoader         = pLoader;
    pUpdateContentTask->m_pCustomLoader   = std::move(pCustomLoader);
    pUpdateContentTask->m_pResourceToLoad = pResourceToLoad;

    // schedule the task to run, either on the main thread or on some other thread
    *pUpdateContentGroup = xiiTaskSystem::StartSingleTask(pUpdateContentTask, bResourceIsLoadedOnMainThread ? xiiTaskPriority::SomeFrameMainThread : xiiTaskPriority::LateNextFrame);

    // restart the next loading task (this one is about to finish)
    xiiResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    xiiResourceManager::RunWorkerTask();

    pCustomLoader.Clear();
  }
}

//////////////////////////////////////////////////////////////////////////

xiiResourceManagerWorkerUpdateContent::xiiResourceManagerWorkerUpdateContent()  = default;
xiiResourceManagerWorkerUpdateContent::~xiiResourceManagerWorkerUpdateContent() = default;

void xiiResourceManagerWorkerUpdateContent::Execute()
{
  if (!m_LoaderData.m_sResourceDescription.IsEmpty())
    m_pResourceToLoad->SetResourceDescription(m_LoaderData.m_sResourceDescription);

  m_pResourceToLoad->CallUpdateContent(m_LoaderData.m_pDataStream);

  if (m_pResourceToLoad->m_uiQualityLevelsLoadable > 0)
  {
    // if the resource can have more details loaded, put it into the preload queue right away again
    xiiResourceManager::PreloadResource(m_pResourceToLoad);
  }

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  XII_ASSERT_DEV(m_pResourceToLoad->GetLoadingState() != xiiResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    xiiResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    m_pResourceToLoad->UpdateMemoryUsage(MemUsage);

    XII_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID());
    XII_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    XII_LOCK(xiiResourceManager::s_ResourceMutex);
    XII_ASSERT_DEV(xiiResourceManager::IsQueuedForLoading(m_pResourceToLoad), "Multi-threaded access detected");
    m_pResourceToLoad->m_Flags.Remove(xiiResourceFlags::IsQueuedForLoading);
    m_pResourceToLoad->m_LastAcquire = xiiResourceManager::GetLastFrameUpdate();
  }

  m_pLoader         = nullptr;
  m_pResourceToLoad = nullptr;
}

XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_WorkerTasks);
