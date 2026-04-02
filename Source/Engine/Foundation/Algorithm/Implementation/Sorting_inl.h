
template <typename Container, typename Comparer>
void xiiSorting::QuickSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  QuickSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::QuickSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  QuickSort(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::InsertionSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  InsertionSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::InsertionSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  InsertionSort(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::BubbleSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  BubbleSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::BubbleSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  BubbleSort(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  SelectionSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  SelectionSort(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSortStable(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  SelectionSortStable(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSortStable(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  SelectionSortStable(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::MergeSort(Container& ref_container, const Comparer& comparer)
{
  if (ref_container.IsEmpty())
    return;

  MergeSort(ref_container, 0, ref_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::MergeSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer)
{
  if (ref_arrayPtr.IsEmpty())
    return;

  MergeSort(ref_arrayPtr, 0, ref_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename ScratchContainer>
void xiiSorting::RadixSort(Container& ref_container, ScratchContainer& ref_scratchBuffer)
{
  if (ref_container.GetCount() < 2)
    return;

  using Element = std::remove_reference_t<decltype(ref_container[0])>;

  XII_ASSERT_DEV(ref_scratchBuffer.GetCount() >= ref_container.GetCount(), "Radix sort scratch buffer has {0} elements, but {1} are required.", ref_scratchBuffer.GetCount(), ref_container.GetCount());

  RadixSortInternal(ref_container, ref_scratchBuffer, DefaultRadixKeyExtractor<Element>());
}

template <typename Container, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSort(Container& ref_container, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc)
{
  if (ref_container.GetCount() < 2)
    return;

  XII_ASSERT_DEV(ref_scratchBuffer.GetCount() >= ref_container.GetCount(), "Radix sort scratch buffer has {0} elements, but {1} are required.", ref_scratchBuffer.GetCount(), ref_container.GetCount());

  RadixSortInternal(ref_container, ref_scratchBuffer, keyFunc);
}

template <typename T, typename ScratchContainer>
void xiiSorting::RadixSort(xiiArrayPtr<T>& ref_arrayPtr, ScratchContainer& ref_scratchBuffer)
{
  RadixSort(ref_arrayPtr, ref_scratchBuffer, DefaultRadixKeyExtractor<T>());
}

template <typename T, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSort(xiiArrayPtr<T>& ref_arrayPtr, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc)
{
  const xiiUInt32 uiCount = ref_arrayPtr.GetCount();
  if (uiCount < 2)
    return;

  xiiArrayPtr<T> scratchBuffer = ref_scratchBuffer;
  XII_ASSERT_DEV(scratchBuffer.GetCount() >= uiCount, "Radix sort scratch buffer has {0} elements, but {1} are required.", scratchBuffer.GetCount(), uiCount);

  T* pPrimary   = ref_arrayPtr.GetPtr();
  T* pSecondary = scratchBuffer.GetPtr();

  T* pSource = pPrimary;
  T* pDest   = pSecondary;

  for (xiiUInt32 pass = 0; pass < 8; ++pass)
  {
    xiiUInt32 uiCounts[256] = {0};

    const xiiUInt8 uiFirstByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, pSource[0]) >> (pass * 8U)) & 0xFFU);
    bool            bSingleBucket = true;

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, pSource[i]) >> (pass * 8U)) & 0xFFU);
      ++uiCounts[uiByte];
      bSingleBucket &= (uiByte == uiFirstByte);
    }

    if (bSingleBucket)
      continue;

    xiiUInt32 uiOffsets[256];
    uiOffsets[0] = 0;
    for (xiiUInt32 i = 1; i < 256; ++i)
    {
      uiOffsets[i] = uiOffsets[i - 1] + uiCounts[i - 1];
    }

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, pSource[i]) >> (pass * 8U)) & 0xFFU);
      pDest[uiOffsets[uiByte]++] = std::move(pSource[i]);
    }

    xiiMath::Swap(pSource, pDest);
  }

  if (pSource != pPrimary)
  {
    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      pPrimary[i] = std::move(pSource[i]);
    }
  }
}

