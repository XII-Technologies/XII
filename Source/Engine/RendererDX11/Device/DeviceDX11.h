
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11Device3;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;

typedef xiiGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0> xiiGALFormatLookupEntryDX11;
typedef xiiGALFormatLookupTable<xiiGALFormatLookupEntryDX11> xiiGALFormatLookupTableDX11;

class xiiGALPassDX11;

/// \brief The DX11 device implementation of the graphics abstraction layer.
class XII_RENDERERDX11_DLL xiiGALDeviceDX11 : public xiiGALDevice
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDX11Device(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  xiiGALDeviceDX11(const xiiGALDeviceCreationDescription& Description);

public:
  virtual ~xiiGALDeviceDX11();

public:
  ID3D11Device*               GetDXDevice() const;
  ID3D11Device3*              GetDXDevice3() const;
  ID3D11DeviceContext*        GetDXImmediateContext() const;
  IDXGIFactory1*              GetDXGIFactory() const;
  xiiGALRenderCommandEncoder* GetRenderCommandEncoder() const;

  const xiiGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  xiiResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

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

  void PresentPlatform(const xiiGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(const xiiUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  friend class xiiGALCommandEncoderImplDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  ID3D11Query* GetTimestamp(xiiGALTimestampHandle hTimestamp);

  ID3D11Resource* FindTempBuffer(xiiUInt32 uiSize);
  ID3D11Resource* FindTempTexture(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiDepth, xiiGALResourceFormat::Enum format);
  void            FreeTempResources(xiiUInt64 uiFrame);

  void FillFormatLookupTable();


  void InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  bool IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  void WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  ID3D11Device*        m_pDevice;
  ID3D11Device3*       m_pDevice3;
  ID3D11DeviceContext* m_pImmediateContext;

  ID3D11Debug* m_pDebug;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  xiiGALFormatLookupTableDX11 m_FormatLookupTable;

  xiiUInt32 m_uiFeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  xiiUniquePtr<xiiGALPassDX11> m_pDefaultPass;

  struct PerFrameData
  {
    ID3D11Query* m_pFence              = nullptr;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double       m_fInvTicksPerSecond  = -1.0;
    xiiUInt64    m_uiFrame             = -1;
  };

  PerFrameData m_PerFrameData[4];
  xiiUInt8     m_uiCurrentPerFrameData = 0;
  xiiUInt8     m_uiNextPerFrameData    = 0;

  xiiUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    XII_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    xiiUInt64       m_uiFrame;
    xiiUInt32       m_uiHash;
  };

  xiiMap<xiiUInt32, xiiDynamicArray<ID3D11Resource*>, xiiCompareHelper<xiiUInt32>, xiiLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  xiiDeque<UsedTempResource, xiiLocalAllocatorWrapper>                                                       m_UsedTempResources[TempResourceType::ENUM_COUNT];

  xiiDynamicArray<ID3D11Query*, xiiLocalAllocatorWrapper> m_Timestamps;
  xiiUInt32                                               m_uiCurrentTimestamp = 0;
  xiiUInt32                                               m_uiNextTimestamp    = 0;

  struct GPUTimingScope* m_pFrameTimingScope    = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope     = nullptr;

  xiiTime m_SyncTimeDiff;
  bool    m_bSyncTimeNeeded = true;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
