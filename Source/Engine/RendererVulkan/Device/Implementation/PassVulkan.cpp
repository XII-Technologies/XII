#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>

xiiGALPassVulkan::xiiGALPassVulkan(xiiGALDevice& device) :
  xiiGALPass(device)
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplVulkan, static_cast<xiiGALDeviceVulkan&>(device));

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
}

xiiGALPassVulkan::~xiiGALPassVulkan() = default;

void xiiGALPassVulkan::Reset()
{
  m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}

void xiiGALPassVulkan::MarkDirty()
{
  m_pCommandEncoderImpl->MarkDirty();
}

void xiiGALPassVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, xiiPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandEncoderImpl->SetCurrentCommandBuffer(commandBuffer, pipelineBarrier);
}

xiiGALRenderCommandEncoder* xiiGALPassVulkan::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  auto& deviceVulkan = static_cast<xiiGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void xiiGALPassVulkan::EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

xiiGALComputeCommandEncoder* xiiGALPassVulkan::BeginComputePlatform(const char* szName)
{
  auto& deviceVulkan = static_cast<xiiGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassVulkan::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}
