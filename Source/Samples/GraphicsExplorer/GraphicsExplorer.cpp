#include <GraphicsExplorer/GraphicsExplorer.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
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
#include <GraphicsFoundation/Shader/InputLayout.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

xiiVec3U32 GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription)
{
  xiiVec3U32 size = {textureDescription.m_Size.width, textureDescription.m_Size.height, textureDescription.m_uiArraySizeOrDepth};
  size.x          = xiiMath::Max(1U, size.x >> uiMipLevelSize);
  size.y          = xiiMath::Max(1U, size.y >> uiMipLevelSize);
  size.z          = xiiMath::Max(1U, size.z >> uiMipLevelSize);
  return size;
}

class xiiGraphicsExplorerWindow : public xiiWindow
{
public:
  xiiGraphicsExplorerWindow() :
    xiiWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void       OnClickClose() override { m_bCloseRequested = true; }
  virtual xiiSizeU32 GetClientAreaSize() const override { return xiiSizeU32(g_uiWindowWidth, g_uiWindowHeight); }
  virtual void       OnResize(const xiiSizeU32& newWindowSize) override
  {
    if (g_uiWindowWidth != newWindowSize.width || g_uiWindowHeight != newWindowSize.height)
    {
      g_uiWindowWidth  = newWindowSize.width;
      g_uiWindowHeight = newWindowSize.height;
      g_bWindowResized = true;
    }
  }

  bool m_bCloseRequested;
};

xiiGraphicsExplorerWindowApp::xiiGraphicsExplorerWindowApp() :
  xiiApplication("Graphics Explorer")
{
}

xiiApplication::Execution xiiGraphicsExplorerWindowApp::Run()
{
  m_pWindow->ProcessWindowMessages();

  if (g_bWindowResized)
  {
    g_bWindowResized = false;

    UpdateSwapChain();
  }

  if (m_pWindow->m_bCloseRequested || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
    return Execution::Quit;

  // Make sure time goes on
  xiiClock::GetGlobalClock()->Update();

  // Update all input state
  xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

  // Engage mouse look
  if (xiiInputManager::GetInputActionState("Main", "Look") == xiiKeyState::Down)
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::ClipToPosition);

    float       fInputValue = 0.0f;
    const float fMouseSpeed = 0.01f;

    xiiVec3 mouseMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "LookPosX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x += fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookNegX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x -= fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookPosY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y -= fInputValue * fMouseSpeed;
    if (xiiInputManager::GetInputActionState("Main", "LookNegY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y += fInputValue * fMouseSpeed;
  }
  else
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
  }

  // Turn camera with arrow keys
  {
    float       fInputValue = 0.0f;
    const float fTurnSpeed  = 1.0f;

    xiiVec3 mouseMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "TurnPosX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x += fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnNegX", &fInputValue) != xiiKeyState::Up)
      mouseMotion.x -= fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnPosY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y += fInputValue * fTurnSpeed;
    if (xiiInputManager::GetInputActionState("Main", "TurnNegY", &fInputValue) != xiiKeyState::Up)
      mouseMotion.y -= fInputValue * fTurnSpeed;
  }

  // Apply translation
  {
    float   fInputValue = 0.0f;
    xiiVec3 cameraMotion(0.0f);

    if (xiiInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != xiiKeyState::Up)
      cameraMotion.x += fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != xiiKeyState::Up)
      cameraMotion.x -= fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != xiiKeyState::Up)
      cameraMotion.y += fInputValue;
    if (xiiInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != xiiKeyState::Up)
      cameraMotion.y -= fInputValue;
  }

  // Perform rendering.
  {
    // Before starting to render in a frame call this function
    m_pDevice->BeginFrame();

    m_pDevice->BeginPipeline("GraphicsExplorer", m_hSwapChain);

    auto pGraphicsQueue = m_pDevice->GetGraphicsQueue();

    if (auto pCommandList = pGraphicsQueue->BeginCommandList())
    {
      xiiGALBeginRenderPassDescription beginRenderPass{
        .m_hRenderPass  = m_hRenderPass,
        .m_hFramebuffer = m_hFrameBuffer,
      };

      auto& clearValue1                      = beginRenderPass.m_ClearValues.ExpandAndGetRef();
      clearValue1.m_DepthStencil.m_fDepth    = 1.0f;
      clearValue1.m_DepthStencil.m_uiStencil = 0U;

      auto& clearValue2        = beginRenderPass.m_ClearValues.ExpandAndGetRef();
      clearValue2.m_ClearColor = xiiColor::Blue;

      pCommandList->BeginRenderPass(beginRenderPass);
      pCommandList->EndRenderPass();

      pGraphicsQueue->Submit(pCommandList);
    }

    m_pDevice->EndPipeline(m_hSwapChain);

    m_pDevice->EndFrame();
  }

  // Make sure telemetry is sent out regularly.
  xiiTelemetry::PerFrameUpdate();

  // Needs to be called once per frame
  xiiResourceManager::PerFrameUpdate();

  // Tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  xiiTaskSystem::FinishFrameTasks();

  return xiiApplication::Execution::Continue;
}

