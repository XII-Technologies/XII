/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#include <Core/Graphics/Camera.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Memory/FrameAllocator.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/SurfaceRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>


#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER XII_ON
#else
#  define USE_DIRECTORY_WATCHER XII_OFF
#endif

xiiCommandLineOptionInt    opt_GpuDrivenMonitor("GpuDrivenScene", "-monitor", "Monitor used by the scene sample.", 0U);
xiiCommandLineOptionInt    opt_GpuDrivenMeshDetail("GpuDrivenScene", "-mesh-detail", "Sphere segments used to demonstrate independent geometry pages.", 32, 8, 512);
xiiCommandLineOptionString opt_GpuDrivenTexture("GpuDrivenScene", "-texture", "Compiled sampled texture asset used by scene materials.", "");
xiiCommandLineOptionString opt_GpuDrivenMesh("GpuDrivenScene", "-mesh", "Compiled mesh asset to stream in place of generated primitives.", "");
xiiCommandLineOptionString opt_GpuDrivenExportMesh("GpuDrivenScene", "-export-mesh", "Export the generated sphere as a compiled native mesh asset.", "");
xiiCommandLineOptionBool   opt_GpuDrivenAuthoredMaterials("GpuDrivenScene", "-authored-materials", "Render through the authored default material shader.", false);
xiiCommandLineOptionBool   opt_GpuDrivenMeshShaders("GpuDrivenScene", "-mesh-shaders", "Use mesh shaders when supported.", true);
xiiCommandLineOptionFloat  opt_GpuDrivenCameraDistance("GpuDrivenScene", "-camera-distance", "Imported mesh camera distance in mesh radii.", 2.5f, 0.25f, 32.0f);
xiiCommandLineOptionString opt_GpuDrivenCapture("GpuDrivenScene", "-capture-dir", "Save rendered geometry inspection frames in this directory.", "");
xiiCommandLineOptionBool   opt_GpuDrivenCameraTour("GpuDrivenScene", "-camera-tour", "Move between near and distant geometry to exercise page demand.", false);
xiiCommandLineOptionInt    opt_GpuDrivenCameraTourPeriod("GpuDrivenScene", "-camera-tour-period", "Frames per near/far camera cycle.", 720, 120, 14400);
xiiCommandLineOptionInt    opt_GpuDrivenSceneCycle("GpuDrivenScene", "-scene-cycle-period", "Alternate populated and empty worlds after this many frames (zero disables).", 0, 0, 14400);
xiiCommandLineOptionInt    opt_GpuDrivenFrameCount("GpuDrivenScene", "-frames", "Quit normally after this many rendered frames (zero runs until closed).", 0U);
xiiCommandLineOptionInt    opt_GpuDrivenGridWidth("GpuDrivenScene", "-grid-width", "Number of scene instances along the X axis.", 24, 1, 512);
xiiCommandLineOptionInt    opt_GpuDrivenGridHeight("GpuDrivenScene", "-grid-height", "Number of scene instances along the Y axis.", 16, 1, 512);
xiiCommandLineOptionString opt_GpuDrivenPipeline("GpuDrivenScene", "-pipeline", "Render pipeline resource used by the view.", "RenderPipelines/Scene.xiiBinRenderPipeline");
namespace
{
  bool g_bWindowResized = false;
}

class xiiGpuDrivenSceneApp final : public xiiApplication
{
public:
  using SUPER = xiiApplication;
  xiiGpuDrivenSceneApp() : xiiApplication("GPU Driven Scene") {}

