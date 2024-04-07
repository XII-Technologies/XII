#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

xiiGALCommandQueueD3D11::xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11) :
  xiiGALCommandQueue(pDeviceD3D11)
{
}

xiiGALCommandQueueD3D11::~xiiGALCommandQueueD3D11()
{
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandQueueD3D11);
