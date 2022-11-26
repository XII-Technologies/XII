#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

xiiResult xiiTexConvProcessor::Assemble3DTexture(xiiImage& dst) const
{
  XII_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return xiiImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture3D);
