#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

namespace
{
  // no tracking for the tracker data itself
  typedef xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation, 0> TrackerDataAllocator;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    XII_ALWAYS_INLINE static xiiAllocatorBase* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    XII_ALWAYS_INLINE AllocatorData() {}

    xiiHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    xiiBitflags<xiiMemoryTrackingFlags>              m_Flags;

    xiiAllocatorId m_ParentId;

    xiiAllocatorBase::Stats m_Stats;

    xiiHashTable<const void*, xiiMemoryTracker::AllocationInfo, xiiHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    XII_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    XII_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    xiiMutex m_Mutex;

    typedef xiiIdTable<xiiAllocatorId, AllocatorData, TrackerDataAllocatorWrapper> AllocatorTable;
    AllocatorTable                                                                 m_AllocatorData;

    xiiAllocatorId m_StaticAllocatorId;
  };

  static TrackerData* s_pTrackerData;
  static bool         s_bIsInitialized  = false;
  static bool         s_bIsInitializing = false;

  static void Initialize()
  {
    if (s_bIsInitialized)
      return;

    XII_ASSERT_DEV(!s_bIsInitializing, "MemoryTracker initialization entered recursively");
    s_bIsInitializing = true;

    if (s_pTrackerDataAllocator == nullptr)
    {
      alignas(XII_ALIGNMENT_OF(TrackerDataAllocator)) static xiiUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      XII_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      alignas(XII_ALIGNMENT_OF(TrackerData)) static xiiUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      XII_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized  = true;
    s_bIsInitializing = false;
  }

  static void DumpLeak(const xiiMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char      szBuffer[512];
    xiiUInt64 uiSize = info.m_uiSize;
    xiiStringUtils::snprintf(szBuffer, XII_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);

    xiiLog::Print(szBuffer);

    if (info.GetStackTrace().GetPtr() != nullptr)
    {
      xiiStackTracer::ResolveStackTrace(info.GetStackTrace(), &xiiLog::Print);
    }

    xiiLog::Print("--------------------------------------------------------------------\n\n");
  }
} // namespace

// Iterator
#define CAST_ITER(ptr) static_cast<TrackerData::AllocatorTable::Iterator*>(ptr)

xiiAllocatorId xiiMemoryTracker::Iterator::Id() const
{
  return CAST_ITER(m_pData)->Id();
}

const char* xiiMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName.GetData();
}

xiiAllocatorId xiiMemoryTracker::Iterator::ParentId() const
{
  return CAST_ITER(m_pData)->Value().m_ParentId;
}

const xiiAllocatorBase::Stats& xiiMemoryTracker::Iterator::Stats() const
{
  return CAST_ITER(m_pData)->Value().m_Stats;
}

void xiiMemoryTracker::Iterator::Next()
{
  CAST_ITER(m_pData)->Next();
}

bool xiiMemoryTracker::Iterator::IsValid() const
{
  return CAST_ITER(m_pData)->IsValid();
}

xiiMemoryTracker::Iterator::~Iterator()
{
  auto it = CAST_ITER(m_pData);
  XII_DELETE(s_pTrackerDataAllocator, it);
  m_pData = nullptr;
}


// static
xiiAllocatorId xiiMemoryTracker::RegisterAllocator(const char* szName, xiiBitflags<xiiMemoryTrackingFlags> flags, xiiAllocatorId parentId)
{
  Initialize();

  XII_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName    = szName;
  data.m_Flags    = flags;
  data.m_ParentId = parentId;

  xiiAllocatorId id = s_pTrackerData->m_AllocatorData.Insert(data);

  if (data.m_sName == XII_STATIC_ALLOCATOR_NAME)
  {
    s_pTrackerData->m_StaticAllocatorId = id;
  }

  return id;
}

