#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererDiligent/State/StateDiligent.h>

// Mapping tables to map xiiGAL constants to Diligent constants
#include <RendererDiligent/State/Implementation/StateDiligent_MappingTables.inl>

// Blend state

xiiGALBlendStateDiligent::xiiGALBlendStateDiligent(const xiiGALBlendStateCreationDescription& Description) :
  xiiGALBlendState(Description)
{
}

xiiGALBlendStateDiligent::~xiiGALBlendStateDiligent() {}

static Diligent::BLEND_OPERATION ToDiligentBlendOp(xiiGALBlendOp::Enum e)
{
  switch (e)
  {
    case xiiGALBlendOp::Add:
      return Diligent::BLEND_OPERATION_ADD;
    case xiiGALBlendOp::Max:
      return Diligent::BLEND_OPERATION_MAX;
    case xiiGALBlendOp::Min:
      return Diligent::BLEND_OPERATION_MIN;
    case xiiGALBlendOp::RevSubtract:
      return Diligent::BLEND_OPERATION_REV_SUBTRACT;
    case xiiGALBlendOp::Subtract:
      return Diligent::BLEND_OPERATION_SUBTRACT;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return Diligent::BLEND_OPERATION_UNDEFINED;
}

static Diligent::BLEND_FACTOR ToDiligentBlend(xiiGALBlend::Enum e)
{
  switch (e)
  {
    case xiiGALBlend::BlendFactor:
      XII_ASSERT_NOT_IMPLEMENTED;
      // If this is used, it also must be implemented in xiiGALContextDiligent::SetBlendStatePlatform
      return Diligent::BLEND_FACTOR_BLEND_FACTOR;
    case xiiGALBlend::DestAlpha:
      return Diligent::BLEND_FACTOR_DEST_ALPHA;
    case xiiGALBlend::DestColor:
      return Diligent::BLEND_FACTOR_DEST_COLOR;
    case xiiGALBlend::InvBlendFactor:
      XII_ASSERT_NOT_IMPLEMENTED;
      // If this is used, it also must be implemented in xiiGALContextDiligent::SetBlendStatePlatform
      return Diligent::BLEND_FACTOR_INV_BLEND_FACTOR;
    case xiiGALBlend::InvDestAlpha:
      return Diligent::BLEND_FACTOR_INV_DEST_ALPHA;
    case xiiGALBlend::InvDestColor:
      return Diligent::BLEND_FACTOR_INV_DEST_COLOR;
    case xiiGALBlend::InvSrcAlpha:
      return Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    case xiiGALBlend::InvSrcColor:
      return Diligent::BLEND_FACTOR_INV_SRC_COLOR;
    case xiiGALBlend::One:
      return Diligent::BLEND_FACTOR_ONE;
    case xiiGALBlend::SrcAlpha:
      return Diligent::BLEND_FACTOR_SRC_ALPHA;
    case xiiGALBlend::SrcAlphaSaturated:
      return Diligent::BLEND_FACTOR_SRC_ALPHA_SAT;
    case xiiGALBlend::SrcColor:
      return Diligent::BLEND_FACTOR_SRC_COLOR;
    case xiiGALBlend::Zero:
      return Diligent::BLEND_FACTOR_ZERO;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return Diligent::BLEND_FACTOR_UNDEFINED;
}

xiiResult xiiGALBlendStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_BlendState.AlphaToCoverageEnable  = m_Description.m_bAlphaToCoverage;
  m_BlendState.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (xiiInt32 i = 0; i < 8; ++i)
  {
    m_BlendState.RenderTargets[i].BlendEnable           = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    m_BlendState.RenderTargets[i].BlendOp               = ToDiligentBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    m_BlendState.RenderTargets[i].BlendOpAlpha          = ToDiligentBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    m_BlendState.RenderTargets[i].DestBlend             = ToDiligentBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    m_BlendState.RenderTargets[i].DestBlendAlpha        = ToDiligentBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    m_BlendState.RenderTargets[i].SrcBlend              = ToDiligentBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    m_BlendState.RenderTargets[i].SrcBlendAlpha         = ToDiligentBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    m_BlendState.RenderTargets[i].RenderTargetWriteMask = (Diligent::COLOR_MASK)(m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask & 0x0F);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}



// Depth Stencil state

xiiGALDepthStencilStateDiligent::xiiGALDepthStencilStateDiligent(const xiiGALDepthStencilStateCreationDescription& Description) :
  xiiGALDepthStencilState(Description)
{
}

xiiGALDepthStencilStateDiligent::~xiiGALDepthStencilStateDiligent() {}

xiiResult xiiGALDepthStencilStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_DepthStencilState.DepthEnable      = m_Description.m_bDepthTest;
  m_DepthStencilState.DepthFunc        = GALCompareFuncToDiligent[m_Description.m_DepthTestFunc];
  m_DepthStencilState.StencilEnable    = m_Description.m_bStencilTest;
  m_DepthStencilState.StencilReadMask  = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  m_DepthStencilState.FrontFace.StencilFailOp      = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_FailOp];
  m_DepthStencilState.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.FrontFace.StencilPassOp      = GALStencilOpTableIndexToDiligent[m_Description.m_FrontFaceStencilOp.m_PassOp];
  m_DepthStencilState.FrontFace.StencilFunc        = GALCompareFuncToDiligent[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const xiiGALStencilOpDescription& backFaceStencilOp =
    m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  m_DepthStencilState.BackFace.StencilFailOp      = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_FailOp];
  m_DepthStencilState.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.BackFace.StencilPassOp      = GALStencilOpTableIndexToDiligent[backFaceStencilOp.m_PassOp];
  m_DepthStencilState.BackFace.StencilFunc        = GALCompareFuncToDiligent[backFaceStencilOp.m_StencilFunc];

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}



