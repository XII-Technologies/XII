#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct XII_GRAPHICSCORE_DLL xiiTextureUtils
{
  static xiiGALResourceFormat::Enum ImageFormatToGalFormat(xiiImageFormat::Enum format, bool bSRGB);
  static xiiImageFormat::Enum       GalFormatToImageFormat(xiiGALResourceFormat::Enum format, bool bRemoveSRGB);
  static xiiImageFormat::Enum       GalFormatToImageFormat(xiiGALResourceFormat::Enum format);


  static void ConfigureSampler(xiiTextureFilterSetting::Enum filter, xiiGALSamplerStateCreationDescription& out_sampler);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
