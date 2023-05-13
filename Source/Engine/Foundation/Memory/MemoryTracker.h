#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>

struct xiiMemoryTrackingFlags
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    None,
    RegisterAllocator = XII_BIT(0),        ///< Register the allocator with the memory tracker. If EnableAllocationTracking is not set as well it is up to the
                                           ///< allocator implementation whether it collects usable stats or not.
    EnableAllocationTracking = XII_BIT(1), ///< Enable tracking of individual allocations
    EnableStackTrace         = XII_BIT(2), ///< Enable stack traces for each allocation

    All = RegisterAllocator | EnableAllocationTracking | EnableStackTrace,

    Default = 0
#if XII_ENABLED(XII_USE_ALLOCATION_TRACKING)
      | RegisterAllocator | EnableAllocationTracking
#endif
#if XII_ENABLED(XII_USE_ALLOCATION_STACK_TRACING)
      | EnableStackTrace
#endif
  };

  struct Bits
  {
    StorageType RegisterAllocator : 1;
    StorageType EnableAllocationTracking : 1;
    StorageType EnableStackTrace : 1;
  };
};

// XII_DECLARE_FLAGS_OPERATORS(xiiMemoryTrackingFlags);

#define XII_STATIC_ALLOCATOR_NAME "Statics"

/// \brief Memory tracker which keeps track of all allocations and constructions
class XII_FOUNDATION_DLL xiiMemoryTracker
{
public:
  struct AllocationInfo
  {
    XII_DECLARE_POD_TYPE();

    XII_FORCE_INLINE AllocationInfo() :
      m_pStackTrace(nullptr), m_uiSize(0), m_uiAlignment(0), m_uiStackTraceLength(0)
    {
    }

    void**    m_pStackTrace;
    size_t    m_uiSize;
    xiiUInt16 m_uiAlignment;
    xiiUInt16 m_uiStackTraceLength;

    XII_ALWAYS_INLINE const xiiArrayPtr<void*> GetStackTrace() const { return xiiArrayPtr<void*>(m_pStackTrace, (xiiUInt32)m_uiStackTraceLength); }

    XII_ALWAYS_INLINE xiiArrayPtr<void*> GetStackTrace() { return xiiArrayPtr<void*>(m_pStackTrace, (xiiUInt32)m_uiStackTraceLength); }

    XII_FORCE_INLINE void SetStackTrace(xiiArrayPtr<void*> stackTrace)
    {
      m_pStackTrace = stackTrace.GetPtr();
      XII_ASSERT_DEV(stackTrace.GetCount() < 0xFFFF, "stack trace too long");
      m_uiStackTraceLength = (xiiUInt16)stackTrace.GetCount();
    }
  };

  class XII_FOUNDATION_DLL Iterator
  {
  public:
    ~Iterator();

    xiiAllocatorId                 Id() const;
    const char*                    Name() const;
    xiiAllocatorId                 ParentId() const;
    const xiiAllocatorBase::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    XII_ALWAYS_INLINE void operator++() { Next(); }

  private:
    friend class xiiMemoryTracker;

    XII_ALWAYS_INLINE Iterator(void* pData) :
      m_pData(pData)
    {
    }

    void* m_pData;
  };

  static xiiAllocatorId RegisterAllocator(const char* szName, xiiBitflags<xiiMemoryTrackingFlags> flags, xiiAllocatorId parentId);
  static void           DeregisterAllocator(xiiAllocatorId allocatorId);

  static void AddAllocation(
    xiiAllocatorId                      allocatorId,
    xiiBitflags<xiiMemoryTrackingFlags> flags,
    const void*                         ptr,
    size_t                              uiSize,
    size_t                              uiAlign,
    xiiTime                             allocationTime);
  static void RemoveAllocation(xiiAllocatorId allocatorId, const void* ptr);
  static void RemoveAllAllocations(xiiAllocatorId allocatorId);
  static void SetAllocatorStats(xiiAllocatorId allocatorId, const xiiAllocatorBase::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static const char*                    GetAllocatorName(xiiAllocatorId allocatorId);
  static const xiiAllocatorBase::Stats& GetAllocatorStats(xiiAllocatorId allocatorId);
  static xiiAllocatorId                 GetAllocatorParentId(xiiAllocatorId allocatorId);
  static const AllocationInfo&          GetAllocationInfo(xiiAllocatorId allocatorId, const void* ptr);

  static void DumpMemoryLeaks();

  static Iterator GetIterator();
};