  Execution Run() override
  {
    const auto frameLimit = opt_GpuDrivenFrameCount.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    if (frameLimit > 0 && m_uiFrameIndex >= static_cast<xiiUInt64>(frameLimit))
    {
      xiiLog::Info("Completed {} Scene renderer frames.", m_uiFrameIndex);
      return Execution::Quit;
    }
    m_pWindow->ProcessWindowMessages();
    if (WasQuitRequested() || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
      return Execution::Quit;
    if (!m_pWindow->IsVisible())
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(16));
      return Execution::Continue;
    }
    if (g_bWindowResized)
    {
      g_bWindowResized = false;
      UpdateSwapChain();
    }
    xiiClock::GetGlobalClock()->Update();
    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_bFileModified = false;
    m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiGpuDrivenSceneApp::OnFileChanged, this));
    if (m_bFileModified)
      xiiResourceManager::ReloadAllResources(false);
#endif
    const auto scenePeriod = static_cast<xiiUInt32>(opt_GpuDrivenSceneCycle.GetOptionValue(xiiCommandLineOption::LogMode::Never));
    const bool populated   = scenePeriod == 0U || (m_uiFrameIndex / scenePeriod) % 2U == 0U;
    if (populated != m_Configuration.m_bPopulateScene)
    {
      m_World.Shutdown();
      m_Configuration.m_bPopulateScene = populated;
      if (m_World.Initialize(m_pDevice.Borrow(), m_Configuration).Failed())
      {
        xiiLog::Error("Scene transition failed at frame {}.", m_uiFrameIndex);
        SetReturnCode(1);
        return Execution::Quit;
      }
      auto* view = m_World.GetRenderView();
      view->SetCamera(&m_Camera);
      view->SetSwapChain(m_pSwapChain.Borrow());
      view->SetRenderPipelineResource(m_hPipeline);
      xiiLog::Info("Scene transition at frame {}: populated={}", m_uiFrameIndex, populated);
    }
    if (scenePeriod != 0U && !populated && m_uiFrameIndex % 30U == 0U)
      xiiResourceManager::FreeUnusedResources(xiiTime::MakeFromMilliseconds(2), xiiTime::MakeFromSeconds(0.25));
    m_pDevice->BeginFrame();
    if (m_pSwapChain && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && m_pSwapChain->GetBackBufferTexture())
    {
      auto*      view = m_World.GetRenderView();
      const auto size = m_pSwapChain->GetCurrentSize();
      view->SetViewport({0, 0, float(size.width), float(size.height)});
      const bool tour   = opt_GpuDrivenCameraTour.GetOptionValue(xiiCommandLineOption::LogMode::Never);
      const auto period = static_cast<xiiUInt32>(opt_GpuDrivenCameraTourPeriod.GetOptionValue(xiiCommandLineOption::LogMode::Never));
      const auto step   = m_uiFrameIndex % period;
      if (m_ImportedBounds.IsValid())
      {
        float distance = opt_GpuDrivenCameraDistance.GetOptionValue(xiiCommandLineOption::LogMode::Never);
        if (tour)
        {
          const float phase   = static_cast<float>(step) / static_cast<float>(period);
          const float outward = xiiMath::Saturate((phase - 0.2f) * 5.0f);
          const float inward  = xiiMath::Saturate((phase - 0.75f) * 5.0f);
          distance *= 1.0f + 15.0f * (outward - inward);
        }
        const xiiVec3 center = m_ImportedBounds.m_vCenter + xiiVec3(6.0f, 0.0f, 0.0f);
        const xiiVec3 offset = xiiVec3(-1.0f, -1.0f, 0.65f).GetNormalized() * (distance * m_ImportedBounds.m_fSphereRadius);
        m_Camera.LookAt(center + offset, center, xiiVec3::MakeAxisZ());
      }
      else if (tour)
      {
        const float phase    = static_cast<float>(m_uiFrameIndex % period) / static_cast<float>(period);
        const float distance = 4.0f + 90.0f * (1.0f - xiiMath::Cos(xiiAngle::MakeFromRadian(phase * xiiMath::Pi<float>() * 2.0f)));
        m_Camera.LookAt(xiiVec3(25.0f - distance, -2.0f, 1.0f + distance * 0.1f), xiiVec3(25.0f, 0.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f));
      }
      m_World.Update(xiiClock::GetGlobalClock()->GetTimeDiff());
      if (view->GetLastRenderResult().Failed() || view->GetRenderPipelineInstance().GetResourceAcquireResult() != xiiResourceAcquireResult::Final)
      {
        xiiLog::Error("The selected Scene pipeline failed to render frame {}.", m_uiFrameIndex);
        SetReturnCode(1);
        m_pDevice->EndFrame();
        return Execution::Quit;
      }
      ++m_uiFrameIndex;
      if (m_uiFrameIndex == 1)
      {
        const auto& statistics = view->GetRenderGraph()->GetStatistics();
        xiiLog::Info("Scene resource: {} operations, {} submissions, {} barriers.", statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount);
      }
      if (m_uiFrameIndex % 60U == 0U)
      {
        const auto residency = xiiGpuResidencyManager::GetStats();
        for (auto category : {xiiGALMemoryCategory::GeometryPages, xiiGALMemoryCategory::TexturePages})
        {
          xiiGALMemoryCategoryStatistics ownership;
          if (m_pDevice->GetMemoryCategoryStatistics(category, ownership))
            xiiLog::Info("Native {}: {} allocated bytes, {} retiring bytes, {} allocations.", xiiGALMemoryCategory::Names[category], ownership.m_uiAllocatedBytes, ownership.m_uiRetiringBytes, ownership.m_uiAllocationCount);
        }
        xiiLog::Info("Residency frame {}: {} geometry pages / {} bytes, {} texture pages / {} bytes, {} requests / {} misses.",
                     m_uiFrameIndex, residency.m_uiGeometryPages, residency.m_uiGeometryPageBytes, residency.m_uiTexturePages,
                     residency.m_uiTexturePageBytes, residency.m_uiPageRequests, residency.m_uiPageMisses);
        xiiLog::Info("Request identities: {} logical, {} high water, {} retired, {} reuses.", residency.m_uiLogicalPageCount, residency.m_uiRequestIdHighWater, residency.m_uiRetiredRequestIds, residency.m_uiRequestIdReuses);
        xiiLog::Info("Physical pools: {} backing bytes, {} reusable, {} retiring, {} reuses / {} allocations / {} released bytes.", residency.m_uiPoolBackingBytes, residency.m_uiPoolReusableBytes, residency.m_uiPoolRetiringBytes, residency.m_uiPoolReuses, residency.m_uiPoolAllocations, residency.m_uiPoolReleasedBytes);
        xiiLog::Info("Page scheduler: {} geometry requests, {} geometry uploads, {} queued, {} IO pending, {} geometry upload bytes remaining.", residency.m_uiGeometryPageRequests, residency.m_uiGeometryPageUploads, residency.m_uiPendingPageRequests, residency.m_uiPendingPageIO, residency.m_uiRemainingUploadBytes);
      }
      if (!m_sCaptureDir.IsEmpty())
      {
        const char* szCapture = nullptr;
        if (tour)
        {
          if (step == period / 5U - 1U)
            szCapture = "Near";
          if (step == period * 3U / 10U - 1U)
            szCapture = "Medium";
          if (step == period * 3U / 4U - 1U)
            szCapture = "Far";
          if (step == period - 1U)
            szCapture = "Return";
        }
        else if (frameLimit > 0 && m_uiFrameIndex == static_cast<xiiUInt64>(frameLimit))
          szCapture = "Scene";
        if (szCapture != nullptr)
        {
          xiiStringBuilder filename(m_sCaptureDir);
          filename.AppendPath(xiiStringBuilder(szCapture, ".png"));
          m_pWindowOutput->RequestScreenshot(filename);
        }
      }
      m_pWindowOutput->PresentImage();
    }
    m_pDevice->EndFrame();
    xiiTelemetry::PerFrameUpdate();
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();
    xiiFrameAllocator::Swap();
    return Execution::Continue;
  }
  void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder sProjectDirectory = ">sdk/Data/Samples/GpuDrivenScene";

    xiiStringBuilder sResolvedProjectDirectory;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDirectory, sResolvedProjectDirectory).AssertSuccess();
    xiiFileSystem::SetSpecialDirectory("project", sResolvedProjectDirectory);

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    m_pDirectoryWatcher->OpenDirectory(sResolvedProjectDirectory, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif

    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    // Compiled pipelines come from the normal generated asset cache, never source control.
    xiiDataDirectory::FolderType::s_sRedirectionFile = "RenderPipelines/PipelineLibrary.xiiAidlt";
    xiiDataDirectory::FolderType::s_sRedirectionPrefix.Clear();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base/AssetCache/Common", "PipelineCache", "pipelinecache").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

    m_sCaptureDir = opt_GpuDrivenCapture.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    if (!m_sCaptureDir.IsEmpty())
      xiiFileSystem::AddDataDirectory("", "Screenshots", ":", xiiDataDirUsage::AllowWrites).AssertSuccess();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    xiiTelemetry::SetServerName("GPU Driven Scene");

    // Activate xiiTelemetry such that the inspector plugin can use the network connection.
    xiiTelemetry::CreateServer();

    // Load the inspector plugin.
    // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
    // When you are using xiiApplication, this is done automatically.
    xiiPlugin::LoadPlugin("xiiInspectorPlugin", xiiPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif

    {
      xiiInputActionConfig closeAction   = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
      closeAction.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
      xiiInputManager::SetInputActionConfig("Main", "CloseApp", closeAction, true);
    }

    {
      xiiWindowCreationDescription windowDescription;
      windowDescription.m_Resolution       = xiiSizeU32(1440U, 810U);
      windowDescription.m_Title            = GetApplicationName();
      windowDescription.m_bShowMouseCursor = true;
      windowDescription.m_bClipMouseCursor = false;
      windowDescription.m_WindowMode       = xiiWindowMode::WindowResizable;
      windowDescription.m_iMonitor         = opt_GpuDrivenMonitor.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
      windowDescription.AdjustWindowSizeAndPosition().IgnoreResult();

      m_pWindow = XII_DEFAULT_NEW(xiiWindow);
      m_pWindow->Initialize(windowDescription).AssertSuccess();
      m_pWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& event) -> void {
        if (event.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
        {
          RequestQuit();
        }
        else if (event.m_Type == xiiWindowEvent::Type::SizeChanged)
        {
          g_bWindowResized = true;
        }
      });
    }

    {
      xiiGALDeviceCreationDescription deviceDescription;
      deviceDescription.m_DeviceFeatures.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_MeshShaders                   = opt_GpuDrivenMeshShaders.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified) ? xiiGALDeviceFeatureState::Optional : xiiGALDeviceFeatureState::Disabled;
      deviceDescription.m_DeviceFeatures.m_BindlessResources             = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray    = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_TimestampQueries              = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_DurationQueries               = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_RayTracing                    = xiiGALDeviceFeatureState::Disabled;
      deviceDescription.m_DeviceFeatures.m_TextureUAVExtendedFormats     = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_NativeFence                   = xiiGALDeviceFeatureState::Optional; //< Timeline fences are required for GPU-side synchronization between the graphics, asynchronous-compute, and transfer queues.

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      deviceDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
      constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
      constexpr const char* szDefaultGraphicsAPI = "D3D12";
#else
      constexpr const char* szDefaultGraphicsAPI = "";
#endif

      xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
      xiiStringView sShaderModel     = {};
      xiiStringView sShaderCompiler  = {};
      xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

      xiiGALShaderManager::Configure(sShaderModel, true);
      XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found.", sShaderCompiler);

      m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceDescription);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      m_pDevice->SetDebugName("Master Graphics Device");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    UpdateSwapChain();

    m_Configuration                      = {};
    m_Configuration.m_uiMeshDetail       = static_cast<xiiUInt32>(opt_GpuDrivenMeshDetail.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    m_Configuration.m_bAuthoredMaterials = opt_GpuDrivenAuthoredMaterials.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Configuration.m_sMesh              = opt_GpuDrivenMesh.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Configuration.m_sExportMesh        = opt_GpuDrivenExportMesh.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Configuration.m_sTexture           = opt_GpuDrivenTexture.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Configuration.m_uiGridWidth        = static_cast<xiiUInt32>(opt_GpuDrivenGridWidth.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    m_Configuration.m_uiGridHeight       = static_cast<xiiUInt32>(opt_GpuDrivenGridHeight.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    xiiGpuDrivenSceneWorld::ConfigureSubsystems(m_Configuration).AssertSuccess();

    xiiDynamicArray<xiiCVar*> overrides;
    xiiCVar::LoadCVarsFromCommandLine(false, true, &overrides);
    for (auto* value : overrides)
      if (value->GetType() == xiiCVarType::Int)
        xiiLog::Info("CVar {} = {}", value->GetName(), static_cast<xiiCVarInt*>(value)->GetValue());
    xiiStartup::StartupHighLevelSystems();

    m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 250.0f);
    m_Camera.LookAt(xiiVec3(-12.0f, -2.0f, 12.0f), xiiVec3(25.0f, 0.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f));

    m_World.Initialize(m_pDevice.Borrow(), m_Configuration).AssertSuccess();
    m_ImportedBounds = m_World.GetImportedMeshBounds();
    if (m_ImportedBounds.IsValid())
      m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, xiiMath::Max(0.001f, m_ImportedBounds.m_fSphereRadius * 0.0001f), m_ImportedBounds.m_fSphereRadius * 40.0f);
    xiiLog::Info("Vulkan mesh shaders enabled: {}.", m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled);
    auto* view = m_World.GetRenderView();
    view->SetCamera(&m_Camera);
    view->SetSwapChain(m_pSwapChain.Borrow());
    const auto                                 pipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    xiiResourceLock<xiiRenderPipelineResource> loaded(pipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded || !loaded->GetPipeline())
    {
      xiiLog::Error("The selected pipeline resource is unavailable or invalid.");
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hPipeline = pipeline;
    if (!m_sCaptureDir.IsEmpty() && !m_Configuration.m_bAuthoredMaterials && m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled)
    {
      m_hPipeline = CreateMeshCapturePipeline(*loaded.GetPointer());
      if (!m_hPipeline.IsValid())
      {
        SetReturnCode(1);
        RequestQuit();
        return;
      }
    }
    view->SetRenderPipelineResource(m_hPipeline);
  }

  void BeforeHighLevelSystemsShutdown() override
  {
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher.Clear();
#endif
    if (m_pDevice)
      m_pDevice->WaitIdle();
    xiiFrameAllocator::Reset();
    m_World.Shutdown();
    m_hPipeline.Invalidate();
    m_pSwapChain.Clear();
    m_pWindowOutput.Clear();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    m_pDevice.Clear();
    if (m_pWindow)
      m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }
  void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();
    SUPER::BeforeCoreSystemsShutdown();
  }

