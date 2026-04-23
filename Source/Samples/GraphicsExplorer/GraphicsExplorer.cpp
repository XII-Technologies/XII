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

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
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

class xiiGraphicsExplorerApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiGraphicsExplorerApp() :
    xiiApplication("Graphics Explorer")
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
      // Before starting to render in a frame call this function.
      m_pDevice->BeginFrame();

      // If swap chain or its back buffer (or depth/rederpass) are not available we must skip rendering.
      bool bCanRender = (m_pSwapChain != nullptr) && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && (m_pSwapChain->GetBackBufferTexture() != nullptr);

      if (bCanRender)
      {
        // Build a minimal render graph that clears the depth and backbuffer.
        ++m_uiFrameIndex;

        m_pRenderGraph->BeginSetup(m_uiFrameIndex);

        auto [pData, hPass] = m_pRenderGraph->AddPass<ClearPassData>("ClearPass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiGraphicsExplorerApp::SetupClearPass, this), xiiMakeDelegate(&xiiGraphicsExplorerApp::ExecuteClearPass, this), /*bHasSideEffects=*/true);

        float fGlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 360.0);
        if (pData)
        {
          pData->m_fGlobalTime = fGlobalTime;
        }

        m_pRenderGraph->EndSetup();

        m_pRenderGraphResourceCache->BeginFrame(m_uiFrameIndex);

        xiiStringBuilder     sError;
        xiiRGCompileSettings settings;
        settings.m_bEnablePassCulling   = true;
        settings.m_bEnableCompileCache  = true;
        settings.m_bEnableSplitBarriers = false;
        settings.m_bEnableAsyncQueues   = true;
        settings.m_bEnableGPUProfiling  = true;

        if (m_pRenderGraph->Compile(settings, &sError).Succeeded())
        {
          m_pRenderGraph->Execute(m_pDevice.Borrow(), /*pView=*/nullptr, m_pRenderGraphBlackboard.Borrow(), m_pRenderGraphResourceCache.Borrow()).AssertSuccess("RenderGraph execution failed.");
        }
        else
        {
          xiiLog::Error("RenderGraph compile failed: {0}", sError);
        }

        m_pRenderGraphResourceCache->EndFrame();

        m_pSwapChain->Present();
      }
      else
      {
        // Ensure the swap chain can perform any internal throttling (e.g. when minimized)
        if (m_pSwapChain)
        {
          m_pSwapChain->Present();
        }
      }

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

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
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
      xiiWindowCreationDescription WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
      WindowCreationDesc.m_Title             = "Graphics Explorer";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(xiiGraphicsExplorerWindow);
      m_pWindow->Initialize(WindowCreationDesc).AssertSuccess();
    }

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

    UpdateSwapChain();

    // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
    xiiStartup::StartupHighLevelSystems();

    m_pRenderGraph              = XII_DEFAULT_NEW(xiiRenderGraph);
    m_pRenderGraphBlackboard    = XII_DEFAULT_NEW(xiiRenderGraphBlackboard);
    m_pRenderGraphResourceCache = XII_DEFAULT_NEW(xiiRenderGraphResourceCache);

    // Initialize the render-graph transient resource cache with our device.
    m_pRenderGraphResourceCache->Initialize(m_pDevice);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    // Shut down telemetry if it was set up.
    xiiTelemetry::CloseConnection();
#endif

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void BeforeHighLevelSystemsShutdown() override
  {
    m_pRenderGraphResourceCache.Clear();
    m_pRenderGraphBlackboard.Clear();
    m_pRenderGraph.Clear();

    m_pSwapChain.Clear();

    // Tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    xiiStartup::ShutdownHighLevelSystems();

    if (xiiGALDevice::GetDefaultDevice() == m_pDevice)
    {
      xiiGALDevice::SetDefaultDevice(nullptr);
    }
    m_pDevice.Clear();

    // Finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }

  void UpdateSwapChain()
  {
    // Create a Swap Chain
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
        m_pSwapChain->Resize(currentSize).IgnoreResult();
      }
    }
  }

private:
  // Render-graph support: per-frame clear pass data
  struct ClearPassData
  {
    xiiRGTextureHandle m_hBackbuffer;
    xiiRGTextureHandle m_hDepth;
    float              m_fGlobalTime = 0.0f;
  };

  void SetupClearPass(ClearPassData& data, xiiRGBuilder& builder)
  {
    data.m_hBackbuffer = builder.ImportTexture("Backbuffer", m_pSwapChain->GetBackBufferTexture(), xiiGALResourceStateFlags::RenderTarget);
    data.m_hBackbuffer = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);

    xiiGALTextureCreationDescription depthStencilTextureDescription;
    depthStencilTextureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
    depthStencilTextureDescription.m_Size.width  = g_uiWindowWidth;
    depthStencilTextureDescription.m_Size.height = g_uiWindowHeight;
    depthStencilTextureDescription.m_Format      = xiiGALResourceFormat::D24UNormalizedS8UInt;
    depthStencilTextureDescription.m_BindFlags   = xiiGALBindFlags::DepthStencil;

    data.m_hDepth = builder.WriteTexture("DepthStencil", depthStencilTextureDescription, xiiGALResourceStateFlags::DepthWrite);

    builder.SetPassSideEffects(true);
  }

  void ExecuteClearPass(const ClearPassData& data, xiiRGPassContext& context)
  {
    xiiGALCommandList& cmd = context.GetCommandList();

    cmd.BeginDebugGroup("GraphicsExplorerClear");
    {
      if (data.m_hDepth.IsValid())
      {
        cmd.ClearDepthStencilView(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 1.0f, 0U);
      }

      if (data.m_hBackbuffer.IsValid())
      {
        cmd.ClearRenderTargetView(context.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeHSV(data.m_fGlobalTime, 1.0f, 0.5f + 0.5f * sinf(data.m_fGlobalTime * 0.5f)));
      }
    }
    cmd.EndDebugGroup();
  }

private:
  xiiSharedPtr<xiiGALDevice>    m_pDevice;
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;

  xiiUniquePtr<xiiRenderGraph>              m_pRenderGraph;
  xiiUniquePtr<xiiRenderGraphBlackboard>    m_pRenderGraphBlackboard;
  xiiUniquePtr<xiiRenderGraphResourceCache> m_pRenderGraphResourceCache;
  xiiUInt64                                 m_uiFrameIndex = 0ULL;
  xiiUniquePtr<xiiGraphicsExplorerWindow>   m_pWindow;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiGraphicsExplorerApp);
