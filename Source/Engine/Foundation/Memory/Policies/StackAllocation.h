#pragma once

#include <Foundation/Containers/HybridArray.h>

namespace xiiMemoryPolicies
{
  /// \brief This allocation policy works like a stack. You can only "push" and "pop" allocations
  ///   in the correct order.
  ///
  /// \note It is also possible to free all allocations at once.
  ///
  /// \see xiiAllocator
  class xiiStackAllocation
  {
  public:
    enum
    {
      Alignment = 16
    };

    XII_FORCE_INLINE xiiStackAllocation(xiiAllocatorBase* pParent) :
      m_pParent(pParent), m_uiNextBucketSize(4096)
    {
    }

    XII_FORCE_INLINE ~xiiStackAllocation()
    {
      XII_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && (m_Buckets.IsEmpty() || m_Buckets[m_uiCurrentBucketIndex].GetPtr() == m_pNextAllocation), "There is still something allocated!");

      for (auto& bucket : m_Buckets)
      {
        m_pParent->Deallocate(bucket.GetPtr());
      }
    }

    XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      XII_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((xiiUInt32)uiAlign));
      uiSize = xiiMemoryUtils::AlignSize(uiSize, (size_t)Alignment);

      bool bFoundBucket = !m_Buckets.IsEmpty() && m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr();

      if (!bFoundBucket)
      {
        // Check if there is an empty bucket that fits the allocation
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

      xiiUInt8* ptr = m_pNextAllocation;
      m_pNextAllocation += uiSize;
      return ptr;
    }

    XII_FORCE_INLINE void Deallocate(void* pPtr)
    {
      // Individual deallocation is not supported by this allocator
    }

    XII_FORCE_INLINE void Reset()
    {
      m_uiCurrentBucketIndex = 0;
      m_pNextAllocation      = !m_Buckets.IsEmpty() ? m_Buckets[0].GetPtr() : nullptr;
    }

    XII_FORCE_INLINE void FillStats(xiiAllocatorBase::Stats& ref_stats)
    {
      ref_stats.m_uiNumAllocations = m_Buckets.GetCount();
      for (auto& bucket : m_Buckets)
      {
        ref_stats.m_uiAllocationSize += bucket.GetCount();
      }
    }

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return m_pParent; }

  private:
    xiiAllocatorBase* m_pParent = nullptr;

    xiiUInt32 m_uiCurrentBucketIndex = 0;
    xiiUInt32 m_uiNextBucketSize     = 0;

    xiiUInt8* m_pNextAllocation = nullptr;

    xiiHybridArray<xiiArrayPtr<xiiUInt8>, 4> m_Buckets;
  };
} // namespace xiiMemoryPolicies
