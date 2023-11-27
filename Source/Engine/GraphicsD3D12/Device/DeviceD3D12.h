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
  Diligent::IDeviceContext* GetImmediateContext();
  Diligent::IEngineFactory* GetFactory();

  const xiiGALFormatLookupTableD3D12& GetFormatLookupTable() const;

  void ReportLiveGPUObjects();

  void FlushPendingObjects();

  // These functions are implemented by a graphics API implementation.
protected:
  virtual xiiResult InitializePlatform() override;
  virtual xiiResult ShutdownPlatform() override;

  virtual void BeginPipelinePlatform(xiiStringView sName, xiiGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain) override;

  virtual xiiGALPass* BeginPassPlatform(xiiStringView sName) override;
  virtual void        EndPassPlatform(xiiGALPass* pPass) override;

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual xiiGALSwapChain* CreateSwapChainPlatform(const xiiGALSwapChainCreationDescription& description) override;
  virtual void             DestroySwapChainPlatform(xiiGALSwapChain* pSwapChain) override;

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& description) override;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState) override;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& description) override;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState) override;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& description) override;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override;

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& description) override;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader) override;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& description, const xiiGALBufferData* pInitialData = nullptr) override;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer) override;

  virtual xiiGALBufferView* CreateBufferViewPlatform(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& description) override;
  virtual void              DestroyBufferViewPlatform(xiiGALBufferView* pBufferView) override;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& description, const xiiGALTextureData* pInitialData = nullptr) override;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture) override;

  virtual xiiGALTextureView* CreateTextureViewPlatform(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& description) override;
  virtual void               DestroyTextureViewPlatform(xiiGALTextureView* pTextureView) override;

  virtual xiiGALSampler* CreateSamplerPlatform(const xiiGALSamplerCreationDescription& description) override;
  virtual void           DestroySamplerPlatform(xiiGALSampler* pSampler) override;

  virtual xiiGALInputLayout* CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description) override;
  virtual void               DestroyInputLayoutPlatform(xiiGALInputLayout* pInputLayout) override;

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& description) override;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery) override;

  virtual xiiGALFence* CreateFencePlatform(const xiiGALFenceCreationDescription& description) override;
  virtual void         DestroyFencePlatform(xiiGALFence* pFence) override;

  virtual xiiGALRenderPass* CreateRenderPassPlatform(const xiiGALRenderPassCreationDescription& description) override;
  virtual void              DestroyRenderPassPlatform(xiiGALRenderPass* pRenderPass) override;

  virtual xiiGALFramebuffer* CreateFramebufferPlatform(const xiiGALFramebufferCreationDescription& description) override;
  virtual void               DestroyFramebufferPlatform(xiiGALFramebuffer* pFramebuffer) override;

  virtual xiiGALBottomLevelAS* CreateBottomLevelASPlatform(const xiiGALBottomLevelASCreationDescription& description) override;
  virtual void                 DestroyBottomLevelASPlatform(xiiGALBottomLevelAS* pBottomLevelAS) override;

  virtual xiiGALTopLevelAS* CreateTopLevelASPlatform(const xiiGALTopLevelASCreationDescription& description) override;
  virtual void              DestroyTopLevelASPlatform(xiiGALTopLevelAS* pTopLevelAS) override;

  virtual void WaitIdlePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  void FillFormatLookupTable();

private:
  xiiGALFormatLookupTableD3D12 m_FormatLookupTable;

  Diligent::IEngineFactory*                     m_pEngineFactory = nullptr;
  Diligent::IRenderDevice*                      m_pDevice        = nullptr;
  xiiDynamicArray<Diligent::IDeviceContext*>    m_pDeviceContexts;
  xiiDynamicArray<Diligent::DisplayModeAttribs> m_DisplayModes;

  xiiUniquePtr<xiiGALPassD3D12> m_pDefaultPass;

  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;
};

#include <GraphicsD3D12/Device/Implementation/DeviceD3D12_inl.h>
