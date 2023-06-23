
#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>

struct xiiDiligentMemoryAllocator;

using xiiGALFormatLookupEntryDiligent = xiiGALFormatLookupEntry<Diligent::TEXTURE_FORMAT, Diligent::TEX_FORMAT_UNKNOWN>;
using xiiGALFormatLookupTableDiligent = xiiGALFormatLookupTable<xiiGALFormatLookupEntryDiligent>;

class xiiPipelineBarrierDiligent;

enum class xiiResourceObjectType
{
  Buffer,
  SRVBufferView,
  UAVBufferView,
  Texture,
  SRVTextureView,
  UAVTextureView,
  ImageView,
  Framebuffer,
  Renderpass,
  PipelineState,
  Shader,
  Query,
  Swapchain,
  Sampler
};

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
  struct PendingDeletion
  {
    XII_DECLARE_POD_TYPE();

    xiiResourceObjectType                            m_ObjectType;
    Diligent::RefCntAutoPtr<Diligent::IDeviceObject> m_pObject;
  };

  Diligent::IRenderDevice*  GetDevice();
  Diligent::IDeviceContext* GetImmediateContext();
  Diligent::IEngineFactory* GetFactory();

  const xiiGALFormatLookupTableDiligent& GetFormatLookupTable() const;
  const Diligent::RENDER_DEVICE_TYPE&    GetDeviceType() const;
  const xiiInt32                         GetValidationLevel() const;

  void ReportLiveGpuObjects();

  void DeleteLater(const PendingDeletion& deletion);

  // These functions need to be implemented by a render API abstraction
protected:
  // Init and shutdown functions

  virtual xiiResult InitPlatform() override;
  virtual xiiResult ShutdownPlatform() override;

  // Pipeline and Pass functions

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

  // Misc functions

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

protected:
  friend class xiiGALCommandEncoderImplDiligent;

  struct PerFrameData
  {
    xiiHybridArray<Diligent::RefCntAutoPtr<Diligent::IFence>, 2u> m_SubmittedFences;

    xiiUInt64 m_uiFrame            = -1;
    double    m_fInvTicksPerSecond = -1.0;

    xiiMutex                  m_PendingDeletionsMutex;
    xiiDeque<PendingDeletion> m_PendingDeletions;
    xiiDeque<PendingDeletion> m_PreviousPendingDeletions;
  };

  void DeletePendingResources(xiiDeque<PendingDeletion>& pendingDeletions);

  void FillFormatLookupTable();

  bool IsFenceReachedPlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);
  void WaitForFencePlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);

  Diligent::IBuffer*  FindTempBuffer(xiiUInt32 uiSize);
  Diligent::ITexture* FindTempTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiGALResourceFormat::Enum format);
  void                FreeTempResources(xiiUInt64 uiFrame);

  Diligent::RENDER_DEVICE_TYPE                                       m_DeviceType = Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
  Diligent::RefCntAutoPtr<Diligent::IEngineFactory>                  m_pEngineFactory;
  Diligent::RefCntAutoPtr<Diligent::IRenderDevice>                   m_pDevice;
  xiiDynamicArray<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> m_pDeviceContexts;
  xiiUInt32                                                          m_uiNumImmediateContexts = 0;
  Diligent::GraphicsAdapterInfo                                      m_AdapterAttribs;
  xiiDynamicArray<Diligent::DisplayModeAttribs>                      m_DisplayModes;

  xiiInt32               m_iValidationLevel = -1;
  xiiUInt32              m_uiAdapterId      = Diligent::DEFAULT_ADAPTER_ID;
  Diligent::ADAPTER_TYPE m_AdapterType      = Diligent::ADAPTER_TYPE_UNKNOWN;
  xiiString              m_sAdapterDetailsString;

  xiiGALFormatLookupTableDiligent m_FormatLookupTable;

  xiiUniquePtr<xiiGALPassDiligent>         m_pDefaultPass;
  xiiUniquePtr<xiiPipelineBarrierDiligent> m_pPipelineBarrier;

  xiiUInt64 m_uiFrameCounter        = 1u; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  xiiUInt64 m_uiSafeFrame           = 0u;
  xiiUInt8  m_uiCurrentPerFrameData = 0u;
  xiiUInt8  m_uiNextPerFrameData    = 0u;

  PerFrameData m_PerFrameData[4];

  struct UsedTempResource
  {
    XII_DECLARE_POD_TYPE();

    Diligent::IDeviceObject* m_pResource;
    xiiUInt64                m_uiFrame;
    xiiUInt32                m_uiHash;
  };

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  xiiMap<xiiUInt32, xiiDynamicArray<Diligent::IDeviceObject*>, xiiCompareHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  xiiDeque<UsedTempResource, xiiLocalAllocatorWrapper>                                                                m_UsedTempResources[TempResourceType::ENUM_COUNT];

#if XII_ENABLED(XII_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;
#endif

  xiiTime m_SyncTimeDiff;
  bool    m_bSyncTimeNeeded = true;
};

#include <RendererDiligent/Device/Implementation/DeviceDiligent_inl.h>
