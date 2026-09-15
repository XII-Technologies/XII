/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr()
{
  m_pInstance  = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(const xiiInternal::NewInstance<U>& instance)
{
  m_pInstance  = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(U* pInstance, xiiAllocator* pAllocator)
{
  m_pInstance  = pInstance;
  m_pAllocator = pAllocator;

  AddReferenceIfValid();
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(const xiiSharedPtr<T>& other)
{
  m_pInstance  = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(const xiiSharedPtr<U>& other)
{
  m_pInstance  = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(xiiSharedPtr<U>&& other)
{
  m_pInstance  = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance  = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(xiiUniquePtr<U>&& other)
{
  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::xiiSharedPtr(std::nullptr_t)
{
  m_pInstance  = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::~xiiSharedPtr()
{
  ReleaseReferenceIfValid();
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(const xiiInternal::NewInstance<U>& instance)
{
  ReleaseReferenceIfValid();

  m_pInstance  = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();

  return *this;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(const xiiSharedPtr<T>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance  = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(const xiiSharedPtr<U>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance  = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(xiiSharedPtr<U>&& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance  = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    other.m_pInstance  = nullptr;
    other.m_pAllocator = nullptr;
  }

  return *this;
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(xiiUniquePtr<U>&& other)
{
  ReleaseReferenceIfValid();

  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();

  return *this;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>& xiiSharedPtr<T>::operator=(std::nullptr_t)
{
  ReleaseReferenceIfValid();

  return *this;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiSharedPtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE void xiiSharedPtr<T>::Clear()
{
  ReleaseReferenceIfValid();
}

template <typename T>
XII_ALWAYS_INLINE T& xiiSharedPtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE T* xiiSharedPtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::operator const T*() const
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::operator T*()
{
  return m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE xiiSharedPtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiSharedPtr<T>::operator==(const xiiSharedPtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE std::strong_ordering xiiSharedPtr<T>::operator<=>(const xiiSharedPtr<T>& rhs) const
{
  return m_pInstance <=> rhs.m_pInstance;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiSharedPtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
XII_ALWAYS_INLINE std::strong_ordering xiiSharedPtr<T>::operator<=>(std::nullptr_t) const
{
  return m_pInstance <=> nullptr;
}

template <typename T>
XII_ALWAYS_INLINE void xiiSharedPtr<T>::AddReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->AddRef();
  }
}

template <typename T>
XII_ALWAYS_INLINE void xiiSharedPtr<T>::ReleaseReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    if (m_pInstance->ReleaseRef() == 0)
    {
      auto pNonConstInstance = const_cast<typename xiiTypeTraits<T>::NonConstType*>(m_pInstance);
      XII_ASSERT_DEV(m_pAllocator != nullptr, "Faux shared pointers should never be released.");
      XII_DELETE(m_pAllocator, pNonConstInstance);
    }

    m_pInstance  = nullptr;
    m_pAllocator = nullptr;
  }
}
