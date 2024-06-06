#pragma once

#include <Foundation/Math/Math.h>

#define STACK_SIZE 64

// ***** Const Iterator *****
template <typename KeyType, typename Comparer>
template <bool REVERSE>
void xiiSetBase<KeyType, Comparer>::IteratorBase<REVERSE>::Advance(xiiInt32 dir0, xiiInt32 dir1)
{
  if (m_pElement == nullptr)
  {
    XII_ASSERT_DEBUG(m_pElement != nullptr, "The Iterator is invalid (end).");
    return;
  }

  // if this element has a right child, go there and then search for the left most child of that
  if (m_pElement->m_pLink[dir1] != m_pElement->m_pLink[dir1]->m_pLink[dir1])
  {
    m_pElement = m_pElement->m_pLink[dir1];

    while (m_pElement->m_pLink[dir0] != m_pElement->m_pLink[dir0]->m_pLink[dir0])
      m_pElement = m_pElement->m_pLink[dir0];

    return;
  }

  // if this element has a parent and this element is that parents left child, go directly to the parent
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) && (m_pElement->m_pParent->m_pLink[dir0] == m_pElement))
  {
    m_pElement = m_pElement->m_pParent;
    return;
  }

  // if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) && (m_pElement->m_pParent->m_pLink[dir1] == m_pElement))
  {
    while (m_pElement->m_pParent->m_pLink[dir1] == m_pElement)
      m_pElement = m_pElement->m_pParent;

    // if we are at the root node..
    if ((m_pElement->m_pParent == nullptr) || (m_pElement->m_pParent == m_pElement->m_pParent->m_pParent))
    {
      m_pElement = nullptr;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = nullptr;
}

template <typename KeyType, typename Comparer>
template <bool REVERSE>
void xiiSetBase<KeyType, Comparer>::IteratorBase<REVERSE>::Next()
{
  if constexpr (REVERSE)
  {
    Advance(1, 0);
  }
  else
  {
    Advance(0, 1);
  }
}

template <typename KeyType, typename Comparer>
template <bool REVERSE>
void xiiSetBase<KeyType, Comparer>::IteratorBase<REVERSE>::Prev()
{
  if constexpr (REVERSE)
  {
    Advance(0, 1);
  }
  else
  {
    Advance(1, 0);
  }
}

// ***** xiiSetBase *****

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Constructor()
{
  m_uiCount = 0;

  m_NilNode.m_uiLevel  = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pFreeElementStack = nullptr;
  m_pRoot             = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename Comparer>
xiiSetBase<KeyType, Comparer>::xiiSetBase(const Comparer& comparer, xiiAllocatorBase* pAllocator) :
  m_Elements(pAllocator), m_Comparer(comparer)
{
  Constructor();
}

template <typename KeyType, typename Comparer>
xiiSetBase<KeyType, Comparer>::xiiSetBase(const xiiSetBase<KeyType, Comparer>& cc, xiiAllocatorBase* pAllocator) :
  m_Elements(pAllocator)
{
  Constructor();

  operator=(cc);
}

template <typename KeyType, typename Comparer>
xiiSetBase<KeyType, Comparer>::~xiiSetBase()
{
  Clear();
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::operator=(const xiiSetBase<KeyType, Comparer>& rhs)
{
  Clear();

  for (Iterator it = rhs.GetIterator(); it.IsValid(); ++it)
    Insert(it.Key());
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Clear()
{
  for (Iterator it = GetIterator(); it.IsValid(); ++it)
    xiiMemoryUtils::Destruct<Node>(it.m_pElement, 1);

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();

  m_uiCount = 0;

  m_NilNode.m_uiLevel  = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename Comparer>
XII_ALWAYS_INLINE bool xiiSetBase<KeyType, Comparer>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename KeyType, typename Comparer>
XII_ALWAYS_INLINE xiiUInt32 xiiSetBase<KeyType, Comparer>::GetCount() const
{
  return m_uiCount;
}


template <typename KeyType, typename Comparer>
XII_ALWAYS_INLINE typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::GetIterator() const
{
  return Iterator(GetLeftMost());
}

template <typename KeyType, typename Comparer>
XII_ALWAYS_INLINE typename xiiSetBase<KeyType, Comparer>::ReverseIterator xiiSetBase<KeyType, Comparer>::GetReverseIterator() const
{
  return ReverseIterator(GetRightMost());
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::GetLeftMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[0] != &m_NilNode)
    pNode = pNode->m_pLink[0];

  return pNode;
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::GetRightMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[1] != &m_NilNode)
    pNode = pNode->m_pLink[1];

  return pNode;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::Internal_Find(const CompatibleKeyType& key) const
{
  Node* pNode = m_pRoot;

  while (pNode != &m_NilNode) // && (pNode->m_Key != key))
  {
    const xiiInt32 dir  = (xiiInt32)m_Comparer.Less(pNode->m_Key, key);
    const xiiInt32 dir2 = (xiiInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      break;

    pNode = pNode->m_pLink[dir];
  }

  if (pNode == &m_NilNode)
    return nullptr;

  return pNode;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::Find(const CompatibleKeyType& key) const
{
  return Iterator(Internal_Find(key));
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE bool xiiSetBase<KeyType, Comparer>::Contains(const CompatibleKeyType& key) const
{
  return Internal_Find(key) != nullptr;
}

template <typename KeyType, typename Comparer>
XII_FORCE_INLINE bool xiiSetBase<KeyType, Comparer>::ContainsSet(const xiiSetBase<KeyType, Comparer>& operand) const
{
  for (const KeyType& key : operand)
  {
    if (!Contains(key))
      return false;
  }

  return true;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::Internal_LowerBound(const CompatibleKeyType& key) const
{
  Node* pNode        = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while (pNode != &m_NilNode)
  {
    const xiiInt32 dir  = (xiiInt32)m_Comparer.Less(pNode->m_Key, key);
    const xiiInt32 dir2 = (xiiInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      return pNode;

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::LowerBound(const CompatibleKeyType& key) const
{
  return Iterator(Internal_LowerBound(key));
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::Internal_UpperBound(const CompatibleKeyType& key) const
{
  Node* pNode        = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while (pNode != &m_NilNode)
  {
    const xiiInt32 dir  = (xiiInt32)m_Comparer.Less(pNode->m_Key, key);
    const xiiInt32 dir2 = (xiiInt32)m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
    {
      Iterator it(pNode);
      ++it;
      return it.m_pElement;
    }

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::UpperBound(const CompatibleKeyType& key) const
{
  return Iterator(Internal_UpperBound(key));
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Union(const xiiSetBase<KeyType, Comparer>& operand)
{
  for (const auto& key : operand)
  {
    Insert(key);
  }
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Difference(const xiiSetBase<KeyType, Comparer>& operand)
{
  for (const auto& key : operand)
  {
    Remove(key);
  }
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Intersection(const xiiSetBase<KeyType, Comparer>& operand)
{
  for (auto it = GetIterator(); it.IsValid();)
  {
    if (!operand.Contains(it.Key()))
      it = Remove(it);
    else
      ++it;
  }
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::Insert(CompatibleKeyType&& key)
{
  Node* pInsertedNode = nullptr;

  m_pRoot             = Insert(m_pRoot, std::forward<CompatibleKeyType>(key), pInsertedNode);
  m_pRoot->m_pParent  = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  return Iterator(pInsertedNode);
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
bool xiiSetBase<KeyType, Comparer>::Remove(const CompatibleKeyType& key)
{
  bool bRemoved       = true;
  m_pRoot             = Remove(m_pRoot, key, bRemoved);
  m_pRoot->m_pParent  = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  return bRemoved;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::AcquireNode(CompatibleKeyType&& key, xiiUInt16 uiLevel, Node* pParent)
{
  Node* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode               = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pParent;
  }

  xiiMemoryUtils::Construct<SkipTrivialTypes, Node>(pNode, 1);

  pNode->m_pParent  = pParent;
  pNode->m_Key      = std::forward<CompatibleKeyType>(key);
  pNode->m_uiLevel  = uiLevel;
  pNode->m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  pNode->m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);

  ++m_uiCount;

  return pNode;
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::ReleaseNode(Node* pNode)
{
  XII_ASSERT_DEBUG(pNode != nullptr, "pNode is invalid.");

  xiiMemoryUtils::Destruct<Node>(pNode, 1);

  // try to reduce the element array, if possible
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
    pNode->m_pParent    = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::SkewNode(Node* root)
{
  if ((root->m_pLink[0]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save                  = root->m_pLink[0];
    root->m_pLink[0]            = save->m_pLink[1];
    root->m_pLink[0]->m_pParent = root;
    save->m_pLink[1]            = root;
    save->m_pLink[1]->m_pParent = save;
    root                        = save;
  }

  return root;
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::SplitNode(Node* root)
{
  if ((root->m_pLink[1]->m_pLink[1]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save                  = root->m_pLink[1];
    root->m_pLink[1]            = save->m_pLink[0];
    root->m_pLink[1]->m_pParent = root;
    save->m_pLink[0]            = root;
    save->m_pLink[0]->m_pParent = save;
    root                        = save;
    ++root->m_uiLevel;
  }

  return root;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::Insert(Node* root, CompatibleKeyType&& key, Node*& pInsertedNode)
{
  if (root == &m_NilNode)
  {
    pInsertedNode = AcquireNode(std::forward<CompatibleKeyType>(key), 1, reinterpret_cast<Node*>(&m_NilNode));
    root          = pInsertedNode;
  }
  else
  {
    Node* it = root;
    Node* up[STACK_SIZE];

    xiiInt32 top = 0;
    xiiInt32 dir = 0;

    while (true)
    {
      XII_ASSERT_DEBUG(top < STACK_SIZE, "xiiSetBase's internal stack is not large enough to be able to sort {0} elements.", GetCount());
      up[top++] = it;
      dir       = m_Comparer.Less(it->m_Key, key) ? 1 : 0;

      // element is identical => do not insert
      if ((xiiInt32)m_Comparer.Less(key, it->m_Key) == dir)
      {
        pInsertedNode = it;
        return root;
      }

      if (it->m_pLink[dir] == &m_NilNode)
        break;

      it = it->m_pLink[dir];
    }

    pInsertedNode    = AcquireNode(std::forward<CompatibleKeyType>(key), 1, it);
    it->m_pLink[dir] = pInsertedNode;

    while (--top >= 0)
    {
      if (top != 0)
        dir = up[top - 1]->m_pLink[1] == up[top];

      up[top] = SkewNode(up[top]);
      up[top] = SplitNode(up[top]);

      if (top != 0)
      {
        up[top - 1]->m_pLink[dir]            = up[top];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
        root = up[top];
    }
  }

  return root;
}

template <typename KeyType, typename Comparer>
template <typename CompatibleKeyType>
typename xiiSetBase<KeyType, Comparer>::Node* xiiSetBase<KeyType, Comparer>::Remove(Node* root, const CompatibleKeyType& key, bool& bRemoved)
{
  bRemoved = false;

  Node* ToErase    = reinterpret_cast<Node*>(&m_NilNode);
  Node* ToOverride = reinterpret_cast<Node*>(&m_NilNode);

  if (root != &m_NilNode)
  {
    Node*    it = root;
    Node*    up[STACK_SIZE];
    xiiInt32 top = 0;
    xiiInt32 dir = 0;

    while (true)
    {
      XII_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
      up[top++] = it;

      if (it == &m_NilNode)
        return root;

      xiiInt32 newdir = (xiiInt32)(m_Comparer.Less(it->m_Key, key));

      if (newdir == (xiiInt32)(m_Comparer.Less(key, it->m_Key)))
        break;

      dir = newdir;

      it = it->m_pLink[dir];
    }

    ToOverride = it;

    if ((it->m_pLink[0] == &m_NilNode) || (it->m_pLink[1] == &m_NilNode))
    {
      xiiInt32 dir2 = it->m_pLink[0] == &m_NilNode;

      if (--top != 0)
      {
        XII_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        up[top - 1]->m_pLink[dir]            = it->m_pLink[dir2];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
        root = it->m_pLink[1];
    }
    else
    {
      Node* heir = it->m_pLink[1];
      Node* prev = it;

      while (heir->m_pLink[0] != &m_NilNode)
      {
        XII_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        up[top++] = prev = heir;

        heir = heir->m_pLink[0];
      }

      ToErase    = heir;
      ToOverride = it;

      prev->m_pLink[prev == it]            = heir->m_pLink[1];
      prev->m_pLink[prev == it]->m_pParent = prev;
    }

    while (--top >= 0)
    {
      if (top != 0)
      {
        XII_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        dir = up[top - 1]->m_pLink[1] == up[top];
      }

      XII_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");

      if ((up[top]->m_pLink[0]->m_uiLevel < up[top]->m_uiLevel - 1) || (up[top]->m_pLink[1]->m_uiLevel < up[top]->m_uiLevel - 1))
      {
        if (up[top]->m_pLink[1]->m_uiLevel > --up[top]->m_uiLevel)
          up[top]->m_pLink[1]->m_uiLevel = up[top]->m_uiLevel;

        up[top]                        = SkewNode(up[top]);
        up[top]->m_pLink[1]            = SkewNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];

        up[top]->m_pLink[1]->m_pLink[1] = SkewNode(up[top]->m_pLink[1]->m_pLink[1]);
        up[top]                         = SplitNode(up[top]);
        up[top]->m_pLink[1]             = SplitNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent  = up[top];
      }

      if (top != 0)
      {
        XII_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");

        up[top - 1]->m_pLink[dir]            = up[top];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
      {
        XII_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        root = up[top];
      }
    }
  }

  root->m_pParent = reinterpret_cast<Node*>(&m_NilNode);


  // if necessary, swap nodes
  if (ToErase != &m_NilNode)
  {
    Node* parent = ToOverride->m_pParent;

    if (parent != &m_NilNode)
    {
      if (parent->m_pLink[0] == ToOverride)
      {
        parent->m_pLink[0]            = ToErase;
        parent->m_pLink[0]->m_pParent = parent;
      }
      if (parent->m_pLink[1] == ToOverride)
      {
        parent->m_pLink[1]            = ToErase;
        parent->m_pLink[1]->m_pParent = parent;
      }
    }
    else
      root = ToErase;

    ToErase->m_uiLevel             = ToOverride->m_uiLevel;
    ToErase->m_pLink[0]            = ToOverride->m_pLink[0];
    ToErase->m_pLink[0]->m_pParent = ToErase;
    ToErase->m_pLink[1]            = ToOverride->m_pLink[1];
    ToErase->m_pLink[1]->m_pParent = ToErase;
  }

  // remove the erased node
  if (ToOverride != &m_NilNode)
  {
    bRemoved = true;
    ReleaseNode(ToOverride);
  }

  return root;
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator xiiSetBase<KeyType, Comparer>::Remove(const Iterator& pos)
{
  XII_ASSERT_DEBUG(pos.m_pElement != nullptr, "The Iterator(pos) is invalid.");

  Iterator temp(pos);
  ++temp;
  Remove(pos.Key());
  return temp;
}

template <typename KeyType, typename Comparer>
bool xiiSetBase<KeyType, Comparer>::operator==(const xiiSetBase<KeyType, Comparer>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (!m_Comparer.Equal(itLhs.Key(), itRhs.Key()))
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

#undef STACK_SIZE

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
xiiSet<KeyType, Comparer, AllocatorWrapper>::xiiSet() :
  xiiSetBase<KeyType, Comparer>(Comparer(), AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
xiiSet<KeyType, Comparer, AllocatorWrapper>::xiiSet(xiiAllocatorBase* pAllocator) :
  xiiSetBase<KeyType, Comparer>(Comparer(), pAllocator)
{
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
xiiSet<KeyType, Comparer, AllocatorWrapper>::xiiSet(const Comparer& comparer, xiiAllocatorBase* pAllocator) :
  xiiSetBase<KeyType, Comparer>(comparer, pAllocator)
{
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
xiiSet<KeyType, Comparer, AllocatorWrapper>::xiiSet(const xiiSet<KeyType, Comparer, AllocatorWrapper>& other) :
  xiiSetBase<KeyType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
xiiSet<KeyType, Comparer, AllocatorWrapper>::xiiSet(const xiiSetBase<KeyType, Comparer>& other) :
  xiiSetBase<KeyType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
void xiiSet<KeyType, Comparer, AllocatorWrapper>::operator=(const xiiSet<KeyType, Comparer, AllocatorWrapper>& rhs)
{
  xiiSetBase<KeyType, Comparer>::operator=(rhs);
}

template <typename KeyType, typename Comparer, typename AllocatorWrapper>
void xiiSet<KeyType, Comparer, AllocatorWrapper>::operator=(const xiiSetBase<KeyType, Comparer>& rhs)
{
  xiiSetBase<KeyType, Comparer>::operator=(rhs);
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::Swap(xiiSetBase<KeyType, Comparer>& other)
{
  SwapNilNode(this->m_pRoot, &this->m_NilNode, &other.m_NilNode);
  SwapNilNode(other.m_pRoot, &other.m_NilNode, &this->m_NilNode);

  xiiMath::Swap(this->m_pRoot, other.m_pRoot);
  xiiMath::Swap(this->m_uiCount, other.m_uiCount);
  xiiMath::Swap(this->m_pFreeElementStack, other.m_pFreeElementStack);
  xiiMath::Swap(this->m_Comparer, other.m_Comparer);

  // after we swapped the root nodes, fix up their parent nodes
  this->m_pRoot->m_pParent = reinterpret_cast<Node*>(&this->m_NilNode);
  other.m_pRoot->m_pParent = reinterpret_cast<Node*>(&other.m_NilNode);

  // the set allocator is stored in this array
  m_Elements.Swap(other.m_Elements);
}

template <typename KeyType, typename Comparer>
void xiiSetBase<KeyType, Comparer>::SwapNilNode(Node*& pCurNode, NilNode* pOld, NilNode* pNew)
{
  if (pCurNode == pOld)
  {
    pCurNode = reinterpret_cast<Node*>(pNew);
    return;
  }

  SwapNilNode(pCurNode->m_pLink[0], pOld, pNew);
  SwapNilNode(pCurNode->m_pLink[1], pOld, pNew);
}
