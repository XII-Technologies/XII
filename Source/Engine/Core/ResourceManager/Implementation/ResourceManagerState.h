#pragma once

#include <Core/CoreInternal.h>
XII_CORE_INTERNAL_HEADER

#include <Core/ResourceManager/ResourceManager.h>

class xiiResourceManagerState
{
private:
  friend class xiiResource;
  friend class xiiResourceManager;
  friend class xiiResourceManagerWorkerDataLoad;
  friend class xiiResourceManagerWorkerUpdateContent;
  friend class xiiResourceHandleReadContext;

  /// \name Events
  ///@{

  xiiEvent<const xiiResourceEvent&, xiiMutex>        m_ResourceEvents;
  xiiEvent<const xiiResourceManagerEvent&, xiiMutex> m_ManagerEvents;

  ///@}
  /// \name Resource Fallbacks
  ///@{

  xiiDynamicArray<xiiResourceManager::ResourceCleanupCB> m_ResourceCleanupCallbacks;

  ///@}
  /// \name Resource Priorities
  ///@{

  xiiMap<const xiiRTTI*, xiiResourcePriority> m_ResourceTypePriorities;

  ///@}

  struct TaskDataUpdateContent
  {
    xiiSharedPtr<xiiResourceManagerWorkerUpdateContent> m_pTask;
    xiiTaskGroupID                                      m_GroupId;
  };

  struct TaskDataDataLoad
  {
    xiiSharedPtr<xiiResourceManagerWorkerDataLoad> m_pTask;
    xiiTaskGroupID                                 m_GroupId;
  };

  bool      m_bTaskNamesInitialized        = false;
  bool      m_bBroadcastExistsEvent        = false;
  xiiUInt32 m_uiForceNoFallbackAcquisition = 0;

  // resources in this queue are waiting for a task to load them
  xiiDeque<xiiResourceManager::LoadingInfo> m_LoadingQueue;

  xiiHashTable<const xiiRTTI*, xiiResourceManager::LoadedResources> m_LoadedResources;

  bool m_bAllowLaunchDataLoadTask = true;
  bool m_bShutdown                = false;

  xiiHybridArray<TaskDataUpdateContent, 24> m_WorkerTasksUpdateContent;
  xiiHybridArray<TaskDataDataLoad, 8>       m_WorkerTasksDataLoad;

  xiiTime   m_LastFrameUpdate;
  xiiUInt32 m_uiLastResourcePriorityUpdateIdx = 0;

  xiiDynamicArray<xiiResource*>                     m_LoadedResourceOfTypeTempContainer;
  xiiHashTable<xiiTempHashedString, const xiiRTTI*> m_ResourcesToUnloadOnMainThread;

  const xiiRTTI*      m_pFreeUnusedLastType = nullptr;
  xiiTempHashedString m_sFreeUnusedLastResourceID;

  // Type Loaders

  xiiMap<const xiiRTTI*, xiiResourceTypeLoader*>            m_ResourceTypeLoader;
  xiiResourceLoaderFromFile                                 m_FileResourceLoader;
  xiiResourceTypeLoader*                                    m_pDefaultResourceLoader = &m_FileResourceLoader;
  xiiMap<xiiResource*, xiiUniquePtr<xiiResourceTypeLoader>> m_CustomLoaders;


  // Override / derived resources

  xiiMap<const xiiRTTI*, xiiHybridArray<xiiResourceManager::DerivedTypeInfo, 4>> m_DerivedTypeInfos;


  // Named resources

  xiiHashTable<xiiTempHashedString, xiiHashedString> m_NamedResources;

  // Asset system interaction

  xiiMap<xiiString, const xiiRTTI*> m_AssetToResourceType;


  // Export mode

  bool      m_bExportMode      = false;
  xiiUInt32 m_uiNextResourceID = 0;

  // Resource Unloading
  xiiTime m_AutoFreeUnusedTimeout   = xiiTime::Zero();
  xiiTime m_AutoFreeUnusedThreshold = xiiTime::Zero();

  xiiMap<const xiiRTTI*, xiiResourceManager::ResourceTypeInfo> m_TypeInfo;
};
