/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// An associative container, similar to xiiMap, but all data is stored in a sorted contiguous array, which makes frequent lookups more
/// efficient.
///
/// Prefer this container over xiiMap when you modify the container less often than you look things up (which is in most cases), and when
/// you do not need to store iterators to elements and require them to stay valid when the container is modified.
///
/// xiiArrayMapBase also allows to store multiple values under the same key (like a multi-map).
template <typename KEY, typename VALUE>
class xiiArrayMapBase
{
  /// \todo Custom comparer

public:
  struct Pair
  {
    KEY   key;
    VALUE value;

    XII_DETECT_TYPE_CLASS(KEY, VALUE);

    XII_ALWAYS_INLINE bool operator<(const Pair& rhs) const { return key < rhs.key; }

    XII_ALWAYS_INLINE bool operator==(const Pair& rhs) const { return key == rhs.key; }
  };

  /// Constructor.
  explicit xiiArrayMapBase(xiiAllocator* pAllocator); // [tested]

  /// Copy-Constructor.
  xiiArrayMapBase(const xiiArrayMapBase& rhs, xiiAllocator* pAllocator); // [tested]

  /// Copy assignment operator.
  void operator=(const xiiArrayMapBase& rhs); // [tested]

  /// Returns the number of elements stored in the map.
  xiiUInt32 GetCount() const; // [tested]

  /// True if the map contains no elements.
  bool IsEmpty() const; // [tested]

  /// Purges all elements from the map.
  void Clear(); // [tested]

  /// Always inserts a new value under the given key. Duplicates are allowed.
  /// Returns the index of the newly added element.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  xiiUInt32 Insert(CompatibleKeyType&& key, CompatibleValueType&& value); // [tested]

  /// Ensures the internal data structure is sorted. This is done automatically every time a lookup needs to be made.
  void Sort() const; // [tested]

  /// Returns an index to one element with the given key. If the key is inserted multiple times, there is no guarantee which one is returned.
  /// Returns xiiInvalidIndex when no such element exists.
  template <typename CompatibleKeyType>
  xiiUInt32 Find(const CompatibleKeyType& key) const; // [tested]

  /// Returns the index to the first element with a key equal or larger than the given key.
  /// Returns xiiInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  xiiUInt32 LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// Returns the index to the first element with a key that is LARGER than the given key.
  /// Returns xiiInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  xiiUInt32 UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// Returns the key that is stored at the given index.
  const KEY& GetKey(xiiUInt32 uiIndex) const; // [tested]

  /// Returns the value that is stored at the given index.
  const VALUE& GetValue(xiiUInt32 uiIndex) const; // [tested]

  /// Returns the value that is stored at the given index.
  VALUE& GetValue(xiiUInt32 uiIndex); // [tested]

  /// Returns a reference to the map data array.
  xiiDynamicArray<Pair>& GetData();

  /// Returns a constant reference to the map data array.
  const xiiDynamicArray<Pair>& GetData() const;

  /// Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  template <typename CompatibleKeyType>
  VALUE& FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted = nullptr); // [tested]

  /// Same as FindOrAdd.
  template <typename CompatibleKeyType>
  VALUE& operator[](const CompatibleKeyType& key); // [tested]

  /// Returns the key/value pair at the given index.
  const Pair& GetPair(xiiUInt32 uiIndex) const; // [tested]

  /// Removes the element at the given index.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  void RemoveAtAndCopy(xiiUInt32 uiIndex, bool bKeepSorted = false);

  /// Removes one element with the given key. Returns true, if one was found and removed. If the same key exists multiple times, you need to
  /// call this function multiple times to remove them all.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  template <typename CompatibleKeyType>
  bool RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted = false); // [tested]

  /// Returns whether an element with the given key exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Returns whether an element with the given key and value already exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key, const VALUE& value) const; // [tested]

  /// Reserves enough memory to store \a size elements.
  void Reserve(xiiUInt32 uiSize); // [tested]

  /// Compacts the internal memory to not waste any space.
  void Compact(); // [tested]

  /// Compares the two containers for equality.
  bool operator==(const xiiArrayMapBase<KEY, VALUE>& rhs) const; // [tested]

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); } // [tested]

  using const_iterator         = typename xiiDynamicArray<Pair>::const_iterator;
  using const_reverse_iterator = typename xiiDynamicArray<Pair>::const_reverse_iterator;
  using iterator               = typename xiiDynamicArray<Pair>::iterator;
  using reverse_iterator       = typename xiiDynamicArray<Pair>::reverse_iterator;

private:
  mutable bool                  m_bSorted;
  mutable xiiDynamicArray<Pair> m_Data;
};

/// See xiiArrayMapBase for details.
template <typename KEY, typename VALUE, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiArrayMap : public xiiArrayMapBase<KEY, VALUE>
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

public:
  xiiArrayMap();
  explicit xiiArrayMap(xiiAllocator* pAllocator);

  xiiArrayMap(const xiiArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  xiiArrayMap(const xiiArrayMapBase<KEY, VALUE>& rhs);

  void operator=(const xiiArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  void operator=(const xiiArrayMapBase<KEY, VALUE>& rhs);
};


template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::iterator begin(xiiArrayMapBase<KEY, VALUE>& ref_container)
{
  return begin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_iterator begin(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return begin(container.GetData());
}
template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_iterator cbegin(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return cbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::reverse_iterator rbegin(xiiArrayMapBase<KEY, VALUE>& ref_container)
{
  return rbegin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_reverse_iterator rbegin(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return rbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_reverse_iterator crbegin(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return crbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::iterator end(xiiArrayMapBase<KEY, VALUE>& ref_container)
{
  return end(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_iterator end(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return end(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_iterator cend(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return cend(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::reverse_iterator rend(xiiArrayMapBase<KEY, VALUE>& ref_container)
{
  return rend(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_reverse_iterator rend(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return rend(container.GetData());
}

template <typename KEY, typename VALUE>
typename xiiArrayMapBase<KEY, VALUE>::const_reverse_iterator crend(const xiiArrayMapBase<KEY, VALUE>& container)
{
  return crend(container.GetData());
}


#include <Foundation/Containers/Implementation/ArrayMap_inl.h>
