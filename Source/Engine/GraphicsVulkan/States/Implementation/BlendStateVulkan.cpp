#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/BlendStateVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBlendStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBlendStateVulkan::xiiGALBlendStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(pDeviceVulkan, creationDescription)
{
}

xiiGALBlendStateVulkan::~xiiGALBlendStateVulkan() = default;

xiiResult xiiGALBlendStateVulkan::InitPlatform()
{
  // \note The blend state attachment count is known and set when the number of render targets are given.
  // \note The attachment count must be equal to the color attachment count in the subpass where this blend state is used.

  if (m_Description.m_RenderTargets.IsEmpty())
    return XII_SUCCESS;

  m_BlendState.pNext             = nullptr;
  m_BlendState.flags             = {};
  m_BlendState.logicOpEnable     = VK_BOOL(m_Description.m_LogicOperationEnable);
  m_BlendState.logicOp           = xiiVulkanTypeConversions::GetLogicOp(m_Description.m_LogicOperation);
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

      rtAttachmentState.blendEnable         = VK_BOOL(rtBlendState.m_bBlendEnable);
      rtAttachmentState.colorBlendOp        = xiiVulkanTypeConversions::GetBlendOp(rtBlendState.m_BlendOperation);
      rtAttachmentState.alphaBlendOp        = xiiVulkanTypeConversions::GetBlendOp(rtBlendState.m_BlendOperationAlpha);
      rtAttachmentState.srcColorBlendFactor = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState.m_SourceBlend);
      rtAttachmentState.srcAlphaBlendFactor = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState.m_SourceBlendAlpha);
      rtAttachmentState.dstColorBlendFactor = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlend);
      rtAttachmentState.dstAlphaBlendFactor = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState.m_DestinationBlendAlpha);
      rtAttachmentState.colorWriteMask      = xiiVulkanTypeConversions::GetColorWriteMask(rtBlendState.m_ColorMask);
    }
  }
  else
  {
    auto& rtBlendState0 = m_Description.m_RenderTargets[0];

    vk::PipelineColorBlendAttachmentState rtAttachmentState0 = {};
    rtAttachmentState0.blendEnable                           = VK_BOOL(rtBlendState0.m_bBlendEnable);
    rtAttachmentState0.colorBlendOp                          = xiiVulkanTypeConversions::GetBlendOp(rtBlendState0.m_BlendOperation);
    rtAttachmentState0.alphaBlendOp                          = xiiVulkanTypeConversions::GetBlendOp(rtBlendState0.m_BlendOperationAlpha);
    rtAttachmentState0.srcColorBlendFactor                   = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState0.m_SourceBlend);
    rtAttachmentState0.srcAlphaBlendFactor                   = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState0.m_SourceBlendAlpha);
    rtAttachmentState0.dstColorBlendFactor                   = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState0.m_DestinationBlend);
    rtAttachmentState0.dstAlphaBlendFactor                   = xiiVulkanTypeConversions::GetBlendFactor(rtBlendState0.m_DestinationBlendAlpha);
    rtAttachmentState0.colorWriteMask                        = xiiVulkanTypeConversions::GetColorWriteMask(rtBlendState0.m_ColorMask);

    for (xiiUInt32 uiAttachmentIndex = 0; uiAttachmentIndex < m_Description.m_RenderTargets.GetCount(); ++uiAttachmentIndex)
    {
      m_BlendAttachmentState.PushBack(rtAttachmentState0);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateVulkan::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_BlendStateVulkan);
