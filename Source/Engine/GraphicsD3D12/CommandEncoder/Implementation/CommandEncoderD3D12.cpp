#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandEncoderD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>

xiiGALCommandEncoderD3D12::xiiGALCommandEncoderD3D12(xiiGALDeviceD3D12& deviceD3D12) :
  m_GALDeviceD3D12(deviceD3D12)
{
}

xiiGALCommandEncoderD3D12::~xiiGALCommandEncoderD3D12()
{
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandEncoderD3D12);
