#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>

#include <GraphicsNull/CommandEncoder/CommandQueueNull.h>

class XII_GRAPHICSNULL_DLL xiiGALDeviceNull final : public xiiGALDevice
{
private:
  friend class xiiMemoryUtils;

  friend xiiInternal::NewInstance<xiiGALDevice> CreateNullDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceNull(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  virtual ~xiiGALDeviceNull();

public:
  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocatorBase* GetAllocator() const { return m_Allocator.GetParent(); }

public:
  virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const override final;

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains) override final;

  virtual xiiInternal::NewInstance<xiiGALSwapChain> CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;

  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr) override final;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) override final;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineState>             CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description) override final;
  virtual void                                                      WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
  xiiUInt64 m_uiFrameNumber = 0U;

  xiiUniquePtr<xiiGALCommandQueueNull> m_pDefaultQueue;
};
