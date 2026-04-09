#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

class xiiAllocationPolicyGuarding
{
public:
  xiiAllocationPolicyGuarding(xiiAllocator* pParent);
  XII_ALWAYS_INLINE ~xiiAllocationPolicyGuarding() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void  Deallocate(void* pPtr);

  XII_ALWAYS_INLINE xiiAllocator* GetParent() const { return nullptr; }

private:
  xiiMutex m_Mutex;

  xiiUInt32 m_uiPageSize;

  xiiStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
};
