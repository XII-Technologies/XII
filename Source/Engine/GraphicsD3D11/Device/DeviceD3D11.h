#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/ResourceFormats.h>

#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>

#include <dxgiformat.h>

enum D3D_FEATURE_LEVEL;

struct IDXGIAdapter1;
struct IDXGIFactory2;
struct IDXGIFactory4;
struct ID3D11Device1;
struct ID3D11Debug;
struct ID3D11DeviceContext1;
struct DXGI_MODE_DESC;

XII_DEFINE_AS_POD_TYPE(DXGI_MODE_DESC);

using xiiGALFormatLookupEntryD3D11 = xiiGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0U>;
using xiiGALFormatLookupTableD3D11 = xiiGALFormatLookupTable<xiiGALFormatLookupEntryD3D11>;

class XII_GRAPHICSD3D11_DLL xiiGALDeviceD3D11 final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceD3D11, xiiGALDevice);

private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateD3D11Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceD3D11(const xiiGALDeviceCreationDescription& description);

public:
  ~xiiGALDeviceD3D11();

public:
  virtual xiiGALCommandQueue* GetGraphicsQueue() const override final;

  virtual xiiGALCommandQueue* GetComputeQueue() const override final;

  virtual xiiGALCommandQueue* GetTransferQueue() const override final;

  virtual xiiGALCommandQueue* GetSparseBindingQueue() const override final;

  // Internal objects retrieval.

  ID3D11Device1* GetD3D11Device() const;
  IDXGIAdapter1* GetDXGIAdapter() const;
  IDXGIFactory4* GetDXGIFactory() const;

  ID3D11DeviceContext1* GetImmediateContext();

  const xiiGALFormatLookupTableD3D11& GetFormatLookupTable() const;

  xiiUInt32 GetCommandQueueIndex(xiiBitflags<xiiGALCommandQueueType> queueType) const;

  void ReportLiveGPUObjects();

  void FlushPendingObjects();

  void ResetCommandQueuesSwapChainReferences();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult ShutdownPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  virtual void BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain) override final;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain) override final;

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform() override final;

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

  virtual void CreateCommandQueuesPlatform() override final;

  virtual void FillCapabilitiesPlatform() override final;

  void FillFormatLookupTable();

private:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  bool HasSDKLayers();
#endif

  void                            GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel);
  xiiDynamicArray<IDXGIAdapter1*> GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel);

  void EnumerateDisplayModes(D3D_FEATURE_LEVEL featureLevel, IDXGIAdapter1* pDXGIAdapter, xiiUInt32 uiOutputID, xiiEnum<xiiGALTextureFormat> format, xiiDynamicArray<xiiGALDisplayModeDescription>& displayModes);

private:
  xiiGALFormatLookupTableD3D11 m_FormatLookupTable;

  IDXGIFactory4*        m_pDXGIFactory   = nullptr;
  IDXGIAdapter1*        m_pDXGIAdapter   = nullptr;
  ID3D11Device1*        m_pDeviceD3D11   = nullptr;
  ID3D11Debug*          m_pDebugD3D11    = nullptr;
  ID3D11DeviceContext1* m_pDeviceContext = nullptr;

  xiiDynamicArray<xiiGALDisplayModeDescription> m_DisplayModes;

  xiiUInt64 m_uiFrameCounter = 0U;

  // 0 : Graphics Queue
  // 1 : Compute Queue
  // 2 : Transfer Queue
  // 3 : Sparse Queue
  xiiUniquePtr<xiiGALCommandQueueD3D11> m_CommandQueues[4];
};

#include <GraphicsD3D11/Device/Implementation/DeviceD3D11_inl.h>
