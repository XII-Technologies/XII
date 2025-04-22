
template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper() :
  m_pCommandList(nullptr), m_pMappedData(nullptr)
{
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper(xiiGALCommandList* pCommandList, xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags) :
  m_pCommandList(pCommandList), m_hBuffer(hBuffer), m_pMappedData(nullptr), m_MapType(mapType), m_MapFlags(mapFlags)
{
  Map(pCommandList, hBuffer, mapType, mapFlags).IgnoreResult();
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper(xiiGALMapHelper&& other) noexcept :
  m_pCommandList(other.m_pCommandList), m_hBuffer(other.m_hBuffer), m_pMappedData(other.m_pMappedData), m_MapType(other.m_MapType), m_MapFlags(other.m_MapFlags)
{
  other.m_pCommandList = nullptr;
  other.m_hBuffer      = xiiGALBufferHandle();
  other.m_pMappedData  = nullptr;
  other.m_MapType      = xiiGALMapType::Default;
  other.m_MapFlags     = xiiGALMapFlags::None;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::~xiiGALMapHelper()
{
  Unmap(m_pCommandList, m_hBuffer).IgnoreResult();
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>& xiiGALMapHelper<DataType>::operator=(xiiGALMapHelper&& other) noexcept
{
  m_pCommandList = std::move(other.m_pCommandList);
  m_hBuffer      = std::move(other.m_hBuffer);
  m_pMappedData  = std::move(other.m_pMappedData);
  m_MapType      = std::move(other.m_MapType);
  m_MapFlags     = std::move(other.m_MapFlags);

  other.m_pCommandList = nullptr;
  other.m_hBuffer      = xiiGALBufferHandle();
  other.m_pMappedData  = nullptr;
  other.m_MapType      = xiiGALMapType::Default;
  other.m_MapFlags     = xiiGALMapFlags::None;

  return *this;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiResult xiiGALMapHelper<DataType>::Map(xiiGALCommandList* pCommandList, xiiGALBufferHandle hBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_ASSERT_DEV(!hBuffer.IsInvalidated() && !m_pMappedData && !m_pCommandList, "Buffer is already mapped or invalidated.");

  Unmap().IgnoreResult();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    const auto& bufferDescription = pCommandList->GetDevice()->GetBuffer(hBuffer)->GetDescription();

    XII_IGNORE_UNUSED(bufferDescription);

    XII_ASSERT_DEV(sizeof(DataType) <= bufferDescription.m_uiSize, "Buffer size is smaller than the mapped data type size.");
  }
#endif

  if (pCommandList->MapBuffer(hBuffer, mapType, mapFlags, reinterpret_cast<void**>(&m_pMappedData)).Failed())
  {
    m_pMappedData = nullptr;

    return XII_FAILURE;
  }

  m_pCommandList = pCommandList;
  m_hBuffer      = hBuffer;
  m_MapType      = mapType;
  m_MapFlags     = mapFlags;

  return XII_SUCCESS;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiResult xiiGALMapHelper<DataType>::Unmap(xiiGALCommandList* pCommandList, xiiGALBufferHandle hBuffer)
{
  if (m_pMappedData != nullptr && !m_hBuffer.IsInvalidated())
  {
    m_pCommandList->UnmapBuffer(m_hBuffer, m_pMappedData, m_MapType, m_MapFlags).IgnoreResult();

    m_hBuffer  = xiiGALBufferHandle();
    m_MapType  = xiiGALMapType::Default;
    m_MapFlags = xiiGALMapFlags::None;
  }

  m_pCommandList = nullptr;
  m_pMappedData  = nullptr;

  return XII_SUCCESS;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::operator DataType*()
{
  return m_pMappedData;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::operator const DataType*() const
{
  return m_pMappedData;
}

template <typename DataType>
XII_ALWAYS_INLINE DataType* xiiGALMapHelper<DataType>::operator->()
{
  return m_pMappedData;
}

template <typename DataType>
XII_ALWAYS_INLINE const DataType* xiiGALMapHelper<DataType>::operator->() const
{
  return m_pMappedData;
}
