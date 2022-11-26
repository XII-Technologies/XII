#pragma once

#include <Foundation/Basics.h>

namespace xiiMemoryPolicies
{
  /// \brief This Allocation policy redirects all operations to its parent.
  ///
  /// \note Note that the stats are taken on the proxy as well as on the parent.
  ///
  /// \see xiiAllocator
  class xiiProxyAllocation
  {
  public:
    XII_FORCE_INLINE xiiProxyAllocation(xiiAllocatorBase* pParent) :
      m_pParent(pParent)
    {
      XII_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
    }

    XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign) { return m_pParent->Allocate(uiSize, uiAlign); }

    XII_FORCE_INLINE void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      return m_pParent->Reallocate(ptr, uiCurrentSize, uiNewSize, uiAlign);
    }

    XII_FORCE_INLINE void Deallocate(void* ptr) { m_pParent->Deallocate(ptr); }

    XII_FORCE_INLINE size_t AllocatedSize(const void* ptr) { return m_pParent->AllocatedSize(ptr); }

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return m_pParent; }

  private:
    xiiAllocatorBase* m_pParent;
  };
} // namespace xiiMemoryPolicies