// Rasterizer state

xiiGALRasterizerStateDiligent::xiiGALRasterizerStateDiligent(const xiiGALRasterizerStateCreationDescription& Description) :
  xiiGALRasterizerState(Description)
{
}

xiiGALRasterizerStateDiligent::~xiiGALRasterizerStateDiligent() {}

xiiResult xiiGALRasterizerStateDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerState.CullMode              = GALCullModeToDiligent[m_Description.m_CullMode];
  m_RasterizerState.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerState.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.DepthClipEnable       = true;
  m_RasterizerState.FillMode              = m_Description.m_bWireFrame ? Diligent::FILL_MODE_WIREFRAME : Diligent::FILL_MODE_SOLID;
  m_RasterizerState.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
  m_RasterizerState.AntialiasedLineEnable = true;
  m_RasterizerState.ScissorEnable         = m_Description.m_bScissorTest;
  m_RasterizerState.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}



// Sampler state

xiiGALSamplerStateDiligent::xiiGALSamplerStateDiligent(const xiiGALSamplerStateCreationDescription& Description) :
  xiiGALSamplerState(Description), m_pSamplerState(nullptr)
{
}

xiiGALSamplerStateDiligent::~xiiGALSamplerStateDiligent() {}

static Diligent::FILTER_TYPE ToDiligentFilter(xiiGALTextureFilterMode::Enum e)
{
  switch (e)
  {
    case xiiGALTextureFilterMode::Point:
      return Diligent::FILTER_TYPE_POINT;
    case xiiGALTextureFilterMode::Linear:
      return Diligent::FILTER_TYPE_LINEAR;
    case xiiGALTextureFilterMode::Anisotropic:
      return Diligent::FILTER_TYPE_ANISOTROPIC;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  return Diligent::FILTER_TYPE_UNKNOWN;
}

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
  samplerCreateDesc.MinFilter      = ToDiligentFilter(m_Description.m_MinFilter);
  samplerCreateDesc.MagFilter      = ToDiligentFilter(m_Description.m_MagFilter);
  samplerCreateDesc.MipFilter      = ToDiligentFilter(m_Description.m_MipFilter);
  samplerCreateDesc.MaxAnisotropy  = m_Description.m_uiMaxAnisotropy;
  samplerCreateDesc.MaxLOD         = m_Description.m_fMaxMip;
  samplerCreateDesc.MinLOD         = m_Description.m_fMinMip;
  samplerCreateDesc.MipLODBias     = m_Description.m_fMipLodBias;

  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);
  pDeviceDiligent->GetDevice()->CreateSampler(samplerCreateDesc, &m_pSamplerState);

  if (m_pSamplerState == nullptr)
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}

xiiResult xiiGALSamplerStateDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_RELEASE(m_pSamplerState);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_State_Implementation_StateDiligent);
