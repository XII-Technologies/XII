/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Bitflags.h>

enum class xiiAllocatorTrackingMode : xiiUInt32
{
  DoNotTrack,                    ///< The allocator doesn't track anything. Use this for best performance.
  Basics,                        ///< The allocator will be known to the system, so it can show up in debugging tools, but barely anything more.
  AllocationStats,               ///< The allocator keeps track of how many allocations and deallocations it did and how large its memory usage is.
  AllocationStatsIgnoreLeaks,    ///< Same as AllocationStats, but any remaining allocations at shutdown are not reported as leaks.
  AllocationStatsAndStacktraces, ///< The allocator will record stack traces for each allocation, which can be used to find memory leaks.

  Default = XII_ALLOC_TRACKING_DEFAULT,
};

/// Global memory tracking system for debugging, profiling, and leak detection.
///
/// This singleton provides comprehensive memory allocation tracking across all allocators
/// in the system. It supports different tracking modes ranging from basic statistics to
/// full stack trace recording for every allocation.
class XII_FOUNDATION_DLL xiiMemoryTracker
{
public:
  struct AllocationInfo
  {
    XII_DECLARE_POD_TYPE();

    XII_FORCE_INLINE AllocationInfo() = default;

    void**    m_pStackTrace        = nullptr;
    size_t    m_uiSize             = 0;
    xiiUInt16 m_uiAlignment        = 0;
    xiiUInt16 m_uiStackTraceLength = 0;

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

    xiiAllocatorId             Id() const;
    xiiStringView              Name() const;
    xiiAllocatorId             ParentId() const;
    const xiiAllocator::Stats& Stats() const;

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

  static xiiAllocatorId RegisterAllocator(xiiStringView sName, xiiAllocatorTrackingMode mode, xiiAllocatorId parentId);
  static void           DeregisterAllocator(xiiAllocatorId allocatorId);

  static void AddAllocation(xiiAllocatorId allocatorId, xiiAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, xiiTime allocationTime);
  static void RemoveAllocation(xiiAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(xiiAllocatorId allocatorId);
  static void SetAllocatorStats(xiiAllocatorId allocatorId, const xiiAllocator::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static xiiStringView              GetAllocatorName(xiiAllocatorId allocatorId);
  static const xiiAllocator::Stats& GetAllocatorStats(xiiAllocatorId allocatorId);
  static xiiAllocatorId             GetAllocatorParentId(xiiAllocatorId allocatorId);
  static const AllocationInfo&      GetAllocationInfo(xiiAllocatorId allocatorId, const void* pPtr);

  static Iterator GetIterator();

  /// Callback for printing strings.
  using PrintFunc = void (*)(const char* szLine);

  /// Reports back information about all currently known root memory leaks.
  ///
  /// Returns the number of found memory leaks.
  static xiiUInt32 PrintMemoryLeaks(PrintFunc printfunc);

  /// Prints the known memory leaks to xiiLog and triggers an assert if there are any.
  ///
  /// This is useful to call at the end of an application, to get a debug breakpoint in case of memory leaks.
  static void DumpMemoryLeaks();
};
