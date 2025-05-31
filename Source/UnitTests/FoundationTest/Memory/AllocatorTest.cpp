#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Memory/StackAllocator.h>

struct alignas(XII_ALIGNMENT_MINIMUM) NonAlignedVector
{
  XII_DECLARE_POD_TYPE();

  NonAlignedVector() :
    x(5.0f), y(6.0f), z(8.0f)
  {
  }

  float x;
  float y;
  float z;
};

struct alignas(16) AlignedVector
{
  XII_DECLARE_POD_TYPE();

  AlignedVector() :
    x(5.0f), y(6.0f), z(8.0f)
  {
  }

  float x;
  float y;
  float z;
  float w;
};

template <typename T>
void TestAlignmentHelper(size_t uiExpectedAlignment)
{
  xiiAllocatorBase* pAllocator = xiiFoundation::GetAlignedAllocator();
  XII_TEST_BOOL(pAllocator != nullptr);

  size_t uiAlignment = alignof(T);
  XII_TEST_INT(uiAlignment, uiExpectedAlignment);

  T testOnStack = T();
  XII_TEST_BOOL(xiiMemoryUtils::IsAligned(&testOnStack, uiExpectedAlignment));

  T*             pTestBuffer = XII_NEW_RAW_BUFFER(pAllocator, T, 32);
  xiiArrayPtr<T> TestArray   = XII_NEW_ARRAY(pAllocator, T, 32);

  // default constructor should be called even if we declare as a pod type
  XII_TEST_FLOAT(TestArray[0].x, 5.0f, 0.0f);
  XII_TEST_FLOAT(TestArray[0].y, 6.0f, 0.0f);
  XII_TEST_FLOAT(TestArray[0].z, 8.0f, 0.0f);

  XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pTestBuffer, uiExpectedAlignment));
  XII_TEST_BOOL(xiiMemoryUtils::IsAligned(TestArray.GetPtr(), uiExpectedAlignment));

  size_t uiExpectedSize = sizeof(T) * 32;

  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::AllocationStats)
  {
    XII_TEST_INT(pAllocator->AllocatedSize(pTestBuffer), uiExpectedSize);

    xiiAllocatorBase::Stats stats = pAllocator->GetStats();
    XII_TEST_INT(stats.m_uiAllocationSize, uiExpectedSize * 2);
    XII_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 2);
  }

  XII_DELETE_ARRAY(pAllocator, TestArray);
  XII_DELETE_RAW_BUFFER(pAllocator, pTestBuffer);

  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::Basics)
  {
    xiiAllocatorBase::Stats stats = pAllocator->GetStats();
    XII_TEST_INT(stats.m_uiAllocationSize, 0);
    XII_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 0);
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(Memory);

