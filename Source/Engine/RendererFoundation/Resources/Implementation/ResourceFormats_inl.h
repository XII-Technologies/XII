
// static
XII_ALWAYS_INLINE xiiUInt32 xiiGALResourceFormat::GetBitsPerElement(xiiGALResourceFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
XII_ALWAYS_INLINE xiiUInt8 xiiGALResourceFormat::GetChannelCount(xiiGALResourceFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
XII_FORCE_INLINE bool xiiGALResourceFormat::IsDepthFormat(xiiGALResourceFormat::Enum format)
{
  return format == DFloat || format == D16 || format == D24S8;
}

// static
XII_FORCE_INLINE bool xiiGALResourceFormat::IsStencilFormat(Enum format)
{
  return format == D24S8;
}

// static
XII_FORCE_INLINE bool xiiGALResourceFormat::IsSrgb(xiiGALResourceFormat::Enum format)
{
  return format == BGRAUByteNormalizedsRGB || format == RGBAUByteNormalizedsRGB || format == BC1sRGB || format == BC2sRGB || format == BC3sRGB ||
    format == BC7UNormalizedsRGB;
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::xiiGALFormatLookupEntry() :
  m_eStorage(InvalidFormat), m_eRenderTarget(InvalidFormat), m_eDepthOnlyType(InvalidFormat), m_eStencilOnlyType(InvalidFormat), m_eDepthStencilType(InvalidFormat), m_eVertexAttributeType(InvalidFormat), m_eResourceViewType(InvalidFormat)
{
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::xiiGALFormatLookupEntry(NativeFormatType Storage) :
  m_eStorage(Storage), m_eRenderTarget(InvalidFormat), m_eDepthOnlyType(InvalidFormat), m_eStencilOnlyType(InvalidFormat), m_eDepthStencilType(InvalidFormat), m_eVertexAttributeType(InvalidFormat), m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(
  NativeFormatType RenderTargetType)
{
  m_eRenderTarget = RenderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType DepthOnlyType)
{
  m_eDepthOnlyType = DepthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType StencilOnlyType)
{
  m_eStencilOnlyType = StencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(
  NativeFormatType DepthStencilType)
{
  m_eDepthStencilType = DepthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(
  NativeFormatType VertexAttributeType)
{
  m_eVertexAttributeType = VertexAttributeType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(
  NativeFormatType ResourceViewType)
{
  m_eResourceViewType = ResourceViewType;
  return *this;
}


template <typename FormatClass>
xiiGALFormatLookupTable<FormatClass>::xiiGALFormatLookupTable()
{
  for (xiiUInt32 i = 0; i < xiiGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template <typename FormatClass>
const FormatClass& xiiGALFormatLookupTable<FormatClass>::GetFormatInfo(xiiGALResourceFormat::Enum eFormat) const
{
  return m_Formats[eFormat];
}

template <typename FormatClass>
void xiiGALFormatLookupTable<FormatClass>::SetFormatInfo(xiiGALResourceFormat::Enum eFormat, const FormatClass& NewFormatInfo)
{
  m_Formats[eFormat] = NewFormatInfo;
}
