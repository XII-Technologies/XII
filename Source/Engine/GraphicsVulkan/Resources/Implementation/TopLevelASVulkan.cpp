#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>

xiiGALTopLevelASVulkan::xiiGALTopLevelASVulkan(const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(creationDescription)
{
}

xiiGALTopLevelASVulkan::~xiiGALTopLevelASVulkan() = default;

xiiResult xiiGALTopLevelASVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::TopLevelASDesc topLevelASDescription;
  topLevelASDescription.Name                 = m_Description.m_sName.GetStartPointer();
  topLevelASDescription.MaxInstanceCount     = m_Description.m_uiMaxInstanceCount;
  topLevelASDescription.Flags                = xiiDiligentTypeConversions::GetRayTracingBuildASFlags(m_Description.m_Flags);
  topLevelASDescription.CompactedSize        = m_Description.m_uiCompactedSize;
  topLevelASDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  pDeviceVulkan->GetDevice()->CreateTLAS(topLevelASDescription, &m_pTopLevelAS);

  return m_pTopLevelAS == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALTopLevelASVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pTopLevelAS);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TopLevelASVulkan);
