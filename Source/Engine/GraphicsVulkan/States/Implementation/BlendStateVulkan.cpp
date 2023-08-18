#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/BlendStateVulkan.h>

xiiGALBlendStateVulkan::xiiGALBlendStateVulkan(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateVulkan::~xiiGALBlendStateVulkan() = default;

xiiResult xiiGALBlendStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

xiiResult xiiGALBlendStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_BlendStateVulkan);
