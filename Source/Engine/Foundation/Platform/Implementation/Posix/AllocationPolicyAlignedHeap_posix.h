/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_FORCE_INLINE void* xiiAllocationPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  // Alignment has to be at least sizeof(void*) otherwise posix_memalign will fail.
  uiAlign = xiiMath::Max<size_t>(uiAlign, 16U);

  void* pPtr = nullptr;

  xiiInt32 iResult = posix_memalign(&pPtr, uiAlign, uiSize);
  XII_IGNORE_UNUSED(iResult);
  XII_ASSERT_DEV(iResult == 0, "posix_memalign failed with error: {0}", iResult);

  XII_CHECK_ALIGNMENT(pPtr, uiAlign);

  return pPtr;
}

XII_ALWAYS_INLINE void xiiAllocationPolicyAlignedHeap::Deallocate(void* pPtr)
{
  free(pPtr);
}
