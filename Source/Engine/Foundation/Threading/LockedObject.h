/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// Provides access to an object while managing a lock (e.g. a mutex) that ensures that during its lifetime the access to the object
/// happens under the lock.
template <typename T, typename O>
class xiiLockedObject
{
public:
  XII_ALWAYS_INLINE explicit xiiLockedObject(T& ref_lock, O* pObject) :
    m_pLock(&ref_lock), m_pObject(pObject)
  {
    m_pLock->Lock();
  }

  xiiLockedObject() = default;

  XII_ALWAYS_INLINE xiiLockedObject(xiiLockedObject<T, O>&& rhs) { *this = std::move(rhs); }

  xiiLockedObject(const xiiLockedObject<T, O>& rhs) = delete;

  void operator=(const xiiLockedObject<T, O>&& rhs)
  {
    if (m_pLock)
    {
      m_pLock->Unlock();
    }

    m_pLock       = rhs.m_pLock;
    rhs.m_pLock   = nullptr;
    m_pObject     = rhs.m_pObject;
    rhs.m_pObject = nullptr;
  }

  void operator=(const xiiLockedObject<T, O>& rhs) = delete;

  XII_ALWAYS_INLINE ~xiiLockedObject()
  {
    if (m_pLock)
    {
      m_pLock->Unlock();
    }
  }

  /// Whether the encapsulated object exists at all or is nullptr
  XII_ALWAYS_INLINE bool isValid() const { return m_pObject != nullptr; }

  O* Borrow() { return m_pObject; }

  const O* Borrow() const { return m_pObject; }

  O* operator->() { return m_pObject; }

  const O* operator->() const { return m_pObject; }

  O& operator*() { return *m_pObject; }

  const O& operator*() const { return *m_pObject; }

  bool operator==(const O* rhs) const { return m_pObject == rhs; }

  bool operator!() const { return m_pObject == nullptr; }

  operator bool() const { return m_pObject != nullptr; }

private:
  mutable T* m_pLock   = nullptr;
  mutable O* m_pObject = nullptr;
};
