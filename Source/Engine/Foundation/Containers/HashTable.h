/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

template <typename KeyType, typename ValueType, typename Hasher>
class xiiHashTableBase;

/// Const iterator.
template <typename KeyType, typename ValueType, typename Hasher>
struct xiiHashTableBaseConstIterator
{
  using iterator_category = std::forward_iterator_tag;
  using value_type        = xiiHashTableBaseConstIterator;
  using difference_type   = std::ptrdiff_t;
  using pointer           = xiiHashTableBaseConstIterator*;
  using reference         = xiiHashTableBaseConstIterator&;

  XII_DECLARE_POD_TYPE();

  xiiHashTableBaseConstIterator() = default;

  /// Checks whether this iterator points to a valid element.
  bool IsValid() const; // [tested]

  /// Checks whether the two iterators point to the same element.
  bool operator==(const xiiHashTableBaseConstIterator& rhs) const;

  /// Returns the 'key' of the element that this iterator points to.
  const KeyType& Key() const; // [tested]

  /// Returns the 'value' of the element that this iterator points to.
  const ValueType& Value() const; // [tested]

  /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
  void Next(); // [tested]

  /// Shorthand for 'Next'
  void operator++(); // [tested]

  /// Returns '*this' to enable foreach
  XII_ALWAYS_INLINE xiiHashTableBaseConstIterator& operator*() { return *this; } // [tested]

protected:
  friend class xiiHashTableBase<KeyType, ValueType, Hasher>;

  explicit xiiHashTableBaseConstIterator(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable);
  void SetToBegin();
  void SetToEnd();

  const xiiHashTableBase<KeyType, ValueType, Hasher>* m_pHashTable     = nullptr;
  xiiUInt32                                           m_uiCurrentIndex = 0; // current element index that this iterator points to.
  xiiUInt32                                           m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.

public:
  struct Pointer
  {
    std::pair<const KeyType&, const ValueType&>        value;
    const std::pair<const KeyType&, const ValueType&>* operator->() const { return &value; }
  };

  XII_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {Key(), Value()}};
  }

  // These function is used to return the values for structured bindings.
  // The number and type of type of each slot are defined in the inl file.
  template <std::size_t Index>
  std::tuple_element_t<Index, xiiHashTableBaseConstIterator>& get() const
  {
    if constexpr (Index == 0)
      return Key();
    if constexpr (Index == 1)
      return Value();
  }
};

/// Iterator with write access.
template <typename KeyType, typename ValueType, typename Hasher>
struct xiiHashTableBaseIterator : public xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>
{
  XII_DECLARE_POD_TYPE();

  /// Creates a new iterator from another.
  XII_ALWAYS_INLINE xiiHashTableBaseIterator(const xiiHashTableBaseIterator& rhs); // [tested]

  /// Assigns one iterator no another.
  XII_ALWAYS_INLINE void operator=(const xiiHashTableBaseIterator& rhs); // [tested]

  // this is required to pull in the const version of this function
  using xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Value;

  /// Returns the 'value' of the element that this iterator points to.
  XII_FORCE_INLINE ValueType& Value(); // [tested]

  /// Returns the 'value' of the element that this iterator points to.
  XII_FORCE_INLINE ValueType& Value() const;

  /// Returns '*this' to enable foreach
  XII_ALWAYS_INLINE xiiHashTableBaseIterator& operator*() { return *this; } // [tested]

private:
  friend class xiiHashTableBase<KeyType, ValueType, Hasher>;

  explicit xiiHashTableBaseIterator(const xiiHashTableBase<KeyType, ValueType, Hasher>& hashTable);

public:
  struct Pointer
  {
    std::pair<const KeyType&, ValueType&>        value;
    const std::pair<const KeyType&, ValueType&>* operator->() const { return &value; }
  };

  XII_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key(), Value()}};
  }

  // These functions are used to return the values for structured bindings.
  // The number and type of type of each slot are defined in the inl file.
  template <std::size_t Index>
  std::tuple_element_t<Index, xiiHashTableBaseIterator>& get()
  {
    if constexpr (Index == 0)
      return xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key();
    if constexpr (Index == 1)
      return Value();
  }

  template <std::size_t Index>
  std::tuple_element_t<Index, xiiHashTableBaseIterator>& get() const
  {
    if constexpr (Index == 0)
      return xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key();
    if constexpr (Index == 1)
      return Value();
  }
};

