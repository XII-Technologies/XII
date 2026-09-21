/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/Containers/Implementation/ArrayIterator.h>

// This #include is quite vital, do not remove it!
#include <Foundation/Strings/FormatString.h>

#include <Foundation/Math/Math.h>

#if XII_ENABLED(XII_INTEROP_STL_SPAN)
#  include <span>
#endif

/// Value used by containers for indices to indicate an invalid index.
#ifndef xiiInvalidIndex
#  define xiiInvalidIndex 0xFFFFFFFFU
#endif

namespace xiiArrayPtrDetail
{
  template <typename U>
  struct ByteTypeHelper
  {
    using type = xiiUInt8;
  };

  template <typename U>
  struct ByteTypeHelper<const U>
  {
    using type = const xiiUInt8;
  };
} // namespace xiiArrayPtrDetail

/// This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
///
/// No data is deallocated at destruction, the xiiArrayPtr only allows for easier access.
template <typename T>
class xiiArrayPtr
{
  template <typename U>
  friend class xiiArrayPtr;

public:
  XII_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "xiiArrayPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "xiiArrayPtr<void> is not allowed (anymore)");

  using ByteType    = typename xiiArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType   = T;
  using PointerType = T*;

  /// Initializes the xiiArrayPtr to be empty.
  XII_ALWAYS_INLINE xiiArrayPtr() : // [tested]
    m_pPtr(nullptr), m_uiCount(0u)
  {
  }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  XII_ALWAYS_INLINE xiiArrayPtr(const xiiArrayPtr<T>& other) // [tested]
  {
    m_pPtr    = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Initializes the xiiArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline xiiArrayPtr(T* pPtr, xiiUInt32 uiCount) : // [tested]
    m_pPtr(pPtr), m_uiCount(uiCount)
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_pPtr == nullptr || m_uiCount == 0)
    {
      m_pPtr    = nullptr;
      m_uiCount = 0;
    }
  }

  /// Initializes the xiiArrayPtr to encapsulate the given array.
  template <size_t N>
  XII_ALWAYS_INLINE xiiArrayPtr(T (&staticArray)[N]) : // [tested]
    m_pPtr(staticArray), m_uiCount(static_cast<xiiUInt32>(N))
  {
  }

  /// Initializes the xiiArrayPtr to be a copy of \a other. No memory is allocated or copied.
  template <typename U>
  XII_ALWAYS_INLINE xiiArrayPtr(const xiiArrayPtr<U>& other) : // [tested]
    m_pPtr(other.m_pPtr), m_uiCount(other.m_uiCount)
  {
  }

#if XII_ENABLED(XII_INTEROP_STL_SPAN)
  template <typename U>
  XII_ALWAYS_INLINE xiiArrayPtr(const std::span<U>& other) :
    m_pPtr(other.data()), m_uiCount((xiiUInt32)other.size())
  {
  }

  operator std::span<const T>() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  operator std::span<T>()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<T> GetSpan()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<const T> GetSpan() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }
