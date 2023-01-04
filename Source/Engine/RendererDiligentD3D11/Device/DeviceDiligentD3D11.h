
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligentD3D11/RendererDiligentD3D11DLL.h>
#include <RendererFoundation/Device/Device.h>

class xiiGALPassDiligentD3D11;

/// \brief The Diligent device implementation of the graphics abstraction layer.
class XII_RENDERERDILIGENTD3D11_DLL xiiGALDeviceDiligentD3D11 : public xiiGALDeviceDiligent
{
private:
  friend xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description);
  xiiGALDeviceDiligentD3D11(const xiiGALDeviceCreationDescription& Description);

public:
  virtual ~xiiGALDeviceDiligentD3D11();

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
  friend class xiiGALCommandEncoderImplDiligentD3D11;

  xiiUniquePtr<xiiGALPassDiligentD3D11> m_pDefaultPass;
};


#include <RendererDiligentD3D11/Device/Implementation/DeviceDiligentD3D11_inl.h>
