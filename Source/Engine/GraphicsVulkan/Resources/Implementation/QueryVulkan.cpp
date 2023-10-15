#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>

xiiGALQueryVulkan::xiiGALQueryVulkan(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(creationDescription)
{
}

xiiGALQueryVulkan::~xiiGALQueryVulkan() = default;

xiiResult xiiGALQueryVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::QueryDesc queryDescription;
  queryDescription.Name = m_Description.m_sName.GetStartPointer();
  queryDescription.Type = xiiDiligentTypeConversions::GetQueryType(m_Description.m_Type);

  pDeviceVulkan->GetDevice()->CreateQuery(queryDescription, &m_pQuery);

  return (m_pQuery != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALQueryVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pQuery);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_QueryVulkan);
