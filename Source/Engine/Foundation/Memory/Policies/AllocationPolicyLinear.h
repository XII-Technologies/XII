/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/SmallArray.h>

/// This policy implements a linear allocator that can only grow and at some point all allocations gets reset at once.
///
/// For debugging purposes, the policy can also overwrite all freed memory with 0xCDCDCDCD to make it easier to find use-after-free situations.
///
/// \see xiiAllocatorWithPolicy
template <bool OverwriteMemoryOnReset = false>
class xiiAllocationPolicyLinear
{
public:
  enum
  {
    Alignment = 16
  };

  XII_FORCE_INLINE xiiAllocationPolicyLinear(xiiAllocator* pParent) :
    m_pParent(pParent), m_uiNextBucketSize(4096)
  {
  }

  XII_FORCE_INLINE ~xiiAllocationPolicyLinear()
  {
    XII_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && (m_Buckets.IsEmpty() || m_Buckets[m_uiCurrentBucketIndex].GetPtr() == m_pNextAllocation), "There is still something allocated!");

    for (auto& bucket : m_Buckets)
    {
      m_pParent->Deallocate(bucket.GetPtr());
    }
  }

  /// Sets the size of the next bucket to allocate. This can be used to prevent an excessive number of buckets if the required total allocation size is known in advance.
  XII_FORCE_INLINE void SetNextBucketSize(xiiUInt32 uiSize)
  {
    m_uiNextBucketSize = uiSize;
  }

  XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_IGNORE_UNUSED(uiAlign);
    XII_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((xiiUInt32)uiAlign));
    uiSize = xiiMemoryUtils::AlignSize(uiSize, (size_t)Alignment);

    bool bFoundBucket = !m_Buckets.IsEmpty() && m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr();

    if (!bFoundBucket)
    {
      // Check if there is an empty bucket that fits the allocation.
      for (xiiUInt32 i = m_uiCurrentBucketIndex + 1; i < m_Buckets.GetCount(); ++i)
      {
        auto& testBucket = m_Buckets[i];
        if (uiSize <= testBucket.GetCount())
        {
          m_uiCurrentBucketIndex = i;
          m_pNextAllocation      = testBucket.GetPtr();
          bFoundBucket           = true;
          break;
        }
      }
    }

    if (!bFoundBucket)
    {
      while (uiSize > m_uiNextBucketSize)
      {
        XII_ASSERT_DEBUG(m_uiNextBucketSize > 0, "");

        m_uiNextBucketSize *= 2;
      }

      m_uiCurrentBucketIndex = m_Buckets.GetCount();

      auto newBucket = xiiArrayPtr<xiiUInt8>(static_cast<xiiUInt8*>(m_pParent->Allocate(m_uiNextBucketSize, Alignment)), m_uiNextBucketSize);
      m_Buckets.PushBack(newBucket);

      m_pNextAllocation = newBucket.GetPtr();

      m_uiNextBucketSize *= 2;
    }

    XII_ASSERT_DEBUG(m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr(), "");

    xiiUInt8* pPtr = m_pNextAllocation;
    m_pNextAllocation += uiSize;
    return pPtr;
  }

  XII_FORCE_INLINE void Deallocate(void* pPtr)
  {
    XII_IGNORE_UNUSED(pPtr);
    // Individual deallocation is not supported by this allocator.
  }

  XII_FORCE_INLINE void Reset()
  {
    m_uiCurrentBucketIndex = 0;
    m_pNextAllocation      = !m_Buckets.IsEmpty() ? m_Buckets[0].GetPtr() : nullptr;

    if constexpr (OverwriteMemoryOnReset)
    {
      for (auto& bucket : m_Buckets)
      {
        xiiMemoryUtils::PatternFill(bucket.GetPtr(), 0xCD, bucket.GetCount());
      }
    }
  }

  XII_FORCE_INLINE void FillStats(xiiAllocator::Stats& ref_stats)
  {
    ref_stats.m_uiAllocationCount   = m_Buckets.GetCount();
    for (auto& bucket : m_Buckets)
    {
      ref_stats.m_uiAllocationSize += bucket.GetCount();
    }
  }

  XII_ALWAYS_INLINE xiiAllocator* GetParent() const { return m_pParent; }

private:
  xiiAllocator* m_pParent = nullptr;

  xiiUInt32 m_uiCurrentBucketIndex = 0;
  xiiUInt32 m_uiNextBucketSize     = 0;

  xiiUInt8* m_pNextAllocation = nullptr;

  xiiSmallArray<xiiArrayPtr<xiiUInt8>, 4> m_Buckets;
};
