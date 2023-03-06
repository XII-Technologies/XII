#include <ShaderExplorer/ShaderExplorer.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

class xiiShaderExplorerWindow : public xiiWindow
{
public:
  xiiShaderExplorerWindow() :
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

xiiShaderExplorerApp::xiiShaderExplorerApp() :
  xiiApplication("Shader Explorer"), m_bStuffChanged(false)
{
}

xiiApplication::Execution xiiShaderExplorerApp::Run()
{
  m_pWindow->ProcessWindowMessages();

  if (g_bWindowResized)
  {
    g_bWindowResized = false;
    UpdateSwapChain();
  }

  if (m_pWindow->m_bCloseRequested || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
    return Execution::Quit;

  // make sure time goes on
  xiiClock::GetGlobalClock()->Update();

  // update all input state
  xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

  // make sure telemetry is sent out regularly
  xiiTelemetry::PerFrameUpdate();

  // mouse look
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

    m_pCamera->RotateLocally(xiiAngle::Radian(0.0), xiiAngle::Radian(mouseMotion.y), xiiAngle::Radian(0.0));
    m_pCamera->RotateGlobally(xiiAngle::Radian(0.0), xiiAngle::Radian(mouseMotion.x), xiiAngle::Radian(0.0));
  }
  else
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
  }

  // turn camera with keys
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

    m_pCamera->RotateLocally(xiiAngle::Radian(0.0), xiiAngle::Radian(mouseMotion.y), xiiAngle::Radian(0.0));
    m_pCamera->RotateGlobally(xiiAngle::Radian(0.0), xiiAngle::Radian(mouseMotion.x), xiiAngle::Radian(0.0));
  }

  // movement
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

  m_bStuffChanged = false;
  m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiShaderExplorerApp::OnFileChanged, this));
  if (m_bStuffChanged)
  {
    xiiResourceManager::ReloadAllResources(false);
  }

  // do the rendering
  {
    // Before starting to render in a frame call this function
    m_pDevice->BeginFrame();

    m_pDevice->BeginPipeline("ShaderExplorer", m_hSwapChain);

    // Must always retrieve the current swapchain render target
    xiiGALPass*                  pGALPass          = m_pDevice->BeginPass("xiiShaderExplorerMainPass");
    const xiiGALSwapChain*       pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    xiiGALRenderTargetViewHandle hBBRTV            = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetRenderTargets().m_hRTs[0]);
    xiiGALRenderTargetViewHandle hBBDSV            = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

    xiiGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    renderingSetup.m_bClearDepth             = true;
    renderingSetup.m_bClearStencil           = true;

    xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(pGALPass, renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight));

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
    m_pDevice->EndPass(pGALPass);

    m_pDevice->EndPipeline(m_hSwapChain);

    m_pDevice->EndFrame();
    xiiRenderContext::GetDefaultInstance()->ResetContextState();
  }

  // needs to be called once per frame
  xiiResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  xiiTaskSystem::FinishFrameTasks();

  return xiiApplication::Execution::Continue;
}

