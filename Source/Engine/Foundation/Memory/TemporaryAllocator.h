/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Memory/Allocator.h>

/// Stack-based allocator for temporary allocations.
///
/// This allocator is designed for short-lived allocations that ideally follow a LIFO pattern but can also handle out-of-order deallocations.
class XII_FOUNDATION_DLL xiiTemporaryAllocator
{
public:
  XII_ALWAYS_INLINE static xiiAllocator* Get() { return s_pAllocator; }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TempAllocator);

  static void Startup();
  static void Shutdown();

  static xiiAllocator* s_pAllocator;
};

/// Wrapper for the allocator that is used for temporary allocations.
struct xiiTemporaryAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocator* GetAllocator() { return xiiTemporaryAllocator::Get(); }
};
