#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/CommonAllocators.h>

#if XII_ENABLED(XII_USE_GUARDED_ALLOCATIONS)
using DefaultHeapType        = xiiGuardedAllocator;
using DefaultAlignedHeapType = xiiGuardedAllocator;
using DefaultStaticHeapType  = xiiAllocator<xiiMemoryPolicies::xiiGuardedAllocation, xiiAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#else
using DefaultHeapType        = xiiHeapAllocator;
using DefaultAlignedHeapType = xiiAlignedHeapAllocator;
using DefaultStaticHeapType  = xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation, xiiAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#endif

static constexpr xiiUInt32 HEAP_ALLOCATOR_BUFFER_SIZE    = sizeof(DefaultHeapType);
static constexpr xiiUInt32 ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultAlignedHeapType);

alignas(XII_ALIGNMENT_MINIMUM) static xiiUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
alignas(XII_ALIGNMENT_MINIMUM) static xiiUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

alignas(XII_ALIGNMENT_MINIMUM) static xiiUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool              xiiFoundation::s_bIsInitialized    = false;
xiiAllocatorBase* xiiFoundation::s_pDefaultAllocator = nullptr;
xiiAllocatorBase* xiiFoundation::s_pAlignedAllocator = nullptr;

void xiiFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiMemoryUtils::ReserveLower4GBAddressSpace();
#endif

  if (s_pDefaultAllocator == nullptr)
  {
    s_pDefaultAllocator = new (s_DefaultAllocatorBuffer) DefaultHeapType("DefaultHeap");
  }

  if (s_pAlignedAllocator == nullptr)
  {
    s_pAlignedAllocator = new (s_AlignedAllocatorBuffer) DefaultAlignedHeapType("AlignedHeap");
  }

  s_bIsInitialized = true;
}

#if defined(XII_CUSTOM_STATIC_ALLOCATOR_FUNC)
extern xiiAllocatorBase* XII_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

xiiAllocatorBase* xiiFoundation::GetStaticAllocator()
{
  static xiiAllocatorBase* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(XII_CUSTOM_STATIC_ALLOCATOR_FUNC)

#  if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

#    if XII_ENABLED(XII_PLATFORM_WINDOWS)
    using GetStaticAllocatorFunc = xiiAllocatorBase* (*)();

    HMODULE                hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func        = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, XII_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
#    else
#      error "Customizing static allocator not implemented"
#    endif

#  else
    return XII_CUSTOM_STATIC_ALLOCATOR_FUNC();
#  endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) DefaultStaticHeapType("Statics");
  }

  return pStaticAllocator;
}

XII_STATICLINK_FILE(Foundation, Foundation_Basics_Basics);