void xiiShaderExplorerApp::AfterCoreSystemsStartup()
{
  m_pCamera = XII_DEFAULT_NEW(xiiCamera);
  m_pCamera->LookAt(xiiVec3(3, 3, 1.5), xiiVec3(0, 0, 0), xiiVec3(0, 1, 0));
  m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);

  xiiStringBuilder sProjectDir = ">sdk/Data/Samples/ShaderExplorer";
  xiiStringBuilder sProjectDirResolved;
  xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

  xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  XII_VERIFY(m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded(), "Failed to watch project directory");

  xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites).IgnoreResult();
  xiiFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", xiiFileSystem::AllowWrites).IgnoreResult();                               // writing to the binary directory
  xiiFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", xiiFileSystem::AllowWrites).IgnoreResult();                  // for shader files
  xiiFileSystem::AddDataDirectory(">user/XII/Projects/ShaderExplorer", "AppData", "appdata", xiiFileSystem::AllowWrites).IgnoreResult(); // app user data

  xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
  xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  constexpr const char* szDefaultRenderer = "D3D11";
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
#  error Renderer not implemented on platform
#endif

  constexpr const char* szDefaultLibraryName = "xiiRendererDiligent";
  xiiGALDeviceFactory::ConfigureLibraryName("D3D11", szDefaultLibraryName);
  xiiGALDeviceFactory::ConfigureLibraryName("D3D12", szDefaultLibraryName);
  xiiGALDeviceFactory::ConfigureLibraryName("Vulkan", szDefaultLibraryName);

  const char* szRendererName   = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel    = "";
  const char* szShaderCompiler = "";
  xiiGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);

  xiiShaderManager::Configure(szShaderModel, true);
  XII_VERIFY(xiiPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

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
    m_pWindow                              = XII_DEFAULT_NEW(xiiShaderExplorerWindow);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
  }

  // Create a device
  {
    xiiGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = true;

    m_pDevice = xiiGALDeviceFactory::CreateDevice(szRendererName, xiiFoundation::GetDefaultAllocator(), DeviceInit);
    XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", szRendererName);
    XII_VERIFY(m_pDevice->Init() == XII_SUCCESS, "Device init failed!");

    xiiGALDevice::SetDefaultDevice(m_pDevice);
  }

  // now that we have a window and device, tell the engine to initialize the rendering infrastructure
  xiiStartup::StartupHighLevelSystems();

  UpdateSwapChain();

  // Setup Shaders and Materials
  {
    m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Materials/screen.xiiMaterial");

    // Create the mesh that we use for rendering
    CreateScreenQuad();
  }
}

void xiiShaderExplorerApp::BeforeHighLevelSystemsShutdown()
{
  m_pDirectoryWatcher->CloseDirectory();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hMaterial.Invalidate();
  m_hQuadMeshBuffer.Invalidate();
  m_pDevice->DestroySwapChain(m_hSwapChain);

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  xiiStartup::ShutdownHighLevelSystems();

  // now we can destroy the graphics device
  m_pDevice->Shutdown().IgnoreResult();

  XII_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->Destroy().IgnoreResult();
  XII_DEFAULT_DELETE(m_pWindow);

  m_pCamera.Clear();
  m_pDirectoryWatcher.Clear();
}

void xiiShaderExplorerApp::UpdateSwapChain()
{
  // Create a Swapchain
  if (m_hSwapChain.IsInvalidated())
  {
    xiiGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow            = m_pWindow;
    swapChainDesc.m_SampleCount        = xiiGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots  = true;
    swapChainDesc.m_InitialPresentMode = xiiGALPresentMode::VSync;
    m_hSwapChain                       = xiiGALWindowSwapChain::Create(swapChainDesc);
  }
  else
  {
    m_pDevice->UpdateSwapChain(m_hSwapChain, xiiGALPresentMode::VSync).IgnoreResult();
  }

  if (!m_hSwapChain.IsInvalidated() && !m_hDepthStencilTexture.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();
  }
  // Create depth texture
  {
    xiiGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth             = g_uiWindowWidth;
    texDesc.m_uiHeight            = g_uiWindowHeight;
    texDesc.m_Format              = xiiGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }
}

void xiiShaderExplorerApp::CreateScreenQuad()
{
  xiiGeometry             geom;
  xiiGeometry::GeoOptions opt;
  opt.m_Color = xiiColor::Black;
  geom.AddRectXY(xiiVec2(2, 2), 1, 1, opt);

  xiiMeshBufferResourceDescriptor desc;
  desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);

  desc.AllocateStreams(geom.GetVertices().GetCount(), xiiGALPrimitiveTopology::Triangles, geom.GetPolygons().GetCount() * 2);

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

void xiiShaderExplorerApp::OnFileChanged(const char* filename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
{
  if (action == xiiDirectoryWatcherAction::Modified && type == xiiDirectoryWatcherType::File)
  {
    xiiLog::Info("The file {0} was modified", filename);
    m_bStuffChanged = true;
  }
}

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderExplorerApp);
