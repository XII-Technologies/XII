/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename Type>
xiiProcessingStreamIterator<Type>::xiiProcessingStreamIterator(const xiiProcessingStream* pStream, xiiUInt64 uiNumElements, xiiUInt64 uiStartIndex)
{
  XII_ASSERT_DEV(pStream != nullptr, "Stream pointer may not be null!");
  XII_ASSERT_DEV(pStream->GetElementSize() == sizeof(Type), "Data size missmatch");

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = xiiMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>(uiStartIndex * m_uiElementStride));
  m_pEndPtr     = xiiMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>((uiStartIndex + uiNumElements) * m_uiElementStride));
}

template <typename Type>
XII_ALWAYS_INLINE Type& xiiProcessingStreamIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template <typename Type>
XII_ALWAYS_INLINE bool xiiProcessingStreamIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template <typename Type>
XII_ALWAYS_INLINE void xiiProcessingStreamIterator<Type>::Advance()
{
  m_pCurrentPtr = xiiMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride));
}

template <typename Type>
XII_ALWAYS_INLINE void xiiProcessingStreamIterator<Type>::Advance(xiiUInt32 uiNumElements)
{
  m_pCurrentPtr = xiiMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride * uiNumElements));
}
