#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>

xiiGALUnorderedAccessViewDiligent::xiiGALUnorderedAccessViewDiligent(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description) :
  xiiGALUnorderedAccessView(pResource, Description), m_pTextureView(nullptr), m_pBufferView(nullptr)
{
}

xiiGALUnorderedAccessViewDiligent::~xiiGALUnorderedAccessViewDiligent() {}

xiiResult xiiGALUnorderedAccessViewDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const xiiGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    xiiLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return XII_FAILURE;
  }

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (viewFormat == xiiGALResourceFormat::Invalid)
      viewFormat = pTexture->GetDescription().m_Format;
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
      xiiGALTexture* pGALTexture = const_cast<xiiGALTexture*>(pTexture);
      pGALTextureDiligent        = static_cast<xiiGALTextureDiligent*>(pGALTexture);
      pTextureDiligent           = pGALTextureDiligent->GetTexture();
    }

    const xiiGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    Diligent::TextureViewDesc UAVDesc;
    UAVDesc.ViewType = Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
    UAVDesc.Format   = viewFormatDiligent;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture1D:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
      }
      break;
      case xiiGALTextureType::Texture1DArray:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
      }
      break;
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
      }
      break;
      case xiiGALTextureType::Texture2DArray:
      case xiiGALTextureType::Texture2DProxyArray:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
      }
      break;
      case xiiGALTextureType::Texture3D:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        UAVDesc.NumDepthSlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstDepthSlice = m_Description.m_uiFirstArraySlice;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
      }
      break;
      case xiiGALTextureType::TextureCube:
      case xiiGALTextureType::TextureCubeArray:
      {
        xiiLog::Error("Unexpected unordered access view type '{}' for texture '{}'.", texDesc.m_Type, texDesc.m_szName);
        return XII_FAILURE;
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    UAVDesc.Flags       = Diligent::TEXTURE_VIEW_FLAG_NONE;
    UAVDesc.AccessFlags = Diligent::UAV_ACCESS_UNSPECIFIED;

    pTextureDiligent->CreateView(UAVDesc, &m_pTextureView);

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

    Diligent::BufferViewDesc UAVDesc;
    UAVDesc.ViewType = Diligent::BUFFER_VIEW_UNORDERED_ACCESS;

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
    {
      UAVDesc.ByteOffset = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      UAVDesc.ByteWidth  = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else
    {
      UAVDesc.ByteWidth            = m_Description.m_uiNumElements;
      UAVDesc.ByteOffset           = m_Description.m_uiFirstElement;
      UAVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(viewFormatDiligent);
      UAVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(viewFormatDiligent);
      UAVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
    }

    pBufferDiligent->CreateView(UAVDesc, &m_pBufferView);

    return (m_pBufferView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALUnorderedAccessViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pBufferView);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_UnorderedAccessViewDiligent);
