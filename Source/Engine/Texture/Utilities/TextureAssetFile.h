/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

class xiiAssetFileHeader;
class xiiImageView;
class xiiStreamWriter;
struct xiiGALSamplerCreationDescription;
struct xiiGALTextureCreationDescription;

namespace xiiTextureAssetFile
{
  /// Writes the current texture asset envelope: asset header, native texture / sampler descriptions and DDS image.
  /// A zero texture width identifies an image-backed asset to the runtime loader; DDS owns the image dimensions.
  /// Shared by asset producers so their binary payload matches native texture resource loading.
  XII_TEXTURE_DLL xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image,
                                       const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header);

  /// Writes a mutable 2D render-target definition without image data. Contents are produced by rendering.
  XII_TEXTURE_DLL xiiResult WriteRenderTarget(xiiStreamWriter& inout_stream, const xiiGALTextureCreationDescription& description,
                                              const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header);
} // namespace xiiTextureAssetFile
