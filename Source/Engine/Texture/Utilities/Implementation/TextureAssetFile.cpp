/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>
#include <Texture/Utilities/VirtualTextureData.h>

namespace
{
  xiiResult WriteEnvelope(xiiStreamWriter& stream, const xiiGALTextureCreationDescription& description, const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header)
  {
    // Native stream operators assert on write failure. Stage the small envelope in memory so
    // destination I/O failures propagate through xiiResult instead of those operators.
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_SUCCEED_OR_RETURN(header.Write(writer));
    writer << description;
    writer << sampler;
    return storage.CopyToStream(stream);
  }
} // namespace

xiiResult xiiTextureAssetFile::WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header)
{
  if (!image.IsValid())
    return XII_FAILURE;
  const auto& properties = xiiGALTextureUtilities::GetResourceFormatProperties(image.GetImageFormat());
  if (image.GetDescription().m_Type == xiiGALResourceDimension::Texture2D &&
      image.GetWidth() >= 256U && image.GetHeight() >= 256U &&
      image.GetWidth(image.GetMipLevelCount() - 1U) == 1U && image.GetHeight(image.GetMipLevelCount() - 1U) == 1U && ((properties.m_ComponentType == xiiGALResourceFormatComponentType::UnsignedNormalized && properties.m_uiComponentSize == 1U) || (properties.IsCompressed() && image.GetImageFormat() != xiiGALResourceFormat::BC6HUF16 && image.GetImageFormat() != xiiGALResourceFormat::BC6HSF16 && image.GetImageFormat() != xiiGALResourceFormat::BC4SNormalized && image.GetImageFormat() != xiiGALResourceFormat::BC5SNormalized)))
  {
    xiiVirtualTextureData pages;
    XII_SUCCEED_OR_RETURN(pages.Build(image, sampler));
    auto envelope   = image.GetDescription();
    envelope.m_Size = {0U, xiiVirtualTextureData::s_uiMarker};
    XII_SUCCEED_OR_RETURN(WriteEnvelope(inout_stream, envelope, sampler, header));
    return pages.Serialize(inout_stream);
  }
  auto description   = image.GetDescription();
  description.m_Size = {0, 0};
  XII_SUCCEED_OR_RETURN(WriteEnvelope(inout_stream, description, sampler, header));
  xiiDdsFileFormat dds;
  return dds.WriteImage(inout_stream, image, "dds");
}

xiiResult xiiTextureAssetFile::WriteRenderTarget(xiiStreamWriter& inout_stream, const xiiGALTextureCreationDescription& description, const xiiGALSamplerCreationDescription& sampler, const xiiAssetFileHeader& header)
{
  if (!description.Is2D() || !description.m_Size.HasNonZeroArea() || description.m_Format == xiiGALResourceFormat::Unknown || description.m_Usage != xiiGALResourceUsage::Mutable || !description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || description.m_uiArraySizeOrDepth == 0 || description.m_uiMipLevels == 0 || description.m_pExistingNativeObject != nullptr)
    return XII_FAILURE;
  return WriteEnvelope(inout_stream, description, sampler, header);
}
