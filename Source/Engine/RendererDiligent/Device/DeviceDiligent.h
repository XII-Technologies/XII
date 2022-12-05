
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDiligent/RendererDiligentDLL.h>
#include <RendererFoundation/Device/Device.h>

typedef xiiGALFormatLookupEntry<Diligent::TEXTURE_FORMAT, (Diligent::TEXTURE_FORMAT)0> xiiGALFormatLookupEntryDiligent;
typedef xiiGALFormatLookupTable<xiiGALFormatLookupEntryDiligent>                       xiiGALFormatLookupTableDiligent;

class xiiGALPassDiligent;

/// \brief The Diligent device implementation of the graphics abstraction layer.
class XII_RENDERERDILIGENT_DLL xiiGALDeviceDiligent : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  xiiGALDeviceDiligent(const xiiGALDeviceCreationDescription& Description);

public:
  virtual ~xiiGALDeviceDiligent();

public:
  Diligent::RefCntAutoPtr<Diligent::IRenderDevice>&  GetDevice();
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext>& GetImmediateContext();
  Diligent::RefCntAutoPtr<Diligent::IEngineFactory>& GetFactory();

  const xiiGALFormatLookupTableDiligent& GetFormatLookupTable() const;
  const Diligent::RENDER_DEVICE_TYPE&    GetDeviceType() const;
  const xiiInt32                         GetValidationLevel() const;

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

  Diligent::IBuffer*  FindTempBuffer(xiiUInt32 uiSize);
  Diligent::ITexture* FindTempTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiGALResourceFormat::Enum format);
  void                FreeTempResources(xiiUInt64 uiFrame);

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  void FillFormatLookupTable();

  bool IsFenceReachedPlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);

  void WaitForFencePlatform(Diligent::IDeviceContext* pContext, Diligent::IQuery* pFence);

  Diligent::RENDER_DEVICE_TYPE                                       m_DeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
  Diligent::RefCntAutoPtr<Diligent::IEngineFactory>                  m_pEngineFactory;
  Diligent::RefCntAutoPtr<Diligent::IRenderDevice>                   m_pDevice;
  xiiDynamicArray<Diligent::RefCntAutoPtr<Diligent::IDeviceContext>> m_pDeviceContexts;
  xiiUInt32                                                          m_NumImmediateContexts = 0;
  Diligent::GraphicsAdapterInfo                                      m_AdapterAttribs;
  xiiDynamicArray<Diligent::DisplayModeAttribs>                      m_DisplayModes;

  xiiInt32               m_ValidationLevel = -1;
  xiiUInt32              m_AdapterId       = Diligent::DEFAULT_ADAPTER_ID;
  Diligent::ADAPTER_TYPE m_AdapterType     = Diligent::ADAPTER_TYPE_UNKNOWN;
  xiiString              m_AdapterDetailsString;

  xiiGALFormatLookupTableDiligent m_FormatLookupTable;

  xiiUniquePtr<xiiGALPassDiligent> m_pDefaultPass;

  struct UsedTempResource
  {
    XII_DECLARE_POD_TYPE();

    Diligent::IDeviceObject* m_pResource;
    xiiUInt64                m_uiFrame;
    xiiUInt32                m_uiHash;
  };

  xiiMap<xiiUInt32, xiiDynamicArray<Diligent::IDeviceObject*>, xiiCompareHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  xiiDeque<UsedTempResource, xiiLocalAllocatorWrapper>                                                                m_UsedTempResources[TempResourceType::ENUM_COUNT];

  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;

  xiiTime m_SyncTimeDiff;
  bool    m_bSyncTimeNeeded = true;
};

#include <RendererDiligent/Device/Implementation/DeviceDiligent_inl.h>
