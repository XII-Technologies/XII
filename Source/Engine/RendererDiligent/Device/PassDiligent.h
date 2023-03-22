
#pragma once

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/Device/Pass.h>

struct xiiGALCommandEncoderRenderState;
class xiiGALRenderCommandEncoder;
class xiiGALComputeCommandEncoder;

class xiiGALCommandEncoderImplDiligent;

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

  void MarkDirty();
  void Reset();
private:
  xiiGALDeviceDiligent& m_GALDeviceDiligent;

  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;
};

#include <RendererDiligent/Device/Implementation/PassDiligent_inl.h>
