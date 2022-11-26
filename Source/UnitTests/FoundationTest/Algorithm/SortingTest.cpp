#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

namespace
{
  struct CustomComparer
  {
    XII_ALWAYS_INLINE bool Less(xiiInt32 a, xiiInt32 b) const { return a > b; }

    // Comparision via operator. Sorting algorithm should prefer Less operator
    bool operator()(xiiInt32 a, xiiInt32 b) const { return a < b; }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  xiiDynamicArray<xiiInt32> a1;

  for (xiiUInt32 i = 0; i < 2000; ++i)
  {
    a1.PushBack(rand() % 100000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "QuickSort")
  {
    xiiDynamicArray<xiiInt32> a2 = a1;

    xiiSorting::QuickSort(a1, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (xiiUInt32 i = 1; i < a1.GetCount(); ++i)
    {
      XII_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    xiiArrayPtr<xiiInt32> arrayPtr = a2;
    xiiSorting::QuickSort(arrayPtr, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (xiiUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "QuickSort - Lambda")
  {
    xiiDynamicArray<xiiInt32> a2 = a1;
    xiiSorting::QuickSort(a2, [](const auto& a, const auto& b) { return a > b; });

    for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      XII_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }
}
