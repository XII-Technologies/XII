#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsUtilities
{
public:
  /// \brief Returns true if all components of the xiiGALTextureComponentMapping are xiiGALTextureComponentSwizzle::Identity.
  static XII_NODISCARD bool IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping);

  /// \brief This returns the basic texture information for a particular format.
  ///
  /// \param format - The texture format for which to provide the information.
  ///
  /// \return A const reference to the xiiGALTextureFormatDescription structure containing the texture format description.
  ///
  /// \remarks This method must be externally synchronized.
  static XII_NODISCARD const xiiGALTextureFormatDescription& GetTextureFormatProperties(xiiEnum<xiiGALTextureFormat> format);

  /// \brief This returns the sparse texture format information for the given texture format, resource dimension and sample count.
  static XII_NODISCARD const xiiGALSparseTextureProperties GetSparseTextureProperties(xiiEnum<xiiGALTextureFormat> format, xiiEnum<xiiGALResourceDimension> dimension, xiiUInt32 uiSampleCount);

  /// \brief This returns the valid pipeline resource flags for a given shader resource type.
  static XII_NODISCARD xiiBitflags<xiiGALPipelineResourceFlags> GetValidPipelineResourceFlags(xiiEnum<xiiGALShaderResourceType> type);
};

#include <GraphicsFoundation/Utilities/Implementation/GraphicsUtilities_inl.h>
