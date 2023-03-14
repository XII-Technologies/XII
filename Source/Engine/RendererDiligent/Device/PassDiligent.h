
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
public:
  XII_ALWAYS_INLINE Diligent::IRenderPass* GetRenderPass(const xiiGALRenderingSetup& renderingSetup);
  XII_ALWAYS_INLINE Diligent::IFramebuffer* GetFramebuffer(const xiiGALRenderingSetup& renderingSetup);

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
  struct RenderPassWrapper
  {
    Diligent::IRenderPass* m_pRenderPass = nullptr;
  };

  struct FramebufferWrapper
  {
    Diligent::IFramebuffer* m_pFramebuffer = nullptr;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);
  };

  void CreateRenderPass(const xiiGALRenderingSetup& renderingSetup, const char* szName);
  void CreateFramebuffer(const xiiGALRenderingSetup& renderingSetup, const char* szName);

  xiiGALDeviceDiligent& m_GALDeviceDiligent;

  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;

  xiiHashTable<xiiGALRenderingSetup, RenderPassWrapper, xiiGALPassDiligent::ResourceCacheHash>  m_RenderPasses;
  xiiHashTable<xiiGALRenderingSetup, FramebufferWrapper, xiiGALPassDiligent::ResourceCacheHash> m_Framebuffers;

  xiiHybridArray<Diligent::OptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_ClearValues;
};

#include <RendererDiligent/Device/Implementation/PassDiligent_inl.h>
