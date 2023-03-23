#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

xiiGALResourceViewDX11::xiiGALResourceViewDX11(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) :
  xiiGALResourceView(pResource, Description), m_pDXResourceView(nullptr)
{
}

xiiGALResourceViewDX11::~xiiGALResourceViewDX11() {}

xiiResult xiiGALResourceViewDX11::InitPlatform(xiiGALDevice* pDevice)
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

  xiiGALDeviceDX11* pDXDevice = static_cast<xiiGALDeviceDX11*>(pDevice);


  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
  if (xiiGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return XII_FAILURE;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  if (pTexture)
  {
    pDXResource                                     = static_cast<const xiiGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
    const xiiGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DProxy:
      case xiiGALTextureType::Texture2DArray:
      case xiiGALTextureType::Texture2DProxyArray:

        if (!bIsArrayView)
        {
          if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            DXSRVDesc.Texture2D.MipLevels       = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
          }
        }
        else
        {
          if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            DXSRVDesc.Texture2DArray.MipLevels       = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2DArray.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
            DXSRVDesc.Texture2DArray.ArraySize       = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
          else
          {
            DXSRVDesc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            DXSRVDesc.Texture2DMSArray.ArraySize       = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
        }

        break;

      case xiiGALTextureType::TextureCube:
      case xiiGALTextureType::TextureCubeArray:

        if (!bIsArrayView)
        {
          DXSRVDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
          DXSRVDesc.TextureCube.MipLevels       = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          DXSRVDesc.ViewDimension                     = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
          DXSRVDesc.TextureCube.MipLevels             = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip       = m_Description.m_uiMostDetailedMipLevel;
          DXSRVDesc.TextureCubeArray.NumCubes         = m_Description.m_uiArraySize;
          DXSRVDesc.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArraySlice;
        }

        break;

      case xiiGALTextureType::Texture3D:

        DXSRVDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
        DXSRVDesc.Texture3D.MipLevels       = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;

        break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return XII_FAILURE;
    }
  }
  else if (pBuffer)
  {
    pDXResource = static_cast<const xiiGALBufferDX11*>(pBuffer)->GetDXBuffer();

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXSRVDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFER;
    DXSRVDesc.BufferEx.FirstElement = DXSRVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXSRVDesc.BufferEx.NumElements = DXSRVDesc.Buffer.NumElements = m_Description.m_uiNumElements;
    DXSRVDesc.BufferEx.Flags                                      = m_Description.m_bRawView ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}

xiiResult xiiGALResourceViewDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXResourceView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_ResourceViewDX11);
