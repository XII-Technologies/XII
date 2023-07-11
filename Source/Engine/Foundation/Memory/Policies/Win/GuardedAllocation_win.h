
#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

namespace xiiMemoryPolicies
{
  struct AlloctionMetaData
  {
    AlloctionMetaData()
    {
      m_uiSize = 0;

      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_magic); ++i)
      {
        m_magic[i] = 0x12345678;
      }
    }

    ~AlloctionMetaData()
    {
      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_magic); ++i)
      {
        XII_ASSERT_DEV(m_magic[i] == 0x12345678, "Magic value has been overwritten. This might be the result of a buffer underrun!");
      }
    }

    size_t    m_uiSize;
    xiiUInt32 m_magic[32];
  };

  xiiGuardedAllocation::xiiGuardedAllocation(xiiAllocatorBase* pParent)
  {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_uiPageSize = sysInfo.dwPageSize;
  }

  void* xiiGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_ASSERT_DEV(xiiMath::IsPowerOf2((xiiUInt32)uiAlign), "Alignment must be power of two");
    uiAlign = xiiMath::Max<size_t>(uiAlign, XII_ALIGNMENT_MINIMUM);

    size_t uiAlignedSize = xiiMemoryUtils::AlignSize(uiSize, uiAlign);
    size_t uiTotalSize   = uiAlignedSize + sizeof(AlloctionMetaData);

    // Align to full pages and add one page in front and one in back.
    size_t uiPageSize     = m_uiPageSize;
    size_t uiFullPageSize = xiiMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
    void*  pMemory        = VirtualAlloc(nullptr, uiFullPageSize + 2 * uiPageSize, MEM_RESERVE, PAGE_NOACCESS);
    XII_ASSERT_DEV(pMemory != nullptr, "Could not reserve memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

    // Add one page and commit the payload pages.
    pMemory   = xiiMemoryUtils::AddByteOffset(pMemory, uiPageSize);
    void* ptr = VirtualAlloc(pMemory, uiFullPageSize, MEM_COMMIT, PAGE_READWRITE);
    XII_ASSERT_DEV(ptr != nullptr, "Could not commit memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

    // Store information in meta data.
    AlloctionMetaData* metaData = xiiMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(ptr), uiFullPageSize - uiTotalSize);
    xiiMemoryUtils::Construct(metaData, 1);
    metaData->m_uiSize = uiAlignedSize;

    // Finally add offset to the actual payload.
    ptr = xiiMemoryUtils::AddByteOffset(metaData, sizeof(AlloctionMetaData));
    return ptr;
  }

  // Deactivate analysis warning for VirtualFree flags, it is needed for the specific functionality.
  XII_MSVC_ANALYSIS_WARNING_PUSH
  XII_MSVC_ANALYSIS_WARNING_DISABLE(6250)

  void xiiGuardedAllocation::Deallocate(void* pPtr)
  {
    xiiLock<xiiMutex> lock(m_Mutex);

    if (!m_AllocationsToFreeLater.CanAppend())
    {
      void* pMemory = m_AllocationsToFreeLater.PeekFront();
      XII_VERIFY(::VirtualFree(pMemory, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

      m_AllocationsToFreeLater.PopFront();
    }

    // Retrieve info from meta data first.
    AlloctionMetaData* metaData      = xiiMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(pPtr), -((ptrdiff_t)sizeof(AlloctionMetaData)));
    size_t             uiAlignedSize = metaData->m_uiSize;

    xiiMemoryUtils::Destruct(metaData, 1);

    // Decommit the pages but do not release the memory yet so use-after-free can be detected.
    size_t uiPageSize     = m_uiPageSize;
    size_t uiTotalSize    = uiAlignedSize + sizeof(AlloctionMetaData);
    size_t uiFullPageSize = xiiMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
    pPtr                  = xiiMemoryUtils::AddByteOffset(pPtr, ((ptrdiff_t)uiAlignedSize) - uiFullPageSize);

    XII_VERIFY(::VirtualFree(pPtr, uiFullPageSize, MEM_DECOMMIT), "Could not decommit memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

    // Finally store the allocation so we can release it later.
    void* pMemory = xiiMemoryUtils::AddByteOffset(pPtr, -((ptrdiff_t)uiPageSize));
    m_AllocationsToFreeLater.PushBack(pMemory);
  }

  XII_MSVC_ANALYSIS_WARNING_POP
} // namespace xiiMemoryPolicies
