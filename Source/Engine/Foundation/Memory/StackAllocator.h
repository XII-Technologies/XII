#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/StackAllocation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

template <xiiUInt32 TrackingFlags = xiiMemoryTrackingFlags::Default>
class xiiStackAllocator : public xiiAllocator<xiiMemoryPolicies::xiiStackAllocation, TrackingFlags>
{
public:
  xiiStackAllocator(xiiStringView sName, xiiAllocatorBase* pParent);
  ~xiiStackAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void  Deallocate(void* pPtr) override;

  /// \brief
  ///   Resets the allocator freeing all memory.
  void Reset();

private:
  struct DestructData
  {
    XII_DECLARE_POD_TYPE();

    xiiMemoryUtils::DestructorFunction m_Func;
    void*                              m_Ptr;
  };

  xiiMutex                       m_Mutex;
  xiiDynamicArray<DestructData>  m_DestructData;
  xiiHashTable<void*, xiiUInt32> m_PtrToDestructDataIndexTable;
};

#include <Foundation/Memory/Implementation/StackAllocator_inl.h>
