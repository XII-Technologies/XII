
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
XII_ALWAYS_INLINE void xiiRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;
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
XII_ALWAYS_INLINE xiiRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd)
{
  m_pCurrent = pStart;
  m_pEnd     = pEnd;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRenderDataBatch::GetCount() const
{
  return m_Data.GetCount();
}

template <typename T>
XII_ALWAYS_INLINE const T* xiiRenderDataBatch::GetFirstData() const
{
  return m_Data.IsEmpty() == false ? xiiStaticCast<const T*>(m_Data.GetPtr()->m_pRenderData) : nullptr;
}

template <typename T>
XII_ALWAYS_INLINE xiiRenderDataBatch::Iterator<T> xiiRenderDataBatch::GetIterator(xiiUInt32 uiStartIndex, xiiUInt32 uiCount) const
{
  xiiUInt32 uiEndIndex = xiiMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex);
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiUInt32 xiiRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

XII_ALWAYS_INLINE const xiiRenderDataBatch& xiiRenderDataBatchList::GetBatch(xiiUInt32 uiIndex) const
{
  return m_Batches[uiIndex];
}
