/// Copyright (c) Theophilus Eriata. All Rights Reserved.

/// Value used by containers for indices to indicate an invalid index.

#ifndef xiiInvalidIndex
#  define xiiInvalidIndex 0xFFFFFFFFU
#endif

// ***** Const Iterator *****

template <typename K, typename V, typename H>
xiiHashTableBaseConstIterator<K, V, H>::xiiHashTableBaseConstIterator(const xiiHashTableBase<K, V, H>& hashTable) :
  m_pHashTable(&hashTable)
{
}

template <typename K, typename V, typename H>
void xiiHashTableBaseConstIterator<K, V, H>::SetToBegin()
{
  if (m_pHashTable->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
    return;
  }
  while (!m_pHashTable->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename V, typename H>
inline void xiiHashTableBaseConstIterator<K, V, H>::SetToEnd()
{
  m_uiCurrentCount = m_pHashTable->m_uiCount;
  m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE bool xiiHashTableBaseConstIterator<K, V, H>::IsValid() const
{
  return m_uiCurrentCount < m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE bool xiiHashTableBaseConstIterator<K, V, H>::operator==(const xiiHashTableBaseConstIterator<K, V, H>& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashTable->m_pEntries == rhs.m_pHashTable->m_pEntries;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE const K& xiiHashTableBaseConstIterator<K, V, H>::Key() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].key;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE const V& xiiHashTableBaseConstIterator<K, V, H>::Value() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
void xiiHashTableBaseConstIterator<K, V, H>::Next()
{
  // if we already iterated over the amount of valid elements that the hash-table stores, early out
  if (m_uiCurrentCount >= m_pHashTable->m_uiCount)
    return;

  // increase the counter of how many elements we have seen
  ++m_uiCurrentCount;
  // increase the index of the element to look at
  ++m_uiCurrentIndex;

  // check that we don't leave the valid range of element indices
  while (m_uiCurrentIndex < m_pHashTable->m_uiCapacity)
  {
    if (m_pHashTable->IsValidEntry(m_uiCurrentIndex))
      return;

    ++m_uiCurrentIndex;
  }

  // if we fell through this loop, we reached the end of all elements in the container
  // set the m_uiCurrentCount to maximum, to enable early-out in the future and to make 'IsValid' return 'false'
  m_uiCurrentCount = m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE void xiiHashTableBaseConstIterator<K, V, H>::operator++()
{
  Next();
}

// These functions are used for structured bindings.
// They describe how many elements can be accessed in the binding and which type they are.
namespace std
{
  template <typename K, typename V, typename H>
  struct tuple_size<xiiHashTableBaseConstIterator<K, V, H>> : integral_constant<size_t, 2>
  {
  };

  template <typename K, typename V, typename H>
  struct tuple_element<0, xiiHashTableBaseConstIterator<K, V, H>>
  {
    using type = const K&;
  };

  template <typename K, typename V, typename H>
  struct tuple_element<1, xiiHashTableBaseConstIterator<K, V, H>>
  {
    using type = const V&;
  };
} // namespace std

// ***** Iterator *****

template <typename K, typename V, typename H>
xiiHashTableBaseIterator<K, V, H>::xiiHashTableBaseIterator(const xiiHashTableBase<K, V, H>& hashTable) :
  xiiHashTableBaseConstIterator<K, V, H>(hashTable)
{
}

template <typename K, typename V, typename H>
xiiHashTableBaseIterator<K, V, H>::xiiHashTableBaseIterator(const xiiHashTableBaseIterator<K, V, H>& rhs) :
  xiiHashTableBaseConstIterator<K, V, H>(*rhs.m_pHashTable)
{
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE void xiiHashTableBaseIterator<K, V, H>::operator=(const xiiHashTableBaseIterator& rhs) // [tested]
{
  this->m_pHashTable     = rhs.m_pHashTable;
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE V& xiiHashTableBaseIterator<K, V, H>::Value()
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE V& xiiHashTableBaseIterator<K, V, H>::Value() const
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}

// These functions are used for structured bindings.
// They describe how many elements can be accessed in the binding and which type they are.
namespace std
{
  template <typename K, typename V, typename H>
  struct tuple_size<xiiHashTableBaseIterator<K, V, H>> : integral_constant<size_t, 2>
  {
  };

  template <typename K, typename V, typename H>
  struct tuple_element<0, xiiHashTableBaseIterator<K, V, H>>
  {
    using type = const K&;
  };

  template <typename K, typename V, typename H>
  struct tuple_element<1, xiiHashTableBaseIterator<K, V, H>>
  {
    using type = V&;
  };
} // namespace std

// ***** xiiHashTableBase *****

template <typename K, typename V, typename H>
xiiHashTableBase<K, V, H>::xiiHashTableBase(xiiAllocator* pAllocator)
{
  m_pEntries    = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount     = 0;
  m_uiCapacity  = 0;
  m_pAllocator  = pAllocator;
}

template <typename K, typename V, typename H>
xiiHashTableBase<K, V, H>::xiiHashTableBase(const xiiHashTableBase<K, V, H>& other, xiiAllocator* pAllocator)
{
  m_pEntries    = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount     = 0;
  m_uiCapacity  = 0;
  m_pAllocator  = pAllocator;

  *this = other;
}

template <typename K, typename V, typename H>
xiiHashTableBase<K, V, H>::xiiHashTableBase(xiiHashTableBase<K, V, H>&& other, xiiAllocator* pAllocator)
{
  m_pEntries    = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount     = 0;
  m_uiCapacity  = 0;
  m_pAllocator  = pAllocator;

  *this = std::move(other);
}

template <typename K, typename V, typename H>
xiiHashTableBase<K, V, H>::~xiiHashTableBase()
{
  Clear();
  XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::operator=(const xiiHashTableBase<K, V, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  xiiUInt32 uiCopied = 0;
  for (xiiUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i].key, rhs.m_pEntries[i].value);
      ++uiCopied;
    }
  }
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::operator=(xiiHashTableBase<K, V, H>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  Clear();

  if (m_pAllocator != rhs.m_pAllocator)
  {
    Reserve(rhs.m_uiCapacity);

    xiiUInt32 uiCopied = 0;
    for (xiiUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
    {
      if (rhs.IsValidEntry(i))
      {
        Insert(std::move(rhs.m_pEntries[i].key), std::move(rhs.m_pEntries[i].value));
        ++uiCopied;
      }
    }

    rhs.Clear();
  }
  else
  {
    XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);

    // Move all data over.
    m_pEntries    = rhs.m_pEntries;
    m_pEntryFlags = rhs.m_pEntryFlags;
    m_uiCount     = rhs.m_uiCount;
    m_uiCapacity  = rhs.m_uiCapacity;

    // Temp copy forgets all its state.
    rhs.m_pEntries    = nullptr;
    rhs.m_pEntryFlags = nullptr;
    rhs.m_uiCount     = 0;
    rhs.m_uiCapacity  = 0;
  }
}

template <typename K, typename V, typename H>
bool xiiHashTableBase<K, V, H>::operator==(const xiiHashTableBase<K, V, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  xiiUInt32 uiCompared = 0;
  for (xiiUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      const V* pRhsValue = nullptr;
      if (!rhs.TryGetValue(m_pEntries[i].key, pRhsValue))
        return false;

      if (m_pEntries[i].value != *pRhsValue)
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::Reserve(xiiUInt32 uiCapacity)
{
  const xiiUInt64 uiCap64         = static_cast<xiiUInt64>(uiCapacity);
  xiiUInt64       uiNewCapacity64 = uiCap64 + (uiCap64 * 2 / 3); // ensure a maximum load of 60%

  uiNewCapacity64 = xiiMath::Min<xiiUInt64>(uiNewCapacity64, 0x80000000llu); // the largest power-of-two in 32 bit

  xiiUInt32 uiNewCapacity32 = static_cast<xiiUInt32>(uiNewCapacity64 & 0xFFFFFFFF);
  XII_ASSERT_DEBUG(uiCapacity <= uiNewCapacity32, "xiiHashSet/Map do not support more than 2 billion entries.");

  if (m_uiCapacity >= uiNewCapacity32)
    return;

  uiNewCapacity32 = xiiMath::Max<xiiUInt32>(xiiMath::PowerOfTwo_Ceil(uiNewCapacity32), CAPACITY_ALIGNMENT);
  SetCapacity(uiNewCapacity32);
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the table is empty.
    XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    XII_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
    m_uiCapacity = 0;
  }
  else
  {
    const xiiUInt32 uiNewCapacity = xiiMath::PowerOfTwo_Ceil(m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
    {
      SetCapacity(uiNewCapacity);
    }
  }
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE xiiUInt32 xiiHashTableBase<K, V, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE bool xiiHashTableBase<K, V, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::Clear()
{
  for (xiiUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      xiiMemoryUtils::Destruct(&m_pEntries[i].key, 1);
      xiiMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  xiiMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType, typename CompatibleValueType>
bool xiiHashTableBase<K, V, H>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value, V* out_pOldValue /*= nullptr*/)
{
  Reserve(m_uiCount + 1);

  xiiUInt32 uiIndex        = H::Hash(key) & (m_uiCapacity - 1);
  xiiUInt32 uiDeletedIndex = xiiInvalidIndex;

  xiiUInt32 uiCounter = 0;
  while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
  {
    if (IsDeletedEntry(uiIndex))
    {
      if (uiDeletedIndex == xiiInvalidIndex)
      {
        uiDeletedIndex = uiIndex;
      }
    }
    else if (H::Equal(m_pEntries[uiIndex].key, key))
    {
      if (out_pOldValue != nullptr)
      {
        *out_pOldValue = std::move(m_pEntries[uiIndex].value);
      }

      m_pEntries[uiIndex].value = std::forward<CompatibleValueType>(value); // Either move or copy assignment.
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
    {
      uiIndex = 0;
    }

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != xiiInvalidIndex ? uiDeletedIndex : uiIndex;

  // Both constructions might either be a move or a copy.
  xiiMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].key, std::forward<CompatibleKeyType>(key));
  xiiMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].value, std::forward<CompatibleValueType>(value));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
bool xiiHashTableBase<K, V, H>::Remove(const CompatibleKeyType& key, V* out_pOldValue /*= nullptr*/)
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex != xiiInvalidIndex)
  {
    if (out_pOldValue != nullptr)
    {
      *out_pOldValue = std::move(m_pEntries[uiIndex].value);
    }

    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
typename xiiHashTableBase<K, V, H>::Iterator xiiHashTableBase<K, V, H>::Remove(const typename xiiHashTableBase<K, V, H>::Iterator& pos)
{
  XII_ASSERT_DEBUG(pos.m_pHashTable == this, "Iterator from wrong hashtable");
  Iterator  it      = pos;
  xiiUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;

  RemoveInternal(uiIndex);

  return it;
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::RemoveInternal(xiiUInt32 uiIndex)
{
  xiiMemoryUtils::Destruct(&m_pEntries[uiIndex].key, 1);
  xiiMemoryUtils::Destruct(&m_pEntries[uiIndex].value, 1);

  xiiUInt32 uiNextIndex = uiIndex + 1;
  if (uiNextIndex == m_uiCapacity)
  {
    uiNextIndex = 0;
  }

  // if the next entry is free we are at the end of a chain and
  // can immediately mark this entry as free as well
  if (IsFreeEntry(uiNextIndex))
  {
    MarkEntryAsFree(uiIndex);

    // run backwards and free all deleted entries in this chain
    xiiUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
    --uiPrevIndex;

    while (IsDeletedEntry(uiPrevIndex))
    {
      MarkEntryAsFree(uiPrevIndex);

      if (uiPrevIndex == 0)
      {
        uiPrevIndex = m_uiCapacity;
      }

      --uiPrevIndex;
    }
  }
  else
  {
    MarkEntryAsDeleted(uiIndex);
  }

  --m_uiCount;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool xiiHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V& out_value) const
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex != xiiInvalidIndex)
  {
    XII_ASSERT_DEBUG(m_pEntries != nullptr, "No entries present"); // To fix static analysis.

    out_value = m_pEntries[uiIndex].value;
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool xiiHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, const V*& out_pValue) const
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex != xiiInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    XII_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool xiiHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V*& out_pValue) const
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex != xiiInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    XII_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename xiiHashTableBase<K, V, H>::ConstIterator xiiHashTableBase<K, V, H>::Find(const CompatibleKeyType& key) const
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex == xiiInvalidIndex)
  {
    return GetEndIterator();
  }

  ConstIterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0

  return it;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename xiiHashTableBase<K, V, H>::Iterator xiiHashTableBase<K, V, H>::Find(const CompatibleKeyType& key)
{
  xiiUInt32 uiIndex = FindEntry(key);
  if (uiIndex == xiiInvalidIndex)
  {
    return GetEndIterator();
  }

  Iterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0
  return it;
}


template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline const V* xiiHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key) const
{
  xiiUInt32 uiIndex = FindEntry(key);
  return (uiIndex != xiiInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline V* xiiHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key)
{
  xiiUInt32 uiIndex = FindEntry(key);
  return (uiIndex != xiiInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
inline V& xiiHashTableBase<K, V, H>::operator[](const K& key)
{
  return FindOrAdd(key, nullptr);
}

template <typename K, typename V, typename H>
V& xiiHashTableBase<K, V, H>::FindOrAdd(const K& key, bool* out_pExisted)
{
  const xiiUInt32 uiHash  = H::Hash(key);
  xiiUInt32       uiIndex = FindEntry(uiHash, key);

  if (out_pExisted)
  {
    *out_pExisted = uiIndex != xiiInvalidIndex;
  }

  if (uiIndex == xiiInvalidIndex)
  {
    Reserve(m_uiCount + 1);

    // search for suitable insertion index again, table might have been resized
    uiIndex = uiHash & (m_uiCapacity - 1);
    while (IsValidEntry(uiIndex))
    {
      ++uiIndex;
      if (uiIndex == m_uiCapacity)
      {
        uiIndex = 0;
      }
    }

    // new entry
    xiiMemoryUtils::CopyConstruct(&m_pEntries[uiIndex].key, key, 1);
    xiiMemoryUtils::Construct<ConstructAll>(&m_pEntries[uiIndex].value, 1);
    MarkEntryAsValid(uiIndex);
    ++m_uiCount;
  }

  XII_ASSERT_DEBUG(m_pEntries != nullptr, "Entries should be present");

  return m_pEntries[uiIndex].value;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
XII_FORCE_INLINE bool xiiHashTableBase<K, V, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != xiiInvalidIndex;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE typename xiiHashTableBase<K, V, H>::Iterator xiiHashTableBase<K, V, H>::GetIterator()
{
  Iterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE typename xiiHashTableBase<K, V, H>::Iterator xiiHashTableBase<K, V, H>::GetEndIterator()
{
  Iterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE typename xiiHashTableBase<K, V, H>::ConstIterator xiiHashTableBase<K, V, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE typename xiiHashTableBase<K, V, H>::ConstIterator xiiHashTableBase<K, V, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE xiiAllocator* xiiHashTableBase<K, V, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename V, typename H>
xiiUInt64 xiiHashTableBase<K, V, H>::GetHeapMemoryUsage() const
{
  return ((xiiUInt64)m_uiCapacity * sizeof(Entry)) + (sizeof(xiiUInt32) * (xiiUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::SetCapacity(xiiUInt32 uiCapacity)
{
  XII_ASSERT_DEBUG(xiiMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const xiiUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity                  = uiCapacity;

  Entry*     pOldEntries    = m_pEntries;
  xiiUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries    = XII_NEW_RAW_BUFFER(m_pAllocator, Entry, m_uiCapacity);
  m_pEntryFlags = XII_NEW_RAW_BUFFER(m_pAllocator, xiiUInt32, GetFlagsCapacity());
  xiiMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (xiiUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      XII_VERIFY(!Insert(std::move(pOldEntries[i].key), std::move(pOldEntries[i].value)), "Implementation error");

      xiiMemoryUtils::Destruct(&pOldEntries[i].key, 1);
      xiiMemoryUtils::Destruct(&pOldEntries[i].value, 1);
    }
  }

  XII_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  XII_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE xiiUInt32 xiiHashTableBase<K, V, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline xiiUInt32 xiiHashTableBase<K, V, H>::FindEntry(xiiUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    xiiUInt32 uiIndex   = uiHash & (m_uiCapacity - 1);
    xiiUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex].key, key))
        return uiIndex;

      ++uiIndex;
      if (uiIndex == m_uiCapacity)
      {
        uiIndex = 0;
      }

      ++uiCounter;
    }
  }
  // not found
  return xiiInvalidIndex;
}

#define XII_HASHTABLE_USE_BITFLAGS XII_ON

template <typename K, typename V, typename H>
XII_FORCE_INLINE xiiUInt32 xiiHashTableBase<K, V, H>::GetFlagsCapacity() const
{
#if XII_ENABLED(XII_HASHTABLE_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename V, typename H>
XII_ALWAYS_INLINE xiiUInt32 xiiHashTableBase<K, V, H>::GetFlags(xiiUInt32* pFlags, xiiUInt32 uiEntryIndex) const
{
#if XII_ENABLED(XII_HASHTABLE_USE_BITFLAGS)
  const xiiUInt32 uiIndex    = uiEntryIndex / 16;
  const xiiUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename V, typename H>
void xiiHashTableBase<K, V, H>::SetFlags(xiiUInt32 uiEntryIndex, xiiUInt32 uiFlags)
{
#if XII_ENABLED(XII_HASHTABLE_USE_BITFLAGS)
  const xiiUInt32 uiIndex    = uiEntryIndex / 16;
  const xiiUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  XII_ASSERT_DEBUG(uiIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
  m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
#else
  XII_ASSERT_DEBUG(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiEntryIndex] = uiFlags;
#endif
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE bool xiiHashTableBase<K, V, H>::IsFreeEntry(xiiUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE bool xiiHashTableBase<K, V, H>::IsValidEntry(xiiUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE bool xiiHashTableBase<K, V, H>::IsDeletedEntry(xiiUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE void xiiHashTableBase<K, V, H>::MarkEntryAsFree(xiiUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE void xiiHashTableBase<K, V, H>::MarkEntryAsValid(xiiUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename V, typename H>
XII_FORCE_INLINE void xiiHashTableBase<K, V, H>::MarkEntryAsDeleted(xiiUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable() :
  xiiHashTableBase<K, V, H>(A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable(xiiAllocator* pAllocator) :
  xiiHashTableBase<K, V, H>(pAllocator)
{
}

template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable(const xiiHashTable<K, V, H, A>& other) :
  xiiHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable(const xiiHashTableBase<K, V, H>& other) :
  xiiHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable(xiiHashTable<K, V, H, A>&& other) :
  xiiHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
xiiHashTable<K, V, H, A>::xiiHashTable(xiiHashTableBase<K, V, H>&& other) :
  xiiHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
void xiiHashTable<K, V, H, A>::operator=(const xiiHashTable<K, V, H, A>& rhs)
{
  xiiHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void xiiHashTable<K, V, H, A>::operator=(const xiiHashTableBase<K, V, H>& rhs)
{
  xiiHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void xiiHashTable<K, V, H, A>::operator=(xiiHashTable<K, V, H, A>&& rhs)
{
  xiiHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename K, typename V, typename H, typename A>
void xiiHashTable<K, V, H, A>::operator=(xiiHashTableBase<K, V, H>&& rhs)
{
  xiiHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename ValueType, typename Hasher>
void xiiHashTableBase<KeyType, ValueType, Hasher>::Swap(xiiHashTableBase<KeyType, ValueType, Hasher>& other)
{
  xiiMath::Swap(this->m_pEntries, other.m_pEntries);
  xiiMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  xiiMath::Swap(this->m_uiCount, other.m_uiCount);
  xiiMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  xiiMath::Swap(this->m_pAllocator, other.m_pAllocator);
}
