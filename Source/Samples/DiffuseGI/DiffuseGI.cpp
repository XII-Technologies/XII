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
  xiiCommandLineOptionBool   opt_NoReuse("DiffuseGI", "-no-reuse", "Use fresh ray paths instead of reservoir resampling.", false);
  xiiCommandLineOptionInt    opt_Paths("DiffuseGI", "-paths", "Override fresh ray paths per pixel (zero uses the authored count).", 0, 0, 32);
  xiiCommandLineOptionBool   opt_NoRay("DiffuseGI", "-no-ray", "Disable hardware rays to render the scene probe fallback.", false);
  xiiCommandLineOptionBool   opt_ScreenOnly("DiffuseGI", "-screen-only", "Use only screen-space diffuse transport for comparison.", false);
  xiiCommandLineOptionBool   opt_Diagnostics("DiffuseGI", "-diagnostics", "Also capture the Editor GI inspection views.", false);

  xiiRenderPipelineResourceHandle LoadPipeline(xiiStringView sName)
  {
    return xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiStringBuilder("RenderPipelines/", sName, ".xiiBinRenderPipeline"));
  }

  xiiRenderPipelineResourceHandle EnableGI(bool bEditor, bool bScreenOnly)
  {
    const auto                                 handle = LoadPipeline(bEditor ? "Editor" : "Scene");
    xiiResourceLock<xiiRenderPipelineResource> root(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> scene(LoadPipeline("Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!root || !root->GetAuthoring() || !scene || !scene->GetAuthoring())
      return {};
    auto                                       descriptor = root->GetAuthoring()->m_Descriptor;
    const auto                                 sceneId    = scene->GetAuthoring()->m_Descriptor.m_Id;
    const auto                                 gi         = xiiUuid::MakeStableUuidFromString("XII.Scene/ScreenIrradiance");
    xiiResourceLock<xiiRenderPipelineResource> library(LoadPipeline("SceneScreenSpaceIrradiance"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!library || !library->GetAuthoring())
      return {};
    const auto& workload = library->GetAuthoring()->m_Descriptor;
    xiiUuid     ray, trace;
    for (const auto& edge : workload.m_Connections)
      if (edge.m_sInput == "Depth" && workload.m_Passes[edge.m_uiTarget].m_pType == xiiGetStaticRTTI<xiiRayTracedIrradiancePass>())
      {
        ray   = workload.m_Passes[edge.m_uiSource].m_Id;
        trace = workload.m_Passes[edge.m_uiTarget].m_Id;
      }
    if (!ray.IsValid())
      return {};
    const auto gate = bScreenOnly ? xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.SceneScreenSpaceIrradiance/Bounce Depth") : ray;
    for (auto& node : descriptor.m_Passes)
      if (bEditor ? node.m_Definition == sceneId : node.m_Id == gi)
      {
        auto& setting = node.m_Overrides.ExpandAndGetRef();
        if (bEditor)
          setting.m_Path.PushBack(gi);
        setting.m_Path.PushBack(gate);
        setting.m_Properties.Insert("Enabled", true);
        const auto paths     = opt_Paths.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        const bool freshOnly = opt_NoReuse.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        if (freshOnly || paths != 0)
        {
          auto& transport = node.m_Overrides.ExpandAndGetRef();
          if (bEditor)
            transport.m_Path.PushBack(gi);
          transport.m_Path.PushBack(trace);
          if (freshOnly)
            transport.m_Properties.Insert("UsePathReservoirs", false);
          if (paths != 0)
            transport.m_Properties.Insert("SampleCount", xiiUInt32(paths));
        }
        return xiiResourceManager::CreateResource<xiiRenderPipelineResource>(bEditor ? "DiffuseGI.Editor" : "DiffuseGI.Scene", std::move(descriptor));
      }
    return {};
  }

  xiiRenderPipelineResourceHandle EnableLocalLightReservoirs(bool editor, bool reconstruct)
  {
    xiiResourceLock<xiiRenderPipelineResource> scene(LoadPipeline("Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> root(LoadPipeline(editor ? "Editor" : "Scene"), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!scene || !scene->GetAuthoring() || !root || !root->GetAuthoring())
      return {};
    const auto& source = scene->GetAuthoring()->m_Descriptor;
    xiiUuid     lightPass, reconstructionPass;
    for (const auto& pass : source.m_Passes)
    {
      if (pass.m_pType == xiiGetStaticRTTI<xiiClusteredLocalSurfaceLightingPass>())
        lightPass = pass.m_Id;
      else if (pass.m_pType == xiiGetStaticRTTI<xiiLocalLightDenoisePass>())
        reconstructionPass = pass.m_Id;
    }
    if (!lightPass.IsValid() || !reconstructionPass.IsValid())
      return {};
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
      return {};
    const xiiStringBuilder name("DiffuseGI.", editor ? "Editor" : "Scene", reconstruct ? ".FilteredLocalReservoirs" : ".RawLocalReservoirs");
    return xiiResourceManager::CreateResource<xiiRenderPipelineResource>(name, std::move(descriptor));
  }

  void AddLocalLights(xiiWorld& world)
  {
    const xiiColorGammaUB colors[] = {xiiColorGammaUB(255, 217, 168), xiiColorGammaUB(182, 219, 255), xiiColorGammaUB(255, 244, 221)};
    for (xiiUInt32 row = 0; row < 5; ++row)
      for (xiiUInt32 column = 0; column < 6; ++column)
      {
        xiiGameObjectDescription description;
        description.m_LocalPosition = xiiVec3(-1.3f + float(row) * 0.78f, -1.65f + float(column) * 0.66f, 2.88f);
        xiiGameObject* owner        = nullptr;
        world.CreateObject(description, owner);
        xiiPointLightComponent* light = nullptr;
        xiiPointLightComponent::CreateComponent(owner, light);
        light->SetRange(5.5f);
        light->SetIntensity(2.5f + float((row + 3U * column) % 4U) * 0.55f);
        light->SetLightColor(colors[(row + column) % XII_ARRAY_SIZE(colors)]);
      }
  }

  xiiMaterialResourceHandle Material(xiiStringView sName, xiiColor color, xiiColor emission)
  {
    xiiMaterialResourceDescriptor material;
    material.m_BaseColor     = color;
    material.m_EmissiveColor = emission;
    material.m_fRoughness    = 0.9f;
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

  void MakeRoom(xiiWorld& world, bool bSkyOnly, bool bLocalLights)
  {
    const auto white   = Material("GI.White", xiiColor(0.75f, 0.75f, 0.75f), xiiColor::Black);
    const auto red     = Material("GI.Red", xiiColor(0.7f, 0.06f, 0.04f), xiiColor::Black);
    const auto green   = Material("GI.Green", xiiColor(0.06f, 0.62f, 0.14f), xiiColor::Black);
    const auto emitter = Material("GI.Emitter", xiiColor::White, bSkyOnly || bLocalLights ? xiiColor::Black : xiiColor(12, 9, 6));
    const auto box     = [&](xiiStringView sName, xiiVec3 vPosition, xiiVec3 vExtent, const xiiMaterialResourceHandle& material) {
      xiiGeometry geometry;
      geometry.AddBox(vExtent, true);
      AddMesh(world, sName, vPosition, geometry, material);
    };
    box("GI.Floor", {0.4f, 0, -0.1f}, {4.2f, 4.3f, 0.2f}, white);
    box("GI.Ceiling", {0.4f, 0, 3.4f}, {4.2f, 4.3f, 0.2f}, white);
    box("GI.Back", {2.5f, 0, 1.65f}, {0.2f, 4.3f, 3.3f}, white);
    box("GI.Left", {0.4f, 2.15f, 1.65f}, {4.2f, 0.2f, 3.3f}, red);
    box("GI.Right", {0.4f, -2.15f, 1.65f}, {4.2f, 0.2f, 3.3f}, green);
    box("GI.Panel", {0.55f, 0, 3.21f}, {1.2f, 1.25f, 0.12f}, emitter);
    box("GI.Pedestal", {1.1f, 0.95f, 0.64f}, {0.95f, 0.95f, 1.28f}, white);
    xiiGeometry sphere;
    sphere.AddStackedSphere(0.65f, 32, 16);
    AddMesh(world, "GI.Sphere", {0.05f, -0.8f, 0.66f}, sphere, white);

    if (bLocalLights)
    {
      AddLocalLights(world);
      return;
    }

    // A blue sky outside the open side provides a second transport source.
    xiiTextureCubeResourceDescriptor sky;
    sky.m_TextureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    sky.m_TextureDescription.m_Size               = {1, 1};
    sky.m_TextureDescription.m_uiArraySizeOrDepth = 6;
    sky.m_TextureDescription.m_Format             = xiiGALResourceFormat::RGBA32Float;
    sky.m_TextureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    const xiiVec4                skyPixel         = bSkyOnly ? xiiVec4(0.3f, 0.65f, 1.25f, 1) : xiiVec4(0.02f, 0.04f, 0.08f, 1);
    xiiGALTextureSubResourceData faces[6];
    for (auto& face : faces)
      face = xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&skyPixel), sizeof(skyPixel)), sizeof(skyPixel), sizeof(skyPixel));
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
      return XII_FAILURE;
    commands->Begin();
    const auto queued = readback.Enqueue(commands.Borrow(), {.m_pTexture = texture});
    commands->End();
    XII_SUCCEED_OR_RETURN(queued);
    auto* queue = device.GetCommandQueue();
    queue->WaitForFenceValue(queue->Submit(commands));
    const auto capture = readback.GetCompleted();
    if (!capture || capture.m_uiBytesPerPixel != 4)
      return XII_FAILURE;
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    xiiImage image;
    image.ResetAndAlloc(texture->GetDescription());
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    const auto                     mappedResult = commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
    if (mappedResult.Succeeded())
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        xiiMemoryUtils::Copy(image.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y), xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth * 4);
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
      SetReturnCode(1);
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
    xiiStringView model, compiler;
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
    description.m_DeviceFeatures.m_RayTracing                 = opt_NoRay.GetOptionValue(xiiCommandLineOption::LogMode::Never) ? xiiGALDeviceFeatureState::Disabled : xiiGALDeviceFeatureState::Optional;
    description.m_DeviceFeatures.m_MeshShaders                = xiiGALDeviceFeatureState::Disabled;
    auto device                                               = xiiGALDeviceFactory::CreateDevice(renderer, xiiFoundation::GetDefaultAllocator(), description);
    if (!device || device->Initialize().Failed())
      return XII_FAILURE;
    xiiGALDevice::SetDefaultDevice(device);
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr));
    xiiLog::Info("Diffuse transport: {}", xiiRayTracingResources::IsHardwareRayTracingSupported() ? "hardware ray tracing available" : "hardware ray tracing unavailable");

    const bool editor = pipeline == "Editor";
#ifdef XII_DIFFUSE_GI_EDITOR
    if (editor)
      xiiLog::Debug("Editor passes: {}, {}", xiiGetStaticRTTI<xiiInfiniteGridPass>()->GetTypeName(), xiiGetStaticRTTI<xiiSceneMeshPickingPass>()->GetTypeName());
#else
    if (editor)
    {
      xiiLog::Error("The Editor pipeline requires a build with Editor support.");
      return XII_FAILURE;
    }
#endif
    const bool localLights = opt_LocalLights.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    const auto base        = LoadPipeline(pipeline);
    const auto enabled     = localLights ? EnableLocalLightReservoirs(editor, true) : EnableGI(editor, opt_ScreenOnly.GetOptionValue(xiiCommandLineOption::LogMode::Never));
    if (!enabled.IsValid())
      return XII_FAILURE;
    xiiWorldDescription worldDescription("Diffuse GI demonstration");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    MakeRoom(world, opt_SkyOnly.GetOptionValue(xiiCommandLineOption::LogMode::Never), localLights);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 48, 0.1f, 40);
    camera.LookAt({-5.5f, 0, 1.65f}, {1.6f, 0, 1.6f}, {0, 0, 1});
    xiiView* view = nullptr;
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Diffuse GI capture", view);
    view->SetCamera(&camera);
    xiiDisplayOutputSettings display = view->GetDisplayOutputSettings();
    display.m_Exposure.m_Mode        = xiiExposureMode::Manual;
    XII_SUCCEED_OR_RETURN(view->SetDisplayOutputSettings(display));
    constexpr xiiUInt32 width = 640, height = 400;
    view->SetViewport({0, 0, float(width), float(height)});
    const auto render = [&](const xiiRenderPipelineResourceHandle& resource, xiiViewRenderMode::Enum mode, xiiStringView sFilename) -> xiiResult {
      xiiGALTextureCreationDescription targetDescription;
      targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
      targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      targetDescription.m_Size      = {width, height};
      targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
      auto target                   = device->CreateTexture(targetDescription);
      if (!target)
        return XII_FAILURE;
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
          continue;
        if (pass.m_sName == "Update Directional Scene Probes")
          xiiLog::Info("{} used the scene probe update.", sFilename);
        else if (pass.m_sName == "Resolve Current Spatial and Temporal GI")
          xiiLog::Info("{} read current spatial and previous temporal diffuse path reservoirs.", sFilename);
        else if (pass.m_sName == "Resolve Spatial and Temporal Local Light Reservoirs")
          xiiLog::Info("{} resolved spatial and temporal local-light reservoirs.", sFilename);
        else if (pass.m_sName == "Resolve Spatial Local Light Reservoirs")
          xiiLog::Info("{} resolved current local-light reservoirs without temporal history.", sFilename);
        else if (pass.m_sName == "Resolve Current Spatial GI")
          xiiLog::Info("{} read current spatial diffuse path reservoirs without temporal history.", sFilename);
      }
      xiiLog::Info("Saving {}.", sFilename);
      return SaveImage(*device, target.Borrow(), xiiStringBuilder(":capture/", sFilename));
    };
    if (localLights)
    {
      XII_SUCCEED_OR_RETURN(render(base, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Exact.png")));
      const auto raw = EnableLocalLightReservoirs(editor, false);
      if (!raw.IsValid())
        return XII_FAILURE;
      XII_SUCCEED_OR_RETURN(render(raw, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Reservoirs.png")));
      return render(enabled, xiiViewRenderMode::Default, xiiStringBuilder(pipeline, "-Local-Reconstructed.png"));
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
