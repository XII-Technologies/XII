#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/ImageConversion.h>

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
#include <GraphicsCore/Textures/TextureLoader.h>

// Constant buffer definition is shared between shader code and C++
#include <GraphicsCore/../../../Data/Samples/TextureSample/Shaders/SampleConstantBuffer.h>

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;
static bool      g_bWindowResized = false;

class xiiTextureSample : public xiiWindow
{
public:
  xiiTextureSample() :
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

class CustomTextureResourceLoader : public xiiTextureResourceLoader
{
public:
  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
};

const xiiInt32 g_iMaxHalfExtent         = 20;
const bool     g_bForceImmediateLoading = false;
const bool     g_bPreloadAllTextures    = false;

// A simple application that creates a window.
class xiiTextureSampleApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiTextureSampleApp() :
    xiiApplication("Texture Sample")
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
    if (xiiInputManager::GetInputActionState("Main", "MouseDown") == xiiKeyState::Down)
    {
      m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
      m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::ClipToPosition);

      float       fInputValue = 0.0f;
      const float fMouseSpeed = 0.5f;

      if (xiiInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != xiiKeyState::Up)
        m_vCameraPosition.x -= fInputValue * fMouseSpeed;
      if (xiiInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != xiiKeyState::Up)
        m_vCameraPosition.x += fInputValue * fMouseSpeed;
      if (xiiInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != xiiKeyState::Up)
        m_vCameraPosition.y += fInputValue * fMouseSpeed;
      if (xiiInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != xiiKeyState::Up)
        m_vCameraPosition.y -= fInputValue * fMouseSpeed;
    }
    else
    {
      m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
      m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
    }

    // Reload resources if modified
    {
      m_bFileModified = false;
      m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiTextureSampleApp::OnFileChanged, this));

      if (m_bFileModified)
      {
        xiiResourceManager::ReloadAllResources(false);
      }
    }

    // Perform rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      m_pDevice->BeginPipeline("TextureSample", m_hSwapChain);

      // Must always retrieve the current swapchain render target
      const xiiGALSwapChain*  pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
      xiiGALTextureViewHandle hBBRTV            = m_pDevice->GetTexture(pPrimarySwapChain->GetBackBufferTexture())->GetDefaultView(xiiGALTextureViewType::RenderTarget);
      xiiGALTextureViewHandle hBBDSV            = m_pDevice->GetTexture(m_hDepthStencilTexture)->GetDefaultView(xiiGALTextureViewType::DepthStencil);

      // Clear attachments.
      {
        xiiGALRenderingSetup renderingSetup;
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
        renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
        renderingSetup.m_bClearDepth             = true;

        xiiGALCommandList* pCommandList = xiiRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), "xiiTextureSampleMainPass");
        xiiRenderContext::GetDefaultInstance()->BeginRenderPass();
        xiiRenderContext::GetDefaultInstance()->EndRenderPass();
        xiiRenderContext::GetDefaultInstance()->EndRendering();
      }

      {
        xiiGALRenderingSetup renderingSetup;
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
        renderingSetup.m_uiRenderTargetClearMask = 0x0U;
        renderingSetup.m_bClearDepth             = false;

        xiiGALCommandList* pCommandList = xiiRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight));

        xiiMat4 Proj = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -(float)g_uiWindowWidth * 0.5f, m_vCameraPosition.x + (float)g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float)g_uiWindowHeight * 0.5f, m_vCameraPosition.y + (float)g_uiWindowHeight * 0.5f, -1.0f, 1.0f);

        xiiRenderContext::GetDefaultInstance()->BindConstantBuffer(XII_STRINGIZE(xiiTextureSampleConstants), m_hSampleConstants);
        xiiRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);

        xiiMat4 mTransform = xiiMat4::IdentityMatrix();

        xiiInt32 iLeftBound  = (xiiInt32)xiiMath::Floor((m_vCameraPosition.x - g_uiWindowWidth * 0.5f) / 100.0f);
        xiiInt32 iLowerBound = (xiiInt32)xiiMath::Floor((m_vCameraPosition.y - g_uiWindowHeight * 0.5f) / 100.0f);
        xiiInt32 iRightBound = (xiiInt32)xiiMath::Ceil((m_vCameraPosition.x + g_uiWindowWidth * 0.5f) / 100.0f) + 1;
        xiiInt32 iUpperBound = (xiiInt32)xiiMath::Ceil((m_vCameraPosition.y + g_uiWindowHeight * 0.5f) / 100.0f) + 1;

        iLeftBound  = xiiMath::Max(iLeftBound, -g_iMaxHalfExtent);
        iRightBound = xiiMath::Min(iRightBound, g_iMaxHalfExtent);
        iLowerBound = xiiMath::Max(iLowerBound, -g_iMaxHalfExtent);
        iUpperBound = xiiMath::Min(iUpperBound, g_iMaxHalfExtent);

        xiiStringBuilder sResourceName;

        for (xiiInt32 y = iLowerBound; y < iUpperBound; ++y)
        {
          for (xiiInt32 x = iLeftBound; x < iRightBound; ++x)
          {
            mTransform.SetTranslationVector(xiiVec3((float)x * 100.0f, (float)y * 100.0f, 0));

            // Update the constant buffer
            {
              xiiTextureSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();
              cb.ModelMatrix                = mTransform;
              cb.ViewProjectionMatrix       = Proj;
            }

            sResourceName.SetPrintf("Loaded_%+03i_%+03i_D", x, y);

            xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);

            // force immediate loading
            if (g_bForceImmediateLoading)
              xiiResourceLock<xiiTexture2DResource> l(hTexture, xiiResourceAcquireMode::BlockTillLoaded);

            xiiRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hTexture);
            xiiRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
            xiiRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
          }
        }

        xiiRenderContext::GetDefaultInstance()->EndRendering();
      }

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
    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/TextureSample";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    // setup the 'asset management system'
    {
      // which redirection table to search
      xiiDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.xiiAsset";
      // which platform assets to use
      xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
    }

    xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites).IgnoreResult();
    xiiFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", xiiFileSystem::AllowWrites).IgnoreResult();                              // writing to the binary directory
    xiiFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", xiiFileSystem::AllowWrites).IgnoreResult();                 // for shader files
    xiiFileSystem::AddDataDirectory(">user/XII/Projects/TextureSample", "AppData", "appdata", xiiFileSystem::AllowWrites).IgnoreResult(); // app user data

    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
    xiiTelemetry::SetServerName("Texture Sample");

    // Activate xiiTelemetry such that the inspector plugin can use the network connection.
    xiiTelemetry::CreateServer();

    // Load the inspector plugin.
    // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
    // When you are using xiiApplication, this is done automatically.
    xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();
