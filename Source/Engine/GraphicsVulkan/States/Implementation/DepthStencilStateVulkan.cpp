#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>
#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDepthStencilStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALDepthStencilStateVulkan::xiiGALDepthStencilStateVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(pDeviceVulkan, creationDescription)
{
}

xiiGALDepthStencilStateVulkan::~xiiGALDepthStencilStateVulkan() = default;

xiiResult xiiGALDepthStencilStateVulkan::InitPlatform()
{
  m_DepthStencilState.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
  m_DepthStencilState.pNext = nullptr;
  m_DepthStencilState.flags = {};

  m_DepthStencilState.depthTestEnable       = VK_BOOL(m_Description.m_bDepthEnable);
  m_DepthStencilState.depthWriteEnable      = VK_BOOL(m_Description.m_bDepthWriteEnable);
  m_DepthStencilState.depthCompareOp        = xiiVulkanTypeConversions::GetCompareOp(m_Description.m_ComparisonDepthFunction);
  m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_DepthStencilState.minDepthBounds        = 0.0f;
  m_DepthStencilState.maxDepthBounds        = 1.0f;

  m_DepthStencilState.stencilTestEnable = VK_BOOL(m_DepthStencilState.stencilTestEnable);
  m_DepthStencilState.front.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.front.writeMask   = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.front.compareOp   = xiiVulkanTypeConversions::GetCompareOp(m_Description.m_FrontFace.m_ComparisonFunction);
  m_DepthStencilState.front.failOp      = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilFailOperation);
  m_DepthStencilState.front.passOp      = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilPassOperation);
  m_DepthStencilState.front.depthFailOp = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_FrontFace.m_StencilDepthFailOperation);
  m_DepthStencilState.front.reference   = 0U;

  m_DepthStencilState.back.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.back.writeMask   = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.back.compareOp   = xiiVulkanTypeConversions::GetCompareOp(m_Description.m_BackFace.m_ComparisonFunction);
  m_DepthStencilState.back.failOp      = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilFailOperation);
  m_DepthStencilState.back.passOp      = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilPassOperation);
  m_DepthStencilState.back.depthFailOp = xiiVulkanTypeConversions::GetStencilOp(m_Description.m_BackFace.m_StencilDepthFailOperation);
  m_DepthStencilState.back.reference   = 0U;

  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateVulkan::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_DepthStencilStateVulkan);
