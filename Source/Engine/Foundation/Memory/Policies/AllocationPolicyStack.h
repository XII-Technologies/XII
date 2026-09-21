/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// This policy implements a stack allocator. It is designed for scenarios where you have a lot of short-lived allocations that are freed in a LIFO order,
/// but it also supports freeing in arbitrary order (at the cost of some fragmentation).
///
/// For debugging purposes, the policy can also overwrite all freed memory with 0xCDCDCDCD to make it easier to find use-after-free situations.
///
/// \see xiiAllocatorWithPolicy
template <bool OverwriteMemoryOnFree = false>
class xiiAllocationPolicyStack
{
public:
  enum
  {
    Alignment = 16
  };

  XII_FORCE_INLINE xiiAllocationPolicyStack(xiiAllocator* pParent) :
    m_pParent(pParent), m_uiMaxAllocSizeLog2(20) // 1024 * 1024 = 2^20
  {
  }

  XII_FORCE_INLINE ~xiiAllocationPolicyStack()
  {
    XII_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && m_uiCurrentBucketOffset == 0 && m_Allocations.IsEmpty(), "There is still something allocated!");

    for (auto& bucket : m_Buckets)
    {
      m_pParent->Deallocate(bucket.GetPtr());
    }
  }

  /// Sets the maximum allocation size that can be handled by this stack allocator. Allocations larger than this size will be directly allocated from the parent allocator.
  XII_FORCE_INLINE void SetMaxAllocationSize(xiiUInt32 uiSize)
  {
    XII_ASSERT_DEV(m_Allocations.IsEmpty(), "Cannot change max allocation size while there are active allocations!");

    m_uiMaxAllocSizeLog2 = xiiMath::Log2i(uiSize);
  }

  XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    // For allocations larger than the max allocation size, we directly allocate from the parent allocator.
    if (uiSize > GetMaxAllocationSize())
    {
      return m_pParent->Allocate(uiSize, uiAlign);
    }

    XII_LOCK(m_Mutex);

    XII_IGNORE_UNUSED(uiAlign);
    XII_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((xiiUInt32)uiAlign));
    const xiiUInt32 uiSize32 = static_cast<xiiUInt32>(xiiMemoryUtils::AlignSize(uiSize, (size_t)Alignment));

    auto bucket = GetOrCreateBucket(uiSize32);
    XII_ASSERT_DEBUG(m_uiCurrentBucketOffset + uiSize <= bucket.GetCount(), "");

    xiiUInt8* pPtr = bucket.GetPtr() + m_uiCurrentBucketOffset;

    const xiiUInt32 uiGlobalOffset = (m_uiCurrentBucketIndex << m_uiMaxAllocSizeLog2) + m_uiCurrentBucketOffset;
    m_Allocations.PushBack({pPtr, uiGlobalOffset, uiSize32});

    m_uiCurrentBucketOffset += uiSize32;

    return pPtr;
  }

  XII_FORCE_INLINE void Deallocate(void* pPtr)
  {
    XII_LOCK(m_Mutex);

    if (m_Allocations.IsEmpty())
    {
      m_pParent->Deallocate(pPtr);
      return;
    }

    auto& lastAllocation = m_Allocations.PeekBack();
    if (lastAllocation.m_Ptr == pPtr)
    {
      const xiiUInt32 uiOffsetMask = xiiMath::Bitmask_LowN<xiiUInt32>(m_uiMaxAllocSizeLog2);

      m_uiCurrentBucketIndex  = static_cast<xiiUInt16>(lastAllocation.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
      m_uiCurrentBucketOffset = lastAllocation.m_uiGlobalOffset & uiOffsetMask;

      if constexpr (OverwriteMemoryOnFree)
      {
        xiiMemoryUtils::PatternFill(static_cast<xiiUInt8*>(pPtr), 0xCD, lastAllocation.m_uiSize);
      }

      m_Allocations.PopBack();

      while (m_Allocations.IsEmpty() == false)
      {
        auto& alloc = m_Allocations.PeekBack();
        if (alloc.m_Ptr != nullptr)
          return;

        m_uiCurrentBucketIndex  = static_cast<xiiUInt16>(alloc.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
        m_uiCurrentBucketOffset = alloc.m_uiGlobalOffset & uiOffsetMask;

        m_Allocations.PopBack();
      }

      return;
    }
    else
    {
      for (xiiUInt32 i = m_Allocations.GetCount() - 1; i > 0; --i)
      {
        auto& alloc = m_Allocations[i - 1];
        if (alloc.m_Ptr == pPtr)
        {
          if constexpr (OverwriteMemoryOnFree)
          {
            xiiMemoryUtils::PatternFill(static_cast<xiiUInt8*>(pPtr), 0xCD, alloc.m_uiSize);
          }

          alloc.m_Ptr = nullptr;
          return;
        }
      }
    }

    // Allocation not found, this means it was a large allocation that was directly allocated from the parent allocator.
    m_pParent->Deallocate(pPtr);
  }

  XII_ALWAYS_INLINE xiiAllocator* GetParent() const { return m_pParent; }

private:
  xiiUInt32 GetMaxAllocationSize() const { return 1u << m_uiMaxAllocSizeLog2; }

  xiiArrayPtr<xiiUInt8> GetOrCreateBucket(xiiUInt32 uiRequestedSize)
  {
    const xiiUInt32 uiMaxAllocSize         = GetMaxAllocationSize();
    const bool      bFitsIntoCurrentBucket = !m_Buckets.IsEmpty() && m_uiCurrentBucketOffset + uiRequestedSize <= uiMaxAllocSize;
    if (!bFitsIntoCurrentBucket)
    {
      m_uiCurrentBucketIndex = static_cast<xiiUInt16>(m_Buckets.GetCount());

      auto newBucket = xiiMakeArrayPtr(static_cast<xiiUInt8*>(m_pParent->Allocate(uiMaxAllocSize, Alignment)), uiMaxAllocSize);
      m_Buckets.PushBack(newBucket);

      m_uiCurrentBucketOffset = 0;
    }

    return m_Buckets[m_uiCurrentBucketIndex];
  }

  xiiAllocator* m_pParent = nullptr;

  xiiMutex m_Mutex;

  xiiUInt16 m_uiMaxAllocSizeLog2    = 0;
  xiiUInt16 m_uiCurrentBucketIndex  = 0;
  xiiUInt32 m_uiCurrentBucketOffset = 0;

  xiiSmallArray<xiiArrayPtr<xiiUInt8>, 4> m_Buckets;

  struct AllocationInfo
  {
    XII_DECLARE_POD_TYPE();

    void*     m_Ptr;
    xiiUInt32 m_uiGlobalOffset;
    xiiUInt32 m_uiSize;
  };

  xiiSmallArray<AllocationInfo, 16> m_Allocations;
};
