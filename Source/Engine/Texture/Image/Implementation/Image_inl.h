/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
struct xiiImageSizeofHelper
{
  static constexpr size_t Size = sizeof(T);
};

template <>
struct xiiImageSizeofHelper<void>
{
  static constexpr size_t Size = 1;
};

template <>
struct xiiImageSizeofHelper<const void>
{
  static constexpr size_t Size = 1;
};

template <typename T>
xiiBlobPtr<const T> xiiImageView::GetBlobPtr() const
{
  for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<T>(uiPlaneIndex);
  }
  return xiiBlobPtr<const T>(reinterpret_cast<T*>(static_cast<xiiUInt8*>(m_DataPtr.GetPtr())), m_DataPtr.GetCount() / xiiImageSizeofHelper<T>::Size);
}

inline xiiConstByteBlobPtr xiiImageView::GetByteBlobPtr() const
{
  for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<xiiUInt8>(uiPlaneIndex);
  }
  return xiiConstByteBlobPtr(static_cast<xiiUInt8*>(m_DataPtr.GetPtr()), m_DataPtr.GetCount());
}

template <typename T>
xiiBlobPtr<T> xiiImage::GetBlobPtr()
{
  xiiBlobPtr<const T> pConstPtr = xiiImageView::GetBlobPtr<T>();

  return xiiBlobPtr<T>(const_cast<T*>(static_cast<const T*>(pConstPtr.GetPtr())), pConstPtr.GetCount());
}

inline xiiByteBlobPtr xiiImage::GetByteBlobPtr()
{
  xiiConstByteBlobPtr pConstPtr = xiiImageView::GetByteBlobPtr();

  return xiiByteBlobPtr(const_cast<xiiUInt8*>(pConstPtr.GetPtr()), pConstPtr.GetCount());
}

template <typename T>
const T* xiiImageView::GetPixelPointer(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 x /*= 0*/, xiiUInt32 y /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  ValidateDataTypeAccessor<T>(uiPlaneIndex);

  XII_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel, uiPlaneIndex), "Invalid x coordinate ({}) for mip level {} and plane {}.", x, uiMipLevel, uiPlaneIndex);
  XII_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel, uiPlaneIndex), "Invalid y coordinate ({}) for mip level {} and plane {}.", y, uiMipLevel, uiPlaneIndex);
  XII_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel, uiPlaneIndex), "Invalid z coordinate ({}) for mip level {} and plane {}.", z, uiMipLevel, uiPlaneIndex);

  xiiUInt32 uiBytesPerBlock = 0;
  if (xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format))
  {
    uiBytesPerBlock = xiiGALTextureUtilities::GetMultiPlanarFormatProperties(m_Description.m_Format).GetPlane(uiPlaneIndex).m_uiBytesPerElement;
  }
  else
  {
    uiBytesPerBlock = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format).GetElementSize();
  }

  xiiUInt64 uiOffset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex) + y * GetRowPitch(uiMipLevel, uiPlaneIndex) + x * uiBytesPerBlock;

  return reinterpret_cast<const T*>(&m_DataPtr[uiOffset]);
}

template <typename T>
T* xiiImage::GetPixelPointer(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 x /*= 0*/, xiiUInt32 y /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/)
{
  return const_cast<T*>(xiiImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z, uiPlaneIndex));
}

template <typename T>
void xiiImageView::ValidateDataTypeAccessor([[maybe_unused]] xiiUInt32 uiPlaneIndex) const
{
  xiiUInt32 uiBytesPerBlock = 0;
  if (xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format))
  {
    uiBytesPerBlock = xiiGALTextureUtilities::GetMultiPlanarFormatProperties(m_Description.m_Format).GetPlane(uiPlaneIndex).m_uiBytesPerElement;
  }
  else
  {
    uiBytesPerBlock = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format).GetElementSize();
  }

  XII_ASSERT_DEV(uiBytesPerBlock % xiiImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
