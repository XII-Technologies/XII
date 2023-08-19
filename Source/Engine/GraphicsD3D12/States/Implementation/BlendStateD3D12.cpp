#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/BlendStateD3D12.h>

#include <GraphicsD3D12/Utilities/D3D12TypeConversions.h>

xiiGALBlendStateD3D12::xiiGALBlendStateD3D12(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateD3D12::~xiiGALBlendStateD3D12() = default;

xiiResult xiiGALBlendStateD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  m_BlendState.AlphaToCoverageEnable  = m_Description.m_bAlphaToCoverage ? TRUE : FALSE;
  m_BlendState.IndependentBlendEnable = m_Description.m_bIndependentBlend ? TRUE : FALSE;

  for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
  {
    auto& rtBlendState      = m_Description.m_RenderTargets[uiAttachmentIndex];
    auto& rtAttachmentState = m_BlendState.RenderTarget[uiAttachmentIndex];

    rtAttachmentState.BlendEnable = rtBlendState.m_bBlendEnable ? TRUE : FALSE;

    rtAttachmentState.SrcBlend  = xiiD3D12TypeConversions::GetD3D12BlendFactor(rtBlendState.m_SourceBlend);
    rtAttachmentState.DestBlend = xiiD3D12TypeConversions::GetD3D12BlendFactor(rtBlendState.m_DestinationBlend);
    rtAttachmentState.BlendOp   = xiiD3D12TypeConversions::GetD3D12BlendOp(rtBlendState.m_BlendOperation);

    rtAttachmentState.SrcBlendAlpha  = xiiD3D12TypeConversions::GetD3D12BlendFactor(rtBlendState.m_SourceBlendAlpha);
    rtAttachmentState.DestBlendAlpha = xiiD3D12TypeConversions::GetD3D12BlendFactor(rtBlendState.m_DestinationBlendAlpha);
    rtAttachmentState.BlendOpAlpha   = xiiD3D12TypeConversions::GetD3D12BlendOp(rtBlendState.m_BlendOperationAlpha);

    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Red))
      rtAttachmentState.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Green))
      rtAttachmentState.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Blue))
      rtAttachmentState.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Alpha))
      rtAttachmentState.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_BlendStateD3D12);
