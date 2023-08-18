#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/DepthStencilStateVulkan.h>

xiiGALDepthStencilStateVulkan::xiiGALDepthStencilStateVulkan(const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(creationDescription)
{
}

xiiGALDepthStencilStateVulkan::~xiiGALDepthStencilStateVulkan() = default;

xiiResult xiiGALDepthStencilStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

xiiResult xiiGALDepthStencilStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_DepthStencilStateVulkan);
