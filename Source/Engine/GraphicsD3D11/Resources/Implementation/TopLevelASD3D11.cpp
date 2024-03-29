#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/TopLevelASD3D11.h>

xiiGALTopLevelASD3D11::xiiGALTopLevelASD3D11(const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(creationDescription)
{
}

xiiGALTopLevelASD3D11::~xiiGALTopLevelASD3D11() = default;

xiiResult xiiGALTopLevelASD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::TopLevelASDesc topLevelASDescription;
  topLevelASDescription.Name                 = m_Description.m_sName.GetStartPointer();
  topLevelASDescription.MaxInstanceCount     = m_Description.m_uiMaxInstanceCount;
  topLevelASDescription.Flags                = xiiDiligentTypeConversions::GetRayTracingBuildASFlags(m_Description.m_Flags);
  topLevelASDescription.CompactedSize        = m_Description.m_uiCompactedSize;
  topLevelASDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  pDeviceD3D11->GetDevice()->CreateTLAS(topLevelASDescription, &m_pTopLevelAS);

  return m_pTopLevelAS == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALTopLevelASD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pTopLevelAS);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TopLevelASD3D11);
