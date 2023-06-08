#include <RendererCore/RendererCorePCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

xiiCVarBool cvar_RenderingMultithreading("Rendering.Multithreading", true, xiiCVarFlags::Default, "Enables multi-threaded update and rendering");
xiiCVarBool cvar_RenderingCachingStaticObjects("Rendering.Caching.StaticObjects", true, xiiCVarFlags::Default, "Enables render data caching of static objects");

xiiEvent<xiiView*, xiiMutex> xiiRenderWorld::s_ViewCreatedEvent;
xiiEvent<xiiView*, xiiMutex> xiiRenderWorld::s_ViewDeletedEvent;

xiiEvent<void*>                                 xiiRenderWorld::s_CameraConfigsModifiedEvent;
bool                                            xiiRenderWorld::s_bModifyingCameraConfigs = false;
xiiMap<xiiString, xiiRenderWorld::CameraConfig> xiiRenderWorld::s_CameraConfigs;

xiiEvent<const xiiRenderWorldExtractionEvent&, xiiMutex> xiiRenderWorld::s_ExtractionEvent;
xiiEvent<const xiiRenderWorldRenderEvent&, xiiMutex>     xiiRenderWorld::s_RenderEvent;
xiiUInt64                                                xiiRenderWorld::s_uiFrameCounter;

namespace
{
  static bool        s_bInExtract;
  static xiiThreadID s_RenderingThreadID;

  static xiiMutex                        s_ExtractTasksMutex;
  static xiiDynamicArray<xiiTaskGroupID> s_ExtractTasks;

  static xiiMutex                        s_ViewsMutex;
  static xiiIdTable<xiiViewId, xiiView*> s_Views;

  static xiiDynamicArray<xiiViewHandle> s_MainViews;

  static xiiMutex                  s_ViewsToRenderMutex;
  static xiiDynamicArray<xiiView*> s_ViewsToRender;

  static xiiDynamicArray<xiiSharedPtr<xiiRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    XII_DECLARE_POD_TYPE();

    xiiRenderPipeline* m_pPipeline;
    xiiViewHandle      m_hView;
  };

  static xiiMutex                           s_PipelinesToRebuildMutex;
  static xiiDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;

  static xiiProxyAllocator* s_pCacheAllocator;

  static xiiMutex s_CachedRenderDataMutex;
  using CachedRenderDataPerComponent = xiiHybridArray<const xiiRenderData*, 4>;
  static xiiHashTable<xiiComponentHandle, CachedRenderDataPerComponent> s_CachedRenderData;
  static xiiDynamicArray<const xiiRenderData*>                          s_DeletedRenderData;

  enum
  {
    MaxNumNewCacheEntries = 32
  };

  static bool                       s_bWriteRenderPipelineDgml = false;
  static xiiConsoleFunction<void()> s_ConFunc_WriteRenderPipelineDgml("WriteRenderPipelineDgml", "()", []() { s_bWriteRenderPipelineDgml = true; });
} // namespace

namespace xiiInternal
{
  struct RenderDataCache
  {
    RenderDataCache(xiiAllocatorBase* pAllocator) :
      m_PerObjectCaches(pAllocator)
    {
      for (xiiUInt32 i = 0; i < MaxNumNewCacheEntries; ++i)
      {
        m_NewEntriesPerComponent.PushBack(NewEntryPerComponent(pAllocator));
      }
    }

    struct PerObjectCache
    {
      PerObjectCache() = default;

      PerObjectCache(xiiAllocatorBase* pAllocator) :
        m_Entries(pAllocator)
      {
      }

      xiiHybridArray<RenderDataCacheEntry, 4> m_Entries;
      xiiUInt16                               m_uiVersion = 0;
    };

    xiiDynamicArray<PerObjectCache> m_PerObjectCaches;

    struct NewEntryPerComponent
    {
      NewEntryPerComponent(xiiAllocatorBase* pAllocator) :
        m_Cache(pAllocator)
      {
      }

