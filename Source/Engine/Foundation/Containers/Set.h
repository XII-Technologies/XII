/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Deque.h>

/// A set container that only stores whether an element resides in it or not. Similar to STL::set
///
/// Sets are similar to maps that do not store a value (or only a bool that is always true).
/// Sets can be used to reduce an unordered number of elements to only those that are unique.
/// Insertion/erasure/lookup in sets is quite fast (O (log n)).
/// This container is implemented with a red-black tree, so it will always be a balanced tree.
template <typename KeyType, typename Comparer>
class xiiSetBase
{
private:
  struct Node;

  /// Only used by the sentinel node.
  struct NilNode
  {
    xiiUInt16 m_uiLevel  = 0;
    Node*     m_pParent  = nullptr;
    Node*     m_pLink[2] = {nullptr, nullptr};
  };

  /// A node storing the key
  struct Node : public NilNode
  {
    KeyType m_Key;
  };

public:
  /// Base class for all iterators.
  template <bool REVERSE>
  struct IteratorBase
  {
    using iterator_category = std::forward_iterator_tag;
    using value_type        = IteratorBase<REVERSE>;
    using difference_type   = std::ptrdiff_t;
    using pointer           = IteratorBase<REVERSE>*;
    using reference         = IteratorBase<REVERSE>&;

    XII_DECLARE_POD_TYPE();

    /// Constructs an invalid iterator.
    XII_ALWAYS_INLINE IteratorBase() :
      m_pElement(nullptr)
    {
    } // [tested]

    /// Checks whether this iterator points to a valid element.
    XII_ALWAYS_INLINE bool IsValid() const { return (m_pElement != nullptr); } // [tested]

    /// Checks whether the two iterators point to the same element.
    XII_ALWAYS_INLINE bool operator==(const typename xiiSetBase<KeyType, Comparer>::IteratorBase<REVERSE>& it2) const { return (m_pElement == it2.m_pElement); }

    /// Returns the 'key' of the element that this iterator points to.
    XII_FORCE_INLINE const KeyType& Key() const
    {
      XII_ASSERT_DEBUG(IsValid(), "Cannot access the 'key' of an invalid iterator.");
      return m_pElement->m_Key;
    } // [tested]

    /// Returns the 'key' of the element that this iterator points to.
    XII_ALWAYS_INLINE const KeyType& operator*() const { return Key(); }

    /// Advances the iterator to the next element in the set. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// Advances the iterator to the previous element in the set. The iterator will not be valid anymore, if the end is reached.
    void Prev(); // [tested]

    /// Shorthand for 'Next'
    XII_ALWAYS_INLINE void operator++() { Next(); } // [tested]

    /// Shorthand for 'Prev'
    XII_ALWAYS_INLINE void operator--() { Prev(); } // [tested]

  protected:
    void Advance(xiiInt32 dir0, xiiInt32 dir1);

    friend class xiiSetBase<KeyType, Comparer>;

    XII_ALWAYS_INLINE explicit IteratorBase(Node* pInit) :
      m_pElement(pInit)
    {
    }

    Node* m_pElement;
  };

  using Iterator        = IteratorBase<false>;
  using ReverseIterator = IteratorBase<true>;

protected:
  /// Initializes the set to be empty.
  xiiSetBase(const Comparer& comparer, xiiAllocator* pAllocator); // [tested]

  /// Copies all keys from the given set into this one.
  xiiSetBase(const xiiSetBase<KeyType, Comparer>& cc, xiiAllocator* pAllocator); // [tested]

  /// Destroys all elements in the set.
  ~xiiSetBase(); // [tested]

  /// Copies all keys from the given set into this one.
  void operator=(const xiiSetBase<KeyType, Comparer>& rhs); // [tested]

public:
  /// Returns whether there are no elements in the set. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// Returns the number of elements currently stored in the set. O(1) operation.
  xiiUInt32 GetCount() const; // [tested]

  /// Destroys all elements in the set and resets its size to zero.
  void Clear(); // [tested]

  /// Returns a constant Iterator to the very first element.
  Iterator GetIterator() const; // [tested]

  /// Returns a constant ReverseIterator to the very last element.
  ReverseIterator GetReverseIterator() const; // [tested]

  /// Inserts the key into the tree and returns an Iterator to it. O(log n) operation.
  template <typename CompatibleKeyType>
  Iterator Insert(CompatibleKeyType&& key); // [tested]

  /// Erases the element with the given key, if it exists. O(log n) operation.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// Erases the element at the given Iterator. O(log n) operation.
  Iterator Remove(const Iterator& pos); // [tested]

  /// Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key) const; // [tested]

