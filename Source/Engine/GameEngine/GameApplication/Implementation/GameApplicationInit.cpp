#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
constexpr const char* szDefaultRenderer = "DX11";
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
constexpr const char* szDefaultRenderer = "Vulkan";
#else
#  error Renderer not implemented on platform
#endif

xiiCommandLineOptionString opt_Renderer("app", "-renderer", "The renderer implementation to use.", szDefaultRenderer);

void xiiGameApplication::Init_ConfigureAssetManagement()
{
  const xiiStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.m_sName, ".xiiAidlt");

  // which redirection table to search
  xiiDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  xiiResourceManager::RegisterResourceForAssetType("Collection", xiiGetStaticRTTI<xiiCollectionResource>());
  xiiResourceManager::RegisterResourceForAssetType("Material", xiiGetStaticRTTI<xiiMaterialResource>());
  xiiResourceManager::RegisterResourceForAssetType("Mesh", xiiGetStaticRTTI<xiiMeshResource>());
  xiiResourceManager::RegisterResourceForAssetType("Animated Mesh", xiiGetStaticRTTI<xiiMeshResource>());
  xiiResourceManager::RegisterResourceForAssetType("Prefab", xiiGetStaticRTTI<xiiPrefabResource>());
  xiiResourceManager::RegisterResourceForAssetType("RenderPipeline", xiiGetStaticRTTI<xiiRenderPipelineResource>());
  xiiResourceManager::RegisterResourceForAssetType("Surface", xiiGetStaticRTTI<xiiSurfaceResource>());
  xiiResourceManager::RegisterResourceForAssetType("Texture 2D", xiiGetStaticRTTI<xiiTexture2DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Render Target", xiiGetStaticRTTI<xiiTexture2DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Texture Cube", xiiGetStaticRTTI<xiiTextureCubeResource>());
  xiiResourceManager::RegisterResourceForAssetType("Color Gradient", xiiGetStaticRTTI<xiiColorGradientResource>());
  xiiResourceManager::RegisterResourceForAssetType("Curve1D", xiiGetStaticRTTI<xiiCurve1DResource>());
  xiiResourceManager::RegisterResourceForAssetType("Skeleton", xiiGetStaticRTTI<xiiSkeletonResource>());
  xiiResourceManager::RegisterResourceForAssetType("Animation Clip", xiiGetStaticRTTI<xiiAnimationClipResource>());
  xiiResourceManager::RegisterResourceForAssetType("Animation Controller", xiiGetStaticRTTI<xiiAnimGraphResource>());
  xiiResourceManager::RegisterResourceForAssetType("Image Data", xiiGetStaticRTTI<xiiImageDataResource>());
  xiiResourceManager::RegisterResourceForAssetType("PropertyAnim", xiiGetStaticRTTI<xiiPropertyAnimResource>());
  xiiResourceManager::RegisterResourceForAssetType("Visual Script", xiiGetStaticRTTI<xiiVisualScriptResource>());
  xiiResourceManager::RegisterResourceForAssetType("Decal", xiiGetStaticRTTI<xiiDecalResource>());
  xiiResourceManager::RegisterResourceForAssetType("Decal Atlas", xiiGetStaticRTTI<xiiDecalAtlasResource>());
  xiiResourceManager::RegisterResourceForAssetType("LUT", xiiGetStaticRTTI<xiiTexture3DResource>());
}

