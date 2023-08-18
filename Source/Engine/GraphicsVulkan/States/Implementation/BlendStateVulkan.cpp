#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/BlendStateVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALBlendStateVulkan::xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateVulkan::~xiiGALBlendStateVulkan() = default;

xiiResult xiiGALBlendStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // \note The blend state attachment count is set when the number of render targets are known.
  // \note The attachment count must be equal to the color attachment count in the subpass where this blend state is used.

  if (m_Description.m_RenderTargets.IsEmpty())
    return XII_SUCCESS;

  m_BlendState.sType             = vk::StructureType::ePipelineColorBlendStateCreateInfo;
  m_BlendState.pNext             = nullptr;
  m_BlendState.flags             = {};
  m_BlendState.logicOpEnable     = m_Description.m_RenderTargets[0].m_LogicOperationEnable;
  m_BlendState.logicOp           = xiiVulkanTypeConversions::GetVkLogicOp(m_Description.m_RenderTargets[0].m_LogicOperation);
  m_BlendState.blendConstants[0] = 0.0f; // We use dynamic blend constants.
  m_BlendState.blendConstants[1] = 0.0f;
  m_BlendState.blendConstants[2] = 0.0f;
  m_BlendState.blendConstants[3] = 0.0f;

  if (m_Description.m_bIndependentBlend)
  {
    for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
    {
      auto& rtBlendState      = m_Description.m_RenderTargets[uiAttachmentIndex];
      auto& rtAttachmentState = m_BlendAttachmentState.ExpandAndGetRef();

      rtAttachmentState.blendEnable         = rtBlendState.m_bBlendEnable;
      rtAttachmentState.colorBlendOp        = xiiVulkanTypeConversions::GetVkBlendOp(rtBlendState.m_BlendOperation);
      rtAttachmentState.alphaBlendOp        = xiiVulkanTypeConversions::GetVkBlendOp(rtBlendState.m_BlendOperationAlpha);
      rtAttachmentState.srcColorBlendFactor = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState.m_SourceBlend);
      rtAttachmentState.srcAlphaBlendFactor = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState.m_SourceBlendAlpha);
      rtAttachmentState.dstColorBlendFactor = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState.m_DestinationBlend);
      rtAttachmentState.dstAlphaBlendFactor = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState.m_DestinationBlendAlpha);

      if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Red))
        rtAttachmentState.colorWriteMask |= vk::ColorComponentFlagBits::eR;
      if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Green))
        rtAttachmentState.colorWriteMask |= vk::ColorComponentFlagBits::eG;
      if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Blue))
        rtAttachmentState.colorWriteMask |= vk::ColorComponentFlagBits::eB;
      if (rtBlendState.m_ColorMask.IsSet(xiiGALColorMask::Alpha))
        rtAttachmentState.colorWriteMask |= vk::ColorComponentFlagBits::eA;
    }
  }
  else
  {
    auto& rtBlendState0 = m_Description.m_RenderTargets[0];

    vk::PipelineColorBlendAttachmentState rtAttachmentState0 = {};
    rtAttachmentState0.blendEnable                           = rtBlendState0.m_bBlendEnable;
    rtAttachmentState0.colorBlendOp                          = xiiVulkanTypeConversions::GetVkBlendOp(rtBlendState0.m_BlendOperation);
    rtAttachmentState0.alphaBlendOp                          = xiiVulkanTypeConversions::GetVkBlendOp(rtBlendState0.m_BlendOperationAlpha);
    rtAttachmentState0.srcColorBlendFactor                   = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState0.m_SourceBlend);
    rtAttachmentState0.srcAlphaBlendFactor                   = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState0.m_SourceBlendAlpha);
    rtAttachmentState0.dstColorBlendFactor                   = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState0.m_DestinationBlend);
    rtAttachmentState0.dstAlphaBlendFactor                   = xiiVulkanTypeConversions::GetVkBlendFactor(rtBlendState0.m_DestinationBlendAlpha);

    if (rtBlendState0.m_ColorMask.IsSet(xiiGALColorMask::Red))
      rtAttachmentState0.colorWriteMask |= vk::ColorComponentFlagBits::eR;
    if (rtBlendState0.m_ColorMask.IsSet(xiiGALColorMask::Green))
      rtAttachmentState0.colorWriteMask |= vk::ColorComponentFlagBits::eG;
    if (rtBlendState0.m_ColorMask.IsSet(xiiGALColorMask::Blue))
      rtAttachmentState0.colorWriteMask |= vk::ColorComponentFlagBits::eB;
    if (rtBlendState0.m_ColorMask.IsSet(xiiGALColorMask::Alpha))
      rtAttachmentState0.colorWriteMask |= vk::ColorComponentFlagBits::eA;

    for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
    {
      m_BlendAttachmentState.PushBack(rtAttachmentState0);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_BlendStateVulkan);
