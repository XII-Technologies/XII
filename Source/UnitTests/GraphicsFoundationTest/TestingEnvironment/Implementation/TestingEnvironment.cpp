/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Configuration/Plugin.h>

xiiGPUTestingEnvironment::xiiGPUTestingEnvironment(xiiStringView sImplementationName) :
  m_sImplementationName(sImplementationName)
{
}

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
    xiiPlugin::UnloadAllPlugins();
  }
}

xiiUniquePtr<xiiWindowBase> xiiGPUTestingEnvironment::CreateWindow(xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiStringView sTitle)
{
  XII_IGNORE_UNUSED(uiWidth);
  XII_IGNORE_UNUSED(uiHeight);
  XII_IGNORE_UNUSED(sTitle);
  return {};
}
