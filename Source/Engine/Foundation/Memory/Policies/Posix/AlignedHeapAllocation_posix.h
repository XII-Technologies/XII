
XII_FORCE_INLINE void* xiiAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  // Alignment has to be at least sizeof(void*) otherwise posix_memalign will fail.
  uiAlign = xiiMath::Max<size_t>(uiAlign, 16u);

  void* ptr = nullptr;

  xiiInt32 iResult = posix_memalign(&ptr, uiAlign, uiSize);
  XII_IGNORE_UNUSED(iResult);
  XII_ASSERT_DEV(iResult == 0, "posix_memalign failed with error: {0}", iResult);

  XII_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

XII_ALWAYS_INLINE void xiiAlignedHeapAllocation::Deallocate(void* ptr)
{
  free(ptr);
}
