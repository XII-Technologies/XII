#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

xiiGALRasterizerStateD3D12::xiiGALRasterizerStateD3D12(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateD3D12::~xiiGALRasterizerStateD3D12() = default;

xiiResult xiiGALRasterizerStateD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerState.FillMode              = xiiDiligentTypeConversions::GetFillMode(m_Description.m_FillMode);
  m_RasterizerState.CullMode              = xiiDiligentTypeConversions::GetCullMode(m_Description.m_CullMode);
  m_RasterizerState.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
  m_RasterizerState.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerState.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.DepthClipEnable       = m_Description.m_bDepthClipEnable;
  m_RasterizerState.AntialiasedLineEnable = m_Description.m_bAntialiasedLineEnable;

  m_RasterizerState.ScissorEnable = m_Description.m_bScissorEnable;

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_RasterizerStateD3D12);
