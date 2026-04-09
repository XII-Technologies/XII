#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocationPolicyStack.h>
#include <Foundation/Memory/TemporaryAllocator.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TempAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    xiiTempAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiTempAllocator::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiAllocator* xiiTempAllocator::s_pAllocator;

// static
void xiiTempAllocator::Startup()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnFree = true;
#else
  static constexpr bool OverwriteMemoryOnFree = false;
#endif
  using StackAllocatorType = xiiAllocatorWithPolicy<xiiAllocationPolicyStack<OverwriteMemoryOnFree>, xiiAllocatorTrackingMode::Basics>;

  s_pAllocator = XII_DEFAULT_NEW(StackAllocatorType, "TempAllocator", xiiFoundation::GetAlignedAllocator());
}

// static
void xiiTempAllocator::Shutdown()
{
  XII_DEFAULT_DELETE(s_pAllocator);
}


XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_TempAllocator);
