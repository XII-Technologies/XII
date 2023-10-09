#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/QueryD3D12.h>

xiiGALQueryD3D12::xiiGALQueryD3D12(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(creationDescription)
{
}

xiiGALQueryD3D12::~xiiGALQueryD3D12() = default;

xiiResult xiiGALQueryD3D12::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(pDevice);

  Diligent::QueryDesc queryDescription;
  queryDescription.Name = m_Description.m_sName.GetStartPointer();
  queryDescription.Type = xiiDiligentTypeConversions::GetQueryType(m_Description.m_Type);

  pDeviceD3D12->GetDevice()->CreateQuery(queryDescription, &m_pQuery);

  return (m_pQuery != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALQueryD3D12::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pQuery);

  return XII_SUCCESS;
}
