#pragma once

template <typename T, xiiUInt32 C>
xiiStaticRingBuffer<T, C>::xiiStaticRingBuffer()
{
  m_pElements      = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount        = 0;
}

template <typename T, xiiUInt32 C>
xiiStaticRingBuffer<T, C>::xiiStaticRingBuffer(const xiiStaticRingBuffer<T, C>& rhs)
{
  m_pElements      = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount        = 0;

  *this = rhs;
}

template <typename T, xiiUInt32 C>
xiiStaticRingBuffer<T, C>::~xiiStaticRingBuffer()
{
  Clear();
}

template <typename T, xiiUInt32 C>
void xiiStaticRingBuffer<T, C>::operator=(const xiiStaticRingBuffer<T, C>& rhs)
{
  Clear();

  for (xiiUInt32 i = 0; i < rhs.GetCount(); ++i)
    PushBack(rhs[i]);
}

template <typename T, xiiUInt32 C>
bool xiiStaticRingBuffer<T, C>::operator==(const xiiStaticRingBuffer<T, C>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < m_uiCount; ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, xiiUInt32 C>
void xiiStaticRingBuffer<T, C>::PushBack(const T& element)
{
  XII_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const xiiUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  xiiMemoryUtils::CopyConstruct(&m_pElements[uiLastElement], element, 1);
  ++m_uiCount;
}

template <typename T, xiiUInt32 C>
void xiiStaticRingBuffer<T, C>::PushBack(T&& element)
{
  XII_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const xiiUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  xiiMemoryUtils::MoveConstruct(&m_pElements[uiLastElement], std::move(element));
  ++m_uiCount;
}

template <typename T, xiiUInt32 C>
T& xiiStaticRingBuffer<T, C>::PeekBack()
{
  XII_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const xiiUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, xiiUInt32 C>
const T& xiiStaticRingBuffer<T, C>::PeekBack() const
{
  XII_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const xiiUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, xiiUInt32 C>
void xiiStaticRingBuffer<T, C>::PopFront(xiiUInt32 uiElements)
{
  XII_ASSERT_DEV(m_uiCount >= uiElements, "The ring-buffer contains {0} elements, cannot remove {1} elements from it.", m_uiCount, uiElements);

  while (uiElements > 0)
  {
    xiiMemoryUtils::Destruct(&m_pElements[m_uiFirstElement], 1);
    ++m_uiFirstElement;
    m_uiFirstElement %= C;
    --m_uiCount;

    --uiElements;
  }
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE const T& xiiStaticRingBuffer<T, C>::PeekFront() const
{
  XII_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE T& xiiStaticRingBuffer<T, C>::PeekFront()
{
  XII_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE const T& xiiStaticRingBuffer<T, C>::operator[](xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, xiiUInt32 C>
XII_FORCE_INLINE T& xiiStaticRingBuffer<T, C>::operator[](xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE xiiUInt32 xiiStaticRingBuffer<T, C>::GetCount() const
{
  return m_uiCount;
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE bool xiiStaticRingBuffer<T, C>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE bool xiiStaticRingBuffer<T, C>::CanAppend(xiiUInt32 uiElements)
{
  return (m_uiCount + uiElements) <= C;
}

template <typename T, xiiUInt32 C>
void xiiStaticRingBuffer<T, C>::Clear()
{
  while (!IsEmpty())
    PopFront();
}

template <typename T, xiiUInt32 C>
XII_ALWAYS_INLINE T* xiiStaticRingBuffer<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}