      xiiGameObjectHandle m_hOwnerObject;
      xiiComponentHandle  m_hOwnerComponent;
      PerObjectCache      m_Cache;
    };

    xiiStaticArray<NewEntryPerComponent, MaxNumNewCacheEntries> m_NewEntriesPerComponent;
    xiiAtomicInteger32                                          m_NewEntriesCount;
  };

#if XII_ENABLED(XII_PLATFORM_64BIT)
  XII_CHECK_AT_COMPILETIME(sizeof(RenderDataCacheEntry) == 16);
#endif
} // namespace xiiInternal

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRenderWorld::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRenderWorld::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiViewHandle xiiRenderWorld::CreateView(const char* szName, xiiView*& out_pView)
{
  xiiView* pView = XII_DEFAULT_NEW(xiiView);

  {
    XII_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(szName);
  pView->InitializePins();

  pView->m_pRenderDataCache = XII_NEW(s_pCacheAllocator, xiiInternal::RenderDataCache, s_pCacheAllocator);

  s_ViewCreatedEvent.Broadcast(pView);

  out_pView = pView;
  return pView->GetHandle();
}

void xiiRenderWorld::DeleteView(const xiiViewHandle& hView)
{
  xiiView* pView = nullptr;

  {
    XII_LOCK(s_ViewsMutex);
    if (!s_Views.Remove(hView, &pView))
      return;
  }

  s_ViewDeletedEvent.Broadcast(pView);

  XII_DELETE(s_pCacheAllocator, pView->m_pRenderDataCache);

  {
    XII_LOCK(s_PipelinesToRebuildMutex);

    for (xiiUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAtAndCopy(i);
      }
    }
  }

  RemoveMainView(hView);

  XII_DEFAULT_DELETE(pView);
}

bool xiiRenderWorld::TryGetView(const xiiViewHandle& hView, xiiView*& out_pView)
{
  XII_LOCK(s_ViewsMutex);
  return s_Views.TryGetValue(hView, out_pView);
}

xiiView* xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::Enum usageHint, xiiCameraUsageHint::Enum alternativeUsageHint /*= xiiCameraUsageHint::None*/, const xiiWorld* pWorld /*= nullptr*/)
{
  XII_LOCK(s_ViewsMutex);

  xiiView* pAlternativeView = nullptr;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView* pView = it.Value();
    if (pWorld != nullptr && pView->GetWorld() != pWorld)
      continue;

    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
    else if (alternativeUsageHint != xiiCameraUsageHint::None && pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = pView;
    }
  }

  return pAlternativeView;
}

void xiiRenderWorld::AddMainView(const xiiViewHandle& hView)
{
  XII_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(hView))
    s_MainViews.PushBack(hView);
}

void xiiRenderWorld::RemoveMainView(const xiiViewHandle& hView)
{
  xiiUInt32 uiIndex = s_MainViews.IndexOf(hView);
  if (uiIndex != xiiInvalidIndex)
  {
    XII_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");
    s_MainViews.RemoveAtAndCopy(uiIndex);
  }
}

void xiiRenderWorld::ClearMainViews()
{
  XII_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

xiiArrayPtr<xiiViewHandle> xiiRenderWorld::GetMainViews()
{
  return s_MainViews;
}

void xiiRenderWorld::CacheRenderData(const xiiView& view, const xiiGameObjectHandle& hOwnerObject, const xiiComponentHandle& hOwnerComponent, xiiUInt16 uiComponentVersion, xiiArrayPtr<xiiInternal::RenderDataCacheEntry> cacheEntries)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    xiiUInt32 uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount;
    if (uiNewEntriesCount >= MaxNumNewCacheEntries)
    {
      return;
    }

    uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount.Increment();
    if (uiNewEntriesCount <= MaxNumNewCacheEntries)
    {
      auto& newEntry               = view.m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntriesCount - 1];
      newEntry.m_hOwnerObject      = hOwnerObject;
      newEntry.m_hOwnerComponent   = hOwnerComponent;
      newEntry.m_Cache.m_Entries   = cacheEntries;
      newEntry.m_Cache.m_uiVersion = uiComponentVersion;
    }
  }
}

