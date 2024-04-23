#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadUtils.h>

/// \brief This struct represents a block of type T, typically 4kb.
template <typename T, xiiUInt32 SizeInBytes>
struct xiiDataBlock
{
  XII_DECLARE_POD_TYPE();

  enum
  {
    SIZE_IN_BYTES = SizeInBytes,
    CAPACITY      = SIZE_IN_BYTES / sizeof(T)
  };

  xiiDataBlock(T* pData, xiiUInt32 uiCount);

  T* ReserveBack();
  T* PopBack();

  bool IsEmpty() const;
  bool IsFull() const;

  T& operator[](xiiUInt32 uiIndex) const;

  T*        m_pData;
  xiiUInt32 m_uiCount;
};

/// \brief A block allocator which can only allocates blocks of memory at once.
template <xiiUInt32 BlockSizeInByte>
class xiiLargeBlockAllocator
{
public:
  xiiLargeBlockAllocator(xiiStringView sName, xiiAllocatorBase* pParent, xiiAllocatorTrackingMode mode = xiiAllocatorTrackingMode::Default);
  ~xiiLargeBlockAllocator();

  template <typename T>
  xiiDataBlock<T, BlockSizeInByte> AllocateBlock();

  template <typename T>
  void DeallocateBlock(xiiDataBlock<T, BlockSizeInByte>& ref_block);


  xiiStringView GetName() const;

  xiiAllocatorId GetId() const;

  const xiiAllocatorBase::Stats& GetStats() const;

private:
  void* Allocate(size_t uiAlign);
  void  Deallocate(void* ptr);

  xiiAllocatorId                      m_Id;
  xiiAllocatorTrackingMode            m_TrackingMode;

  xiiMutex    m_Mutex;
  xiiThreadID m_ThreadID;

  struct SuperBlock
  {
    XII_DECLARE_POD_TYPE();

    static constexpr xiiUInt32 NUM_BLOCKS    = 16U;
    static constexpr xiiUInt32 SIZE_IN_BYTES = BlockSizeInByte * NUM_BLOCKS;

    void* m_pBasePtr = nullptr;

    xiiUInt32 m_uiUsedBlocks;
  };

  xiiDynamicArray<SuperBlock> m_SuperBlocks;
  xiiDynamicArray<xiiUInt32>  m_FreeBlocks;
};

#include <Foundation/Memory/Implementation/LargeBlockAllocator_inl.h>
