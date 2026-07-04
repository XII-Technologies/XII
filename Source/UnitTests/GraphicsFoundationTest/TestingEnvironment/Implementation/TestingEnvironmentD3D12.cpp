/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <TestFramework/Framework/TestFramework.h>

XII_IMPLEMENT_SINGLETON(xiiGPUTestingEnvironmentD3D12);

xiiGPUTestingEnvironmentD3D12::xiiGPUTestingEnvironmentD3D12() :
  m_SingletonRegistrar(this)
{
}

xiiGPUTestingEnvironmentD3D12::~xiiGPUTestingEnvironmentD3D12()
{
  Shutdown();
}

xiiResult xiiGPUTestingEnvironmentD3D12::Initialize()
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

  xiiGALDeviceCreationDescription deviceCreationDescription;
  deviceCreationDescription.m_DeviceFeatures.m_WireframeFill                      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_MultithreadedResourceCreation      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ComputeShaders                     = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_GeometryShaders                    = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_Tessellation                       = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_MeshShaders                        = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_RayTracing                         = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_BindlessResources                  = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_OcclusionQueries                   = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_BinaryOcclusionQueries             = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TimestampQueries                   = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_PipelineStatisticsQueries          = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_DurationQueries                    = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_DepthBiasClamp                     = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_DepthClamp                         = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_IndependentBlend                   = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_DualSourceBlend                    = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_MultiViewport                      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TextureCompressionBC               = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_PixelUAVWritesAndAtomics           = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TextureUAVExtendedFormats          = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ShaderFloat16                      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer16BitAccess          = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer16BitAccess           = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ShaderInputOutput16                = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ShaderInt8                         = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ResourceBuffer8BitAccess           = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_UniformBuffer8BitAccess            = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray         = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_WaveOperation                      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_InstanceDataStepRate               = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_NativeFence                        = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TileShaders                        = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries      = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_VariableRateShading                = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_SparseResources                    = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_SubpassFramebufferFetch            = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_TextureComponentSwizzle            = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_DepthStencilResolve                = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ExternalMemory                     = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ExternalSemaphore                  = xiiGALDeviceFeatureState::Optional;
  deviceCreationDescription.m_DeviceFeatures.m_ExternalFence                      = xiiGALDeviceFeatureState::Optional;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
  deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

  {
    constexpr const char* szDefaultGraphicsAPI = "D3D12";

    m_pDevice = xiiGALDeviceFactory::CreateDevice(szDefaultGraphicsAPI, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found.", szDefaultGraphicsAPI);

    XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

    m_pDevice->SetDebugName("Master Graphics Device");
  }

  return XII_SUCCESS;
}

void xiiGPUTestingEnvironmentD3D12::Shutdown()
{
  m_pDevice.Clear();
}

xiiUniquePtr<xiiWindowBase> xiiGPUTestingEnvironmentD3D12::CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle)
{
  return xiiUniquePtr<xiiWindowBase>();
}
