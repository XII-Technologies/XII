#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>

using xiiGALFormatLookupEntryD3D12 = xiiGALFormatLookupEntry<Diligent::TEXTURE_FORMAT, (Diligent::TEXTURE_FORMAT)0U>;
using xiiGALFormatLookupTableD3D12 = xiiGALFormatLookupTable<xiiGALFormatLookupEntryD3D12>;

class XII_GRAPHICSD3D12_DLL xiiGALDeviceD3D12 final : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceD3D12(const xiiGALDeviceCreationDescription& description);

public:
  ~xiiGALDeviceD3D12();

public:
  // Internal objects retrieval.

  Diligent::IRenderDevice*  GetDevice();
  Diligent::IEngineFactory* GetFactory();
  Diligent::IDeviceContext* GetImmediateContext();
  Diligent::IDeviceContext* GetComputeContext();
  Diligent::IDeviceContext* GetTransferContext();
  Diligent::IDeviceContext* GetSparseBindingContext();

  const xiiGALFormatLookupTableD3D12& GetFormatLookupTable() const;

  void ReportLiveGPUObjects();

  void FlushPendingObjects();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult ShutdownPlatform() override final;

  virtual void BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain) override final;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform() override final;

  virtual xiiGALSwapChain* CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual void             DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual xiiGALCommandList* CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description) override final;
  virtual void               DestroyCommandListPlatform(xiiGALCommandList* pCommandList) override final;

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

  virtual void FillCapabilitiesPlatform() override final;

  void CreateCommandQueues();

  void FillFormatLookupTable();

private:
  xiiGALFormatLookupTableD3D12 m_FormatLookupTable;

  Diligent::IEngineFactory*                     m_pEngineFactory = nullptr;
  Diligent::IRenderDevice*                      m_pDevice        = nullptr;
  xiiDynamicArray<Diligent::IDeviceContext*>    m_pDeviceContexts;
  xiiDynamicArray<Diligent::DisplayModeAttribs> m_DisplayModes;

  xiiHybridArray<Diligent::ImmediateContextCreateInfo, XII_GAL_MAX_ADAPTER_QUEUE_COUNT> m_ContextDescriptions;

  // 0 : Graphics Queue
  // 1 : Compute Queue
  // 2 : Transfer Queue
  // 3 : Sparse Queue
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_CommandQueues[4];

  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;
};

#include <GraphicsD3D12/Device/Implementation/DeviceD3D12_inl.h>
