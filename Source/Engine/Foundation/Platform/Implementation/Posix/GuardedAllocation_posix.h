
namespace xiiMemoryPolicies
{
  xiiGuardedAllocation::xiiGuardedAllocation(xiiAllocatorBase* pParent) { XII_ASSERT_NOT_IMPLEMENTED; }

  void* xiiGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void xiiGuardedAllocation::Deallocate(void* ptr) { XII_ASSERT_NOT_IMPLEMENTED; }
} // namespace xiiMemoryPolicies
