#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

XII_CHECK_AT_COMPILETIME(XII_GAL_MAX_RENDERTARGET_COUNT == Diligent::MAX_RENDER_TARGETS);

xiiGALRenderTargetViewDiligent::xiiGALRenderTargetViewDiligent(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) :
  xiiGALRenderTargetView(pTexture, Description), m_pRenderTargetView(nullptr), m_pDepthStencilView(nullptr), m_pUnorderedAccessView(nullptr)
{
}

xiiGALRenderTargetViewDiligent::~xiiGALRenderTargetViewDiligent() {}

xiiResult xiiGALRenderTargetViewDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

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
  Diligent::TEXTURE_FORMAT                ViewFormat = Diligent::TEX_FORMAT_UNKNOWN;

  xiiGALTextureDiligent* pGALTextureDiligent = nullptr;
  Diligent::ITexture*    pTextureDiligent    = nullptr;
  {
    xiiGALResourceBase* pGALTexture = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
    pGALTextureDiligent             = static_cast<xiiGALTextureDiligent*>(pGALTexture);
    pTextureDiligent                = pGALTextureDiligent->GetTexture();
  }

  if (m_Description.m_OverrideViewFormat != xiiGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  const bool bIsDepthFormat = xiiGALResourceFormat::IsDepthFormat(viewFormat);
  ViewFormat                = bIsDepthFormat ? pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType : pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;

  if (ViewFormat == Diligent::TEX_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get format for view!");
    return XII_FAILURE;
  }

  Diligent::TextureViewDesc viewDesc;
  viewDesc.ViewType = bIsDepthFormat ? Diligent::TEXTURE_VIEW_DEPTH_STENCIL : Diligent::TEXTURE_VIEW_RENDER_TARGET;
  viewDesc.Format   = ViewFormat;

  switch (texDesc.m_Type)
  {
    case xiiGALTextureType::Texture1D:
    {
      viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D;
      viewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
    }
    break;
    case xiiGALTextureType::Texture1DArray:
    {
      viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_1D_ARRAY;
      viewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      viewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
      viewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
    }
    break;
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::Texture2DProxy:
    {
      if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
      {
        viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
        viewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
      }
      else
      {
        viewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
      }
    }
    break;
    case xiiGALTextureType::Texture2DArray:
    case xiiGALTextureType::Texture2DProxyArray:
    {
      if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
      {
        viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        viewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
        viewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        viewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
      }
      else
      {
        viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        viewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
        viewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
      }
    }
    break;
    case xiiGALTextureType::Texture3D:
    {
      if (!bIsDepthFormat)
      {
        viewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_3D;
        viewDesc.NumDepthSlices  = m_Description.m_uiSliceCount;
        viewDesc.FirstDepthSlice = m_Description.m_uiFirstSlice;
        viewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
      }
      else
      {
        xiiLog::Error("Depth stencil views are not supported for 3D textures.");
        return XII_FAILURE;
      }
    }
    break;
    case xiiGALTextureType::TextureCube:
    case xiiGALTextureType::TextureCubeArray:
    {
      xiiLog::Error("Unexpected render target view type '{}' for texture '{}'.", texDesc.m_Type, texDesc.m_szName);
      return XII_FAILURE;
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  viewDesc.Flags       = Diligent::TEXTURE_VIEW_FLAG_NONE;
  viewDesc.AccessFlags = Diligent::UAV_ACCESS_UNSPECIFIED;

  if (bIsDepthFormat)
  {
    pTextureDiligent->CreateView(viewDesc, &m_pDepthStencilView);

    return (m_pDepthStencilView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }
  else
  {
    pTextureDiligent->CreateView(viewDesc, &m_pRenderTargetView);

    return (m_pRenderTargetView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }
}

xiiResult xiiGALRenderTargetViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pRenderTargetView);
  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pDepthStencilView);
  XII_GAL_DILIGENT_UNWRAPPED_RELEASE(m_pUnorderedAccessView);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_RenderTargetViewDiligent);