#endif

  /// Convert to const version.
  operator xiiArrayPtr<const T>() const { return xiiArrayPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); } // [tested]

  /// Copies the pointer and size of /a other. Does not allocate any data.
  XII_ALWAYS_INLINE void operator=(const xiiArrayPtr<T>& other) // [tested]
  {
    m_pPtr    = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Clears the array
  XII_ALWAYS_INLINE void Clear()
  {
    m_pPtr    = nullptr;
    m_uiCount = 0;
  }

  XII_ALWAYS_INLINE void operator=(std::nullptr_t) // [tested]
  {
    m_pPtr    = nullptr;
    m_uiCount = 0;
  }

  /// Returns the pointer to the array.
  XII_ALWAYS_INLINE PointerType GetPtr() const // [tested]
  {
    return m_pPtr;
  }

  /// Returns the pointer to the array.
  XII_ALWAYS_INLINE PointerType GetPtr() // [tested]
  {
    return m_pPtr;
  }

  /// Returns the pointer behind the last element of the array
  XII_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// Returns the pointer behind the last element of the array
  XII_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// Returns whether the array is empty.
  XII_ALWAYS_INLINE bool IsEmpty() const // [tested]
  {
    return GetCount() == 0;
  }

  /// Returns the number of elements in the array.
  XII_ALWAYS_INLINE xiiUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// Creates a sub-array from this array.
  XII_FORCE_INLINE xiiArrayPtr<T> GetSubArray(xiiUInt32 uiStart, xiiUInt32 uiCount) const // [tested]
  {
    // the first check is necessary to also detect errors when uiStart+uiCount would overflow
    XII_ASSERT_DEV(uiStart <= GetCount() && uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).", uiStart + uiCount, GetCount());
    return xiiArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  XII_FORCE_INLINE xiiArrayPtr<T> GetSubArray(xiiUInt32 uiStart) const // [tested]
  {
    XII_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return xiiArrayPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// Reinterprets this array as a byte array.
  XII_ALWAYS_INLINE xiiArrayPtr<const ByteType> ToByteArray() const
  {
    return xiiArrayPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// Reinterprets this array as a byte array.
  XII_ALWAYS_INLINE xiiArrayPtr<ByteType> ToByteArray() { return xiiArrayPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }

  /// Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  XII_ALWAYS_INLINE xiiArrayPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return xiiArrayPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  XII_ALWAYS_INLINE xiiArrayPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return xiiArrayPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// Index access.
  XII_FORCE_INLINE const ValueType& operator[](xiiUInt32 uiIndex) const // [tested]
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// Index access.
  XII_FORCE_INLINE ValueType& operator[](xiiUInt32 uiIndex) // [tested]
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// Compares the two arrays for equality.
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator==(const xiiArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return xiiMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  /// Compares the two arrays for equality.
  inline bool operator==(const xiiArrayPtr<T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return xiiMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  /// Compares the two arrays for less.
  inline bool operator<(const xiiArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return GetCount() < other.GetCount();

    for (xiiUInt32 i = 0; i < GetCount(); ++i)
    {
      if (GetPtr()[i] < other.GetPtr()[i])
        return true;

      if (other.GetPtr()[i] < GetPtr()[i])
        return false;
    }

    return false;
  }

  /// Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const xiiArrayPtr<const T>& other) // [tested]
  {
    XII_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    xiiMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  XII_ALWAYS_INLINE void Swap(xiiArrayPtr<T>& other)
  {
    ::xiiMath::Swap(m_pPtr, other.m_pPtr);
    ::xiiMath::Swap(m_uiCount, other.m_uiCount);
  }

  /// Checks whether the given value can be found in the array. O(n) complexity.
  XII_ALWAYS_INLINE bool Contains(const T& value) const // [tested]
  {
    return IndexOf(value) != xiiInvalidIndex;
  }

  /// Searches for the first occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  inline xiiUInt32 IndexOf(const T& value, xiiUInt32 uiStartIndex = 0) const // [tested]
  {
    for (xiiUInt32 i = uiStartIndex; i < m_uiCount; ++i)
    {
      if (xiiMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }

    return xiiInvalidIndex;
  }

  /// Searches for the last occurrence of the given value and returns its index or xiiInvalidIndex if not found.
  inline xiiUInt32 LastIndexOf(const T& value, xiiUInt32 uiStartIndex = xiiInvalidIndex) const // [tested]
  {
    for (xiiUInt32 i = ::xiiMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
    {
      if (xiiMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }
    return xiiInvalidIndex;
  }

  using const_iterator         = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator               = T*;
  using reverse_iterator       = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr;
  xiiUInt32   m_uiCount;
};

//////////////////////////////////////////////////////////////////////////

using xiiByteArrayPtr      = xiiArrayPtr<xiiUInt8>;
using xiiConstByteArrayPtr = xiiArrayPtr<const xiiUInt8>;

//////////////////////////////////////////////////////////////////////////

/// Helper function to create xiiArrayPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiArrayPtr<T> xiiMakeArrayPtr(T* pPtr, xiiUInt32 uiCount)
{
  return xiiArrayPtr<T>(pPtr, uiCount);
}

/// Helper function to create xiiArrayPtr from a static array the a size known at compile-time.
template <typename T, xiiUInt32 N>
XII_ALWAYS_INLINE xiiArrayPtr<T> xiiMakeArrayPtr(T (&staticArray)[N])
{
  return xiiArrayPtr<T>(staticArray);
}

/// Helper function to create xiiConstByteArrayPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiConstByteArrayPtr xiiMakeByteArrayPtr(const T* pPtr, xiiUInt32 uiCount)
{
  return xiiConstByteArrayPtr(static_cast<const xiiUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create xiiByteArrayPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiByteArrayPtr xiiMakeByteArrayPtr(T* pPtr, xiiUInt32 uiCount)
{
  return xiiByteArrayPtr(reinterpret_cast<xiiUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create xiiByteArrayPtr from a void pointer and a count.
XII_ALWAYS_INLINE xiiByteArrayPtr xiiMakeByteArrayPtr(void* pPtr, xiiUInt32 uiBytes)
{
  return xiiByteArrayPtr(reinterpret_cast<xiiUInt8*>(pPtr), uiBytes);
}

/// Helper function to create xiiConstByteArrayPtr from a const void pointer and a count.
XII_ALWAYS_INLINE xiiConstByteArrayPtr xiiMakeByteArrayPtr(const void* pPtr, xiiUInt32 uiBytes)
{
  return xiiConstByteArrayPtr(static_cast<const xiiUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename xiiArrayPtr<T>::iterator begin(xiiArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr();
}

template <typename T>
typename xiiArrayPtr<T>::const_iterator begin(const xiiArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename xiiArrayPtr<T>::const_iterator cbegin(const xiiArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename xiiArrayPtr<T>::reverse_iterator rbegin(xiiArrayPtr<T>& ref_container)
{
  return typename xiiArrayPtr<T>::reverse_iterator(ref_container.GetPtr() + ref_container.GetCount() - 1);
}

template <typename T>
typename xiiArrayPtr<T>::const_reverse_iterator rbegin(const xiiArrayPtr<T>& container)
{
  return typename xiiArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename xiiArrayPtr<T>::const_reverse_iterator crbegin(const xiiArrayPtr<T>& container)
{
  return typename xiiArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename xiiArrayPtr<T>::iterator end(xiiArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr() + ref_container.GetCount();
}

template <typename T>
typename xiiArrayPtr<T>::const_iterator end(const xiiArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename xiiArrayPtr<T>::const_iterator cend(const xiiArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename xiiArrayPtr<T>::reverse_iterator rend(xiiArrayPtr<T>& ref_container)
{
  return typename xiiArrayPtr<T>::reverse_iterator(ref_container.GetPtr() - 1);
}

template <typename T>
typename xiiArrayPtr<T>::const_reverse_iterator rend(const xiiArrayPtr<T>& container)
{
  return typename xiiArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename xiiArrayPtr<T>::const_reverse_iterator crend(const xiiArrayPtr<T>& container)
{
  return typename xiiArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}
