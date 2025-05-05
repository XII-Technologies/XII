#include <GraphicsNull/GraphicsNullPCH.h>

#include <Core/System/Window.h>
#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Device/SwapChainNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSwapChainNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALSwapChainNull::xiiGALSwapChainNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALSwapChainCreationDescription& creationDescription) :
  xiiGALSwapChain(pDeviceNull, creationDescription)
{
}

xiiGALSwapChainNull::~xiiGALSwapChainNull()
{
  m_Description.m_pWindow->RemoveReference();
}

xiiResult xiiGALSwapChainNull::InitPlatform()
{
  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_Description.m_pWindow->AddReference();

  return CreateBackBufferInternal();
}

xiiResult xiiGALSwapChainNull::CreateBackBufferInternal()
{
  xiiSharedPtr<xiiGALDeviceNull> pDeviceNull   = m_pDevice.Downcast<xiiGALDeviceNull>();
  xiiEnum<xiiGALResourceFormat>  textureFormat = pDeviceNull->GetDescription().m_GraphicsDeviceType != xiiGALGraphicsDeviceType::Vulkan ? xiiGALResourceFormat::RGBA8UNormalizedSRGB : xiiGALResourceFormat::BGRA8UNormalizedSRGB;

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Size               = m_Description.m_pWindow->GetClientAreaSize();
  textureDescription.m_uiArraySizeOrDepth = 1U;
  textureDescription.m_Format             = textureFormat;
  textureDescription.m_uiMipLevels        = 1U;
  textureDescription.m_uiSampleCount      = xiiGALSampleCount::OneSample;
  textureDescription.m_BindFlags          = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  textureDescription.m_Usage              = xiiGALResourceUsage::Default;
  textureDescription.m_CPUAccessFlags     = xiiGALCPUAccessFlag::Read;
  textureDescription.m_MiscFlags          = xiiGALMiscTextureFlags::None;

  textureDescription.m_ClearValue.m_ResourceFormat           = textureFormat;
  textureDescription.m_ClearValue.m_ClearColor               = xiiColor::Black;
  textureDescription.m_ClearValue.m_DepthStencil.m_fDepth    = 1.0f;
  textureDescription.m_ClearValue.m_DepthStencil.m_uiStencil = 0U;
  textureDescription.m_uiCommandQueueMask                    = XII_BIT(0);

  textureDescription.m_pExisitingNativeObject = nullptr;

  m_pBackBufferTexture = pDeviceNull->CreateTexture(textureDescription);
  XII_ASSERT_RELEASE(m_pBackBufferTexture != nullptr, "Couldn't create native backbuffer texture object!");

  m_pBackBufferTexture->SetDebugName("SwapChain Null Render Target.");

  m_Description.m_Resolution = textureDescription.m_Size;

  return XII_SUCCESS;
}

void xiiGALSwapChainNull::Present()
{
}

xiiResult xiiGALSwapChainNull::Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform)
{
  XII_IGNORE_UNUSED(newSize);
  XII_IGNORE_UNUSED(newTransform);

  m_pBackBufferTexture.Clear();

  return CreateBackBufferInternal();
}

void xiiGALSwapChainNull::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
  XII_IGNORE_UNUSED(displayMode);
}

void xiiGALSwapChainNull::SetWindowedMode()
{
}

void xiiGALSwapChainNull::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
  XII_IGNORE_UNUSED(uiMaxLatency);
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Device_Implementation_SwapChainNull);
