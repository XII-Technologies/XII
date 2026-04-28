/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Texture/Image/Image.h>

struct XII_GRAPHICSCORE_DLL xiiTextureUtils
{
  static xiiEnum<xiiGALResourceFormat> ImageFormatToGalFormat(xiiEnum<xiiImageFormat> format, bool bSRGB);
  static xiiEnum<xiiImageFormat>       GalFormatToImageFormat(xiiEnum<xiiGALResourceFormat> format, bool bRemoveSRGB);
  static xiiEnum<xiiImageFormat>       GalFormatToImageFormat(xiiEnum<xiiGALResourceFormat> format);

  static void ConfigureSampler(xiiEnum<xiiTextureFilterSetting> filter, xiiGALSamplerCreationDescription& out_sampler);

  static xiiEnum<xiiGALTextureAddressMode> GALTextureAddressMode(xiiEnum<xiiImageAddressMode> mode);

  /// \brief Sets the texture filter mode that is used by default for texture resources.
  ///
  /// The built in default is Anisotropic 4x.
  /// If the default setting is changed, already loaded textures might not adjust.
  /// Nearest filtering is not allowed as a default filter.
  static void SetDefaultTextureFilter(xiiEnum<xiiTextureFilterSetting> filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  static XII_ALWAYS_INLINE xiiEnum<xiiTextureFilterSetting> GetDefaultTextureFilter() { return s_DefaultTextureFilter; }

  /// \brief Returns the texture filter mode that is used for a specific texture.
  static xiiEnum<xiiTextureFilterSetting> GetSpecificTextureFilter(xiiEnum<xiiTextureFilterSetting> filter);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;

private:
  static xiiEnum<xiiTextureFilterSetting> s_DefaultTextureFilter; ///< The default texture filter mode.
};
