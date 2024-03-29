#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/States/BlendStateD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBlendStateD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBlendStateD3D11::xiiGALBlendStateD3D11(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateD3D11::~xiiGALBlendStateD3D11() = default;

xiiResult xiiGALBlendStateD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  m_BlendState.AlphaToCoverageEnable  = D3D11_BOOL(m_Description.m_bAlphaToCoverage);
  m_BlendState.IndependentBlendEnable = D3D11_BOOL(m_Description.m_bIndependentBlend);

  for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
  {
    auto& rtBlendState      = m_Description.m_RenderTargets[uiAttachmentIndex];
    auto& rtAttachmentState = m_BlendState.RenderTarget[uiAttachmentIndex];

    rtAttachmentState.BlendEnable = D3D11_BOOL(rtBlendState.m_bBlendEnable);

    rtAttachmentState.SrcBlend  = xiiD3D11TypeConversions::GetD3D11BlendFactor(rtBlendState.m_SourceBlend);
    rtAttachmentState.DestBlend = xiiD3D11TypeConversions::GetD3D11BlendFactor(rtBlendState.m_DestinationBlend);
    rtAttachmentState.BlendOp   = xiiD3D11TypeConversions::GetD3D11BlendOp(rtBlendState.m_BlendOperation);

    rtAttachmentState.SrcBlendAlpha  = xiiD3D11TypeConversions::GetD3D11BlendFactor(rtBlendState.m_SourceBlendAlpha);
    rtAttachmentState.DestBlendAlpha = xiiD3D11TypeConversions::GetD3D11BlendFactor(rtBlendState.m_DestinationBlendAlpha);
    rtAttachmentState.BlendOpAlpha   = xiiD3D11TypeConversions::GetD3D11BlendOp(rtBlendState.m_BlendOperationAlpha);

    rtAttachmentState.RenderTargetWriteMask = xiiD3D11TypeConversions::GetColorWriteMask(rtBlendState.m_ColorMask);
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_States_Implementation_BlendStateD3D11);
