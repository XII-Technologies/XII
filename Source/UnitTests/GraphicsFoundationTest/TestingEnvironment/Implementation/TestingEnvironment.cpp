/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

namespace
{
  xiiResult xiiConfigureGPUTestDataDirectories()
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

    return XII_SUCCESS;
  }

  void xiiShutdownGPUTestDataDirectories()
  {
    xiiFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
    xiiFileSystem::RemoveDataDirectoryGroup("UnitTestData");
    xiiFileSystem::RemoveDataDirectoryGroup("Base");
    xiiFileSystem::RemoveDataDirectoryGroup("ShaderCache");
  }
} // namespace

xiiGPUTestingEnvironment::xiiGPUTestingEnvironment() = default;

xiiGPUTestingEnvironment::~xiiGPUTestingEnvironment()
{
  Shutdown();
}

xiiResult xiiGPUTestingEnvironment::Initialize()
{
  XII_SUCCEED_OR_RETURN(xiiConfigureGPUTestDataDirectories());

  xiiGALDeviceCreationDescription description;
  description.m_DeviceFeatures.m_WireframeFill                      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_MultithreadedResourceCreation      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ComputeShaders                     = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_GeometryShaders                    = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_Tessellation                       = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_MeshShaders                        = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_RayTracing                         = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_BindlessResources                  = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_OcclusionQueries                   = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_BinaryOcclusionQueries             = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TimestampQueries                   = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_PipelineStatisticsQueries          = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_DurationQueries                    = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_DepthBiasClamp                     = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_DepthClamp                         = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_IndependentBlend                   = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_DualSourceBlend                    = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_MultiViewport                      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TextureCompressionBC               = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_PixelUAVWritesAndAtomics           = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TextureUAVExtendedFormats          = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ShaderFloat16                      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ResourceBuffer16BitAccess          = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_UniformBuffer16BitAccess           = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ShaderInputOutput16                = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ShaderInt8                         = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ResourceBuffer8BitAccess           = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_UniformBuffer8BitAccess            = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ShaderResourceRuntimeArray         = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_WaveOperation                      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_InstanceDataStepRate               = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_NativeFence                        = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TileShaders                        = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TransferQueueTimestampQueries      = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_VariableRateShading                = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_SparseResources                    = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_SubpassFramebufferFetch            = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_TextureComponentSwizzle            = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_DepthStencilResolve                = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ExternalMemory                     = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ExternalSemaphore                  = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ExternalFence                      = xiiGALDeviceFeatureState::Optional;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  description.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
  description.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
  constexpr const char* szDefaultGraphicsAPI = "D3D12";
#else
  constexpr const char* szDefaultGraphicsAPI = "";
#endif

  m_sImplementationName         = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
  xiiStringView sShaderModel    = {};
  xiiStringView sShaderCompiler = {};
  xiiGALDeviceFactory::GetShaderModelAndCompiler(m_sImplementationName, sShaderModel, sShaderCompiler);

  xiiGALShaderManager::Configure(sShaderModel, true);
  XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found.", sShaderCompiler);
  m_sShaderModel = sShaderModel;

  m_pDevice = xiiGALDeviceFactory::CreateDevice(m_sImplementationName, xiiFoundation::GetDefaultAllocator(), description);
  if (m_pDevice == nullptr || m_pDevice->Initialize().Failed())
  {
    m_pDevice.Clear();
    return XII_FAILURE;
  }

  xiiStringBuilder sDebugName("GraphicsFoundationTest ", m_sImplementationName, " Device");
  m_pDevice->SetDebugName(sDebugName);

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironment::Shutdown()
{
  if (m_pDevice != nullptr)
  {
    m_pDevice->WaitIdle();
    m_pDevice.Clear();
  }

  xiiShutdownGPUTestDataDirectories();
}

xiiUniquePtr<xiiWindowBase> xiiGPUTestingEnvironment::CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle)
{
  XII_IGNORE_UNUSED(uiWidth);
  XII_IGNORE_UNUSED(uiHeight);
  XII_IGNORE_UNUSED(sTitle);
  return {};
}
