/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Algorithm/RadixKeyExtractor.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

/// This class provides implementations of different sorting algorithms.
class xiiSorting
{
public:
  /// Sorts the elements in container using a in-place quick sort implementation (not stable).
  template <typename Container, typename Comparer>
  static void QuickSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// Sorts the elements in the array using a in-place quick sort implementation (not stable).
  template <typename T, typename Comparer>
  static void QuickSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [tested]


  /// Sorts the elements in container using insertion sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void InsertionSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// Sorts the elements in the array using insertion sort (stable and in-place).
  template <typename T, typename Comparer>
  static void InsertionSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [tested]


  /// Sorts the elements in container using bubble sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void BubbleSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// Sorts the elements in the array using bubble sort (stable and in-place).
  template <typename T, typename Comparer>
  static void BubbleSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [tested]


  /// Sorts the elements in container using selection sort (unstable and in-place).
  template <typename Container, typename Comparer>
  static void SelectionSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// Sorts the elements in the array using selection sort (unstable and in-place).
  template <typename T, typename Comparer>
  static void SelectionSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [tested]


  /// Sorts the elements in container using selection sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void SelectionSortStable(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// Sorts the elements in the array using selection sort (stable and in-place).
  template <typename T, typename Comparer>
  static void SelectionSortStable(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [tested]


  /// Sorts the elements in container using merge sort (stable and not in-place).
  template <typename Container, typename Comparer>
  static void MergeSort(Container& ref_container, const Comparer& comparer = Comparer()); // [untested]

  /// Sorts the elements in the array using merge sort (stable and not in-place).
  template <typename T, typename Comparer>
  static void MergeSort(xiiArrayPtr<T>& ref_pArray, const Comparer& comparer = Comparer()); // [untested]


  /// Sorts the elements in container by an unsigned 64-bit radix key (stable), reusing external scratch memory.
  template <typename Container, typename ScratchContainer, typename KeyFunc>
  static void RadixSort(Container& ref_container, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc); // [tested]

  /// Sorts the elements in the array by an unsigned 64-bit radix key (stable), reusing external scratch memory.
  template <typename T, typename ScratchContainer, typename KeyFunc>
  static void RadixSort(xiiArrayPtr<T>& ref_pArray, ScratchContainer& ref_scratchBuffer, const KeyFunc& keyFunc); // [tested]

private:
  enum
  {
    INSERTION_THRESHOLD = 16
  };

  template <typename Container>
  XII_ALWAYS_INLINE constexpr static auto xiiGetPtr(Container& c) -> decltype(c.GetData())
  {
    return c.GetData();
  }

  template <typename T>
  XII_ALWAYS_INLINE constexpr static T* xiiGetPtr(xiiArrayPtr<T>& p)
  {
    return p.GetPtr();
  }


  // Perform comparison either with "Less(a,b)" (preferred) or with operator ()(a,b)
  template <typename Element, typename Comparer>
  XII_ALWAYS_INLINE constexpr static auto DoCompare(const Comparer& comparer, const Element& a, const Element& b, xiiInt32) -> decltype(comparer.Less(a, b))
  {
    return comparer.Less(a, b);
  }
  template <typename Element, typename Comparer>
  XII_ALWAYS_INLINE constexpr static auto DoCompare(const Comparer& comparer, const Element& a, const Element& b, long) -> decltype(comparer(a, b))
  {
    return comparer(a, b);
  }
  template <typename Element, typename Comparer>
  XII_ALWAYS_INLINE constexpr static bool DoCompare(const Comparer& comparer, const Element& a, const Element& b)
  {
    // Int/Long is used to prefer the Int version if both are available.
    // (Kudos to http://stackoverflow.com/a/9154394/5347927 where I've learned this trick)
    return DoCompare(comparer, a, b, 0);
  }


  template <typename Container, typename Comparer>
  static void QuickSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename Container, typename Comparer>
  static xiiUInt32 Partition(Container& container, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer);


  template <typename T, typename Comparer>
  static void QuickSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static xiiUInt32 Partition(T* pPtr, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void InsertionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void InsertionSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void BubbleSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void BubbleSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void SelectionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void SelectionSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void SelectionSortStable(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void SelectionSortStable(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void MergeSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void MergeSort(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void Merge(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void Merge(xiiArrayPtr<T>& pArray, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename ScratchContainer, typename KeyFunc>
  static void RadixSortInternal(Container& container, ScratchContainer& scratchBuffer, const KeyFunc& keyFunc);
};

#include <Foundation/Algorithm/Implementation/Sorting_inl.h>
