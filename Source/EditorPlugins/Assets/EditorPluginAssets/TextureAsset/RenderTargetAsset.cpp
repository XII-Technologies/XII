/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <Texture/Utilities/TextureAssetFile.h>

xiiTransformStatus xiiTextureAssetDocument::WriteRenderTargetAsset(xiiStringView sTargetFile, const xiiAssetFileHeader& header, const xiiTextureAssetProfileConfig* pProfile) const
{
  if (pProfile == nullptr)
    return xiiTransformStatus("Texture asset profile is missing.");
  const auto* properties = GetProperties();
  xiiUInt32   resolution = 0;
  if (pProfile->GetRenderTargetResolution(properties->m_Resolution, properties->m_fResolutionScale, resolution).Failed())
    return xiiTransformStatus("Invalid render-target resolution or profile scale.");

  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {resolution, resolution};
  description.m_Usage     = xiiGALResourceUsage::Mutable;
  description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  switch (properties->m_RtFormat)
  {
    case xiiRenderTargetFormat::RGBA8sRgb:
      description.m_Format = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      break;
    case xiiRenderTargetFormat::RGBA8:
      description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      break;
    case xiiRenderTargetFormat::RGB10:
      description.m_Format = xiiGALResourceFormat::RG11B10Float;
      break;
    case xiiRenderTargetFormat::RGBA16:
      description.m_Format = xiiGALResourceFormat::RGBA16Float;
      break;
    default:
      return xiiTransformStatus("Invalid render-target format.");
  }
  xiiGALSamplerCreationDescription sampler;
  if (properties->GetSamplerDescription(sampler).Failed())
    return xiiTransformStatus("Invalid render-target sampler filtering, anisotropy, LOD bounds or address mode.");
  xiiDeferredFileWriter file;
  file.SetOutput(sTargetFile);
  XII_SUCCEED_OR_RETURN(xiiTextureAssetFile::WriteRenderTarget(file, description, sampler, header));
  if (file.Close().Failed())
    return xiiTransformStatus(xiiFmt("Writing render-target asset failed: '{}'", sTargetFile));
  return XII_SUCCESS;
}
