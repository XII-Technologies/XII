#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

namespace xiiMemoryPolicies
{
  xiiAllocationPolicyGuarding::xiiAllocationPolicyGuarding(xiiAllocator* pParent)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    XII_IGNORE_UNUSED(m_uiPageSize);
    XII_IGNORE_UNUSED(m_Mutex);
    XII_IGNORE_UNUSED(m_AllocationsToFreeLater);
  }

  void* xiiAllocationPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void xiiAllocationPolicyGuarding::Deallocate(void* ptr)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }
} // namespace xiiMemoryPolicies
