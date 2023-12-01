#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>

xiiGALFenceVulkan::xiiGALFenceVulkan(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(creationDescription)
{
}

xiiGALFenceVulkan::~xiiGALFenceVulkan() = default;

xiiResult xiiGALFenceVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::FenceDesc fenceDescription;
  fenceDescription.Name = m_Description.m_sName.GetStartPointer();
  fenceDescription.Type = xiiDiligentTypeConversions::GetFenceType(m_Description.m_Type);

  pDeviceVulkan->GetDevice()->CreateFence(fenceDescription, &m_pFence);

  return (m_pFence != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALFenceVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pFence);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FenceVulkan);
