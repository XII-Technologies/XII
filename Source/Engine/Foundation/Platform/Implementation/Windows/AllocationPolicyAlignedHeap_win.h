
XII_FORCE_INLINE void* xiiAllocationPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = xiiMath::Max<size_t>(uiAlign, 16u);

  void* pPtr = _aligned_malloc(uiSize, uiAlign);
  XII_CHECK_ALIGNMENT(pPtr, uiAlign);

  return pPtr;
}

XII_ALWAYS_INLINE void xiiAllocationPolicyAlignedHeap::Deallocate(void* pPtr)
{
  _aligned_free(pPtr);
}
