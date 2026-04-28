/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T, xiiUInt32 C>
xiiStaticArray<T, C>::xiiStaticArray()
{
  XII_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
}

template <typename T, xiiUInt32 C>
xiiStaticArray<T, C>::xiiStaticArray(const xiiStaticArray<T, C>& rhs)
{
  XII_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
  *this              = (xiiArrayPtr<const T>)rhs; // redirect this to the xiiArrayPtr version
}

template <typename T, xiiUInt32 C>
template <xiiUInt32 OtherCapacity>
xiiStaticArray<T, C>::xiiStaticArray(const xiiStaticArray<T, OtherCapacity>& rhs)
{
  static_assert(OtherCapacity <= C);

  XII_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = (xiiArrayPtr<const T>)rhs; // redirect this to the xiiArrayPtr version
}

template <typename T, xiiUInt32 C>
xiiStaticArray<T, C>::xiiStaticArray(const xiiArrayPtr<const T>& rhs)
{
  XII_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = rhs;
}

template <typename T, xiiUInt32 C>
xiiStaticArray<T, C>::~xiiStaticArray()
{
  this->Clear();
  XII_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE T* xiiStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE const T* xiiStaticArray<T, C>::GetStaticArray() const
{
  return reinterpret_cast<const T*>(m_Data);
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE void xiiStaticArray<T, C>::Reserve(xiiUInt32 uiCapacity)
{
  XII_IGNORE_UNUSED(uiCapacity);

  XII_ASSERT_DEV(uiCapacity <= C, "The static array has a fixed capacity of {0}, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE void xiiStaticArray<T, C>::operator=(const xiiStaticArray<T, C>& rhs)
{
  *this = (xiiArrayPtr<const T>)rhs; // redirect this to the xiiArrayPtr version
}

template <typename T, xiiUInt32 C>
template <xiiUInt32 OtherCapacity>
XII_ALWAYS_INLINE void xiiStaticArray<T, C>::operator=(const xiiStaticArray<T, OtherCapacity>& rhs)
{
  *this = (xiiArrayPtr<const T>)rhs; // redirect this to the xiiArrayPtr version
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE void xiiStaticArray<T, C>::operator=(const xiiArrayPtr<const T>& rhs)
{
  xiiArrayBase<T, xiiStaticArray<T, C>>::operator=(rhs);
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE T* xiiStaticArray<T, C>::GetElementsPtr()
{
  return GetStaticArray();
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE const T* xiiStaticArray<T, C>::GetElementsPtr() const
{
  return GetStaticArray();
}
