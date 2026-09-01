/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

namespace
{
  xiiGALTextureCreationDescription MakeImageDescription(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiEnum<xiiGALResourceFormat> format)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = uiDepth > 1 ? xiiGALResourceDimension::Texture3D : xiiGALResourceDimension::Texture2D;
    description.m_Size               = xiiSizeU32(uiWidth, uiHeight);
    description.m_uiArraySizeOrDepth = xiiMath::Max(1U, uiDepth);
    description.m_Format             = format;
    description.m_uiMipLevels        = 1U;

    return description;
  }

  xiiEnum<xiiGALResourceFormat> GetPlaneFormat(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
    {
      return xiiGALTextureUtilities::GetMultiPlanarFormatProperties(format).GetPlane(uiPlaneIndex).m_SubFormat;
    }

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");
    return format;
  }

  xiiUInt32 GetPlaneWidth(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiWidth, xiiUInt32 uiPlaneIndex)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
    {
      return xiiGALTextureUtilities::GetMultiPlanarFormatProperties(format).GetPlaneWidth(uiWidth, uiPlaneIndex);
    }

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");
    return uiWidth;
  }

  xiiUInt32 GetPlaneHeight(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiHeight, xiiUInt32 uiPlaneIndex)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
    {
      return xiiGALTextureUtilities::GetMultiPlanarFormatProperties(format).GetPlaneHeight(uiHeight, uiPlaneIndex);
    }

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");
    return uiHeight;
  }

  xiiUInt32 GetBytesPerBlock(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
    {
      return xiiGALTextureUtilities::GetMultiPlanarFormatProperties(format).GetPlane(uiPlaneIndex).m_uiBytesPerElement;
    }

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");
    return xiiGALTextureUtilities::GetResourceFormatProperties(format).GetElementSize();
  }
} // namespace

xiiImageView::xiiImageView()
{
  Clear();
}

xiiImageView::xiiImageView(const xiiGALTextureCreationDescription& description, xiiConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(description, imageData);
}

void xiiImageView::Clear()
{
  m_Description = {};
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool xiiImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void xiiImageView::ResetAndViewExternalStorage(const xiiGALTextureCreationDescription& description, xiiConstByteBlobPtr imageData)
{
  m_Description = description;

  xiiUInt64 uiDataSize = ComputeLayout();

  XII_IGNORE_UNUSED(uiDataSize);
  XII_ASSERT_DEV(imageData.GetCount() == uiDataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)", imageData.GetCount(), uiDataSize);

  // Const cast is safe here as we will only perform non-const access if this is a xiiImage which owns mutable access to the storage.
  m_DataPtr = xiiBlobPtr<xiiUInt8>(const_cast<xiiUInt8*>(static_cast<const xiiUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

xiiResult xiiImageView::SaveTo(xiiStringView sFileName) const
{
  XII_LOG_BLOCK("Writing Image", sFileName);

  if (m_Description.m_Format == xiiGALResourceFormat::Unknown)
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

const xiiGALTextureCreationDescription& xiiImageView::GetDescription() const
{
  return m_Description;
}

xiiEnum<xiiGALResourceFormat> xiiImageView::GetImageFormat() const
{
  return m_Description.m_Format;
}

xiiUInt32 xiiImageView::GetWidth(xiiUInt32 uiMipLevel /*= 0*/) const
{
  XII_ASSERT_DEV(uiMipLevel < m_Description.m_uiMipLevels, "Invalid mip level {} for image with {} mip levels.", uiMipLevel, m_Description.m_uiMipLevels);

  return xiiMath::Max(m_Description.m_Size.width >> uiMipLevel, 1U);
}

xiiUInt32 xiiImageView::GetHeight(xiiUInt32 uiMipLevel /*= 0*/) const
{
  XII_ASSERT_DEV(uiMipLevel < m_Description.m_uiMipLevels, "Invalid mip level {} for image with {} mip levels.", uiMipLevel, m_Description.m_uiMipLevels);

  return xiiMath::Max(m_Description.m_Size.height >> uiMipLevel, 1U);
}

xiiUInt32 xiiImageView::GetDepth(xiiUInt32 uiMipLevel /*= 0*/) const
{
  XII_ASSERT_DEV(uiMipLevel < m_Description.m_uiMipLevels, "Invalid mip level {} for image with {} mip levels.", uiMipLevel, m_Description.m_uiMipLevels);

  return xiiMath::Max(m_Description.m_uiArraySizeOrDepth >> uiMipLevel, 1U);
}

xiiUInt32 xiiImageView::GetMipLevelCount() const
{
  return m_Description.m_uiMipLevels;
}

xiiUInt32 xiiImageView::GetNumFaces() const
{
  return m_Description.IsCube() ? 6U : 1U;
}

xiiUInt32 xiiImageView::GetNumArrayIndices() const
{
  return m_Description.IsCube() ? m_Description.m_uiArraySizeOrDepth / 6U : m_Description.m_uiArraySizeOrDepth;
}

xiiUInt32 xiiImageView::GetPlaneCount() const
{
  if (xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format))
  {
    return xiiGALTextureUtilities::GetMultiPlanarFormatProperties(m_Description.m_Format).GetPlaneCount();
  }

  return 1U;
}

xiiUInt32 xiiImageView::GetNumBlocksX(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  const xiiUInt32 uiPlaneWidth = GetPlaneWidth(m_Description.m_Format, GetWidth(uiMipLevel), uiPlaneIndex);

  if (xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format))
  {
    return uiPlaneWidth;
  }

  return xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format).GetBlockCountX(uiPlaneWidth);
}

xiiUInt32 xiiImageView::GetNumBlocksY(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  const xiiUInt32 uiPlaneHeight = GetPlaneHeight(m_Description.m_Format, GetHeight(uiMipLevel), uiPlaneIndex);

  if (xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format))
  {
    return uiPlaneHeight;
  }

  return xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format).GetBlockCountY(uiPlaneHeight);
}

