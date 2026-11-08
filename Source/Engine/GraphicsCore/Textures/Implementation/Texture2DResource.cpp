/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTexture2DResource, 1, xiiRTTIDefaultAllocator<xiiTexture2DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTexture2DResource);

xiiTexture2DResource::xiiTexture2DResource() :
  xiiResource(DoUpdate::OnAnyThread, 1U)
{
}

xiiTexture2DResource::xiiTexture2DResource(xiiResource::DoUpdate ResourceUpdateThread) :
  xiiResource(ResourceUpdateThread, 1U)
{
}

xiiResourceLoadDescription xiiTexture2DResource::UnloadData(Unload whatToUnload)
{
  XII_IGNORE_UNUSED(whatToUnload);
  XII_LOCK(m_TextureMutex);
  m_pGALTexture.Clear();
  m_pSampler.Clear();
  m_NextRefinementAttempt = xiiTime::MakeZero();
  return {xiiResourceState::Unloaded, 0, 1};
}

bool xiiTexture2DResource::CanLoadAdditionalQualityLevel() const
{
  const auto pDevice = xiiGALDevice::HasDefaultDevice() ? xiiGALDevice::GetDefaultDevice() : nullptr;
  if (pDevice == nullptr || pDevice->IsStreamingMemoryPressured())
    return false;

  XII_LOCK(m_TextureMutex);
  return xiiTime::Now() >= m_NextRefinementAttempt;
}

void xiiTexture2DResource::FillOutDescriptor(xiiTexture2DResourceDescriptor& ref_td, const xiiImage* pImage, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALTextureSubResourceData, 32>& ref_initData)
{
  const xiiUInt32 uiHighestMipLevel = pImage->GetMipLevelCount() - uiNumMipLevels;

  const xiiEnum<xiiGALResourceFormat>    format           = pImage->GetImageFormat();
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(format);

  ref_td.m_TextureDescription.m_Format      = format;
  ref_td.m_TextureDescription.m_Size.width  = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_TextureDescription.m_Size.height = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_TextureDescription.m_uiMipLevels = uiNumMipLevels;
  ref_td.m_TextureDescription.m_Usage       = xiiGALResourceUsage::Immutable;
  ref_td.m_TextureDescription.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);

  xiiUInt32 uiDepth = pImage->GetDepth(uiHighestMipLevel);
  if (uiDepth > 1)
  {
    ref_td.m_TextureDescription.m_Type               = xiiGALResourceDimension::Texture3D;
    ref_td.m_TextureDescription.m_uiArraySizeOrDepth = uiDepth;
  }
  else
  {
    ref_td.m_TextureDescription.m_uiArraySizeOrDepth = pImage->GetNumArrayIndices();
    ref_td.m_TextureDescription.m_Type               = (ref_td.m_TextureDescription.m_uiArraySizeOrDepth > 1) ? xiiGALResourceDimension::Texture2DArray : xiiGALResourceDimension::Texture2D;

    if (pImage->GetNumFaces() == 6)
      ref_td.m_TextureDescription.m_Type = xiiGALResourceDimension::TextureCube;
  }

  if (formatProperties.IsCompressed())
  {
    ref_td.m_TextureDescription.m_Size.width  = xiiMath::RoundUp(ref_td.m_TextureDescription.m_Size.width, 4);
    ref_td.m_TextureDescription.m_Size.height = xiiMath::RoundUp(ref_td.m_TextureDescription.m_Size.height, 4);
  }

  XII_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (xiiUInt32 arrayIndex = 0; arrayIndex < pImage->GetNumArrayIndices(); ++arrayIndex)
  {
    for (xiiUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (xiiUInt32 mip = uiHighestMipLevel; mip < pImage->GetMipLevelCount(); ++mip)
      {
        xiiGALTextureSubResourceData& id = ref_initData.ExpandAndGetRef();
        id.m_pData                       = pImage->GetSubImageView(mip, face, arrayIndex).GetByteBlobPtr();

        XII_ASSERT_DEV(pImage->GetDepthPitch(mip) < xiiMath::MaxValue<xiiUInt64>(), "Depth pitch exceeds xiiGAL limits.");

        if (formatProperties.IsCompressed())
        {
          const xiiUInt32 uiMemPitchFactor = formatProperties.GetElementSize() * 2 / 8;

          id.m_uiStride = xiiMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiStride = pImage->GetRowPitch(mip);
        }

        id.m_uiDepthStride = pImage->GetDepthPitch(mip);

        out_uiMemoryUsed += static_cast<xiiUInt32>(id.m_uiDepthStride);
      }
    }
  }

  const xiiArrayPtr<xiiGALTextureSubResourceData> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}

