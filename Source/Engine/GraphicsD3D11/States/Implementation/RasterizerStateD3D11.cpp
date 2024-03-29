#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/States/RasterizerStateD3D11.h>

#include <GraphicsD3D11/Utilities/D3D11TypeConversions.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRasterizerStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALRasterizerStateD3D11::xiiGALRasterizerStateD3D11(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateD3D11::~xiiGALRasterizerStateD3D11() = default;

xiiResult xiiGALRasterizerStateD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerState.FillMode              = xiiD3D11TypeConversions::GetFillMode(m_Description.m_FillMode);
  m_RasterizerState.CullMode              = xiiD3D11TypeConversions::GetCullMode(m_Description.m_CullMode);
  m_RasterizerState.FrontCounterClockwise = D3D11_BOOL(m_Description.m_bFrontCounterClockwise);
  m_RasterizerState.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerState.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.DepthClipEnable       = D3D11_BOOL(m_Description.m_bDepthClipEnable);
  m_RasterizerState.AntialiasedLineEnable = D3D11_BOOL(m_Description.m_bAntialiasedLineEnable);
  m_RasterizerState.MultisampleEnable     = m_Description.m_bAntialiasedLineEnable;

  // Not available in the D3D11 API currently.
  // m_RasterizerState.ScissorEnable = D3D11_BOOL(m_Description.m_bScissorEnable);

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_RasterizerStateD3D11);
