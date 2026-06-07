/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Converter/TextureConverterProcessor.h>
#include <Texture/Image/ImageUtils.h>

xiiResult xiiTextureConverterProcessor::Assemble3DTexture(xiiImage& dst) const
{
  XII_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return xiiImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}