void xiiRenderWorld::DeleteAllCachedRenderData()
{
  XII_PROFILE_SCOPE("DeleteAllCachedRenderData");

  XII_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  {
    XII_LOCK(s_ViewsMutex);

    for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
    {
      xiiView* pView = it.Value();
      pView->m_pRenderDataCache->m_PerObjectCaches.Clear();
    }
  }

  {
    XII_LOCK(s_CachedRenderDataMutex);

    for (auto it = s_CachedRenderData.GetIterator(); it.IsValid(); ++it)
    {
      auto& cachedRenderDataPerComponent = it.Value();

      for (auto pCachedRenderData : cachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      cachedRenderDataPerComponent.Clear();
    }
  }
}

void xiiRenderWorld::DeleteCachedRenderData(const xiiGameObjectHandle& hOwnerObject, const xiiComponentHandle& hOwnerComponent)
{
  XII_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(hOwnerObject);

  XII_LOCK(s_CachedRenderDataMutex);

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    s_CachedRenderData.Remove(hOwnerComponent);
  }
}

void xiiRenderWorld::ResetRenderDataCache(xiiView& ref_view)
{
  ref_view.m_pRenderDataCache->m_PerObjectCaches.Clear();
  ref_view.m_pRenderDataCache->m_NewEntriesCount = 0;

  if (ref_view.GetWorld() != nullptr)
  {
    if (ref_view.GetWorld()->GetObjectDeletionEvent().HasEventHandler(&xiiRenderWorld::DeleteCachedRenderDataForObject) == false)
    {
      ref_view.GetWorld()->GetObjectDeletionEvent().AddEventHandler(&xiiRenderWorld::DeleteCachedRenderDataForObject);
    }
  }
}

void xiiRenderWorld::DeleteCachedRenderDataForObject(const xiiGameObject* pOwnerObject)
{
  XII_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(pOwnerObject->GetHandle());

  XII_LOCK(s_CachedRenderDataMutex);

  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    xiiComponentHandle hComponent = pComponent->GetHandle();

    CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
    if (s_CachedRenderData.TryGetValue(hComponent, pCachedRenderDataPerComponent))
    {
      for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      s_CachedRenderData.Remove(hComponent);
    }
  }
}

void xiiRenderWorld::DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pOwnerObject)
{
  DeleteCachedRenderDataForObject(pOwnerObject);

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataForObjectRecursive(it);
  }
}

xiiArrayPtr<const xiiInternal::RenderDataCacheEntry> xiiRenderWorld::GetCachedRenderData(const xiiView& view, const xiiGameObjectHandle& hOwner, xiiUInt16 uiComponentVersion)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    const auto& perObjectCaches = view.m_pRenderDataCache->m_PerObjectCaches;
    xiiUInt32   uiCacheIndex    = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < perObjectCaches.GetCount())
    {
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion == uiComponentVersion)
      {
        return perObjectCache.m_Entries;
      }
    }
  }

  return xiiArrayPtr<const xiiInternal::RenderDataCacheEntry>();
}

void xiiRenderWorld::AddViewToRender(const xiiViewHandle& hView)
{
  xiiView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  {
    XII_LOCK(s_ViewsToRenderMutex);
    XII_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    // make sure the view is put at the end of the array, if it is already there, reorder it
    // this ensures that the views that have been referenced by the last other view, get rendered first
    xiiUInt32 uiIndex = s_ViewsToRender.IndexOf(pView);
    if (uiIndex != xiiInvalidIndex)
    {
      s_ViewsToRender.RemoveAtAndCopy(uiIndex);
      s_ViewsToRender.PushBack(pView);
      return;
    }

    s_ViewsToRender.PushBack(pView);
  }

  if (cvar_RenderingMultithreading)
  {
    xiiTaskGroupID extractTaskID = xiiTaskSystem::StartSingleTask(pView->GetExtractTask(), xiiTaskPriority::EarlyThisFrame);

    {
      XII_LOCK(s_ExtractTasksMutex);
      s_ExtractTasks.PushBack(extractTaskID);
    }
  }
  else
  {
    pView->ExtractData();
  }
}

