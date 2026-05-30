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
struct ID3D12Resource;
struct IUnknown;

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

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALCommandListPoolD3D12* GetCommandListPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALQueryPoolD3D12* GetCommandQueueQueryPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const;

  [[nodiscard]] XII_ALWAYS_INLINE xiiGALFencePoolD3D12*         GetD3D12FencePool() const { return m_pFencePool.Borrow(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDescriptorSetPoolD3D12* GetResourceDescriptorPool() const { return m_pResourceDescriptorPool.Borrow(); }

  // Internal objects retrieval.

  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocator*            GetAllocator() const { return m_Allocator.GetParent(); }
  [[nodiscard]] XII_ALWAYS_INLINE xiiD3D12MemoryAllocator* GetD3D12Allocator() const { return m_pAllocatorD3D12.Borrow(); }

  [[nodiscard]] XII_ALWAYS_INLINE ID3D12Device1* GetD3D12Device() const { return m_pD3D12Device; }
  [[nodiscard]] XII_ALWAYS_INLINE IDXGIAdapter1* GetDXGIAdapter() const { return m_pDXGIAdapter; }
  [[nodiscard]] XII_ALWAYS_INLINE IDXGIFactory4* GetDXGIFactory() const { return m_pDXGIFactory; }

  void SafeReleaseDeviceObject(IUnknown*& pObject);
  void SafeReleaseBuffer(ID3D12Resource*& pResource, xiiD3D12Allocation& allocation);
  void SafeReleaseTexture(ID3D12Resource*& pResource, xiiD3D12Allocation& allocation, bool bIsStagingTexture);

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
  class DeferredDeletionQueue;

  xiiResult      EnumerateAdapters(xiiDynamicArray<IDXGIAdapter1*>& out_adapters);
  bool           IsAdapterCompatible(IDXGIAdapter1* pAdapter, D3D_FEATURE_LEVEL minFeatureLevel, bool bPermitSoftwareAdapters);
  xiiResult      GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel, xiiDynamicArray<IDXGIAdapter1*>& out_CompatibleAdapters, bool bPermitSoftwareAdapters);
  IDXGIAdapter1* SelectBestAdapter(xiiArrayPtr<IDXGIAdapter1*> pCompatibleAdapters);
  xiiResult      SelectAdapterByIndex(xiiUInt32 uiAdapterIndex, D3D_FEATURE_LEVEL minFeatureLevel, IDXGIAdapter1** out_ppAdapter, bool bPermitSoftwareAdapter = false, bool bPreferBestIfIndexInvalid = true);

private:
  IDXGIFactory4* m_pDXGIFactory = nullptr;
  IDXGIAdapter1* m_pDXGIAdapter = nullptr;
  ID3D12Device1* m_pD3D12Device = nullptr;

  xiiUniquePtr<xiiD3D12MemoryAllocator> m_pAllocatorD3D12;
  xiiUniquePtr<DeferredDeletionQueue>   m_pDeferredDeletionQueue;

  xiiDynamicArray<xiiGALDisplayModeDescriptionD3D12> m_DisplayModes;

  xiiGALQueueInformationD3D12              m_GraphicsQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12>    m_pGraphicsCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolD3D12>       m_pGraphicsCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandListPoolD3D12> m_pGraphicsCommandListPool;

  xiiGALQueueInformationD3D12              m_ComputeQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12>    m_pComputeCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolD3D12>       m_pComputeCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandListPoolD3D12> m_pComputeCommandListPool;

  xiiGALQueueInformationD3D12              m_TransferQueueInformation;
  xiiUniquePtr<xiiGALCommandQueueD3D12>    m_pTransferCommandQueue;
  xiiUniquePtr<xiiGALQueryPoolD3D12>       m_pTransferCommandQueueQueryPool;
  xiiUniquePtr<xiiGALCommandListPoolD3D12> m_pTransferCommandListPool;

  xiiUniquePtr<xiiGALFencePoolD3D12>         m_pFencePool;
  xiiUniquePtr<xiiGALDescriptorSetPoolD3D12> m_pResourceDescriptorPool;
};

#include <GraphicsD3D12/Device/Implementation/DeviceD3D12_inl.h>
