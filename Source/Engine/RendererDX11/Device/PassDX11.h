
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct xiiGALCommandEncoderRenderState;
class xiiGALRenderCommandEncoder;
class xiiGALComputeCommandEncoder;

class xiiGALCommandEncoderImplDX11;

class xiiGALPassDX11 : public xiiGALPass
{
protected:
  friend class xiiGALDeviceDX11;
  friend class xiiMemoryUtils;

  xiiGALPassDX11(xiiGALDevice& device);
  virtual ~xiiGALPassDX11();

  virtual xiiGALRenderCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void                        EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(const char* szName);
  void EndPass();

private:
  xiiUniquePtr<xiiGALCommandEncoderRenderState> m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDX11>    m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
