
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

class XII_RENDERERDILIGENT_DLL xiiGALPassDiligent : public xiiGALPass
{
public:
  Diligent::IRenderPass*  RequestRenderPass(const xiiGALRenderingSetup& renderingSetup);
  Diligent::IFramebuffer* RequestFrameBuffer(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup& renderTargetSetup, xiiEnum<xiiGALMSAASampleCount> out_MSAA);

protected:
  friend class xiiGALDeviceDiligent;
  friend class xiiMemoryUtils;

  xiiGALPassDiligent(xiiGALDevice& device);
  virtual ~xiiGALPassDiligent();

  virtual xiiGALRenderCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void                        EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder) override;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) override;

  void ReleaseRenderPassResources();

private:
  struct FramebufferKey
  {
    Diligent::IRenderPass*  m_pRenderPass;
    xiiGALRenderTargetSetup m_RenderTargetSetup;
  };

  struct RenderPassDesc
  {
    xiiHybridArray<Diligent::RenderPassAttachmentDesc, XII_GAL_MAX_RENDERTARGET_COUNT> m_Attachments;
  };

  struct FramebufferDesc
  {
    Diligent::FramebufferDesc                                                   m_FramebufferDesc;
    xiiHybridArray<Diligent::ITextureView*, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_Attachments;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);

    static xiiUInt32 Hash(const FramebufferKey& key);
    static bool      Equal(const FramebufferKey& a, const FramebufferKey& b);
  };

  Diligent::IRenderPass* RequestRenderPassInternal(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& desc);
  void                   GetRenderPassDesc(const xiiGALRenderingSetup& renderingSetup, RenderPassDesc& out_Desc);
  void                   GetFrameBufferDesc(Diligent::IRenderPass* pRenderPass, const xiiGALRenderTargetSetup& renderTargetSetup, FramebufferDesc out_Desc);

  xiiHashTable<xiiGALRenderingSetup, Diligent::IRenderPass*, xiiGALPassDiligent::ResourceCacheHash> m_RenderPasses;
  xiiHashTable<FramebufferKey, Diligent::IFramebuffer*, xiiGALPassDiligent::ResourceCacheHash>      m_Framebuffers;

  xiiHybridArray<Diligent::OptimizedClearValue, XII_GAL_MAX_RENDERTARGET_COUNT + 1> m_ClearValues;

  xiiUniquePtr<xiiGALCommandEncoderRenderState>  m_pCommandEncoderState;
  xiiUniquePtr<xiiGALCommandEncoderImplDiligent> m_pCommandEncoderImpl;

  xiiUniquePtr<xiiGALRenderCommandEncoder>  m_pRenderCommandEncoder;
  xiiUniquePtr<xiiGALComputeCommandEncoder> m_pComputeCommandEncoder;

  xiiGALDeviceDiligent& m_GALDeviceDiligent;
};

#include <RendererDiligent/Device/Implementation/PassDiligent_inl.h>