// static
void xiiMemoryTracker::DeregisterAllocator(xiiAllocatorId allocatorId)
{
  XII_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  xiiUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }

    XII_REPORT_FAILURE("Allocator '{0}' leaked {1} allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

// static
void xiiMemoryTracker::AddAllocation(xiiAllocatorId allocatorId, xiiBitflags<xiiMemoryTrackingFlags> flags, const void* ptr, size_t uiSize, size_t uiAlign, xiiTime allocationTime)
{
  XII_ASSERT_DEV((flags & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0, "Allocation tracking is turned off, but xiiMemoryTracker::AddAllocation() is called anyway.");

  XII_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  xiiArrayPtr<void*> stackTrace;
  if (flags.IsSet(xiiMemoryTrackingFlags::EnableStackTrace))
  {
    void*              pBuffer[64];
    xiiArrayPtr<void*> tempTrace(pBuffer);
    const xiiUInt32    uiNumTraces = xiiStackTracer::GetStackTrace(tempTrace);

    stackTrace = XII_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces);
    xiiMemoryUtils::Copy(stackTrace.GetPtr(), pBuffer, uiNumTraces);
  }

  {
    XII_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
    data.m_Stats.m_uiNumAllocations++;
    data.m_Stats.m_uiAllocationSize += uiSize;
    data.m_Stats.m_uiPerFrameAllocationSize += uiSize;
    data.m_Stats.m_PerFrameAllocationTime += allocationTime;

    XII_ASSERT_DEBUG(data.m_Flags == flags, "Given flags have to be identical to allocator flags");
    auto pInfo           = &data.m_Allocations[ptr];
    pInfo->m_uiSize      = uiSize;
    pInfo->m_uiAlignment = (xiiUInt16)uiAlign;
    pInfo->SetStackTrace(stackTrace);
  }
}

// static
void xiiMemoryTracker::RemoveAllocation(xiiAllocatorId allocatorId, const void* ptr)
{
  xiiArrayPtr<void*> stackTrace;

  {
    XII_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

    AllocationInfo info;
    if (data.m_Allocations.Remove(ptr, &info))
    {
      data.m_Stats.m_uiNumDeallocations++;
      data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

      stackTrace = info.GetStackTrace();
    }
    else
    {
      XII_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", xiiArgP(ptr));
    }
  }

  XII_DELETE_ARRAY(s_pTrackerDataAllocator, stackTrace);
}

// static
void xiiMemoryTracker::RemoveAllAllocations(xiiAllocatorId allocatorId)
{
  XII_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

    XII_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void xiiMemoryTracker::SetAllocatorStats(xiiAllocatorId allocatorId, const xiiAllocatorBase::Stats& stats)
{
  XII_LOCK(*s_pTrackerData);

  s_pTrackerData->m_AllocatorData[allocatorId].m_Stats = stats;
}

// static
void xiiMemoryTracker::ResetPerFrameAllocatorStats()
{
  XII_LOCK(*s_pTrackerData);

  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    AllocatorData& data                     = it.Value();
    data.m_Stats.m_uiPerFrameAllocationSize = 0;
    data.m_Stats.m_PerFrameAllocationTime.SetZero();
  }
}

// static
const char* xiiMemoryTracker::GetAllocatorName(xiiAllocatorId allocatorId)
{
  XII_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName.GetData();
}

// static
const xiiAllocatorBase::Stats& xiiMemoryTracker::GetAllocatorStats(xiiAllocatorId allocatorId)
{
  XII_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

// static
xiiAllocatorId xiiMemoryTracker::GetAllocatorParentId(xiiAllocatorId allocatorId)
{
  XII_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_ParentId;
}

// static
const xiiMemoryTracker::AllocationInfo& xiiMemoryTracker::GetAllocationInfo(xiiAllocatorId allocatorId, const void* ptr)
{
  XII_LOCK(*s_pTrackerData);

  const AllocatorData&  data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(ptr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  XII_REPORT_FAILURE("Could not find info for allocation {0}", xiiArgP(ptr));
  return invalidInfo;
}


struct LeakInfo
{
  XII_DECLARE_POD_TYPE();

  xiiAllocatorId m_AllocatorId;
  size_t         m_uiSize      = 0;
  const void*    m_pParentLeak = nullptr;

  XII_ALWAYS_INLINE bool IsRootLeak() const { return m_pParentLeak == nullptr && m_AllocatorId != s_pTrackerData->m_StaticAllocatorId; }
};

// static
void xiiMemoryTracker::DumpMemoryLeaks()
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return;
  XII_LOCK(*s_pTrackerData);

  static xiiHashTable<const void*, LeakInfo, xiiHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;
  leakTable.Clear();

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize      = it2.Value().m_uiSize;
      leak.m_pParentLeak = nullptr;

      leakTable.Insert(it2.Key(), leak);
    }
  }

  // find dependencies
  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void*     ptr  = it.Key();
    const LeakInfo& leak = it.Value();

    const void* curPtr = ptr;
    const void* endPtr = xiiMemoryUtils::AddByteOffset(ptr, leak.m_uiSize);

    while (curPtr < endPtr)
    {
      const void* testPtr = *reinterpret_cast<const void* const*>(curPtr);

      LeakInfo* dependentLeak = nullptr;
      if (leakTable.TryGetValue(testPtr, dependentLeak))
      {
        dependentLeak->m_pParentLeak = ptr;
      }

      curPtr = xiiMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  xiiUInt64 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void*     ptr  = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.IsRootLeak())
    {
      if (uiNumLeaks == 0)
      {
        xiiLog::Print("\n\n--------------------------------------------------------------------\n"
                      "Memory Leak Report:"
                      "\n--------------------------------------------------------------------\n\n");
      }

      const AllocatorData&             data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];
      xiiMemoryTracker::AllocationInfo info;
      data.m_Allocations.TryGetValue(ptr, info);

      DumpLeak(info, data.m_sName.GetData());

      ++uiNumLeaks;
    }
  }

  if (uiNumLeaks > 0)
  {
    xiiLog::Printf("\n--------------------------------------------------------------------\n"
                   "Found %llu root memory leak(s)."
                   "\n--------------------------------------------------------------------\n\n",
                   uiNumLeaks);

    XII_REPORT_FAILURE("Found {0} root memory leak(s).", uiNumLeaks);
  }
}

// static
xiiMemoryTracker::Iterator xiiMemoryTracker::GetIterator()
{
  auto pInnerIt = XII_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}


XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_MemoryTracker);
