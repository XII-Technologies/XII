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

  D3D11_QUERY_DESC    queryDescription = {};
  queryDescription.Query            = xiiD3D11TypeConversions::GetQueryType(m_Description.m_Type);

  for (xiiUInt32 i = 0; i < (m_Description.m_Type == xiiGALQueryType::Duration ? 2 : 1); ++i)
  {
    if (FAILED(pDeviceD3D11->GetD3D11Device()->CreateQuery(&queryDescription, &m_pQueryD3D11[i])))
    {
      xiiLog::Error("Failed to create D3D11 query object.");
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiGALQueryD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_D3D11_RELEASE_ARRAY(m_pQueryD3D11);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_QueryD3D11);
