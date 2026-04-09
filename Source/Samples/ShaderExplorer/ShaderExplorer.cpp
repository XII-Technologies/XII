#include <Foundation/Application/Application.h>
#include <Foundation/Communication/GlobalEvent.h>
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

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// Define this to force usage of fileserve functionality.
// #define USE_FILESERVE XII_ON

// To use fileserve, run the xiiFileServe application with a command line that tells it where the ":project"
// data directory is located on the PC, for example:
//
// xiiFileServe.exe -fs_start -specialdirs project "C:\XII\Data\Samples\ShaderExplorer"

#if !defined(USE_FILESERVE)
#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
// on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#    define USE_FILESERVE XII_ON
#  else
#    define USE_FILESERVE XII_OFF
#  endif
#endif

#if XII_DISABLED(USE_FILESERVE) && XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER XII_ON
#else
#  define USE_DIRECTORY_WATCHER XII_OFF
#endif

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

      m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(mouseMotion.y), xiiAngle::MakeFromRadian(0.0f));
      m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(mouseMotion.x), xiiAngle::MakeFromRadian(0.0f));
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

      m_pCamera->RotateLocally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(mouseMotion.y), xiiAngle::MakeFromRadian(0.0f));
      m_pCamera->RotateGlobally(xiiAngle::MakeFromRadian(0.0f), xiiAngle::MakeFromRadian(mouseMotion.x), xiiAngle::MakeFromRadian(0.0f));
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
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    {
      m_bFileModified = false;
      m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiShaderExplorerApp::OnFileChanged, this));

      if (m_bFileModified)
      {
        xiiResourceManager::ReloadAllResources(false);
      }
    }
