#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

xiiGALResourceViewDiligent::xiiGALResourceViewDiligent(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) :
  xiiGALResourceView(pResource, Description), m_pTextureView(nullptr), m_pBufferView(nullptr)
{
}

xiiGALResourceViewDiligent::~xiiGALResourceViewDiligent() {}

xiiResult xiiGALResourceViewDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const xiiGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return XII_FAILURE;
  }

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (viewFormat == xiiGALResourceFormat::Invalid)
      viewFormat = pTexture->GetDescription().m_Format;
  }
  else if (pBuffer)
  {
    if (viewFormat == xiiGALResourceFormat::Invalid)
      viewFormat = xiiGALResourceFormat::RUInt;

    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      xiiLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return XII_FAILURE;
    }
  }

  Diligent::TEXTURE_FORMAT viewFormatDiligent = Diligent::TEX_FORMAT_UNKNOWN;
  if (xiiGALResourceFormat::IsDepthFormat(viewFormat))
  {
    viewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthOnlyType;
  }
  else
  {
    viewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
  }

  if (viewFormatDiligent == Diligent::TEX_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get valid format for resource view! ({0})", viewFormat);
    return XII_FAILURE;
  }

  if (pTexture)
  {
    xiiGALTextureDiligent* pGALTextureDiligent = nullptr;
    Diligent::ITexture*    pTextureDiligent    = nullptr;
    {
      xiiGALResourceBase* pGALTexture = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
      pGALTextureDiligent             = static_cast<xiiGALTextureDiligent*>(pGALTexture);
      pTextureDiligent                = pGALTextureDiligent->GetTexture();
    }

    const xiiGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    Diligent::TextureViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    SRVDesc.Format   = viewFormatDiligent;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture1D:
      {
        SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D;
        SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
        SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      break;
      case xiiGALTextureType::Texture1DArray:
      {
        SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
        SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
        SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      break;
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          SRVDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
        }
      }
      break;
      case xiiGALTextureType::Texture2DArray:
      case xiiGALTextureType::Texture2DProxyArray:
      {
        if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
          SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
          SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
          SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
          SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
      }
      break;
      case xiiGALTextureType::Texture3D:
      {
        SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
        SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      break;
      case xiiGALTextureType::TextureCube:
      {
        SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE;
        SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
        SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      break;
      case xiiGALTextureType::TextureCubeArray:
      {
        SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
        SRVDesc.NumArraySlices  = m_Description.m_uiArraySize * 6u;
        SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
        SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    if (pGALTextureDiligent->GetDescription().m_bAllowDynamicMipGeneration)
      SRVDesc.Flags |= Diligent::TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;

    SRVDesc.AccessFlags = Diligent::UAV_ACCESS_UNSPECIFIED;

    pTextureDiligent->CreateView(SRVDesc, &m_pTextureView);

    return (m_pTextureView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }
  else if (pBuffer)
  {
    Diligent::IBuffer*    pBufferDiligent    = nullptr;
    xiiGALBufferDiligent* pGALBufferDiligent = nullptr;
    {
      xiiGALBuffer* pGALBuffer = const_cast<xiiGALBuffer*>(pBuffer);
      pGALBufferDiligent       = static_cast<xiiGALBufferDiligent*>(pGALBuffer);
      pBufferDiligent          = pGALBufferDiligent->GetBuffer();
    }

    Diligent::BufferViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::BUFFER_VIEW_SHADER_RESOURCE;

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
    {
      SRVDesc.ByteOffset = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      SRVDesc.ByteWidth  = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else
    {
      SRVDesc.ByteWidth            = m_Description.m_uiNumElements;
      SRVDesc.ByteOffset           = m_Description.m_uiFirstElement;
      SRVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(viewFormatDiligent);
      SRVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(viewFormatDiligent);
      SRVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
    }

    pBufferDiligent->CreateView(SRVDesc, &m_pBufferView);

    return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }

  return XII_FAILURE;
}

xiiResult xiiGALResourceViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pBufferView);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_ResourceViewDiligent);
