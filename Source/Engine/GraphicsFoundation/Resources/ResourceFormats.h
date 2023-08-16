#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

template <typename NativeFormatType, NativeFormatType InvalidFormat>
class xiiGALFormatLookupEntry
{
public:
  inline xiiGALFormatLookupEntry();

  inline xiiGALFormatLookupEntry(NativeFormatType storage);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& IL(NativeFormatType inputLayoutType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eInputLayoutType;
  NativeFormatType m_eResourceViewType;
};

/// \brief Reusable table class to store lookup information (from xiiGALTextureFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class xiiGALFormatLookupTable
{
public:
  xiiGALFormatLookupTable();

  XII_ALWAYS_INLINE const FormatClass& GetFormatInfo(xiiGALTextureFormat::Enum format) const;

  XII_ALWAYS_INLINE void SetFormatInfo(xiiGALTextureFormat::Enum format, const FormatClass& newFormatInfo);

private:
  xiiStaticArray<FormatClass, xiiGALTextureFormat::ENUM_COUNT> m_Formats;
};

#include <GraphicsFoundation/Resources/Implementation/ResourceFormats_inl.h>