void xiiRenderWorld::ExtractMainViews()
{
  XII_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  xiiRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type           = xiiRenderWorldExtractionEvent::Type::BeginExtraction;
  extractionEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_ExtractionEvent.Broadcast(extractionEvent);

  if (cvar_RenderingMultithreading)
  {
    s_ExtractTasks.Clear();

    xiiTaskGroupID extractTaskID = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
    s_ExtractTasks.PushBack(extractTaskID);

    {
      XII_LOCK(s_ViewsMutex);

      for (xiiUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        xiiView* pView = nullptr;
        if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
        {
          s_ViewsToRender.PushBack(pView);
          xiiTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
        }
      }
    }

    xiiTaskSystem::StartTaskGroup(extractTaskID);

    {
      XII_PROFILE_SCOPE("Wait for Extraction");

      while (true)
      {
        xiiTaskGroupID taskID;

        {
          XII_LOCK(s_ExtractTasksMutex);
          if (s_ExtractTasks.IsEmpty())
            break;

          taskID = s_ExtractTasks.PeekBack();
          s_ExtractTasks.PopBack();
        }

        xiiTaskSystem::WaitForGroup(taskID);
      }
    }
  }
  else
  {
    for (xiiUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      xiiView* pView = nullptr;
      if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
      {
        s_ViewsToRender.PushBack(pView);
        pView->ExtractData();
      }
    }
  }

  // filter out duplicates and reverse order so that dependent views are rendered first
  {
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForExtraction()];
    filteredRenderPipelines.Clear();

    for (xiiUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      auto& pRenderPipeline = s_ViewsToRender[i]->m_pRenderPipeline;
      if (!filteredRenderPipelines.Contains(pRenderPipeline))
      {
        filteredRenderPipelines.PushBack(pRenderPipeline);
      }
    }

    s_ViewsToRender.Clear();
  }

  extractionEvent.m_Type = xiiRenderWorldExtractionEvent::Type::EndExtraction;
  s_ExtractionEvent.Broadcast(extractionEvent);

  s_bInExtract = false;
}

void xiiRenderWorld::Render(xiiRenderContext* pRenderContext)
{
  XII_PROFILE_SCOPE("xiiRenderWorld::Render");

  xiiRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type           = xiiRenderWorldRenderEvent::Type::BeginRender;
  renderEvent.m_uiFrameCounter = s_uiFrameCounter;
  {
    XII_PROFILE_SCOPE("BeginRender");
    s_RenderEvent.Broadcast(renderEvent);
  }

  if (!cvar_RenderingMultithreading)
  {
    RebuildPipelines();
  }

  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  if (s_bWriteRenderPipelineDgml)
  {
    // Executed via WriteRenderPipelineDgml console command.
    s_bWriteRenderPipelineDgml = false;
    const xiiDateTime dt       = xiiTimestamp::CurrentTimestamp();
    for (xiiUInt32 i = 0; i < filteredRenderPipelines.GetCount(); ++i)
    {
      auto&            pRenderPipeline = filteredRenderPipelines[i];
      xiiStringBuilder sPath(":appdata/Profiling/", xiiApplication::GetApplicationInstance()->GetApplicationName());
      sPath.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}_Pipeline{}_{}.dgml", dt.GetYear(), xiiArgU(dt.GetMonth(), 2, true), xiiArgU(dt.GetDay(), 2, true), xiiArgU(dt.GetHour(), 2, true), xiiArgU(dt.GetMinute(), 2, true), xiiArgU(dt.GetSecond(), 2, true), i, pRenderPipeline->GetViewName().GetData());

      xiiDGMLGraph graph(xiiDGMLGraph::Direction::TopToBottom);
      pRenderPipeline->CreateDgmlGraph(graph);
      if (xiiDGMLGraphWriter::WriteGraphToFile(sPath, graph).Failed())
      {
        xiiLog::Error("Failed to write render pipeline dgml: {}", sPath);
      }
    }
  }

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted soon.
    if (pRenderPipeline->GetRefCount() > 1)
    {
      pRenderPipeline->Render(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }

  filteredRenderPipelines.Clear();

  {
    renderEvent.m_Type = xiiRenderWorldRenderEvent::Type::EndRender;
    XII_PROFILE_SCOPE("EndRender");
    s_RenderEvent.Broadcast(renderEvent);
  }
}

