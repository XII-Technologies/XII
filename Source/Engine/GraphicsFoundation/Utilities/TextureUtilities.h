#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALTextureUtilities
{
public:
  /// \brief Returns true if all components of the xiiGALTextureComponentMapping are xiiGALTextureComponentSwizzle::Identity.
  static [[nodiscard]] bool IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping);

  /// \brief This returns the basic texture information for a particular format.
  ///
  /// \param format - The texture format for which to provide the information.
  ///
  /// \return A const reference to the xiiGALResourceFormatDescription structure containing the texture format description.
  ///
  /// \remarks This method must be externally synchronized.
  static [[nodiscard]] const xiiGALResourceFormatDescription& GetResourceFormatProperties(xiiEnum<xiiGALResourceFormat> format);

  /// \brief This returns the sparse texture format information for the given texture format, resource dimension and sample count.
  static [[nodiscard]] const xiiGALSparseTextureProperties GetSparseTextureProperties(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALResourceDimension> dimension, xiiUInt32 uiSampleCount);

  /// \brief This returns the mip level size of a given texture. This is typically used when retrieving the frame buffer size for a particular texture.
  static [[nodiscard]] xiiVec3U32 GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription);

  /// \brief This returns the mip size for a given mip level.
  static [[nodiscard]] xiiUInt32 GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel);

  /// \brief Returns an offset from the beginning of the buffer backing a staging texture to the specified location within the given subresource.
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
  static [[nodiscard]] xiiUInt64 GetStagingTextureLocationOffset(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevel, xiiUInt32 uiAlignment, xiiUInt32 uiLocationX, xiiUInt32 uiLocationY, xiiUInt32 uiLocationZ);

  /// \brief Returns an offset from the beginning of the buffer backing a staging texture to the given subresource.
  /// Texels within subresources are assumed to be tightly packed. There is no padding except between whole subresources.
  static [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64 GetStagingTextureSubresourceOffset(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevel, xiiUInt32 uiAlignment)
  {
    return GetStagingTextureLocationOffset(textureDescription, uiArraySlice, uiMipLevel, uiAlignment, 0, 0, 0);
  }

  /// \brief Returns the total memory size required to store the staging texture data.
  static [[nodiscard]] XII_ALWAYS_INLINE xiiUInt64 GetStagingTextureDataSize(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiAlignment = 4U)
  {
    return GetStagingTextureSubresourceOffset(textureDescription, textureDescription.GetArraySize(), 0, uiAlignment);
  }

  /// \brief This returns the default texture view type for a source format and the view type that are matched with the bind flags.
  ///
  /// \param format    - The source texture format.
  /// \param viewType  - The view type to be created.
  /// \param bindFlags - The texture bind flags.
  static [[nodiscard]] xiiEnum<xiiGALResourceFormat> GetDefaultTextureViewFormat(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALTextureViewType> viewType, xiiBitflags<xiiGALBindFlags> bindFlags);

  /// \brief Returns the default texture 1D creation description.
  static [[nodiscard]] xiiGALTextureCreationDescription GetDefaultTexture1DDescription() noexcept;

  /// \brief Returns the default texture 2D creation description.
  static [[nodiscard]] xiiGALTextureCreationDescription GetDefaultTexture2DDescription() noexcept;

  /// \brief Returns the default texture 3D creation description.
  static [[nodiscard]] xiiGALTextureCreationDescription GetDefaultTexture3DDescription() noexcept;

  /// \brief Returns the default texture cube creation description.
  static [[nodiscard]] xiiGALTextureCreationDescription GetDefaultTextureCubeDescription() noexcept;
};
