#include <RendererDiligentD3D11/RendererDiligentD3D11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererDiligent/Resources/BufferDiligent.h>
#include <RendererDiligent/Resources/QueryDiligent.h>
#include <RendererDiligent/Resources/RenderTargetViewDiligent.h>
#include <RendererDiligent/Resources/ResourceViewDiligent.h>
#include <RendererDiligent/Resources/TextureDiligent.h>
#include <RendererDiligent/Resources/UnorderedAccessViewDiligent.h>
#include <RendererDiligent/Shader/ShaderDiligent.h>
#include <RendererDiligent/Shader/VertexDeclarationDiligent.h>
#include <RendererDiligent/State/StateDiligent.h>
#include <RendererDiligent/Utilities/DiligentConversions.h>
#include <RendererDiligentD3D11/CommandEncoder/CommandEncoderImplDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/DeviceDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/PassDiligentD3D11.h>
#include <RendererDiligentD3D11/Device/SwapChainDiligentD3D11.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>

xiiInternal::NewInstance<xiiGALDevice> CreateDiligentDevice(xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& Description)
{
  return XII_NEW(pAllocator, xiiGALDeviceDiligentD3D11, Description);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererDiligent, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  xiiGALDeviceFactory::RegisterCreatorFunc("DiligentD3D11", &CreateDiligentDevice, "DX11_SM50", "xiiShaderCompilerHLSL");
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiGALDeviceFactory::UnregisterCreatorFunc("DiligentD3D11");
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGALDeviceDiligentD3D11::xiiGALDeviceDiligentD3D11(const xiiGALDeviceCreationDescription& Description) :
  xiiGALDeviceDiligent(Description)
{
}

xiiGALDeviceDiligentD3D11::~xiiGALDeviceDiligentD3D11() = default;

// Init & shutdown functions

xiiResult xiiGALDeviceDiligentD3D11::InitPlatform()
{
  xiiGALDeviceDiligent::InitPlatform().AssertSuccess("Failed To Initialize Graphics Device");

  // Create default pass
  m_pDefaultPass = XII_NEW(&m_Allocator, xiiGALPassDiligentD3D11, *this);

  xiiClipSpaceDepthRange::Default           = xiiClipSpaceDepthRange::ZeroToOne;
  xiiClipSpaceYMode::RenderToTextureDefault = xiiClipSpaceYMode::Regular;

  xiiGALWindowSwapChain::SetFactoryMethod([this](const xiiGALWindowSwapChainCreationDescription& desc) -> xiiGALSwapChainHandle { return CreateSwapChain([this, &desc](xiiAllocatorBase* pAllocator) -> xiiGALSwapChain* { return XII_NEW(pAllocator, xiiGALSwapChainDiligentD3D11, desc); }); });

  m_SyncTimeDiff.SetZero();

  return XII_SUCCESS;
}

xiiResult xiiGALDeviceDiligentD3D11::ShutdownPlatform()
{
  xiiGALWindowSwapChain::SetFactoryMethod({});

  xiiGALDeviceDiligent::ShutdownPlatform().AssertSuccess("Failed To Shutdown Graphics Device");

  return XII_SUCCESS;
}

// Pipeline & Pass functions

void xiiGALDeviceDiligentD3D11::BeginPipelinePlatform(const char* szName, xiiGALSwapChain* pSwapChain)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPipelineTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void xiiGALDeviceDiligentD3D11::EndPipelinePlatform(xiiGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
}

xiiGALPass* xiiGALDeviceDiligentD3D11::BeginPassPlatform(const char* szName)
{
#if XII_ENABLED(XII_USE_PROFILING)
  m_pPassTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  m_pDefaultPass->BeginPass(szName);

  return m_pDefaultPass.Borrow();
}

void xiiGALDeviceDiligentD3D11::EndPassPlatform(xiiGALPass* pPass)
{
  XII_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif

  m_pDefaultPass->EndPass();
}

// State creation functions

xiiGALBlendState* xiiGALDeviceDiligentD3D11::CreateBlendStatePlatform(const xiiGALBlendStateCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateBlendStatePlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyBlendStatePlatform(xiiGALBlendState* pBlendState)
{
  return xiiGALDeviceDiligent::DestroyBlendStatePlatform(pBlendState);
}

xiiGALDepthStencilState* xiiGALDeviceDiligentD3D11::CreateDepthStencilStatePlatform(const xiiGALDepthStencilStateCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateDepthStencilStatePlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyDepthStencilStatePlatform(xiiGALDepthStencilState* pDepthStencilState)
{
  xiiGALDeviceDiligent::DestroyDepthStencilStatePlatform(pDepthStencilState);
}

xiiGALRasterizerState* xiiGALDeviceDiligentD3D11::CreateRasterizerStatePlatform(const xiiGALRasterizerStateCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateRasterizerStatePlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyRasterizerStatePlatform(xiiGALRasterizerState* pRasterizerState)
{
  xiiGALDeviceDiligent::DestroyRasterizerStatePlatform(pRasterizerState);
}

xiiGALSamplerState* xiiGALDeviceDiligentD3D11::CreateSamplerStatePlatform(const xiiGALSamplerStateCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateSamplerStatePlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroySamplerStatePlatform(xiiGALSamplerState* pSamplerState)
{
  xiiGALDeviceDiligent::DestroySamplerStatePlatform(pSamplerState);
}


// Resource creation functions

xiiGALShader* xiiGALDeviceDiligentD3D11::CreateShaderPlatform(const xiiGALShaderCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateShaderPlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyShaderPlatform(xiiGALShader* pShader)
{
  xiiGALDeviceDiligent::DestroyShaderPlatform(pShader);
}

xiiGALBuffer* xiiGALDeviceDiligentD3D11::CreateBufferPlatform(const xiiGALBufferCreationDescription& Description, xiiArrayPtr<const xiiUInt8> pInitialData)
{
  return xiiGALDeviceDiligent::CreateBufferPlatform(Description, pInitialData);
}

void xiiGALDeviceDiligentD3D11::DestroyBufferPlatform(xiiGALBuffer* pBuffer)
{
  xiiGALDeviceDiligent::DestroyBufferPlatform(pBuffer);
}

xiiGALTexture* xiiGALDeviceDiligentD3D11::CreateTexturePlatform(const xiiGALTextureCreationDescription& Description, xiiArrayPtr<xiiGALSystemMemoryDescription> pInitialData)
{
  return xiiGALDeviceDiligent::CreateTexturePlatform(Description, pInitialData);
}

void xiiGALDeviceDiligentD3D11::DestroyTexturePlatform(xiiGALTexture* pTexture)
{
  xiiGALDeviceDiligent::DestroyTexturePlatform(pTexture);
}

xiiGALResourceView* xiiGALDeviceDiligentD3D11::CreateResourceViewPlatform(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateResourceViewPlatform(pResource, Description);
}

void xiiGALDeviceDiligentD3D11::DestroyResourceViewPlatform(xiiGALResourceView* pResourceView)
{
  xiiGALDeviceDiligent::DestroyResourceViewPlatform(pResourceView);
}

xiiGALRenderTargetView* xiiGALDeviceDiligentD3D11::CreateRenderTargetViewPlatform(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateRenderTargetViewPlatform(pTexture, Description);
}

void xiiGALDeviceDiligentD3D11::DestroyRenderTargetViewPlatform(xiiGALRenderTargetView* pRenderTargetView)
{
  xiiGALDeviceDiligent::DestroyRenderTargetViewPlatform(pRenderTargetView);
}

xiiGALUnorderedAccessView* xiiGALDeviceDiligentD3D11::CreateUnorderedAccessViewPlatform(xiiGALResourceBase* pTextureOfBuffer, const xiiGALUnorderedAccessViewCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateUnorderedAccessViewPlatform(pTextureOfBuffer, Description);
}

void xiiGALDeviceDiligentD3D11::DestroyUnorderedAccessViewPlatform(xiiGALUnorderedAccessView* pUnorderedAccessView)
{
  xiiGALDeviceDiligent::DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
}



// Other rendering creation functions

xiiGALQuery* xiiGALDeviceDiligentD3D11::CreateQueryPlatform(const xiiGALQueryCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateQueryPlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyQueryPlatform(xiiGALQuery* pQuery)
{
  xiiGALDeviceDiligent::DestroyQueryPlatform(pQuery);
}

xiiGALVertexDeclaration* xiiGALDeviceDiligentD3D11::CreateVertexDeclarationPlatform(const xiiGALVertexDeclarationCreationDescription& Description)
{
  return xiiGALDeviceDiligent::CreateVertexDeclarationPlatform(Description);
}

void xiiGALDeviceDiligentD3D11::DestroyVertexDeclarationPlatform(xiiGALVertexDeclaration* pVertexDeclaration)
{
  xiiGALDeviceDiligent::DestroyVertexDeclarationPlatform(pVertexDeclaration);
}

xiiGALTimestampHandle xiiGALDeviceDiligentD3D11::GetTimestampPlatform()
{
  // TODO

  return {0, 0};
}

xiiResult xiiGALDeviceDiligentD3D11::GetTimestampResultPlatform(xiiGALTimestampHandle hTimestamp, xiiTime& result)
{
  // TODO

  return XII_FAILURE;
}

// Swap chain functions


// Misc functions

void xiiGALDeviceDiligentD3D11::BeginFramePlatform(const xiiUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  xiiStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);

#if XII_ENABLED(XII_USE_PROFILING)
  m_pFrameTimingScope = xiiProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif
}

void xiiGALDeviceDiligentD3D11::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

#if XII_ENABLED(XII_USE_PROFILING)
  xiiProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  FreeTempResources(GetImmediateContext()->GetFrameNumber());
}

void xiiGALDeviceDiligentD3D11::FillCapabilitiesPlatform()
{
  xiiGALDeviceDiligent::FillCapabilitiesPlatform();
}

void xiiGALDeviceDiligentD3D11::WaitIdlePlatform()
{
  xiiGALDeviceDiligent::WaitIdlePlatform();
}



XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Device_Implementation_DeviceDiligent);
