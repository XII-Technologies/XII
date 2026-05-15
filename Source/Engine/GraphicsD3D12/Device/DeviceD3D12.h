/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Device/Device.h>

enum D3D_FEATURE_LEVEL;

struct IDXGIAdapter1;
struct IDXGIFactory2;
struct IDXGIFactory4;
struct ID3D12Device1;
struct ID3D12Debug;

class xiiD3D12MemoryAllocator;

class XII_GRAPHICSD3D12_DLL xiiGALDeviceD3D12 final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceD3D12, xiiGALDevice);

private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateD3D12Device(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceD3D12(xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description);

public:
  ~xiiGALDeviceD3D12();

public:
  virtual xiiGALCommandQueue* GetCommandQueue(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const override final;

  // Internal objects retrieval.

  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocator*            GetAllocator() const { return m_Allocator.GetParent(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiD3D12MemoryAllocator* GetD3D12Allocator() const { return m_pAllocatorD3D12.Borrow(); }

  [[nodiscard]] XII_ALWAYS_INLINE ID3D12Device1* GetD3D12Device() const { return m_pD3D12Device; }
  [[nodiscard]] XII_ALWAYS_INLINE IDXGIAdapter1* GetDXGIAdapter() const { return m_pDXGIAdapter; }
  [[nodiscard]] XII_ALWAYS_INLINE IDXGIFactory4* GetDXGIFactory() const { return m_pDXGIFactory; }

  void ReportLiveGPUObjects();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;

  virtual void BeginFramePlatform() override final;
  virtual void EndFramePlatform() override final;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALCommandList>               CreateCommandListPlatform(const xiiGALCommandListCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBlendState>                CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALDepthStencilState>         CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRasterizerState>           CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALShader>                    CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBuffer>                    CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;
  virtual xiiInternal::NewInstance<xiiGALTexture>                   CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;
  virtual xiiInternal::NewInstance<xiiGALSampler>                   CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALQuery>                     CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFence>                     CreateFencePlatform(const xiiGALFenceCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRenderPass>                CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALFramebuffer>               CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALBottomLevelAS>             CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTopLevelAS>                CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALPipelineResourceSignature> CreatePipelineResourceSignaturePlatform(const xiiGALPipelineResourceSignatureCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALGraphicsPipelineState>     CreateGraphicsPipelineStatePlatform(const xiiGALGraphicsPipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALComputePipelineState>      CreateComputePipelineStatePlatform(const xiiGALComputePipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALRayTracingPipelineState>   CreateRayTracingPipelineStatePlatform(const xiiGALRayTracingPipelineStateCreationDescription& description) override final;
  virtual xiiInternal::NewInstance<xiiGALTilePipelineState>         CreateTilePipelineStatePlatform(const xiiGALTilePipelineStateCreationDescription& description) override final;

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
  void                            GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL featureLevel);
  xiiDynamicArray<IDXGIAdapter1*> GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel);

private:
  IDXGIFactory4* m_pDXGIFactory = nullptr;
  IDXGIAdapter1* m_pDXGIAdapter = nullptr;
  ID3D12Device1* m_pD3D12Device = nullptr;
  ID3D12Debug1*  m_pD3D12Debug  = nullptr;

  xiiUniquePtr<xiiD3D12MemoryAllocator> m_pAllocatorD3D12;

  xiiDynamicArray<xiiGALDisplayModeDescriptionD3D12> m_DisplayModes;

  xiiGALQueueInformationD3D12           m_GraphicsQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pGraphicsCommandQueue;

  xiiGALQueueInformationD3D12           m_ComputeQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pComputeCommandQueue;

  xiiGALQueueInformationD3D12           m_TransferQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12> m_pTransferCommandQueue;
};