xiiUInt32 xiiImageView::GetNumBlocksZ(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  XII_IGNORE_UNUSED(uiPlaneIndex);
  return GetDepth(uiMipLevel);
}

xiiUInt64 xiiImageView::GetRowPitch(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  return static_cast<xiiUInt64>(GetNumBlocksX(uiMipLevel, uiPlaneIndex)) * GetBytesPerBlock(m_Description.m_Format, uiPlaneIndex);
}

xiiUInt64 xiiImageView::GetDepthPitch(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  return GetRowPitch(uiMipLevel, uiPlaneIndex) * GetNumBlocksY(uiMipLevel, uiPlaneIndex);
}

xiiUInt64 xiiImageView::ComputeDataSize() const
{
  xiiUInt64 uiDataSize = 0;

  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < GetNumArrayIndices(); uiArrayIndex++)
  {
    for (xiiUInt32 uiFace = 0; uiFace < GetNumFaces(); uiFace++)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < GetMipLevelCount(); uiMipLevel++)
      {
        for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  return uiDataSize;
}

xiiImageView xiiImageView::GetRowView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 y /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  const xiiEnum<xiiGALResourceFormat> planeFormat = GetPlaneFormat(m_Description.m_Format, uiPlaneIndex);
  xiiGALTextureCreationDescription    description = MakeImageDescription(GetNumBlocksX(uiMipLevel, uiPlaneIndex), 1, 1, planeFormat);

  xiiUInt64 offset = 0;
  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  xiiBlobPtr<const xiiUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return xiiImageView(description, xiiConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void xiiImageView::ReinterpretAs(xiiGALResourceFormat::Enum format)
{
  const bool bSourceCompressed = xiiGALResourceFormat::IsMultiplanar(m_Description.m_Format) ? false : xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format).IsCompressed();
  const bool bTargetCompressed = xiiGALResourceFormat::IsMultiplanar(format) ? false : xiiGALTextureUtilities::GetResourceFormatProperties(format).IsCompressed();

  XII_ASSERT_DEBUG(bTargetCompressed == bSourceCompressed, "Cannot reinterpret compressed and non-compressed formats");

  xiiGALTextureCreationDescription newDescription = m_Description;
  newDescription.m_Format                         = format;

  xiiImageView validationView;
  validationView.m_Description = newDescription;
  XII_ASSERT_DEBUG(validationView.ComputeDataSize() == ComputeDataSize(), "Cannot reinterpret between formats of different sizes");

  m_Description.m_Format = format;
}

