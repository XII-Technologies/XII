#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local xiiAllocator* s_pAllocator = nullptr;

xiiLocalAllocatorWrapper::xiiLocalAllocatorWrapper(xiiAllocator* pAllocator)
{
  s_pAllocator = pAllocator;
}

void xiiLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

xiiAllocator* xiiLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}
