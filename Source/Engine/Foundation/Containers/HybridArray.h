#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief A hybrid array uses in-place storage to handle the first few elements without any allocation. It dynamically resizes when more elements are needed.
///
/// It is often more efficient to use a hybrid array, rather than a dynamic array, when the number of needed elements is typically low or when the array is used only temporarily. In this case costly allocations can often be prevented entirely.
/// However, if the number of elements is unpredictable or usually very large, prefer a dynamic array, to avoid wasting (stack) memory for a hybrid array that is rarely large enough to be used.
/// The xiiHybridArray is derived from xiiDynamicArray and can therefore be passed to functions that expect a xiiDynamicArray, even for output.
template <typename T, xiiUInt32 Size, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiHybridArray : public xiiDynamicArray<T, AllocatorWrapper>
{
public:
  /// \brief Creates an empty array. Does not allocate any data yet.
  xiiHybridArray(); // [tested]

  /// \brief Creates an empty array. Does not allocate any data yet.
  explicit xiiHybridArray(xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  xiiHybridArray(const xiiHybridArray<T, Size, AllocatorWrapper>& other); // [tested]

  /// \brief Creates a copy of the given array.
  explicit xiiHybridArray(const xiiArrayPtr<const T>& other); // [tested]

  /// \brief Moves the given array.
  xiiHybridArray(xiiHybridArray<T, Size, AllocatorWrapper>&& other) noexcept; // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const xiiHybridArray<T, Size, AllocatorWrapper>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const xiiArrayPtr<const T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(xiiHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept; // [tested]

protected:
  /// \brief The fixed size array.
  struct alignas(alignof(T))
  {
    xiiUInt8 m_StaticData[Size * sizeof(T)];
  };

  XII_ALWAYS_INLINE T* GetStaticArray() { return reinterpret_cast<T*>(m_StaticData); }

  XII_ALWAYS_INLINE const T* GetStaticArray() const { return reinterpret_cast<const T*>(m_StaticData); }
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>
