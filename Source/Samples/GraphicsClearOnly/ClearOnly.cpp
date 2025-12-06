#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Time/Clock.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

class xiiClearOnlyWindow : public xiiWindow
{
public:
  xiiClearOnlyWindow() : xiiWindow() { m_bCloseRequested = false; }

  virtual void       OnClickClose() override { m_bCloseRequested = true; }
  virtual xiiSizeU32 GetClientAreaSize() const override { return xiiSizeU32(g_uiWindowWidth, g_uiWindowHeight); }
  virtual void       OnResize(const xiiSizeU32& newWindowSize) override
  {
    xiiWindow::OnResize(newWindowSize);

    if (g_uiWindowWidth != newWindowSize.width || g_uiWindowHeight != newWindowSize.height)
    {
      g_uiWindowWidth  = newWindowSize.width;
      g_uiWindowHeight = newWindowSize.height;
      g_bWindowResized = true;
    }
  }

  bool m_bCloseRequested;
};

class xiiClearOnlyApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiClearOnlyApp() : xiiApplication("Graphics Clear Only")
  {
  }

  virtual Execution Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (!m_pWindow->IsVisible())
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(16));
      return Execution::Continue;
    }

    if (g_bWindowResized)
    {
      g_bWindowResized = false;

      UpdateSwapChain();
    }

    if (m_pWindow->m_bCloseRequested)
      return Execution::Quit;

    // Advance time and task system
    xiiClock::GetGlobalClock()->Update();

    // Perform rendering: just clear the backbuffer and depth
    m_pDevice->BeginFrame();

    auto pGraphicsQueue = m_pDevice->GetCommandQueue();

    m_pCommandList->Begin();
    {
      xiiGALBeginRenderPassDescription beginRenderPass(m_pRenderPass, GetCurrentFramebuffer());

      auto& depthClearValue                      = beginRenderPass.m_ClearValues.ExpandAndGetRef();
      depthClearValue.m_DepthStencil.m_fDepth    = 1.0f;
      depthClearValue.m_DepthStencil.m_uiStencil = 0U;

      float fGlobalTime             = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 360.0);
      auto& colorClearValue         = beginRenderPass.m_ClearValues.ExpandAndGetRef();
      colorClearValue.m_ClearColour = xiiColor::MakeHSV(fGlobalTime, 1.0f, 0.5f + 0.5f * sinf(fGlobalTime * 0.5f));

      m_pCommandList->BeginRenderPass(beginRenderPass);
      m_pCommandList->EndRenderPass();
    }
    m_pCommandList->End();

    pGraphicsQueue->Submit(m_pCommandList);

    m_pSwapChain->Present();

    m_pDevice->EndFrame();

    // Per-frame housekeeping
    xiiTelemetry::PerFrameUpdate();
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();

    return Execution::Continue;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/GraphicsExplorer";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiDataDirUsage::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    // Create a window for rendering
    {
      xiiWindowCreationDescription WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
      WindowCreationDesc.m_Title             = "Graphics Clear Only";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(xiiClearOnlyWindow);
      m_pWindow->Initialize(WindowCreationDesc).AssertSuccess();
    }

    // Create device and command list
    {
      xiiGALDeviceCreationDescription deviceCreationDescription;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

      xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");
      m_pDevice                      = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      m_pDevice->SetDebugName("Master Graphics Device");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    {
      m_pCommandList = m_pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      XII_ASSERT_DEV(m_pCommandList != nullptr, "Failed to create command list!");
    }

    UpdateSwapChain();

    CreateRenderPass();

    xiiStartup::StartupHighLevelSystems();
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void BeforeHighLevelSystemsShutdown() override
  {
    m_pCommandList.Clear();
    m_FramebufferCache.Clear();
    m_pRenderPass.Clear();
    m_pDepthStencilTexture.Clear();
    m_pSwapChain.Clear();

    xiiStartup::ShutdownHighLevelSystems();

    if (xiiGALDevice::GetDefaultDevice() == m_pDevice)
    {
      xiiGALDevice::SetDefaultDevice(nullptr);
    }
    m_pDevice.Clear();

    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }

  void UpdateSwapChain()
  {
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget;
      swapChainDescription.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapChainDescription.m_uiBufferCount         = 2U;
      swapChainDescription.m_fDefaultDepthValue    = 1.0f;
      swapChainDescription.m_uiDefaultStencilValue = 0U;

      m_pSwapChain = m_pDevice->CreateSwapChain(swapChainDescription);
      m_pSwapChain->SetPresentMode(xiiGALPresentMode::VSync);
    }
    else
    {
      auto currentSize = xiiSizeU32(g_uiWindowWidth, g_uiWindowHeight);

      if (m_pSwapChain->GetCurrentSize() != currentSize)
      {
        m_FramebufferCache.Clear();
        m_pDepthStencilTexture.Clear();

        m_pSwapChain->Resize(currentSize).IgnoreResult();
      }
    }

    if (!m_pDepthStencilTexture)
    {
      xiiGALTextureCreationDescription texDesc;
      texDesc.m_Type        = xiiGALResourceDimension::Texture2D;
      texDesc.m_Size.width  = g_uiWindowWidth;
      texDesc.m_Size.height = g_uiWindowHeight;
      texDesc.m_Format      = xiiGALResourceFormat::D24UNormalizedS8UInt;
      texDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil;

      m_pDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
      m_pDepthStencilTexture->SetDebugName("Depth Stencil");
    }
  }

  void CreateRenderPass()
  {
    if (!m_pRenderPass)
    {
      xiiSharedPtr<xiiGALTexture> pBackBuffer           = m_pSwapChain->GetBackBufferTexture();
      const auto&                 backBufferTextureDesc = pBackBuffer->GetDescription();

      xiiGALRenderPassCreationDescription renderPassDesc;

      const auto& depthTextureDesc    = m_pDepthStencilTexture->GetDescription();
      auto&       depthAttachmentDesc = renderPassDesc.m_Attachments.ExpandAndGetRef();

      depthAttachmentDesc.m_Format                = depthTextureDesc.m_Format;
      depthAttachmentDesc.m_uiSampleCount         = static_cast<xiiUInt8>(depthTextureDesc.m_uiSampleCount);
      depthAttachmentDesc.m_InitialStateFlags     = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentDesc.m_FinalStateFlags       = xiiGALResourceStateFlags::DepthWrite;
      depthAttachmentDesc.m_LoadOperation         = xiiGALAttachmentLoadOperation::Clear;
      depthAttachmentDesc.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
      depthAttachmentDesc.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Clear;
      depthAttachmentDesc.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;

      auto& colorAttachmentDesc = renderPassDesc.m_Attachments.ExpandAndGetRef();

      colorAttachmentDesc.m_Format                = backBufferTextureDesc.m_Format;
      colorAttachmentDesc.m_uiSampleCount         = static_cast<xiiUInt8>(backBufferTextureDesc.m_uiSampleCount);
      colorAttachmentDesc.m_InitialStateFlags     = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDesc.m_FinalStateFlags       = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDesc.m_LoadOperation         = xiiGALAttachmentLoadOperation::Clear;
      colorAttachmentDesc.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
      colorAttachmentDesc.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Discard;
      colorAttachmentDesc.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Discard;

      xiiGALSubPassDescription& subpassDesc = renderPassDesc.m_SubPasses.ExpandAndGetRef();
      {
        auto& depthAttachmentRef                = subpassDesc.m_DepthStencilAttachment.ExpandAndGetRef();
        depthAttachmentRef.m_ResourceStateFlags = xiiGALResourceStateFlags::DepthWrite;
        depthAttachmentRef.m_uiAttachmentIndex  = 0U;

        auto& colorAttachmentRef                = subpassDesc.m_RenderTargetAttachments.ExpandAndGetRef();
        colorAttachmentRef.m_ResourceStateFlags = xiiGALResourceStateFlags::RenderTarget;
        colorAttachmentRef.m_uiAttachmentIndex  = 1U;
      }

      xiiGALSubPassDependencyDescription& dependencyDesc = renderPassDesc.m_Dependencies.ExpandAndGetRef();
      dependencyDesc.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
      dependencyDesc.m_uiDestinationSubPass              = 0U;
      dependencyDesc.m_SourceStageFlags                  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDesc.m_DestinationStageFlags             = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDesc.m_DestinationAccessFlags            = xiiGALAccessFlags::None;

      m_pRenderPass = m_pDevice->CreateRenderPass(renderPassDesc);
      XII_ASSERT_DEV(m_pRenderPass != nullptr, "Failed to create render pass.");
    }
  }

  xiiSharedPtr<xiiGALFramebuffer> CreateFramebuffer(xiiSharedPtr<xiiGALTextureView> pDestinationRenderTarget, xiiSharedPtr<xiiGALTextureView> pDestinationDepthStencil)
  {
    XII_ASSERT_DEV(pDestinationRenderTarget != nullptr, "Destination render target must not be null.");
    XII_ASSERT_DEV(m_pRenderPass != nullptr, "Render pass must be created before creating a framebuffer.");

    const auto& textureDescription = pDestinationRenderTarget->GetTexture()->GetDescription();
    const auto& viewDescription    = pDestinationRenderTarget->GetDescription();

    xiiVec3U32 vSize = xiiGALTextureUtilities::GetMipLevelSize(viewDescription.m_uiMostDetailedMip, textureDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_pRenderPass       = m_pRenderPass;
    framebufferDescription.m_FramebufferSize   = {vSize.x, vSize.y};
    framebufferDescription.m_uiArraySliceCount = viewDescription.m_uiArrayOrDepthSlicesCount;
    framebufferDescription.m_Attachments.PushBack(pDestinationDepthStencil);
    framebufferDescription.m_Attachments.PushBack(pDestinationRenderTarget);

    xiiSharedPtr<xiiGALFramebuffer> pFramebuffer = m_pDevice->CreateFramebuffer(framebufferDescription);
    XII_ASSERT_DEV(pFramebuffer != nullptr, "Failed to create framebuffer.");

    m_FramebufferCache.PushBack(pFramebuffer);

    return pFramebuffer;
  }

  xiiSharedPtr<xiiGALFramebuffer> GetCurrentFramebuffer()
  {
    auto pBackBufferTexture = m_pSwapChain->GetBackBufferTexture();
    auto pBackBufferView    = pBackBufferTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);

    for (xiiUInt32 i = 0; i < m_FramebufferCache.GetCount(); ++i)
    {
      const auto& framebufferDescription = m_FramebufferCache[i]->GetDescription();

      if (framebufferDescription.m_pRenderPass == m_pRenderPass && framebufferDescription.m_Attachments[1] == pBackBufferView)
      {
        return m_FramebufferCache[i];
      }
    }
    return CreateFramebuffer(m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget), m_pDepthStencilTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil));
  }

private:
  xiiUniquePtr<xiiClearOnlyWindow> m_pWindow;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiSharedPtr<xiiGALTexture>   m_pDepthStencilTexture;

  xiiSharedPtr<xiiGALCommandList>                     m_pCommandList;
  xiiSharedPtr<xiiGALRenderPass>                      m_pRenderPass;
  xiiHybridArray<xiiSharedPtr<xiiGALFramebuffer>, 3U> m_FramebufferCache;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiClearOnlyApp);
