/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper() :
  m_pCommandList(nullptr), m_pBuffer(nullptr), m_pMappedData(nullptr), m_MapType(static_cast<xiiGALMapType::Enum>(-1)), m_MapFlags(static_cast<xiiGALMapFlags::Enum>(-1))
{
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper(xiiGALCommandList* pCommandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags) :
  xiiGALMapHelper()
{
  Map(pCommandList, pBuffer, mapType, mapFlags).IgnoreResult();
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper(xiiGALCommandList& commandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags) :
  xiiGALMapHelper()
{
  Map(&commandList, pBuffer, mapType, mapFlags).IgnoreResult();
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::xiiGALMapHelper(xiiGALMapHelper&& other) noexcept :
  m_pCommandList(other.m_pCommandList), m_pBuffer(other.m_pBuffer), m_pMappedData(other.m_pMappedData), m_MapType(other.m_MapType), m_MapFlags(other.m_MapFlags)
{
  other.m_pCommandList = nullptr;
  other.m_pBuffer      = nullptr;
  other.m_pMappedData = nullptr;
  other.m_MapType     = xiiGALMapType::Default;
  other.m_MapFlags    = xiiGALMapFlags::None;
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>::~xiiGALMapHelper()
{
  Unmap().IgnoreResult();
}

template <typename DataType>
XII_ALWAYS_INLINE xiiGALMapHelper<DataType>& xiiGALMapHelper<DataType>::operator=(xiiGALMapHelper&& other) noexcept
{
  if (this == &other)
    return *this;

  Unmap().IgnoreResult();

  m_pCommandList = other.m_pCommandList;
  m_pBuffer      = other.m_pBuffer;
  m_pMappedData  = other.m_pMappedData;
  m_MapType      = other.m_MapType;
  m_MapFlags     = other.m_MapFlags;

  other.m_pCommandList = nullptr;
  other.m_pBuffer      = nullptr;
  other.m_pMappedData  = nullptr;
  other.m_MapType     = xiiGALMapType::Default;
  other.m_MapFlags    = xiiGALMapFlags::None;

  return *this;
}

template <typename DataType>
XII_FORCE_INLINE xiiResult xiiGALMapHelper<DataType>::Map(xiiGALCommandList* pCommandList, xiiGALBuffer* pBuffer, xiiEnum<xiiGALMapType> mapType, xiiBitflags<xiiGALMapFlags> mapFlags)
{
  XII_ASSERT_DEV(!m_pBuffer && !m_pMappedData && !m_pCommandList, "Buffer is already mapped or invalidated.");

  Unmap().IgnoreResult();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    const auto& bufferDescription = pBuffer->GetDescription();

    XII_ASSERT_DEBUG(sizeof(DataType) <= bufferDescription.m_uiSize, "Buffer size is smaller than the mapped data type size.");
  }
#endif

  if (pCommandList->MapBuffer(pBuffer, mapType, mapFlags, reinterpret_cast<void*&>(m_pMappedData)).Failed())
  {
    m_pMappedData = nullptr;

    return XII_FAILURE;
  }

  m_pCommandList = pCommandList;
  m_pBuffer      = pBuffer;
  m_MapType      = mapType;
  m_MapFlags     = mapFlags;

  if (m_MapFlags.IsSet(xiiGALMapFlags::Discard))
  {
    xiiMemoryUtils::ZeroFill((DataType*)m_pMappedData, 1);
  }
  return XII_SUCCESS;
}

template <typename DataType>
XII_FORCE_INLINE xiiResult xiiGALMapHelper<DataType>::Unmap()
{
  xiiResult result = XII_SUCCESS;

  if (m_pBuffer)
  {
    result = m_pCommandList->UnmapBuffer(m_pBuffer, m_MapType);

    m_pBuffer  = nullptr;
    m_MapType  = static_cast<xiiGALMapType::Enum>(-1);
    m_MapFlags = static_cast<xiiGALMapFlags::Enum>(-1);
  }

  m_pCommandList = nullptr;
  m_pMappedData  = nullptr;

  return result;
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

template <typename DataType>
XII_ALWAYS_INLINE DataType* xiiGALMapHelper<DataType>::GetMappedData() const
{
  return m_pMappedData;
}
