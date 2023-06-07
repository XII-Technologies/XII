#pragma once

#include <Foundation/Math/Math.h>

namespace xiiMemoryPolicies
{
  /// \brief Allocation policy to support custom alignment per allocation.
  ///
  /// \see xiiAllocator
  template <typename T>
  class xiiAlignedAllocation
  {
  public:
    xiiAlignedAllocation(xiiAllocatorBase* pParent) :
      m_allocator(pParent)
    {
    }

    void* Allocate(size_t uiSize, size_t uiAlign)
    {
      XII_ASSERT_DEV(uiAlign < (1 << 24), "Alignment of {0} is too big. Maximum supported alignment is 16MB.", uiAlign);

      const xiiUInt32 uiPadding     = (xiiUInt32)(uiAlign - 1 + MetadataSize);
      const size_t    uiAlignedSize = uiSize + uiPadding;

      xiiUInt8* pMemory = (xiiUInt8*)m_allocator.Allocate(uiAlignedSize, XII_ALIGNMENT_MINIMUM);

      xiiUInt8* pAlignedMemory = xiiMemoryUtils::AlignBackwards(pMemory + uiPadding, uiAlign);

      xiiUInt32* pMetadata = GetMetadataPtr(pAlignedMemory);
      *pMetadata           = PackMetadata((xiiUInt32)(pAlignedMemory - pMemory), (xiiUInt32)uiAlign);

      return pAlignedMemory;
    }

    void Deallocate(void* pPtr)
    {
      const xiiUInt32 uiOffset = UnpackOffset(GetMetadata(ptr));
      xiiUInt8*       pMemory  = static_cast<xiiUInt8*>(ptr) - uiOffset;
      m_allocator.Deallocate(pMemory);
    }

    size_t AllocatedSize(const void* pPtr)
    {
      const xiiUInt32 uiMetadata = GetMetadata(ptr);
      const xiiUInt32 uiOffset   = UnpackOffset(uiMetadata);
      const xiiUInt32 uiAlign    = UnpackAlignment(uiMetadata);
      const xiiUInt32 uiPadding  = uiAlign - 1 + MetadataSize;

      const xiiUInt8* pMemory = static_cast<const xiiUInt8*>(ptr) - uiOffset;
      return m_allocator.AllocatedSize(pMemory) - uiPadding;
    }

    size_t UsedMemorySize(const void* pPtr)
    {
      const xiiUInt32 uiOffset = UnpackOffset(GetMetadata(ptr));
      const xiiUInt8* pMemory  = static_cast<const xiiUInt8*>(ptr) - uiOffset;
      return m_allocator.UsedMemorySize(pMemory);
    }

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return m_allocator.GetParent(); }

  private:
    enum
    {
      MetadataSize = sizeof(xiiUInt32)
    };

    // Meta-data is stored 4 bytes before the aligned memory
    inline xiiUInt32* GetMetadataPtr(void* pAlignedMemory)
    {
      return static_cast<xiiUInt32*>(xiiMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    inline xiiUInt32 GetMetadata(const void* pAlignedMemory)
    {
      return *static_cast<const xiiUInt32*>(xiiMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    // Store offset between pMemory and pAlignedMemory in the lower 24 bit of meta-data.
    // The upper 8 bit are used to store the Log2 of the alignment.
    XII_ALWAYS_INLINE xiiUInt32 PackMetadata(xiiUInt32 uiOffset, xiiUInt32 uiAlignment) { return uiOffset | (xiiMath::Log2i(uiAlignment) << 24); }

    XII_ALWAYS_INLINE xiiUInt32 UnpackOffset(xiiUInt32 uiMetadata) { return uiMetadata & 0x00FFFFFF; }

    XII_ALWAYS_INLINE xiiUInt32 UnpackAlignment(xiiUInt32 uiMetadata) { return 1 << (uiMetadata >> 24); }

    T m_allocator;
  };
} // namespace xiiMemoryPolicies
