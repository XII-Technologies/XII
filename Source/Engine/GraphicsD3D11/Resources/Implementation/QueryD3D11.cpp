#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/QueryD3D11.h>

xiiGALQueryD3D11::xiiGALQueryD3D11(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(creationDescription)
{
}

xiiGALQueryD3D11::~xiiGALQueryD3D11() = default;

xiiResult xiiGALQueryD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  Diligent::QueryDesc queryDescription;
  queryDescription.Name = m_Description.m_sName.GetStartPointer();
  queryDescription.Type = xiiDiligentTypeConversions::GetQueryType(m_Description.m_Type);

  pDeviceD3D11->GetDevice()->CreateQuery(queryDescription, &m_pQuery);

  return (m_pQuery != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALQueryD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_D3D11_RELEASE(m_pQuery);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_QueryD3D11);
