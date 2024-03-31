#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/TextureD3D11.h>
#include <GraphicsD3D11/Resources/TextureViewD3D11.h>

xiiGALTextureViewD3D11::xiiGALTextureViewD3D11(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pTexture, creationDescription)
{
}

xiiGALTextureViewD3D11::~xiiGALTextureViewD3D11() = default;

xiiResult xiiGALTextureViewD3D11::InitPlatform(xiiGALDevice* pDevice)
{
// xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_D3D11_RELEASE(m_pTextureView);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D11::CreateSRV(ID3D11ShaderResourceView** ppShaderResourceView)
{
  XII_ASSERT_DEV(m_Description.m_ViewType == xiiGALTextureViewType::ShaderResource, "xiiGALTextureViewType::ShaderResource view type is expected.");

  if (!(m_Description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || m_Description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray || m_Description.m_ResourceDimension == xiiGALResourceDimension::TextureCube || m_Description.m_ResourceDimension == xiiGALResourceDimension::TextureCubeArray))
  {
    xiiLog::Error("Unsupported texture view type.");
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(m_Description.m_Format != xiiGALTextureFormat::Unknown, "");

  xiiGALDeviceD3D11*  pDeviceD3D11  = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALTextureD3D11* pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(m_pDevice->GetTexture(m_Description.m_hTexture));

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
  viewDescription.Format                          = xiiD3D11TypeConversions::GetFormat(m_Description.m_Format);

  const auto& textureDescription = pTextureD3D11->GetDescription();

  switch (m_Description.m_ResourceDimension)
  {
    case xiiGALResourceDimension::Texture1D:
    {
      viewDescription.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE1D;
      viewDescription.Texture1D.MipLevels       = m_Description.m_uiMipLevelCount;
      viewDescription.Texture1D.MostDetailedMip = m_Description.m_uiMostDetailedMip;
    }
    break;
    case xiiGALResourceDimension::Texture1DArray:
    {
      viewDescription.ViewDimension                  = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
      viewDescription.Texture1DArray.ArraySize       = m_Description.m_uiArrayOrDepthSlicesCount;
      viewDescription.Texture1DArray.FirstArraySlice = m_Description.m_uiFirstArrayOrDepthSlice;
      viewDescription.Texture1DArray.MipLevels       = m_Description.m_uiMipLevelCount;
      viewDescription.Texture1DArray.MostDetailedMip = m_Description.m_uiMostDetailedMip;
    }
    break;
    case xiiGALResourceDimension::Texture2D:
    {
      if (textureDescription.m_uiSampleCount > 1U)
      {
        viewDescription.ViewDimension                           = D3D_SRV_DIMENSION_TEXTURE2DMS;
        viewDescription.Texture2DMS.UnusedField_NothingToDefine = 0U;
      }
      else
      {
        viewDescription.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE2D;
        viewDescription.Texture2D.MipLevels       = m_Description.m_uiMipLevelCount;
        viewDescription.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMip;
      }
    }
    break;
    case xiiGALResourceDimension::Texture2DArray:
    {
      if (textureDescription.m_uiSampleCount > 1U)
      {
        viewDescription.ViewDimension                    = D3D_SRV_DIMENSION_TEXTURE2DMSARRAY;
        viewDescription.Texture2DMSArray.ArraySize       = m_Description.m_uiArrayOrDepthSlicesCount;
        viewDescription.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstArrayOrDepthSlice;
      }
      else
      {
        viewDescription.ViewDimension                  = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
        viewDescription.Texture2DArray.ArraySize       = m_Description.m_uiArrayOrDepthSlicesCount;
        viewDescription.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArrayOrDepthSlice;
        viewDescription.Texture2DArray.MipLevels       = m_Description.m_uiMipLevelCount;
        viewDescription.Texture2DArray.MostDetailedMip = m_Description.m_uiMostDetailedMip;
      }
    }
    break;
    case xiiGALResourceDimension::Texture3D:
    {
      viewDescription.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE3D;
      viewDescription.Texture3D.MipLevels       = m_Description.m_uiMipLevelCount;
      viewDescription.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMip;
    }
    break;
    case xiiGALResourceDimension::TextureCube:
    {
      viewDescription.ViewDimension               = D3D_SRV_DIMENSION_TEXTURECUBE;
      viewDescription.TextureCube.MipLevels       = m_Description.m_uiMipLevelCount;
      viewDescription.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMip;
    }
    break;
    case xiiGALResourceDimension::TextureCubeArray:
    {
      viewDescription.ViewDimension                     = D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
      viewDescription.TextureCubeArray.MipLevels        = m_Description.m_uiMipLevelCount;
      viewDescription.TextureCubeArray.MostDetailedMip  = m_Description.m_uiMostDetailedMip;
      viewDescription.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArrayOrDepthSlice;
      viewDescription.TextureCubeArray.NumCubes         = m_Description.m_uiArrayOrDepthSlicesCount / 6U;
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateShaderResourceView(pTextureD3D11->GetTexture(), &viewDescription, ppShaderResourceView)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture2D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D11::CreateRTV(ID3D11RenderTargetView** ppRenderTargetView)
{
  xiiGALDeviceD3D11*  pDeviceD3D11  = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALTextureD3D11* pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(m_pDevice->GetTexture(m_Description.m_hTexture));

  D3D11_RENDER_TARGET_VIEW_DESC viewDescription = {};

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateRenderTargetView(pTextureD3D11->GetTexture(), &viewDescription, ppRenderTargetView)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture2D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D11::CreateDSV(ID3D11DepthStencilView** ppDepthStencilView)
{
  xiiGALDeviceD3D11*  pDeviceD3D11  = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALTextureD3D11* pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(m_pDevice->GetTexture(m_Description.m_hTexture));

  D3D11_DEPTH_STENCIL_VIEW_DESC viewDescription = {};

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateDepthStencilView(pTextureD3D11->GetTexture(), &viewDescription, ppDepthStencilView)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture2D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewD3D11::CreateUAV(ID3D11UnorderedAccessView** ppUnorderedAccessView)
{
  xiiGALDeviceD3D11*  pDeviceD3D11  = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALTextureD3D11* pTextureD3D11 = static_cast<xiiGALTextureD3D11*>(m_pDevice->GetTexture(m_Description.m_hTexture));

  D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription = {};

  if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateUnorderedAccessView(pTextureD3D11->GetTexture(), &viewDescription, ppUnorderedAccessView)))
  {
    xiiLog::Error("Failed to create the Direct3D11 Texture2D.");
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TextureViewD3D11);
