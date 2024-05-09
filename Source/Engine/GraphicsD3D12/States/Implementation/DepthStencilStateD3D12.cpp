#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDepthStencilStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALDepthStencilStateD3D12::xiiGALDepthStencilStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(pDeviceD3D12, creationDescription)
{
}

xiiGALDepthStencilStateD3D12::~xiiGALDepthStencilStateD3D12() = default;

xiiResult xiiGALDepthStencilStateD3D12::InitPlatform()
{
  m_DepthStencilState.DepthEnable    = D3D12_BOOL(m_Description.m_bDepthEnable);
  m_DepthStencilState.DepthWriteMask = m_Description.m_bDepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  m_DepthStencilState.DepthFunc      = xiiD3D12TypeConversions::GetComparisonFunc(m_Description.m_ComparisonDepthFunction);

  m_DepthStencilState.StencilEnable    = D3D12_BOOL(m_Description.m_bStencilEnable);
  m_DepthStencilState.StencilReadMask  = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  m_DepthStencilState.FrontFace.StencilFailOp      = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilFailOperation);
  m_DepthStencilState.FrontFace.StencilDepthFailOp = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilDepthFailOperation);
  m_DepthStencilState.FrontFace.StencilPassOp      = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilPassOperation);
  m_DepthStencilState.FrontFace.StencilFunc        = xiiD3D12TypeConversions::GetComparisonFunc(m_Description.m_FrontFace.m_ComparisonFunction);

  m_DepthStencilState.BackFace.StencilFailOp      = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilFailOperation);
  m_DepthStencilState.BackFace.StencilDepthFailOp = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilDepthFailOperation);
  m_DepthStencilState.BackFace.StencilPassOp      = xiiD3D12TypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilPassOperation);
  m_DepthStencilState.BackFace.StencilFunc        = xiiD3D12TypeConversions::GetComparisonFunc(m_Description.m_BackFace.m_ComparisonFunction);

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_DepthStencilStateD3D12);
