/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/ArrayPtr.h>

constexpr xiiUInt32 xiiSmallInvalidIndex = 0xFFFF;

/// \brief Implementation of a dynamically growing array with in-place storage and small memory overhead.
///
/// Best-case performance for the PushBack operation is in O(1) if the xiiHybridArray does not need to be expanded.
/// In the worst case, PushBack is in O(n).
/// Look-up is guaranteed to always be in O(1).
template <typename T, xiiUInt16 Size>
class xiiSmallArrayBase
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  XII_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  xiiSmallArrayBase();                                                                  // [tested]
  xiiSmallArrayBase(const xiiSmallArrayBase<T, Size>& other, xiiAllocator* pAllocator); // [tested]
  xiiSmallArrayBase(const xiiArrayPtr<const T>& other, xiiAllocator* pAllocator);       // [tested]
  xiiSmallArrayBase(xiiSmallArrayBase<T, Size>&& other, xiiAllocator* pAllocator);      // [tested]

  ~xiiSmallArrayBase(); // [tested]

  // Can't use regular assignment operators since we need to pass an allocator. Use CopyFrom or MoveFrom methods instead.
  void operator=(const xiiSmallArrayBase<T, Size>& rhs) = delete;
  void operator=(xiiSmallArrayBase<T, Size>&& rhs)      = delete;

  /// \brief Copies the data from some other array into this one.
  void CopyFrom(const xiiArrayPtr<const T>& other, xiiAllocator* pAllocator); // [tested]

  /// \brief Moves the data from some other array into this one.
  void MoveFrom(xiiSmallArrayBase<T, Size>&& other, xiiAllocator* pAllocator); // [tested]

  /// \brief Conversion to const xiiArrayPtr.
  operator xiiArrayPtr<const T>() const; // [tested]

  /// \brief Conversion to xiiArrayPtr.
  operator xiiArrayPtr<T>(); // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator==(const xiiSmallArrayBase<T, Size>& rhs) const; // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator<(const xiiSmallArrayBase<T, Size>& rhs) const; // [tested]
  bool operator<(const xiiArrayPtr<const T>& rhs) const;       // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](xiiUInt32 uiIndex) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](xiiUInt32 uiIndex); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(xiiUInt16 uiCount, xiiAllocator* pAllocator); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Constructs all new elements by copying the FillValue.
  void SetCount(xiiUInt16 uiCount, const T& fillValue, xiiAllocator* pAllocator); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  template <typename = void>                                               // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(xiiUInt16 uiCount, xiiAllocator* pAllocator); // [tested]

  /// \brief Ensures the container has at least \a uiCount elements. Ie. calls SetCount() if the container has fewer elements, does nothing
  /// otherwise.
  void EnsureCount(xiiUInt16 uiCount, xiiAllocator* pAllocator); // [tested]

  /// \brief Returns the number of active elements in the array.
  xiiUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the array.
  void Clear(); // [tested]

  /// \brief Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(const T& value, xiiUInt32 uiIndex, xiiAllocator* pAllocator); // [tested]

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(T&& value, xiiUInt32 uiIndex, xiiAllocator* pAllocator); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by shifting all following elements
  bool RemoveAndCopy(const T& value); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveAndSwap(const T& value); // [tested]

  /// \brief Removes the element at index and fills the gap by shifting all following elements
  void RemoveAtAndCopy(xiiUInt32 uiIndex, xiiUInt16 uiNumElements = 1); // [tested]

  /// \brief Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtAndSwap(xiiUInt32 uiIndex, xiiUInt16 uiNumElements = 1); // [tested]

  /// \brief Searches for the first occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  xiiUInt32 IndexOf(const T& value, xiiUInt32 uiStartIndex = 0) const; // [tested]

  /// \brief Searches for the last occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  xiiUInt32 LastIndexOf(const T& value, xiiUInt32 uiStartIndex = xiiSmallInvalidIndex) const; // [tested]

  /// \brief Grows the array by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(xiiAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(const T& value, xiiAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(T&& value, xiiAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(T&& value); // [tested]

  /// \brief Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const xiiArrayPtr<const T>& range, xiiAllocator* pAllocator); // [tested]

  /// \brief Removes count elements from the end of the array.
  void PopBack(xiiUInt32 uiCountToRemove = 1); // [tested]

  /// \brief Returns the last element of the array.
  T& PeekBack(); // [tested]

  /// \brief Returns the last element of the array.
  const T& PeekBack() const; // [tested]

  /// \brief Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Sort with default comparer
  void Sort(); // [tested]

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  T* GetData();

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  const T* GetData() const;

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<T> GetArrayPtr(); // [tested]

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<const T> GetArrayPtr() const; // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<typename xiiArrayPtr<T>::ByteType> GetByteArrayPtr(); // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  xiiArrayPtr<typename xiiArrayPtr<const T>::ByteType> GetByteArrayPtr() const; // [tested]

  /// \brief Expands the array so it can at least store the given capacity.
  void Reserve(xiiUInt16 uiCapacity, xiiAllocator* pAllocator); // [tested]

  /// \brief Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(xiiAllocator* pAllocator); // [tested]

  /// \brief Returns the reserved number of elements that the array can hold without reallocating.
  xiiUInt32 GetCapacity() const { return m_uiCapacity; }

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const; // [tested]

  using value_type             = T;
  using const_reference        = const T&;
  using const_iterator         = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator               = T*;
  using reverse_iterator       = reverse_pointer_iterator<T>;

  template <typename U>
  const U& GetUserData() const; // [tested]

  template <typename U>
  U& GetUserData(); // [tested]

