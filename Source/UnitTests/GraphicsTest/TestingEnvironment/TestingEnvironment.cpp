#include <GraphicsTest/GraphicsTestPCH.h>

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

XII_IMPLEMENT_SINGLETON(xiiGPUTestingEnvironmentD3D11);

xiiGPUTestingEnvironmentD3D11::xiiGPUTestingEnvironmentD3D11() :
  m_SingletonRegistrar(this)
{
}

xiiResult xiiGPUTestingEnvironmentD3D11::Initialize()
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
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_SeparablePrograms                  = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderResourceQueries              = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill                      = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_MultithreadedResourceCreation      = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_ComputeShaders                     = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_GeometryShaders                    = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_Tessellation                       = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_MeshShaders                        = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_RayTracing                         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_BindlessResources                  = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_OcclusionQueries                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_BinaryOcclusionQueries             = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_TimestampQueries                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_PipelineStatisticsQueries          = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_DurationQueries                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_DepthBiasClamp                     = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_DepthClamp                         = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_IndependentBlend                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_DualSourceBlend                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_MultiViewport                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureCompressionBC               = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_PixelUAVWritesAndAtomics           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureUAVExtendedFormats          = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderFloat16                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer16BitAccess          = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer16BitAccess           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderInputOutput16                = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderInt8                         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer8BitAccess           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer8BitAccess            = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_WaveOperation                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_InstanceDataStepRate               = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_NativeFence                        = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_TileShaders                        = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_VariableRateShading                = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_SparseResources                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_SubpassFramebufferFetch            = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureComponentSwizzle            = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::All;
#elif XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

    constexpr const char* szDefaultGraphicsAPI = "D3D11";
    xiiStringView         sGraphicsAPIName     = szDefaultGraphicsAPI;
    xiiStringView         sShaderModel         = {};
    xiiStringView         sShaderCompiler      = {};
    xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

#if TODO_ENABLE
    xiiShaderManager::Configure(sShaderModel, true);
    XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);
#endif

    m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sGraphicsAPIName);
    XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

    m_pDevice->SetDebugName("Master Graphics Device (D3D11)");
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentD3D11::Shutdown()
{
}

xiiResult xiiGPUTestingEnvironmentD3D11::CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Device has not yet been initialized. Ensure to call Initialize().");

  if (m_pWindow != nullptr && m_pWindow->GetClientAreaSize() != xiiSizeU32(uiResolutionX, uiResolutionY))
  {
    DestroyWindow();
  }

  if (m_pWindow == nullptr)
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_Title             = "XII - Test";
    WindowCreationDesc.m_bShowMouseCursor  = true;
    WindowCreationDesc.m_bClipMouseCursor  = false;
    WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
    m_pWindow                              = XII_DEFAULT_NEW(xiiWindow);

    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return XII_FAILURE;
  }

  // Create a window for the swapchain.
  if (m_hSwapChain.IsInvalidated())
  {
    xiiGALSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow               = m_pWindow;
    swapChainDesc.m_Resolution.width      = uiResolutionX;
    swapChainDesc.m_Resolution.height     = uiResolutionY;
    swapChainDesc.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    swapChainDesc.m_UsageFlags                 = xiiGALSwapChainUsageFlags::RenderTarget;
    swapChainDesc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
    swapChainDesc.m_uiBufferCount         = 2U;
    swapChainDesc.m_fDefaultDepthValue    = 1.0f;
    swapChainDesc.m_uiDefaultStencilValue = 0U;

    m_hSwapChain = m_pDevice->CreateSwapChain(swapChainDesc);

    if (m_hSwapChain.IsInvalidated())
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentD3D11::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }

    m_pDevice->WaitIdle();
  }

  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);
  }
}

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
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_SeparablePrograms                  = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderResourceQueries              = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill                      = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_MultithreadedResourceCreation      = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_ComputeShaders                     = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_GeometryShaders                    = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_Tessellation                       = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_MeshShaders                        = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_RayTracing                         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_BindlessResources                  = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_OcclusionQueries                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_BinaryOcclusionQueries             = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_TimestampQueries                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_PipelineStatisticsQueries          = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_DurationQueries                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_DepthBiasClamp                     = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_DepthClamp                         = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_IndependentBlend                   = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_DualSourceBlend                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_MultiViewport                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureCompressionBC               = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_PixelUAVWritesAndAtomics           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureUAVExtendedFormats          = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderFloat16                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer16BitAccess          = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer16BitAccess           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderInputOutput16                = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderInt8                         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer8BitAccess           = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer8BitAccess            = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray         = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_WaveOperation                      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_InstanceDataStepRate               = xiiGALDeviceFeatureState::Enabled;
    deviceCreationDescription.m_DeviceFeatures.m_NativeFence                        = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_TileShaders                        = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries      = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_VariableRateShading                = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_SparseResources                    = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_SubpassFramebufferFetch            = xiiGALDeviceFeatureState::Disabled;
    deviceCreationDescription.m_DeviceFeatures.m_TextureComponentSwizzle            = xiiGALDeviceFeatureState::Optional;
    deviceCreationDescription.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::All;
#elif XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
    deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

    constexpr const char* szDefaultGraphicsAPI = "Vulkan";
    xiiStringView         sGraphicsAPIName     = szDefaultGraphicsAPI;
    xiiStringView         sShaderModel         = {};
    xiiStringView         sShaderCompiler      = {};
    xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

#if TODO_ENABLE
    xiiShaderManager::Configure(sShaderModel, true);
    XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);
#endif

    m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sGraphicsAPIName);
    XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

    m_pDevice->SetDebugName("Master Graphics Device (Vulkan)");
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentVulkan::Shutdown()
{
  DestroyWindow();

  if (m_pDevice != nullptr)
  {
    m_pDevice->Shutdown().IgnoreResult();
  }
}

xiiResult xiiGPUTestingEnvironmentVulkan::CreateWindow(xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Device has not yet been initialized. Ensure to call Initialize().");

  if (m_pWindow != nullptr && m_pWindow->GetClientAreaSize() != xiiSizeU32(uiResolutionX, uiResolutionY))
  {
    DestroyWindow();
  }

  if (m_pWindow == nullptr)
  {
    xiiWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width  = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_Title             = "XII - Test";
    WindowCreationDesc.m_bShowMouseCursor  = true;
    WindowCreationDesc.m_bClipMouseCursor  = false;
    WindowCreationDesc.m_WindowMode        = xiiWindowMode::WindowResizable;
    m_pWindow                              = XII_DEFAULT_NEW(xiiWindow);

    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return XII_FAILURE;
  }

  // Create a window for the swapchain.
  if (m_hSwapChain.IsInvalidated())
  {
    xiiGALSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow               = m_pWindow;
    swapChainDesc.m_Resolution.width      = uiResolutionX;
    swapChainDesc.m_Resolution.height     = uiResolutionY;
    swapChainDesc.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    swapChainDesc.m_UsageFlags                 = xiiGALSwapChainUsageFlags::RenderTarget;
    swapChainDesc.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
    swapChainDesc.m_uiBufferCount         = 2U;
    swapChainDesc.m_fDefaultDepthValue    = 1.0f;
    swapChainDesc.m_uiDefaultStencilValue = 0U;

    m_hSwapChain = m_pDevice->CreateSwapChain(swapChainDesc);

    if (m_hSwapChain.IsInvalidated())
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentVulkan::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }

    m_pDevice->WaitIdle();
  }

  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    XII_DEFAULT_DELETE(m_pWindow);
  }
}
