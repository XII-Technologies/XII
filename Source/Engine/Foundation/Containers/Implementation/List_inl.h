/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Math/Math.h>

// **** ListElement ****

template <typename T>
xiiListBase<T>::ListElementBase::ListElementBase() :
  m_pPrev(nullptr), m_pNext(nullptr)
{
}

template <typename T>
xiiListBase<T>::ListElement::ListElement(const T& data) :
  m_Data(data)
{
}

// **** xiiListBase ****

template <typename T>
xiiListBase<T>::xiiListBase(xiiAllocator* pAllocator) :
  m_End(reinterpret_cast<ListElement*>(&m_Last)), m_uiCount(0), m_Elements(pAllocator), m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);
}

template <typename T>
xiiListBase<T>::xiiListBase(const xiiListBase<T>& cc, xiiAllocator* pAllocator) :
  m_End(reinterpret_cast<ListElement*>(&m_Last)), m_uiCount(0), m_Elements(pAllocator), m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);

  operator=(cc);
}

template <typename T>
xiiListBase<T>::~xiiListBase()
{
  Clear();
}

template <typename T>
void xiiListBase<T>::operator=(const xiiListBase<T>& cc)
{
  Clear();
  Insert(GetIterator(), cc.GetIterator(), cc.GetEndIterator());
}

template <typename T>
typename xiiListBase<T>::ListElement* xiiListBase<T>::AcquireNode()
{
  ListElement* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode               = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pNext;
  }

  xiiMemoryUtils::Construct<SkipTrivialTypes, ListElement>(pNode, 1);
  return pNode;
}

template <typename T>
void xiiListBase<T>::ReleaseNode(ListElement* pNode)
{
  xiiMemoryUtils::Destruct<ListElement>(pNode, 1);

  if (pNode == &m_Elements.PeekBack())
  {
    m_Elements.PopBack();
  }
  else if (pNode == &m_Elements.PeekFront())
  {
    m_Elements.PopFront();
  }
  else
  {
    pNode->m_pNext      = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}


template <typename T>
XII_ALWAYS_INLINE typename xiiListBase<T>::Iterator xiiListBase<T>::GetIterator()
{
  return Iterator(m_First.m_pNext);
}

template <typename T>
XII_ALWAYS_INLINE typename xiiListBase<T>::Iterator xiiListBase<T>::GetEndIterator()
{
  return m_End;
}

template <typename T>
XII_ALWAYS_INLINE typename xiiListBase<T>::ConstIterator xiiListBase<T>::GetIterator() const
{
  return ConstIterator(m_First.m_pNext);
}

template <typename T>
XII_ALWAYS_INLINE typename xiiListBase<T>::ConstIterator xiiListBase<T>::GetEndIterator() const
{
  return m_End;
}

template <typename T>
XII_ALWAYS_INLINE xiiUInt32 xiiListBase<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
XII_ALWAYS_INLINE bool xiiListBase<T>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename T>
void xiiListBase<T>::Clear()
{
  if (!IsEmpty())
  {
    Remove(GetIterator(), GetEndIterator());
  }

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();
}

template <typename T>
XII_FORCE_INLINE void xiiListBase<T>::Compact()
{
  m_Elements.Compact();
}

template <typename T>
XII_FORCE_INLINE T& xiiListBase<T>::PeekFront()
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
XII_FORCE_INLINE T& xiiListBase<T>::PeekBack()
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}

template <typename T>
XII_FORCE_INLINE const T& xiiListBase<T>::PeekFront() const
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
XII_FORCE_INLINE const T& xiiListBase<T>::PeekBack() const
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}


template <typename T>
XII_ALWAYS_INLINE T& xiiListBase<T>::PushBack()
{
  return *Insert(GetEndIterator());
}

template <typename T>
XII_ALWAYS_INLINE void xiiListBase<T>::PushBack(const T& element)
{
  Insert(GetEndIterator(), element);
}

