#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandEncoderState.h>
#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>

#include <GraphicsVulkan/CommandEncoder/CommandEncoderVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Device/PassVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>

xiiGALPassVulkan::xiiGALPassVulkan(xiiGALDevice& device) :
  xiiGALPass(device), m_GALDeviceVulkan(static_cast<xiiGALDeviceVulkan&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderGraphicsState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderVulkan, static_cast<xiiGALDeviceVulkan&>(device));

  m_pGraphicsCommandEncoder = XII_DEFAULT_NEW(xiiGALGraphicsCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder  = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pGraphicsCommandEncoder.Borrow();
}

xiiGALPassVulkan::~xiiGALPassVulkan() = default;

xiiGALGraphicsCommandEncoder* xiiGALPassVulkan::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName /* = {} */)
{
  auto pRenderPassVulkan  = static_cast<xiiGALRenderPassVulkan*>(pRenderPass);
  auto pFramebufferVulkan = static_cast<xiiGALFramebufferVulkan*>(pFramebuffer);

  m_pCommandEncoderImpl->BeginRendering(renderingSetup, pRenderPassVulkan, pFramebufferVulkan);

  return m_pGraphicsCommandEncoder.Borrow();
}

void xiiGALPassVulkan::EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pGraphicsCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");

  m_pCommandEncoderImpl->EndRendering();
}

xiiGALComputeCommandEncoder* xiiGALPassVulkan::BeginComputePlatform(xiiStringView sName /* = {} */)
{
  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassVulkan::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder.");

  m_pCommandEncoderImpl->EndCompute();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Device_Implementation_PassVulkan);
