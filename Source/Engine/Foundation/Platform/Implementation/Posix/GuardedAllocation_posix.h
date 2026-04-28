/// Copyright (c) Theophilus Eriata. All Rights Reserved.

namespace xiiMemoryPolicies
{
  xiiGuardedAllocation::xiiGuardedAllocation(xiiAllocator* pParent)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    XII_IGNORE_UNUSED(m_Mutex);
    XII_IGNORE_UNUSED(m_uiPageSize);
    XII_IGNORE_UNUSED(m_AllocationsToFreeLater);
  }

  void* xiiGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void xiiGuardedAllocation::Deallocate(void* ptr) { XII_ASSERT_NOT_IMPLEMENTED; }
} // namespace xiiMemoryPolicies