template <typename Container, typename ScratchContainer, typename KeyFunc>
void xiiSorting::RadixSortInternal(Container& container, ScratchContainer& scratchBuffer, const KeyFunc& keyFunc)
{
  const xiiUInt32 uiCount = container.GetCount();
  if (uiCount < 2)
    return;

  XII_ASSERT_DEV(scratchBuffer.GetCount() >= uiCount, "Radix sort scratch buffer has {0} elements, but {1} are required.", scratchBuffer.GetCount(), uiCount);

  bool bSourceIsContainer = true;

  for (xiiUInt32 pass = 0; pass < 8; ++pass)
  {
    xiiUInt32 uiCounts[256] = {0};

    const xiiUInt8 uiFirstByte = bSourceIsContainer ?
      static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, container[0]) >> (pass * 8U)) & 0xFFU) :
      static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, scratchBuffer[0]) >> (pass * 8U)) & 0xFFU);

    bool bSingleBucket = true;

    if (bSourceIsContainer)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, container[i]) >> (pass * 8U)) & 0xFFU);
        ++uiCounts[uiByte];
        bSingleBucket &= (uiByte == uiFirstByte);
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, scratchBuffer[i]) >> (pass * 8U)) & 0xFFU);
        ++uiCounts[uiByte];
        bSingleBucket &= (uiByte == uiFirstByte);
      }
    }

    if (bSingleBucket)
      continue;

    xiiUInt32 uiOffsets[256];
    uiOffsets[0] = 0;
    for (xiiUInt32 i = 1; i < 256; ++i)
    {
      uiOffsets[i] = uiOffsets[i - 1] + uiCounts[i - 1];
    }

    if (bSourceIsContainer)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, container[i]) >> (pass * 8U)) & 0xFFU);
        scratchBuffer[uiOffsets[uiByte]++] = std::move(container[i]);
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiUInt8 uiByte = static_cast<xiiUInt8>((ExtractRadixKey(keyFunc, scratchBuffer[i]) >> (pass * 8U)) & 0xFFU);
        container[uiOffsets[uiByte]++] = std::move(scratchBuffer[i]);
      }
    }

    bSourceIsContainer = !bSourceIsContainer;
  }

  if (!bSourceIsContainer)
  {
    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      container[i] = std::move(scratchBuffer[i]);
    }
  }
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
        QuickSort(container, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(container, uiSecondHalfStartIndex, uiEndIndex, comparer);
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
void xiiSorting::QuickSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(arrayPtr, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const xiiUInt32 uiPivotIndex = Partition(ptr, uiStartIndex, uiEndIndex, comparer);

      xiiUInt32 uiFirstHalfEndIndex    = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      xiiUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, ptr[uiFirstHalfEndIndex], ptr[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, ptr[uiPivotIndex], ptr[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(arrayPtr, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(arrayPtr, uiSecondHalfStartIndex, uiEndIndex, comparer);
    }
  }
}

template <typename T, typename Comparer>
xiiUInt32 xiiSorting::Partition(T* ptr, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer)
{
  xiiUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, ptr[uiLeft], ptr[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, ptr[uiRight], ptr[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, ptr[uiLeft], ptr[uiPivotIndex]))
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

    if (DoCompare(comparer, ptr[uiLeft], ptr[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, ptr[uiRight], ptr[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  xiiMath::Swap(ptr[uiPivotIndex], ptr[uiRight]); // move pivot to right

  xiiUInt32 uiIndex = uiLeft;
  for (xiiUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, ptr[i], ptr[uiRight]))
    {
      xiiMath::Swap(ptr[i], ptr[uiIndex]);
      ++uiIndex;
    }
  }

  xiiMath::Swap(ptr[uiIndex], ptr[uiRight]); // move pivot back in place

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
void xiiSorting::InsertionSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiHoleIndex   = i;
    T         valueToInsert = std::move(ptr[uiHoleIndex]);

    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, valueToInsert, ptr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const xiiUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      xiiMemoryUtils::RelocateOverlapped(ptr + uiHoleIndex + 1, ptr + uiHoleIndex, uiMoveCount);
      xiiMemoryUtils::MoveConstruct(ptr + uiHoleIndex, std::move(valueToInsert));
    }
    else
    {
      ptr[uiHoleIndex] = std::move(valueToInsert);
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
void xiiSorting::BubbleSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T*   ptr       = arrayPtr.GetPtr();
  bool bIsSorted = false;

  while (!bIsSorted)
  {
    bIsSorted = true;

    for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
    {
      xiiUInt32 uiHoleIndex = i;
      if (DoCompare(comparer, ptr[uiHoleIndex], ptr[uiHoleIndex - 1]))
      {
        xiiMath::Swap(ptr[uiHoleIndex], ptr[uiHoleIndex - 1]);
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
void xiiSorting::SelectionSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex  = i;
    xiiUInt32 uiHoleIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, ptr[uiHoleIndexMin], ptr[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    xiiMath::Swap(ptr[uiHoleIndex], ptr[uiMinIndex]);
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
void xiiSorting::SelectionSortStable(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  for (xiiUInt32 i = uiStartIndex; i <= uiEndIndex; ++i)
  {
    xiiUInt32 uiMinIndex = i;

    // Find Minimum
    for (xiiUInt32 j = i + 1; j <= uiEndIndex; ++j)
    {
      // If Container at j is less than the current minimum, set Container current index (j) as the new minimum.
      xiiUInt32 uiHoleIndexMin = j;
      if (DoCompare(comparer, ptr[uiHoleIndexMin], ptr[uiMinIndex]))
      {
        uiMinIndex = j;
      }
    }

    // In a stable selection sort, loop through all remaining elements and swap in to preserve order.
    // This essentially 'shifts' the minimum farthest to the left of the container (to the current 'i').
    auto valueStorage = ptr[uiMinIndex];
    while (uiMinIndex > i)
    {
      ptr[uiMinIndex] = ptr[uiMinIndex - 1];
      --uiMinIndex;
    }
    ptr[i] = valueStorage;
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
void xiiSorting::MergeSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex >= uiEndIndex)
    return;

  // The same as uiStartIndex + (uiEndIndex - uiStartIndex) / 2.
  xiiUInt32 uiMiddleIndex = (uiStartIndex + uiEndIndex) >> 1;

  // Sort first and second halves and merge the result.
  MergeSort(arrayPtr, uiStartIndex, uiMiddleIndex, comparer);
  MergeSort(arrayPtr, uiMiddleIndex + 1, uiEndIndex, comparer);
  Merge(arrayPtr, uiStartIndex, uiMiddleIndex, uiEndIndex, comparer);
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
void xiiSorting::Merge(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  xiiUInt32 i{}, j{}, k{};
  xiiUInt32 uiRightSideSize = uiEndIndex - uiMiddleIndex;
  xiiUInt32 uiLeftSideSize  = uiMiddleIndex - uiStartIndex + 1;

  // Create temporary arrays to store container data.
  T* leftSideContainer  = new T[uiLeftSideSize];
  T* rightSideContainer = new T[uiRightSideSize];

  // Copy left side data.
  for (i = 0; i < uiLeftSideSize; ++i)
  {
    T value              = ptr[uiStartIndex + i];
    leftSideContainer[i] = value;
  }

  // Copy right side data.
  for (j = 0; j < uiRightSideSize; ++j)
  {
    T value               = ptr[uiMiddleIndex + 1 + j];
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
      ptr[k] = leftSideContainer[i];
      ++i;
    }
    else
    {
      ptr[k] = rightSideContainer[j];
      ++j;
    }

    ++k;
  }

  // Copy the remaining elements of the left side array if any.
  while (i < uiLeftSideSize)
  {
    ptr[k] = leftSideContainer[i];
    ++i;
    ++k;
  }

  // Copy the remaining elements of the right side array if any.
  while (j < uiRightSideSize)
  {
    ptr[k] = rightSideContainer[j];
    ++j;
    ++k;
  }

  // Free heap allocated temporary arrays.
  delete[] leftSideContainer, rightSideContainer;
}
