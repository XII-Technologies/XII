#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local xiiAllocatorBase* s_pAllocator = nullptr;

xiiLocalAllocatorWrapper::xiiLocalAllocatorWrapper(xiiAllocatorBase* pAllocator)
{
  s_pAllocator = pAllocator;
}

void xiiLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

xiiAllocatorBase* xiiLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorWrapper);
