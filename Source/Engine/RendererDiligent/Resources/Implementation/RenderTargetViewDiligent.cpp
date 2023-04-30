#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>

bool IsArrayView(const Diligent::RESOURCE_DIMENSION& dimension)
{
  switch (dimension)
  {
    case Diligent::RESOURCE_DIM_TEX_1D_ARRAY:
    case Diligent::RESOURCE_DIM_TEX_2D_ARRAY:
    case Diligent::RESOURCE_DIM_TEX_CUBE_ARRAY:
      return true;
  }
  return false;
}

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
  const bool bIsArrayView   = IsArrayView(pTextureDiligent->GetDesc().Type);
  ViewFormat                = bIsDepthFormat ? pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType : pDeviceDiligent->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;

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

    if (pGALTextureDiligent->GetDescription().m_bAllowDynamicMipGeneration)
      DSViewDesc.Flags |= Diligent::TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;

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
        DSViewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
      }
      else
      {
        DSViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        DSViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }

    DSViewDesc.Flags = Diligent::TEXTURE_VIEW_FLAG_NONE;

    pTextureDiligent->CreateView(DSViewDesc, &m_pDepthStencilView);

    return (m_pDepthStencilView != nullptr) ? XII_SUCCESS : XII_FAILURE;
  }
  else
  {
    Diligent::TextureViewDesc RTViewDesc;
    RTViewDesc.ViewType = Diligent::TEXTURE_VIEW_RENDER_TARGET;
    RTViewDesc.Format   = ViewFormat;

    if (pGALTextureDiligent->GetDescription().m_bAllowDynamicMipGeneration)
      RTViewDesc.Flags |= Diligent::TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION;

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
        RTViewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
      }
      else
      {
        RTViewDesc.TextureDim      = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
        RTViewDesc.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.NumArraySlices  = m_Description.m_uiSliceCount;
      }
    }

    pTextureDiligent->CreateView(RTViewDesc, &m_pRenderTargetView);

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
