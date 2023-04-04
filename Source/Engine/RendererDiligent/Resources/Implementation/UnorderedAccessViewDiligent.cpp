#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

xiiGALUnorderedAccessViewDiligent::xiiGALUnorderedAccessViewDiligent(
  xiiGALResourceBase*                                 pResource,
  const xiiGALUnorderedAccessViewCreationDescription& Description) :
  xiiGALUnorderedAccessView(pResource, Description), m_pUnorderedAccessTextureView(nullptr), m_pUnorderedAccessBufferView(nullptr)
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

  if (pTexture)
  {
    xiiGALTextureDiligent* pGALTextureDiligent = nullptr;
    Diligent::ITexture*    pTextureDiligent    = nullptr;
    {

      xiiGALTexture* pGALTexture = const_cast<xiiGALTexture*>(pTexture);
      pGALTextureDiligent        = static_cast<xiiGALTextureDiligent*>(pGALTexture);
      pTextureDiligent           = pGALTextureDiligent->GetTexture();
    }

    const xiiGALTextureCreationDescription& texDesc        = pTexture->GetDescription();
    const bool                              bIsArrayView   = IsArrayView(texDesc, m_Description);
    const bool                              bIsDepthFormat = xiiGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);
    xiiGALResourceFormat::Enum              viewFormat     = m_Description.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;

    Diligent::TextureViewDesc UAVDesc;
    UAVDesc.ViewType        = Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
    UAVDesc.Format          = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture1D:
        UAVDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_1D;
        break;

      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
        UAVDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
        break;

      case xiiGALTextureType::TextureCube:
        UAVDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_CUBE;
        break;

      case xiiGALTextureType::Texture1DArray:
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        break;

      case xiiGALTextureType::Texture2DArray:
      case xiiGALTextureType::Texture2DProxyArray:
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        break;

      case xiiGALTextureType::TextureCubeArray:
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        break;

      case xiiGALTextureType::Texture3D:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        UAVDesc.FirstDepthSlice = m_Description.m_uiFirstArraySlice;
        UAVDesc.NumDepthSlices  = m_Description.m_uiArraySize;
      }
      break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;

        return XII_FAILURE;
    }

    pTextureDiligent->CreateView(UAVDesc, &m_pUnorderedAccessTextureView);

    if (m_pUnorderedAccessTextureView == nullptr)
    {
      return XII_FAILURE;
    }
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
      UAVDesc.Format.ValueType     = Diligent::VT_UNDEFINED;
      UAVDesc.Format.IsNormalized  = false;
      UAVDesc.Format.NumComponents = 0u;
      UAVDesc.ByteOffset           = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      UAVDesc.ByteWidth            = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else if (m_Description.m_bRawView)
    {
      xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == xiiGALResourceFormat::Invalid)
        viewFormat = xiiGALResourceFormat::RUInt;

      Diligent::TEXTURE_FORMAT ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;

      UAVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(ViewFormatDiligent);
      UAVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(ViewFormatDiligent);
      UAVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
      UAVDesc.ByteOffset           = sizeof(xiiUInt32) * m_Description.m_uiFirstElement;
      UAVDesc.ByteWidth            = sizeof(xiiUInt32) * m_Description.m_uiNumElements;
    }
    else
    {
      xiiGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == xiiGALResourceFormat::Invalid)
        viewFormat = xiiGALResourceFormat::RUInt;

      Diligent::TEXTURE_FORMAT ViewFormatDiligent = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;

      UAVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(ViewFormatDiligent);
      UAVDesc.Format.IsNormalized  = xiiDiligentUtils::GALIsFormatNormalized(ViewFormatDiligent);
      UAVDesc.Format.NumComponents = xiiGALResourceFormat::GetChannelCount(viewFormat);
      UAVDesc.ByteOffset           = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      UAVDesc.ByteWidth            = pGALBufferDiligent->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }

    pBufferDiligent->CreateView(UAVDesc, &m_pUnorderedAccessBufferView);

    if (m_pUnorderedAccessBufferView == nullptr)
    {
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALUnorderedAccessViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pUnorderedAccessTextureView);
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pUnorderedAccessBufferView);

  return XII_SUCCESS;
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_UnorderedAccessViewDiligent);
