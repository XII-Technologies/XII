
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
