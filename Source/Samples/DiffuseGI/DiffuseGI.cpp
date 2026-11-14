/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#ifdef XII_DIFFUSE_GI_EDITOR
#  include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#  include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#endif
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/SkyLightComponent.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Denoising/LocalLightDenoisePass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGISceneUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/RayTracedIrradiancePass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <Texture/Image/Image.h>

namespace
{
  xiiCommandLineOptionPath   opt_Output("DiffuseGI", "-output", "Directory for the rendered PNGs.", "");
  xiiCommandLineOptionInt    opt_Frames("DiffuseGI", "-frames", "Frames of temporal accumulation per image.", 20, 1, 240);
  xiiCommandLineOptionString opt_Pipeline("DiffuseGI", "-pipeline", "Authored root to render: Scene or Editor.", "Scene");
  xiiCommandLineOptionBool   opt_LocalLights("DiffuseGI", "-local-lights", "Compare exact and reservoir evaluation of a dense local-light scene.", false);
  xiiCommandLineOptionBool   opt_SkyOnly("DiffuseGI", "-sky-only", "Light the room with the sky and disable the emissive panel.", false);
  xiiCommandLineOptionBool   opt_OffscreenEmitter("DiffuseGI", "-offscreen-emitter", "Compare camera and world probes with an emitter outside the frame.", false);
  xiiCommandLineOptionBool   opt_BlockEmitter("DiffuseGI", "-block-emitter", "Add an opaque cover beneath the offscreen emitter.", false);
  xiiCommandLineOptionBool   opt_DenseEmitter("DiffuseGI", "-dense-emitter", "Use a tessellated offscreen panel with 4,608 triangles.", false);
  xiiCommandLineOptionBool   opt_OffscreenLight("DiffuseGI", "-offscreen-light", "Illuminate a non-emissive offscreen panel with short-range point lights.", false);
  xiiCommandLineOptionBool   opt_NoLightShadows("DiffuseGI", "-no-light-shadows", "Disable shadows for the offscreen point lights.", false);
  xiiCommandLineOptionBool   opt_NoReuse("DiffuseGI", "-no-reuse", "Use fresh ray paths instead of reservoir resampling.", false);
  xiiCommandLineOptionInt    opt_Paths("DiffuseGI", "-paths", "Override fresh ray paths per pixel (zero uses the authored count).", 0, 0, 32);
  xiiCommandLineOptionBool   opt_NoRay("DiffuseGI", "-no-ray", "Disable hardware rays to render the scene probe fallback.", false);
  xiiCommandLineOptionBool   opt_ScreenOnly("DiffuseGI", "-screen-only", "Use only screen-space diffuse transport for comparison.", false);
  xiiCommandLineOptionBool   opt_Diagnostics("DiffuseGI", "-diagnostics", "Also capture the Editor GI inspection views.", false);