XII_CREATE_SIMPLE_TEST(Memory, Allocator)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Alignment")
  {
    TestAlignmentHelper<NonAlignedVector>(XII_ALIGNMENT_MINIMUM);
    TestAlignmentHelper<AlignedVector>(16);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LargeBlockAllocator")
  {
    constexpr xiiUInt32 BLOCK_SIZE_IN_BYTES = 4096 * 4;
    const xiiUInt32     uiPageSize          = xiiSystemInformation::Get().GetMemoryPageSize();

    xiiLargeBlockAllocator<BLOCK_SIZE_IN_BYTES> allocator("Test", xiiFoundation::GetDefaultAllocator(), xiiAllocatorTrackingMode::AllocationStats);

    xiiDynamicArray<xiiDataBlock<xiiInt32, BLOCK_SIZE_IN_BYTES>> blocks;
    blocks.Reserve(1000);

    for (xiiUInt32 i = 0; i < 17; ++i)
    {
      auto block = allocator.AllocateBlock<xiiInt32>();
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(block.m_pData, uiPageSize)); // test page alignment
      XII_TEST_INT(block.m_uiCount, 0);

      blocks.PushBack(block);
    }

    xiiAllocatorBase::Stats stats = allocator.GetStats();

    XII_TEST_BOOL(stats.m_uiNumAllocations == 17);
    XII_TEST_BOOL(stats.m_uiNumDeallocations == 0);
    XII_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (xiiUInt32 i = 0; i < 200; ++i)
    {
      auto block = allocator.AllocateBlock<xiiInt32>();
      blocks.PushBack(block);
    }

    for (xiiUInt32 i = 0; i < 200; ++i)
    {
      allocator.DeallocateBlock(blocks.PeekBack());
      blocks.PopBack();
    }

    stats = allocator.GetStats();

    XII_TEST_BOOL(stats.m_uiNumAllocations == 217);
    XII_TEST_BOOL(stats.m_uiNumDeallocations == 200);
    XII_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (xiiUInt32 i = 0; i < 2000; ++i)
    {
      xiiUInt32 uiAction = rand() % 2;
      if (uiAction == 0)
      {
        blocks.PushBack(allocator.AllocateBlock<xiiInt32>());
      }
      else if (blocks.GetCount() > 0)
      {
        xiiUInt32 uiIndex = rand() % blocks.GetCount();
        auto      block   = blocks[uiIndex];

        allocator.DeallocateBlock(block);

        blocks.RemoveAtAndSwap(uiIndex);
      }
    }

    for (xiiUInt32 i = 0; i < blocks.GetCount(); ++i)
    {
      allocator.DeallocateBlock(blocks[i]);
    }

    stats = allocator.GetStats();

    XII_TEST_BOOL(stats.m_uiNumAllocations - stats.m_uiNumDeallocations == 0);
    XII_TEST_BOOL(stats.m_uiAllocationSize == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StackAllocator")
  {
    xiiStackAllocator<> allocator("TestStackAllocator", xiiFoundation::GetAlignedAllocator());

    void* blocks[8];
    for (size_t i = 0; i < XII_ARRAY_SIZE(blocks); i++)
    {
      size_t size = i + 1;
      blocks[i]   = allocator.Allocate(size, sizeof(void*), nullptr);
      XII_TEST_BOOL(blocks[i] != nullptr);
      if (i > 0)
      {
        XII_TEST_BOOL((xiiUInt8*)blocks[i - 1] + (size - 1) <= blocks[i]);
      }
    }

    for (size_t i = XII_ARRAY_SIZE(blocks); i--;)
    {
      allocator.Deallocate(blocks[i]);
    }

    size_t sizes[] = {128, 128, 4096, 1024, 1024, 16000, 512, 512, 768, 768, 16000, 16000, 16000, 16000};
    void*  allocs[XII_ARRAY_SIZE(sizes)];
    for (size_t i = 0; i < XII_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      XII_TEST_BOOL(allocs[i] != nullptr);
    }
    for (size_t i = XII_ARRAY_SIZE(sizes); i--;)
    {
      allocator.Deallocate(allocs[i]);
    }
    allocator.Reset();

    for (size_t i = 0; i < XII_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      XII_TEST_BOOL(allocs[i] != nullptr);
    }
    allocator.Reset();
    allocs[0] = allocator.Allocate(8, sizeof(void*), nullptr);
    XII_TEST_BOOL(allocs[0] < allocs[1]);
    allocator.Deallocate(allocs[0]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StackAllocator with non-PODs")
  {
    xiiStackAllocator<> allocator("TestStackAllocator", xiiFoundation::GetAlignedAllocator());

    xiiDynamicArray<xiiConstructionCounter*> counters;
    counters.Reserve(100);

    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      counters.PushBack(XII_NEW(&allocator, xiiConstructionCounter));
    }

    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      XII_NEW(&allocator, NonAlignedVector);
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasConstructed(100));

    for (xiiUInt32 i = 0; i < 50; ++i)
    {
      XII_DELETE(&allocator, counters[i * 2]);
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasDestructed(50));

    allocator.Reset();

    XII_TEST_BOOL(xiiConstructionCounter::HasDestructed(50));
  }
}
