#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Device/Device.h>

#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>

#include <dxgiformat.h>

enum D3D_FEATURE_LEVEL;

struct IDXGIAdapter4;
struct IDXGIFactory5;
struct ID3D11Device5;
struct ID3D11Debug;
struct ID3D11DeviceContext4;
struct DXGI_MODE_DESC;

XII_DEFINE_AS_POD_TYPE(DXGI_MODE_DESC);

class XII_GRAPHICSD3D11_DLL xiiGALDeviceD3D11 final : public xiiGALDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceD3D11, xiiGALDevice);

private:
  friend class xiiMemoryUtils;

  friend xiiInternal::NewInstance<xiiGALDevice> CreateD3D11Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  xiiGALDeviceD3D11(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description);

  virtual ~xiiGALDeviceD3D11();

public:
  XII_ALWAYS_INLINE virtual xiiGALCommandQueue* GetDefaultCommandQueue(xiiBitflags<xiiGALCommandQueueType> queueType, bool bAllowGraphicsCommandQueueFallback) const override final
  {
    XII_IGNORE_UNUSED(queueType);
    XII_IGNORE_UNUSED(bAllowGraphicsCommandQueueFallback);
    return m_pGraphicsCommandQueue.Borrow();
  };

  [[nodiscard]] XII_ALWAYS_INLINE xiiAllocatorBase* GetAllocator() const { return m_Allocator.GetParent(); }

  // Internal objects retrieval.

  XII_ALWAYS_INLINE ID3D11Device5* GetD3D11Device() const { return m_pDeviceD3D11; };
  XII_ALWAYS_INLINE IDXGIAdapter4* GetDXGIAdapter() const { return m_pDXGIAdapter; };
  XII_ALWAYS_INLINE IDXGIFactory5* GetDXGIFactory() const { return m_pDXGIFactory; };
  XII_ALWAYS_INLINE ID3D11DeviceContext4* GetImmediateContext() const { return m_pDeviceContext; };

  void ReportLiveGPUObjects();

  ID3D11Resource* FindTemporaryBuffer(xiiUInt32 uiSize);
  ID3D11Resource* FindTemporaryTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiEnum<xiiGALResourceFormat> format);
  void            FreeTemporaryResources(xiiUInt64 uiFrame);

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override final;
  virtual xiiResult PostInitializePlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  virtual void BeginFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains, const xiiUInt64 uiRenderFrame) override final;
  virtual void EndFramePlatform(xiiArrayPtr<xiiSharedPtr<xiiGALSwapChain>> swapchains) override final;

  virtual xiiInternal::NewInstance<xiiGALSwapChain>                 CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override final;
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

  virtual void WaitIdlePlatform() override final;

  virtual xiiResult FillCapabilitiesPlatform() override final;

private:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  bool HasSDKLayers();
#endif

  void                            GetHardwareAdapter(IDXGIFactory5* pFactory, IDXGIAdapter4** ppAdapter, D3D_FEATURE_LEVEL featureLevel);
  xiiDynamicArray<IDXGIAdapter4*> GetCompatibleAdapters(D3D_FEATURE_LEVEL minFeatureLevel);

  void EnumerateDisplayModes(D3D_FEATURE_LEVEL featureLevel, IDXGIAdapter4* pDXGIAdapter, xiiUInt32 uiOutputID, xiiEnum<xiiGALResourceFormat> format, xiiDynamicArray<xiiGALDisplayModeDescription>& displayModes);

private:
  IDXGIFactory5*        m_pDXGIFactory   = nullptr;
  IDXGIAdapter4*        m_pDXGIAdapter   = nullptr;
  ID3D11Device5*        m_pDeviceD3D11   = nullptr;
  ID3D11Debug*          m_pDebugD3D11    = nullptr;
  ID3D11DeviceContext4* m_pDeviceContext = nullptr;

  xiiDynamicArray<xiiGALDisplayModeDescription> m_DisplayModes;

  xiiUInt64 m_uiFrameCounter = 0U;

  xiiUniquePtr<xiiGALCommandQueueD3D11> m_pGraphicsCommandQueue;

  struct TemporaryResourceType
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Buffer = 0,
      Texture,

      ENUM_COUNT
    };
  };

  struct UsedTempResource
  {
    XII_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource = nullptr;
    xiiUInt64       m_uiFrame   = 0U;
    xiiUInt32       m_uiHash    = 0U;
  };

  xiiMap<xiiUInt32, xiiDynamicArray<ID3D11Resource*>, xiiCompareHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_FreeTempResources[TemporaryResourceType::ENUM_COUNT];
  xiiDeque<UsedTempResource, xiiLocalAllocatorWrapper>                                                       m_UsedTempResources[TemporaryResourceType::ENUM_COUNT];
};
