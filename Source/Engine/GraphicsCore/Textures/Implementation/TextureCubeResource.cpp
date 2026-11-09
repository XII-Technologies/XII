/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <utility>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeResource, 1, xiiRTTIDefaultAllocator<xiiTextureCubeResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTextureCubeResource);

xiiTextureCubeResource::xiiTextureCubeResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
  m_Format           = xiiGALResourceFormat::Unknown;
  m_uiWidthAndHeight = 0;
}

xiiResourceLoadDescription xiiTextureCubeResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);
  XII_LOCK(m_TextureMutex);
  m_pEnvironmentMap.Clear();
  m_pGALTexture.Clear();
  m_pSampler.Clear();
  return {xiiResourceState::Unloaded, 0, 1};
}


xiiResourceLoadDescription xiiTextureCubeResource::UpdateContent(xiiStreamReader* pStream)
{
  const bool                       bHasResidentImage = GetGALTexture() != nullptr;
  const xiiResourceLoadDescription previous          = bHasResidentImage ? xiiResourceLoadDescription{xiiResourceState::Loaded, GetNumQualityLevelsDiscardable(), GetNumQualityLevelsLoadable()} : xiiResourceLoadDescription{xiiResourceState::LoadedResourceMissing, 0, 0};
  if (pStream == nullptr)
    return previous;

  xiiTextureCubeResourceDescriptor td;
  xiiImage*                        pImage      = nullptr;
  bool                             bIsFallback = false;

  {
    *pStream >> td.m_TextureDescription;
    *pStream >> td.m_SamplerDescription;
    *pStream >> bIsFallback;

    if (pStream->ReadBytes(&pImage, sizeof(xiiImage*)) != sizeof(xiiImage*) || pImage == nullptr)
      return previous;
  }

  if (!pImage->IsValid() || (bHasResidentImage && (bIsFallback || previous.m_uiQualityLevelsLoadable == 0U)))
    return previous;

  const xiiUInt32 uiTotalMipLevels = pImage->GetMipLevelCount();
  const xiiUInt32 uiNumMipLevels   = uiTotalMipLevels;
  td.m_uiQualityLevelsDiscardable  = 0U;
  td.m_uiQualityLevelsLoadable     = bIsFallback ? 1U : 0U;

  const xiiUInt32 uiHighestMipLevel = pImage->GetMipLevelCount() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    xiiLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    return previous;
  }

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Format      = pImage->GetImageFormat();
  textureDescription.m_Size.width  = pImage->GetWidth(uiHighestMipLevel);
  textureDescription.m_Size.height = pImage->GetWidth(uiHighestMipLevel);
  textureDescription.m_uiMipLevels = uiNumMipLevels;
  textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage       = xiiGALResourceUsage::Immutable;

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    textureDescription.m_Type               = xiiGALResourceDimension::Texture3D;
    textureDescription.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    textureDescription.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    textureDescription.m_Type               = (textureDescription.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;

    if (pImage->GetNumFaces() == 6)
    {
      textureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
      textureDescription.m_uiArraySizeOrDepth = 6;
    }
  }

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

  xiiHybridArray<xiiGALTextureSubResourceData, 32> InitData;

  for (xiiUInt32 arrayIndex = 0; arrayIndex < pImage->GetNumArrayIndices(); ++arrayIndex)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetMipLevelCount(); ++mip)
      {
        xiiGALTextureSubResourceData& id = InitData.ExpandAndGetRef();
        id.m_pData                       = pImage->GetSubImageView(mip, face, arrayIndex).GetByteBlobPtr();

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt64>(), "Depth pitch exceeds xiiGAL limits.");

        if (formatProperties.IsCompressed())
        {
          const xiiUInt64 uiMemPitchFactor = formatProperties.GetElementSize() * 2 / 8;

          id.m_uiStride = xiiMath::Max<xiiUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = pImage->GetRowPitch(mip);
        }

        id.m_uiDepthStride = pImage->GetDepthPitch(mip);
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(InitData);

  td.m_TextureDescription = textureDescription;
  td.m_InitialContent     = InitDataPtr;

  const auto result = CreateResource(std::move(td));
  return result.m_State == xiiResourceState::Loaded ? result : previous;
}

void xiiTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  XII_LOCK(m_TextureMutex);
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_pGALTexture ? m_pGALTexture->GetMemoryConsumption() : 0U;
  if (m_pEnvironmentMap)
  {
    out_NewMemoryUsage.m_uiMemoryGPU += m_pEnvironmentMap->m_uiMemoryGPU;
    out_NewMemoryUsage.m_uiMemoryCPU += sizeof(xiiEnvironmentMap);
  }
}

xiiStatus xiiTextureCubeResource::GetEnvironmentMap(xiiSharedPtr<const xiiEnvironmentMap>& out_map)
{
  out_map.Clear();
  xiiSharedPtr<xiiGALTexture> source;
  {
    XII_LOCK(m_TextureMutex);
    if (m_pGALTexture == nullptr)
      return xiiStatus("Cannot convolve an unloaded cubemap.");
    out_map = m_pEnvironmentMap;
    if (out_map)
      return XII_SUCCESS;
    source = m_pGALTexture;
  }
  // Shader acquisition may wait for resource loader tasks, including this
  // resource's memory update. Never hold its mutex across those acquisitions.
  xiiSharedPtr<const xiiEnvironmentMap> prepared;
  XII_SUCCEED_OR_RETURN(xiiEnvironmentMap::Create(source, prepared));
  {
    XII_LOCK(m_TextureMutex);
    if (m_pGALTexture == source)
    {
      if (m_pEnvironmentMap == nullptr)
        m_pEnvironmentMap = prepared;
      prepared = m_pEnvironmentMap;
    }
  }
  RefreshMemoryUsage();
  out_map = std::move(prepared);
  return XII_SUCCESS;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTextureCubeResource, xiiTextureCubeResourceDescriptor)
{
  auto pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};

  auto& description = descriptor.m_TextureDescription;
  if (description.m_Size.width != description.m_Size.height)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  description.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);
  xiiGALTextureData textureData(descriptor.m_InitialContent);
  auto              pTexture = pDevice->CreateTexture(description, descriptor.m_InitialContent.IsEmpty() ? nullptr : &textureData);
  if (pTexture == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  auto pSampler = pDevice->CreateSampler(descriptor.m_SamplerDescription);
  if (pSampler == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  pTexture->SetDebugName(GetResourceIdOrDescription());

  XII_LOCK(m_TextureMutex);
  m_Format           = description.m_Format;
  m_uiWidthAndHeight = description.m_Size.width;
  m_pEnvironmentMap.Clear();
  m_pGALTexture = std::move(pTexture);
  m_pSampler    = std::move(pSampler);
  return {xiiResourceState::Loaded, descriptor.m_uiQualityLevelsDiscardable, descriptor.m_uiQualityLevelsLoadable};
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_Implementation_TextureCubeResource);
