#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

bool IsArrayView(const xiiGALTextureCreationDescription& texDesc, const xiiGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

xiiGALRenderTargetViewDiligent::xiiGALRenderTargetViewDiligent(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) :
  xiiGALRenderTargetView(pTexture, Description), m_pRenderTargetView(nullptr), m_pDepthStencilView(nullptr), m_pUnorderedAccessView(nullptr)
{
}

xiiGALRenderTargetViewDiligent::~xiiGALRenderTargetViewDiligent() {}

xiiResult xiiGALRenderTargetViewDiligent::InitPlatform(xiiGALDevice* pDevice)
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

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  Diligent::TEXTURE_FORMAT ViewFormat = Diligent::TEX_FORMAT_UNKNOWN;

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
    xiiLog::Error("Couldn't get Diligent format for view!");
    return XII_FAILURE;
  }

  if (m_Description.m_pExisitingNativeObject)
  {
    m_bIsNativeObjectWrapper = true;

    if (bIsDepthFormat)
    {
      m_pDepthStencilView = static_cast<Diligent::ITextureView*>(m_Description.m_pExisitingNativeObject);
    }
    else
    {
      m_pRenderTargetView = static_cast<Diligent::ITextureView*>(m_Description.m_pExisitingNativeObject);
    }

    return XII_SUCCESS;
  }

  xiiGALResourceBase*                          pRes             = const_cast<xiiGALResourceBase*>(pTexture->GetParentResource());
  Diligent::RefCntAutoPtr<Diligent::ITexture>& pTextureDiligent = static_cast<xiiGALTextureDiligent*>(pRes)->GetTexture();
  const bool                                   bIsArrayView     = IsArrayView(texDesc, m_Description);

  if (bIsDepthFormat)
  {
    Diligent::TextureViewDesc DSViewDesc;
    DSViewDesc.ViewType = Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
    DSViewDesc.Format   = ViewFormat;

    if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        DSViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
        DSViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
      }
      else
      {
        DSViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        DSViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
        DSViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        DSViewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
      }
      else
      {
        DSViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        DSViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }

    DSViewDesc.Flags = Diligent::TEXTURE_VIEW_FLAG_NONE;
    // if (m_Description.m_bReadOnly)
    // DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    xiiGALTexture* pTex = const_cast<xiiGALTexture*>(pDevice->GetTexture(m_Description.m_hTexture));
    static_cast<xiiGALTextureDiligent*>(pTex)->GetTexture()->CreateView(DSViewDesc, &m_pDepthStencilView);

    if (m_pDepthStencilView == nullptr)
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
    Diligent::TextureViewDesc RTViewDesc;
    RTViewDesc.ViewType = Diligent::TEXTURE_VIEW_RENDER_TARGET;
    RTViewDesc.Format   = ViewFormat;

    if (texDesc.m_SampleCount == xiiGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        RTViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D;
        RTViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
      }
      else
      {
        RTViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        RTViewDesc.MostDetailedMip = m_Description.m_uiMipLevel;
        RTViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        RTViewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
      }
      else
      {
        RTViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        RTViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }

    xiiGALTexture* pTex = const_cast<xiiGALTexture*>(pDevice->GetTexture(m_Description.m_hTexture));
    static_cast<xiiGALTextureDiligent*>(pTex)->GetTexture()->CreateView(RTViewDesc, &m_pRenderTargetView);

    if (m_pRenderTargetView == nullptr)
    {
      xiiLog::Error("Couldn't create rendertarget view!");
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }
}

xiiResult xiiGALRenderTargetViewDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  // Only release native texture if it isn't a native texture object wrapper
  if (!m_bIsNativeObjectWrapper)
    XII_GAL_DILIGENT_RELEASE(m_pRenderTargetView);

  XII_GAL_DILIGENT_RELEASE(m_pDepthStencilView);
  XII_GAL_DILIGENT_RELEASE(m_pUnorderedAccessView);

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_RenderTargetViewDiligent);
