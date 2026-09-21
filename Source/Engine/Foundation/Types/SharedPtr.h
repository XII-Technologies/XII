/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/UniquePtr.h>

/// A Shared ptr manages a shared object and destroys that object when no one references it anymore. The managed object must derive
/// from xiiRefCounted.
template <typename T>
class xiiSharedPtr
{
public:
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Creates an empty shared ptr.
  xiiSharedPtr();

  /// Creates a shared ptr from a freshly created instance through XII_NEW or XII_DEFAULT_NEW.
  template <typename U>
  xiiSharedPtr(const xiiInternal::NewInstance<U>& instance);

  /// Creates a shared ptr from a pointer and an allocator. The passed allocator will be used to destroy the instance when the shared
  /// ptr goes out of scope.
  template <typename U>
  xiiSharedPtr(U* pInstance, xiiAllocator* pAllocator);

  /// Copy constructs a shared ptr from another. Both will hold a reference to the managed object afterwards.
  xiiSharedPtr(const xiiSharedPtr<T>& other);

  /// Copy constructs a shared ptr from another. Both will hold a reference to the managed object afterwards.
  template <typename U>
  xiiSharedPtr(const xiiSharedPtr<U>& other);

  /// Move constructs a shared ptr from another. The other shared ptr will be empty afterwards.
  template <typename U>
  xiiSharedPtr(xiiSharedPtr<U>&& other);

  /// Move constructs a shared ptr from a unique ptr. The unique ptr will be empty afterwards.
  template <typename U>
  xiiSharedPtr(xiiUniquePtr<U>&& other);

  /// Initialization with nullptr to be able to return nullptr in functions that return shared ptr.
  xiiSharedPtr(std::nullptr_t);

  /// Destroys the managed object using the stored allocator if no one else references it anymore.
  ~xiiSharedPtr();

  /// Sets the shared ptr from a freshly created instance through XII_NEW or XII_DEFAULT_NEW.
  template <typename U>
  xiiSharedPtr<T>& operator=(const xiiInternal::NewInstance<U>& instance);

  /// Sets the shared ptr from another. Both will hold a reference to the managed object afterwards.
  xiiSharedPtr<T>& operator=(const xiiSharedPtr<T>& other);

  /// Sets the shared ptr from another. Both will hold a reference to the managed object afterwards.
  template <typename U>
  xiiSharedPtr<T>& operator=(const xiiSharedPtr<U>& other);

  /// Move assigns a shared ptr from another. The other shared ptr will be empty afterwards.
  template <typename U>
  xiiSharedPtr<T>& operator=(xiiSharedPtr<U>&& other);

  /// Move assigns a shared ptr from a unique ptr. The unique ptr will be empty afterwards.
  template <typename U>
  xiiSharedPtr<T>& operator=(xiiUniquePtr<U>&& other);

  /// Assigns a nullptr to the shared ptr. Same as Reset.
  xiiSharedPtr<T>& operator=(std::nullptr_t);

  /// Borrows the managed object. The shared ptr stays unmodified.
  T* Borrow() const;

  /// Destroys the managed object if no one else references it anymore and resets the shared ptr.
  void Clear();

  /// Provides access to the managed object.
  T& operator*() const;

  /// Provides access to the managed object.
  T* operator->() const;

  /// Provides access to the managed object.
  operator const T*() const;

  /// Provides access to the managed object.
  operator T*();

  /// Returns true if there is managed object and false if the shared ptr is empty.
  explicit operator bool() const;

  /// Compares the shared ptr against another shared ptr.
  bool operator==(const xiiSharedPtr<T>& rhs) const;

  /// Compares the shared ptr against another shared ptr.
  std::strong_ordering operator<=>(const xiiSharedPtr<T>& rhs) const;

  /// Compares the shared ptr against nullptr.
  bool operator==(std::nullptr_t) const;

  /// Compares the shared ptr against nullptr.
  std::strong_ordering operator<=>(std::nullptr_t) const;

  /// Returns a copy of this, as a xiiSharedPtr<DERIVED>. Downcasts the stored pointer (using static_cast).
  ///
  /// Does not check whether the cast would be valid, that is all your responsibility.
  template <typename DERIVED>
  xiiSharedPtr<DERIVED> Downcast() const
  {
    return xiiSharedPtr<DERIVED>(static_cast<DERIVED*>(m_pInstance), m_pAllocator);
  }

private:
  template <typename U>
  friend class xiiSharedPtr;

  void AddReferenceIfValid();
  void ReleaseReferenceIfValid();

  T*            m_pInstance;
  xiiAllocator* m_pAllocator;
};

#include <Foundation/Types/Implementation/SharedPtr_inl.h>
