#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/States/DepthStencilStateD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDepthStencilStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALDepthStencilStateD3D11::xiiGALDepthStencilStateD3D11(const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(creationDescription)
{
}

xiiGALDepthStencilStateD3D11::~xiiGALDepthStencilStateD3D11() = default;

xiiResult xiiGALDepthStencilStateD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  m_DepthStencilState.DepthEnable    = D3D11_BOOL(m_Description.m_bDepthEnable);
  m_DepthStencilState.DepthWriteMask = m_Description.m_bDepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  m_DepthStencilState.DepthFunc      = xiiD3D11TypeConversions::GetD3D11ComparisonFunc(m_Description.m_ComparisonDepthFunction);

  m_DepthStencilState.StencilEnable    = D3D11_BOOL(m_Description.m_bStencilEnable);
  m_DepthStencilState.StencilReadMask  = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  m_DepthStencilState.FrontFace.StencilFailOp      = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_FrontFace.m_StencilFailOperation);
  m_DepthStencilState.FrontFace.StencilDepthFailOp = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_FrontFace.m_StencilDepthFailOperation);
  m_DepthStencilState.FrontFace.StencilPassOp      = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_FrontFace.m_StencilPassOperation);
  m_DepthStencilState.FrontFace.StencilFunc        = xiiD3D11TypeConversions::GetD3D11ComparisonFunc(m_Description.m_FrontFace.m_ComparisonFunction);

  m_DepthStencilState.BackFace.StencilFailOp      = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_BackFace.m_StencilFailOperation);
  m_DepthStencilState.BackFace.StencilDepthFailOp = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_BackFace.m_StencilDepthFailOperation);
  m_DepthStencilState.BackFace.StencilPassOp      = xiiD3D11TypeConversions::GetD3D11StencilOp(m_Description.m_BackFace.m_StencilPassOperation);
  m_DepthStencilState.BackFace.StencilFunc        = xiiD3D11TypeConversions::GetD3D11ComparisonFunc(m_Description.m_BackFace.m_ComparisonFunction);

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_DepthStencilStateD3D11);
