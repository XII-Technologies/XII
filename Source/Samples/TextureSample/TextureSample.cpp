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
#include <Foundation/Utilities/GraphicsUtils.h>
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
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>

// Constant buffer definition is shared between shader code and C++
#include <GraphicsCore/../../../Data/Samples/TextureSample/Shaders/SampleConstantBuffer.h>

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
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    {
      m_bFileModified = false;
      m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiTextureSampleApp::OnFileChanged, this));

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
        pRenderContext->BindConstantBuffer(XII_PP_STRINGIFY(xiiTextureSampleConstants), m_pSampleConstantBuffer);
        pRenderContext->BindMaterial(m_hMaterial);

        xiiMat4 mProjection = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -(float)g_uiWindowWidth * 0.5f, m_vCameraPosition.x + (float)g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float)g_uiWindowHeight * 0.5f, m_vCameraPosition.y + (float)g_uiWindowHeight * 0.5f, -1.0f, 1.0f);
        xiiMat4 mTransform  = xiiMat4::MakeIdentity();

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

            // Update the constant buffer.
            {
              xiiGALMapHelper<xiiTextureSampleConstants> pTextureSampleConstants(pRenderContext->GetCommandList(), m_pSampleConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
              pTextureSampleConstants->ModelMatrix          = mTransform;
              pTextureSampleConstants->ViewProjectionMatrix = mProjection;
            }

            sResourceName.SetPrintf("Loaded_%+03i_%+03i_D", x, y);

            xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);

            // Force immediate loading.
            if (g_bForceImmediateLoading)
            {
              xiiResourceLock<xiiTexture2DResource> l(hTexture, xiiResourceAcquireMode::BlockTillLoaded);
            }

            pRenderContext->BindTexture2D("DiffuseTexture", hTexture);
            pRenderContext->BindMeshBuffer(m_hQuadMeshBuffer);
            pRenderContext->DrawMeshBuffer().IgnoreResult();
          }
        }
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

    return xiiApplication::Execution::Continue;
  }

  virtual void AfterCoreSystemsStartup() override
  {
#if XII_ENABLED(USE_FILESERVE)
    xiiPlugin::LoadPlugin("xiiFileservePlugin").AssertSuccess("Failed to load FileServe plugin.");
#endif

    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/TextureSample";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();
    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif


    // setup the 'asset management system'
    {
      // which redirection table to search
      xiiDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.xiiAsset";
      // which platform assets to use
      xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
    }

    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

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
      xiiWindowCreationDescription WindowCreationDesc;
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
      xiiGALDeviceCreationDescription deviceCreationDescription;
      deviceCreationDescription.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;

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
      m_pSampleConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(m_pDevice, sizeof(xiiTextureSampleConstants), XII_PP_STRINGIFY(xiiTextureSampleConstants));
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
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB | xiiGALSwapChainUsageFlags::ShaderResource;
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
    geom.AddRect(xiiVec2(100, 100), 1, 1, opt);

    xiiDynamicArray<Vertex>    Vertices;
    xiiDynamicArray<xiiUInt16> Indices;

    Vertices.Reserve(geom.GetVertices().GetCount());
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
    desc.AddStream(xiiGALInputLayoutSemantic::TexCoord0, xiiGALResourceFormat::RG32Float);

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
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher->CloseDirectory();
    m_pDirectoryWatcher.Clear();
#endif

    m_pSampleConstantBuffer.Clear();

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

    // Finally destroy the window.
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_DISABLED(XII_PLATFORM_ANDROID)
    // Shut down telemetry if it was set up.
    xiiTelemetry::CloseConnection();
#endif

    SUPER::BeforeCoreSystemsShutdown();
  }

private:
  xiiUniquePtr<xiiTextureSample> m_pWindow;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiSharedPtr<xiiGALTexture>   m_pDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiVec2 m_vCameraPosition = xiiVec2::MakeZero();

  CustomTextureResourceLoader m_TextureResourceLoader;
  xiiSharedPtr<xiiGALBuffer>  m_pSampleConstantBuffer;

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;
#endif
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
