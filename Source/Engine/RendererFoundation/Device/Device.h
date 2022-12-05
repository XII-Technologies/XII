
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class xiiColor;

/// \brief The xiiRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class XII_RENDERERFOUNDATION_DLL xiiGALDevice
{
public:
  xiiEvent<const xiiGALDeviceEvent&> m_Events;

  // Init & shutdown functions

  xiiResult Init();
  xiiResult Shutdown();

  // Pipeline & Pass functions

  void BeginPipeline(const char* szName, xiiGALSwapChainHandle hSwapChain);
  void EndPipeline(xiiGALSwapChainHandle hSwapChain);

  xiiGALPass* BeginPass(const char* szName);
  void        EndPass(xiiGALPass* pPass);

  // State creation functions

  xiiGALBlendStateHandle CreateBlendState(const xiiGALBlendStateCreationDescription& Description);
  void                   DestroyBlendState(xiiGALBlendStateHandle hBlendState);

  xiiGALDepthStencilStateHandle CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& Description);
  void                          DestroyDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState);

  xiiGALRasterizerStateHandle CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& Description);
  void                        DestroyRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);

  xiiGALSamplerStateHandle CreateSamplerState(const xiiGALSamplerStateCreationDescription& Description);
  void                     DestroySamplerState(xiiGALSamplerStateHandle hSamplerState);

  // Resource creation functions

  xiiGALShaderHandle CreateShader(const xiiGALShaderCreationDescription& Description);
  void               DestroyShader(xiiGALShaderHandle hShader);

  xiiGALBufferHandle CreateBuffer(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData = xiiArrayPtr<const xiiUInt8>());
  void               DestroyBuffer(xiiGALBufferHandle hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  xiiGALBufferHandle CreateVertexBuffer(xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, const char* szName, xiiArrayPtr<const xiiUInt8> pInitialData = xiiArrayPtr<const xiiUInt8>(), bool bDataIsMutable = false);
  xiiGALBufferHandle CreateIndexBuffer(xiiGALIndexType::Enum IndexType, xiiUInt32 uiIndexCount, const char* szName, xiiArrayPtr<const xiiUInt8> pInitialData = xiiArrayPtr<const xiiUInt8>(), bool bDataIsMutable = false);
  xiiGALBufferHandle CreateConstantBuffer(xiiUInt32 uiBufferSize, const char* szName);

  xiiGALTextureHandle CreateTexture(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData = xiiArrayPtr<xiiGALSystemMemoryDescription>());
  void                DestroyTexture(xiiGALTextureHandle hTexture);

  xiiGALTextureHandle CreateProxyTexture(xiiGALTextureHandle hParentTexture, xiiUInt32 uiSlice, const char* szName);
  void                DestroyProxyTexture(xiiGALTextureHandle hProxyTexture);

  // Resource views
  xiiGALResourceViewHandle GetDefaultResourceView(xiiGALTextureHandle hTexture);
  xiiGALResourceViewHandle GetDefaultResourceView(xiiGALBufferHandle hBuffer);

  xiiGALResourceViewHandle CreateResourceView(const xiiGALResourceViewCreationDescription& Description);
  void                     DestroyResourceView(xiiGALResourceViewHandle hResourceView);

  // Render target views
  xiiGALRenderTargetViewHandle GetDefaultRenderTargetView(xiiGALTextureHandle hTexture);

  xiiGALRenderTargetViewHandle CreateRenderTargetView(const xiiGALRenderTargetViewCreationDescription& Description);
  void                         DestroyRenderTargetView(xiiGALRenderTargetViewHandle hRenderTargetView);

  // Unordered access views
  xiiGALUnorderedAccessViewHandle CreateUnorderedAccessView(const xiiGALUnorderedAccessViewCreationDescription& Description);
  void                            DestroyUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView);


  // Other rendering creation functions

  using SwapChainFactoryFunction = xiiDelegate<xiiGALSwapChain*(xiiAllocatorBase*)>;
  xiiGALSwapChainHandle CreateSwapChain(const SwapChainFactoryFunction& func);
  xiiResult             UpdateSwapChain(xiiGALSwapChainHandle hSwapChain, xiiEnum<xiiGALPresentMode> newPresentMode);
  void                  DestroySwapChain(xiiGALSwapChainHandle hSwapChain);

  xiiGALQueryHandle CreateQuery(const xiiGALQueryCreationDescription& Description);
  void              DestroyQuery(xiiGALQueryHandle hQuery);

  xiiGALVertexDeclarationHandle CreateVertexDeclaration(const xiiGALVertexDeclarationCreationDescription& Description);
  void                          DestroyVertexDeclaration(xiiGALVertexDeclarationHandle hVertexDeclaration);

  // Timestamp functions

  xiiResult GetTimestampResult(xiiGALTimestampHandle hTimestamp, xiiTime& result);

  /// \todo Map functions to save on memcpys

  // Swap chain functions

  xiiGALTextureHandle GetBackBufferTextureFromSwapChain(xiiGALSwapChainHandle hSwapChain);


  // Misc functions

  void BeginFrame(const xiiUInt64 uiRenderFrame = 0);
  void EndFrame();

  xiiGALTimestampHandle GetTimestamp();

  const xiiGALDeviceCreationDescription* GetDescription() const;

  const xiiGALSwapChain* GetSwapChain(xiiGALSwapChainHandle hSwapChain) const;
  template <typename T>
  const T* GetSwapChain(xiiGALSwapChainHandle hSwapChain) const
  {
    return static_cast<const T*>(GetSwapChainInternal(hSwapChain, xiiGetStaticRTTI<T>()));
  }

  const xiiGALShader*              GetShader(xiiGALShaderHandle hShader) const;
  const xiiGALTexture*             GetTexture(xiiGALTextureHandle hTexture) const;
  const xiiGALBuffer*              GetBuffer(xiiGALBufferHandle hBuffer) const;
  const xiiGALDepthStencilState*   GetDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState) const;
  const xiiGALBlendState*          GetBlendState(xiiGALBlendStateHandle hBlendState) const;
  const xiiGALRasterizerState*     GetRasterizerState(xiiGALRasterizerStateHandle hRasterizerState) const;
  const xiiGALVertexDeclaration*   GetVertexDeclaration(xiiGALVertexDeclarationHandle hVertexDeclaration) const;
  const xiiGALSamplerState*        GetSamplerState(xiiGALSamplerStateHandle hSamplerState) const;
  const xiiGALResourceView*        GetResourceView(xiiGALResourceViewHandle hResourceView) const;
  const xiiGALRenderTargetView*    GetRenderTargetView(xiiGALRenderTargetViewHandle hRenderTargetView) const;
  const xiiGALUnorderedAccessView* GetUnorderedAccessView(xiiGALUnorderedAccessViewHandle hUnorderedAccessView) const;
  const xiiGALQuery*               GetQuery(xiiGALQueryHandle hQuery) const;

  const xiiGALDeviceCapabilities& GetCapabilities() const;

  virtual xiiUInt64 GetMemoryConsumptionForTexture(const xiiGALTextureCreationDescription& Description) const;
  virtual xiiUInt64 GetMemoryConsumptionForBuffer(const xiiGALBufferCreationDescription& Description) const;

  static void          SetDefaultDevice(xiiGALDevice* pDefaultDevice);
  static xiiGALDevice* GetDefaultDevice();
  static bool          HasDefaultDevice();

  /// \brief Waits for the GPU to be idle and destroys any pending resources and GPU objects.
  void WaitIdle();

  // public in case someone external needs to lock multiple operations
  mutable xiiMutex m_Mutex;

