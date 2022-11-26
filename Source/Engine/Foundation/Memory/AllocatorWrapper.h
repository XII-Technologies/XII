#pragma once

struct xiiNullAllocatorWrapper
{
  XII_FORCE_INLINE static xiiAllocatorBase* GetAllocator()
  {
    XII_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct xiiDefaultAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocatorBase* GetAllocator() { return xiiFoundation::GetDefaultAllocator(); }
};

struct xiiStaticAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocatorBase* GetAllocator() { return xiiFoundation::GetStaticAllocator(); }
};

struct xiiAlignedAllocatorWrapper
{
  XII_ALWAYS_INLINE static xiiAllocatorBase* GetAllocator() { return xiiFoundation::GetAlignedAllocator(); }
};

struct XII_FOUNDATION_DLL xiiLocalAllocatorWrapper
{
  xiiLocalAllocatorWrapper(xiiAllocatorBase* pAllocator);

  void Reset();

  static xiiAllocatorBase* GetAllocator();
};
