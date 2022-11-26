
XII_FORCE_INLINE void* xiiAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = xiiMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  XII_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

XII_ALWAYS_INLINE void xiiAlignedHeapAllocation::Deallocate(void* ptr)
{
  _aligned_free(ptr);
}