void xiiRenderWorld::BeginFrame()
{
  XII_PROFILE_SCOPE("BeginFrame");

  s_RenderingThreadID = xiiThreadUtils::GetCurrentThreadID();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView* pView = it.Value();
    pView->EnsureUpToDate();
  }

  RebuildPipelines();
}

void xiiRenderWorld::EndFrame()
{
  XII_PROFILE_SCOPE("EndFrame");

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  ClearRenderDataCache();
  UpdateRenderDataCache();

  s_RenderingThreadID = (xiiThreadID)0;
}

bool xiiRenderWorld::GetUseMultithreadedRendering()
{
  return cvar_RenderingMultithreading;
}


bool xiiRenderWorld::IsRenderingThread()
{
  return s_RenderingThreadID == xiiThreadUtils::GetCurrentThreadID();
}

void xiiRenderWorld::DeleteCachedRenderDataInternal(const xiiGameObjectHandle& hOwnerObject)
{
  xiiUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;
  xiiWorld* pWorld       = xiiWorld::GetWorld(hOwnerObject);

  XII_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView* pView = it.Value();
    if (pView->GetWorld() != nullptr && pView->GetWorld() == pWorld)
    {
      auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

      if (uiCacheIndex < perObjectCaches.GetCount())
      {
        perObjectCaches[uiCacheIndex].m_Entries.Clear();
        perObjectCaches[uiCacheIndex].m_uiVersion = 0;
      }
    }
  }
}

void xiiRenderWorld::ClearRenderDataCache()
{
  XII_PROFILE_SCOPE("Clear Render Data Cache");

  for (auto pRenderData : s_DeletedRenderData)
  {
    xiiRenderData* ptr = const_cast<xiiRenderData*>(pRenderData);
    XII_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void xiiRenderWorld::UpdateRenderDataCache()
{
  XII_PROFILE_SCOPE("Update Render Data Cache");

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView*  pView                              = it.Value();
    xiiUInt32 uiNumNewEntries                    = xiiMath::Min<xiiInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

    for (xiiUInt32 uiNewEntryIndex = 0; uiNewEntryIndex < uiNumNewEntries; ++uiNewEntryIndex)
    {
      auto& newEntries = pView->m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntryIndex];
      XII_ASSERT_DEV(!newEntries.m_hOwnerObject.IsInvalidated(), "Implementation error");

      // find or create cached render data
      auto& cachedRenderDataPerComponent = s_CachedRenderData[newEntries.m_hOwnerComponent];

      const xiiUInt32 uiNumCachedRenderData = cachedRenderDataPerComponent.GetCount();
      if (uiNumCachedRenderData == 0) // Nothing cached yet
      {
        cachedRenderDataPerComponent = CachedRenderDataPerComponent(s_pCacheAllocator);
      }

      xiiUInt32 uiCachedRenderDataIndex = 0;
      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (newEntry.m_pRenderData != nullptr)
        {
          if (uiCachedRenderDataIndex >= cachedRenderDataPerComponent.GetCount())
          {
            const xiiRTTI* pRtti   = newEntry.m_pRenderData->GetDynamicRTTI();
            newEntry.m_pRenderData = pRtti->GetAllocator()->Clone<xiiRenderData>(newEntry.m_pRenderData, s_pCacheAllocator);

            cachedRenderDataPerComponent.PushBack(newEntry.m_pRenderData);
          }
          else
          {
            // replace with cached render data
            newEntry.m_pRenderData = cachedRenderDataPerComponent[uiCachedRenderDataIndex];
          }

          ++uiCachedRenderDataIndex;
        }
      }

      // add entry for this view
      const xiiUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      perObjectCaches.EnsureCount(uiCacheIndex + 1);

      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
      {
        perObjectCache.m_Entries.Clear();
        perObjectCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
      }

      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (!perObjectCache.m_Entries.Contains(newEntry))
        {
          perObjectCache.m_Entries.PushBack(newEntry);
        }
      }

      // keep entries sorted, otherwise the logic xiiExtractor::ExtractRenderData doesn't work
      perObjectCache.m_Entries.Sort();
    }
  }
}

