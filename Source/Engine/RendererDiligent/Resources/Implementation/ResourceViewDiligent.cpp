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
  xiiGALResourceView(pResource, Description), m_pResourceView(nullptr)
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
    xiiLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return XII_FAILURE;
  }

  if (pTexture)
  {
    xiiGALResourceBase*                          pRes             = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    Diligent::RefCntAutoPtr<Diligent::ITexture>& pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes)->GetTexture();
    const xiiGALTextureCreationDescription&      texDesc          = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    Diligent::RefCntAutoPtr<Diligent::ITextureView> pTexView;
    Diligent::TextureViewDesc                       SRVDesc;
    SRVDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      {
        if (!bIsArrayView)
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
        else
        {
          if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
          {
            SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
            SRVDesc.NumMipLevels    = m_Description.m_uiMipLevelsToUse;
            SRVDesc.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
            SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
            SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
          else
          {
            SRVDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
            SRVDesc.NumArraySlices  = m_Description.m_uiArraySize;
            SRVDesc.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
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

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }

    pTextureDiligent->CreateView(SRVDesc, &pTexView);

    if (!pTexView)
    {
      xiiLog::Error("Failed to create texture view.");
      return XII_FAILURE;
    }

    m_pResourceView = pTexView;
  }
  else if (pBuffer)
  {
    xiiGALResourceBase*                         pRes            = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    Diligent::RefCntAutoPtr<Diligent::IBuffer>& pBufferDiligent = static_cast<xiiGALBufferDiligent*>(pRes)->GetBuffer();

    Diligent::RefCntAutoPtr<Diligent::IBufferView> pBufferView;
    Diligent::BufferViewDesc                       SRVDesc;
    SRVDesc.ViewType = Diligent::BUFFER_VIEW_SHADER_RESOURCE;

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      SRVDesc.Format.ValueType = Diligent::VT_UNDEFINED;

    SRVDesc.ByteOffset = m_Description.m_uiFirstElement;
    SRVDesc.ByteWidth  = m_Description.m_uiNumElements;

    pBufferDiligent->CreateView(SRVDesc, &pBufferView);

    if (!pBufferView)
    {
      xiiLog::Error("Failed to create buffer view.");
      return XII_FAILURE;
    }

    m_pResourceView = pBufferView;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALResourceViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_RELEASE(m_pResourceView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_ResourceViewDiligent);
