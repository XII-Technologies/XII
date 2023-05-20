
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

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

  xiiGALRenderCommandEncoder* BeginRenderPass();
  void                        EndRenderPass();

  void ReleaseRenderPassResources();

private:
  struct RenderPassInfo
  {
    Diligent::IRenderPass* m_pRenderPass = nullptr;
  };

  struct FramebufferInfo
  {
    Diligent::IFramebuffer* m_pFramebuffer = nullptr;
  };

  struct RenderPassDesc
  {
    xiiHybridArray<Diligent::RenderPassAttachmentDesc, XII_GAL_MAX_RENDERTARGET_COUNT> m_Attachments;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);
  };

  Diligent::IRenderPass*  RequestRenderPass(const xiiGALRenderingSetup& renderingSetup);
  Diligent::IFramebuffer* RequestFrameBuffer(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup& renderTargetSetup, xiiVec2U32 out_Size, xiiEnum<xiiGALMSAASampleCount> out_MSAA);

  Diligent::IRenderPass* RequestRenderPassInternal(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& desc);
  void                   GetRenderPassDesc(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& out_Desc);
  void                   GetFrameBufferDesc(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup, Diligent::FramebufferDesc out_Desc);

  void CreateRenderPass(const xiiGALRenderingSetup& renderingSetup, const char* szName);
  void CreateFramebuffer(const xiiGALRenderingSetup& renderingSetup, const char* szName);

  xiiHashTable<xiiGALRenderingSetup, RenderPassInfo, xiiGALPassDiligent::ResourceCacheHash>  m_RenderPasses;
  xiiHashTable<xiiGALRenderingSetup, FramebufferInfo, xiiGALPassDiligent::ResourceCacheHash> m_Framebuffers;

  xiiHybridArray<Diligent::OptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_ClearValues;

  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;

  xiiGALDeviceDiligent& m_GALDeviceDiligent;
};

#include <RendererDiligent/Device/Implementation/PassDiligent_inl.h>