xiiUInt64 xiiImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(GetMipLevelCount() * GetNumFaces() * GetNumArrayIndices() * GetPlaneCount());

  xiiUInt64 uiDataSize = 0;

  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < GetNumArrayIndices(); uiArrayIndex++)
  {
    for (xiiUInt32 uiFace = 0; uiFace < GetNumFaces(); uiFace++)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < GetMipLevelCount(); uiMipLevel++)
      {
        for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void xiiImageView::ValidateSubImageIndices(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const
{
  XII_IGNORE_UNUSED(uiMipLevel);
  XII_IGNORE_UNUSED(uiFace);
  XII_IGNORE_UNUSED(uiArrayIndex);
  XII_IGNORE_UNUSED(uiPlaneIndex);

  XII_ASSERT_DEV(uiMipLevel < GetMipLevelCount(), "Invalid mip level");
  XII_ASSERT_DEV(uiFace < GetNumFaces(), "Invalid uiFace");
  XII_ASSERT_DEV(uiArrayIndex < GetNumArrayIndices(), "Invalid array slice");
  XII_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const xiiUInt64& xiiImageView::GetSubImageOffset(xiiUInt32 uiMipLevel, xiiUInt32 uiFace, xiiUInt32 uiArrayIndex, xiiUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + GetMipLevelCount() * (uiFace + GetNumFaces() * uiArrayIndex))];
}

xiiImage::xiiImage()
{
  Clear();
}

xiiImage::xiiImage(const xiiGALTextureCreationDescription& description)
{
  ResetAndAlloc(description);
}

xiiImage::xiiImage(const xiiGALTextureCreationDescription& description, xiiByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(description, externalData);
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

void xiiImage::ResetAndAlloc(const xiiGALTextureCreationDescription& description)
{
  m_Description                  = description;
  const xiiUInt64 uiRequiredSize = ComputeDataSize();

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < uiRequiredSize)
  {
    m_InternalStorage.SetCountUninitialized(uiRequiredSize);

    m_DataPtr = m_InternalStorage.GetBlobPtr<xiiUInt8>();
  }

  xiiImageView::ResetAndViewExternalStorage(description, xiiConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void xiiImage::ResetAndUseExternalStorage(const xiiGALTextureCreationDescription& description, xiiByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  xiiImageView::ResetAndViewExternalStorage(description, externalData);
}

void xiiImage::ResetAndMove(xiiImage&& other)
{
  m_Description = other.GetDescription();

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
  ResetAndAlloc(other.GetDescription());

  memcpy(GetBlobPtr<xiiUInt8>().GetPtr(), other.GetBlobPtr<xiiUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiResult xiiImage::LoadFrom(xiiStringView sFileName)
{
  XII_LOG_BLOCK("Loading Image", sFileName);
  XII_PROFILE_SCOPE(xiiPathUtils::GetFileNameAndExtension(sFileName));

  xiiFileReader reader;
  if (reader.Open(sFileName).Failed())
  {
    xiiLog::Warning("Failed to open image file '{0}'", xiiArgSensitive(sFileName, "File"));
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(sFileName);

  if (const xiiImageFileFormat* pFormat = xiiImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImage(reader, *this, it).Failed())
    {
      xiiLog::Warning("Failed to read image file '{0}'", xiiArgSensitive(sFileName, "File"));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Warning("No known image file format for extension '{0}'", it);

  return XII_FAILURE;
}

xiiResult xiiImage::Convert(xiiGALResourceFormat::Enum targetFormat)
{
  return xiiImageConversion::Convert(*this, *this, targetFormat);
}

xiiImageView xiiImageView::GetSubImageView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/) const
{
  xiiGALTextureCreationDescription description = MakeImageDescription(GetWidth(uiMipLevel), GetHeight(uiMipLevel), GetDepth(uiMipLevel), m_Description.m_Format);

  const xiiUInt64& uiOffset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  xiiUInt64        uiSize   = *(&uiOffset + GetPlaneCount()) - uiOffset;

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(uiOffset, uiSize);

  return xiiImageView(description, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

xiiImage xiiImage::GetSubImageView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/)
{
  xiiImageView constView = xiiImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  return xiiImage(constView.GetDescription(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImageView xiiImageView::GetPlaneView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  const xiiEnum<xiiGALResourceFormat> planeFormat = GetPlaneFormat(m_Description.m_Format, uiPlaneIndex);
  xiiGALTextureCreationDescription    description = MakeImageDescription(GetPlaneWidth(m_Description.m_Format, GetWidth(uiMipLevel), uiPlaneIndex), GetPlaneHeight(m_Description.m_Format, GetHeight(uiMipLevel), uiPlaneIndex), GetDepth(uiMipLevel), planeFormat);

  const xiiUInt64& uiOffset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  xiiUInt64        uiSize   = *(&uiOffset + 1) - uiOffset;

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(uiOffset, uiSize);

  return xiiImageView(description, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

xiiImage xiiImage::GetPlaneView(xiiUInt32 uiMipLevel /* = 0 */, xiiUInt32 uiFace /* = 0 */, xiiUInt32 uiArrayIndex /* = 0 */, xiiUInt32 uiPlaneIndex /* = 0 */)
{
  xiiImageView constView = xiiImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  return xiiImage(constView.GetDescription(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImage xiiImage::GetSliceView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/)
{
  xiiImageView constView = xiiImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  return xiiImage(constView.GetDescription(), xiiByteBlobPtr(const_cast<xiiUInt8*>(constView.GetBlobPtr<xiiUInt8>().GetPtr()), constView.GetBlobPtr<xiiUInt8>().GetCount()));
}

xiiImageView xiiImageView::GetSliceView(xiiUInt32 uiMipLevel /*= 0*/, xiiUInt32 uiFace /*= 0*/, xiiUInt32 uiArrayIndex /*= 0*/, xiiUInt32 z /*= 0*/, xiiUInt32 uiPlaneIndex /*= 0*/) const
{
  const xiiEnum<xiiGALResourceFormat> planeFormat = GetPlaneFormat(m_Description.m_Format, uiPlaneIndex);
  xiiGALTextureCreationDescription    description = MakeImageDescription(GetPlaneWidth(m_Description.m_Format, GetWidth(uiMipLevel), uiPlaneIndex), GetPlaneHeight(m_Description.m_Format, GetHeight(uiMipLevel), uiPlaneIndex), 1, planeFormat);

  const xiiUInt64& uiOffset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  xiiUInt64        uiSize   = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  xiiBlobPtr<const xiiUInt8> subView = m_DataPtr.GetSubArray(uiOffset + z * uiSize, uiSize);

  return xiiImageView(description, xiiConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool xiiImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<xiiUInt8>() != m_DataPtr;
}
