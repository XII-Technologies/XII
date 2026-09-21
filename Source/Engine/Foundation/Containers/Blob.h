/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// This class encapsulates a blob's storage and it's size. It is recommended to use this class instead of directly working on the void* of the blob.
///
/// No data is deallocated at destruction, the xiiBlobPtr only allows for easier access.
template <typename T>
class xiiBlobPtr
{
public:
  XII_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "xiiBlobPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "xiiBlobPtr<void> is not allowed (anymore)");

  using ByteType    = typename xiiArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType   = T;
  using PointerType = T*;

  /// Initializes the xiiBlobPtr to be empty.
  xiiBlobPtr() = default;

  /// Initializes the xiiBlobPtr with the given pointer and number of elements. No memory is allocated or copied.
  template <typename U>
  inline xiiBlobPtr(U* pPtr, xiiUInt64 uiCount) :
    m_pPtr(pPtr), m_uiCount(uiCount)
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_pPtr == nullptr || m_uiCount == 0)
    {
      m_pPtr    = nullptr;
      m_uiCount = 0;
    }
  }

  /// Initializes the xiiBlobPtr to encapsulate the given array.
  template <size_t N>
  XII_ALWAYS_INLINE xiiBlobPtr(ValueType (&staticArray)[N]) :
    m_pPtr(staticArray), m_uiCount(static_cast<xiiUInt64>(N))
  {
  }

  /// Initializes the xiiBlobPtr to be a copy of \a other. No memory is allocated or copied.
  XII_ALWAYS_INLINE xiiBlobPtr(const xiiBlobPtr<T>& other) :
    m_pPtr(other.m_pPtr), m_uiCount(other.m_uiCount)
  {
  }

  /// Initializes the xiiBlobPtr to be a copy of \a other. No memory is allocated or copied.
  XII_ALWAYS_INLINE xiiBlobPtr(const xiiArrayPtr<T>& other) :
    m_pPtr(other.GetPtr()), m_uiCount(other.GetCount())
  {
  }

  /// Convert to const version.
  operator xiiBlobPtr<const T>() const { return xiiBlobPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  XII_ALWAYS_INLINE void operator=(const xiiBlobPtr<T>& other)
  {
    m_pPtr    = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  XII_ALWAYS_INLINE void operator=(const xiiArrayPtr<T>& other)
  {
    m_pPtr    = other.GetPtr();
    m_uiCount = other.GetCount();
  }

  /// Clears the array
  XII_ALWAYS_INLINE void Clear()
  {
    m_pPtr    = nullptr;
    m_uiCount = 0;
  }

  XII_ALWAYS_INLINE void operator=(std::nullptr_t)
  {
    m_pPtr    = nullptr;
    m_uiCount = 0;
  }

  /// Returns the pointer to the array.
  XII_ALWAYS_INLINE PointerType GetPtr() const { return m_pPtr; }

  /// Returns the pointer to the array.
  XII_ALWAYS_INLINE PointerType GetPtr() { return m_pPtr; }

  /// Returns the pointer behind the last element of the array
  XII_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// Returns the pointer behind the last element of the array
  XII_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// Returns whether the array is empty.
  XII_ALWAYS_INLINE bool IsEmpty() const { return GetCount() == 0; }

  /// Returns the number of elements in the array.
  XII_ALWAYS_INLINE xiiUInt64 GetCount() const { return m_uiCount; }

  /// Creates a sub-array from this array.
  XII_FORCE_INLINE xiiBlobPtr<T> GetSubArray(xiiUInt64 uiStart, xiiUInt64 uiCount) const // [tested]
  {
    XII_ASSERT_DEV(uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).", uiStart + uiCount, GetCount());
    return xiiBlobPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  XII_FORCE_INLINE xiiBlobPtr<T> GetSubArray(xiiUInt64 uiStart) const // [tested]
  {
    XII_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return xiiBlobPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// Reinterprets this array as a byte array.
  XII_ALWAYS_INLINE xiiBlobPtr<const ByteType> ToByteBlob() const
  {
    return xiiBlobPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// Reinterprets this array as a byte array.
  XII_ALWAYS_INLINE xiiBlobPtr<ByteType> ToByteBlob() { return xiiBlobPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }

  /// Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  XII_ALWAYS_INLINE xiiBlobPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return xiiBlobPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  XII_ALWAYS_INLINE xiiBlobPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return xiiBlobPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// Index access.
  XII_FORCE_INLINE const ValueType& operator[](xiiUInt64 uiIndex) const // [tested]
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// Index access.
  XII_FORCE_INLINE ValueType& operator[](xiiUInt64 uiIndex) // [tested]
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// Compares the two arrays for equality.
  inline bool operator==(const xiiBlobPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return xiiMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  /// Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const xiiBlobPtr<const T>& other) // [tested]
  {
    XII_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    xiiMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  XII_ALWAYS_INLINE void Swap(xiiBlobPtr<T>& other)
  {
    xiiMath::Swap(m_pPtr, other.m_pPtr);
    xiiMath::Swap(m_uiCount, other.m_uiCount);
  }

  using const_iterator         = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator               = T*;
  using reverse_iterator       = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr    = nullptr;
  xiiUInt64   m_uiCount = 0u;
};

//////////////////////////////////////////////////////////////////////////

using xiiByteBlobPtr      = xiiBlobPtr<xiiUInt8>;
using xiiConstByteBlobPtr = xiiBlobPtr<const xiiUInt8>;

//////////////////////////////////////////////////////////////////////////

/// Helper function to create xiiBlobPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiBlobPtr<T> xiiMakeBlobPtr(T* pPtr, xiiUInt64 uiCount)
{
  return xiiBlobPtr<T>(pPtr, uiCount);
}

/// Helper function to create xiiBlobPtr from a static array the a size known at compile-time.
template <typename T, xiiUInt64 N>
XII_ALWAYS_INLINE xiiBlobPtr<T> xiiMakeBlobPtr(T (&staticArray)[N])
{
  return xiiBlobPtr<T>(staticArray);
}

/// Helper function to create xiiConstByteBlobPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiConstByteBlobPtr xiiMakeByteBlobPtr(const T* pPtr, xiiUInt32 uiCount)
{
  return xiiConstByteBlobPtr(static_cast<const xiiUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create xiiByteBlobPtr from a pointer of some type and a count.
template <typename T>
XII_ALWAYS_INLINE xiiByteBlobPtr xiiMakeByteBlobPtr(T* pPtr, xiiUInt32 uiCount)
{
  return xiiByteBlobPtr(reinterpret_cast<xiiUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create xiiByteBlobPtr from a void pointer and a count.
XII_ALWAYS_INLINE xiiByteBlobPtr xiiMakeByteBlobPtr(void* pPtr, xiiUInt32 uiBytes)
{
  return xiiByteBlobPtr(reinterpret_cast<xiiUInt8*>(pPtr), uiBytes);
}

/// Helper function to create xiiConstByteBlobPtr from a const void pointer and a count.
XII_ALWAYS_INLINE xiiConstByteBlobPtr xiiMakeByteBlobPtr(const void* pPtr, xiiUInt32 uiBytes)
{
  return xiiConstByteBlobPtr(static_cast<const xiiUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename xiiBlobPtr<T>::iterator begin(xiiBlobPtr<T>& ref_container)
{
  return ref_container.GetPtr();
}

template <typename T>
typename xiiBlobPtr<T>::const_iterator begin(const xiiBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename xiiBlobPtr<T>::const_iterator cbegin(const xiiBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename xiiBlobPtr<T>::reverse_iterator rbegin(xiiBlobPtr<T>& ref_container)
{
  return typename xiiBlobPtr<T>::reverse_iterator(ref_container.GetPtr() + ref_container.GetCount() - 1);
}

template <typename T>
typename xiiBlobPtr<T>::const_reverse_iterator rbegin(const xiiBlobPtr<T>& container)
{
  return typename xiiBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename xiiBlobPtr<T>::const_reverse_iterator crbegin(const xiiBlobPtr<T>& container)
{
  return typename xiiBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename xiiBlobPtr<T>::iterator end(xiiBlobPtr<T>& ref_container)
{
  return ref_container.GetPtr() + ref_container.GetCount();
}

template <typename T>
typename xiiBlobPtr<T>::const_iterator end(const xiiBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename xiiBlobPtr<T>::const_iterator cend(const xiiBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename xiiBlobPtr<T>::reverse_iterator rend(xiiBlobPtr<T>& ref_container)
{
  return typename xiiBlobPtr<T>::reverse_iterator(ref_container.GetPtr() - 1);
}

template <typename T>
typename xiiBlobPtr<T>::const_reverse_iterator rend(const xiiBlobPtr<T>& container)
{
  return typename xiiBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename xiiBlobPtr<T>::const_reverse_iterator crend(const xiiBlobPtr<T>& container)
{
  return typename xiiBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

/// xiiBlob allows to store simple binary data larger than 4GB.
/// This storage class is used by xiiImage to allow processing of large textures for example.
/// In the current implementation the start of the allocated memory is guaranteed to be 64 byte aligned.
class XII_FOUNDATION_DLL xiiBlob
{
public:
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Default constructor. Does not allocate any memory.
  xiiBlob();

  /// Move constructor. Moves the storage pointer from the other blob to this blob.
  xiiBlob(xiiBlob&& other);

  /// Move assignment. Moves the storage pointer from the other blob to this blob.
  void operator=(xiiBlob&& rhs);

  /// Default destructor. Will call Clear() to deallocate the memory.
  ~xiiBlob();

  /// Sets the blob to the content of pSource.
  /// This will allocate the necessary memory if needed and then copy uiSize bytes from pSource.
  void SetFrom(const void* pSource, xiiUInt64 uiSize);

  /// Deallocates the memory allocated by this instance.
  void Clear();

  /// \bried Is data blob empty
  bool IsEmpty() const;

  /// Allocates uiCount bytes for storage in this object. The bytes will have undefined content.
  void SetCountUninitialized(xiiUInt64 uiCount);

  /// Convenience method to clear the content of the blob to all 0 bytes.
  void ZeroFill();

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  xiiBlobPtr<T> GetBlobPtr()
  {
    return xiiBlobPtr<T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  xiiBlobPtr<const T> GetBlobPtr() const
  {
    return xiiBlobPtr<const T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  xiiByteBlobPtr GetByteBlobPtr() { return xiiByteBlobPtr(reinterpret_cast<xiiUInt8*>(m_pStorage), m_uiSize); }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  xiiConstByteBlobPtr GetByteBlobPtr() const { return xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(m_pStorage), m_uiSize); }

private:
  void*     m_pStorage = nullptr;
  xiiUInt64 m_uiSize   = 0;
};
