/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/Resources/Texture.h>

/// This describes the mip level properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMipLevelProperties : public xiiHashableStruct<xiiGALMipLevelProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiSizeU32 m_LogicalSize      = xiiSizeU32(0, 0); ///< The logical mip width and height.
  xiiSizeU32 m_StorageSize      = xiiSizeU32(0, 0); ///< The storage mip width and height. For compressed formats, storage width and height are rounded up to the block size. For example, for a texture mip with logical width or height 10 and BC1 format (with 4x4 pixel block size), the storage width or height will be 12.
  xiiUInt32  m_uiDepth          = 0;                ///< The mip level depth. Note that logical and storage depths are always equivalent.
  xiiUInt64  m_uiRowSize        = 0;                ///< The row size in bytes. For compressed formats, row size defines the size of one row of compressed blocks.
  xiiUInt64  m_uiDepthSliceSize = 0;                ///< The depth slice size in bytes.
  xiiUInt64  m_uiMipSize        = 0;                ///< The total mip level data size in bytes.
};

/// This describes the information required to perform a copy operation between a buffer and a texture.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferToTextureCopyDescription : public xiiHashableStruct<xiiGALBufferToTextureCopyDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64         m_uiRowSize           = 0;                             ///< Texture region row size, in bytes. For compressed formats, this is the size of one row of compressed blocks.
  xiiUInt64         m_uiRowStride         = 0;                             ///< Row stride, in bytes. The stride is computed by aligning the RowSize, and is thus always >= RowSize.
  xiiUInt32         m_uiRowStrideInTexels = 0;                             ///< Row stride in texels.
  xiiUInt32         m_uiRowCount          = 0;                             ///< The number of rows in the region. For compressed formats, this is the number of compressed-block rows.
  xiiUInt32         m_uiDepthStride       = 0;                             ///< Depth stride (RowStride * RowCount).
  xiiUInt32         m_uiMemorySize        = 0;                             ///< Total memory size required to store the pixels in the region.
  xiiBoundingBoxU32 m_Region              = xiiBoundingBoxU32::MakeZero(); ///< Texture region.
};

