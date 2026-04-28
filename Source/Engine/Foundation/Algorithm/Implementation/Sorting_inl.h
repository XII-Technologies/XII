/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Memory/MemoryUtils.h>

template <typename Container, typename Comparer>
void xiiSorting::QuickSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  QuickSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::QuickSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  QuickSort(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::InsertionSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  InsertionSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::InsertionSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  InsertionSort(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::BubbleSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  BubbleSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::BubbleSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  BubbleSort(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  SelectionSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  SelectionSort(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSortStable(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  SelectionSortStable(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSortStable(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  SelectionSortStable(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::MergeSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  MergeSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::MergeSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer)
{
  if (ref_pArray.IsEmpty())
    return;

  MergeSort(ref_pArray, 0, ref_pArray.GetCount() - 1, comparer);
}

template <typename Container, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSort(Container& ref_container, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc)
{
  if (ref_container.IsEmpty())
    return;

  XII_ASSERT_DEV(ref_scratchBuffer.GetCount() >= ref_container.GetCount(), "Radix sort scratch buffer has {0} elements, but {1} are required.", ref_scratchBuffer.GetCount(), ref_container.GetCount());

  RadixSortInternal(ref_container, ref_scratchBuffer, keyFunc);
}

template <typename T, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSort(xiiArrayPtr<T>& ref_pArray, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc)
{
  if (ref_pArray.IsEmpty())
    return;

  XII_ASSERT_DEV(ref_scratchBuffer.GetCount() >= ref_pArray.GetCount(), "Radix sort scratch buffer has {0} elements, but {1} are required.", ref_scratchBuffer.GetCount(), ref_pArray.GetCount());

  RadixSortInternal(ref_pArray, ref_scratchBuffer, keyFunc);
}

template <typename Container, typename Comparer>
void xiiSorting::QuickSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(container, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const xiiUInt32 uiPivotIndex = Partition(container, uiStartIndex, uiEndIndex, comparer);

      xiiUInt32 uiFirstHalfEndIndex    = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      xiiUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, container[uiFirstHalfEndIndex], container[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }
      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, container[uiPivotIndex], container[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
      {
        QuickSort(container, uiStartIndex, uiFirstHalfEndIndex, comparer);
      }
      if (uiSecondHalfStartIndex < uiEndIndex)
      {
        QuickSort(container, uiSecondHalfStartIndex, uiEndIndex, comparer);
      }
    }
  }
}

template <typename Container, typename Comparer>
xiiUInt32 xiiSorting::Partition(Container& container, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer)
{
  xiiUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, container[uiLeft], container[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, container[uiRight], container[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, container[uiLeft], container[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, container[uiLeft], container[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, container[uiRight], container[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  xiiMath::Swap(container[uiPivotIndex], container[uiRight]); // move pivot to right

  xiiUInt32 uiIndex = uiLeft;
  for (xiiUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, container[i], container[uiRight]))
    {
      xiiMath::Swap(container[i], container[uiIndex]);
      ++uiIndex;
    }
  }

  xiiMath::Swap(container[uiIndex], container[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename T, typename Comparer>
void xiiSorting::QuickSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* pPtr = pArray.GetPtr();

  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(pArray, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const xiiUInt32 uiPivotIndex = Partition(pPtr, uiStartIndex, uiEndIndex, comparer);

      xiiUInt32 uiFirstHalfEndIndex    = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      xiiUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, pPtr[uiFirstHalfEndIndex], pPtr[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, pPtr[uiPivotIndex], pPtr[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
      {
        QuickSort(pArray, uiStartIndex, uiFirstHalfEndIndex, comparer);
      }

      if (uiSecondHalfStartIndex < uiEndIndex)
      {
        QuickSort(pArray, uiSecondHalfStartIndex, uiEndIndex, comparer);
      }
    }
  }
}

template <typename T, typename Comparer>
xiiUInt32 xiiSorting::Partition(T* pPtr, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer)
{
  xiiUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  xiiMath::Swap(pPtr[uiPivotIndex], pPtr[uiRight]); // move pivot to right

  xiiUInt32 uiIndex = uiLeft;
  for (xiiUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, pPtr[i], pPtr[uiRight]))
    {
      xiiMath::Swap(pPtr[i], pPtr[uiIndex]);
      ++uiIndex;
    }
  }

  xiiMath::Swap(pPtr[uiIndex], pPtr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Container, typename Comparer>
void xiiSorting::InsertionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, container[uiHoleIndex], container[uiHoleIndex - 1]))
    {
      xiiMath::Swap(container[uiHoleIndex], container[uiHoleIndex - 1]);
      --uiHoleIndex;
    }
  }
}

template <typename T, typename Comparer>
void xiiSorting::InsertionSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* pPtr = pArray.GetPtr();

  for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiHoleIndex   = i;
    T         valueToInsert = std::move(pPtr[uiHoleIndex]);

    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, valueToInsert, pPtr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const xiiUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      xiiMemoryUtils::RelocateOverlapped(pPtr + uiHoleIndex + 1, pPtr + uiHoleIndex, uiMoveCount);
      xiiMemoryUtils::MoveConstruct(pPtr + uiHoleIndex, std::move(valueToInsert));
    }
    else
    {
      pPtr[uiHoleIndex] = std::move(valueToInsert);
    }
  }
}

template <typename Container, typename Comparer>
void xiiSorting::BubbleSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  bool bIsSorted = false;

  while (!bIsSorted)
  {
    bIsSorted = true;

    for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
    {
      xiiUInt32 uiHoleIndex = i;
      if (DoCompare(comparer, container[uiHoleIndex], container[uiHoleIndex - 1]))
      {
        xiiMath::Swap(container[uiHoleIndex], container[uiHoleIndex - 1]);
        bIsSorted = false;
      }
    }
  }
}

template <typename T, typename Comparer>
void xiiSorting::BubbleSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T*   pPtr      = pArray.GetPtr();
  bool bIsSorted = false;

  while (!bIsSorted)
  {
    bIsSorted = true;

    for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
    {
      xiiUInt32 uiHoleIndex = i;
      if (DoCompare(comparer, pPtr[uiHoleIndex], pPtr[uiHoleIndex - 1]))
      {
        xiiMath::Swap(pPtr[uiHoleIndex], pPtr[uiHoleIndex - 1]);
        bIsSorted = false;
      }
    }
  }
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex  = i;
    xiiUInt32 uiHoleIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, container[uiHoleIndexMin], container[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    xiiMath::Swap(container[uiHoleIndex], container[uiMinIndex]);
  }
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* pPtr = pArray.GetPtr();

  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex  = i;
    xiiUInt32 uiHoleIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, pPtr[uiHoleIndexMin], pPtr[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    xiiMath::Swap(pPtr[uiHoleIndex], pPtr[uiMinIndex]);
  }
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSortStable(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, container[uiHoleIndexMin], container[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    // In a stable selection sort, loop through all remaining elements and swap in to preserve order.
    // This essentially 'shifts' the minimum farthest to the left of the container (to the current 'i').
    auto valueStorage = container[uiMinIndex];
    while (uiMinIndex > i)
    {
      container[uiMinIndex] = container[uiMinIndex - 1];
      --uiMinIndex;
    }
    container[i] = valueStorage;
  }
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSortStable(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* pPtr = pArray.GetPtr();

  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, pPtr[uiHoleIndexMin], pPtr[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    // In a stable selection sort, loop through all remaining elements and swap in to preserve order.
    // This essentially 'shifts' the minimum farthest to the left of the container (to the current 'i').
    auto valueStorage = pPtr[uiMinIndex];
    while (uiMinIndex > i)
    {
      pPtr[uiMinIndex] = pPtr[uiMinIndex - 1];
      --uiMinIndex;
    }
    pPtr[i] = valueStorage;
  }
}

template <typename Container, typename Comparer>
void xiiSorting::MergeSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex >= uiEndIndex)
    return;

  // The same as uiStartIndex + (uiEndIndex - uiStartIndex) / 2.
  xiiUInt32 uiMiddleIndex = (uiStartIndex + uiEndIndex) >> 1;

  MergeSort(container, uiStartIndex, uiMiddleIndex, comparer);
  MergeSort(container, uiMiddleIndex + 1, uiEndIndex, comparer);
  Merge(container, uiStartIndex, uiMiddleIndex, uiEndIndex, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::MergeSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex >= uiEndIndex)
    return;

  // The same as uiStartIndex + (uiEndIndex - uiStartIndex) / 2.
  xiiUInt32 uiMiddleIndex = (uiStartIndex + uiEndIndex) >> 1;

  // Sort first and second halves and merge the result.
  MergeSort(pArray, uiStartIndex, uiMiddleIndex, comparer);
  MergeSort(pArray, uiMiddleIndex + 1, uiEndIndex, comparer);
  Merge(pArray, uiStartIndex, uiMiddleIndex, uiEndIndex, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::Merge(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  xiiUInt32 i{}, j{}, k{};
  xiiUInt32 uiRightSideSize = uiEndIndex - uiMiddleIndex;
  xiiUInt32 uiLeftSideSize  = uiMiddleIndex - uiStartIndex + 1;

  // Create temporary arrays to store container data.
  auto leftSideContainer  = new std::remove_reference_t<decltype(container[0])>[uiLeftSideSize];
  auto rightSideContainer = new std::remove_reference_t<decltype(container[0])>[uiRightSideSize];

  // Copy left side data.
  for (i = 0; i < uiLeftSideSize; ++i)
  {
    leftSideContainer[i] = container[uiStartIndex + i];
  }

  // Copy right side data.
  for (j = 0; j < uiRightSideSize; ++j)
  {
    rightSideContainer[j] = container[uiMiddleIndex + 1 + j];
  }

  // Merge and sort the temporary arrays into the container.
  i = 0;            // Initial index of the left side array.
  j = 0;            // Initial index of the right side array.
  k = uiStartIndex; // Initial index tracking which part of the container has been merged.

  while (i < uiLeftSideSize && j < uiRightSideSize)
  {
    if (DoCompare(comparer, leftSideContainer[i], rightSideContainer[j]))
    {
      container[k] = leftSideContainer[i];
      ++i;
    }
    else
    {
      container[k] = rightSideContainer[j];
      ++j;
    }

    ++k;
  }

  // Copy the remaining elements of the left side array if any.
  while (i < uiLeftSideSize)
  {
    container[k] = leftSideContainer[i];
    ++i;
    ++k;
  }

  // Copy the remaining elements of the right side array if any.
  while (j < uiRightSideSize)
  {
    container[k] = rightSideContainer[j];
    ++j;
    ++k;
  }

  // Free heap allocated temporary arrays.
  delete[] leftSideContainer, rightSideContainer;
}

template <typename T, typename Comparer>
void xiiSorting::Merge(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* pPtr = pArray.GetPtr();

  xiiUInt32 i{}, j{}, k{};
  xiiUInt32 uiRightSideSize = uiEndIndex - uiMiddleIndex;
  xiiUInt32 uiLeftSideSize  = uiMiddleIndex - uiStartIndex + 1;

  // Create temporary arrays to store container data.
  T* leftSideContainer  = new T[uiLeftSideSize];
  T* rightSideContainer = new T[uiRightSideSize];

  // Copy left side data.
  for (i = 0; i < uiLeftSideSize; ++i)
  {
    T value              = pPtr[uiStartIndex + i];
    leftSideContainer[i] = value;
  }

  // Copy right side data.
  for (j = 0; j < uiRightSideSize; ++j)
  {
    T value               = pPtr[uiMiddleIndex + 1 + j];
    rightSideContainer[j] = value;
  }

  // Merge and sort the temporary arrays into the container.
  i = 0;            // Initial index of the left side array.
  j = 0;            // Initial index of the right side array.
  k = uiStartIndex; // Initial index tracking which part of the container has been merged.

  while (i < uiLeftSideSize && j < uiRightSideSize)
  {
    if (DoCompare(comparer, leftSideContainer[i], rightSideContainer[j]))
    {
      pPtr[k] = leftSideContainer[i];
      ++i;
    }
    else
    {
      pPtr[k] = rightSideContainer[j];
      ++j;
    }

    ++k;
  }

  // Copy the remaining elements of the left side array if any.
  while (i < uiLeftSideSize)
  {
    pPtr[k] = leftSideContainer[i];
    ++i;
    ++k;
  }

  // Copy the remaining elements of the right side array if any.
  while (j < uiRightSideSize)
  {
    pPtr[k] = rightSideContainer[j];
    ++j;
    ++k;
  }

  // Free heap allocated temporary arrays.
  delete[] leftSideContainer, rightSideContainer;
}

template <typename Container, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSortInternal(Container& container, ScratchContainer& scratchBuffer, const KeyFunc& keyFunc)
{
  const xiiUInt32 uiCount = container.GetCount();
  if (uiCount < 2)
    return;

  XII_ASSERT_DEV(scratchBuffer.GetCount() >= uiCount, "Radix sort scratch buffer has {0} elements, but {1} are required.", scratchBuffer.GetCount(), uiCount);

  // Phase 1: single pass over the source to build all 8 histograms at once and cache the extracted keys.

  // Key cache: avoids re-invoking keyFunc (and any bit-remapping) during scatter.
  // Stored in scratchBuffer's backing allocation to avoid an extra heap allocation, we use a raw key array on the side instead.
  // If a separate allocation is undesirable, keys can be re-extracted during scatter at the cost of keyFunc overhead.

  // 8 histograms of 256 buckets, laid out contiguously for cache friendliness.
  xiiUInt32 counts[8][256] = {};

  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    const xiiUInt64 uiKey = ExtractRadixKey(keyFunc, container[i]);

    ++counts[0][(uiKey) & 0xFFU];
    ++counts[1][(uiKey >> 8U) & 0xFFU];
    ++counts[2][(uiKey >> 16U) & 0xFFU];
    ++counts[3][(uiKey >> 24U) & 0xFFU];
    ++counts[4][(uiKey >> 32U) & 0xFFU];
    ++counts[5][(uiKey >> 40U) & 0xFFU];
    ++counts[6][(uiKey >> 48U) & 0xFFU];
    ++counts[7][(uiKey >> 56U) & 0xFFU];
  }

  // Phase 2: convert counts -> exclusive prefix-sum offsets, skipping passes where all elements fall into one bucket (counts[p][b] == uiCount).

  // offsets[p][b] = destination index for the first element of bucket b in pass p.
  xiiUInt32 offsets[8][256] = {};

  // passMask: bit p set means pass p is non-trivial and must be executed.
  xiiUInt8 uiPassMask = 0;

  for (xiiUInt32 uiPass = 0; uiPass < 8; ++uiPass)
  {
    xiiUInt32 uiRunning = 0;
    for (xiiUInt32 b = 0; b < 256; ++b)
    {
      offsets[uiPass][b] = uiRunning;

      // A bucket that alone holds all elements means this pass is a no-op.
      if (counts[uiPass][b] == uiCount)
        break; // uiPassMask bit stays 0, so remaining offsets don't matter.

      uiRunning += counts[uiPass][b];

      // If we reach here on any bucket other than bucket 0 filling everything, at least two buckets are non-empty -> pass is needed.
      if (counts[uiPass][b] != 0)
      {
        uiPassMask |= static_cast<xiiUInt8>(1U << uiPass);
      }
    }
  }

  // Phase 3: scatter passes, ping-ponging between container and scratchBuffer.

  bool bSourceIsContainer = true;

  for (xiiUInt32 uiPass = 0; uiPass < 8; ++uiPass)
  {
    if (!(uiPassMask & (1U << uiPass)))
      continue;

    const xiiUInt32 uiShift  = uiPass * 8U;
    xiiUInt32*      pOffsets = offsets[uiPass];

    if (bSourceIsContainer)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt64 uiKey  = ExtractRadixKey(keyFunc, container[i]);
        const xiiUInt8  uiByte = static_cast<xiiUInt8>((uiKey >> uiShift) & 0xFFU);

        xiiMemoryUtils::Copy(&scratchBuffer[pOffsets[uiByte]++], &container[i], 1U);
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt64 uiKey  = ExtractRadixKey(keyFunc, scratchBuffer[i]);
        const xiiUInt8  uiByte = static_cast<xiiUInt8>((uiKey >> uiShift) & 0xFFU);

        xiiMemoryUtils::Copy(&container[pOffsets[uiByte]++], &scratchBuffer[i], 1U);
      }
    }

    bSourceIsContainer = !bSourceIsContainer;
  }

  if (!bSourceIsContainer)
  {
    xiiMemoryUtils::Copy(xiiGetPtr(container), xiiGetPtr(scratchBuffer), uiCount);
  }
}
