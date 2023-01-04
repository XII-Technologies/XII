#include <RendererDiligentD3D11/RendererDiligentD3D11PCH.h>

#include <RendererDiligentD3D11/CommandEncoder/CommandEncoderImplDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/DeviceDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/PassDiligentD3D11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

xiiGALPassDiligentD3D11::xiiGALPassDiligentD3D11(xiiGALDevice& device) :
  xiiGALPass(device)
{
  m_pCommandEncoderState = XII_DEFAULT_NEW(xiiGALCommandEncoderRenderState);
  m_pCommandEncoderImpl  = XII_DEFAULT_NEW(xiiGALCommandEncoderImplDiligentD3D11, static_cast<xiiGALDeviceDiligentD3D11&>(device));

  m_pRenderCommandEncoder  = XII_DEFAULT_NEW(xiiGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = XII_DEFAULT_NEW(xiiGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

xiiGALPassDiligentD3D11::~xiiGALPassDiligentD3D11() = default;

xiiGALRenderCommandEncoder* xiiGALPassDiligentD3D11::BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void xiiGALPassDiligentD3D11::EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

xiiGALComputeCommandEncoder* xiiGALPassDiligentD3D11::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();
  return m_pComputeCommandEncoder.Borrow();
}

void xiiGALPassDiligentD3D11::EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder)
{
  XII_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void xiiGALPassDiligentD3D11::BeginPass(const char* szName)
{
}

void xiiGALPassDiligentD3D11::EndPass()
{
}