  xiiRenderPipelineResourceHandle LoadPipeline(xiiStringView sName)
  {
    return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiStringBuilder("RenderPipelines/", sName, ".xiiBinRenderPipeline"));
  }

  xiiRenderPipelineResourceHandle EnableGI(bool bEditor, bool bScreenOnly, bool bWorldGeometry = true)
  {
    const auto                                 handle = LoadPipeline(bEditor ? "Editor" : "Scene");
    xiiResourceLock<xiiRenderPipelineResource> root(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> scene(LoadPipeline("Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!root || !root->GetAuthoring() || !scene || !scene->GetAuthoring())
    {
      return {};
    }
    auto                                       descriptor = root->GetAuthoring()->m_Descriptor;
    const auto                                 sceneId    = scene->GetAuthoring()->m_Descriptor.m_Id;
    const auto                                 gi         = xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance");
    xiiResourceLock<xiiRenderPipelineResource> library(LoadPipeline("SceneScreenSpaceIrradiance"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!library || !library->GetAuthoring())
    {
      return {};
    }
    const auto& workload = library->GetAuthoring()->m_Descriptor;
    xiiUuid     ray;
    xiiUuid     trace;
    for (const auto& edge : workload.m_Connections)
    {
      if (edge.m_sInput == "Depth" && workload.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
      {
        ray   = workload.m_Passes[edge.m_uiSource].m_Id;
        trace = workload.m_Passes[edge.m_uiTarget].m_Id;
      }
    }
    if (!ray.IsValid())
    {
      return {};
    }
    const auto gate = bScreenOnly ? xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceIrradiance/Bounce Depth") : ray;
    for (auto& node : descriptor.m_Passes)
    {
      if (bEditor ? node.m_Definition == sceneId : node.m_Id == gi)
      {
        auto& setting = node.m_Overrides.ExpandAndGetRef();
        if (bEditor)
        {
          setting.m_Path.PushBack(gi);
        }
        setting.m_Path.PushBack(gate);
        setting.m_Properties.Insert("Enabled", true);
        if (!bWorldGeometry)
        {
          for (const auto& pass : workload.m_Passes)
          {
            if (pass.m_pType == xiiGetStaticRTTI<xiiDDGISceneUpdatePass>())
            {
              auto& probes = node.m_Overrides.ExpandAndGetRef();
              if (bEditor)
              {
                probes.m_Path.PushBack(gi);
              }
              probes.m_Path.PushBack(pass.m_Id);
              probes.m_Properties.Insert("UseWorldGeometry", false);
            }
          }
        }
        const auto paths     = opt_Paths.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        const bool freshOnly = opt_NoReuse.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        if (freshOnly || paths != 0)
        {
          auto& transport = node.m_Overrides.ExpandAndGetRef();
          if (bEditor)
          {
            transport.m_Path.PushBack(gi);
          }
          transport.m_Path.PushBack(trace);
          if (freshOnly)
          {
            transport.m_Properties.Insert("UsePathReservoirs", false);
          }
          if (paths != 0)
          {
            transport.m_Properties.Insert("SampleCount", xiiUInt32(paths));
          }
        }
        return xiiResourceManager::CreateResource<xiiRenderPipelineResource>(xiiStringBuilder(bEditor ? "DiffuseGI.Editor" : "DiffuseGI.Scene", bWorldGeometry ? "" : ".CameraProbes"), std::move(descriptor));
      }
    }
    return {};
  }

  xiiRenderPipelineResourceHandle EnableLocalLightReservoirs(bool editor, bool reconstruct)
  {
    xiiResourceLock<xiiRenderPipelineResource> scene(LoadPipeline("Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> root(LoadPipeline(editor ? "Editor" : "Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!scene || !scene->GetAuthoring() || !root || !root->GetAuthoring())
    {
      return {};
    }
    const auto& source = scene->GetAuthoring()->m_Descriptor;
    xiiUuid     lightPass;
    xiiUuid     reconstructionPass;
    for (const auto& pass : source.m_Passes)
    {
      if (pass.m_pType == xiiGetStaticRTTI<xiiClusteredLocalSurfaceLightingPass>())
      {
        lightPass = pass.m_Id;
      }
      else if (pass.m_pType == xiiGetStaticRTTI<xiiLocalLightDenoisePass>())
      {
        reconstructionPass = pass.m_Id;
      }
    }
    if (!lightPass.IsValid() || !reconstructionPass.IsValid())
    {
      return {};
    }
    auto       descriptor  = root->GetAuthoring()->m_Descriptor;
    const auto setProperty = [&](xiiUuid id, xiiStringView property, bool value) {
      for (auto& pass : descriptor.m_Passes)
      {
        if (editor && pass.m_Definition == source.m_Id)
        {
          auto& setting = pass.m_Overrides.ExpandAndGetRef();
          setting.m_Path.PushBack(id);
          setting.m_Properties.Insert(property, value);
          return true;
        }
        if (!editor && pass.m_Id == id)
        {
          pass.m_Properties.Insert(property, value);
          return true;
        }
      }
      return false;
    };
    if (!setProperty(lightPass, "UseReservoirs", true) || !setProperty(reconstructionPass, "Enabled", reconstruct))
    {
      return {};
    }
    const xiiStringBuilder name("DiffuseGI.", editor ? "Editor" : "Scene", reconstruct ? ".FilteredLocalReservoirs" : ".RawLocalReservoirs");
    return xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(descriptor));
  }

  void AddLocalLights(xiiWorld& world)
  {
    const xiiColorGammaUB colors[] = {xiiColorGammaUB(255, 217, 168), xiiColorGammaUB(182, 219, 255), xiiColorGammaUB(255, 244, 221)};
    for (xiiUInt32 row = 0; row < 5; ++row)
    {
      for (xiiUInt32 column = 0; column < 6; ++column)
      {
        xiiGameObjectDescription description;
        description.m_LocalPosition = xiiVec3(-1.3F + float(row) * 0.78F, -1.65F + float(column) * 0.66F, 2.88F);
        xiiGameObject* owner        = nullptr;
        world.CreateObject(description, owner);
        xiiPointLightComponent* light = nullptr;
        xiiPointLightComponent::CreateComponent(owner, light);
        light->SetRange(5.5F);
        light->SetIntensity(2.5F + float((row + 3U * column) % 4U) * 0.55F);
        light->SetLightColor(colors[(row + column) % XII_ARRAY_SIZE(colors)]);
      }
    }
  }

  xiiMaterialResourceHandle Material(xiiStringView sName, xiiColor color, xiiColor emission)
  {
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor     = color;
    material.m_EmissiveColor = emission;
    material.m_fRoughness    = 0.9F;
    material.m_FeatureFlags  = xiiMaterialFeatureFlags::ReceivesLighting | xiiMaterialFeatureFlags::CastsShadows;
    return xiiResourceManager::CreateResource<xiiMaterialResource>(sName, std::move(material));
  }

  void AddMesh(xiiWorld& world, xiiStringView sName, xiiVec3 vPosition, xiiGeometry& geometry, const xiiMaterialResourceHandle& material)
  {
    geometry.TriangulatePolygons();
    geometry.ComputeTangents();
    xiiMeshResourceDescriptor mesh;
    mesh.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    mesh.AddSection(geometry.GetPolygons().GetCount(), 0, 0);
    mesh.ComputeBounds();
    xiiGameObjectDescription objectDescription;
    objectDescription.m_LocalPosition = vPosition;
    xiiGameObject* object             = nullptr;
    world.CreateObject(objectDescription, object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(xiiResourceManager::CreateResource<xiiMeshResource>(sName, std::move(mesh)));
    component->SetMaterialOverride(0, material);
  }

  void MakeRoom(xiiWorld& world, bool bSkyOnly, bool bLocalLights, bool bOffscreenEmitter, bool bOffscreenLight)
  {
    const auto white    = Material("GI.White", xiiColor(0.75F, 0.75F, 0.75F), xiiColor::Black);
    const auto red      = Material("GI.Red", xiiColor(0.7F, 0.06F, 0.04F), xiiColor::Black);
    const auto green    = Material("GI.Green", xiiColor(0.06F, 0.62F, 0.14F), xiiColor::Black);
    const auto emission = bOffscreenLight ? xiiColor::Black : (bOffscreenEmitter ? xiiColor(96, 60, 24) : (bSkyOnly || bLocalLights ? xiiColor::Black : xiiColor(12, 9, 6)));
    const auto emitter  = Material("GI.Emitter", xiiColor::White, emission);
    const auto box      = [&](xiiStringView sName, xiiVec3 vPosition, xiiVec3 vExtent, const xiiMaterialResourceHandle& material) {
      xiiGeometry geometry;
      geometry.AddBox(vExtent, true);
      AddMesh(world, sName, vPosition, geometry, material);
    };
    box("GI.Floor", {0.4F, 0, -0.1F}, {4.2F, 4.3F, 0.2F}, white);
    box("GI.Ceiling", {0.4F, 0, 3.4F}, {4.2F, 4.3F, 0.2F}, white);
    box("GI.Back", {2.5F, 0, 1.65F}, {0.2F, 4.3F, 3.3F}, white);
    box("GI.Left", {0.4F, 2.15F, 1.65F}, {4.2F, 0.2F, 3.3F}, red);
    box("GI.Right", {0.4F, -2.15F, 1.65F}, {4.2F, 0.2F, 3.3F}, green);
    if (bOffscreenEmitter)
    {
      if (opt_DenseEmitter.GetOptionValue(xiiCommandLineOption::LogMode::Never))
      {
        xiiGeometry             panel;
        xiiGeometry::GeoOptions options;
        options.m_MainAxis = xiiBasisAxis::NegativeZ;
        panel.AddRect({0.7F, 3.8F}, 48, 48, options);
        AddMesh(world, "GI.DensePanel", {-2.65F, 0, 3.2F}, panel, emitter);
      }
      else
      {
        box("GI.Panel", {-2.65F, 0, 3.26F}, {0.7F, 3.8F, 0.12F}, emitter);
      }
      if (bOffscreenLight)
      {
        const bool  blocked     = opt_BlockEmitter.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        const bool  castShadows = !opt_NoLightShadows.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        const auto  cover       = blocked ? Material("GI.LightCover", xiiColor::Black, xiiColor::Black) : xiiMaterialResourceHandle();
        xiiGeometry covers;
        for (float y : {-1.35F, -0.45F, 0.45F, 1.35F})
        {
          xiiGameObjectDescription description;
          description.m_LocalPosition = {-2.65F, y, 3.04F};
          xiiGameObject* owner        = nullptr;
          world.CreateObject(description, owner);
          xiiPointLightComponent* light = nullptr;
          xiiPointLightComponent::CreateComponent(owner, light);
          light->SetRange(0.90F);
          light->SetIntensity(480.0F);
          light->SetLightColor(xiiColorGammaUB(255, 183, 100));
          light->SetCastShadows(castShadows);
          if (blocked)
          {
            xiiGeometry::GeoOptions options;
            options.m_Transform.SetTranslationVector({-2.65F, y, 3.075F});
            covers.AddBox({0.24F, 0.28F, 0.02F}, true, options);
          }
        }
        if (blocked)
        {
          AddMesh(world, "GI.LightCovers", xiiVec3::MakeZero(), covers, cover);
        }
      }
      else if (opt_BlockEmitter.GetOptionValue(xiiCommandLineOption::LogMode::Never))
      {
        box("GI.EmitterCover", {-2.65F, 0, 3.15F}, {0.94F, 4.1F, 0.1F}, white);
      }
    }
    else
    {
      box("GI.Panel", {0.55F, 0, 3.21F}, {1.2F, 1.25F, 0.12F}, emitter);
    }
    box("GI.Pedestal", {1.1F, 0.95F, 0.64F}, {0.95F, 0.95F, 1.28F}, white);
    xiiGeometry sphere;
    sphere.AddStackedSphere(0.65F, 32, 16);
    AddMesh(world, "GI.Sphere", {0.05F, -0.8F, 0.66F}, sphere, white);

    if (bLocalLights)
    {
      AddLocalLights(world);
      return;
    }
    if (bOffscreenEmitter)
    {
      return;
    }

    // A blue sky outside the open side provides a second transport source.
    xiiTextureCubeResourceDescriptor sky;
    sky.m_TextureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    sky.m_TextureDescription.m_Size               = {1, 1};
    sky.m_TextureDescription.m_uiArraySizeOrDepth = 6;
    sky.m_TextureDescription.m_Format             = xiiGALResourceFormat::RGBA32Float;
    sky.m_TextureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    const xiiVec4                skyPixel         = bSkyOnly ? xiiVec4(0.3F, 0.65F, 1.25F, 1) : xiiVec4(0.02F, 0.04F, 0.08F, 1);
    xiiGALTextureSubResourceData faces[6];
    for (auto& face : faces)
    {
      face = xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&skyPixel), sizeof(skyPixel)), sizeof(skyPixel), sizeof(skyPixel));
    }
    sky.m_InitialContent     = faces;
    xiiGameObject* skyObject = nullptr;
    world.CreateObject({}, skyObject);
    xiiSkyLightComponent* light = nullptr;
    xiiSkyLightComponent::CreateComponent(skyObject, light);
    light->SetEnvironmentMap(xiiResourceManager::CreateResource<xiiTextureCubeResource>("GI.Sky", std::move(sky)));
  }

  xiiResult SaveImage(xiiGALDevice& device, xiiGALTexture* texture, xiiStringView sPath)
  {
    xiiGALTextureReadback readback(&device);
    auto                  commands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!commands)
    {
      return XII_FAILURE;
    }
    commands->Begin();
    const auto queued = readback.Enqueue(commands.Borrow(), {.m_pTexture = texture});
    commands->End();
    XII_SUCCEED_OR_RETURN(queued);
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    const auto capture = readback.GetCompleted();
    if (!capture || capture.m_uiBytesPerPixel != 4)
    {
      return XII_FAILURE;
    }
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    xiiImage image;
    image.ResetAndAlloc(texture->GetDescription());
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    const auto                     mappedResult = commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
    if (mappedResult.Succeeded())
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        xiiMemoryUtils::Copy(image.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y), xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth * 4);
      }
      commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}).AssertSuccess();
    }
    commands->End();
    XII_SUCCEED_OR_RETURN(mappedResult);
    return image.SaveTo(sPath);
  }
} // namespace

