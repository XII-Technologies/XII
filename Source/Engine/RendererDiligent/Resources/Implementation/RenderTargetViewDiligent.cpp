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

  xiiGALResourceBase* pRes             = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
  Diligent::ITexture* pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes)->GetTexture();

  if (m_Description.m_OverrideViewFormat != xiiGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  const bool bIsDepthFormat = xiiGALResourceFormat::IsDepthFormat(viewFormat);

  if (bIsDepthFormat)
  {
    ViewFormat = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    ViewFormat = pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (ViewFormat == Diligent::TEX_FORMAT_UNKNOWN)
  {
    xiiLog::Error("Couldn't get format for view!");
    return XII_FAILURE;
  }

  if (bIsDepthFormat)
  {
    Diligent::TextureViewDesc DSViewDesc;
    DSViewDesc.ViewType = Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
    DSViewDesc.Format   = ViewFormat;

    DSViewDesc.TextureDim      = pTextureDiligent->GetDesc().Type;
    DSViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
    DSViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
    DSViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;

    DSViewDesc.Flags = Diligent::TEXTURE_VIEW_FLAG_NONE;

    xiiGALTexture* pTex = const_cast<xiiGALTexture*>(pDevice->GetTexture(m_Description.m_hTexture));
    static_cast<xiiGALTextureDiligent*>(pTex)->GetTexture()->CreateView(DSViewDesc, &m_pDepthStencilView);

    if (m_pDepthStencilView == nullptr)
    {
      xiiLog::Error("Failed to create depth stencil view!");
      return XII_FAILURE;
    }
    else
    {
      return XII_SUCCESS;
    }
  }
  else
  {
    Diligent::TextureViewDesc RTViewDesc;
    RTViewDesc.ViewType = Diligent::TEXTURE_VIEW_RENDER_TARGET;
    RTViewDesc.Format   = ViewFormat;

    RTViewDesc.TextureDim      = pTextureDiligent->GetDesc().Type;
    RTViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
    RTViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
    RTViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;

    xiiGALTexture* pTex = const_cast<xiiGALTexture*>(pDevice->GetTexture(m_Description.m_hTexture));
    static_cast<xiiGALTextureDiligent*>(pTex)->GetTexture()->CreateView(RTViewDesc, &m_pRenderTargetView);

    if (m_pRenderTargetView == nullptr)
    {
      xiiLog::Error("Failed to create rendertarget view!");
      return XII_FAILURE;
    }

    return XII_SUCCESS;
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
