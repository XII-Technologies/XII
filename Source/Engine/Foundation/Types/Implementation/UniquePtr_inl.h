
template <typename T>
XII_ALWAYS_INLINE xiiUniquePtr<T>::xiiUniquePtr() = default;

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUniquePtr<T>::xiiUniquePtr(const xiiInternal::NewInstance<U>& instance)
{
  m_pInstance  = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUniquePtr<T>::xiiUniquePtr(U* pInstance, xiiAllocator* pAllocator)
{
  m_pInstance  = pInstance;
  m_pAllocator = pAllocator;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUniquePtr<T>::xiiUniquePtr(xiiUniquePtr<U>&& other)
{
  m_pInstance  = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance  = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
XII_ALWAYS_INLINE xiiUniquePtr<T>::xiiUniquePtr(std::nullptr_t)
{
}

template <typename T>
XII_ALWAYS_INLINE xiiUniquePtr<T>::~xiiUniquePtr()
{
  Clear();
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUniquePtr<T>& xiiUniquePtr<T>::operator=(const xiiInternal::NewInstance<U>& instance)
{
  Clear();

  m_pInstance  = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  return *this;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUniquePtr<T>& xiiUniquePtr<T>::operator=(xiiUniquePtr<U>&& other)
{
  Clear();

  m_pInstance  = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance  = nullptr;
  other.m_pAllocator = nullptr;

  return *this;
}

template <typename T>
XII_ALWAYS_INLINE xiiUniquePtr<T>& xiiUniquePtr<T>::operator=(std::nullptr_t)
{
  Clear();

  return *this;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiUniquePtr<T>::Release()
{
  T* pInstance = m_pInstance;

  m_pInstance  = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiUniquePtr<T>::Release(xiiAllocator*& out_pAllocator)
{
  T* pInstance   = m_pInstance;
  out_pAllocator = m_pAllocator;

  m_pInstance  = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiUniquePtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE void xiiUniquePtr<T>::Clear()
{
  if (m_pAllocator != nullptr)
  {
    XII_DELETE(m_pAllocator, m_pInstance);
  }

  m_pInstance  = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
XII_ALWAYS_INLINE T& xiiUniquePtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiUniquePtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE xiiUniquePtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiUniquePtr<T>::operator==(const xiiUniquePtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE std::strong_ordering xiiUniquePtr<T>::operator<=>(const xiiUniquePtr<T>& rhs) const
{
  return m_pInstance <=> rhs.m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiUniquePtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
XII_ALWAYS_INLINE std::strong_ordering xiiUniquePtr<T>::operator<=>(std::nullptr_t) const
{
  return m_pInstance <=> nullptr;
}

//////////////////////////////////////////////////////////////////////////
// free functions

template <typename T>
XII_ALWAYS_INLINE bool operator==(const xiiUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
XII_ALWAYS_INLINE bool operator==(const xiiUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
XII_ALWAYS_INLINE bool operator==(const T* lhs, const xiiUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
XII_ALWAYS_INLINE bool operator==(T* lhs, const xiiUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}
