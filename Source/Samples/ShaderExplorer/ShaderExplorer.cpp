#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>

#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

class xiiShaderExplorer : public xiiWindow
{
public:
  xiiShaderExplorer() :
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

// A simple application that creates a window.
class xiiShaderExplorerApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiShaderExplorerApp() :
    xiiApplication("Shader Explorer")
  {
  }

  virtual Execution Run() override
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

      m_pCamera->RotateLocally(xiiAngle::Radian(0.0f), xiiAngle::Radian(mouseMotion.y), xiiAngle::Radian(0.0f));
      m_pCamera->RotateGlobally(xiiAngle::Radian(0.0f), xiiAngle::Radian(mouseMotion.x), xiiAngle::Radian(0.0f));
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

      m_pCamera->RotateLocally(xiiAngle::Radian(0.0f), xiiAngle::Radian(mouseMotion.y), xiiAngle::Radian(0.0f));
      m_pCamera->RotateGlobally(xiiAngle::Radian(0.0f), xiiAngle::Radian(mouseMotion.x), xiiAngle::Radian(0.0f));
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

      m_pCamera->MoveLocally(cameraMotion.y, cameraMotion.x, 0.0f);
    }

    // Reload resources if modified
    {
      m_bFileModified = false;
      m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiShaderExplorerApp::OnFileChanged, this));

      if (m_bFileModified)
      {
        xiiResourceManager::ReloadAllResources(false);
      }
    }

    // Perform rendering
    {
      m_pDevice->BeginFrame();

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

  virtual void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/ShaderExplorer";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
    xiiTelemetry::SetServerName("Sample Window");

    // Activate xiiTelemetry such that the inspector plugin can use the network connection.
    xiiTelemetry::CreateServer();

    // Load the inspector plugin.
    // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
    // When you are using xiiApplication, this is done automatically.
    xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();
#endif

    m_pCamera = XII_DEFAULT_NEW(xiiCamera);
    m_pCamera->LookAt(xiiVec3(3, 3, 1.5), xiiVec3(0, 0, 0), xiiVec3(0, 1, 0));
    m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);

    XII_VERIFY(m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded(), "Failed to watch project directory.");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    constexpr const char* szDefaultGraphicsAPI = "D3D12";
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
    constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#else
#  error Graphics API not implemented on platform.
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
      WindowCreationDesc.m_Title             = "Shader Explorer";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(xiiShaderExplorer);
      m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
    }

    // Create a device
    {
      xiiGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;

      m_pDevice = xiiGALDeviceFactory::CreateDevice(szDefaultGraphicsAPI, xiiFoundation::GetDefaultAllocator(), DeviceInit);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", szDefaultGraphicsAPI);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
    xiiStartup::StartupHighLevelSystems();

    UpdateSwapChain();
  }

  void UpdateSwapChain()
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
      swapChainDesc.m_bIsPrimary            = true;

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
  }

  void OnFileChanged(xiiStringView sFilename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
  {
    if (action == xiiDirectoryWatcherAction::Modified && type == xiiDirectoryWatcherType::File)
    {
      xiiLog::Info("File modified: '{0}'.", sFilename);
      m_bFileModified = true;
    }
  }

  virtual void BeforeHighLevelSystemsShutdown() override
  {
    m_pDirectoryWatcher->CloseDirectory();

    m_pDevice->DestroySwapChain(m_hSwapChain);
    m_hSwapChain.Invalidate();

    // Tell the engine that we are about to destroy window and graphics device and that it therefore needs to cleanup anything that depends on that.
    xiiStartup::ShutdownHighLevelSystems();

    // Now we can shutdown the graphics device.
    m_pDevice->Shutdown().IgnoreResult();

    XII_DEFAULT_DELETE(m_pDevice);

    // Finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);

    m_pCamera.Clear();
    m_pDirectoryWatcher.Clear();
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
    // Shut down telemetry if it was set up.
    xiiTelemetry::CloseConnection();
#endif

    SUPER::BeforeCoreSystemsShutdown();
  }

private:
  xiiShaderExplorer* m_pWindow = nullptr;

  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;

  xiiUniquePtr<xiiCamera>           m_pCamera;
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bFileModified = false;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderExplorerApp);
