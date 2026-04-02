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

  struct RadixSortTestItem
  {
    xiiUInt64 m_uiKey           = 0;
    xiiUInt32 m_uiOriginalIndex = 0;
  };

  struct RadixSortTestKeyExtractor
  {
    XII_ALWAYS_INLINE xiiUInt64 GetKey(const RadixSortTestItem& value) const { return value.m_uiKey; }

    // Key extraction should prefer GetKey.
    xiiUInt64 operator()(const RadixSortTestItem& value) const { return ~value.m_uiKey; }
  };

  xiiDynamicArray<RadixSortTestItem> CreateRadixSortTestItems()
  {
    xiiDynamicArray<RadixSortTestItem> items;
    items.Reserve(4096);

    for (xiiUInt32 i = 0; i < 4096; ++i)
    {
      auto& item             = items.ExpandAndGetRef();
      item.m_uiOriginalIndex = i;
      item.m_uiKey           = static_cast<xiiUInt64>(((i * 2654435761u) ^ (i >> 3u)) & 0x3FFu);
    }

    for (xiiUInt32 i = 0; i < items.GetCount(); ++i)
    {
      const xiiUInt32 uiSwapIndex = (i * 1103515245u + 12345u) % items.GetCount();
      xiiMath::Swap(items[i], items[uiSwapIndex]);
    }

    return items;
  }

  template <typename Container>
  void VerifyRadixSortStableOrder(const Container& items)
  {
    for (xiiUInt32 i = 1; i < items.GetCount(); ++i)
    {
      XII_TEST_BOOL(items[i - 1].m_uiKey <= items[i].m_uiKey);

      if (items[i - 1].m_uiKey == items[i].m_uiKey)
      {
        XII_TEST_BOOL(items[i - 1].m_uiOriginalIndex < items[i].m_uiOriginalIndex);
      }
    }
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  xiiDynamicArray<xiiInt32> a1;
  xiiDynamicArray<xiiInt32> a10;
  xiiDynamicArray<xiiInt32> a20;
  xiiDynamicArray<xiiInt32> a30;
  xiiDynamicArray<xiiInt32> a40;

  for (xiiUInt32 i = 0; i < 2000; ++i)
  {
    a1.PushBack(rand() % 100000);
    a10.PushBack(rand() % 100000);
    a20.PushBack(rand() % 100000);
    a30.PushBack(rand() % 100000);
    a40.PushBack(rand() % 100000);
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


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "BubbleSort")
  {
    xiiDynamicArray<xiiInt32> a2 = a10;

    xiiSorting::BubbleSort(a10, CustomComparer());

    for (xiiUInt32 i = 1; i < a10.GetCount(); ++i)
    {
      XII_TEST_BOOL(a10[i - 1] >= a10[i]);
    }

    xiiArrayPtr<xiiInt32> arrayPtr = a2;
    xiiSorting::BubbleSort(arrayPtr, CustomComparer());

    for (xiiUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "BubbleSort - Lambda")
  {
    xiiDynamicArray<xiiInt32> a2 = a10;
    xiiSorting::BubbleSort(a2, [](const auto& a, const auto& b) { return a > b; });

    for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      XII_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SelectionSort")
  {
    xiiDynamicArray<xiiInt32> a2 = a20;

    xiiSorting::SelectionSort(a20, CustomComparer());

    for (xiiUInt32 i = 1; i < a20.GetCount(); ++i)
    {
      XII_TEST_BOOL(a20[i - 1] >= a20[i]);
    }

    xiiArrayPtr<xiiInt32> arrayPtr = a2;
    xiiSorting::SelectionSort(arrayPtr, CustomComparer());

    for (xiiUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SelectionSort - Lambda")
  {
    xiiDynamicArray<xiiInt32> a2 = a20;
    xiiSorting::SelectionSort(a2, [](const auto& a, const auto& b) { return a > b; });

    for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      XII_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SelectionSortStable")
  {
    xiiDynamicArray<xiiInt32> a2 = a30;

    xiiSorting::SelectionSortStable(a30, CustomComparer());

    for (xiiUInt32 i = 1; i < a30.GetCount(); ++i)
    {
      XII_TEST_BOOL(a30[i - 1] >= a30[i]);
    }

    xiiArrayPtr<xiiInt32> arrayPtr = a2;
    xiiSorting::SelectionSortStable(arrayPtr, CustomComparer());

    for (xiiUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SelectionSortStable - Lambda")
  {
    xiiDynamicArray<xiiInt32> a2 = a30;
    xiiSorting::SelectionSortStable(a2, [](const auto& a, const auto& b) { return a > b; });

    for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      XII_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MergeSort")
  {
    xiiDynamicArray<xiiInt32> a2 = a40;

    xiiSorting::MergeSort(a40, CustomComparer());

    for (xiiUInt32 i = 1; i < a40.GetCount(); ++i)
    {
      XII_TEST_BOOL(a40[i - 1] >= a40[i]);
    }

    xiiArrayPtr<xiiInt32> arrayPtr = a2;
    xiiSorting::MergeSort(arrayPtr, CustomComparer());

    for (xiiUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MergeSort - Lambda")
  {
    xiiDynamicArray<xiiInt32> a2 = a40;
    xiiSorting::MergeSort(a2, [](const auto& a, const auto& b) { return a > b; });

    for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      XII_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - UInt64")
  {
    xiiDynamicArray<xiiUInt64> values;
    values.Reserve(5000);

    for (xiiUInt32 i = 0; i < 5000; ++i)
    {
      const xiiUInt64 uiValue = (static_cast<xiiUInt64>(rand() % 8192) << 32u) | static_cast<xiiUInt64>(rand() % 8192);
      values.PushBack(uiValue);
    }

    xiiDynamicArray<xiiUInt64> valuesPtrSort = values;
    xiiDynamicArray<xiiUInt64> scratchBuffer;
    scratchBuffer.SetCountUninitialized(values.GetCount());

    xiiSorting::RadixSort(values, scratchBuffer, DefaultRadixKeyExtractor<xiiUInt64>());

    for (xiiUInt32 i = 1; i < values.GetCount(); ++i)
    {
      XII_TEST_BOOL(values[i - 1] <= values[i]);
    }

    xiiArrayPtr<xiiUInt64> valuesPtr = valuesPtrSort;
    xiiSorting::RadixSort(valuesPtr, scratchBuffer, DefaultRadixKeyExtractor<xiiUInt64>());

    for (xiiUInt32 i = 1; i < valuesPtr.GetCount(); ++i)
    {
      XII_TEST_BOOL(valuesPtr[i - 1] <= valuesPtr[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - KeyExtractor")
  {
    xiiDynamicArray<RadixSortTestItem> items = CreateRadixSortTestItems();
    xiiDynamicArray<RadixSortTestItem> scratchBuffer;
    scratchBuffer.SetCount(items.GetCount());

    xiiSorting::RadixSort(items, scratchBuffer, RadixSortTestKeyExtractor());
    VerifyRadixSortStableOrder(items);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - ArrayPtr Scratch")
  {
    xiiDynamicArray<RadixSortTestItem> items = CreateRadixSortTestItems();
    xiiDynamicArray<RadixSortTestItem> scratchBuffer;

    xiiArrayPtr<RadixSortTestItem> itemPtr = items;
    scratchBuffer.SetCount(itemPtr.GetCount());
    xiiSorting::RadixSort(itemPtr, scratchBuffer, RadixSortTestKeyExtractor());

    VerifyRadixSortStableOrder(itemPtr);
  }
}
