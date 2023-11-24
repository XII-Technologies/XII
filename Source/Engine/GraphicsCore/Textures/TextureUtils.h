#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct XII_GRAPHICSCORE_DLL xiiTextureUtils
{
  static xiiEnum<xiiGALTextureFormat> ImageFormatToGalFormat(xiiEnum<xiiImageFormat> format, bool bSRGB);
  static xiiEnum<xiiImageFormat>      GalFormatToImageFormat(xiiEnum<xiiGALTextureFormat> format, bool bRemoveSRGB);
  static xiiEnum<xiiImageFormat>      GalFormatToImageFormat(xiiEnum<xiiGALTextureFormat> format);

  static void ConfigureSampler(xiiEnum<xiiTextureFilterSetting> filter, xiiGALSamplerCreationDescription& out_sampler);

  static xiiEnum<xiiGALTextureAddressMode> GALTextureAddressMode(xiiEnum<xiiImageAddressMode> mode);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
