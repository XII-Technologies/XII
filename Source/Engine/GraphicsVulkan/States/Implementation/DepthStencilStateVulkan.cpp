#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALDepthStencilStateVulkan::xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(creationDescription)
{
}

xiiGALDepthStencilStateVulkan::~xiiGALDepthStencilStateVulkan() = default;

xiiResult xiiGALDepthStencilStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  m_DepthStencilState.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
  m_DepthStencilState.pNext = nullptr;
  m_DepthStencilState.flags = {};

  m_DepthStencilState.depthTestEnable       = m_Description.m_bDepthEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.depthWriteEnable      = m_Description.m_bDepthWriteEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.depthCompareOp        = xiiVulkanTypeConversions::GetVkCompareOp(m_Description.m_ComparisonDepthFunction);
  m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_DepthStencilState.minDepthBounds        = 0.0f;
  m_DepthStencilState.maxDepthBounds        = 1.0f;

  m_DepthStencilState.stencilTestEnable = m_DepthStencilState.stencilTestEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.front.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.front.writeMask   = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.front.compareOp   = xiiVulkanTypeConversions::GetVkCompareOp(m_Description.m_FrontFace.m_ComparisonFunction);
  m_DepthStencilState.front.failOp      = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_FrontFace.m_StencilFailOperation);
  m_DepthStencilState.front.passOp      = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_FrontFace.m_StencilPassOperation);
  m_DepthStencilState.front.depthFailOp = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_FrontFace.m_StencilDepthFailOperation);
  m_DepthStencilState.front.reference   = 0U;

  m_DepthStencilState.back.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.back.writeMask   = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.back.compareOp   = xiiVulkanTypeConversions::GetVkCompareOp(m_Description.m_BackFace.m_ComparisonFunction);
  m_DepthStencilState.back.failOp      = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_BackFace.m_StencilFailOperation);
  m_DepthStencilState.back.passOp      = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_BackFace.m_StencilPassOperation);
  m_DepthStencilState.back.depthFailOp = xiiVulkanTypeConversions::GetVkStencilOp(m_Description.m_BackFace.m_StencilDepthFailOperation);
  m_DepthStencilState.back.reference   = 0U;

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_DepthStencilStateVulkan);