void xiiGraphicsExplorerWindowApp::AfterCoreSystemsStartup()
{
  xiiStringBuilder sProjectDir = ">sdk/Data/Samples/GraphicsExplorer";
  xiiStringBuilder sProjectDirResolved;
  xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

  xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
  xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
  xiiTelemetry::SetServerName("Graphics Explorer");

  // Activate xiiTelemetry such that the inspector plugin can use the network connection.
  xiiTelemetry::CreateServer();

  // Load the inspector plugin.
  // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
  // When you are using xiiApplication, this is done automatically.
  xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();
#endif

  // Register Input
  {
    xiiInputActionConfig cfg;

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
    xiiInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookPosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosX;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookPosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegX;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookPosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosY;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookPosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "LookNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegY;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "LookNegY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnPosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyRight;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnPosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyLeft;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnPosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyDown;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnPosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "TurnNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyUp;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "TurnNegY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling    = false;
    xiiInputManager::SetInputActionConfig("Main", "Look", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyD;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegX");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyA;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyW;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

    cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegY");
    cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyS;
    cfg.m_bApplyTimeScaling    = true;
    xiiInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);
  }

  // Create a window for rendering
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    WindowCreationDesc.m_Title             = "Graphics Explorer";
    WindowCreationDesc.m_bShowMouseCursor  = true;
    WindowCreationDesc.m_bClipMouseCursor  = false;
    WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
    m_pWindow                              = XII_DEFAULT_NEW(xiiGraphicsExplorerWindow);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
  }

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
  constexpr const char* szDefaultGraphicsAPI = "D3D11";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
  constexpr const char* szDefaultGraphicsAPI = "D3D12";
#elif BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#else
  constexpr const char* szDefaultGraphicsAPI = "Null";
#endif

  {
    xiiGALDeviceCreationDescription deviceInitializationDescription;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    deviceInitializationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
    deviceInitializationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
    xiiStringView sShaderModel     = {};
    xiiStringView sShaderCompiler  = {};
    xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

#if TODO_ENABLE
    xiiShaderManager::Configure(sShaderModel, true);
    XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);
#endif

    m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceInitializationDescription);
    XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sGraphicsAPIName);
    XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

    m_pDevice->SetDebugName("Master Graphics Device");

    xiiGALDevice::SetDefaultDevice(m_pDevice);
  }

  UpdateSwapChain();

  // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
  xiiStartup::StartupHighLevelSystems();
}

void xiiGraphicsExplorerWindowApp::BeforeCoreSystemsShutdown()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
  // Shut down telemetry if it was set up.
  xiiTelemetry::CloseConnection();
#endif

  SUPER::BeforeCoreSystemsShutdown();
}

void xiiGraphicsExplorerWindowApp::BeforeHighLevelSystemsShutdown()
{
  m_pDevice->DestroyFramebuffer(m_hFrameBuffer);
  m_hFrameBuffer.Invalidate();

  m_pDevice->DestroyRenderPass(m_hRenderPass);
  m_hRenderPass.Invalidate();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_pDevice->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  // Tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  xiiStartup::ShutdownHighLevelSystems();

  // Now we can shutdown the graphics device.
  m_pDevice->Shutdown().IgnoreResult();

  XII_DEFAULT_DELETE(m_pDevice);

  // Finally destroy the window
  m_pWindow->Destroy().IgnoreResult();
  XII_DEFAULT_DELETE(m_pWindow);
}