private:
  xiiRenderPipelineResourceHandle CreateMeshCapturePipeline(const xiiRenderPipelineResource& source)
  {
    auto                       definition     = source.GetAuthoring()->m_Descriptor;
    const auto                 meshGroupCount = xiiMath::Min(65535U, m_pDevice->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties.m_uiMaxThreadGroupCountX);
    xiiDynamicArray<xiiUInt32> commands;
    for (const auto& connection : definition.m_Connections)
    {
      auto& raster = definition.m_Passes[connection.m_uiTarget];
      if (connection.m_sInput == "DrawCommands" &&
          (raster.m_pType == xiiGetStaticRTTI<xiiDepthRasterPass>() || raster.m_pType == xiiGetStaticRTTI<xiiGBufferRasterPass>()) &&
          definition.m_Passes[connection.m_uiSource].m_pType == xiiGetStaticRTTI<xiiVertexPullDrawCommandsPass>())
      {
        raster.m_Properties.Insert("UseMeshShaders", true);
        if (!commands.Contains(connection.m_uiSource))
          commands.PushBack(connection.m_uiSource);
      }
    }
    if (commands.IsEmpty())
    {
      xiiLog::Error("The capture pipeline has no opaque raster passes connected to vertex-pull draw commands.");
      return {};
    }
    for (const auto index : commands)
    {
      auto& command   = definition.m_Passes[index];
      command.m_pType = xiiGetStaticRTTI<xiiMeshDrawCommandsPass>();
      command.m_Properties.Clear();
      command.m_Properties.Insert("MeshGroupCountX", meshGroupCount);
    }
    for (auto& pass : definition.m_Passes)
      if (pass.m_pType == xiiGetStaticRTTI<xiiSurfaceRasterConstantsPass>())
        pass.m_Properties.Insert("MeshGroupCountX", meshGroupCount);
    for (xiiUInt32 i = definition.m_Connections.GetCount(); i-- > 0U;)
    {
      const auto& connection = definition.m_Connections[i];
      if (commands.Contains(connection.m_uiTarget) && connection.m_sInput != "VisibleMeshlets" && connection.m_sInput != "VisibleMeshletCount")
        definition.m_Connections.RemoveAtAndCopy(i);
    }
    return xiiResourceManager::CreateResource<xiiRenderPipelineResource>("GpuDrivenScene.MeshCapture", std::move(definition));
  }

  void UpdateSwapChain()
  {
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription description;
      description.m_pWindow           = m_pWindow.Borrow();
      description.m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      description.m_UsageFlags        = xiiGALSwapChainUsageFlags::RenderTarget;
      description.m_uiBufferCount     = 3U;
      description.m_UsageFlags.AddOrRemove(xiiGALSwapChainUsageFlags::CopySource, !m_sCaptureDir.IsEmpty());
      m_pWindowOutput = XII_DEFAULT_NEW(xiiWindowOutputTargetGAL, description);
      m_pSwapChain    = m_pWindowOutput->m_pSwapChain;
      m_pWindowOutput->SetVSyncEnabled(m_sCaptureDir.IsEmpty());
    }
    else if (m_pSwapChain->GetCurrentSize() != m_pWindow->GetClientAreaSize())
    {
      // Resize destroys the old swapchain. Waiting here guarantees presentation has released all
      // acquired images and satisfies VUID-vkDestroySwapchainKHR-swapchain-01282.
      m_pDevice->WaitIdle();
      m_pSwapChain->Resize(m_pWindow->GetClientAreaSize()).AssertSuccess();
      if (auto* view = m_World.GetRenderView())
        view->InvalidateTemporalHistory();
    }
  }

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  void OnFileChanged(xiiStringView sFileName, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
  {
    if (action == xiiDirectoryWatcherAction::Modified && type == xiiDirectoryWatcherType::File)
    {
      xiiLog::Info("File modified: '{0}'.", sFileName);

      m_bFileModified = true;
    }
  }
#endif

  xiiSharedPtr<xiiGALDevice>             m_pDevice;
  xiiSharedPtr<xiiGALSwapChain>          m_pSwapChain;
  xiiUniquePtr<xiiWindow>                m_pWindow;
  xiiUniquePtr<xiiWindowOutputTargetGAL> m_pWindowOutput;
  xiiString                              m_sCaptureDir;
  xiiBoundingBoxSphere                   m_ImportedBounds = xiiBoundingBoxSphere::MakeInvalid();
  xiiRenderPipelineResourceHandle        m_hPipeline;
  xiiGpuDrivenSceneConfiguration         m_Configuration;
  xiiGpuDrivenSceneWorld                 m_World;
  xiiCamera                              m_Camera;
  xiiUInt64                              m_uiFrameIndex = 0;
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;
#endif
};
XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