xiiResourceLoadDescription xiiTexture2DResource::UpdateContent(xiiStreamReader* pStream)
{
  const bool                       bHasResidentImage = GetGALTexture() != nullptr;
  const xiiResourceLoadDescription previous          = bHasResidentImage ? xiiResourceLoadDescription{xiiResourceState::Loaded, GetNumQualityLevelsDiscardable(), GetNumQualityLevelsLoadable()} : xiiResourceLoadDescription{xiiResourceState::LoadedResourceMissing, 0, 0};
  if (pStream == nullptr)
    return previous;

  xiiTexture2DResourceDescriptor td;
  xiiImage*                      pImage      = nullptr;
  bool                           bIsFallback = false;

  {
    *pStream >> td.m_TextureDescription;
    *pStream >> td.m_SamplerDescription;
    *pStream >> bIsFallback;

    if (pStream->ReadBytes(&pImage, sizeof(xiiImage*)) != sizeof(xiiImage*) || pImage == nullptr)
      return previous;
  }

  if (td.m_TextureDescription.m_Size.width > 0)
  {
    // Definition-backed render targets have no image or streaming quality levels.
    if (!td.m_TextureDescription.Is2D() || !td.m_TextureDescription.m_Size.HasNonZeroArea() || td.m_TextureDescription.m_Usage != xiiGALResourceUsage::Mutable || !td.m_TextureDescription.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
      return previous;
    td.m_uiQualityLevelsDiscardable = 0;
    td.m_uiQualityLevelsLoadable    = 0;
    return CreateResource(std::move(td));
  }
  if (!pImage->IsValid() || (bHasResidentImage && (bIsFallback || previous.m_uiQualityLevelsLoadable == 0U)))
    return previous;

  const xiiUInt32 uiTotalMipLevels = pImage->GetMipLevelCount();
  const xiiUInt32 uiNumMipLevels   = bHasResidentImage ? uiTotalMipLevels : xiiMath::Min(uiTotalMipLevels, 6U);
  td.m_uiQualityLevelsDiscardable  = 0U;
  td.m_uiQualityLevelsLoadable     = bIsFallback || uiNumMipLevels < uiTotalMipLevels ? 1U : 0U;
  td.m_bStreamingRefinement        = bHasResidentImage;

  xiiTemporaryHybridArray<xiiGALTextureSubResourceData, 32> initData;
  xiiUInt32                                                 uiImageBytes = 0U;
  FillOutDescriptor(td, pImage, uiNumMipLevels, uiImageBytes, initData);

  const auto result = CreateResource(std::move(td));
  if (result.m_State != xiiResourceState::Loaded && bHasResidentImage)
  {
    XII_LOCK(m_TextureMutex);
    m_NextRefinementAttempt = xiiTime::Now() + xiiTime::MakeFromSeconds(1);
  }
  return result.m_State == xiiResourceState::Loaded ? result : previous;
}

void xiiTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  XII_LOCK(m_TextureMutex);
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_pGALTexture ? m_pGALTexture->GetMemoryConsumption() : 0U;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiTexture2DResource, xiiTexture2DResourceDescriptor)
{
  auto pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};

  auto& description = descriptor.m_TextureDescription;
  description.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);
  xiiGALTextureData textureData(descriptor.m_InitialContent);
  textureData.m_bStreamingRefinement = descriptor.m_bStreamingRefinement;
  auto pTexture                      = pDevice->CreateTexture(description, descriptor.m_InitialContent.IsEmpty() ? nullptr : &textureData);
  if (pTexture == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  auto pSampler = pDevice->CreateSampler(descriptor.m_SamplerDescription);
  if (pSampler == nullptr)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  pTexture->SetDebugName(GetResourceIdOrDescription());

  XII_LOCK(m_TextureMutex);
  m_Type        = description.m_Type;
  m_Format      = description.m_Format;
  m_uiWidth     = description.m_Size.width;
  m_uiHeight    = description.m_Size.height;
  m_pGALTexture = std::move(pTexture);
  m_pSampler    = std::move(pSampler);
  return {xiiResourceState::Loaded, descriptor.m_uiQualityLevelsDiscardable, descriptor.m_uiQualityLevelsLoadable};
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_Implementation_Texture2DResource);
