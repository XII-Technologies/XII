#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Device/DeviceD3D11.h>
#include <GraphicsD3D11/Resources/RenderPassD3D11.h>

xiiGALRenderPassD3D11::xiiGALRenderPassD3D11(const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(creationDescription)
{
}

xiiGALRenderPassD3D11::~xiiGALRenderPassD3D11() = default;

xiiResult xiiGALRenderPassD3D11::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALRenderPassD3D11::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Resources_Implementation_RenderPassD3D11);
