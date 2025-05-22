
template <typename T>
XII_ALWAYS_INLINE const T& xiiRenderDataBatch::Iterator<T>::operator*() const
{
  return *xiiStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
XII_ALWAYS_INLINE const T* xiiRenderDataBatch::Iterator<T>::operator->() const
{
  return xiiStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
XII_ALWAYS_INLINE xiiRenderDataBatch::Iterator<T>::operator const T*() const
{
  return xiiStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
XII_FORCE_INLINE void xiiRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;

  if (m_Filter.IsValid())
  {
    while (m_pCurrent < m_pEnd && m_Filter(m_pCurrent->m_pRenderData))
    {
      ++m_pCurrent;
    }
  }
}

template <typename T>
XII_ALWAYS_INLINE bool xiiRenderDataBatch::Iterator<T>::IsValid() const
{
  return m_pCurrent < m_pEnd;
}

template <typename T>
XII_ALWAYS_INLINE void xiiRenderDataBatch::Iterator<T>::operator++()
{
  Next();
}

template <typename T>
XII_FORCE_INLINE xiiRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter) :
  m_Filter(filter)
{
  const SortableRenderData* pCurrent = pStart;
  if (m_Filter.IsValid())
  {
    while (pCurrent < pEnd && m_Filter(pCurrent->m_pRenderData))
    {
      ++pCurrent;
    }
  }

  m_pCurrent = pCurrent;
  m_pEnd     = pEnd;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRenderDataBatch::GetCount() const
{
  return m_Data.GetCount();
}

template <typename T>
XII_FORCE_INLINE const T* xiiRenderDataBatch::GetFirstData() const
{
  auto it = Iterator<T>(m_Data.GetPtr(), m_Data.GetPtr() + m_Data.GetCount(), m_Filter);
  return it.IsValid() ? (const T*)it : nullptr;
}

template <typename T>
XII_FORCE_INLINE xiiRenderDataBatch::Iterator<T> xiiRenderDataBatch::GetIterator(xiiUInt32 uiStartIndex, xiiUInt32 uiCount) const
{
  xiiUInt32 uiEndIndex = xiiMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex, m_Filter);
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiUInt32 xiiRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

XII_FORCE_INLINE xiiRenderDataBatch xiiRenderDataBatchList::GetBatch(xiiUInt32 uiIndex) const
{
  xiiRenderDataBatch batch = m_Batches[uiIndex];
  batch.m_Filter           = m_Filter;

  return batch;
}