void xiiGraphicsExplorerWindowApp::UpdateSwapChain()
{
  // Create a Swapchain
  if (m_hSwapChain.IsInvalidated())
  {
    xiiGALSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow               = m_pWindow;
    swapChainDesc.m_bIsPrimary            = true;
    swapChainDesc.m_Resolution.width      = g_uiWindowWidth;
    swapChainDesc.m_Resolution.height     = g_uiWindowHeight;
    swapChainDesc.m_ColorBufferFormat     = xiiGALTextureFormat::RGBA8UNormalizedSRGB;
    swapChainDesc.m_Usage                 = xiiGALSwapChainUsageFlags::RenderTarget;
    swapChainDesc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
    swapChainDesc.m_uiBufferCount         = 2U;
    swapChainDesc.m_fDefaultDepthValue    = 1.0f;
    swapChainDesc.m_uiDefaultStencilValue = 0U;

    m_hSwapChain = m_pDevice->CreateSwapChain(swapChainDesc);
  }
  else
  {
    auto pSwapChain  = m_pDevice->GetSwapChain(m_hSwapChain);
    auto currentSize = xiiSizeU32(g_uiWindowWidth, g_uiWindowHeight);

    if (pSwapChain->GetCurrentSize() != currentSize)
    {
      pSwapChain->Resize(m_pDevice, currentSize).IgnoreResult();
    }
  }

  // Do not destroy the texture if the swapchain is minimized
  if (!m_hSwapChain.IsInvalidated() && !m_hDepthStencilTexture.IsInvalidated() && m_pWindow->GetClientAreaSize().HasNonZeroArea())
  {
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);

    m_hDepthStencilTexture.Invalidate();
  }

  // Create depth texture
  if (m_pWindow->GetClientAreaSize().HasNonZeroArea())
  {
    xiiGALTextureCreationDescription texDesc;
    texDesc.m_Type        = xiiGALResourceDimension::Texture2D;
    texDesc.m_Size.width  = g_uiWindowWidth;
    texDesc.m_Size.height = g_uiWindowHeight;
    texDesc.m_Format      = xiiGALTextureFormat::D24UNormalizedS8UInt;
    texDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    m_pDevice->GetTexture(m_hDepthStencilTexture)->SetDebugName("Depth Stencil");
  }

  // Create render pass
  {
    if (!m_hRenderPass.IsInvalidated())
    {
      m_pDevice->DestroyRenderPass(m_hRenderPass);

      m_hRenderPass.Invalidate();
    }

    xiiGALRenderPassCreationDescription renderPassDesc;
    renderPassDesc.m_sName = "xiiGraphicsExplorerMainPass";

    const auto& depthTextureDesc    = m_pDevice->GetTexture(m_hDepthStencilTexture)->GetDescription();
    auto&       depthAttachmentDesc = renderPassDesc.m_Attachments.ExpandAndGetRef();

    depthAttachmentDesc.m_Format                = depthTextureDesc.m_Format;
    depthAttachmentDesc.m_uiSampleCount         = static_cast<xiiUInt8>(depthTextureDesc.m_uiSampleCount);
    depthAttachmentDesc.m_InitialStateFlags     = xiiGALResourceStateFlags::Unknown;
    depthAttachmentDesc.m_FinalStateFlags       = xiiGALResourceStateFlags::DepthWrite;
    depthAttachmentDesc.m_LoadOperation         = xiiGALAttachmentLoadOperation::Clear;
    depthAttachmentDesc.m_StoreOperation        = xiiGALAttachmentStoreOperation::Store;
    depthAttachmentDesc.m_StencilLoadOperation  = xiiGALAttachmentLoadOperation::Clear;
    depthAttachmentDesc.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;

    const auto& hBackBuffer           = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
    const auto& backBufferTextureDesc = m_pDevice->GetTexture(hBackBuffer)->GetDescription();
    auto&       colorAttachmentDesc   = renderPassDesc.m_Attachments.ExpandAndGetRef();

    colorAttachmentDesc.m_Format                = backBufferTextureDesc.m_Format;
    colorAttachmentDesc.m_uiSampleCount         = static_cast<xiiUInt8>(backBufferTextureDesc.m_uiSampleCount);
    colorAttachmentDesc.m_InitialStateFlags     = xiiGALResourceStateFlags::Unknown;
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
    dependencyDesc.m_DestinationAccessFlags            = xiiGALAccessFlags::DepthStencilWrite | xiiGALAccessFlags::RenderTargetWrite;

    m_hRenderPass = m_pDevice->CreateRenderPass(renderPassDesc);
    XII_ASSERT_DEV(!m_hRenderPass.IsInvalidated(), "Failed to create render pass.");
  }

  // Create frame buffer
  {
    if (!m_hFrameBuffer.IsInvalidated())
    {
      m_pDevice->DestroyFramebuffer(m_hFrameBuffer);

      m_hFrameBuffer.Invalidate();
    }

    const auto& hBackBuffer           = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
    const auto& hBackBufferView       = m_pDevice->GetTexture(hBackBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    const auto& hDepthStencilView     = m_pDevice->GetTexture(m_hDepthStencilTexture)->GetDefaultView(xiiGALTextureViewType::DepthStencil);
    const auto& backBufferTextureDesc = m_pDevice->GetTexture(hBackBuffer)->GetDescription();
    const auto& backBufferViewDesc    = m_pDevice->GetTextureView(hBackBufferView)->GetDescription();

    xiiVec3U32 vSize = GetMipLevelSize(backBufferViewDesc.m_uiMostDetailedMip, backBufferTextureDesc);

    xiiGALFramebufferCreationDescription framebufferDesc;
    framebufferDesc.m_hRenderPass       = m_hRenderPass;
    framebufferDesc.m_FramebufferSize   = {vSize.x, vSize.y};
    framebufferDesc.m_uiArraySliceCount = backBufferTextureDesc.GetArraySize();
    framebufferDesc.m_Attachments.PushBack(hDepthStencilView);
    framebufferDesc.m_Attachments.PushBack(hBackBufferView);

    m_hFrameBuffer = m_pDevice->CreateFramebuffer(framebufferDesc);
  }
}

XII_CONSOLEAPP_ENTRY_POINT(xiiGraphicsExplorerWindowApp);
