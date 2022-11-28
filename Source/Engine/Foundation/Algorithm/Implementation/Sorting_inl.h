
template <typename Container, typename Comparer>
void xiiSorting::QuickSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  QuickSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::QuickSort(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  QuickSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::InsertionSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  InsertionSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::InsertionSort(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  InsertionSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::BubbleSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  BubbleSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::BubbleSort(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  BubbleSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  SelectionSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSort(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  SelectionSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::SelectionSortStable(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  SelectionSortStable(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::SelectionSortStable(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  SelectionSortStable(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void xiiSorting::MergeSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  MergeSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void xiiSorting::MergeSort(xiiArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  MergeSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
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
      uiPivotIndex = uiPivotIndex;
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
      uiPivotIndex = uiPivotIndex;
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
      uiPivotIndex = uiPivotIndex;
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
      uiPivotIndex = uiPivotIndex;
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
  bool sorted = false;

  while (!sorted)
  {
    sorted = true;

    for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
    {
      xiiUInt32 uiHoleIndex = i;
      if (DoCompare(comparer, container[uiHoleIndex], container[uiHoleIndex - 1]))
      {
        xiiMath::Swap(container[uiHoleIndex], container[uiHoleIndex - 1]);
        sorted = false;
      }
    }
  }
}

template <typename T, typename Comparer>
void xiiSorting::BubbleSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer)
{
  T*   ptr    = arrayPtr.GetPtr();
  bool sorted = false;

  while (!sorted)
  {
    sorted = true;

    for (xiiUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
    {
      xiiUInt32 uiHoleIndex = i;
      if (DoCompare(comparer, ptr[uiHoleIndex], ptr[uiHoleIndex - 1]))
      {
        xiiMath::Swap(ptr[uiHoleIndex], ptr[uiHoleIndex - 1]);
        sorted = false;
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