#endif

    // Perform rendering
    {
      // Before starting to render in a frame call this function.
      m_pDevice->BeginFrame();

      xiiRenderContext* pRenderContext = xiiRenderContext::GetDefaultInstance();

      xiiRenderingSetup renderingSetup;
      renderingSetup.AddColorAttachment({.m_pRenderTarget = m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget), .m_LoadOp = xiiGALAttachmentLoadOperation::Clear})
        .SetDepthStencilAttachment({.m_pDSTarget = m_pDepthStencilTexture->GetDefaultView(xiiGALTextureViewType::DepthStencil), .m_LoadOp = xiiGALAttachmentLoadOperation::Clear, .m_StencilLoadOp = xiiGALAttachmentLoadOperation::Clear})
        .Build();

      pRenderContext->BeginRendering(renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), "xiiShaderExplorerMainPass");
      {
        {
          xiiGlobalConstants* pGlobalConstants = pRenderContext->GetGlobalConstants();

          xiiMat4 m0, m1;
          m0                                       = m_pCamera->GetViewMatrix(xiiCameraEye::Left);
          m1                                       = m_pCamera->GetViewMatrix(xiiCameraEye::Right);
          pGlobalConstants->WorldToCameraMatrix[0] = m0;
          pGlobalConstants->WorldToCameraMatrix[1] = m1;
          pGlobalConstants->CameraToWorldMatrix[0] = m0.GetInverse();
          pGlobalConstants->CameraToWorldMatrix[1] = m1.GetInverse();
          pGlobalConstants->ViewportSize           = xiiVec4((float)g_uiWindowWidth, (float)g_uiWindowHeight, 1.0f / (float)g_uiWindowWidth, 1.0f / (float)g_uiWindowHeight);

          pRenderContext->SetGlobalAndWorldTimeConstants();
        }

        pRenderContext->BindMaterial(m_hMaterial);
        pRenderContext->BindMeshBuffer(m_hQuadMeshBuffer);
        pRenderContext->DrawMeshBuffer().IgnoreResult();
      }
      pRenderContext->EndRendering();

      m_pSwapChain->Present();

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

    // For plugins (like FileServe) that need to hook into the game update.
    XII_BROADCAST_EVENT(GameApp_UpdatePlugins);

    return xiiApplication::Execution::Continue;
  }

  virtual void AfterCoreSystemsStartup() override
  {
#if XII_ENABLED(USE_FILESERVE)
    xiiPlugin::LoadPlugin("xiiFileservePlugin").AssertSuccess("Failed to load FileServe plugin.");
#endif

    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/ShaderExplorer";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();
    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif

    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
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
      WindowCreationDesc.m_Title             = "Shader Explorer";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(xiiShaderExplorer);
      m_pWindow->Initialize(WindowCreationDesc).AssertSuccess();
    }

    // Create a device
    {
      xiiGALDeviceCreationDescription deviceCreationDescription;
      deviceCreationDescription.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;
      deviceCreationDescription.m_DeviceFeatures.m_NativeFence                        = xiiGALDeviceFeatureState::Optional;
      deviceCreationDescription.m_DeviceFeatures.m_ExternalMemory                     = xiiGALDeviceFeatureState::Optional;
      deviceCreationDescription.m_DeviceFeatures.m_ExternalSemaphore                  = xiiGALDeviceFeatureState::Optional;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

      xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");
      xiiStringView sShaderModel     = {};
      xiiStringView sShaderCompiler  = {};
      xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

      xiiGALShaderManager::Configure(sShaderModel, true);
      XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);

      m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      m_pDevice->SetDebugName("Master Graphics Device");

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

  void CreateScreenQuad()
  {
    m_hQuadMeshBuffer = xiiResourceManager::GetExistingResource<xiiMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}");

    if (!m_hQuadMeshBuffer.IsValid())
    {
      xiiGeometry             geom;
      xiiGeometry::GeoOptions opt;
      opt.m_Color = xiiColor::Black;
      geom.AddRect(xiiVec2(2, 2), 1, 1, opt);

      xiiMeshBufferResourceDescriptor desc;
      desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);

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

      m_hQuadMeshBuffer = xiiResourceManager::GetOrCreateResource<xiiMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", std::move(desc), "Shader Explorer Screen");
    }
  }

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  void OnFileChanged(xiiStringView sFilename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
  {
    if (action == xiiDirectoryWatcherAction::Modified && type == xiiDirectoryWatcherType::File)
    {
      xiiLog::Info("File modified: '{0}'.", sFilename);
      m_bFileModified = true;
    }
  }
#endif

  virtual void BeforeHighLevelSystemsShutdown() override
  {
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher->CloseDirectory();
    m_pDirectoryWatcher.Clear();
#endif

    m_hMaterial.Invalidate();
    m_hQuadMeshBuffer.Invalidate();

    m_pDepthStencilTexture.Clear();
    m_pSwapChain.Clear();

    // Tell the engine that we are about to destroy window and graphics device and that it therefore needs to cleanup anything that depends on that.
    xiiStartup::ShutdownHighLevelSystems();

    if (xiiGALDevice::GetDefaultDevice() == m_pDevice)
    {
      xiiGALDevice::SetDefaultDevice(nullptr);
    }

    // Now we can shutdown the graphics device.
    m_pDevice.Clear();

    // Finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();

    m_pCamera.Clear();
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

  void UpdateSwapChain()
  {
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget | xiiGALSwapChainUsageFlags::ShaderResource;
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
        // Clear frame buffer cache since swap chain images and depth stencil will be recreated.
        m_pDepthStencilTexture.Clear();

        m_pSwapChain->Resize(currentSize).IgnoreResult();
      }
    }

    if (!m_pDepthStencilTexture)
    {
      xiiGALTextureCreationDescription textureDescription;
      textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
      textureDescription.m_Size.width  = g_uiWindowWidth;
      textureDescription.m_Size.height = g_uiWindowHeight;
      textureDescription.m_Format      = xiiGALResourceFormat::D24UNormalizedS8UInt;
      textureDescription.m_BindFlags   = xiiGALBindFlags::DepthStencil;

      m_pDepthStencilTexture = m_pDevice->CreateTexture(textureDescription);

      m_pDepthStencilTexture->SetDebugName("Depth Stencil");
    }
  }

private:
  xiiUniquePtr<xiiShaderExplorer> m_pWindow;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiSharedPtr<xiiGALTexture>   m_pDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiUniquePtr<xiiCamera> m_pCamera;

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;
#endif
};

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderExplorerApp);
