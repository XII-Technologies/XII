#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/BlendStateVulkan.h>

xiiGALBlendStateVulkan::xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateVulkan::~xiiGALBlendStateVulkan() = default;

xiiResult xiiGALBlendStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  m_BlendState.AlphaToCoverageEnable  = m_Description.m_bAlphaToCoverage;
  m_BlendState.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
  {
    auto& rtBlendState      = m_Description.m_RenderTargets[uiAttachmentIndex];
    auto& rtAttachmentState = m_BlendState.RenderTargets[uiAttachmentIndex];

    rtAttachmentState.BlendEnable = rtBlendState.m_bBlendEnable;

    rtAttachmentState.SrcBlend  = xiiDiligentTypeConversions::GetBlendFactor(rtBlendState.m_SourceBlend);
    rtAttachmentState.DestBlend = xiiDiligentTypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlend);
    rtAttachmentState.BlendOp   = xiiDiligentTypeConversions::GetBlendOp(rtBlendState.m_BlendOperation);

    rtAttachmentState.SrcBlendAlpha  = xiiDiligentTypeConversions::GetBlendFactor(rtBlendState.m_SourceBlendAlpha);
    rtAttachmentState.DestBlendAlpha = xiiDiligentTypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlendAlpha);
    rtAttachmentState.BlendOpAlpha   = xiiDiligentTypeConversions::GetBlendOp(rtBlendState.m_BlendOperationAlpha);

    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Red))
      rtAttachmentState.RenderTargetWriteMask |= Diligent::COLOR_MASK_RED;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Green))
      rtAttachmentState.RenderTargetWriteMask |= Diligent::COLOR_MASK_GREEN;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Blue))
      rtAttachmentState.RenderTargetWriteMask |= Diligent::COLOR_MASK_BLUE;
    if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Alpha))
      rtAttachmentState.RenderTargetWriteMask |= Diligent::COLOR_MASK_ALPHA;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_BlendStateVulkan);