protected:
  enum
  {
    CAPACITY_ALIGNMENT = 4
  };

  void SetCapacity(xiiUInt16 uiCapacity, xiiAllocator* pAllocator);

  T*       GetElementsPtr();
  const T* GetElementsPtr() const;

  xiiUInt16 m_uiCount    = 0;
  xiiUInt16 m_uiCapacity = Size;

  xiiUInt32 m_uiUserData = 0;

  union
  {
    struct alignas(alignof(T))
    {
      xiiUInt8 m_StaticData[Size * sizeof(T)];
    };

    T* m_pElements = nullptr;
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief \see xiiSmallArrayBase
template <typename T, xiiUInt16 Size, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiSmallArray : public xiiSmallArrayBase<T, Size>
{
  using SUPER = xiiSmallArrayBase<T, Size>;

public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  XII_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  xiiSmallArray();

  xiiSmallArray(const xiiSmallArray<T, Size, AllocatorWrapper>& other);
  explicit xiiSmallArray(const xiiArrayPtr<const T>& other);
  xiiSmallArray(xiiSmallArray<T, Size, AllocatorWrapper>&& other);

  ~xiiSmallArray();

  void operator=(const xiiSmallArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const xiiArrayPtr<const T>& rhs);
  void operator=(xiiSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept;

  void SetCount(xiiUInt16 uiCount);                     // [tested]
  void SetCount(xiiUInt16 uiCount, const T& fillValue); // [tested]
  void EnsureCount(xiiUInt16 uiCount);                  // [tested]

  template <typename = void>
  void SetCountUninitialized(xiiUInt16 uiCount); // [tested]

  void InsertAt(xiiUInt32 uiIndex, const T& value); // [tested]
  void InsertAt(xiiUInt32 uiIndex, T&& value);      // [tested]

  T&   ExpandAndGetRef();                                // [tested]
  void PushBack(const T& value);                         // [tested]
  void PushBack(T&& value);                              // [tested]
  void PushBackRange(const xiiArrayPtr<const T>& range); // [tested]

  void Reserve(xiiUInt16 uiCapacity);
  void Compact();
};

#include <Foundation/Containers/Implementation/SmallArray_inl.h>
