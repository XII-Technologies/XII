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
