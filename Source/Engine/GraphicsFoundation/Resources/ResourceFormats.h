#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This stores a native format type for various GAL texture formats.
template <typename NativeFormatType, NativeFormatType InvalidFormat>
class xiiGALFormatLookupEntry
{
public:
  inline xiiGALFormatLookupEntry();

  inline xiiGALFormatLookupEntry(NativeFormatType storage);

  /// \brief This retrieves the render target texture format type.
  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  /// \brief This retrieves the depth texture format type.
  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  /// \brief This retrieves the stencil format type.
  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  /// \brief This retrieves the depth stencil format type.
  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  /// \brief This retrieves the input layout format type.
  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& IL(NativeFormatType inputLayoutType);

  /// \brief This retrieves the resource view format type.
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

  XII_ALWAYS_INLINE const FormatClass& GetFormatInfo(xiiEnum<xiiGALTextureFormat> format) const;

  XII_ALWAYS_INLINE void SetFormatInfo(xiiEnum<xiiGALTextureFormat> format, const FormatClass& newFormatInfo);

private:
  xiiStaticArray<FormatClass, xiiGALTextureFormat::ENUM_COUNT> m_Formats;
};

#include <GraphicsFoundation/Resources/Implementation/ResourceFormats_inl.h>
