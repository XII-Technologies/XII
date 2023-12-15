#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of a hashtable which stores key/value pairs.
///
/// The hashtable maps keys to values by using the hash of the key as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all key/value pairs are stored in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded, which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like xiiHashHelper.

/// \see xiiHashHelper
template <typename KeyType, typename ValueType, typename Hasher>
class xiiHashTableBase
{
public:
  /// \brief Const iterator.
  struct ConstIterator
  {
    using iterator_category = std::forward_iterator_tag;
    using value_type        = ConstIterator;
    using difference_type   = std::ptrdiff_t;
    using pointer           = ConstIterator*;
    using reference         = ConstIterator&;

    XII_DECLARE_POD_TYPE();

    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    bool operator==(const typename xiiHashTableBase<KeyType, ValueType, Hasher>::ConstIterator& rhs) const;

    /// \brief Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    const ValueType& Value() const; // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

    /// \brief Returns '*this' to enable foreach
    XII_ALWAYS_INLINE ConstIterator& operator*() { return *this; } // [tested]

  protected:
    friend class xiiHashTableBase<KeyType, ValueType, Hasher>;

    explicit ConstIterator(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable);
    void SetToBegin();
    void SetToEnd();

    const xiiHashTableBase<KeyType, ValueType, Hasher>* m_pHashTable     = nullptr;
    xiiUInt32                                           m_uiCurrentIndex = 0; // current element index that this iterator points to.
    xiiUInt32                                           m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.
  };

  /// \brief Iterator with write access.
  struct Iterator : public ConstIterator
  {
    XII_DECLARE_POD_TYPE();

    /// \brief Creates a new iterator from another.
    XII_ALWAYS_INLINE Iterator(const Iterator& rhs); // [tested]

    /// \brief Assigns one iterator no another.
    XII_ALWAYS_INLINE void operator=(const Iterator& rhs); // [tested]

    // this is required to pull in the const version of this function
    using ConstIterator::Value;

    /// \brief Returns the 'value' of the element that this iterator points to.
    XII_FORCE_INLINE ValueType& Value(); // [tested]

    /// \brief Returns '*this' to enable foreach
    XII_ALWAYS_INLINE Iterator& operator*() { return *this; } // [tested]

  private:
    friend class xiiHashTableBase<KeyType, ValueType, Hasher>;

    explicit Iterator(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable);
  };

protected:
  /// \brief Creates an empty hashtable. Does not allocate any data yet.
  explicit xiiHashTableBase(xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashtable.
  xiiHashTableBase(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  xiiHashTableBase(xiiHashTableBase<KeyType, ValueType, Hasher>&& rhs, xiiAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~xiiHashTableBase(); // [tested]

  /// \brief Copies the data from another hashtable into this one.
  void operator=(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  void operator=(xiiHashTableBase<KeyType, ValueType, Hasher>&& rhs); // [tested]

public:
  /// \brief Compares this table to another table.
  bool operator==(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs) const; // [tested]

  /// \brief Expands the hashtable by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the given number of entries.
  void Reserve(xiiUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the hashtable to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashtable is empty.
  void Compact(); // [tested]

  /// \brief Returns the number of active entries in the table.
  xiiUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the hashtable does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the table.
  void Clear(); // [tested]

  /// \brief Inserts the key value pair or replaces value if an entry with the given key already exists.
  ///
  /// Returns true if an existing value was replaced and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  bool Insert(CompatibleKeyType&& key, CompatibleValueType&& value, ValueType* out_pOldValue = nullptr); // [tested]

  /// \brief Removes the entry with the given key. Returns whether an entry was removed and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key, ValueType* out_pOldValue = nullptr); // [tested]

  /// \brief Erases the key/value pair at the given Iterator. Returns an iterator to the element after the given iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Cannot remove an element with just a ConstIterator
  void Remove(const ConstIterator& pos) = delete;

  /// \brief Returns whether an entry with the given key was found and if found writes out the corresponding value to out_value.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue) const; // [tested]

  /// \brief Searches for key, returns a ConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key);

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  const ValueType* GetValue(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  ValueType* GetValue(const CompatibleKeyType& key); // [tested]

  /// \brief Returns the value to the given key if found or creates a new entry with the given key and a default constructed value.
  ValueType& operator[](const KeyType& key); // [tested]

  /// \brief Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  ValueType& FindOrAdd(const KeyType& key, bool* pExisted); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns an Iterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  Iterator GetEndIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a ConstIterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  ConstIterator GetEndIterator() const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  xiiAllocatorBase* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(xiiHashTableBase<KeyType, ValueType, Hasher>& other); // [tested]


private:
  struct Entry
  {
    KeyType   key;
    ValueType value;
  };

  Entry*     m_pEntries;
  xiiUInt32* m_pEntryFlags;

  xiiUInt32 m_uiCount;
  xiiUInt32 m_uiCapacity;

  xiiAllocatorBase* m_pAllocator;

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

/// \brief \see xiiHashTableBase
template <typename KeyType, typename ValueType, typename Hasher = xiiHashHelper<KeyType>, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiHashTable : public xiiHashTableBase<KeyType, ValueType, Hasher>
{
public:
  xiiHashTable();
  explicit xiiHashTable(xiiAllocatorBase* pAllocator);

  xiiHashTable(const xiiHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& other);
  xiiHashTable(const xiiHashTableBase<KeyType, ValueType, Hasher>& other);

  xiiHashTable(xiiHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& other);
  xiiHashTable(xiiHashTableBase<KeyType, ValueType, Hasher>&& other);


  void operator=(const xiiHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs);

  void operator=(xiiHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(xiiHashTableBase<KeyType, ValueType, Hasher>&& rhs);
};

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::Iterator begin(xiiHashTableBase<KeyType, ValueType, Hasher>& ref_container)
{
  return ref_container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::ConstIterator begin(const xiiHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::ConstIterator cbegin(const xiiHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::Iterator end(xiiHashTableBase<KeyType, ValueType, Hasher>& ref_container)
{
  return ref_container.GetEndIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::ConstIterator end(const xiiHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetEndIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename xiiHashTableBase<KeyType, ValueType, Hasher>::ConstIterator cend(const xiiHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashTable_inl.h>
