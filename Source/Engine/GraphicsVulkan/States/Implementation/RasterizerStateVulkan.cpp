#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

xiiGALRasterizerStateVulkan::xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateVulkan::~xiiGALRasterizerStateVulkan() = default;

xiiResult xiiGALRasterizerStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

xiiResult xiiGALRasterizerStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_RasterizerStateVulkan);