#endif

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

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosX");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosX;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegX");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegX;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "MovePosY");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMovePosY;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "MoveNegY");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseMoveNegY;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "MouseDown");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseButton0;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "MouseDown", cfg, true);
    }

    // Create a window for rendering
    {
      xiiWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
      WindowCreationDesc.m_Title             = "Texture Sample";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(xiiTextureSample);
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
      // The shader (referenced by the material) also defines the render pipeline state, such as backface-culling and depth-testing

      m_hMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Materials/Texture.xiiMaterial");

      // Create the mesh that we use for rendering
      CreateSquareMesh();
    }

    // Setup default resources
    {
      xiiTexture2DResourceHandle hFallback = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/Reference_D.dds");
      xiiTexture2DResourceHandle hMissing  = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/MissingTexture_D.dds");

      xiiResourceManager::SetResourceTypeLoadingFallback<xiiTexture2DResource>(hFallback);
      xiiResourceManager::SetResourceTypeMissingFallback<xiiTexture2DResource>(hMissing);

      // Redirect all texture load operations through our custom loader, so that we can duplicate the single source texture
      // that we have as often as we like (to waste memory)
      xiiResourceManager::SetResourceTypeLoader<xiiTexture2DResource>(&m_TextureResourceLoader);
    }

    // Setup constant buffer that this sample uses
    {
      m_hSampleConstants = xiiRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
    }

    // Pre-allocate all textures
    {
      // We only do this to be able to see the unloaded resources in the xiiInspector
      // This does NOT preload the resources

      xiiStringBuilder sResourceName;
      for (xiiInt32 y = -g_iMaxHalfExtent; y < g_iMaxHalfExtent; ++y)
      {
        for (xiiInt32 x = -g_iMaxHalfExtent; x < g_iMaxHalfExtent; ++x)
        {
          sResourceName.SetPrintf("Loaded_%+03i_%+03i_D", x, y);

          xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);

          if (g_bPreloadAllTextures)
          {
            xiiResourceManager::PreloadResource(hTexture);
          }
        }
      }
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

  void CreateSquareMesh()
  {
    struct Vertex
    {
      xiiVec3 Position;
      xiiVec2 TexCoord0;
    };

    xiiGeometry             geom;
    xiiGeometry::GeoOptions opt;
    opt.m_Color = xiiColor::Black;
    geom.AddRectXY(xiiVec2(100, 100), 1, 1, opt);

    xiiDynamicArray<Vertex>    Vertices;
    xiiDynamicArray<xiiUInt16> Indices;

    Vertices.Reserve(geom.GetVertices().GetCount());
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALTextureFormat::RGB32Float);
    desc.AddStream(xiiGALInputLayoutSemantic::TexCoord0, xiiGALTextureFormat::RG32Float);

    desc.AllocateStreams(geom.GetVertices().GetCount(), xiiGALPrimitiveTopology::TriangleList, geom.GetPolygons().GetCount() * 2);

    for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      xiiVec2 tc(geom.GetVertices()[v].m_vPosition.x / 100.0f, geom.GetVertices()[v].m_vPosition.y / -100.0f);
      tc += xiiVec2(0.5f);

      desc.SetVertexData<xiiVec3>(0, v, geom.GetVertices()[v].m_vPosition);
      desc.SetVertexData<xiiVec2>(1, v, tc);
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
  xiiTextureSample* m_pWindow = nullptr;

  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiVec2 m_vCameraPosition = xiiVec2::ZeroVector();

  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;

  CustomTextureResourceLoader                          m_TextureResourceLoader;
  xiiConstantBufferStorageHandle                       m_hSampleConstants;
  xiiConstantBufferStorage<xiiTextureSampleConstants>* m_pSampleConstantBuffer;
};

