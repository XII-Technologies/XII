#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererDiligent/State/StateDiligent.h>

// Mapping tables to map xiiGAL constants to Diligent constants
#include <RendererDiligent/State/Implementation/StateDiligent_MappingTables.inl>

XII_CHECK_AT_COMPILETIME_MSG(XII_GAL_MAX_RENDERTARGET_COUNT == DILIGENT_MAX_RENDER_TARGETS, "The max render targets must be equal.");

// Blend State

xiiGALBlendStateDiligent::xiiGALBlendStateDiligent(const xiiGALBlendStateCreationDescription& Description) :
  xiiGALBlendState(Description)
{
}

xiiGALBlendStateDiligent::~xiiGALBlendStateDiligent() {}

xiiResult xiiGALBlendStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_BlendStateDesc.AlphaToCoverageEnable  = m_Description.m_bAlphaToCoverage;
  m_BlendStateDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (xiiUInt32 i = 0; i < XII_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    m_BlendStateDesc.RenderTargets[i].BlendEnable           = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    m_BlendStateDesc.RenderTargets[i].BlendOp               = xiiDiligentUtils::ToDiligentBlendOperation(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    m_BlendStateDesc.RenderTargets[i].BlendOpAlpha          = xiiDiligentUtils::ToDiligentBlendOperation(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    m_BlendStateDesc.RenderTargets[i].DestBlend             = xiiDiligentUtils::ToDiligentBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    m_BlendStateDesc.RenderTargets[i].DestBlendAlpha        = xiiDiligentUtils::ToDiligentBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    m_BlendStateDesc.RenderTargets[i].SrcBlend              = xiiDiligentUtils::ToDiligentBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    m_BlendStateDesc.RenderTargets[i].SrcBlendAlpha         = xiiDiligentUtils::ToDiligentBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    m_BlendStateDesc.RenderTargets[i].RenderTargetWriteMask = xiiDiligentUtils::ToDiligentColorWriteMask(m_Description.m_RenderTargetBlendDescriptions[i].m_ColorWriteMask);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

// Depth Stencil State

xiiGALDepthStencilStateDiligent::xiiGALDepthStencilStateDiligent(const xiiGALDepthStencilStateCreationDescription& Description) :
  xiiGALDepthStencilState(Description)
{
}

xiiGALDepthStencilStateDiligent::~xiiGALDepthStencilStateDiligent() {}

xiiResult xiiGALDepthStencilStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_DepthStencilStateDesc.DepthEnable      = m_Description.m_bDepthTest;
  m_DepthStencilStateDesc.DepthWriteEnable = m_Description.m_bDepthWrite;
  m_DepthStencilStateDesc.DepthFunc        = GALCompareFuncToDiligent[m_Description.m_DepthTestFunc];

  m_DepthStencilStateDesc.StencilEnable    = m_Description.m_bStencilTest;
  m_DepthStencilStateDesc.StencilReadMask  = m_Description.m_uiStencilReadMask;
  m_DepthStencilStateDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  m_DepthStencilStateDesc.FrontFace.StencilFailOp      = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_FailOp];
  m_DepthStencilStateDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  m_DepthStencilStateDesc.FrontFace.StencilPassOp      = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_PassOp];
  m_DepthStencilStateDesc.FrontFace.StencilFunc        = GALCompareFuncToDiligent[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const xiiGALStencilOpDescription& backFaceStencilOp = m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  m_DepthStencilStateDesc.BackFace.StencilFailOp      = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_FailOp];
  m_DepthStencilStateDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_DepthFailOp];
  m_DepthStencilStateDesc.BackFace.StencilPassOp      = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_PassOp];
  m_DepthStencilStateDesc.BackFace.StencilFunc        = GALCompareFuncToDiligent[backFaceStencilOp.m_StencilFunc];

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

// Rasterizer State

xiiGALRasterizerStateDiligent::xiiGALRasterizerStateDiligent(const xiiGALRasterizerStateCreationDescription& Description) :
  xiiGALRasterizerState(Description)
{
}

xiiGALRasterizerStateDiligent::~xiiGALRasterizerStateDiligent() {}

xiiResult xiiGALRasterizerStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerStateDesc.CullMode              = GALCullModeToDiligent[m_Description.m_CullMode];
  m_RasterizerStateDesc.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerStateDesc.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerStateDesc.DepthClipEnable       = m_Description.m_fDepthBiasClamp > 0.0f;
  m_RasterizerStateDesc.FillMode              = m_Description.m_bWireFrame ? Diligent::FILL_MODE::FILL_MODE_WIREFRAME : Diligent::FILL_MODE::FILL_MODE_SOLID;
  m_RasterizerStateDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
  m_RasterizerStateDesc.AntialiasedLineEnable = true;
  m_RasterizerStateDesc.ScissorEnable         = m_Description.m_bScissorTest;
  m_RasterizerStateDesc.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;

  if (m_Description.m_bConservativeRasterization)
  {
    xiiLog::Warning("Rasterizer state description enables conservative rasterization which is not available!");
  }

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

// Sampler State

xiiGALSamplerStateDiligent::xiiGALSamplerStateDiligent(const xiiGALSamplerStateCreationDescription& Description) :
  xiiGALSamplerState(Description), m_pSamplerState(nullptr)
{
}

xiiGALSamplerStateDiligent::~xiiGALSamplerStateDiligent() {}

xiiResult xiiGALSamplerStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  Diligent::SamplerDesc samplerCreateDesc = {};

  samplerCreateDesc.AddressU       = GALTextureAddressModeToDiligent[m_Description.m_AddressU];
  samplerCreateDesc.AddressV       = GALTextureAddressModeToDiligent[m_Description.m_AddressV];
  samplerCreateDesc.AddressW       = GALTextureAddressModeToDiligent[m_Description.m_AddressW];
  samplerCreateDesc.BorderColor[0] = m_Description.m_BorderColor.r;
  samplerCreateDesc.BorderColor[1] = m_Description.m_BorderColor.g;
  samplerCreateDesc.BorderColor[2] = m_Description.m_BorderColor.b;
  samplerCreateDesc.BorderColor[3] = m_Description.m_BorderColor.a;
  samplerCreateDesc.ComparisonFunc = GALCompareFuncToDiligent[m_Description.m_SampleCompareFunc];
  samplerCreateDesc.MinFilter      = xiiDiligentUtils::ToDiligentFilter(m_Description.m_MinFilter);
  samplerCreateDesc.MagFilter      = xiiDiligentUtils::ToDiligentFilter(m_Description.m_MagFilter);
  samplerCreateDesc.MipFilter      = xiiDiligentUtils::ToDiligentFilter(m_Description.m_MipFilter);
  samplerCreateDesc.MaxAnisotropy  = m_Description.m_uiMaxAnisotropy;
  samplerCreateDesc.MaxLOD         = m_Description.m_fMaxMip;
  samplerCreateDesc.MinLOD         = m_Description.m_fMinMip;
  samplerCreateDesc.MipLODBias     = m_Description.m_fMipLodBias;

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);
  pDeviceDiligent->GetDevice()->CreateSampler(samplerCreateDesc, &m_pSamplerState);

  return m_pSamplerState == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALSamplerStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pSamplerState);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_State_Implementation_StateDiligent);
