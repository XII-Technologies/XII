/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRasterizerStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALRasterizerStateD3D12::xiiGALRasterizerStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(std::move(pDeviceD3D12), creationDescription)
{
}

xiiGALRasterizerStateD3D12::~xiiGALRasterizerStateD3D12() = default;

xiiResult xiiGALRasterizerStateD3D12::InitPlatform()
{
  m_RasterizerState.FillMode              = xiiD3D12TypeConversions::GetFillMode(m_Description.m_FillMode);
  m_RasterizerState.CullMode              = xiiD3D12TypeConversions::GetCullMode(m_Description.m_CullMode);
  m_RasterizerState.FrontCounterClockwise = D3D12_BOOL(m_Description.m_bFrontCounterClockwise);
  m_RasterizerState.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerState.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.DepthClipEnable       = D3D12_BOOL(m_Description.m_bDepthClipEnable);
  m_RasterizerState.AntialiasedLineEnable = D3D12_BOOL(m_Description.m_bAntialiasedLineEnable);
  m_RasterizerState.MultisampleEnable     = D3D12_BOOL(m_Description.m_bAntialiasedLineEnable);

  // Not available in the D3D12 API currently.
  // m_RasterizerState.ScissorEnable = D3D12_BOOL(m_Description.m_bScissorEnable);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_RasterizerStateD3D12);