  /// Checks whether the given key is in the container.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Checks whether all keys of the given set are in the container.
  bool ContainsSet(const xiiSetBase<KeyType, Comparer>& operand) const; // [tested]

  /// Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no such
  /// element.
  template <typename CompatibleKeyType>
  Iterator LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no such
  /// element.
  template <typename CompatibleKeyType>
  Iterator UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// Makes this set the union of itself and the operand.
  void Union(const xiiSetBase<KeyType, Comparer>& operand); // [tested]

  /// Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const xiiSetBase<KeyType, Comparer>& operand); // [tested]

  /// Makes this set the intersection of itself and the operand.
  void Intersection(const xiiSetBase<KeyType, Comparer>& operand); // [tested]

  /// Returns the allocator that is used by this instance.
  xiiAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// Comparison operator
  bool operator==(const xiiSetBase<KeyType, Comparer>& rhs) const; // [tested]

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

  /// Swaps this map with the other one.
  void Swap(xiiSetBase<KeyType, Comparer>& other); // [tested]

private:
  template <typename CompatibleKeyType>
  Node* Internal_Find(const CompatibleKeyType& key) const;
  template <typename CompatibleKeyType>
  Node* Internal_LowerBound(const CompatibleKeyType& key) const;
  template <typename CompatibleKeyType>
  Node* Internal_UpperBound(const CompatibleKeyType& key) const;

private:
  void Constructor();

  /// Creates one new node and initializes it.
  template <typename CompatibleKeyType>
  Node* AcquireNode(CompatibleKeyType&& key, xiiUInt16 uiLevel, Node* pParent);

  /// Destroys the given node.
  void ReleaseNode(Node* pNode);

  /// Red-Black Tree stuff(Anderson Tree to be exact).
  ///
  /// Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
  Node* SkewNode(Node* root);
  Node* SplitNode(Node* root);

  template <typename CompatibleKeyType>
  Node* Insert(Node* root, CompatibleKeyType&& key, Node*& pInsertedNode);
  template <typename CompatibleKeyType>
  Node* Remove(Node* root, const CompatibleKeyType& key, bool& bRemoved);

  /// Returns the left-most node of the tree(smallest key).
  Node* GetLeftMost() const;

  /// Returns the right-most node of the tree(largest key).
  Node* GetRightMost() const;

  /// Needed during Swap() to fix up the NilNode pointers from one container to the other
  void SwapNilNode(Node*& pCurNode, NilNode* pOld, NilNode* pNew);

  /// Root node of the tree.
  Node* m_pRoot;

  /// Sentinel node.
  NilNode m_NilNode;

  /// Number of active nodes in the tree.
  xiiUInt32 m_uiCount;

  /// Data store. Keeps all the nodes.
  xiiDeque<Node, xiiNullAllocatorWrapper, false> m_Elements;

  /// Stack of recently discarded nodes to quickly acquire new nodes.
  Node* m_pFreeElementStack;

  /// Comparer object
  Comparer m_Comparer;
};

/// \see xiiSetBase
template <typename KeyType, typename Comparer = xiiCompareHelper<KeyType>, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiSet : public xiiSetBase<KeyType, Comparer>
{
public:
  xiiSet();
  explicit xiiSet(xiiAllocator* pAllocator);
  xiiSet(const Comparer& comparer, xiiAllocator* pAllocator);

  xiiSet(const xiiSet<KeyType, Comparer, AllocatorWrapper>& other);
  xiiSet(const xiiSetBase<KeyType, Comparer>& other);

  void operator=(const xiiSet<KeyType, Comparer, AllocatorWrapper>& rhs);
  void operator=(const xiiSetBase<KeyType, Comparer>& rhs);
};


template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator begin(xiiSetBase<KeyType, Comparer>& ref_container)
{
  return ref_container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator begin(const xiiSetBase<KeyType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator cbegin(const xiiSetBase<KeyType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator end(xiiSetBase<KeyType, Comparer>& ref_container)
{
  XII_IGNORE_UNUSED(ref_container);

  return typename xiiSetBase<KeyType, Comparer>::Iterator();
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator end(const xiiSetBase<KeyType, Comparer>& container)
{
  XII_IGNORE_UNUSED(container);

  return typename xiiSetBase<KeyType, Comparer>::Iterator();
}

template <typename KeyType, typename Comparer>
typename xiiSetBase<KeyType, Comparer>::Iterator cend(const xiiSetBase<KeyType, Comparer>& container)
{
  XII_IGNORE_UNUSED(container);

  return typename xiiSetBase<KeyType, Comparer>::Iterator();
}

#include <Foundation/Containers/Implementation/Set_inl.h>
