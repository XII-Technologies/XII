/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/ArrayBase.h>

/// Wraps a C-style array, which has a fixed size at compile-time, with a more convenient interface.
///
/// xiiStaticArray can be used to create a fixed size array, either on the stack or as a class member.
/// Additionally it allows to use that array as a 'cache', i.e. not all its elements need to be constructed.
/// As such it can be used whenever a fixed size array is sufficient, but a more powerful interface is desired,
/// and when the number of elements in an array is dynamic at run-time, but always capped at a fixed limit.
template <typename T, xiiUInt32 Capacity>
class xiiStaticArray : public xiiArrayBase<T, xiiStaticArray<T, Capacity>>
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  XII_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  /// Creates an empty array.
  xiiStaticArray(); // [tested]

  /// Creates a copy of the given array.
  xiiStaticArray(const xiiStaticArray<T, Capacity>& rhs); // [tested]

  /// Creates a copy of the given array.
  template <xiiUInt32 OtherCapacity>
  xiiStaticArray(const xiiStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// Creates a copy of the given array.
  explicit xiiStaticArray(const xiiArrayPtr<const T>& rhs); // [tested]

  /// Destroys all objects.
  ~xiiStaticArray(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const xiiStaticArray<T, Capacity>& rhs); // [tested]

  /// Copies the data from some other contiguous array into this one.
  template <xiiUInt32 OtherCapacity>
  void operator=(const xiiStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const xiiArrayPtr<const T>& rhs); // [tested]

  /// For the static array Reserve is a no-op. However the function checks if the requested capacity is below or equal to the static capacity.
  void Reserve(xiiUInt32 uiCapacity);

protected:
  T*       GetElementsPtr();
  const T* GetElementsPtr() const;
  friend class xiiArrayBase<T, xiiStaticArray<T, Capacity>>;

private:
  T*       GetStaticArray();
  const T* GetStaticArray() const;

  /// The fixed size array.
  struct alignas(alignof(T))
  {
    xiiUInt8 m_Data[Capacity * sizeof(T)];
  };

  friend class xiiArrayBase<T, xiiStaticArray<T, Capacity>>;
};

// TODO static_assert with a ',' in the expression does not work
// static_assert(xiiGetTypeClass< xiiStaticArray<xiiInt32, 4> >::value == 2, "static array is not memory relocatable");

#include <Foundation/Containers/Implementation/StaticArray_inl.h>
