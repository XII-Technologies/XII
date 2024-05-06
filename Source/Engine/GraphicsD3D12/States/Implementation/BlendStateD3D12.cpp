#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/BlendStateD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBlendStateD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBlendStateD3D12::xiiGALBlendStateD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(pDeviceD3D12, creationDescription)
{
}

xiiGALBlendStateD3D12::~xiiGALBlendStateD3D12() = default;

xiiResult xiiGALBlendStateD3D12::InitPlatform()
{
  m_BlendState.AlphaToCoverageEnable  = D3D12_BOOL(m_Description.m_bAlphaToCoverage);
  m_BlendState.IndependentBlendEnable = D3D12_BOOL(m_Description.m_bIndependentBlend);

  for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
  {
    auto& rtBlendState      = m_Description.m_RenderTargets[uiAttachmentIndex];
    auto& rtAttachmentState = m_BlendState.RenderTarget[uiAttachmentIndex];

    rtAttachmentState.BlendEnable = D3D12_BOOL(rtBlendState.m_bBlendEnable);

    rtAttachmentState.SrcBlend  = xiiD3D12TypeConversions::GetBlendFactor(rtBlendState.m_SourceBlend);
    rtAttachmentState.DestBlend = xiiD3D12TypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlend);
    rtAttachmentState.BlendOp   = xiiD3D12TypeConversions::GetBlendOp(rtBlendState.m_BlendOperation);

    rtAttachmentState.SrcBlendAlpha  = xiiD3D12TypeConversions::GetBlendFactor(rtBlendState.m_SourceBlendAlpha);
    rtAttachmentState.DestBlendAlpha = xiiD3D12TypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlendAlpha);
    rtAttachmentState.BlendOpAlpha   = xiiD3D12TypeConversions::GetBlendOp(rtBlendState.m_BlendOperationAlpha);

    rtAttachmentState.RenderTargetWriteMask = xiiD3D12TypeConversions::GetColorWriteMask(rtBlendState.m_ColorMask);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateD3D12::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_BlendStateD3D12);
