#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Pass.h>

xiiGALRenderCommandEncoder* xiiGALPass::BeginRendering(const xiiGALRenderingSetup& renderingSetup, const char* szName /*= ""*/)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  xiiGALRenderCommandEncoder* pCommandEncoder = BeginRenderingPlatform(renderingSetup, szName);

  m_bMarker = !xiiStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void xiiGALPass::EndRendering(xiiGALRenderCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndRenderingPlatform(pCommandEncoder);
}

xiiGALComputeCommandEncoder* xiiGALPass::BeginCompute(const char* szName /*= ""*/)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  xiiGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(szName);

  m_bMarker = !xiiStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void xiiGALPass::EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndComputePlatform(pCommandEncoder);
}

xiiGALRenderCommandEncoder* xiiGALPass::BeginRenderPass(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  xiiGALRenderCommandEncoder* pCommandEncoder = BeginRenderPassPlatform(renderingSetup, szName);

  return pCommandEncoder;
}

void xiiGALPass::EndRenderPass()
{
  EndRenderPassPlatform();
}

xiiGALPass::xiiGALPass(xiiGALDevice& device) :
  m_Device(device)
{
}

xiiGALPass::~xiiGALPass() = default;


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Pass);
