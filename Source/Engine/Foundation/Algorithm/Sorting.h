
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

/// \brief This class provides implementations of different sorting algorithms.
class xiiSorting
{
public:
  /// \brief Sorts the elements in container using a in-place quick sort implementation (not stable).
  template <typename Container, typename Comparer>
  static void QuickSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using a in-place quick sort implementation (not stable).
  template <typename T, typename Comparer>
  static void QuickSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using insertion sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void InsertionSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using insertion sort (stable and in-place).
  template <typename T, typename Comparer>
  static void InsertionSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using bubble sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void BubbleSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using bubble sort (stable and in-place).
  template <typename T, typename Comparer>
  static void BubbleSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using selection sort (unstable and in-place).
  template <typename Container, typename Comparer>
  static void SelectionSort(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using selection sort (unstable and in-place).
  template <typename T, typename Comparer>
  static void SelectionSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using selection sort (stable and in-place).
  template <typename Container, typename Comparer>
  static void SelectionSortStable(Container& ref_container, const Comparer& comparer = Comparer()); // [tested]

  /// \brief Sorts the elements in the array using selection sort (stable and in-place).
  template <typename T, typename Comparer>
  static void SelectionSortStable(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [tested]


  /// \brief Sorts the elements in container using merge sort (stable and not in-place).
  template <typename Container, typename Comparer>
  static void MergeSort(Container& ref_container, const Comparer& comparer = Comparer()); // [untested]

  /// \brief Sorts the elements in the array using merge sort (stable and not in-place).
  template <typename T, typename Comparer>
  static void MergeSort(xiiArrayPtr<T>& ref_arrayPtr, const Comparer& comparer = Comparer()); // [untested]

private:
  enum
  {
    INSERTION_THRESHOLD = 16
  };

  // Perform comparison either with "Less(a,b)" (prefered) or with operator ()(a,b)
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
  static void QuickSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static xiiUInt32 Partition(T* ptr, xiiUInt32 uiLeft, xiiUInt32 uiRight, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void InsertionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void InsertionSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void BubbleSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void BubbleSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void SelectionSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void SelectionSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void SelectionSortStable(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void SelectionSortStable(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void MergeSort(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void MergeSort(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);


  template <typename Container, typename Comparer>
  static void Merge(Container& container, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);

  template <typename T, typename Comparer>
  static void Merge(xiiArrayPtr<T>& arrayPtr, xiiUInt32 uiStartIndex, xiiUInt32 uiMiddleIndex, xiiUInt32 uiEndIndex, const Comparer& comparer);
};

#include <Foundation/Algorithm/Implementation/Sorting_inl.h>
