#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALRasterizerStateVulkan::xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateVulkan::~xiiGALRasterizerStateVulkan() = default;

xiiResult xiiGALRasterizerStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerState.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
  m_RasterizerState.pNext = nullptr;
  m_RasterizerState.flags = {};

  m_RasterizerState.depthClampEnable        = VK_BOOL(m_Description.m_bDepthClipEnable);
  m_RasterizerState.rasterizerDiscardEnable = VK_FALSE;
  m_RasterizerState.polygonMode             = xiiVulkanTypeConversions::GetVkPolygonMode(m_Description.m_FillMode);
  m_RasterizerState.cullMode                = xiiVulkanTypeConversions::GetVkCullMode(m_Description.m_CullMode);
  m_RasterizerState.frontFace               = m_Description.m_bFrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;

  m_RasterizerState.depthBiasEnable         = VK_BOOL(m_Description.m_iDepthBias != 0 || m_Description.m_fSlopeScaledDepthBias != 0.0f);
  m_RasterizerState.depthBiasConstantFactor = static_cast<float>(m_Description.m_iDepthBias);
  m_RasterizerState.depthBiasClamp          = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.depthBiasSlopeFactor    = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.lineWidth               = 1.0f;

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_RasterizerStateVulkan);
