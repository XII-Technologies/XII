/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

xiiImageView::xiiImageView()
{
  Clear();
}

xiiImageView::xiiImageView(const xiiImageHeader& header, xiiConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void xiiImageView::Clear()
{
  xiiImageHeader::Clear();
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool xiiImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void xiiImageView::ResetAndViewExternalStorage(const xiiImageHeader& header, xiiConstByteBlobPtr imageData)
{
  static_cast<xiiImageHeader&>(*this) = header;

  xiiUInt64 uiDataSize = ComputeLayout();

  XII_IGNORE_UNUSED(uiDataSize);
  XII_ASSERT_DEV(imageData.GetCount() == uiDataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)", imageData.GetCount(), uiDataSize);

  // Const cast is safe here as we will only perform non-const access if this is a xiiImage which owns mutable access to the storage
  m_DataPtr = xiiBlobPtr<xiiUInt8>(const_cast<xiiUInt8*>(static_cast<const xiiUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

xiiResult xiiImageView::SaveTo(xiiStringView sFileName) const
{
  XII_LOG_BLOCK("Writing Image", sFileName);

  if (m_Format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Cannot write image '{0}' - image data is invalid or empty", sFileName);
    return XII_FAILURE;
  }

  xiiFileWriter writer;
  if (writer.Open(sFileName) == XII_FAILURE)
  {
    xiiLog::Error("Failed to open image file '{0}'", sFileName);
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(sFileName);

  if (const xiiImageFileFormat* pFormat = xiiImageFileFormat::GetWriterFormat(it))
  {
    if (pFormat->WriteImage(writer, *this, it) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write image file '{0}'", sFileName);
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Error("No known image file format for extension '{0}'", it);
  return XII_FAILURE;
}

const xiiImageHeader& xiiImageView::GetHeader() const
{
  return *this;
}

xiiImageView xiiImageView::GetRowView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 y /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  xiiImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the subformat
  xiiImageFormat::Enum subFormat = xiiImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * xiiImageFormat::GetBlockWidth(subFormat) / xiiImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(xiiImageFormat::GetBlockHeight(m_Format, 0) * xiiImageFormat::GetBlockHeight(subFormat) / xiiImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(xiiImageFormat::GetBlockDepth(subFormat) / xiiImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(xiiImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex));

  xiiUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  xiiBlobPtr<const xiiUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return xiiImageView(header, xiiConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void xiiImageView::ReinterpretAs(xiiImageFormat::Enum format)
{
  XII_ASSERT_DEBUG(xiiImageFormat::IsCompressed(format) == xiiImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");
  XII_ASSERT_DEBUG(xiiImageFormat::GetBitsPerPixel(GetImageFormat()) == xiiImageFormat::GetBitsPerPixel(format), "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

xiiUInt64 xiiImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices * GetPlaneCount());

  xiiUInt64 uiDataSize = 0;

  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (xiiUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  // Push back total size as a marker
  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void xiiImageView::ValidateSubImageIndices(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const
{
  XII_IGNORE_UNUSED(uiMipLevel);
  XII_IGNORE_UNUSED(uiFace);
  XII_IGNORE_UNUSED(uiArrayIndex);
  XII_IGNORE_UNUSED(uiPlaneIndex);

  XII_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  XII_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  XII_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  XII_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const xiiUInt64& xiiImageView::GetSubImageOffset(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex))];
}

xiiImage::xiiImage()
{
  Clear();
}

xiiImage::xiiImage(const xiiImageHeader& header)
{
  ResetAndAlloc(header);
}

xiiImage::xiiImage(const xiiImageHeader& header, xiiByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(header, externalData);
}

xiiImage::xiiImage(xiiImage&& other)
{
  ResetAndMove(std::move(other));
}

xiiImage::xiiImage(const xiiImageView& other)
{
  ResetAndCopy(other);
}

void xiiImage::operator=(xiiImage&& rhs)
{
  ResetAndMove(std::move(rhs));
}

void xiiImage::Clear()
{
  m_InternalStorage.Clear();

  xiiImageView::Clear();
}

void xiiImage::ResetAndAlloc(const xiiImageHeader& header)
{
  const xiiUInt64 requiredSize = header.ComputeDataSize();

  // it is debatable whether this function should reuse external storage, at all
  // however, it is especially dangerous to rely on the external storage being big enough, since many functions just take a xiiImage as a
  // destination parameter and expect it to behave correctly when any of the Reset functions is called on it; it is not intuitive, that
  // Reset may fail due to how the image was previously reset

  // therefore, if external storage is insufficient, fall back to internal storage

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < requiredSize)
  {
    m_InternalStorage.SetCountUninitialized(requiredSize);
    m_DataPtr = m_InternalStorage.GetBlobPtr<xiiUInt8>();
  }

  xiiImageView::ResetAndViewExternalStorage(header, xiiConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void xiiImage::ResetAndUseExternalStorage(const xiiImageHeader& header, xiiByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  xiiImageView::ResetAndViewExternalStorage(header, externalData);
}

void xiiImage::ResetAndMove(xiiImage&& other)
{
  static_cast<xiiImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_InternalStorage.Clear();
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr         = other.m_DataPtr;
    other.Clear();
  }
  else
  {
    m_InternalStorage = std::move(other.m_InternalStorage);
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr         = m_InternalStorage.GetBlobPtr<xiiUInt8>();
    other.Clear();
  }
}

void xiiImage::ResetAndCopy(const xiiImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<xiiUInt8>().GetPtr(), other.GetBlobPtr<xiiUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiResult xiiImage::LoadFrom(xiiStringView sFileName)
{
  XII_LOG_BLOCK("Loading Image", sFileName);
  XII_PROFILE_SCOPE(xiiPathUtils::GetFileNameAndExtension(sFileName));

  xiiFileReader reader;
  if (reader.Open(sFileName) == XII_FAILURE)
  {
    xiiLog::Warning("Failed to open image file '{0}'", xiiArgSensitive(sFileName, "File"));
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(sFileName);

  if (const xiiImageFileFormat* pFormat = xiiImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImage(reader, *this, it) != XII_SUCCESS)
    {
      xiiLog::Warning("Failed to read image file '{0}'", xiiArgSensitive(sFileName, "File"));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Warning("No known image file format for extension '{0}'", it);

  return XII_FAILURE;
}

xiiResult xiiImage::Convert(xiiImageFormat::Enum targetFormat)
{
  return xiiImageConversion::Convert(*this, *this, targetFormat);
}

xiiImageView xiiImageView::GetSubImageView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/) const
{
  xiiImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(GetDepth(uiMipLevel));
  header.SetImageFormat(m_Format);

  const xiiUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  xiiUInt64        size   = *(&offset + GetPlaneCount()) - offset;

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return xiiImageView(header, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

xiiImage xiiImage::GetSubImageView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/)
{
  xiiImageView constView = xiiImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create a xiiImage attached to the view. Const cast is safe here since we own the storage.
  return xiiImage(constView.GetHeader(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImageView xiiImageView::GetPlaneView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  xiiImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  xiiImageFormat::Enum subFormat = xiiImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * xiiImageFormat::GetBlockWidth(subFormat) / xiiImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * xiiImageFormat::GetBlockHeight(subFormat) / xiiImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(GetDepth(uiMipLevel) * xiiImageFormat::GetBlockDepth(subFormat) / xiiImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  const xiiUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  xiiUInt64        size   = *(&offset + 1) - offset;

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return xiiImageView(header, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

xiiImage xiiImage::GetPlaneView(xiiUInt32 uiMipLevel /* = 0 */, xiiUInt32 uiFace /* = 0 */, xiiUInt32 uiArrayIndex /* = 0 */, xiiUInt32 uiPlaneIndex /* = 0 */)
{
  xiiImageView constView = xiiImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  // Create a xiiImage attached to the view. Const cast is safe here since we own the storage.
  return xiiImage(constView.GetHeader(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImage xiiImage::GetSliceView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/)
{
  xiiImageView constView = xiiImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  // Create a xiiImage attached to the view. Const cast is safe here since we own the storage.
  return xiiImage(constView.GetHeader(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImageView xiiImageView::GetSliceView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  xiiImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  xiiImageFormat::Enum subFormat = xiiImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * xiiImageFormat::GetBlockWidth(subFormat) / xiiImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * xiiImageFormat::GetBlockHeight(subFormat) / xiiImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(xiiImageFormat::GetBlockDepth(subFormat) / xiiImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  xiiUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  xiiUInt64 size   = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return xiiImageView(header, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool xiiImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<xiiUInt8>() != m_DataPtr;
}
