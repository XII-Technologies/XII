/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
#  include <Foundation/System/StackTracer.h>
#endif

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiResource, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CORE_DLL void IncreaseResourceRefCount(xiiResource* pResource, const void* pOwner)
{
#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
  {
    XII_LOCK(pResource->m_HandleStackTraceMutex);

    auto& info = pResource->m_HandleStackTraces[pOwner];

    xiiArrayPtr<void*> ptr(info.m_Ptrs);

    info.m_uiNumPtrs = xiiStackTracer::GetStackTrace(ptr);
  }
#else
  XII_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Increment();
}

XII_CORE_DLL void DecreaseResourceRefCount(xiiResource* pResource, const void* pOwner)
{
#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
  {
    XII_LOCK(pResource->m_HandleStackTraceMutex);

    if (!pResource->m_HandleStackTraces.Remove(pOwner, nullptr))
    {
      XII_REPORT_FAILURE("No associated stack-trace!");
    }
  }
#else
  XII_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Decrement();
}

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
XII_CORE_DLL void MigrateResourceRefCount(xiiResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  XII_LOCK(pResource->m_HandleStackTraceMutex);

  // allocate / resize the hash-table first to ensure the iterator stays valid
  auto& newInfo = pResource->m_HandleStackTraces[pNewOwner];

  auto it = pResource->m_HandleStackTraces.Find(pOldOwner);
  if (!it.IsValid())
  {
    XII_REPORT_FAILURE("No associated stack-trace!");
  }
  else
  {
    newInfo = it.Value();
    pResource->m_HandleStackTraces.Remove(it);
  }
}
#endif

xiiResource::~xiiResource()
{
  XII_ASSERT_DEV(!xiiResourceManager::IsQueuedForLoading(this), "Cannot deallocate a resource while it is still qeued for loading");
}

xiiResource::xiiResource(DoUpdate resourceUpdateThread, xiiUInt8 uiQualityLevelsLoadable)
{
  m_Flags.AddOrRemove(xiiResourceFlags::UpdateOnMainThread, resourceUpdateThread == DoUpdate::OnMainThread);

  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
}

#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)
static void LogStackTrace(const char* szText)
{
  xiiLog::Info(szText);
};
#endif

void xiiResource::PrintHandleStackTraces()
{
#if XII_ENABLED(XII_RESOURCEHANDLE_STACK_TRACES)

  XII_LOCK(m_HandleStackTraceMutex);

  XII_LOG_BLOCK("Resource Handle Stack Traces");

  for (auto& it : m_HandleStackTraces)
  {
    XII_LOG_BLOCK("Handle Trace");

    xiiStackTracer::ResolveStackTrace(xiiArrayPtr<void*>(it.Value().m_Ptrs, it.Value().m_uiNumPtrs), LogStackTrace);
  }

#else

  xiiLog::Warning("Compile with XII_RESOURCEHANDLE_STACK_TRACES set to XII_ON to enable support for resource handle stack traces.");

#endif
}

void xiiResource::SetResourceDescription(xiiStringView sDescription)
{
  m_sResourceDescription = sDescription;
}

void xiiResource::SetUniqueID(xiiStringView sUniqueID, bool bIsReloadable)
{
  m_sUniqueID      = sUniqueID;
  m_uiUniqueIDHash = xiiHashingUtils::StringHash(sUniqueID);
  SetIsReloadable(bIsReloadable);

  xiiResourceEvent e;
  e.m_pResource = this;
  e.m_Type      = xiiResourceEvent::Type::ResourceCreated;
  xiiResourceManager::BroadcastResourceEvent(e);
}

void xiiResource::CallUnloadData(Unload WhatToUnload)
{
  XII_LOG_BLOCK("xiiResource::UnloadData", GetResourceID());

  xiiResourceEvent e;
  e.m_pResource = this;
  e.m_Type      = xiiResourceEvent::Type::ResourceContentUnloading;
  xiiResourceManager::BroadcastResourceEvent(e);

  xiiResourceLoadDescription ld = UnloadData(WhatToUnload);

  XII_ASSERT_DEV(ld.m_State != xiiResourceState::Invalid, "UnloadData() did not return a valid resource load state");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsDiscardable correctly");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState               = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable    = ld.m_uiQualityLevelsLoadable;
}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
thread_local const xiiResource* g_pCurrentlyUpdatingContent = nullptr;

const xiiResource* xiiResource::GetCurrentlyUpdatingContent()
{
  return g_pCurrentlyUpdatingContent;
}
#endif

