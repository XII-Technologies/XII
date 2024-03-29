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
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

xiiResult xiiGALTopLevelASD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_TopLevelASD3D11);
