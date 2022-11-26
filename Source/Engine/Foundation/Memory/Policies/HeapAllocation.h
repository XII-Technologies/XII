#pragma once

#include <Foundation/Basics.h>

namespace xiiMemoryPolicies
{
  /// \brief Default heap memory allocation policy.
  ///
  /// \see xiiAllocator
  class xiiHeapAllocation
  {
  public:
    XII_ALWAYS_INLINE xiiHeapAllocation(xiiAllocatorBase* pParent) {}
    XII_ALWAYS_INLINE ~xiiHeapAllocation() {}

    XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
      // if these asserts fail, you need to check what container made the allocation and change it
      // to use an aligned allocator, e.g. xiiAlignedAllocatorWrapper

      // unfortunately using XII_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
      // alignment interestingly, the code that does that, seems to work fine anyway
      XII_ASSERT_DEBUG(
        uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

      void* ptr = malloc(PadSize(uiSize));
      XII_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    XII_FORCE_INLINE void* Reallocate(void* currentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      void* ptr = realloc(RestorePtr(currentPtr), PadSize(uiNewSize));
      XII_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    XII_ALWAYS_INLINE void Deallocate(void* ptr) { free(RestorePtr(ptr)); }

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return nullptr; }

  private:
    XII_ALWAYS_INLINE size_t PadSize(size_t uiSize)
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      return uiSize + 2 * XII_ALIGNMENT_MINIMUM;
#else
      return uiSize;
#endif
    }

    XII_ALWAYS_INLINE void* OffsetPtr(void* ptr)
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      xiiUInt32 uiOffset            = xiiMemoryUtils::IsAligned(ptr, 2 * XII_ALIGNMENT_MINIMUM) ? XII_ALIGNMENT_MINIMUM : 2 * XII_ALIGNMENT_MINIMUM;
      ptr                           = xiiMemoryUtils::AddByteOffset(ptr, uiOffset - 4);
      *static_cast<xiiUInt32*>(ptr) = uiOffset;
      return xiiMemoryUtils::AddByteOffset(ptr, 4);
#else
      return ptr;
#endif
    }

    XII_ALWAYS_INLINE void* RestorePtr(void* ptr)
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      ptr               = xiiMemoryUtils::AddByteOffset(ptr, -4);
      xiiInt32 uiOffset = *static_cast<xiiUInt32*>(ptr);
      return xiiMemoryUtils::AddByteOffset(ptr, -uiOffset + 4);
#else
      return ptr;
#endif
    }
  };
} // namespace xiiMemoryPolicies