void xiiGameApplication::Init_SetupDefaultResources()
{
  SUPER::Init_SetupDefaultResources();

  xiiResourceManager::SetIncrementalUnloadForResourceType<xiiShaderPermutationResource>(false);

  // Shaders
  {
    xiiShaderResourceDescriptor desc;
    xiiShaderResourceHandle     hFallbackShader = xiiResourceManager::CreateResource<xiiShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");
    xiiShaderResourceHandle     hMissingShader  = xiiResourceManager::CreateResource<xiiShaderResource>("MissingShaderResource", std::move(desc), "MissingShaderResource");

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

  // Render to 2D Textures
  {
    xiiRenderToTexture2DResourceDescriptor desc;
    desc.m_uiWidth  = 128;
    desc.m_uiHeight = 128;

    xiiRenderToTexture2DResourceHandle hMissingTexture = xiiResourceManager::CreateResource<xiiRenderToTexture2DResource>("R22DT_Missing", std::move(desc));

    xiiResourceManager::SetResourceTypeMissingFallback<xiiRenderToTexture2DResource>(hMissingTexture);
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

    xiiMeshResourceHandle hMissingMesh = xiiResourceManager::LoadResource<xiiMeshResource>("Meshes/MissingMesh.xiiMesh");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiMeshResource>(hMissingMesh);
  }

  // Prefabs
  {
    // xiiPrefabResourceDescriptor emptyPrefab;
    // xiiPrefabResourceHandle hMissingPrefab = xiiResourceManager::CreateResource<xiiPrefabResource>("MissingPrefabResource", emptyPrefab,
    // "MissingPrefabResource");

    // xiiPrefabResourceHandle hMissingPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>("Prefabs/MissingPrefab.xiiObjectGraph");
    // xiiResourceManager::SetResourceTypeMissingFallback<xiiPrefabResource>(hMissingPrefab);
  }

  // Collections
  {
    xiiCollectionResourceDescriptor desc;
    xiiCollectionResourceHandle     hMissingCollection = xiiResourceManager::CreateResource<xiiCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    xiiResourceManager::SetResourceTypeMissingFallback<xiiCollectionResource>(hMissingCollection);
  }

  // Render Pipelines
  {
    xiiRenderPipelineResourceHandle hMissingRenderPipeline = xiiRenderPipelineResource::CreateMissingPipeline();
    xiiResourceManager::SetResourceTypeMissingFallback<xiiRenderPipelineResource>(hMissingRenderPipeline);
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

  // Visual Script
  {
    xiiVisualScriptResourceDescriptor desc;

    xiiVisualScriptResourceHandle hResource = xiiResourceManager::CreateResource<xiiVisualScriptResource>("MissingVisualScript", std::move(desc), "Missing Visual Script Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiVisualScriptResource>(hResource);
  }

  // Property Animations
  {
    xiiPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = xiiTime::Seconds(0.1);

    xiiPropertyAnimResourceHandle hResource = xiiResourceManager::CreateResource<xiiPropertyAnimResource>("MissingPropertyAnim", std::move(desc), "Missing Property Animation Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiPropertyAnimResource>(hResource);
  }

  // Animation Skeleton
  {
    xiiSkeletonResourceDescriptor desc;

    xiiSkeletonResourceHandle hResource = xiiResourceManager::CreateResource<xiiSkeletonResource>("MissingSkeleton", std::move(desc), "Missing Skeleton Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiSkeletonResource>(hResource);
  }

  // Animation Clip
  {
    xiiAnimationClipResourceDescriptor desc;

    xiiAnimationClipResourceHandle hResource = xiiResourceManager::CreateResource<xiiAnimationClipResource>("MissingAnimationClip", std::move(desc), "Missing Animation Clip Resource");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiAnimationClipResource>(hResource);
  }

  // Decal Atlas
  {
    xiiResourceManager::AllowResourceTypeAcquireDuringUpdateContent<xiiDecalAtlasResource, xiiTexture2DResource>();
  }
}

const char* GetRendererNameFromCommandLine()
{
  return opt_Renderer.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified);
}

const char* xiiGameApplication::GetActiveRenderer()
{
  return GetRendererNameFromCommandLine();
}

void xiiGameApplication::Init_SetupGraphicsDevice()
{
  xiiGALDeviceCreationDescription DeviceInit;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  DeviceInit.m_bDebugDevice = true;
#endif

  {
    xiiGALDevice* pDevice = nullptr;

    if (s_DefaultDeviceCreator.IsValid())
    {
      pDevice = s_DefaultDeviceCreator(DeviceInit);
    }
    else
    {
      const char* szRendererName = GetRendererNameFromCommandLine();
      pDevice                    = xiiGALDeviceFactory::CreateDevice(szRendererName, xiiFoundation::GetDefaultAllocator(), DeviceInit);
      XII_ASSERT_DEV(pDevice != nullptr, "Device implemention for '{}' not found", szRendererName);
    }

    XII_VERIFY(pDevice->Init() == XII_SUCCESS, "Graphics device creation failed!");
    xiiGALDevice::SetDefaultDevice(pDevice);
  }

  // Create GPU resource pool
  xiiGPUResourcePool* pResourcePool = XII_DEFAULT_NEW(xiiGPUResourcePool);
  xiiGPUResourcePool::SetDefaultInstance(pResourcePool);
}

void xiiGameApplication::Init_LoadRequiredPlugins()
{
  xiiPlugin::InitializeStaticallyLinkedPlugins();

  constexpr const char* szDefaultLibraryName = "xiiRendererDiligent";
  xiiGALDeviceFactory::RegisterLibraryName("DX11", "xiiRendererDX11");
  xiiGALDeviceFactory::RegisterLibraryName("D3D11", szDefaultLibraryName);
  xiiGALDeviceFactory::RegisterLibraryName("D3D12", szDefaultLibraryName);
  xiiGALDeviceFactory::RegisterLibraryName("Vulkan", szDefaultLibraryName);

  const char* szRendererName   = GetRendererNameFromCommandLine();
  const char* szShaderModel    = "";
  const char* szShaderCompiler = "";
  xiiGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);
  xiiShaderManager::Configure(szShaderModel, true);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiPlugin::LoadPlugin("xiiInspectorPlugin").IgnoreResult();

  // On sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#  if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  xiiPlugin::LoadPlugin("xiiFileservePlugin").IgnoreResult(); // don't care if it fails to load
#  endif

#endif

  XII_VERIFY(xiiPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);
}

void xiiGameApplication::Deinit_ShutdownGraphicsDevice()
{
  if (!xiiGALDevice::HasDefaultDevice())
    return;

  // Cleanup resource pool
  xiiGPUResourcePool::SetDefaultInstance(nullptr);

  xiiResourceManager::FreeAllUnusedResources();

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  pDevice->Shutdown().IgnoreResult();
  XII_DEFAULT_DELETE(pDevice);
  xiiGALDevice::SetDefaultDevice(nullptr);

  xiiGALDeviceFactory::UnregisterLibraryName("DX11");
  xiiGALDeviceFactory::UnregisterLibraryName("D3D11");
  xiiGALDeviceFactory::UnregisterLibraryName("D3D12");
  xiiGALDeviceFactory::UnregisterLibraryName("Vulkan");
}


XII_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationInit);