void xiiResource::CallUpdateContent(xiiStreamReader* pStream)
{
  XII_PROFILE_SCOPE("CallUpdateContent");

  XII_LOG_BLOCK("xiiResource::UpdateContent", GetResourceDescription());

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  const xiiResource* pPreviouslyUpdatingContent = g_pCurrentlyUpdatingContent;
  g_pCurrentlyUpdatingContent                   = this;
  xiiResourceLoadDescription ld                 = UpdateContent(pStream);
  g_pCurrentlyUpdatingContent                   = pPreviouslyUpdatingContent;
#else
  xiiResourceLoadDescription ld = UpdateContent(pStream);
#endif

  XII_ASSERT_DEV(ld.m_State != xiiResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  if (ld.m_State == xiiResourceState::LoadedResourceMissing)
  {
    ReportResourceIsMissing();
  }

  IncResourceChangeCounter();

  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable    = ld.m_uiQualityLevelsLoadable;
  m_LoadingState               = ld.m_State;

  xiiResourceEvent e;
  e.m_pResource = this;
  e.m_Type      = xiiResourceEvent::Type::ResourceContentUpdated;
  xiiResourceManager::BroadcastResourceEvent(e);

  xiiLog::Debug("Updated {0} - '{1}'", GetDynamicRTTI()->GetTypeName(), xiiArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

float xiiResource::GetLoadingPriority(xiiTime now) const
{
  if (m_Priority == xiiResourcePriority::Critical)
    return 0.0f;

  // low priority values mean it gets loaded earlier
  float fPriority = static_cast<float>(m_Priority) * 10.0f;

  if (GetLoadingState() == xiiResourceState::Loaded)
  {
    // already loaded -> more penalty
    fPriority += 30.0f;

    // the more it could discard, the less important it is to load more of it
    fPriority += GetNumQualityLevelsDiscardable() * 10.0f;
  }
  else
  {
    const xiiBitflags<xiiResourceFlags> flags = GetBaseResourceFlags();

    if (flags.IsAnySet(xiiResourceFlags::ResourceHasFallback))
    {
      // if the resource has a very specific fallback, it is least important to be get loaded
      fPriority += 20.0f;
    }
    else if (flags.IsAnySet(xiiResourceFlags::ResourceHasTypeFallback))
    {
      // if it has at least a type fallback, it is less important to get loaded
      fPriority += 10.0f;
    }
  }

  // everything acquired in the last N seconds gets a higher priority by getting the lowest penalty
  const float secondsSinceAcquire = (float)(now - GetLastAcquireTime()).GetSeconds();
  const float fTimePriority       = xiiMath::Min(10.0f, secondsSinceAcquire);

  return fPriority + fTimePriority;
}

void xiiResource::SetPriority(xiiResourcePriority priority)
{
  if (m_Priority == priority)
    return;

  m_Priority = priority;

  xiiResourceEvent e;
  e.m_pResource = this;
  e.m_Type      = xiiResourceEvent::Type::ResourcePriorityChanged;
  xiiResourceManager::BroadcastResourceEvent(e);
}

xiiResourceTypeLoader* xiiResource::GetDefaultResourceTypeLoader() const
{
  return xiiResourceManager::GetDefaultResourceLoader();
}

void xiiResource::ReportResourceIsMissing()
{
  xiiLog::SeriousWarning("Missing Resource of Type '{2}': '{0}' ('{1}')", xiiArgSensitive(GetResourceID(), "ResourceID"), xiiArgSensitive(m_sResourceDescription, "ResourceDesc"), GetDynamicRTTI()->GetTypeName());
}

void xiiResource::VerifyAfterCreateResource(const xiiResourceLoadDescription& ld)
{
  XII_ASSERT_DEV(ld.m_State != xiiResourceState::Invalid, "CreateResource() did not return a valid resource load state");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsDiscardable correctly");
  XII_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsLoadable correctly");

  IncResourceChangeCounter();

  m_LoadingState               = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable    = ld.m_uiQualityLevelsLoadable;

  // Update Memory Usage
  {
    xiiResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    UpdateMemoryUsage(MemUsage);

    XII_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", GetResourceID());
    XII_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", GetResourceID());

    m_MemoryUsage = MemUsage;
  }

  xiiResourceEvent e;
  e.m_pResource = this;
  e.m_Type      = xiiResourceEvent::Type::ResourceContentUpdated;
  xiiResourceManager::BroadcastResourceEvent(e);

  xiiLog::Debug("Created {0} - '{1}' ", GetDynamicRTTI()->GetTypeName(), xiiArgSensitive(GetResourceIdOrDescription(), "ResourceDesc"));
}

XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);
