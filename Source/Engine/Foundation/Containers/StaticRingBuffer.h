/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/StaticArray.h>

/// A ring-buffer container that will use a static array of a given capacity to cycle through elements.
///
/// If you need a dynamic ring-buffer, use a xiiDeque.
template <typename T, xiiUInt32 Capacity>
class xiiStaticRingBuffer
{
public:
  static_assert(Capacity > 1, "ORLY?");

  /// Constructs an empty ring-buffer.
  xiiStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  xiiStaticRingBuffer(const xiiStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Destructs all remaining elements.
  ~xiiStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  void operator=(const xiiStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Compares two ring-buffers for equality.
  bool operator==(const xiiStaticRingBuffer<T, Capacity>& rhs) const; // [tested]

  /// Appends an element at the end of the ring-buffer. Asserts that CanAppend() is true.
  void PushBack(const T& element); // [tested]

  /// Appends an element at the end of the ring-buffer. Asserts that CanAppend() is true.
  void PushBack(T&& element); // [tested]

  /// Accesses the latest element in the ring-buffer.
  T& PeekBack(); // [tested]

  /// Accesses the latest element in the ring-buffer.
  const T& PeekBack() const; // [tested]

  /// Removes the oldest element from the ring-buffer.
  void PopFront(xiiUInt32 uiElements = 1); // [tested]

  /// Accesses the oldest element in the ring-buffer.
  const T& PeekFront() const; // [tested]

  /// Accesses the oldest element in the ring-buffer.
  T& PeekFront(); // [tested]

  /// Accesses the n-th element in the ring-buffer.
  const T& operator[](xiiUInt32 uiIndex) const; // [tested]

  /// Accesses the n-th element in the ring-buffer.
  T& operator[](xiiUInt32 uiIndex); // [tested]

  /// Returns the number of elements that are currently in the ring-buffer.
  xiiUInt32 GetCount() const; // [tested]

  /// Returns true if the ring-buffer currently contains no elements.
  bool IsEmpty() const; // [tested]

  /// Returns true, if the ring-buffer can store at least uiElements additional elements.
  bool CanAppend(xiiUInt32 uiElements = 1); // [tested]

  /// Destructs all elements in the ring-buffer.
  void Clear(); // [tested]

private:
  T* GetStaticArray();

  /// The fixed size array.
  struct alignas(alignof(T))
  {
    xiiUInt8 m_Data[Capacity * sizeof(T)];
  };

  T*        m_pElements;
  xiiUInt32 m_uiCount;
  xiiUInt32 m_uiFirstElement;
};

#include <Foundation/Containers/Implementation/StaticRingBuffer_inl.h>
