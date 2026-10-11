/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>

xiiResult xiiTextureAssetFile::WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image,
                                          const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header)
{
  if (!image.IsValid())
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(header.Write(inout_stream));
  auto description   = image.GetDescription();
  description.m_Size = {0, 0};
  inout_stream << description;
  inout_stream << sampler;
  xiiDdsFileFormat dds;
  return dds.WriteImage(inout_stream, image, "dds");
}
