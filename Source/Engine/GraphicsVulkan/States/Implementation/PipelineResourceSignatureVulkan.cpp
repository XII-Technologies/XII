#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/PipelineResourceSignatureVulkan.h>

xiiGALPipelineResourceSignatureVulkan::xiiGALPipelineResourceSignatureVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceVulkan, creationDescription)
{
}

xiiGALPipelineResourceSignatureVulkan::~xiiGALPipelineResourceSignatureVulkan() = default;

xiiResult xiiGALPipelineResourceSignatureVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALPipelineResourceSignatureVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_PipelineResourceSignatureVulkan);
