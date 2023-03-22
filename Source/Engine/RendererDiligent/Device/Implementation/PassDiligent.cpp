#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/CommandEncoder/CommandEncoderImplDiligent.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Device/PassDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

XII_CHECK_AT_COMPILETIME(sizeof(xiiUInt32) == sizeof(xiiGALRenderTargetViewHandle));
namespace
{
  XII_ALWAYS_INLINE xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const xiiUInt32&>(Value);
    return Stream;
  }
} // namespace

xiiGALPassDiligent::xiiGALPassDiligent(xiiGALDevice& device) :
  xiiGALPass(device), m_GALDeviceDiligent(static_cast<xiiGALDeviceDiligent&>(device))
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplDiligent, m_GALDeviceDiligent);

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

xiiGALPassDiligent::~xiiGALPassDiligent()
{
}

xiiGALRenderCommandEncoder* xiiGALPassDiligent::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void xiiGALPassDiligent::EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

xiiGALComputeCommandEncoder* xiiGALPassDiligent::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassDiligent::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}

void xiiGALPassDiligent::MarkDirty()
{
  m_pCommandEncoderImpl->MarkDirty();
}

void xiiGALPassDiligent::Reset()
{
  m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}


XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_PassDiligent);
