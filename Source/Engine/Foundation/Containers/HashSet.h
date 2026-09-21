/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// Implementation of a hashset.
///
/// The hashset stores values by using the hash as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all values are stored
/// in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded,
/// which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like xiiHashHelper.

/// \see xiiHashHelper
template <typename KeyType, typename Hasher>
class xiiHashSetBase
{
public:
  /// Const iterator.
  class ConstIterator
  {
  public:
    /// Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// Checks whether the two iterators point to the same element.
    bool operator==(const typename xiiHashSetBase<KeyType, Hasher>::ConstIterator& rhs) const;

    /// Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// Returns the 'key' of the element that this iterator points to.
    XII_ALWAYS_INLINE const KeyType& operator*() const { return Key(); } // [tested]

    /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class xiiHashSetBase<KeyType, Hasher>;

    explicit ConstIterator(const xiiHashSetBase<KeyType, Hasher>& hashSet);
    void SetToBegin();
    void SetToEnd();

    const xiiHashSetBase<KeyType, Hasher>* m_pHashSet       = nullptr;
    xiiUInt32                              m_uiCurrentIndex = 0; // current element index that this iterator points to.
    xiiUInt32                              m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.
  };

protected:
  /// Creates an empty hashset. Does not allocate any data yet.
  explicit xiiHashSetBase(xiiAllocator* pAllocator); // [tested]

  /// Creates a copy of the given hashset.
  xiiHashSetBase(const xiiHashSetBase<KeyType, Hasher>& rhs, xiiAllocator* pAllocator); // [tested]

  /// Moves data from an existing hashtable into this one.
  xiiHashSetBase(xiiHashSetBase<KeyType, Hasher>&& rhs, xiiAllocator* pAllocator); // [tested]

  /// Destructor.
  ~xiiHashSetBase(); // [tested]

  /// Copies the data from another hashset into this one.
  void operator=(const xiiHashSetBase<KeyType, Hasher>& rhs); // [tested]

  /// Moves data from an existing hashset into this one.
  void operator=(xiiHashSetBase<KeyType, Hasher>&& rhs); // [tested]

public:
  /// Compares this table to another table.
  bool operator==(const xiiHashSetBase<KeyType, Hasher>& rhs) const; // [tested]

  /// Expands the hashset by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the
  /// given number of entries.
  void Reserve(xiiUInt32 uiCapacity); // [tested]

  /// Tries to compact the hashset to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashset is empty.
  void Compact(); // [tested]

  /// Returns the number of active entries in the table.
  xiiUInt32 GetCount() const; // [tested]

  /// Returns true, if the hashset does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the table.
  void Clear(); // [tested]

  /// Inserts the key. Returns whether the key was already existing.
  template <typename CompatibleKeyType>
  bool Insert(CompatibleKeyType&& key); // [tested]

  /// Removes the entry with the given key. Returns if an entry was removed.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// Erases the key at the given Iterator. Returns an iterator to the element after the given iterator.
  ConstIterator Remove(const ConstIterator& pos); // [tested]

  /// Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Checks whether all keys of the given set are in the container.
  bool ContainsSet(const xiiHashSetBase<KeyType, Hasher>& operand) const; // [tested]

  /// Makes this set the union of itself and the operand.
  void Union(const xiiHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const xiiHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Makes this set the intersection of itself and the operand.
  void Intersection(const xiiHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns a constant Iterator to the first element that is not part of the hashset. Needed to implement range based for loop
  /// support.
  ConstIterator GetEndIterator() const;

  /// Returns the allocator that is used by this instance.
  xiiAllocator* GetAllocator() const;

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const; // [tested]

  /// Swaps this map with the other one.
  void Swap(xiiHashSetBase<KeyType, Hasher>& other); // [tested]

  /// Searches for key, returns a ConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

private:
  KeyType*   m_pEntries;
  xiiUInt32* m_pEntryFlags;

  xiiUInt32 m_uiCount;
  xiiUInt32 m_uiCapacity;

  xiiAllocator* m_pAllocator;

  enum
  {
    FREE_ENTRY         = 0,
    VALID_ENTRY        = 1,
    DELETED_ENTRY      = 2,
    FLAGS_MASK         = 3,
    CAPACITY_ALIGNMENT = 32
  };

  void SetCapacity(xiiUInt32 uiCapacity);

  void RemoveInternal(xiiUInt32 uiIndex);

  template <typename CompatibleKeyType>
  xiiUInt32 FindEntry(const CompatibleKeyType& key) const;

  template <typename CompatibleKeyType>
  xiiUInt32 FindEntry(xiiUInt32 uiHash, const CompatibleKeyType& key) const;

  xiiUInt32 GetFlagsCapacity() const;
  xiiUInt32 GetFlags(xiiUInt32* pFlags, xiiUInt32 uiEntryIndex) const;
  void      SetFlags(xiiUInt32 uiEntryIndex, xiiUInt32 uiFlags);

  bool IsFreeEntry(xiiUInt32 uiEntryIndex) const;
  bool IsValidEntry(xiiUInt32 uiEntryIndex) const;
  bool IsDeletedEntry(xiiUInt32 uiEntryIndex) const;

  void MarkEntryAsFree(xiiUInt32 uiEntryIndex);
  void MarkEntryAsValid(xiiUInt32 uiEntryIndex);
  void MarkEntryAsDeleted(xiiUInt32 uiEntryIndex);
};

/// \see xiiHashSetBase
template <typename KeyType, typename Hasher = xiiHashHelper<KeyType>, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiHashSet : public xiiHashSetBase<KeyType, Hasher>
{
public:
  xiiHashSet();
  explicit xiiHashSet(xiiAllocator* pAllocator);

  xiiHashSet(const xiiHashSet<KeyType, Hasher, AllocatorWrapper>& other);
  xiiHashSet(const xiiHashSetBase<KeyType, Hasher>& other);

  xiiHashSet(xiiHashSet<KeyType, Hasher, AllocatorWrapper>&& other);
  xiiHashSet(xiiHashSetBase<KeyType, Hasher>&& other);

  void operator=(const xiiHashSet<KeyType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const xiiHashSetBase<KeyType, Hasher>& rhs);

  void operator=(xiiHashSet<KeyType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(xiiHashSetBase<KeyType, Hasher>&& rhs);
};

template <typename KeyType, typename Hasher>
typename xiiHashSetBase<KeyType, Hasher>::ConstIterator begin(const xiiHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename xiiHashSetBase<KeyType, Hasher>::ConstIterator cbegin(const xiiHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename xiiHashSetBase<KeyType, Hasher>::ConstIterator end(const xiiHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

template <typename KeyType, typename Hasher>
typename xiiHashSetBase<KeyType, Hasher>::ConstIterator cend(const xiiHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashSet_inl.h>
