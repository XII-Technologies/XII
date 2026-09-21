/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Memory/TemporaryAllocator.h>
#include <Foundation/Types/PointerWithFlags.h>

/// Implementation of a dynamically growing array.
///
/// Best-case performance for the PushBack operation is O(1) if the xiiDynamicArray doesn't need to be expanded.
/// In the worst case, PushBack is O(n).
/// Look-up is guaranteed to always be O(1).
template <typename T>
class xiiDynamicArrayBase : public xiiArrayBase<T, xiiDynamicArrayBase<T>>
{
protected:
  /// Creates an empty array. Does not allocate any data yet.
  explicit xiiDynamicArrayBase(xiiAllocator* pAllocator); // [tested]

  xiiDynamicArrayBase(T* pInplaceStorage, xiiUInt32 uiCapacity, xiiAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  xiiDynamicArrayBase(const xiiDynamicArrayBase<T>& other, xiiAllocator* pAllocator); // [tested]

  /// Moves the given array into this one.
  xiiDynamicArrayBase(xiiDynamicArrayBase<T>&& other, xiiAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  xiiDynamicArrayBase(const xiiArrayPtr<const T>& other, xiiAllocator* pAllocator); // [tested]

  /// Destructor.
  ~xiiDynamicArrayBase(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const xiiDynamicArrayBase<T>& rhs); // [tested]

  /// Moves the data from some other contiguous array into this one.
  void operator=(xiiDynamicArrayBase<T>&& rhs) noexcept; // [tested]

  T*       GetElementsPtr();
  const T* GetElementsPtr() const;

  friend class xiiArrayBase<T, xiiDynamicArrayBase<T>>;

public:
  /// Expands the array so it can at least store the given capacity.
  void Reserve(xiiUInt32 uiCapacity); // [tested]

  /// Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// Returns the allocator that is used by this instance.
  xiiAllocator* GetAllocator() const { return const_cast<xiiAllocator*>(m_pAllocator.GetPtr()); }

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const; // [tested]

  /// swaps the contents of this array with another one
  void Swap(xiiDynamicArrayBase<T>& other); // [tested]

private:
  enum Storage
  {
    Owned    = 0,
    External = 1
  };

  xiiPointerWithFlags<xiiAllocator, 1> m_pAllocator;

  enum
  {
    CAPACITY_ALIGNMENT = 16
  };

  void SetCapacity(xiiUInt32 uiCapacity);
};

/// \see xiiDynamicArrayBase
template <typename T, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiDynamicArray : public xiiDynamicArrayBase<T>
{
public:
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  xiiDynamicArray();
  explicit xiiDynamicArray(xiiAllocator* pAllocator);

  xiiDynamicArray(const xiiDynamicArray<T, AllocatorWrapper>& other);
  xiiDynamicArray(const xiiDynamicArrayBase<T>& other);
  explicit xiiDynamicArray(const xiiArrayPtr<const T>& other);

  xiiDynamicArray(xiiDynamicArray<T, AllocatorWrapper>&& other);
  xiiDynamicArray(xiiDynamicArrayBase<T>&& other);

  void operator=(const xiiDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const xiiDynamicArrayBase<T>& rhs);
  void operator=(const xiiArrayPtr<const T>& rhs);

  void operator=(xiiDynamicArray<T, AllocatorWrapper>&& rhs) noexcept;
  void operator=(xiiDynamicArrayBase<T>&& rhs) noexcept;

protected:
  xiiDynamicArray(T* pInplaceStorage, xiiUInt32 uiCapacity, xiiAllocator* pAllocator) :
    xiiDynamicArrayBase<T>(pInplaceStorage, uiCapacity, pAllocator)
  {
  }
};

/// A dynamic array that uses the temporary allocator.
///
/// This is ideal for temporary arrays that are only used within a short scope.
/// The temp allocator is optimized for short-lived allocations and can be more efficient than the default allocator for this use case.
template <typename T>
class xiiTemporaryArray : public xiiDynamicArray<T>
{
public:
  xiiTemporaryArray();

  void operator=(const xiiDynamicArrayBase<T>& rhs);
  void operator=(const xiiArrayPtr<const T>& rhs);

  void operator=(xiiDynamicArrayBase<T>&& rhs) noexcept;
};

/// Overload of xiiMakeArrayPtr for const dynamic arrays of pointer pointing to const type.
template <typename T, typename AllocatorWrapper>
xiiArrayPtr<const T* const> xiiMakeArrayPtr(const xiiDynamicArray<T*, AllocatorWrapper>& dynArray);

/// Overload of xiiMakeArrayPtr for const dynamic arrays.
template <typename T, typename AllocatorWrapper>
xiiArrayPtr<const T> xiiMakeArrayPtr(const xiiDynamicArray<T, AllocatorWrapper>& dynArray);

/// Overload of xiiMakeArrayPtr for dynamic arrays.
template <typename T, typename AllocatorWrapper>
xiiArrayPtr<T> xiiMakeArrayPtr(xiiDynamicArray<T, AllocatorWrapper>& ref_dynArray);

static_assert(xiiGetTypeClass<xiiDynamicArray<xiiInt32>>::value == 2, "Dynamic array is not memory relocatable.");

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>
