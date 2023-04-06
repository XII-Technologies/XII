#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/State/StateDX11.h>

#include <d3d11.h>
#include <d3d11_3.h>


// Mapping tables to map xiiGAL constants to DX11 constants
#include <RendererDX11/State/Implementation/StateDX11_MappingTables.inl>

// Blend state

xiiGALBlendStateDX11::xiiGALBlendStateDX11(const xiiGALBlendStateCreationDescription& Description) :
  xiiGALBlendState(Description), m_pDXBlendState(nullptr)
{
}

xiiGALBlendStateDX11::~xiiGALBlendStateDX11() {}

static D3D11_BLEND_OP ToD3DBlendOp(xiiGALBlendOperation::Enum e)
{
  switch (e)
  {
    case xiiGALBlendOperation::Add:
      return D3D11_BLEND_OP_ADD;
    case xiiGALBlendOperation::Max:
      return D3D11_BLEND_OP_MAX;
    case xiiGALBlendOperation::Min:
      return D3D11_BLEND_OP_MIN;
    case xiiGALBlendOperation::RevSubtract:
      return D3D11_BLEND_OP_REV_SUBTRACT;
    case xiiGALBlendOperation::Subtract:
      return D3D11_BLEND_OP_SUBTRACT;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_OP_ADD;
}

static D3D11_BLEND ToD3DBlend(xiiGALBlendFactor::Enum e)
{
  switch (e)
  {
    case xiiGALBlendFactor::BlendFactor:
      XII_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in xiiGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_BLEND_FACTOR;
    case xiiGALBlendFactor::DestAlpha:
      return D3D11_BLEND_DEST_ALPHA;
    case xiiGALBlendFactor::DestColor:
      return D3D11_BLEND_DEST_COLOR;
    case xiiGALBlendFactor::InvBlendFactor:
      XII_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in xiiGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_INV_BLEND_FACTOR;
    case xiiGALBlendFactor::InvDestAlpha:
      return D3D11_BLEND_INV_DEST_ALPHA;
    case xiiGALBlendFactor::InvDestColor:
      return D3D11_BLEND_INV_DEST_COLOR;
    case xiiGALBlendFactor::InvSrcAlpha:
      return D3D11_BLEND_INV_SRC_ALPHA;
    case xiiGALBlendFactor::InvSrcColor:
      return D3D11_BLEND_INV_SRC_COLOR;
    case xiiGALBlendFactor::One:
      return D3D11_BLEND_ONE;
    case xiiGALBlendFactor::SrcAlpha:
      return D3D11_BLEND_SRC_ALPHA;
    case xiiGALBlendFactor::SrcAlphaSaturated:
      return D3D11_BLEND_SRC_ALPHA_SAT;
    case xiiGALBlendFactor::SrcColor:
      return D3D11_BLEND_SRC_COLOR;
    case xiiGALBlendFactor::Zero:
      return D3D11_BLEND_ZERO;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_ONE;
}

xiiUInt8 ToD3DWriteMask(xiiGALColorWriteMask::Enum mask)
{
  return ((mask & xiiGALColorWriteMask::Red) ? D3D11_COLOR_WRITE_ENABLE_RED : 0) |
    ((mask & xiiGALColorWriteMask::Green) ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0) |
    ((mask & xiiGALColorWriteMask::Blue) ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0) |
    ((mask & xiiGALColorWriteMask::Alpha) ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0);
}

xiiResult xiiGALBlendStateDX11::InitPlatform(xiiGALDevice* pDevice)
{
  D3D11_BLEND_DESC DXDesc;
  DXDesc.AlphaToCoverageEnable  = m_Description.m_bAlphaToCoverage;
  DXDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (xiiInt32 i = 0; i < 8; ++i)
  {
    DXDesc.RenderTarget[i].BlendEnable           = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    DXDesc.RenderTarget[i].BlendOp               = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    DXDesc.RenderTarget[i].BlendOpAlpha          = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    DXDesc.RenderTarget[i].DestBlend             = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    DXDesc.RenderTarget[i].DestBlendAlpha        = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    DXDesc.RenderTarget[i].SrcBlend              = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    DXDesc.RenderTarget[i].SrcBlendAlpha         = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    DXDesc.RenderTarget[i].RenderTargetWriteMask = ToD3DWriteMask(m_Description.m_RenderTargetBlendDescriptions[i].m_ColorWriteMask);
  }

  if (FAILED(static_cast<xiiGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateBlendState(&DXDesc, &m_pDXBlendState)))
  {
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXBlendState);
  return XII_SUCCESS;
}

// Depth Stencil state

xiiGALDepthStencilStateDX11::xiiGALDepthStencilStateDX11(const xiiGALDepthStencilStateCreationDescription& Description) :
  xiiGALDepthStencilState(Description), m_pDXDepthStencilState(nullptr)
{
}

xiiGALDepthStencilStateDX11::~xiiGALDepthStencilStateDX11() {}

xiiResult xiiGALDepthStencilStateDX11::InitPlatform(xiiGALDevice* pDevice)
{
  D3D11_DEPTH_STENCIL_DESC DXDesc;
  DXDesc.DepthEnable      = m_Description.m_bDepthTest;
  DXDesc.DepthWriteMask   = m_Description.m_bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  DXDesc.DepthFunc        = GALCompareFuncToDX11[m_Description.m_DepthTestFunc];
  DXDesc.StencilEnable    = m_Description.m_bStencilTest;
  DXDesc.StencilReadMask  = m_Description.m_uiStencilReadMask;
  DXDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  DXDesc.FrontFace.StencilFailOp      = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_FailOp];
  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  DXDesc.FrontFace.StencilPassOp      = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_PassOp];
  DXDesc.FrontFace.StencilFunc        = GALCompareFuncToDX11[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const xiiGALStencilOpDescription& backFaceStencilOp =
    m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  DXDesc.BackFace.StencilFailOp      = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_FailOp];
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_DepthFailOp];
  DXDesc.BackFace.StencilPassOp      = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_PassOp];
  DXDesc.BackFace.StencilFunc        = GALCompareFuncToDX11[backFaceStencilOp.m_StencilFunc];


  if (FAILED(static_cast<xiiGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateDepthStencilState(&DXDesc, &m_pDXDepthStencilState)))
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}

xiiResult xiiGALDepthStencilStateDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXDepthStencilState);
  return XII_SUCCESS;
}


// Rasterizer state

xiiGALRasterizerStateDX11::xiiGALRasterizerStateDX11(const xiiGALRasterizerStateCreationDescription& Description) :
  xiiGALRasterizerState(Description), m_pDXRasterizerState(nullptr)
{
}

xiiGALRasterizerStateDX11::~xiiGALRasterizerStateDX11() {}



xiiResult xiiGALRasterizerStateDX11::InitPlatform(xiiGALDevice* pDevice)
{
  const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  if (NeedsStateDesc2)
  {
    D3D11_RASTERIZER_DESC2 DXDesc2;
    DXDesc2.CullMode              = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc2.DepthBias             = m_Description.m_iDepthBias;
    DXDesc2.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
    DXDesc2.DepthClipEnable       = TRUE;
    DXDesc2.FillMode              = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc2.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc2.MultisampleEnable     = TRUE;
    DXDesc2.AntialiasedLineEnable = TRUE;
    DXDesc2.ScissorEnable         = m_Description.m_bScissorTest;
    DXDesc2.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;
    DXDesc2.ConservativeRaster =
      m_Description.m_bConservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    DXDesc2.ForcedSampleCount = 0;

    if (!pDevice->GetCapabilities().m_bConservativeRasterization && m_Description.m_bConservativeRasterization)
    {
      xiiLog::Error("Rasterizer state description enables conservative rasterization which is not available!");
      return XII_FAILURE;
    }

    ID3D11RasterizerState2* pDXRasterizerState2 = nullptr;

    if (FAILED(static_cast<xiiGALDeviceDX11*>(pDevice)->GetDXDevice3()->CreateRasterizerState2(&DXDesc2, &pDXRasterizerState2)))
    {
      return XII_FAILURE;
    }
    else
    {
      m_pDXRasterizerState = pDXRasterizerState2;
      return XII_SUCCESS;
    }
  }
  else
  {
    D3D11_RASTERIZER_DESC DXDesc;
    DXDesc.CullMode              = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc.DepthBias             = m_Description.m_iDepthBias;
    DXDesc.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
    DXDesc.DepthClipEnable       = TRUE;
    DXDesc.FillMode              = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc.MultisampleEnable     = TRUE;
    DXDesc.AntialiasedLineEnable = TRUE;
    DXDesc.ScissorEnable         = m_Description.m_bScissorTest;
    DXDesc.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;

    if (FAILED(static_cast<xiiGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateRasterizerState(&DXDesc, &m_pDXRasterizerState)))
    {
      return XII_FAILURE;
    }
    else
    {
      return XII_SUCCESS;
    }
  }
}


xiiResult xiiGALRasterizerStateDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXRasterizerState);
  return XII_SUCCESS;
}


// Sampler state

xiiGALSamplerStateDX11::xiiGALSamplerStateDX11(const xiiGALSamplerStateCreationDescription& Description) :
  xiiGALSamplerState(Description), m_pDXSamplerState(nullptr)
{
}

xiiGALSamplerStateDX11::~xiiGALSamplerStateDX11() {}

/*
 */

xiiResult xiiGALSamplerStateDX11::InitPlatform(xiiGALDevice* pDevice)
{
  D3D11_SAMPLER_DESC DXDesc;
  DXDesc.AddressU       = GALTextureAddressModeToDX11[m_Description.m_AddressU];
  DXDesc.AddressV       = GALTextureAddressModeToDX11[m_Description.m_AddressV];
  DXDesc.AddressW       = GALTextureAddressModeToDX11[m_Description.m_AddressW];
  DXDesc.BorderColor[0] = m_Description.m_BorderColor.r;
  DXDesc.BorderColor[1] = m_Description.m_BorderColor.g;
  DXDesc.BorderColor[2] = m_Description.m_BorderColor.b;
  DXDesc.BorderColor[3] = m_Description.m_BorderColor.a;
  DXDesc.ComparisonFunc = GALCompareFuncToDX11[m_Description.m_SampleCompareFunc];

  if (m_Description.m_MagFilter == xiiGALTextureFilterMode::Anisotropic || m_Description.m_MinFilter == xiiGALTextureFilterMode::Anisotropic ||
      m_Description.m_MipFilter == xiiGALTextureFilterMode::Anisotropic)
  {
    if (m_Description.m_SampleCompareFunc == xiiGALCompareFunc::Never)
      DXDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    else
      DXDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
  }
  else
  {
    xiiUInt32 uiTableIndex = 0;

    if (m_Description.m_MipFilter == xiiGALTextureFilterMode::Linear)
      uiTableIndex |= 1;

    if (m_Description.m_MagFilter == xiiGALTextureFilterMode::Linear)
      uiTableIndex |= 2;

    if (m_Description.m_MinFilter == xiiGALTextureFilterMode::Linear)
      uiTableIndex |= 4;

    if (m_Description.m_SampleCompareFunc != xiiGALCompareFunc::Never)
      uiTableIndex |= 8;

    DXDesc.Filter = GALFilterTableIndexToDX11[uiTableIndex];
  }

  DXDesc.MaxAnisotropy = m_Description.m_uiMaxAnisotropy;
  DXDesc.MaxLOD        = m_Description.m_fMaxMip;
  DXDesc.MinLOD        = m_Description.m_fMinMip;
  DXDesc.MipLODBias    = m_Description.m_fMipLodBias;

  if (FAILED(static_cast<xiiGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateSamplerState(&DXDesc, &m_pDXSamplerState)))
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}


xiiResult xiiGALSamplerStateDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXSamplerState);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_State_Implementation_StateDX11);