class XII_GRAPHICSFOUNDATION_DLL xiiGALTextureUtilities
{
public:
  /// Returns true if all components of the xiiGALTextureComponentMapping are xiiGALTextureComponentSwizzle::Identity.
  [[nodiscard]] static bool IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping);

  /// This returns the basic texture information for a particular format.
  ///
  /// \param format - The texture format for which to provide the information.
  ///
  /// \return A const reference to the xiiGALResourceFormatDescription structure containing the texture format description.
  [[nodiscard]] static const xiiGALResourceFormatDescription& GetResourceFormatProperties(xiiEnum<xiiGALResourceFormat> format);

  /// This returns the multi-planar format information for a particular format.
  ///
  /// \param format - The texture format for which to provide the information.
  ///
  /// \return A const reference to the xiiGALMultiPlanarFormatDescription structure containing the multi-planar format description.
  [[nodiscard]] static const xiiGALMultiPlanarFormatDescription& GetMultiPlanarFormatProperties(xiiEnum<xiiGALResourceFormat> format);

  /// Returns true if the given format is a multi-planar format.
  [[nodiscard]] static XII_ALWAYS_INLINE bool IsCompressed(xiiEnum<xiiGALResourceFormat> format)
  {
    return !xiiGALResourceFormat::IsMultiplanar(format) && GetResourceFormatProperties(format).IsCompressed();
  }

  /// Returns the number of components for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetComponentCount(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return GetMultiPlanarFormatProperties(format).GetPlaneCount();

    return GetResourceFormatProperties(format).m_uiComponentCount;
  }

  /// Returns the bits per component for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBitsPerComponent(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return GetMultiPlanarFormatProperties(format).GetPlane(uiPlaneIndex).m_uiBytesPerElement * 8U;

    return GetResourceFormatProperties(format).m_uiComponentSize * 8U;
  }

  /// Returns the bytes per block for a given texture format. For most formats, this is equal to the bytes per pixel. For compressed formats, this is the total number of bytes in a compression block.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBytesPerBlock(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return GetMultiPlanarFormatProperties(format).GetPlane(uiPlaneIndex).m_uiBytesPerElement;

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");

    return GetResourceFormatProperties(format).GetElementSize();
  }

  /// Returns the bits per block for a given texture format. For most formats, this is equal to the bits per pixel. For compressed formats, this is the total number of bits in a compression block.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBitsPerBlock(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    return GetBytesPerBlock(format, uiPlaneIndex) * 8U;
  }

  /// Returns the bits per pixel for a given texture format. For compressed formats, this is the average bits per pixel across a block.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBitsPerPixel(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return GetMultiPlanarFormatProperties(format).GetPlane(0).m_uiBytesPerElement * 8U;

    const xiiGALResourceFormatDescription& properties = GetResourceFormatProperties(format);

    return properties.IsCompressed() ? (properties.GetElementSize() * 8U) / properties.GetTexelsPerBlock() : properties.GetElementSize() * 8U;
  }

  /// Returns the exact bits per pixel for a given texture format. For compressed formats, this may be a fractional value.
  [[nodiscard]] static XII_ALWAYS_INLINE float GetExactBitsPerPixel(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return static_cast<float>(GetBitsPerPixel(format));

    const xiiGALResourceFormatDescription& properties = GetResourceFormatProperties(format);

    return properties.IsCompressed() ? static_cast<float>(properties.GetElementSize() * 8U) / static_cast<float>(properties.GetTexelsPerBlock()) : static_cast<float>(properties.GetElementSize() * 8U);
  }

  /// Returns the block width for a given texture format. For most formats, this is 1. For compressed formats, this may be greater than 1.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBlockWidth(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    XII_IGNORE_UNUSED(uiPlaneIndex);

    return xiiGALResourceFormat::IsMultiplanar(format) ? 1U : GetResourceFormatProperties(format).GetBlockWidth();
  }

  /// Returns the block height for a given texture format. For most formats, this is 1. For compressed formats, this may be greater than 1.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBlockHeight(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    XII_IGNORE_UNUSED(uiPlaneIndex);

    return xiiGALResourceFormat::IsMultiplanar(format) ? 1U : GetResourceFormatProperties(format).GetBlockHeight();
  }

  /// Returns the block depth for a given texture format. For most formats, this is 1. For 3D textures, this may be greater than 1.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBlockDepth(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex = 0)
  {
    XII_IGNORE_UNUSED(format);
    XII_IGNORE_UNUSED(uiPlaneIndex);

    return 1U;
  }

  /// Returns true if the format requires first-level block alignment (i.e., if the block width or height is greater than 1).
  [[nodiscard]] static XII_ALWAYS_INLINE bool RequiresFirstLevelBlockAlignment(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return false;

    const xiiGALResourceFormatDescription& properties = GetResourceFormatProperties(format);

    return properties.GetBlockWidth() > 1U || properties.GetBlockHeight() > 1U;
  }

  /// Returns the sub-format for a given multi-planar texture format and plane index.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> GetPlaneSubFormat(xiiEnum<xiiGALResourceFormat> format, xiiUInt32 uiPlaneIndex)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return GetMultiPlanarFormatProperties(format).GetPlane(uiPlaneIndex).m_SubFormat;

    XII_ASSERT_DEV(uiPlaneIndex == 0, "Single-plane formats only have plane 0.");

    return format;
  }

  /// Returns the component type for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormatComponentType> GetComponentType(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return xiiGALResourceFormatComponentType::UnsignedInteger;

    return GetResourceFormatProperties(format).m_ComponentType;
  }

  /// This returns the sparse texture format information for the given texture format, resource dimension and sample count.
  [[nodiscard]] static const xiiGALSparseTextureProperties GetSparseTextureProperties(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALResourceDimension> dimension, xiiUInt32 uiSampleCount);

  /// This returns the mip level size of a given texture. This is typically used when retrieving the frame buffer size for a particular texture.
  [[nodiscard]] static xiiVec3U32 GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription);

  /// This returns the mip size for a given mip level.
  [[nodiscard]] static xiiUInt32 GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel);

  /// This returns the mip level properties for a given texture and mip level.
  [[nodiscard]] static xiiGALMipLevelProperties GetMipLevelProperties(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel);

  /// This returns the number of mip levels for a given texture.
  [[nodiscard]] static xiiUInt32 GetMipLevelCount(const xiiGALTextureCreationDescription& textureDescription);

  /// Returns an offset from the beginning of the buffer backing a staging texture to the specified location within the given subresource.
  ///
  /// \param textureDescription - Staging texture description.
  /// \param uiArraySlice       - Array slice.
  /// \param uiMipLevel         - Mip level.
  /// \param uiAlignment        - Subresource alignment. The alignment is applied to whole subresources only, but not to the row/depth strides.
  ///                             In other words, there may be padding between subresources, but texels in every subresource are assumed to be tightly packed.
  /// \param uiLocationX        - X location within the subresource.
  /// \param uiLocationY        - Y location within the subresource.
  /// \param uiLocationZ        - Z location within the subresource.
  ///
  /// \return Offset from the beginning of the buffer to the given location.
  ///
  /// \remarks Alignment is applied to the subresource sizes, such that the beginning of data of every subresource starts at an offset aligned by 'Alignment'.
  ///          The alignment is not applied to the row/depth strides and texels in all subresources are assumed to be tightly packed.
  ///
  ///             Subres 0
  ///              stride
  ///       |<-------------->|
  ///       |________________|       Subres 1
  ///       |                |        stride
  ///       |                |     |<------->|
  ///       |                |     |_________|
  ///       |    Subres 0    |     |         |
  ///       |                |     | Subres 1|
  ///       |                |     |         |                     _
  ///       |________________|     |_________|         ...        |_|
  ///       A                      A                              A
  ///       |                      |                              |
  ///     Buffer start            Subres 1 offset,               Subres N offset,
  ///                          aligned by 'Alignment'         aligned by 'Alignment'
  ///
  [[nodiscard]] static xiiUInt64 GetStagingTextureLocationOffset(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevel, xiiUInt32 uiAlignment, xiiUInt32 uiLocationX, xiiUInt32 uiLocationY, xiiUInt32 uiLocationZ);

  /// Returns an offset from the beginning of the buffer backing a staging texture to the given subresource.
  /// Texels within subresources are assumed to be tightly packed. There is no padding except between whole subresources.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt64 GetStagingTextureSubresourceOffset(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevel, xiiUInt32 uiAlignment)
  {
    return GetStagingTextureLocationOffset(textureDescription, uiArraySlice, uiMipLevel, uiAlignment, 0, 0, 0);
  }

  /// Computes the information required to perform a copy operation between a buffer and a texture.
  static xiiGALBufferToTextureCopyDescription GetBufferToTextureCopyDescription(xiiGALResourceFormat::Enum format, const xiiBoundingBoxU32& region, xiiUInt32 uiRowStrideAlignment);

  /// Copies texture subresource data on the CPU.
  ///
  /// \param sourceSubresource        - Source subresource data.
  /// \param uiRowCount               - The number of rows in the subresource.
  /// \param uiDepthSliceCount        - The number of depth slices in the subresource.
  /// \param uiRowSize                - Subresource data row size, in bytes.
  /// \param pDestinationData         - Pointer to the destination subresource data.
  /// \param uiDestinationRowStride   - Destination subresource row stride, in bytes.
  /// \param uiDestinationDepthStride - Destination subresource depth stride, in bytes.
  static void CopyTextureSubresource(const xiiGALTextureSubResourceData& sourceSubresource, xiiUInt32 uiRowCount, xiiUInt32 uiDepthSliceCount, xiiUInt64 uiRowSize, void* pDestinationData, xiiUInt64 uiDestinationRowStride, xiiUInt64 uiDestinationDepthStride);

  /// Returns the total memory size required to store the staging texture data.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt64 GetStagingTextureDataSize(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiAlignment = 4U)
  {
    return GetStagingTextureSubresourceOffset(textureDescription, textureDescription.m_uiArraySizeOrDepth, 0, uiAlignment);
  }

  /// This returns the default texture view type for a source format and the view type that are matched with the bind flags.
  ///
  /// \param format    - The source texture format.
  /// \param viewType  - The view type to be created.
  /// \param bindFlags - The texture bind flags.
  [[nodiscard]] static xiiEnum<xiiGALResourceFormat> GetDefaultTextureViewFormat(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALTextureViewType> viewType, xiiBitflags<xiiGALBindFlags> bindFlags);

  /// Returns the default texture 1D creation description.
  [[nodiscard]] static xiiGALTextureCreationDescription GetDefaultTexture1DDescription() noexcept;

  /// Returns the default texture 2D creation description.
  [[nodiscard]] static xiiGALTextureCreationDescription GetDefaultTexture2DDescription() noexcept;

  /// Returns the default texture 3D creation description.
  [[nodiscard]] static xiiGALTextureCreationDescription GetDefaultTexture3DDescription() noexcept;

  /// Returns the default texture cube creation description.
  [[nodiscard]] static xiiGALTextureCreationDescription GetDefaultTextureCubeDescription() noexcept;

  /// Returns the total number of subresources for the given texture description.
  XII_ALWAYS_INLINE static xiiUInt32 GetSubResourceCount(const xiiGALTextureCreationDescription& description) noexcept { return description.m_uiMipLevels * description.m_uiArraySizeOrDepth; }

  /// Returns the required row pitch for the given texture description and mip level.
  XII_ALWAYS_INLINE static xiiUInt32 GetRequiredRowPitch(const xiiGALTextureCreationDescription& description, xiiUInt32 uiMipLevel)
  {
    const xiiGALMipLevelProperties mipLevelProperties = GetMipLevelProperties(description, uiMipLevel);
    return static_cast<xiiUInt32>(mipLevelProperties.m_uiRowSize);
  }

  /// Returns the required slice pitch for the given texture description and mip level.
  XII_ALWAYS_INLINE static xiiUInt32 GetRequiredSlicePitch(const xiiGALTextureCreationDescription& description, xiiUInt32 uiMipLevel)
  {
    const xiiGALMipLevelProperties mipLevelProperties = GetMipLevelProperties(description, uiMipLevel);
    return static_cast<xiiUInt32>(mipLevelProperties.m_uiDepthSliceSize);
  }

  /// Fills the out_subresourceData array with zero-initialized data pointers for all subresources of the given texture description.
  [[nodiscard]] static xiiGALTextureData GetZeroMemoryInitialData(const xiiGALTextureCreationDescription description, xiiHybridArray<xiiGALTextureSubResourceData, 2U>& out_subresourceData, xiiDynamicArray<xiiUInt8>& out_Data);

  static void CopySubresourceToMemory(const xiiGALTextureCreationDescription& description, const xiiGALMappedTextureSubresource& subresourceData, const xiiGALTextureMipLevelData& mipLevelData, xiiArrayPtr<xiiUInt8> pTargetData, xiiUInt32 uiTargetRowStride);
};
