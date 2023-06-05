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

  if (pTexture)
  {
    xiiGALTextureDiligent* pGALTextureDiligent = nullptr;
    Diligent::ITexture*    pTextureDiligent    = nullptr;
    {
      xiiGALResourceBase* pGALTexture = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
      pGALTextureDiligent             = static_cast<xiiGALTextureDiligent*>(pGALTexture);
      pTextureDiligent                = pGALTextureDiligent->GetTexture();
    }

    const xiiGALTextureCreationDescription& texDesc        = pTexture->GetDescription();
    const bool                              bIsDepthFormat = xiiGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);
    xiiGALResourceFormat::Enum              viewFormat     = m_Description.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;

    Diligent::TextureViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    SRVDesc.Format   = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;

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
    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      xiiLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return XII_FAILURE;
    }

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
      SRVDesc.Format.ValueType     = Diligent::VT_UNDEFINED;
      SRVDesc.Format.IsNormalized  = false;
      SRVDesc.Format.NumComponents = 0u;
      SRVDesc.ByteOffset           = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      SRVDesc.ByteWidth            = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else if (m_Description.m_bRawView)
    {
      xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == xiiGALResourceFormat::Invalid)
        viewFormat = xiiGALResourceFormat::RUInt;

      Diligent::TEXTURE_FORMAT ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;

      SRVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(ViewFormatDiligent);
      SRVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(ViewFormatDiligent);
      SRVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
      SRVDesc.ByteOffset           = sizeof(xiiUInt32) * m_Description.m_uiFirstElement;
      SRVDesc.ByteWidth            = sizeof(xiiUInt32) * m_Description.m_uiNumElements;
    }
    else
    {
      xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == xiiGALResourceFormat::Invalid)
        viewFormat = xiiGALResourceFormat::RUInt;

      Diligent::TEXTURE_FORMAT ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;

      SRVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(ViewFormatDiligent);
      SRVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(ViewFormatDiligent);
      SRVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
      SRVDesc.ByteOffset           = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      SRVDesc.ByteWidth            = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }

    pBufferDiligent->CreateView(SRVDesc, &m_pBufferView);

    return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }

  return XII_FAILURE;
}

xiiResult xiiGALResourceViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pBufferView);
  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_ResourceViewDiligent);