private:
  static xiiGALDevice* s_pDefaultDevice;

protected:
  xiiGALDevice(const xiiGALDeviceCreationDescription& Description);

  virtual ~xiiGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(xiiGALResourceBase* pResource);

  template <typename HandleType>
  void AddDeadObject(xiiUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(xiiUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  const xiiGALSwapChain* GetSwapChainInternal(xiiGALSwapChainHandle hSwapChain, const xiiRTTI* pRequestedType) const;

  xiiGALTextureHandle FinalizeTextureInternal(const xiiGALTextureCreationDescription& desc, xiiGALTexture* pTexture);
  xiiGALBufferHandle  FinalizeBufferInternal(const xiiGALBufferCreationDescription& desc, xiiGALBuffer* pBuffer);

  xiiProxyAllocator        m_Allocator;
  xiiLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable              = xiiIdTable<xiiGALShaderHandle::IdType, xiiGALShader*, xiiLocalAllocatorWrapper>;
  using BlendStateTable          = xiiIdTable<xiiGALBlendStateHandle::IdType, xiiGALBlendState*, xiiLocalAllocatorWrapper>;
  using DepthStencilStateTable   = xiiIdTable<xiiGALDepthStencilStateHandle::IdType, xiiGALDepthStencilState*, xiiLocalAllocatorWrapper>;
  using RasterizerStateTable     = xiiIdTable<xiiGALRasterizerStateHandle::IdType, xiiGALRasterizerState*, xiiLocalAllocatorWrapper>;
  using BufferTable              = xiiIdTable<xiiGALBufferHandle::IdType, xiiGALBuffer*, xiiLocalAllocatorWrapper>;
  using TextureTable             = xiiIdTable<xiiGALTextureHandle::IdType, xiiGALTexture*, xiiLocalAllocatorWrapper>;
  using ResourceViewTable        = xiiIdTable<xiiGALResourceViewHandle::IdType, xiiGALResourceView*, xiiLocalAllocatorWrapper>;
  using SamplerStateTable        = xiiIdTable<xiiGALSamplerStateHandle::IdType, xiiGALSamplerState*, xiiLocalAllocatorWrapper>;
  using RenderTargetViewTable    = xiiIdTable<xiiGALRenderTargetViewHandle::IdType, xiiGALRenderTargetView*, xiiLocalAllocatorWrapper>;
  using UnorderedAccessViewTable = xiiIdTable<xiiGALUnorderedAccessViewHandle::IdType, xiiGALUnorderedAccessView*, xiiLocalAllocatorWrapper>;
  using SwapChainTable           = xiiIdTable<xiiGALSwapChainHandle::IdType, xiiGALSwapChain*, xiiLocalAllocatorWrapper>;
  using QueryTable               = xiiIdTable<xiiGALQueryHandle::IdType, xiiGALQuery*, xiiLocalAllocatorWrapper>;
  using VertexDeclarationTable   = xiiIdTable<xiiGALVertexDeclarationHandle::IdType, xiiGALVertexDeclaration*, xiiLocalAllocatorWrapper>;

  ShaderTable              m_Shaders;
  BlendStateTable          m_BlendStates;
  DepthStencilStateTable   m_DepthStencilStates;
  RasterizerStateTable     m_RasterizerStates;
  BufferTable              m_Buffers;
  TextureTable             m_Textures;
  ResourceViewTable        m_ResourceViews;
  SamplerStateTable        m_SamplerStates;
  RenderTargetViewTable    m_RenderTargetViews;
  UnorderedAccessViewTable m_UnorderedAccessViews;
  SwapChainTable           m_SwapChains;
  QueryTable               m_Queries;
  VertexDeclarationTable   m_VertexDeclarations;


  // Hash tables used to prevent state object duplication
  xiiHashTable<xiiUInt32, xiiGALBlendStateHandle, xiiHashHelper<xiiUInt32>, xiiLocalAllocatorWrapper>        m_BlendStateTable;
  xiiHashTable<xiiUInt32, xiiGALDepthStencilStateHandle, xiiHashHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_DepthStencilStateTable;
  xiiHashTable<xiiUInt32, xiiGALRasterizerStateHandle, xiiHashHelper<xiiUInt32>, xiiLocalAllocatorWrapper>   m_RasterizerStateTable;
  xiiHashTable<xiiUInt32, xiiGALSamplerStateHandle, xiiHashHelper<xiiUInt32>, xiiLocalAllocatorWrapper>      m_SamplerStateTable;
  xiiHashTable<xiiUInt32, xiiGALVertexDeclarationHandle, xiiHashHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_VertexDeclarationTable;

  struct DeadObject
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiType;
    xiiUInt32 m_uiHandle;
  };

  xiiDynamicArray<DeadObject, xiiLocalAllocatorWrapper> m_DeadObjects;

  xiiGALDeviceCreationDescription m_Description;

  xiiGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class xiiMemoryUtils;

  // Init & shutdown functions

  virtual xiiResult InitPlatform()     = 0;
  virtual xiiResult ShutdownPlatform() = 0;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain) = 0;
  virtual void EndPipelinePlatform(xiiGALSwapChain* pSwapChain)                       = 0;

  virtual xiiGALPass* BeginPassPlatform(const char* szName) = 0;
  virtual void        EndPassPlatform(xiiGALPass* pPass)    = 0;

  // State creation functions

  virtual xiiGALBlendState* CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description) = 0;
  virtual void              DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)                         = 0;

  virtual xiiGALDepthStencilState* CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description) = 0;
  virtual void                     DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)                  = 0;

  virtual xiiGALRasterizerState* CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description) = 0;
  virtual void                   DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)                    = 0;

  virtual xiiGALSamplerState* CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description) = 0;
  virtual void                DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState)                       = 0;

  // Resource creation functions

  virtual xiiGALShader* CreateShaderPlatform(const xiiGALShaderCreationDescription& Description) = 0;
  virtual void          DestroyShaderPlatform(xiiGALShader* pShader)                             = 0;

  virtual xiiGALBuffer* CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData) = 0;
  virtual void          DestroyBufferPlatform(xiiGALBuffer* pBuffer)                                                                       = 0;

  virtual xiiGALTexture* CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData) = 0;
  virtual void           DestroyTexturePlatform(xiiGALTexture* pTexture)                                                                                     = 0;

  virtual xiiGALResourceView* CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description) = 0;
  virtual void                DestroyResourceViewPlatform(xiiGALResourceView* pResourceView)                                                      = 0;

  virtual xiiGALRenderTargetView* CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void                    DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView)                                            = 0;

  virtual xiiGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void                       DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView)                                               = 0;

  // Other rendering creation functions

  virtual xiiGALQuery* CreateQueryPlatform(const xiiGALQueryCreationDescription& Description) = 0;
  virtual void         DestroyQueryPlatform(xiiGALQuery* pQuery)                              = 0;

  virtual xiiGALVertexDeclaration* CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void                     DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration)                  = 0;

  // Timestamp functions

  virtual xiiGALTimestampHandle GetTimestampPlatform()                                                        = 0;
  virtual xiiResult             GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result) = 0;

  // Misc functions

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) = 0;
  virtual void EndFramePlatform()                                = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  virtual void WaitIdlePlatform() = 0;


  /// \endcond

private:
  bool m_bBeginFrameCalled    = false;
  bool m_bBeginPipelineCalled = false;
  bool m_bBeginPassCalled     = false;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
