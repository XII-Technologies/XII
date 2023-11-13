#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>
#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>

#include <GraphicsD3D12/CommandEncoder/CommandEncoderD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Device/PassD3D12.h>

xiiGALPassD3D12::xiiGALPassD3D12(xiiGALDevice& device) :
  xiiGALPass(device), m_GALDeviceD3D12(static_cast<xiiGALDeviceD3D12&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderGraphicsState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderD3D12, static_cast<xiiGALDeviceD3D12&>(device));

  m_pGraphicsCommandEncoder = XII_DEFAULT_NEW(xiiGALGraphicsCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder  = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pGraphicsCommandEncoder.Borrow();
}

xiiGALPassD3D12::~xiiGALPassD3D12() = default;

xiiGALGraphicsCommandEncoder* xiiGALPassD3D12::BeginRenderingPlatform(xiiStringView sName /* = {} */)
{
  return m_pGraphicsCommandEncoder.Borrow();
}

void xiiGALPassD3D12::EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pGraphicsCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");
}

xiiGALComputeCommandEncoder* xiiGALPassD3D12::BeginComputePlatform(xiiStringView sName /* = {} */)
{
  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassD3D12::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Device_Implementation_PassD3D12);
