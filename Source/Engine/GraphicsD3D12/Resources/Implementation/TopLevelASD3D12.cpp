#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>

xiiGALTopLevelASD3D12::xiiGALTopLevelASD3D12(const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(creationDescription)
{
}

xiiGALTopLevelASD3D12::~xiiGALTopLevelASD3D12() = default;

xiiResult xiiGALTopLevelASD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::TopLevelASDesc topLevelASDescription;
  topLevelASDescription.Name                 = m_Description.m_sName.GetStartPointer();
  topLevelASDescription.MaxInstanceCount     = m_Description.m_uiMaxInstanceCount;
  topLevelASDescription.Flags                = xiiDiligentTypeConversions::GetRayTracingBuildASFlags(m_Description.m_Flags);
  topLevelASDescription.CompactedSize        = m_Description.m_uiCompactedSize;
  topLevelASDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  pDeviceD3D12->GetDevice()->CreateTLAS(topLevelASDescription, &m_pTopLevelAS);

  return m_pTopLevelAS == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALTopLevelASD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pTopLevelAS);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TopLevelASD3D12);
