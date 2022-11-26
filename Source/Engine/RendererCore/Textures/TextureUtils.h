#pragma once

#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct XII_RENDERERCORE_DLL xiiTextureUtils
{
  static xiiGALResourceFormat::Enum ImageFormatToGalFormat(xiiImageFormat::Enum format, bool bSRGB);
  static xiiImageFormat::Enum       GalFormatToImageFormat(xiiGALResourceFormat::Enum format, bool bRemoveSRGB);
  static xiiImageFormat::Enum       GalFormatToImageFormat(xiiGALResourceFormat::Enum format);


  static void ConfigureSampler(xiiTextureFilterSetting::Enum filter, xiiGALSamplerStateCreationDescription& out_Sampler);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