xiiResourceLoadData CustomTextureResourceLoader::OpenDataStream(const xiiResource* pResource)
{
  xiiString sFileToLoad = pResource->GetResourceID();

  if (sFileToLoad.StartsWith("Loaded"))
  {
    sFileToLoad = "Textures/Loaded_D.dds"; // redirect all "Loaded_XYZ" files to the same source file
  }

  // the entire rest is copied from xiiTextureResourceLoader

  LoadedData* pData = XII_DEFAULT_NEW(LoadedData);

  xiiResourceLoadData res;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  {
    xiiFileReader File;
    if (File.Open(sFileToLoad).Failed())
      return res;

    xiiFileStats stat;
    if (xiiOSFile::GetFileStats(File.GetFilePathAbsolute(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
  }
#endif


  if (pData->m_Image.LoadFrom(sFileToLoad).Failed())
    return res;

  if (pData->m_Image.GetImageFormat() == xiiImageFormat::B8G8R8_UNORM)
  {
    xiiImageConversion::Convert(pData->m_Image, pData->m_Image, xiiImageFormat::B8G8R8A8_UNORM).IgnoreResult();
  }

  xiiMemoryStreamWriter w(&pData->m_Storage);

  xiiImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(xiiImage*)).IgnoreResult();

  /// This is a hack to get the SRGB information for the texture

  const xiiStringBuilder sName = xiiPathUtils::GetFileName(sFileToLoad);

  bool bIsFallback = false;
  bool bSRGB       = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

  w << bIsFallback;
  w << bSRGB;

  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiTextureSampleApp);
