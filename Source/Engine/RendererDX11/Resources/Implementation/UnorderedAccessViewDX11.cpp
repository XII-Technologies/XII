#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>

#include <d3d11.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

xiiGALUnorderedAccessViewDX11::xiiGALUnorderedAccessViewDX11(
  xiiGALResourceBase*                                 pResource,
  const xiiGALUnorderedAccessViewCreationDescription& Description) :
  xiiGALUnorderedAccessView(pResource, Description), m_pDXUnorderedAccessView(nullptr)
{
}

xiiGALUnorderedAccessViewDX11::~xiiGALUnorderedAccessViewDX11() {}

xiiResult xiiGALUnorderedAccessViewDX11::InitPlatform(xiiGALDevice* pDevice)
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

  D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
  DXUAVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  if (pTexture)
  {
    pDXResource                                     = static_cast<const xiiGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
    const xiiGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    switch (texDesc.m_Type)
    {
      case xiiGALTextureType::Texture2D:
      case xiiGALTextureType::Texture2DArray:
      case xiiGALTextureType::Texture2DProxy:
      case xiiGALTextureType::Texture2DProxyArray:

        if (!bIsArrayView)
        {
          DXUAVDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
          DXUAVDesc.Texture2D.MipSlice = m_Description.m_uiMipLevelToUse;
        }
        else
        {
          DXUAVDesc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
          DXUAVDesc.Texture2DArray.MipSlice        = m_Description.m_uiMipLevelToUse;
          DXUAVDesc.Texture2DArray.ArraySize       = m_Description.m_uiArraySize;
          DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
        break;

      case xiiGALTextureType::TextureCube:
      case xiiGALTextureType::TextureCubeArray:
        DXUAVDesc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        DXUAVDesc.Texture2DArray.MipSlice        = m_Description.m_uiMipLevelToUse;
        DXUAVDesc.Texture2DArray.ArraySize       = m_Description.m_uiArraySize;
        DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        break;

      case xiiGALTextureType::Texture3D:

        DXUAVDesc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
        DXUAVDesc.Texture3D.MipSlice    = m_Description.m_uiMipLevelToUse;
        DXUAVDesc.Texture3D.FirstWSlice = m_Description.m_uiFirstArraySlice;
        DXUAVDesc.Texture3D.WSize       = m_Description.m_uiArraySize;
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
      DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXUAVDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    DXUAVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXUAVDesc.Buffer.NumElements  = m_Description.m_uiNumElements;
    DXUAVDesc.Buffer.Flags        = 0;
    if (m_Description.m_bRawView)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
    if (m_Description.m_bAppend)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &m_pDXUnorderedAccessView)))
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}

xiiResult xiiGALUnorderedAccessViewDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXUnorderedAccessView);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_UnorderedAccessViewDX11);
