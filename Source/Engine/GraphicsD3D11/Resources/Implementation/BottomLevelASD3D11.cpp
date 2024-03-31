#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/BottomLevelASD3D11.h>

xiiGALBottomLevelASD3D11::xiiGALBottomLevelASD3D11(const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(creationDescription)
{
}

xiiGALBottomLevelASD3D11::~xiiGALBottomLevelASD3D11() = default;

xiiResult xiiGALBottomLevelASD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  xiiLog::Error("xiiGALBottomLevelAS resource is unsupported in Direct3D11.");

  return XII_FAILURE;
}

xiiResult xiiGALBottomLevelASD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_BottomLevelASD3D11);
