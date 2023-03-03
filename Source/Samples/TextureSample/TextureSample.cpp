#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/ImageConversion.h>

// Constant buffer definition is shared between shader code and C++
#include <RendererCore/../../../Data/Samples/TextureSample/Shaders/SampleConstantBuffer.h>

class TextureSampleWindow : public xiiWindow
{
public:
  TextureSampleWindow() :
    xiiWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void OnClickClose() override { m_bCloseRequested = true; }

  bool m_bCloseRequested;
};

static xiiUInt32 g_uiWindowWidth  = 960;
static xiiUInt32 g_uiWindowHeight = 540;

class CustomTextureResourceLoader : public xiiTextureResourceLoader
{
public:
  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
};

const xiiInt32 g_iMaxHalfExtent         = 20;
const bool     g_bForceImmediateLoading = false;
const bool     g_bPreloadAllTextures    = false;

class TextureSample : public xiiApplication
{
  CustomTextureResourceLoader                          m_TextureResourceLoader;
  xiiConstantBufferStorageHandle                       m_hSampleConstants;
  xiiConstantBufferStorage<xiiTextureSampleConstants>* m_pSampleConstantBuffer;

public:
  typedef xiiApplication SUPER;

  TextureSample() :
    xiiApplication("Texture Sample")
  {
    m_vCameraPosition.SetZero();
  }

  void AfterCoreSystemsStartup() override
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
    xiiFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", xiiFileSystem::AllowWrites).IgnoreResult();              // writing to the binary directory
    xiiFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", xiiFileSystem::AllowWrites).IgnoreResult(); // for shader files
    xiiFileSystem::AddDataDirectory(">user/XII/Projects/TextureSample", "AppData", "appdata",
                                    xiiFileSystem::AllowWrites)
      .IgnoreResult(); // app user data

    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">sdk/Data/FreeContent", "Shared", "shared").IgnoreResult();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project", xiiFileSystem::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    xiiTelemetry::SetServerName(GetApplicationName());
    xiiTelemetry::CreateServer();
    xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();

#if BUILDSYSTEM_ENABLE_DILIGENT_SUPPORT
    constexpr const char* szDefaultRenderer = "Diligent";
    xiiGraphicsDevice::Default              = xiiGraphicsDevice::Vulkan;
#else
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    constexpr const char* szDefaultRenderer = "DX11";
#  else
#    error Renderer not implemented on platform
#  endif
#endif

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
      m_pWindow                              = XII_DEFAULT_NEW(TextureSampleWindow);
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

    // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
    xiiStartup::StartupHighLevelSystems();

    // Create a Swapchain
    {
      xiiGALWindowSwapChainCreationDescription swapChainDesc;
      swapChainDesc.m_pWindow           = m_pWindow;
      swapChainDesc.m_SampleCount       = xiiGALMSAASampleCount::None;
      swapChainDesc.m_bAllowScreenshots = true;
      m_hSwapChain                      = xiiGALWindowSwapChain::Create(swapChainDesc);

      const xiiGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

      xiiGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth             = g_uiWindowWidth;
      texDesc.m_uiHeight            = g_uiWindowHeight;
      texDesc.m_Format              = xiiGALResourceFormat::D24S8;
      texDesc.m_bCreateRenderTarget = true;

      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    }

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
      m_hSampleConstants = xiiRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer, XII_STRINGIZE(xiiTextureSampleConstants));
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
          sResourceName.Printf("Loaded_%+03i_%+03i_D", x, y);

          xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);

          if (g_bPreloadAllTextures)
            xiiResourceManager::PreloadResource(hTexture);
        }
      }
    }
  }


  Execution Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
      return Execution::Quit;

    // make sure time goes on
    xiiClock::GetGlobalClock()->Update();

    // update all input state
    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

    if (xiiInputManager::GetInputActionState("Main", "MouseDown") == xiiKeyState::Down)
    {
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

    // update all input state
    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

    // make sure telemetry is sent out regularly
    xiiTelemetry::PerFrameUpdate();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      m_pDevice->BeginPipeline("TextureSample", m_hSwapChain);

      // Must always retrieve the current swapchain render target
      xiiGALPass*                  pGALPass          = m_pDevice->BeginPass("xiiTextureSampleMainPass");
      const xiiGALSwapChain*       pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
      xiiGALRenderTargetViewHandle hBBRTV            = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetRenderTargets().m_hRTs[0]);
      xiiGALRenderTargetViewHandle hBBDSV            = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

      xiiGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
      renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
      renderingSetup.m_bClearDepth             = true;

      xiiGALRenderCommandEncoder* pCommandEncoder = xiiRenderContext::GetDefaultInstance()->BeginRendering(pGALPass, renderingSetup, xiiRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight));

      xiiMat4 Proj = xiiGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -(float)g_uiWindowWidth * 0.5f, m_vCameraPosition.x + (float)g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float)g_uiWindowHeight * 0.5f, m_vCameraPosition.y + (float)g_uiWindowHeight * 0.5f, -1.0f, 1.0f);

      xiiRenderContext::GetDefaultInstance()->BindConstantBuffer("xiiTextureSampleConstants", m_hSampleConstants);
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

          sResourceName.Printf("Loaded_%+03i_%+03i_D", x, y);

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

  void BeforeCoreSystemsShutdown() override
  {
    // make sure that no textures are continue to be streamed in while the engine shuts down
    xiiResourceManager::EngineAboutToShutdown();

    xiiRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);
    m_hSampleConstants.Invalidate();

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    m_hMaterial.Invalidate();
    m_hQuadMeshBuffer.Invalidate();

    // tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    xiiStartup::ShutdownHighLevelSystems();

    xiiResourceManager::FreeAllUnusedResources();

    m_pDevice->DestroySwapChain(m_hSwapChain);

    // now we can destroy the graphics device
    m_pDevice->Shutdown().IgnoreResult();

    XII_DEFAULT_DELETE(m_pDevice);

    // finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);

    xiiTelemetry::CloseConnection();
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
    desc.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);
    desc.AddStream(xiiGALVertexAttributeSemantic::TexCoord0, xiiGALResourceFormat::UVFloat);

    desc.AllocateStreams(geom.GetVertices().GetCount(), xiiGALPrimitiveTopology::Triangles, geom.GetPolygons().GetCount() * 2);

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

private:
  TextureSampleWindow* m_pWindow;
  xiiGALDevice*        m_pDevice;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiVec2 m_vCameraPosition;
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

XII_CONSOLEAPP_ENTRY_POINT(TextureSample);