// static
void xiiRenderWorld::AddRenderPipelineToRebuild(xiiRenderPipeline* pRenderPipeline, const xiiViewHandle& hView)
{
  XII_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_hView == hView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild       = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_hView     = hView;
}

// static
void xiiRenderWorld::RebuildPipelines()
{
  XII_PROFILE_SCOPE("RebuildPipelines");

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    xiiView* pView = nullptr;
    if (s_Views.TryGetValue(pipelineToRebuild.m_hView, pView))
    {
      if (pipelineToRebuild.m_pPipeline->Rebuild(*pView) == xiiRenderPipeline::PipelineState::RebuildError)
      {
        xiiLog::Error("Failed to rebuild pipeline '{}' for view '{}'", pipelineToRebuild.m_pPipeline->m_sName, pView->GetName());
      }
    }
  }

  s_PipelinesToRebuild.Clear();
}

void xiiRenderWorld::OnEngineStartup()
{
  s_pCacheAllocator = XII_DEFAULT_NEW(xiiProxyAllocator, "Cached Render Data", xiiFoundation::GetDefaultAllocator());

  s_CachedRenderData = xiiHashTable<xiiComponentHandle, CachedRenderDataPerComponent>(s_pCacheAllocator);
}

void xiiRenderWorld::OnEngineShutdown()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  for (auto it : s_CachedRenderData)
  {
    auto& cachedRenderDataPerComponent = it.Value();
    if (cachedRenderDataPerComponent.IsEmpty() == false)
    {
      XII_REPORT_FAILURE("Leaked cached render data of type '{}'", cachedRenderDataPerComponent[0]->GetDynamicRTTI()->GetTypeName());
    }
  }
#endif

  ClearRenderDataCache();

  XII_DEFAULT_DELETE(s_pCacheAllocator);

  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  ClearMainViews();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    xiiView* pView = it.Value();
    XII_DEFAULT_DELETE(pView);
  }

  s_Views.Clear();
}

void xiiRenderWorld::BeginModifyCameraConfigs()
{
  XII_ASSERT_DEBUG(!s_bModifyingCameraConfigs, "Recursive call not allowed.");
  s_bModifyingCameraConfigs = true;
}

void xiiRenderWorld::EndModifyCameraConfigs()
{
  XII_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call xiiRenderWorld::BeginModifyCameraConfigs first");
  s_bModifyingCameraConfigs = false;
  s_CameraConfigsModifiedEvent.Broadcast(nullptr);
}

void xiiRenderWorld::ClearCameraConfigs()
{
  XII_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call xiiRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs.Clear();
}

void xiiRenderWorld::SetCameraConfig(const char* szName, const CameraConfig& config)
{
  XII_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call xiiRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs[szName] = config;
}

const xiiRenderWorld::CameraConfig* xiiRenderWorld::FindCameraConfig(const char* szName)
{
  auto it = s_CameraConfigs.Find(szName);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

XII_STATICLINK_FILE(RendererCore, RendererCore_RenderWorld_Implementation_RenderWorld);