class xiiDiffuseGICaptureApp final : public xiiApplication
{
public:
  xiiDiffuseGICaptureApp() : xiiApplication("Diffuse GI") {}

  Execution Run() override
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    if (Capture().Failed())
    {
      SetReturnCode(1);
    }
    return Execution::Quit;
  }

private:
  xiiResult Capture()
  {
    const xiiString output   = opt_Output.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    const xiiString pipeline = opt_Pipeline.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    if (output.IsEmpty() || (pipeline != "Scene" && pipeline != "Editor"))
    {
      xiiLog::Error("Provide -output <directory> and select -pipeline Scene or Editor.");
      return XII_FAILURE;
    }
    XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(output));
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(output, "Captures", ":capture", xiiDataDirUsage::AllowWrites));
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Output", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites));
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base"));
    xiiDataDirectory::FolderType::s_sRedirectionFile = "RenderPipelines/PipelineLibrary.xiiAidlt";
    XII_SUCCEED_OR_RETURN(xiiFileSystem::AddDataDirectory(">sdk/Data/Base/AssetCache/Common", "PipelineCache"));
#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* defaultRenderer = "Vulkan";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
    constexpr const char* defaultRenderer = "D3D12";
#else
    constexpr const char* defaultRenderer = "";
#endif
    const auto    renderer = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, defaultRenderer);
    xiiStringView model;
    xiiStringView compiler;
    xiiGALDeviceFactory::GetShaderModelAndCompiler(renderer, model, compiler);
    if (model.IsEmpty() || compiler.IsEmpty())
    {
      xiiLog::Error("No shader compiler is available for renderer '{}'.", renderer);
      return XII_FAILURE;
    }
    xiiGALShaderManager::Configure(model, true);
    XII_SUCCEED_OR_RETURN(xiiPlugin::LoadPlugin(compiler));
    xiiGALDeviceCreationDescription description;
    description.m_DeviceFeatures.m_NativeFence                = xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_TextureUAVExtendedFormats  = xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_IndependentBlend           = xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_BindlessResources          = xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_ShaderResourceRuntimeArray = xiiGALDeviceFeatureState::Optional;
    const bool offscreenLight                                 = opt_OffscreenLight.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    const bool offscreenEmitter                               = offscreenLight || opt_OffscreenEmitter.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    description.m_DeviceFeatures.m_RayTracing                 = offscreenEmitter || opt_NoRay.GetOptionValue(xiiCommandLineOption::LogMode::Never) ? xiiGALDeviceFeatureState::Disabled : xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_MeshShaders                = xiiGALDeviceFeatureState::Disabled;
    auto device                                               = xiiGALDeviceFactory::CreateDevice(renderer, xiiFoundation::GetDefaultAllocator(), description);
    if (!device || device->Initialize().Failed())
    {
      return XII_FAILURE;
    }
    xiiGALDevice::SetDefaultDevice(device);
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr));
    xiiLog::Info("Diffuse transport: {}", xiiRayTracingResources::IsHardwareRayTracingSupported() ? "hardware ray tracing available" : "hardware ray tracing unavailable");

    const bool editor = pipeline == "Editor";
