#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

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

class MinimalSampleWindow : public xiiWindow
{
public:
  MinimalSampleWindow() :
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

class MinimalSample : public xiiApplication
{
private:
  MinimalSampleWindow* m_pWindow = nullptr;

  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

public:
  typedef xiiApplication SUPER;

  MinimalSample() :
    xiiApplication("MinimalSampleApp")
  {
  }

  virtual void AfterCoreSystemsStartup() override
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

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

    // Create a window for rendering
    {
      xiiWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width  = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
      WindowCreationDesc.m_Title             = "Hello Graphics Abstraction";
      WindowCreationDesc.m_bShowMouseCursor  = true;
      WindowCreationDesc.m_bClipMouseCursor  = false;
      WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
      m_pWindow                              = XII_DEFAULT_NEW(MinimalSampleWindow);
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
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void BeforeHighLevelSystemsShutdown()
  {
    // Now shutdown the graphics device
    m_pDevice->Shutdown().IgnoreResult();
    XII_DEFAULT_DELETE(m_pDevice);

    // Finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);
  }

  virtual xiiApplication::Execution Run() override
  {
    return Execution::Quit;
  }
};


XII_CONSOLEAPP_ENTRY_POINT(MinimalSample);
