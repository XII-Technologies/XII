#pragma once

#include <Foundation/Memory/Allocator.h>

/// \brief Stack-based allocator for temporary allocations.
///
/// This allocator is designed for short-lived allocations that ideally follow a LIFO pattern but can also handle out-of-order deallocations.
class XII_FOUNDATION_DLL xiiTempAllocator
{
public:
  XII_ALWAYS_INLINE static xiiAllocator* Get() { return s_pAllocator; }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TempAllocator);

  static void Startup();
  static void Shutdown();

  static xiiAllocator* s_pAllocator;
};

/// \brief Wrapper for the allocator that is used for temporary allocations.
struct xiiTempAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocator* GetAllocator() { return xiiTempAllocator::Get(); }
};
