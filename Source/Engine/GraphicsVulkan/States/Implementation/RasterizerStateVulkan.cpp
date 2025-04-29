#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRasterizerStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALRasterizerStateVulkan::xiiGALRasterizerStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(pDeviceVulkan, creationDescription)
{
}

xiiGALRasterizerStateVulkan::~xiiGALRasterizerStateVulkan() = default;

xiiResult xiiGALRasterizerStateVulkan::InitPlatform()
{
  m_RasterizerState.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
  m_RasterizerState.pNext = nullptr;
  m_RasterizerState.flags = {};

  m_RasterizerState.depthClampEnable        = VK_BOOL(m_Description.m_bDepthClipEnable);
  m_RasterizerState.rasterizerDiscardEnable = vk::False;
  m_RasterizerState.polygonMode             = xiiVulkanTypeConversions::GetPolygonMode(m_Description.m_FillMode);
  m_RasterizerState.cullMode                = xiiVulkanTypeConversions::GetCullMode(m_Description.m_CullMode);
  m_RasterizerState.frontFace               = m_Description.m_bFrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;

  m_RasterizerState.depthBiasEnable         = VK_BOOL(m_Description.m_iDepthBias != 0 || m_Description.m_fSlopeScaledDepthBias != 0.0f);
  m_RasterizerState.depthBiasConstantFactor = static_cast<float>(m_Description.m_iDepthBias);
  m_RasterizerState.depthBiasClamp          = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.depthBiasSlopeFactor    = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.lineWidth               = 1.0f;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_RasterizerStateVulkan);
