#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace xiiMemoryPolicies
{
  class xiiGuardedAllocation
  {
  public:
    xiiGuardedAllocation(xiiAllocatorBase* pParent);
    XII_ALWAYS_INLINE ~xiiGuardedAllocation() {}

    void* Allocate(size_t uiSize, size_t uiAlign);
    void  Deallocate(void* ptr);

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return nullptr; }

  private:
    xiiMutex m_Mutex;

    xiiUInt32 m_uiPageSize;

    xiiStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
  };
} // namespace xiiMemoryPolicies