#ifdef XII_DIFFUSE_GI_EDITOR
    if (editor)
    {
      xiiLog::Debug("Editor passes: {}, {}", xiiGetStaticRTTI<xiiInfiniteGridPass>()->GetTypeName(), xiiGetStaticRTTI<xiiSceneMeshPickingPass>()->GetTypeName());
    }
#else
    if (editor)
    {
      xiiLog::Error("The Editor pipeline requires a build with Editor support.");
      return XII_FAILURE;
    }
#endif
    const bool localLights = !offscreenEmitter && opt_LocalLights.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    const auto base        = LoadPipeline(pipeline);
    const auto enabled     = localLights ? EnableLocalLightReservoirs(editor, true) : EnableGI(editor, !offscreenEmitter && opt_ScreenOnly.GetOptionValue(xiiCommandLineOption::LogMode::Never));
    if (!enabled.IsValid())
    {
      return XII_FAILURE;
    }
    xiiWorldDescription worldDescription("Diffuse GI demonstration");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    MakeRoom(world, opt_SkyOnly.GetOptionValue(xiiCommandLineOption::LogMode::Never), localLights, offscreenEmitter, offscreenLight);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 48, 0.1F, 40);
    camera.LookAt({-5.5F, 0, 1.65F}, {1.6F, 0, 1.6F}, {0, 0, 1});
    xiiView* view = nullptr;
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Diffuse GI capture", view);
    view->SetCamera(&camera);
    xiiDisplayOutputSettings display = view->GetDisplayOutputSettings();
    display.m_Exposure.m_Mode        = xiiExposureMode::Manual;
    XII_SUCCEED_OR_RETURN(view->SetDisplayOutputSettings(display));
    constexpr xiiUInt32 width  = 640;
    constexpr xiiUInt32 height = 400;
    view->SetViewport({0, 0, float(width), float(height)});
    const auto render = [&](const xiiRenderPipelineResourceHandle& resource, xiiViewRenderMode::Enum mode, xiiStringView sFilename) -> xiiResult {
      xiiGALTextureCreationDescription targetDescription;
      targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
      targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      targetDescription.m_Size      = {width, height};
      targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
      auto target                   = device->CreateTexture(targetDescription);
      if (!target)
      {
        return XII_FAILURE;
      }
      view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
      view->SetRenderPipelineResource(resource);
      view->SetViewRenderMode(mode);
      view->InvalidateTemporalHistory();
      const auto frames = opt_Frames.GetOptionValue(xiiCommandLineOption::LogMode::Never);
      for (xiiInt32 frame = 0; frame < frames; ++frame)
      {
        world.Update();
        xiiTaskSystem::FinishFrameTasks();
        xiiFrameAllocator::Swap();
        if (view->GetLastRenderResult().Failed())
        {
          xiiLog::Error("Could not render '{}': {}", sFilename, view->GetRenderPipelineInstance().GetLastBuildError());
          return XII_FAILURE;
        }
      }
      device->WaitIdle();
      for (const auto& pass : view->GetRenderGraph()->GetCompiledPasses())
      {
        if (pass.m_bIsCulled)
        {
          continue;
        }
        if (pass.m_sName == "Update Directional Scene Probes")
        {
          xiiLog::Info("{} used the scene probe update.", sFilename);
        }
        else if (pass.m_sName == "Trace World Geometry for Directional Probes")
        {
          xiiLog::Info("{} used the world-geometry probe update.", sFilename);
        }
        else if (pass.m_sName == "Trace World Meshlets for Directional Probes")
        {
          xiiLog::Info("{} used the meshlet-bounded world probe update.", sFilename);
        }
        else if (pass.m_sName == "Shade Offscreen Hits for Directional Probes")
        {
          xiiLog::Info("{} used analytic lighting for offscreen probe hits.", sFilename);
        }
        else if (pass.m_sName == "Resolve Current Spatial and Temporal GI")
        {
          xiiLog::Info("{} read current spatial and previous temporal diffuse path reservoirs.", sFilename);
        }
        else if (pass.m_sName == "Resolve Spatial and Temporal Local Light Reservoirs")
        {
          xiiLog::Info("{} resolved spatial and temporal local-light reservoirs.", sFilename);
        }
        else if (pass.m_sName == "Resolve Spatial Local Light Reservoirs")
        {
          xiiLog::Info("{} resolved current local-light reservoirs without temporal history.", sFilename);
        }
        else if (pass.m_sName == "Resolve Current Spatial GI")
        {
          xiiLog::Info("{} read current spatial diffuse path reservoirs without temporal history.", sFilename);
        }
      }
      xiiLog::Info("Saving {}.", sFilename);
      return SaveImage(*device, target.Borrow(), xiiStringBuilder(":capture/", sFilename));
    };
    if (localLights)
    {
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Exact.png")));
      const auto raw = EnableLocalLightReservoirs(editor, false);
      if (!raw.IsValid())
      {
        return XII_FAILURE;
      }
      XII_SUCCEED_OR_RETURN(render(raw, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Reservoirs.png")));
      return render(enabled, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Reconstructed.png"));
    }
    if (offscreenEmitter)
    {
      const auto cameraProbes = EnableGI(editor, false, false);
      if (!cameraProbes.IsValid())
      {
        return XII_FAILURE;
      }
      XII_SUCCEED_OR_RETURN(render(cameraProbes, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Camera-Probes.png")));
      return render(enabled, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-World-Probes.png"));
    }
    XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-GI-Off.png")));
    XII_SUCCEED_OR_RETURN(render(enabled, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-GI-On.png")));
    if (editor && opt_Diagnostics.GetOptionValue(xiiCommandLineOption::LogMode::Never))
    {
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::DiffuseGIRaw, "Editor-GI-Raw.png"));
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::DiffuseGI, "Editor-GI-Reconstructed.png"));
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::DiffuseGIHistory, "Editor-GI-History.png"));
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::DiffuseGIReconstructionDelta, "Editor-GI-Reconstruction-Delta.png"));
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::DiffuseGIFrameDelta, "Editor-GI-Frame-Delta.png"));
    }
    return XII_SUCCESS;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiDiffuseGICaptureApp);
