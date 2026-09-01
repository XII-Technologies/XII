/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Components/StateMachine/StateMachineResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Resources/BlackboardTemplateResource.h>
#include <GameEngine/Resources/ImageDataResource.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Particles/ParticleGraph.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
constexpr const char* szDefaultGraphicsAPI = "D3D12";
#else
constexpr const char* szDefaultGraphicsAPI = "";
#endif

xiiCommandLineOptionString opt_Renderer("app", "-renderer", "The renderer implementation to use.", szDefaultGraphicsAPI);

void xiiGameApplication::Init_ConfigureAssetManagement()
{
  const xiiStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.GetConfigName(), ".xiiAidlt");

  // which redirection table to search
  xiiDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  xiiResourceManager::RegisterResourceForAssetType("Animated Mesh", xiiGetStaticRTTI<xiiMeshResource>());
  xiiResourceManager::RegisterResourceForAssetType("Animation Clip", xiiGetStaticRTTI<xiiAnimationClipResource>());
  xiiResourceManager::RegisterResourceForAssetType("Animation Graph", xiiGetStaticRTTI<xiiAnimGraphResource>());
  xiiResourceManager::RegisterResourceForAssetType("BlackboardTemplate", xiiGetStaticRTTI<xiiBlackboardTemplateResource>());
  xiiResourceManager::RegisterResourceForAssetType("Collection", xiiGetStaticRTTI<xiiCollectionResource>());
  xiiResourceManager::RegisterResourceForAssetType("ColorGradient", xiiGetStaticRTTI<xiiColorGradientResource>());
  xiiResourceManager::RegisterResourceForAssetType("Curve1D", xiiGetStaticRTTI<xiiCurve1DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Decal", xiiGetStaticRTTI<xiiDecalResource>());
  xiiResourceManager::RegisterResourceForAssetType("Decal Atlas", xiiGetStaticRTTI<xiiDecalAtlasResource>());
  xiiResourceManager::RegisterResourceForAssetType("Image Data", xiiGetStaticRTTI<xiiImageDataResource>());
  xiiResourceManager::RegisterResourceForAssetType("LUT", xiiGetStaticRTTI<xiiTexture3DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Material", xiiGetStaticRTTI<xiiMaterialResource>());
  xiiResourceManager::RegisterResourceForAssetType("Mesh", xiiGetStaticRTTI<xiiMeshResource>());
  xiiResourceManager::RegisterResourceForAssetType("Prefab", xiiGetStaticRTTI<xiiPrefabResource>());
  xiiResourceManager::RegisterResourceForAssetType("Render Target", xiiGetStaticRTTI<xiiTexture2DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Shader", xiiGetStaticRTTI<xiiShaderResource>());
  xiiResourceManager::RegisterResourceForAssetType("Skeleton", xiiGetStaticRTTI<xiiSkeletonResource>());
  xiiResourceManager::RegisterResourceForAssetType("StateMachine", xiiGetStaticRTTI<xiiStateMachineResource>());
  xiiResourceManager::RegisterResourceForAssetType("Substance Texture", xiiGetStaticRTTI<xiiTexture2DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Surface", xiiGetStaticRTTI<xiiSurfaceResource>());
  xiiResourceManager::RegisterResourceForAssetType("Texture 2D", xiiGetStaticRTTI<xiiTexture2DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Texture 3D", xiiGetStaticRTTI<xiiTexture3DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Texture Cube", xiiGetStaticRTTI<xiiTextureCubeResource>());
  xiiResourceManager::RegisterResourceForAssetType("Particle Graph", xiiGetStaticRTTI<xiiParticleGraphResource>());
}

void xiiGameApplication::Init_SetupDefaultResources()
{
  SUPER::Init_SetupDefaultResources();

  xiiResourceManager::SetIncrementalUnloadForResourceType<xiiShaderPermutationResource>(false);

  // Shaders
  {
    xiiShaderResourceDescriptor desc;
    xiiShaderResourceHandle     hFallbackShader = xiiResourceManager::CreateResource<xiiShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");

    xiiShaderResourceDescriptor desc2;
    xiiShaderResourceHandle     hMissingShader = xiiResourceManager::CreateResource<xiiShaderResource>("MissingShaderResource", std::move(desc2), "MissingShaderResource");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiShaderResource>(hFallbackShader);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiShaderResource>(hMissingShader);
  }

  // Shader Permutation
  {
    xiiShaderPermutationResourceDescriptor desc;
    xiiShaderPermutationResourceHandle     hFallbackShaderPermutation = xiiResourceManager::CreateResource<xiiShaderPermutationResource>("FallbackShaderPermutationResource", std::move(desc), "FallbackShaderPermutationResource");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiShaderPermutationResource>(hFallbackShaderPermutation);
  }

  // 2D Textures
  {
    xiiTexture2DResourceHandle hFallbackTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/LoadingTexture_D.dds");
    xiiTexture2DResourceHandle hMissingTexture  = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/MissingTexture_D.dds");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiTexture2DResource>(hFallbackTexture);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiTexture2DResource>(hMissingTexture);
  }

  // Cube Textures
  {
    /// \todo Loading Cubemap Texture

    xiiTextureCubeResourceHandle hFallbackTexture = xiiResourceManager::LoadResource<xiiTextureCubeResource>("Textures/MissingCubeMap.dds");
    xiiTextureCubeResourceHandle hMissingTexture  = xiiResourceManager::LoadResource<xiiTextureCubeResource>("Textures/MissingCubeMap.dds");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiTextureCubeResource>(hFallbackTexture);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiTextureCubeResource>(hMissingTexture);
  }

  // Materials
  {
    xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<xiiMaterialResource, xiiMaterialResource>();

    xiiMaterialResourceHandle hMissingMaterial  = xiiResourceManager::LoadResource<xiiMaterialResource>("Materials/Common/MissingMaterial.xiiMaterial");
    xiiMaterialResourceHandle hFallbackMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>("Materials/Common/LoadingMaterial.xiiMaterial");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiMaterialResource>(hFallbackMaterial);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiMaterialResource>(hMissingMaterial);
  }

  // Meshes
  {
    xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<xiiMeshResource, xiiMeshBufferResource>();

    // Create mesh buffer resource
    xiiGeometry geom;
    geom.AddBox(xiiVec3(1), true);
    geom.TriangulatePolygons(4);
    geom.ComputeTangents();
    geom.ComputeFaceNormals();
    geom.ComputeSmoothVertexNormals();

    xiiMeshBufferResourceDescriptor meshBufferDesc;
    meshBufferDesc.AddCommonStreams();
    meshBufferDesc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);
    meshBufferDesc.ComputeBounds();

    xiiMeshBufferResourceHandle hMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("MissingMesh_Box", std::move(meshBufferDesc));

    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);

    // Create mesh resource
    xiiMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
    desc.SetMaterial(0, "");
    desc.ComputeBounds();

    xiiMeshResourceHandle hMissingMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>("Meshes/MissingMesh.xiiBinMesh", std::move(desc), pMeshBuffer->GetResourceDescription());
    xiiResourceManager::SetResourceTypeMissingFallback<xiiMeshResource>(hMissingMesh);
  }

  // Prefabs
  {
    xiiPrefabResourceDescriptor emptyPrefab;
    xiiPrefabResourceHandle     hMissingPrefab = xiiResourceManager::CreateResource<xiiPrefabResource>("MissingPrefabResource", std::move(emptyPrefab), "MissingPrefabResource");

    xiiResourceManager::SetResourceTypeMissingFallback<xiiPrefabResource>(hMissingPrefab);
  }

  // Collections
  {
    xiiCollectionResourceDescriptor desc;
    xiiCollectionResourceHandle     hMissingCollection = xiiResourceManager::CreateResource<xiiCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    xiiResourceManager::SetResourceTypeMissingFallback<xiiCollectionResource>(hMissingCollection);
  }

  // Color Gradient
  {
    xiiColorGradientResourceDescriptor cg;
    cg.m_Gradient.AddColorControlPoint(0, xiiColor::RebeccaPurple);
    cg.m_Gradient.AddColorControlPoint(1, xiiColor::LawnGreen);
    cg.m_Gradient.SortControlPoints();

    xiiColorGradientResourceHandle hResource = xiiResourceManager::CreateResource<xiiColorGradientResource>("MissingColorGradient", std::move(cg), "Missing Color Gradient Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiColorGradientResource>(hResource);
  }

  // 1D Curve
  {
    xiiCurve1DResourceDescriptor cd;
    auto&                        curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);
    curve.CreateLinearApproximation();

    xiiCurve1DResourceHandle hResource = xiiResourceManager::CreateResource<xiiCurve1DResource>("MissingCurve1D", std::move(cd), "Missing Curve1D Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiCurve1DResource>(hResource);
  }
}

xiiStringView GetGraphicsAPINameFromCommandLine()
{
  return opt_Renderer.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified);
}

xiiStringView xiiGameApplication::GetActiveRenderer()
{
  return GetGraphicsAPINameFromCommandLine();
}

void xiiGameApplication::Init_SetupGraphicsDevice()
{
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
    xiiSharedPtr<xiiGALDevice> pDevice;

    if (s_DefaultDeviceCreator.IsValid())
    {
      pDevice = s_DefaultDeviceCreator(deviceCreationDescription);
    }
    else
    {
      xiiStringView sGraphicsAPIName = GetGraphicsAPINameFromCommandLine();

      pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
      XII_ASSERT_DEV(pDevice != nullptr, "Device implementation for '{}' not found.", sGraphicsAPIName);
    }

    XII_VERIFY(pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

    pDevice->SetDebugName("Master Graphics Device");

    xiiGALDevice::SetDefaultDevice(pDevice);
  }
}

void xiiGameApplication::Init_LoadRequiredPlugins()
{
  xiiPlugin::InitializeStaticallyLinkedPlugins();

  xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
  xiiStringView sShaderModel     = {};
  xiiStringView sShaderCompiler  = {};
  xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

  xiiGALShaderManager::Configure(sShaderModel, true);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();

  // On sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin.
#  if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  xiiPlugin::LoadPlugin("xiiFileservePlugin").IgnoreResult(); // don't care if it fails to load
#  endif

#endif

  XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", sShaderCompiler);
}

void xiiGameApplication::Deinit_ShutdownGraphicsDevice()
{
  if (!xiiGALDevice::HasDefaultDevice())
    return;

  xiiResourceManager::FreeAllUnusedResources();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  xiiGALDevice::SetDefaultDevice(nullptr);
  pDevice.Clear();
}

XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationInit);
