#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/Pass.h>

xiiGALPass::xiiGALPass(xiiGALDevice& device) :
  m_Device(device)
{
}

xiiGALPass::~xiiGALPass() = default;

xiiGALGraphicsCommandEncoder* xiiGALPass::BeginRendering(xiiGALRenderPass* pRenderPass, xiiStringView sName)
{
  XII_ASSERT_DEV(pRenderPass != nullptr, "pRenderPass is nullptr.");
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "Command Encoder nesting is not permitted.");

  m_sName                     = sName;
  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Graphics;

  xiiGALGraphicsCommandEncoder* pCommandEncoder = BeginRenderingPlatform(pRenderPass, m_sName);

  if (!m_sName.IsEmpty())
  {
    pCommandEncoder->PushMarker(m_sName);
  }

  return pCommandEncoder;
}

void xiiGALPass::EndRendering(xiiGALGraphicsCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "BeginRendering has not been called.");

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  if (!m_sName.IsEmpty())
  {
    pCommandEncoder->PopMarker();
    m_sName = {};
  }

  EndRenderingPlatform(pCommandEncoder);
}

xiiGALComputeCommandEncoder* xiiGALPass::BeginCompute(xiiStringView sName)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "Command Encoder nesting is not permitted.");

  m_sName                     = sName;
  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Compute;

  xiiGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(m_sName);

  if (!m_sName.IsEmpty())
  {
    pCommandEncoder->PushMarker(m_sName);
  }

  return pCommandEncoder;
}

void xiiGALPass::EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_CurrentCommandEncoderType == xiiGALCommandEncoderType::Invalid, "BeginCompute has not been called.");

  m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  if (!m_sName.IsEmpty())
  {
    pCommandEncoder->PopMarker();
    m_sName = {};
  }

  EndComputePlatform(pCommandEncoder);
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_Pass);
