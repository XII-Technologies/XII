#include <GraphicsTest/GraphicsTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <TestFramework/Framework/TestFramework.h>

class xiiGraphicsTestWindow : public xiiWindow
{
public:
  xiiGraphicsTestWindow() :
    xiiWindow()
  {
    m_bCloseRequested = false;
    m_uiWidth         = m_CreationDescription.m_Resolution.width;
    m_uiHeight        = m_CreationDescription.m_Resolution.height;
  }

  virtual void       OnClickClose() override { m_bCloseRequested = true; }
  virtual xiiSizeU32 GetClientAreaSize() const override { return xiiSizeU32(m_uiWidth, m_uiHeight); }
  virtual void       OnResize(const xiiSizeU32& newWindowSize) override
  {
    if (m_uiWidth != newWindowSize.width || m_uiHeight != newWindowSize.height)
    {
      m_uiWidth  = newWindowSize.width;
      m_uiHeight = newWindowSize.height;
    }
  }

  bool      m_bCloseRequested;
  xiiUInt32 m_uiWidth;
  xiiUInt32 m_uiHeight;
};

XII_IMPLEMENT_SINGLETON(xiiGPUTestingEnvironmentVulkan);

xiiGPUTestingEnvironmentVulkan::xiiGPUTestingEnvironmentVulkan() :
  m_SingletonRegistrar(this)
{
}

xiiResult xiiGPUTestingEnvironmentVulkan::Initialize()
{
  {
    xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

    xiiStringBuilder sBaseDir = ">sdk/Data/Base/";
    xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites)); // for shader files

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sBaseDir, "Base"));

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiDataDirUsage::AllowWrites));

    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

    sReadDir.Set(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  }

  // Create device.
  if (m_pDevice == nullptr)
  {
    xiiStringView                   sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill = xiiGALDeviceFeatureState::Optional;
    // Set other feature flags optional to be permissive for test environment.

    m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    if (!m_pDevice || m_pDevice->Initialize().Failed())
    {
      xiiLog::Error("Failed to create/initialize device for testing.");
      m_pDevice.Clear();
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentVulkan::Shutdown()
{
  // Destroy swapchain and window before device shutdown.
  DestroySwapChain();
  DestroyWindow();

  if (m_pDevice)
  {
    m_pDevice.Clear();
  }
}

xiiResult xiiGPUTestingEnvironmentVulkan::CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY)
{
  if (m_pWindow)
    return XII_SUCCESS;

  m_pWindow = XII_DEFAULT_NEW(xiiGraphicsTestWindow);

  xiiWindowCreationDescription windowDescription;
  windowDescription.m_Resolution.width  = uiResolutionX;
  windowDescription.m_Resolution.height = uiResolutionY;
  windowDescription.m_Title             = "GraphicsTestWindow";

  if (m_pWindow->Initialize(windowDescription).Failed())
  {
    m_pWindow.Clear();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentVulkan::DestroyWindow()
{
  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }
}

xiiResult xiiGPUTestingEnvironmentVulkan::CreateSwapChainForWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY)
{
  if (!m_pDevice)
    return XII_FAILURE;

  if (!m_pWindow)
  {
    XII_SUCCEED_OR_RETURN(CreateWindow(uiResolutionX, uiResolutionY));
  }

  if (m_pSwapChain)
    return XII_SUCCESS;

  xiiGALSwapChainCreationDescription swapChainDescription;
  swapChainDescription.m_pWindow           = m_pWindow.Borrow();
  swapChainDescription.m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB;

  m_pSwapChain = m_pDevice->CreateSwapChain(swapChainDescription);
  if (!m_pSwapChain)
    return XII_FAILURE;

  // Create a depth texture matching the swapchain.
  xiiGALTextureCreationDescription textureDescription = xiiGALTextureCreationDescription();
  textureDescription.m_Type                           = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Size.width                     = uiResolutionX;
  textureDescription.m_Size.height                    = uiResolutionY;
  textureDescription.m_Format                         = xiiGALResourceFormat::D24UNormalizedS8UInt;

  m_pDepthStencilTexture = m_pDevice->CreateTexture(textureDescription);

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentVulkan::DestroySwapChain()
{
  if (m_pSwapChain)
  {
    m_pSwapChain.Clear();
  }
  m_pDepthStencilTexture.Clear();
}

void xiiGPUTestingEnvironmentVulkan::BeginFrame()
{
  if (m_pDevice)
  {
    m_pDevice->BeginFrame();
  }
}

void xiiGPUTestingEnvironmentVulkan::EndFrame()
{
  if (m_pDevice)
  {
    m_pDevice->EndFrame();
  }
}

void xiiGPUTestingEnvironmentVulkan::Present()
{
  if (m_pSwapChain)
  {
    m_pSwapChain->Present();
  }
}

void xiiGPUTestingEnvironmentVulkan::ProcessWindowMessages()
{
  if (m_pWindow)
  {
    m_pWindow->ProcessWindowMessages();
  }
}
