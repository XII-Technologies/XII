
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>

struct xiiDiligentMemoryAllocator;

using xiiGALFormatLookupEntryDiligent = xiiGALFormatLookupEntry<Diligent::TEXTURE_FORMAT, Diligent::TEX_FORMAT_UNKNOWN>;
using xiiGALFormatLookupTableDiligent = xiiGALFormatLookupTable<xiiGALFormatLookupEntryDiligent>;

/// \brief The Diligent device implementation of the graphics abstraction layer.
class XII_RENDERERDILIGENT_DLL xiiGALDeviceDiligent : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceD3D11(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceD3D12(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDeviceVulkan(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);

  xiiGALDeviceDiligent(const xiiGALDeviceCreationDescription& Description, Diligent::RENDER_DEVICE_TYPE DeviceType);

public:
  virtual ~xiiGALDeviceDiligent();

public:
  Diligent::IRenderDevice*  GetDevice();
  Diligent::IDeviceContext* GetImmediateContext();
  Diligent::IEngineFactory* GetFactory();

  const xiiGALFormatLookupTableDiligent& GetFormatLookupTable() const;
  const Diligent::RENDER_DEVICE_TYPE&    GetDeviceType() const;
  const xiiInt32                         GetValidationLevel() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  virtual xiiResult InitPlatform() override;
  virtual xiiResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain) override;

  virtual xiiGALPass* BeginPassPlatform(const char* szName) override;
  virtual void        EndPassPlatform(xiiGALPass* pPass) override;


  // State creation functions

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description) override;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState) override;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description) override;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState) override;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description) override;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState) override;

  virtual xiiGALSamplerState* CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description) override;
  virtual void                DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& Description) override;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader) override;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData) override;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer) override;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) override;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture) override;

  virtual xiiGALResourceView* CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) override;
  virtual void                DestroyResourceViewPlatform(xiiGALResourceView* pResourceView) override;

  virtual xiiGALRenderTargetView* CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) override;
  virtual void                    DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView) override;

  xiiGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void               DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& Description) override;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery) override;

  virtual xiiGALVertexDeclaration* CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description) override;
  virtual void                     DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual xiiGALTimestampHandle GetTimestampPlatform() override;
  virtual xiiResult             GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result) override;

  // Swap chain functions

  // Misc functions

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

protected:
  friend class xiiGALCommandEncoderImplDiligent;

  void FillFormatLookupTable();

  bool IsFenceReachedPlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);

  void WaitForFencePlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);

  Diligent::RENDER_DEVICE_TYPE                  m_DeviceType = Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
  Diligent::IEngineFactory*                     m_pEngineFactory = nullptr;
  Diligent::IRenderDevice*                      m_pDevice = nullptr;
  xiiDynamicArray<Diligent::IDeviceContext*>    m_pDeviceContexts;
  xiiUInt32                                     m_uiNumImmediateContexts = 0;
  Diligent::GraphicsAdapterInfo                 m_AdapterAttribs;
  xiiDynamicArray<Diligent::DisplayModeAttribs> m_DisplayModes;

  xiiInt32               m_iValidationLevel = -1;
  xiiUInt32              m_uiAdapterId      = Diligent::DEFAULT_ADAPTER_ID;
  Diligent::ADAPTER_TYPE m_AdapterType      = Diligent::ADAPTER_TYPE_UNKNOWN;
  xiiString              m_sAdapterDetailsString;

  xiiGALFormatLookupTableDiligent m_FormatLookupTable;

  std::unique_ptr<xiiDiligentMemoryAllocator> m_pMemoryAllocator;

  xiiUniquePtr<xiiGALPassDiligent> m_pDefaultPass;

#if XII_ENABLED(XII_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;
#endif

  xiiTime m_SyncTimeDiff;
  bool    m_bSyncTimeNeeded = true;
};

#include <RendererDiligent/Device/Implementation/DeviceDiligent_inl.h>
