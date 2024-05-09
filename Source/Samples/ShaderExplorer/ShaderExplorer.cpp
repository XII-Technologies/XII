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
#include <GraphicsFoundation/Shader/InputLayout.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

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
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      m_pDevice->BeginPipeline("ShaderExplorer", m_hSwapChain);

      // Must always retrieve the current swapchain render target
      const xiiGALSwapChain*  pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
      xiiGALTextureViewHandle hBBRTV            = m_pDevice->GetTexture(pPrimarySwapChain->GetBackBufferTexture())->GetDefaultView(xiiGALTextureViewType::RenderTarget);
      xiiGALTextureViewHandle hBBDSV            = m_pDevice->GetTexture(m_hDepthStencilTexture)->GetDefaultView(xiiGALTextureViewType::DepthStencil);

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
      renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
      renderingSetup.m_bClearDepth             = true;
      renderingSetup.m_bClearStencil           = true;

      xiiGALCommandList* pCommandList = xiiRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), "xiiShaderExplorerMainPass");

      auto& gc = xiiRenderContext::GetDefaultInstance()->WriteGlobalConstants();
      xiiMemoryUtils::ZeroFill(&gc, 1);

      gc.WorldToCameraMatrix[0] = m_pCamera->GetViewMatrix(xiiCameraEye::Left);
      gc.WorldToCameraMatrix[1] = m_pCamera->GetViewMatrix(xiiCameraEye::Right);
      gc.CameraToWorldMatrix[0] = gc.WorldToCameraMatrix[0].GetInverse();
      gc.CameraToWorldMatrix[1] = gc.WorldToCameraMatrix[1].GetInverse();
      gc.ViewportSize           = xiiVec4((float)g_uiWindowWidth, (float)g_uiWindowHeight, 1.0f / (float)g_uiWindowWidth, 1.0f / (float)g_uiWindowHeight);
      // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
      gc.GlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
      gc.WorldTime  = gc.GlobalTime;

      xiiRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);
      xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
      xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
      xiiRenderContext::GetDefaultInstance()->EndRendering();

      m_pDevice->EndPipeline(m_hSwapChain);

      m_pDevice->EndFrame();

      xiiRenderContext::GetDefaultInstance()->ResetContextState();
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

    xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites).IgnoreResult();
    xiiFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", xiiFileSystem::AllowWrites).IgnoreResult();                               // writing to the binary directory
    xiiFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", xiiFileSystem::AllowWrites).IgnoreResult();                  // for shader files
    xiiFileSystem::AddDataDirectory(">user/XII/Projects/ShaderExplorer", "AppData", "appdata", xiiFileSystem::AllowWrites).IgnoreResult(); // app user data

    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
    xiiTelemetry::SetServerName("Shader Explorer");

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

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "D3D11";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "D3D12";
#elif BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#else
    constexpr const char* szDefaultGraphicsAPI = "Null";
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

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      DeviceInit.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
      DeviceInit.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

      xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
      xiiStringView sShaderModel     = {};
      xiiStringView sShaderCompiler  = {};
      xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

      xiiShaderManager::Configure(sShaderModel, true);
      XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);

      m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), DeviceInit);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
    xiiStartup::StartupHighLevelSystems();

    UpdateSwapChain();

    // Setup Shaders and Materials
    {
      m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Materials/screen.xiiMaterial");

      // Create the mesh that we use for rendering
      CreateScreenQuad();
    }
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
    }
  }

  void CreateScreenQuad()
  {
    xiiGeometry             geom;
    xiiGeometry::GeoOptions opt;
    opt.m_Color = xiiColor::Black;
    geom.AddRectXY(xiiVec2(2, 2), 1, 1, opt);

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALTextureFormat::RGB32Float);

    desc.AllocateStreams(geom.GetVertices().GetCount(), xiiGALPrimitiveTopology::TriangleList, geom.GetPolygons().GetCount() * 2);

    for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      desc.SetVertexData<xiiVec3>(0, v, geom.GetVertices()[v].m_vPosition);
    }

    xiiUInt32 t = 0;
    for (xiiUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (xiiUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        desc.SetTriangleIndices(t, geom.GetPolygons()[p].m_Vertices[0], geom.GetPolygons()[p].m_Vertices[v + 1], geom.GetPolygons()[p].m_Vertices[v + 2]);

        ++t;
      }
    }

    m_hQuadMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}");

    if (!m_hQuadMeshBuffer.IsValid())
      m_hQuadMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", std::move(desc));
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

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    m_hMaterial.Invalidate();
    m_hQuadMeshBuffer.Invalidate();
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
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiUniquePtr<xiiCamera>           m_pCamera;
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bFileModified = false;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderExplorerApp);
