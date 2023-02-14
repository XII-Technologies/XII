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

  xiiGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    const xiiGALTextureCreationDescription& TexDesc = pTexture->GetDescription();
    if (ViewFormat == xiiGALResourceFormat::Invalid)
      ViewFormat = TexDesc.m_Format;
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
    xiiGALResourceBase*                     pRes             = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    Diligent::ITexture*                     pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes)->GetTexture();
    const xiiGALTextureCreationDescription& texDesc          = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    Diligent::TextureViewDesc UAVDesc;
    UAVDesc.ViewType = Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
    UAVDesc.Format   = ViewFormatDiligent;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        if (!bIsArrayView)
        {
          UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
          UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
        }
        else
        {
          UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
          UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
          UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
          UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
      }
      break;

      case xiiGALTextureType::TextureCube:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_CUBE;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
        UAVDesc.NumArraySlices  = m_Description.m_uiArraySize;
        UAVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
      }
      break;

      case xiiGALTextureType::Texture3D:
      {
        UAVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        UAVDesc.MostDetailedMip = m_Description.m_uiMipLevelToUse;
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
    // TODO: Get current format is normalized.

    xiiGALResourceBase* pRes            = const_cast<xiiGALResourceBase*>(pBuffer->GetParentResource());
    Diligent::IBuffer*  pBufferDiligent = static_cast<xiiGALBufferDiligent*>(pRes)->GetBuffer();

    Diligent::BufferViewDesc UAVDesc;
    UAVDesc.ViewType             = Diligent::BUFFER_VIEW_UNORDERED_ACCESS;
    UAVDesc.Format.ValueType     = xiiDiligentUtils::GALToDiligentFormat(ViewFormatDiligent);
    UAVDesc.Format.NumComponents = xiiDiligentUtils::GALToDiligentNumComponent(ViewFormatDiligent);

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      UAVDesc.Format.ValueType = Diligent::VT_UNDEFINED;

    UAVDesc.ByteOffset = m_Description.m_uiFirstElement;
    UAVDesc.ByteWidth  = m_Description.m_uiNumElements;

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