template <typename T>
XII_ALWAYS_INLINE T& xiiListBase<T>::PushFront()
{
  return *Insert(GetIterator());
}

template <typename T>
XII_ALWAYS_INLINE void xiiListBase<T>::PushFront(const T& element)
{
  Insert(GetIterator(), element);
}

template <typename T>
XII_FORCE_INLINE void xiiListBase<T>::PopBack()
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_Last.m_pPrev));
}

template <typename T>
void xiiListBase<T>::PopFront()
{
  XII_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_First.m_pNext));
}

template <typename T>
typename xiiListBase<T>::Iterator xiiListBase<T>::Insert(const Iterator& pos)
{
  XII_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* pElement = AcquireNode();

  pElement->m_pNext = pos.m_pElement;
  pElement->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = pElement;
  pos.m_pElement->m_pPrev          = pElement;

  return Iterator(pElement);
}

template <typename T>
typename xiiListBase<T>::Iterator xiiListBase<T>::Insert(const Iterator& pos, const T& data)
{
  XII_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* pElement = AcquireNode();
  pElement->m_Data      = data;

  pElement->m_pNext = pos.m_pElement;
  pElement->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = pElement;
  pos.m_pElement->m_pPrev          = pElement;

  return Iterator(pElement);
}

template <typename T>
void xiiListBase<T>::Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last)
{
  XII_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");
  XII_ASSERT_DEV(first.m_pElement != nullptr, "The iterator (first) is invalid.");
  XII_ASSERT_DEV(last.m_pElement != nullptr, "The iterator (last) is invalid.");

  while (first != last)
  {
    Insert(pos, *first);
    ++first;
  }
}

template <typename T>
typename xiiListBase<T>::Iterator xiiListBase<T>::Remove(const Iterator& pos)
{
  XII_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  XII_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ListElement* pPrev = pos.m_pElement->m_pPrev;
  ListElement* pNext = pos.m_pElement->m_pNext;

  pPrev->m_pNext = pNext;
  pNext->m_pPrev = pPrev;

  ReleaseNode(pos.m_pElement);

  return Iterator(pNext);
}

template <typename T>
typename xiiListBase<T>::Iterator xiiListBase<T>::Remove(Iterator first, const Iterator& last)
{
  XII_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  XII_ASSERT_DEV(first.m_pElement != nullptr, "The iterator (first) is invalid.");
  XII_ASSERT_DEV(last.m_pElement != nullptr, "The iterator (last) is invalid.");

  while (first != last)
  {
    first = Remove(first);
  }

  return last;
}

/*! If uiNewSize is smaller than the size of the list, elements are popped from the back, until the desired size is reached.
    If uiNewSize is larger than the size of the list, default-constructed elements are appended to the list, until the desired size is reached.
*/
template <typename T>
void xiiListBase<T>::SetCount(xiiUInt32 uiNewSize)
{
  while (m_uiCount > uiNewSize)
  {
    PopBack();
  }

  while (m_uiCount < uiNewSize)
  {
    PushBack();
  }
}

template <typename T>
bool xiiListBase<T>::operator==(const xiiListBase<T>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (*itLhs != *itRhs)
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

template <typename T, typename A>
xiiList<T, A>::xiiList() :
  xiiListBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
xiiList<T, A>::xiiList(xiiAllocator* pAllocator) :
  xiiListBase<T>(pAllocator)
{
}

template <typename T, typename A>
xiiList<T, A>::xiiList(const xiiList<T, A>& other) :
  xiiListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
xiiList<T, A>::xiiList(const xiiListBase<T>& other) :
  xiiListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
void xiiList<T, A>::operator=(const xiiList<T, A>& rhs)
{
  xiiListBase<T>::operator=(rhs);
}

template <typename T, typename A>
void xiiList<T, A>::operator=(const xiiListBase<T>& rhs)
{
  xiiListBase<T>::operator=(rhs);
}
