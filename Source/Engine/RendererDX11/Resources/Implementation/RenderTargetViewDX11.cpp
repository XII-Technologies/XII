#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

xiiGALRenderTargetViewDX11::xiiGALRenderTargetViewDX11(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) :
  xiiGALRenderTargetView(pTexture, Description), m_pRenderTargetView(nullptr), m_pDepthStencilView(nullptr), m_pUnorderedAccessView(nullptr)
{
}

xiiGALRenderTargetViewDX11::~xiiGALRenderTargetViewDX11() {}

xiiResult xiiGALRenderTargetViewDX11::InitPlatform(xiiGALDevice* pDevice)
{
  const xiiGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    xiiLog::Error("No valid texture handle given for rendertarget view creation!");
    return XII_FAILURE;
  }

  const xiiGALTextureCreationDescription& texDesc    = pTexture->GetDescription();
  xiiGALResourceFormat::Enum              viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != xiiGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;


  xiiGALDeviceDX11* pDXDevice = static_cast<xiiGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;

  const bool bIsDepthFormat = xiiGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get DXGI format for view!");
    return XII_FAILURE;
  }

  ID3D11Resource* pDXResource  = static_cast<const xiiGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
  const bool      bIsArrayView = IsArrayView(texDesc, m_Description);

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        DSViewDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        DSViewDesc.Texture2DArray.MipSlice        = m_Description.m_uiMipLevel;
        DSViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DArray.ArraySize       = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        // DSViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        DSViewDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
        DSViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DMSArray.ArraySize       = m_Description.m_uiSliceCount;
      }
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pDXResource, &DSViewDesc, &m_pDepthStencilView)))
    {
      xiiLog::Error("Couldn't create depth stencil view!");
      return XII_FAILURE;
    }
    else
    {
      return XII_SUCCESS;
    }
  }
  else
  {
    D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
    RTViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        RTViewDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        RTViewDesc.Texture2DArray.MipSlice        = m_Description.m_uiMipLevel;
        RTViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DArray.ArraySize       = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        // RTViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        RTViewDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
        RTViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DMSArray.ArraySize       = m_Description.m_uiSliceCount;
      }
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pDXResource, &RTViewDesc, &m_pRenderTargetView)))
    {
      xiiLog::Error("Couldn't create rendertarget view!");
      return XII_FAILURE;
    }
    else
    {
      return XII_SUCCESS;
    }
  }
}

xiiResult xiiGALRenderTargetViewDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pRenderTargetView);
  XII_GAL_DX11_RELEASE(m_pDepthStencilView);
  XII_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_RenderTargetViewDX11);
