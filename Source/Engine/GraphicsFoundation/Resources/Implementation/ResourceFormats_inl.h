
template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::xiiGALFormatLookupEntry() :
  m_eStorage(InvalidFormat), m_eRenderTarget(InvalidFormat), m_eDepthOnlyType(InvalidFormat), m_eStencilOnlyType(InvalidFormat), m_eDepthStencilType(InvalidFormat), m_eInputLayoutType(InvalidFormat), m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::xiiGALFormatLookupEntry(NativeFormatType storage) :
  m_eStorage(storage), m_eRenderTarget(InvalidFormat), m_eDepthOnlyType(InvalidFormat), m_eStencilOnlyType(InvalidFormat), m_eDepthStencilType(InvalidFormat), m_eInputLayoutType(InvalidFormat), m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(NativeFormatType renderTargetType)
{
  m_eRenderTarget = renderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType depthOnlyType)
{
  m_eDepthOnlyType = depthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType stencilOnlyType)
{
  m_eStencilOnlyType = stencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(NativeFormatType depthStencilType)
{
  m_eDepthStencilType = depthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::IL(NativeFormatType inputLayoutType)
{
  m_eInputLayoutType = inputLayoutType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(NativeFormatType resourceViewType)
{
  m_eResourceViewType = resourceViewType;
  return *this;
}


template <typename FormatClass>
xiiGALFormatLookupTable<FormatClass>::xiiGALFormatLookupTable()
{
  for (xiiUInt32 i = 0; i < xiiGALTextureFormat::ENUM_COUNT; ++i)
  {
    m_Formats.PushBack(FormatClass());
  }
}

template <typename FormatClass>
const FormatClass& xiiGALFormatLookupTable<FormatClass>::GetFormatInfo(xiiEnum<xiiGALTextureFormat> format) const
{
  return m_Formats[format];
}

template <typename FormatClass>
void xiiGALFormatLookupTable<FormatClass>::SetFormatInfo(xiiEnum<xiiGALTextureFormat> format, const FormatClass& newFormatInfo)
{
  m_Formats[format] = newFormatInfo;
}
