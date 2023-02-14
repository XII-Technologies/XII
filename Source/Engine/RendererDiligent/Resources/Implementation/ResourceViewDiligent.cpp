#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

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

  xiiGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (ViewFormat == xiiGALResourceFormat::Invalid)
      ViewFormat = pTexture->GetDescription().m_Format;
  }
  else if (pBuffer)
  {
    if (ViewFormat == xiiGALResourceFormat::Invalid)
      ViewFormat = xiiGALResourceFormat::RUInt;

    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      xiiLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return XII_FAILURE;
    }
  }

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  Diligent::TEXTURE_FORMAT ViewFormatDiligent = Diligent::TEX_FORMAT_UNKNOWN;
  if (xiiGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (ViewFormatDiligent == Diligent::TEX_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get valid format for resource view! ({0})", ViewFormat);
    return XII_FAILURE;
  }

  if (pTexture)
  {
    xiiGALTexture*         pRes                = const_cast<xiiGALTexture*>(pTexture);
    xiiGALTextureDiligent* pGALTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes);
    Diligent::ITexture*    pTextureDiligent    = pGALTextureDiligent->GetTexture();

    const xiiGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    Diligent::TextureViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    SRVDesc.Format   = ViewFormatDiligent;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        if (!bIsArrayView)
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
          SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
      }
      break;

      case xiiGALTextureType::TextureCube:
      {
        if (!bIsArrayView)
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
          SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
          SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
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

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;

        return XII_FAILURE;
    }

    pTextureDiligent->CreateView(SRVDesc, &m_pTextureView);

    if (m_pTextureView == nullptr)
    {
      return XII_FAILURE;
    }
  }
  else if (pBuffer)
  {
    // TODO: Get current format is normalized.

    xiiGALBuffer*         pRes               = const_cast<xiiGALBuffer*>(pBuffer);
    xiiGALBufferDiligent* pGALBufferDiligent = static_cast<xiiGALBufferDiligent*>(pRes);
    Diligent::IBuffer*    pBufferDiligent    = pGALBufferDiligent->GetBuffer();

    Diligent::BufferViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::BUFFER_VIEW_SHADER_RESOURCE;

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      SRVDesc.Format.ValueType = Diligent::VT_UNDEFINED;

    SRVDesc.ByteOffset = m_Description.m_uiFirstElement;
    SRVDesc.ByteWidth  = m_Description.m_uiNumElements;

    pBufferDiligent->CreateView(SRVDesc, &m_pBufferView);

    if (m_pBufferView == nullptr)
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALResourceViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pBufferView);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_ResourceViewDiligent);