/// Implementation of a hashtable which stores key/value pairs.
///
/// The hashtable maps keys to values by using the hash of the key as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all key/value pairs are stored
/// in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded,
/// which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like xiiHashHelper.
///
/// \see xiiHashHelper
template <typename KeyType, typename ValueType, typename Hasher>
class xiiHashTableBase
{
public:
  using Iterator      = xiiHashTableBaseIterator<KeyType, ValueType, Hasher>;
  using ConstIterator = xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>;

protected:
  /// Creates an empty hashtable. Does not allocate any data yet.
  explicit xiiHashTableBase(xiiAllocator* pAllocator); // [tested]

  /// Creates a copy of the given hashtable.
  xiiHashTableBase(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs, xiiAllocator* pAllocator); // [tested]

  /// Moves data from an existing hashtable into this one.
  xiiHashTableBase(xiiHashTableBase<KeyType, ValueType, Hasher>&& rhs, xiiAllocator* pAllocator); // [tested]

  /// Destructor.
  ~xiiHashTableBase(); // [tested]

  /// Copies the data from another hashtable into this one.
  void operator=(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs); // [tested]

  /// Moves data from an existing hashtable into this one.
  void operator=(xiiHashTableBase<KeyType, ValueType, Hasher>&& rhs); // [tested]

public:
  /// Compares this table to another table.
  bool operator==(const xiiHashTableBase<KeyType, ValueType, Hasher>& rhs) const; // [tested]

  /// Expands the hashtable by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the given
  /// number of entries.
  void Reserve(xiiUInt32 uiCapacity); // [tested]

  /// Tries to compact the hashtable to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashtable is empty.
  void Compact(); // [tested]

  /// Returns the number of active entries in the table.
  xiiUInt32 GetCount() const; // [tested]

  /// Returns true, if the hashtable does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the table.
  void Clear(); // [tested]

  /// Inserts the key value pair or replaces value if an entry with the given key already exists.
  ///
  /// Returns true if an existing value was replaced and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  bool Insert(CompatibleKeyType&& key, CompatibleValueType&& value, ValueType* out_pOldValue = nullptr); // [tested]

  /// Removes the entry with the given key. Returns whether an entry was removed and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key, ValueType* out_pOldValue = nullptr); // [tested]

  /// Erases the key/value pair at the given Iterator. Returns an iterator to the element after the given iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// Cannot remove an element with just a xiiHashTableBaseConstIterator
  void Remove(const ConstIterator& pos) = delete;

  /// Returns whether an entry with the given key was found and if found writes out the corresponding value to out_value.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const; // [tested]

  /// Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const; // [tested]

  /// Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue) const; // [tested]

  /// Searches for key, returns a xiiHashTableBaseConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

  /// Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key);

  /// Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  const ValueType* GetValue(const CompatibleKeyType& key) const; // [tested]

  /// Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  ValueType* GetValue(const CompatibleKeyType& key); // [tested]

  /// Returns the value to the given key if found or creates a new entry with the given key and a default constructed value.
  ValueType& operator[](const KeyType& key); // [tested]

  /// Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  ValueType& FindOrAdd(const KeyType& key, bool* out_pExisted = nullptr); // [tested]

  /// Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// Returns an Iterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  Iterator GetEndIterator(); // [tested]

  /// Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns a xiiHashTableBaseConstIterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  ConstIterator GetEndIterator() const; // [tested]

  /// Returns the allocator that is used by this instance.
  xiiAllocator* GetAllocator() const;

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const; // [tested]

  /// Swaps this map with the other one.
  void Swap(xiiHashTableBase<KeyType, ValueType, Hasher>& other); // [tested]

private:
  friend struct xiiHashTableBaseConstIterator<KeyType, ValueType, Hasher>;
  friend struct xiiHashTableBaseIterator<KeyType, ValueType, Hasher>;

  struct Entry
  {
    KeyType   key;
    ValueType value;
  };

  Entry*     m_pEntries    = nullptr;
  xiiUInt32* m_pEntryFlags = nullptr;

  xiiUInt32 m_uiCount    = 0;
  xiiUInt32 m_uiCapacity = 0;

  xiiAllocator* m_pAllocator = nullptr;

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

/// \see xiiHashTableBase
template <typename KeyType, typename ValueType, typename Hasher = xiiHashHelper<KeyType>, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiHashTable : public xiiHashTableBase<KeyType, ValueType, Hasher>
{
public:
  xiiHashTable();
  explicit xiiHashTable(xiiAllocator* pAllocator);

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
