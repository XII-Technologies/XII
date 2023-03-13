
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/Device/Pass.h>

struct xiiGALCommandEncoderRenderState;
class xiiGALRenderCommandEncoder;
class xiiGALComputeCommandEncoder;

class xiiGALCommandEncoderImplDiligent;

class XII_RENDERERDILIGENT_DLL xiiGALPassDiligent : public xiiGALPass
{
public:
  XII_ALWAYS_INLINE Diligent::IRenderPass* GetRenderPass();

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
  void CreateRenderPass(const xiiGALRenderingSetup& renderingSetup);

  xiiGALDeviceDiligent& m_GALDeviceDiligent;

  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;

  Diligent::RenderPassDesc                       m_RenderpassDescriptor;
  Diligent::RefCntAutoPtr<Diligent::IRenderPass> m_pRenderPass;
};

#include <RendererDiligent/Device/Implementation/PassDiligent_inl.h>
