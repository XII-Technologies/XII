#pragma once

template <typename KEY, typename VALUE>
inline xiiArrayMapBase<KEY, VALUE>::xiiArrayMapBase(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_bSorted = true;
}

template <typename KEY, typename VALUE>
inline xiiArrayMapBase<KEY, VALUE>::xiiArrayMapBase(const xiiArrayMapBase& rhs, xiiAllocatorBase* pAllocator) :
  m_bSorted(rhs.m_bSorted), m_Data(pAllocator)
{
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
inline void xiiArrayMapBase<KEY, VALUE>::operator=(const xiiArrayMapBase& rhs)
{
  m_bSorted = rhs.m_bSorted;
  m_Data    = rhs.m_Data;
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE xiiUInt32 xiiArrayMapBase<KEY, VALUE>::GetCount() const
{
  return m_Data.GetCount();
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE bool xiiArrayMapBase<KEY, VALUE>::IsEmpty() const
{
  return m_Data.IsEmpty();
}

template <typename KEY, typename VALUE>
inline void xiiArrayMapBase<KEY, VALUE>::Clear()
{
  m_bSorted = true;
  m_Data.Clear();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType, typename CompatibleValueType>
inline xiiUInt32 xiiArrayMapBase<KEY, VALUE>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value)
{
  Pair& ref = m_Data.ExpandAndGetRef();
  ref.key   = std::forward<CompatibleKeyType>(key);
  ref.value = std::forward<CompatibleValueType>(value);
  m_bSorted = false;
  return m_Data.GetCount() - 1;
}

template <typename KEY, typename VALUE>
inline void xiiArrayMapBase<KEY, VALUE>::Sort() const
{
  if (m_bSorted)
    return;

  m_bSorted = true;
  m_Data.Sort();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
xiiUInt32 xiiArrayMapBase<KEY, VALUE>::Find(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  xiiUInt32 lb = 0;
  xiiUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const xiiUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else // equal
    {
      return middle;
    }
  }

  return xiiInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
xiiUInt32 xiiArrayMapBase<KEY, VALUE>::LowerBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  xiiUInt32 lb = 0;
  xiiUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const xiiUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  if (lb == m_Data.GetCount())
    return xiiInvalidIndex;

  return lb;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
xiiUInt32 xiiArrayMapBase<KEY, VALUE>::UpperBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  xiiUInt32 lb = 0;
  xiiUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const xiiUInt32 middle = lb + ((ub - lb) >> 1);

    if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else
    {
      lb = middle + 1;
    }
  }

  if (ub == m_Data.GetCount())
    return xiiInvalidIndex;

  return ub;
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE const KEY& xiiArrayMapBase<KEY, VALUE>::GetKey(xiiUInt32 uiIndex) const
{
  return m_Data[uiIndex].key;
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE const VALUE& xiiArrayMapBase<KEY, VALUE>::GetValue(xiiUInt32 uiIndex) const
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
VALUE& xiiArrayMapBase<KEY, VALUE>::GetValue(xiiUInt32 uiIndex)
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE xiiDynamicArray<typename xiiArrayMapBase<KEY, VALUE>::Pair>& xiiArrayMapBase<KEY, VALUE>::GetData()
{
  m_bSorted = false;
  return m_Data;
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE const xiiDynamicArray<typename xiiArrayMapBase<KEY, VALUE>::Pair>& xiiArrayMapBase<KEY, VALUE>::GetData() const
{
  return m_Data;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
VALUE& xiiArrayMapBase<KEY, VALUE>::FindOrAdd(const CompatibleKeyType& key, bool* pExisted)
{
  xiiUInt32 uiIndex = Find<CompatibleKeyType>(key);

  if (pExisted)
    *pExisted = uiIndex != xiiInvalidIndex;

  if (uiIndex == xiiInvalidIndex)
  {
    uiIndex = Insert(key, VALUE());
  }

  return GetValue(uiIndex);
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE VALUE& xiiArrayMapBase<KEY, VALUE>::operator[](const CompatibleKeyType& key)
{
  return FindOrAdd(key);
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE const typename xiiArrayMapBase<KEY, VALUE>::Pair& xiiArrayMapBase<KEY, VALUE>::GetPair(xiiUInt32 uiIndex) const
{
  return m_Data[uiIndex];
}

template <typename KEY, typename VALUE>
void xiiArrayMapBase<KEY, VALUE>::RemoveAtAndCopy(xiiUInt32 uiIndex, bool bKeepSorted)
{
  if (bKeepSorted && m_bSorted)
  {
    m_Data.RemoveAtAndCopy(uiIndex);
  }
  else
  {
    m_Data.RemoveAtAndSwap(uiIndex);
    m_bSorted = false;
  }
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool xiiArrayMapBase<KEY, VALUE>::RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted)
{
  const xiiUInt32 uiIndex = Find(key);

  if (uiIndex == xiiInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex, bKeepSorted);
  return true;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
XII_ALWAYS_INLINE bool xiiArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key) const
{
  return Find(key) != xiiInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool xiiArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key, const VALUE& value) const
{
  xiiUInt32 atpos = LowerBound(key);

  if (atpos == xiiInvalidIndex)
    return false;

  while (atpos < m_Data.GetCount())
  {
    if (m_Data[atpos].key != key)
      return false;

    if (m_Data[atpos].value == value)
      return true;

    ++atpos;
  }

  return false;
}


template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE void xiiArrayMapBase<KEY, VALUE>::Reserve(xiiUInt32 uiSize)
{
  m_Data.Reserve(uiSize);
}

template <typename KEY, typename VALUE>
XII_ALWAYS_INLINE void xiiArrayMapBase<KEY, VALUE>::Compact()
{
  m_Data.Compact();
}

template <typename KEY, typename VALUE>
bool xiiArrayMapBase<KEY, VALUE>::operator==(const xiiArrayMapBase<KEY, VALUE>& rhs) const
{
  Sort();
  rhs.Sort();

  return m_Data == rhs.m_Data;
}

template <typename KEY, typename VALUE, typename A>
xiiArrayMap<KEY, VALUE, A>::xiiArrayMap() :
  xiiArrayMapBase<KEY, VALUE>(A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
xiiArrayMap<KEY, VALUE, A>::xiiArrayMap(xiiAllocatorBase* pAllocator) :
  xiiArrayMapBase<KEY, VALUE>(pAllocator)
{
}

template <typename KEY, typename VALUE, typename A>
xiiArrayMap<KEY, VALUE, A>::xiiArrayMap(const xiiArrayMap<KEY, VALUE, A>& rhs) :
  xiiArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
xiiArrayMap<KEY, VALUE, A>::xiiArrayMap(const xiiArrayMapBase<KEY, VALUE>& rhs) :
  xiiArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
void xiiArrayMap<KEY, VALUE, A>::operator=(const xiiArrayMap<KEY, VALUE, A>& rhs)
{
  xiiArrayMapBase<KEY, VALUE>::operator=(rhs);
}

template <typename KEY, typename VALUE, typename A>
void xiiArrayMap<KEY, VALUE, A>::operator=(const xiiArrayMapBase<KEY, VALUE>& rhs)
{
  xiiArrayMapBase<KEY, VALUE>::operator=(rhs);
}
