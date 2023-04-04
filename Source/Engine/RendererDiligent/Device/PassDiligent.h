
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

class XII_RENDERERDILIGENT_DLL xiiGALPassDiligent : public xiiGALPass
{
protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALPassDiligent(xiiGALDevice& device);
  virtual ~xiiGALPassDiligent();

  virtual xiiGALRenderCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void                        EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

private:
  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
