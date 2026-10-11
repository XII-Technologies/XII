/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

class xiiAssetFileHeader;
class xiiImageView;
class xiiStreamWriter;
struct xiiGALSamplerCreationDescription;

namespace xiiTextureAssetFile
{
  /// Writes the current texture asset envelope: asset header, native texture / sampler descriptions and DDS image.
  /// A zero texture width identifies an image-backed asset to the runtime loader; DDS owns the image dimensions.
  /// Shared by asset producers so their binary payload matches native texture resource loading.
  XII_TEXTURE_DLL xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image,
                                       const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header);
} // namespace xiiTextureAssetFile
