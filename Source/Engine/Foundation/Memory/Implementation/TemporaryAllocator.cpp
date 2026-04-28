/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocationPolicyStack.h>
#include <Foundation/Memory/TemporaryAllocator.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TempAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    xiiTemporaryAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiTemporaryAllocator::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiAllocator* xiiTemporaryAllocator::s_pAllocator;

// static
void xiiTemporaryAllocator::Startup()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnFree = true;
#else
  static constexpr bool OverwriteMemoryOnFree = false;
#endif
  using StackAllocatorType = xiiAllocatorWithPolicy<xiiAllocationPolicyStack<OverwriteMemoryOnFree>, xiiAllocatorTrackingMode::Basics>;

  s_pAllocator = XII_DEFAULT_NEW(StackAllocatorType, "TemporaryAllocator", xiiFoundation::GetAlignedAllocator());
}

// static
void xiiTemporaryAllocator::Shutdown()
{
  XII_DEFAULT_DELETE(s_pAllocator);
}


XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_TempAllocator);
