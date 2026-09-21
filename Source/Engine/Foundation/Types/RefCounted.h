/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Threading/AtomicUtils.h>

class XII_FOUNDATION_DLL xiiRefCountingImpl
{
public:
  /// Constructor
  xiiRefCountingImpl() = default; // [tested]

  xiiRefCountingImpl(const xiiRefCountingImpl& rhs) // [tested]
  {
    XII_IGNORE_UNUSED(rhs);

    // Do not copy the reference count.
  }

  void operator=(const xiiRefCountingImpl& rhs) // [tested]
  {
    XII_IGNORE_UNUSED(rhs);

    // Do not copy the reference count.
  }

  /// Increments the reference counter. Returns the new reference count.
  inline xiiUInt32 AddRef() const // [tested]
  {
    return xiiAtomicUtils::Increment(m_uiRefCount);
  }

  /// Decrements the reference counter. Returns the new reference count.
  inline xiiUInt32 ReleaseRef() const // [tested]
  {
    return xiiAtomicUtils::Decrement(m_uiRefCount);
  }

  /// Returns true if the reference count is greater than 0, false otherwise.
  inline bool IsReferenced() const // [tested]
  {
    return m_uiRefCount > 0;
  }

  /// Returns the current reference count.
  inline xiiUInt32 GetRefCount() const // [tested]
  {
    return m_uiRefCount;
  }

private:
  mutable xiiUInt32 m_uiRefCount = 0U; ///< Stores the current reference count.
};

/// Base class for reference counted objects.
class XII_FOUNDATION_DLL xiiRefCounted : public xiiRefCountingImpl
{
public:
  /// Adds a virtual destructor.
  virtual ~xiiRefCounted() = default;
};

/// Stores a pointer to a reference counted object and automatically increases / decreases the reference count.
///
/// Note that no automatic deletion etc. happens, this is just to have shared base functionality for reference
/// counted objects. The actual action which, should happen once an object is no longer referenced, obliges
/// to the system that is using the objects.
template <typename T>
class xiiScopedRefPointer
{
public:
  /// Constructor.
  xiiScopedRefPointer() :
    m_pReferencedObject(nullptr)
  {
  }

  /// Constructor, increases the ref count of the given object.
  xiiScopedRefPointer(T* pReferencedObject) :
    m_pReferencedObject(pReferencedObject)
  {
    AddReferenceIfValid();
  }

  xiiScopedRefPointer(const xiiScopedRefPointer<T>& other)
  {
    m_pReferencedObject = other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// Destructor - releases the reference on the ref-counted object (if there is one).
  ~xiiScopedRefPointer() { ReleaseReferenceIfValid(); }

  /// Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly
  /// assigned object.
  void operator=(T* pNewReference)
  {
    if (pNewReference == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = pNewReference;

    AddReferenceIfValid();
  }

  /// Assignment operator, decreases the ref count of the currently referenced object and increases the ref count of the newly
  /// assigned object.
  void operator=(const xiiScopedRefPointer<T>& other)
  {
    if (other.m_pReferencedObject == m_pReferencedObject)
      return;

    ReleaseReferenceIfValid();

    m_pReferencedObject = other.m_pReferencedObject;

    AddReferenceIfValid();
  }

  /// Returns the referenced object (may be nullptr).
  operator const T*() const { return m_pReferencedObject; }

  /// Returns the referenced object (may be nullptr).
  operator T*() { return m_pReferencedObject; }

  /// Returns the referenced object (may be nullptr).
  const T* operator->() const
  {
    XII_ASSERT_DEV(m_pReferencedObject != nullptr, "Pointer is nullptr.");
    return m_pReferencedObject;
  }

  /// Returns the referenced object (may be nullptr)
  T* operator->()
  {
    XII_ASSERT_DEV(m_pReferencedObject != nullptr, "Pointer is nullptr.");
    return m_pReferencedObject;
  }

private:
  /// Internal helper function to add a reference on the current object (if != nullptr)
  inline void AddReferenceIfValid()
  {
    if (m_pReferencedObject != nullptr)
    {
      m_pReferencedObject->AddRef();
    }
  }

  /// Internal helper function to release a reference on the current object (if != nullptr)
  inline void ReleaseReferenceIfValid()
  {
    if (m_pReferencedObject != nullptr)
    {
      m_pReferencedObject->ReleaseRef();
    }
  }

  T* m_pReferencedObject; ///< Stores a pointer to the referenced object
};


template <typename TYPE>
class xiiRefCountedContainer : public xiiRefCounted
{
public:
  TYPE m_Content;
};
