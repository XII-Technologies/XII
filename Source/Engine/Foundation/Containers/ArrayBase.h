/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>

#if XII_ENABLED(XII_INTEROP_STL_SPAN)
#  include <span>
#endif

/// Value used by containers for indices to indicate an invalid index.
#ifndef xiiInvalidIndex
#  define xiiInvalidIndex 0xFFFFFFFFU
#endif

/// Base class for all array containers. Implements all the basic functionality that only requires a pointer and the element count.
template <typename T, typename Derived>
class xiiArrayBase
{
public:
  using ValueType = T;

  /// Constructor.
  xiiArrayBase(); // [tested]

  /// Destructor.
  ~xiiArrayBase(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const xiiArrayPtr<const T>& rhs); // [tested]

  /// Conversion to const xiiArrayPtr.
  operator xiiArrayPtr<const T>() const; // [tested]

  /// Conversion to xiiArrayPtr.
  operator xiiArrayPtr<T>(); // [tested]

  /// Compares this array to another contiguous array type.
  bool operator==(const xiiArrayBase<T, Derived>& rhs) const; // [tested]

  /// Compares this array to another contiguous array type.
  bool operator<(const xiiArrayBase<T, Derived>& rhs) const; // [tested]

  /// Compares this array to another contiguous array type.
  bool operator<(const xiiArrayPtr<const T>& rhs) const; // [tested]

  /// Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](xiiUInt32 uiIndex) const; // [tested]

  /// Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](xiiUInt32 uiIndex); // [tested]

  /// Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(xiiUInt32 uiCount); // [tested]

  /// Resizes the array to have exactly uiCount elements. Constructs all new elements by copying the FillValue.
  void SetCount(xiiUInt32 uiCount, const T& fillValue); // [tested]

  /// Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  template <typename = void>                     // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(xiiUInt32 uiCount); // [tested]

  /// Ensures the container has at least \a uiCount elements. Ie. calls SetCount() if the container has fewer elements, does nothing
  /// otherwise.
  void EnsureCount(xiiUInt32 uiCount); // [tested]

  /// Returns the number of active elements in the array.
  xiiUInt32 GetCount() const; // [tested]

  /// Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the array.
  void Clear(); // [tested]

  /// Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// Inserts value at index by shifting all following elements.
  void InsertAt(xiiUInt32 uiIndex, const T& value); // [tested]

  /// Inserts value at index by shifting all following elements.
  void InsertAt(xiiUInt32 uiIndex, T&& value); // [tested]

  /// Inserts all elements in the range starting at the given index, shifting the elements after the index.
  void InsertRange(const xiiArrayPtr<const T>& range, xiiUInt32 uiIndex); // [tested]

  /// Removes the first occurrence of value and fills the gap by shifting all following elements
  bool RemoveAndCopy(const T& value); // [tested]

  /// Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveAndSwap(const T& value); // [tested]

  /// Removes the element at index and fills the gap by shifting all following elements
  void RemoveAtAndCopy(xiiUInt32 uiIndex, xiiUInt32 uiNumElements = 1); // [tested]

  /// Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtAndSwap(xiiUInt32 uiIndex, xiiUInt32 uiNumElements = 1); // [tested]

  /// Searches for the first occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  xiiUInt32 IndexOf(const T& value, xiiUInt32 uiStartIndex = 0) const; // [tested]

  /// Searches for the last occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  xiiUInt32 LastIndexOf(const T& value, xiiUInt32 uiStartIndex = xiiInvalidIndex) const; // [tested]

  /// Grows the array by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(); // [tested]

  /// Expands the array by N new items and returns a pointer to the first new one.
  T* ExpandBy(xiiUInt32 uiNumNewItems);

  /// Pushes value at the end of the array.
  void PushBack(const T& value); // [tested]

  /// Pushes value at the end of the array.
  void PushBack(T&& value); // [tested]

  /// Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(T&& value); // [tested]

  /// Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const xiiArrayPtr<const T>& range); // [tested]

  /// Removes count elements from the end of the array.
  void PopBack(xiiUInt32 uiCountToRemove = 1); // [tested]

  /// Returns the last element of the array.
  T& PeekBack(); // [tested]

  /// Returns the last element of the array.
  const T& PeekBack() const; // [tested]

  /// Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// Sort with default comparer
  void Sort(); // [tested]

  /// Returns a pointer to the array data, or nullptr if the array is empty.
  T* GetData();

  /// Returns a pointer to the array data, or nullptr if the array is empty.
  const T* GetData() const;

  /// Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<T> GetArrayPtr(); // [tested]

  /// Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<const T> GetArrayPtr() const; // [tested]

  /// Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<typename xiiArrayPtr<T>::ByteType> GetByteArrayPtr(); // [tested]

  /// Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<typename xiiArrayPtr<const T>::ByteType> GetByteArrayPtr() const; // [tested]

  /// Returns the reserved number of elements that the array can hold without reallocating.
  xiiUInt32 GetCapacity() const { return m_uiCapacity; }

  using const_iterator         = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator               = T*;
  using reverse_iterator       = reverse_pointer_iterator<T>;

#if XII_ENABLED(XII_INTEROP_STL_SPAN)
  operator std::span<const T>() const
  {
    return std::span(GetData(), static_cast<size_t>(GetCount()));
  }

  operator std::span<T>()
  {
    return std::span(GetData(), static_cast<size_t>(GetCount()));
  }

  std::span<T> GetSpan()
  {
    return std::span(GetData(), static_cast<size_t>(GetCount()));
  }

  std::span<const T> GetSpan() const
  {
    return std::span(GetData(), static_cast<size_t>(GetCount()));
  }
#endif

protected:
  void DoSwap(xiiArrayBase<T, Derived>& other);

  /// Element-type access to m_Data.
  T* m_pElements = nullptr;

  /// The number of elements used from the array.
  xiiUInt32 m_uiCount = 0;

  /// The number of elements which can be stored in the array without re-allocating.
  xiiUInt32 m_uiCapacity = 0;
};

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::iterator begin(xiiArrayBase<T, Derived>& ref_container)
{
  return ref_container.GetData();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_iterator begin(const xiiArrayBase<T, Derived>& container)
{
  return container.GetData();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_iterator cbegin(const xiiArrayBase<T, Derived>& container)
{
  return container.GetData();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::reverse_iterator rbegin(xiiArrayBase<T, Derived>& ref_container)
{
  return typename xiiArrayBase<T, Derived>::reverse_iterator(ref_container.GetData() + ref_container.GetCount() - 1);
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_reverse_iterator rbegin(const xiiArrayBase<T, Derived>& container)
{
  return typename xiiArrayBase<T, Derived>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_reverse_iterator crbegin(const xiiArrayBase<T, Derived>& container)
{
  return typename xiiArrayBase<T, Derived>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::iterator end(xiiArrayBase<T, Derived>& ref_container)
{
  return ref_container.GetData() + ref_container.GetCount();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_iterator end(const xiiArrayBase<T, Derived>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_iterator cend(const xiiArrayBase<T, Derived>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::reverse_iterator rend(xiiArrayBase<T, Derived>& ref_container)
{
  return typename xiiArrayBase<T, Derived>::reverse_iterator(ref_container.GetData() - 1);
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_reverse_iterator rend(const xiiArrayBase<T, Derived>& container)
{
  return typename xiiArrayBase<T, Derived>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, typename Derived>
typename xiiArrayBase<T, Derived>::const_reverse_iterator crend(const xiiArrayBase<T, Derived>& container)
{
  return typename xiiArrayBase<T, Derived>::const_reverse_iterator(container.GetData() - 1);
}

#include <Foundation/Containers/Implementation/ArrayBase_inl.h>
