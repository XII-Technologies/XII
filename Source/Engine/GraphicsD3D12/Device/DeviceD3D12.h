#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsD3D12/MemoryAllocator/MemoryAllocatorD3D12.h>
#include <GraphicsFoundation/Device/Device.h>

#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>

enum D3D_FEATURE_LEVEL;

struct IDXGIAdapter1;
struct IDXGIFactory2;
struct IDXGIFactory4;
struct ID3D12Device1;
struct ID3D12Debug;

XII_DEFINE_AS_POD_TYPE(DXGI_MODE_DESC);

class XII_GRAPHICSD3D12_DLL xiiGALDeviceD3D12 final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceD3D12, xiiGALDevice);

private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceD3D12(const xiiGALDeviceCreationDescription& description);

public:
  ~xiiGALDeviceD3D12();

public:
  XII_ALWAYS_INLINE virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const override final
  {
    if (((queueType & xiiGALCommandQueueType::Graphics) == xiiGALCommandQueueType::Graphics) && m_pGraphicsCommandQueue != nullptr)
      return m_pGraphicsCommandQueue.Borrow();

    if (((queueType & xiiGALCommandQueueType::Compute) == xiiGALCommandQueueType::Compute) && m_pComputeCommandQueue != nullptr)
      return m_pComputeCommandQueue.Borrow();

    if (((queueType & xiiGALCommandQueueType::Transfer) == xiiGALCommandQueueType::Transfer) && m_pTransferCommandQueue != nullptr)
      return m_pTransferCommandQueue.Borrow();

    return bAllowGraphicsCommandQueueFallback ? GetDefaultCommandQueue(xiiGALCommandQueueType::Graphics, false) : nullptr;
  };

  // Internal objects retrieval.

  XII_ALWAYS_INLINE ID3D12Device1* GetD3D12Device() const { return m_pD3D12Device; }
  XII_ALWAYS_INLINE IDXGIAdapter1* GetDXGIAdapter() const { return m_pDXGIAdapter; }
  XII_ALWAYS_INLINE IDXGIFactory4* GetDXGIFactory() const { return m_pDXGIFactory; }

  XII_ALWAYS_INLINE xiiMemoryAllocatorD3D12* GetD3D12Allocator() const { return m_pAllocatorD3D12.Borrow(); }

  void ReportLiveGPUObjects();

  void FlushPendingObjects();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;
  virtual xiiResult ShutdownPlatform() override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiGALSwapChain*> swapchains) override final;

  virtual xiiGALSwapChain* CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual void             DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState) override final;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState) override final;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override final;

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader) override final;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr) override final;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer) override final;

  virtual xiiGALBufferView* CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description) override final;
  virtual void              DestroyBufferViewPlatform(xiiGALBufferView* pBufferView) override final;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) override final;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture) override final;

  virtual xiiGALTextureView* CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description) override final;
  virtual void               DestroyTextureViewPlatform(xiiGALTextureView* pTextureView) override final;

  virtual xiiGALSampler* CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual void           DestroySamplerPlatform(xiiGALSampler* pSampler) override final;

  virtual xiiGALInputLayout* CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description) override final;
  virtual void               DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout) override final;

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery) override final;

  virtual xiiGALFence* CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual void         DestroyFencePlatform(xiiGALFence* pFence) override final;

  virtual xiiGALRenderPass* CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual void              DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass) override final;

  virtual xiiGALFramebuffer* CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual void               DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer) override final;

  virtual xiiGALBottomLevelAS* CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual void                 DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS) override final;

  virtual xiiGALTopLevelAS* CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual void              DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS) override final;

  virtual xiiGALPipelineResourceSignature* CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual void                             DestroyPipelineResourceSignaturePlatform(xiiGALPipelineResourceSignature* pPipelineResourceSignature) override final;

  virtual xiiGALPipelineState* CreatePipelineStatePlatform(const xiiGALPipelineStateCreationDescription& description) override final;
  virtual void                 DestroyPipelineStatePlatform(xiiGALPipelineState* pPipelineState) override final;

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
  void                            GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel);
  xiiDynamicArray<IDXGIAdapter1*> GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel);

  void EnumerateDisplayModes(D3D_FEATURE_LEVEL featureLevel, IDXGIAdapter1* pDXGIAdapter, xiiUInt32 uiOutputID, xiiEnum<xiiGALTextureFormat> format, xiiDynamicArray<xiiGALDisplayModeDescription>& displayModes);

private:
  IDXGIFactory4* m_pDXGIFactory = nullptr;
  IDXGIAdapter1* m_pDXGIAdapter = nullptr;
  ID3D12Device1* m_pD3D12Device = nullptr;
  ID3D12Debug1*  m_pD3D12Debug  = nullptr;

  xiiUniquePtr<xiiMemoryAllocatorD3D12> m_pAllocatorD3D12;

  xiiDynamicArray<xiiGALDisplayModeDescription> m_DisplayModes;

  xiiUInt64 m_uiFrameCounter = 0U;

  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pGraphicsCommandQueue;
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pComputeCommandQueue;
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pTransferCommandQueue;
};
