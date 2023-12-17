#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>
#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>

#include <GraphicsNull/CommandEncoder/CommandEncoderNull.h>
#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Device/PassNull.h>
#include <GraphicsNull/Resources/FramebufferNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>

xiiGALPassNull::xiiGALPassNull(xiiGALDevice& device) :
  xiiGALPass(device), m_GALDeviceNull(static_cast<xiiGALDeviceNull&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderGraphicsState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderNull, static_cast<xiiGALDeviceNull&>(device));

  m_pGraphicsCommandEncoder = XII_DEFAULT_NEW(xiiGALGraphicsCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder  = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pGraphicsCommandEncoder.Borrow();
}

xiiGALPassNull::~xiiGALPassNull() = default;

xiiGALGraphicsCommandEncoder* xiiGALPassNull::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName /* = {} */)
{
  auto pRenderPassNull  = static_cast<xiiGALRenderPassNull*>(pRenderPass);
  auto pFramebufferNull = static_cast<xiiGALFramebufferNull*>(pFramebuffer);

  m_pCommandEncoderImpl->BeginRendering(renderingSetup, pRenderPassNull, pFramebufferNull);

  return m_pGraphicsCommandEncoder.Borrow();
}

void xiiGALPassNull::EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pGraphicsCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");

  m_pCommandEncoderImpl->EndRendering();
}

xiiGALComputeCommandEncoder* xiiGALPassNull::BeginComputePlatform(xiiStringView sName /* = {} */)
{
  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassNull::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");

  m_pCommandEncoderImpl->EndCompute();
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Device_Implementation_PassNull);
