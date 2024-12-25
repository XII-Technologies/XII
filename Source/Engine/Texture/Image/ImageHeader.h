#pragma once

#include <Foundation/Basics/Assert.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>

#include <Texture/Image/ImageFormat.h>
#include <Texture/TextureDLL.h>

/// \brief A class containing image meta data, such as format and dimensions.
///
/// This class has no associated behavior or functionality, and its getters and setters have no effect other than changing
/// the contained value. It is intended as a container to be modified by image utils and loaders.
class XII_TEXTURE_DLL xiiImageHeader
{
public:
  /// \brief Constructs an image using an unknown format and zero size.
  xiiImageHeader() { Clear(); }

  /// \brief Constructs an image using an unknown format and zero size.
  void Clear()
  {
    m_uiNumMipLevels    = 1;
    m_uiNumFaces        = 1;
    m_uiNumArrayIndices = 1;
    m_uiWidth           = 0;
    m_uiHeight          = 0;
    m_uiDepth           = 1;
    m_Format            = xiiImageFormat::UNKNOWN;
  }

  /// \brief Sets the image format.
  void SetImageFormat(const xiiImageFormat::Enum& format) { m_Format = format; }

  /// \brief Returns the image format.
  xiiImageFormat::Enum GetImageFormat() const { return m_Format; }

  /// \brief Sets the image width.
  void SetWidth(xiiUInt32 uiWidth) { m_uiWidth = uiWidth; }

  /// \brief Returns the image width for a given mip level, clamped to 1.
  xiiUInt32 GetWidth(xiiUInt32 uiMipLevel = 0) const
  {
    XII_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return xiiMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  /// \brief Sets the image height.
  void SetHeight(xiiUInt32 uiHeight) { m_uiHeight = uiHeight; }

  /// \brief Returns the image height for a given mip level, clamped to 1.
  xiiUInt32 GetHeight(xiiUInt32 uiMipLevel = 0) const
  {
    XII_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return xiiMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  /// \brief Sets the image depth. The default is 1.
  void SetDepth(xiiUInt32 uiDepth) { m_uiDepth = uiDepth; }

  /// \brief Returns the image depth for a given mip level, clamped to 1.
  xiiUInt32 GetDepth(xiiUInt32 uiMipLevel = 0) const
  {
    XII_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return xiiMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  /// \brief Sets the number of mip levels, including the full-size image.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumMipLevels(xiiUInt32 uiNumMipLevels) { m_uiNumMipLevels = uiNumMipLevels; }

  /// \brief Returns the number of mip levels, including the full-size image.
  xiiUInt32 GetNumMipLevels() const { return m_uiNumMipLevels; }

  /// \brief Sets the number of cubemap faces. Use 1 for a non-cubemap.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumFaces(xiiUInt32 uiNumFaces) { m_uiNumFaces = uiNumFaces; }

  /// \brief Returns the number of cubemap faces, or 1 for a non-cubemap.
  xiiUInt32 GetNumFaces() const { return m_uiNumFaces; }

  /// \brief Sets the number of array indices.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumArrayIndices(xiiUInt32 uiNumArrayIndices) { m_uiNumArrayIndices = uiNumArrayIndices; }

  /// \brief Returns the number of array indices.
  xiiUInt32 GetNumArrayIndices() const { return m_uiNumArrayIndices; }

  /// \brief Returns the number of image planes.
  xiiUInt32 GetPlaneCount() const
  {
    return xiiImageFormat::GetPlaneCount(m_Format);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  xiiUInt32 GetNumBlocksX(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const
  {
    return xiiImageFormat::GetNumBlocksX(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  xiiUInt32 GetNumBlocksY(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const
  {
    return xiiImageFormat::GetNumBlocksY(m_Format, GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the depth direction.
  xiiUInt32 GetNumBlocksZ(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const
  {
    return xiiImageFormat::GetNumBlocksZ(m_Format, GetDepth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
  xiiUInt64 GetRowPitch(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const
  {
    return xiiImageFormat::GetRowPitch(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
  xiiUInt64 GetDepthPitch(xiiUInt32 uiMipLevel = 0, xiiUInt32 uiPlaneIndex = 0) const
  {
    return xiiImageFormat::GetDepthPitch(m_Format, GetWidth(uiMipLevel), GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Computes the data size required for an image with the header's format and dimensions.
  xiiUInt64 ComputeDataSize() const
  {
    xiiUInt64 uiDataSize = 0;

    for (xiiUInt32 uiMipLevel = 0; uiMipLevel < GetNumMipLevels(); uiMipLevel++)
    {
      for (xiiUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
      {
        uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * static_cast<xiiUInt64>(GetDepth(uiMipLevel));
      }
    }

    return xiiMath::SafeMultiply64(uiDataSize, xiiMath::SafeMultiply32(GetNumArrayIndices(), GetNumFaces()));
  }

  /// \brief Computes the number of mip maps in the full mip chain.
  xiiUInt32 ComputeNumberOfMipMaps() const
  {
    xiiUInt32 numMipMaps = 1;
    xiiUInt32 width      = GetWidth();
    xiiUInt32 height     = GetHeight();
    xiiUInt32 depth      = GetDepth();

    while (width > 1 || height > 1 || depth > 1)
    {
      width  = xiiMath::Max(1u, width / 2);
      height = xiiMath::Max(1u, height / 2);
      depth  = xiiMath::Max(1u, depth / 2);

      numMipMaps++;
    }

    return numMipMaps;
  }

  bool operator==(const xiiImageHeader& other) const
  {
    return m_uiNumMipLevels == other.m_uiNumMipLevels &&
      m_uiNumFaces == other.m_uiNumFaces &&
      m_uiNumArrayIndices == other.m_uiNumArrayIndices &&
      m_uiWidth == other.m_uiWidth &&
      m_uiHeight == other.m_uiHeight &&
      m_uiDepth == other.m_uiDepth &&
      m_Format == other.m_Format;
  }

protected:
  xiiUInt32 m_uiNumMipLevels;
  xiiUInt32 m_uiNumFaces;
  xiiUInt32 m_uiNumArrayIndices;

  xiiUInt32 m_uiWidth;
  xiiUInt32 m_uiHeight;
  xiiUInt32 m_uiDepth;

  xiiImageFormat::Enum m_Format;
};
