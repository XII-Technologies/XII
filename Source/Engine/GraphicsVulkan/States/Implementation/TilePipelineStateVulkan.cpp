/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/States/TilePipelineStateVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTilePipelineStateVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTilePipelineStateVulkan::xiiGALTilePipelineStateVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTilePipelineStateCreationDescription& creationDescription) :
  xiiGALTilePipelineState(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALTilePipelineStateVulkan::~xiiGALTilePipelineStateVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineCache));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipeline));
  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkPipelineLayout));
}

xiiResult xiiGALTilePipelineStateVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (pDeviceVulkan->GetGraphicsDeviceAdapterProperties().m_Features.m_TileShaders != xiiGALDeviceFeatureState::Enabled)
  {
    xiiLog::Error("Tile pipeline creation failed: Tile Shaders are disabled on the current Vulkan device.");
    return XII_FAILURE;
  }

  xiiLog::Error("Tile pipeline creation is not supported by the current GraphicsVulkan implementation.");

  return XII_FAILURE;
}

void xiiGALTilePipelineStateVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_vkPipeline == VK_NULL_HANDLE)
    return;

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkPipeline, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_TilePipelineStateVulkan);
