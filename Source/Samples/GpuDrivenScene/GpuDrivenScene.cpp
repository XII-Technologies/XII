/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
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

#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletCullPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuVisibilityResources.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER XII_ON
#else
#  define USE_DIRECTORY_WATCHER XII_OFF
#endif

xiiCommandLineOptionInt    opt_GpuDrivenMonitor("GpuDrivenScene", "-monitor", "Monitor used by the GPU-driven scene sample.", 0U);
xiiCommandLineOptionInt    opt_GpuDrivenFrameCount("GpuDrivenScene", "-frames", "Quit normally after rendering this many frames (zero runs until closed).", 0U);
xiiCommandLineOptionBool   opt_GpuDrivenRayTracing("GpuDrivenScene", "-ray-tracing", "Enable optional hardware ray tracing scene builds and validation dispatch.", true);
xiiCommandLineOptionInt    opt_GpuDrivenGridWidth("GpuDrivenScene", "-grid-width", "Number of scene instances along the X axis.", 24, 1, 512);
xiiCommandLineOptionInt    opt_GpuDrivenGridHeight("GpuDrivenScene", "-grid-height", "Number of scene instances along the Y axis.", 16, 1, 512);
xiiCommandLineOptionString opt_GpuDrivenDepthPipeline("GpuDrivenScene", "-depth-pipeline", "Authored depth-pyramid/history pipeline resource.", "RenderPipelines/DepthPyramid.xiiBinRenderPipeline");

xiiCommandLineOptionString opt_GpuDrivenVisibilityPipeline("GpuDrivenScene", "-visibility-pipeline", "Authored world mesh visibility/draw resource.", "RenderPipelines/WorldMeshDraws.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowPipeline("GpuDrivenScene", "-shadow-pipeline", "Authored shadow target/raster resource.", "RenderPipelines/ShadowMap.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowPagePipeline("GpuDrivenScene", "-shadow-page-pipeline", "Authored raster resource for supplied virtual page targets.", "RenderPipelines/ShadowMeshDepth.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowFeedbackPipeline("GpuDrivenScene", "-shadow-feedback-pipeline", "Authored virtual shadow demand/readback resource.", "RenderPipelines/VirtualShadowFeedback.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenCascadePipeline("GpuDrivenScene", "-shadow-cascade-pipeline", "Authored stabilized directional cascade resource.", "RenderPipelines/ShadowCascades.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowWorkPipeline("GpuDrivenScene", "-shadow-work-pipeline", "Authored virtual shadow raster work selection resource.", "RenderPipelines/VirtualShadowRasterWork.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowPublishPipeline("GpuDrivenScene", "-shadow-publish-pipeline", "Authored virtual shadow page publication resource.", "RenderPipelines/VirtualShadowPagePublish.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenShadowTablesPipeline("GpuDrivenScene", "-shadow-tables-pipeline", "Authored virtual-shadow residency table upload resource.", "RenderPipelines/VirtualShadowTables.xiiBinRenderPipeline");

xiiCommandLineOptionString opt_GpuDrivenRayProbePipeline("GpuDrivenScene", "-ray-probe-pipeline", "Authored world TLAS and closest-triangle probe resource.", "RenderPipelines/WorldRayProbe.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenDirectionalPipeline("GpuDrivenScene", "-directional-pipeline", "Authored directional mesh constants/shadow sampling/raster resource.", "RenderPipelines/DirectionalMesh.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenTargetsPipeline("GpuDrivenScene", "-targets-pipeline", "Authored scene color/depth initialization resource.", "RenderPipelines/SceneTargets.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenPresentationPipeline("GpuDrivenScene", "-presentation-pipeline", "Authored final view output resource.", "RenderPipelines/ViewPresentation.xiiBinRenderPipeline");
xiiCommandLineOptionString opt_GpuDrivenSensorExportPipeline("GpuDrivenScene", "-sensor-export-pipeline", "Authored external sensor counter lifetime resource.", "RenderPipelines/BufferExport.xiiBinRenderPipeline");

namespace
{
  bool                g_bWindowResized       = false;
  constexpr xiiUInt32 g_uiDirectionalLightId = 1U;

  bool BuildVirtualShadowPageRegion(const xiiMat4& viewProjection, const xiiBoundingBox& worldBounds, xiiUInt32 uiVirtualResolution, xiiUInt32 uiPageSize, xiiUInt32 uiMipLevel, xiiRectU32& out_region)
  {
    if (!worldBounds.IsValid() || uiVirtualResolution == 0U || uiPageSize == 0U || uiVirtualResolution % uiPageSize != 0U || uiMipLevel >= 32U)
      return false;

    xiiVec3 corners[8];
    worldBounds.GetCorners(corners);
    xiiVec2 vMinimum(xiiMath::MaxValue<float>());
    xiiVec2 vMaximum(-xiiMath::MaxValue<float>());
    bool    bProjected = false;
    for (const xiiVec3& corner : corners)
    {
      const xiiVec4 clip = viewProjection * xiiVec4(corner.x, corner.y, corner.z, 1.0f);
      if (xiiMath::Abs(clip.w) <= 1e-6f)
        continue;

      const xiiVec2 uv(clip.x / clip.w * 0.5f + 0.5f, -clip.y / clip.w * 0.5f + 0.5f);
      vMinimum.x = xiiMath::Min(vMinimum.x, uv.x);
      vMinimum.y = xiiMath::Min(vMinimum.y, uv.y);
      vMaximum.x = xiiMath::Max(vMaximum.x, uv.x);
      vMaximum.y = xiiMath::Max(vMaximum.y, uv.y);
      bProjected = true;
    }
    if (!bProjected || vMaximum.x < 0.0f || vMaximum.y < 0.0f || vMinimum.x >= 1.0f || vMinimum.y >= 1.0f)
      return false;

    const xiiUInt32 uiBasePages    = uiVirtualResolution / uiPageSize;
    const xiiUInt32 uiPagesPerAxis = xiiMath::Max(uiBasePages >> uiMipLevel, 1U);
    const xiiUInt32 uiMinimumX     = xiiMath::Min(static_cast<xiiUInt32>(xiiMath::Saturate(vMinimum.x) * uiPagesPerAxis), uiPagesPerAxis - 1U);
    const xiiUInt32 uiMinimumY     = xiiMath::Min(static_cast<xiiUInt32>(xiiMath::Saturate(vMinimum.y) * uiPagesPerAxis), uiPagesPerAxis - 1U);
    const xiiUInt32 uiMaximumX     = xiiMath::Min(static_cast<xiiUInt32>(xiiMath::Saturate(vMaximum.x) * uiPagesPerAxis), uiPagesPerAxis - 1U);
    const xiiUInt32 uiMaximumY     = xiiMath::Min(static_cast<xiiUInt32>(xiiMath::Saturate(vMaximum.y) * uiPagesPerAxis), uiPagesPerAxis - 1U);
    out_region                     = xiiRectU32(uiMinimumX, uiMinimumY, uiMaximumX - uiMinimumX + 1U, uiMaximumY - uiMinimumY + 1U);
    return true;
  }

} // namespace

class xiiGpuDrivenSceneApp final : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiGpuDrivenSceneApp() : xiiApplication("GPU Driven Scene") {}

  Execution Run() override
  {
    const xiiInt32 iFrameLimit = opt_GpuDrivenFrameCount.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    if (iFrameLimit > 0 && m_uiFrameIndex >= static_cast<xiiUInt64>(iFrameLimit))
    {
      xiiLog::Info("Completed {} GPU-driven frames.", m_uiFrameIndex);
      return Execution::Quit;
    }

    m_pWindow->ProcessWindowMessages();
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
    if (WasQuitRequested() || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
      return Execution::Quit;

    xiiClock::GetGlobalClock()->Update();

    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

    // Reload resources if modified
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    {
      m_bFileModified = false;

      m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiGpuDrivenSceneApp::OnFileChanged, this));

      if (m_bFileModified)
      {
        xiiResourceManager::ReloadAllResources(false);
      }
    }
#endif

    m_pDevice->BeginFrame();
    const bool bCanRender = m_pSwapChain != nullptr && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && m_pSwapChain->GetBackBufferTexture() != nullptr;
    if (bCanRender)
    {
      xiiRenderGraphFrame frame;
      if (frame.Begin(m_Configuration.m_uiFramesInFlight).Failed())
      {
        xiiLog::Error("Could not begin the GPU-driven rendering frame.");
        SetReturnCode(1);
        m_pDevice->EndFrame();
        return Execution::Quit;
      }
      m_uiFrameIndex = frame.GetFrameIndex();
      m_World.Update(m_uiFrameIndex, xiiClock::GetGlobalClock()->GetTimeDiff());

      m_TargetSize       = m_pWindow->GetClientAreaSize();
      const float aspect = static_cast<float>(m_TargetSize.width) / static_cast<float>(m_TargetSize.height);
      xiiMat4     projection;
      m_Camera.GetProjectionMatrix(aspect, projection, xiiCameraEye::Left, xiiClipSpaceDepthRange::ZeroToOne);
      // Canonical scene depth is reversed-Z, matching native depth/history and visibility consumers.
      projection.SetRow(2, projection.GetRow(3) - projection.GetRow(2));
      m_ViewProjection = projection * m_Camera.GetViewMatrix();

      xiiStringBuilder              error;
      xiiRenderGraphCompileSettings settings;
      settings.m_bEnablePassCulling   = true;
      settings.m_bEnableCompileCache  = true;
      settings.m_bEnableAsyncQueues   = false;
      settings.m_bEnableSplitBarriers = false;
      settings.m_bEnableGPUProfiling  = true;
      if (xiiRenderGraphManager::ExecuteGraph(frame, m_hRenderGraph, nullptr, settings, &error).Succeeded())
      {
        if (m_uiFrameIndex == 1U || (m_uiVirtualPagesThisFrame != 0 && !m_bReportedVirtualBatch))
        {
          const xiiRenderGraphStatistics& statistics = xiiRenderGraphManager::GetGraph(m_hRenderGraph)->GetStatistics();
          xiiLog::Info("GPU-driven render graph: {} registered, {} compiled, {} culled passes; {} queue submissions, {} barriers ({} split).", statistics.m_uiRegisteredPassCount, statistics.m_uiCompiledPassCount, statistics.m_uiCulledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount, statistics.m_uiSplitBarrierCount);
          if (m_uiVirtualPagesThisFrame != 0)
          {
            xiiLog::Info("Authored virtual shadow batch: {} physical pages in device frame {}.", m_uiVirtualPagesThisFrame, m_uiFrameIndex);
            m_bReportedVirtualBatch = true;
          }
        }
      }
      else
      {
        xiiLog::Error("GPU-driven render graph execution failed: {0}", error);
        SetReturnCode(1);
        frame.End();
        m_pDevice->EndFrame();
        return Execution::Quit;
      }
      if (WasQuitRequested())
      {
        frame.End();
        m_pDevice->EndFrame();
        return Execution::Quit;
      }
      m_pSwapChain->Present();
    }
    else if (m_pSwapChain)
    {
      m_pSwapChain->Present();
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
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

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
      deviceDescription.m_DeviceFeatures.m_MeshShaders                   = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_BindlessResources             = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray    = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_TimestampQueries              = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_DurationQueries               = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_RayTracing                    = opt_GpuDrivenRayTracing.GetOptionValue(xiiCommandLineOption::LogMode::Always) ? xiiGALDeviceFeatureState::Optional : xiiGALDeviceFeatureState::Disabled;
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

    m_Configuration                = {};
    m_Configuration.m_uiGridWidth  = static_cast<xiiUInt32>(opt_GpuDrivenGridWidth.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    m_Configuration.m_uiGridHeight = static_cast<xiiUInt32>(opt_GpuDrivenGridHeight.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    xiiGpuDrivenSceneWorld::ConfigureSubsystems(m_Configuration).AssertSuccess();

    xiiStartup::StartupHighLevelSystems();

    m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 250.0f);
    m_Camera.LookAt(xiiVec3(-12.0f, -2.0f, 12.0f), xiiVec3(25.0f, 0.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f));

    m_World.Initialize(m_pDevice.Borrow(), m_Configuration).AssertSuccess();
    m_hVisibilityPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenVisibilityPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeVisibilityPipelines().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hShadowPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeShadowPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hShadowPagePipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowPagePipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeShadowPagePipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hShadowTablesPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowTablesPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeShadowTablesPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hShadowWorkPipeline    = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowWorkPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    m_hShadowPublishPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowPublishPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeShadowDataPipelines().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hCascadePipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenCascadePipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeCascadePipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hShadowFeedbackPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenShadowFeedbackPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeShadowFeedbackPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    m_hDepthPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenDepthPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeDepthPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    m_hTargetsPipeline      = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenTargetsPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    m_hPresentationPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenPresentationPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    m_hSensorExportPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenSensorExportPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeViewBoundaryPipelines().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    m_hDirectionalPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenDirectionalPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    if (SynchronizeDirectionalPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    if (m_pDevice->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled)
    {
      m_hRayProbePipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenRayProbePipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
      if (SynchronizeRayProbePipeline().Failed())
      {
        SetReturnCode(1);
        RequestQuit();
        return;
      }
    }

    m_hRenderGraph = xiiRenderGraphManager::CreateGraph("GPU Driven Scene", xiiMakeDelegate(&xiiGpuDrivenSceneApp::BuildRenderGraph, this));
  }

  void BeforeHighLevelSystemsShutdown() override
  {
    xiiTelemetry::CloseConnection();

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher->CloseDirectory();
    m_pDirectoryWatcher.Clear();
#endif

    if (m_pDevice)
      m_pDevice->WaitIdle();
    if (!m_hRenderGraph.IsInvalidated())
      xiiRenderGraphManager::DestroyGraph(m_hRenderGraph).AssertSuccess("Failed to destroy the GPU-driven render graph.");
    m_Targets.Clear();
    m_Presentation.Clear();
    m_SensorExport.Clear();
    m_hTargetsPipeline.Invalidate();
    m_hPresentationPipeline.Invalidate();
    m_hSensorExportPipeline.Invalidate();
    m_VirtualPageRuntimes.Clear();
    m_ShadowPageDefinition = nullptr;
    m_hShadowPagePipeline.Invalidate();
    m_ShadowWork.Clear();
    m_ShadowPublish.Clear();
    m_hShadowWorkPipeline.Invalidate();
    m_hShadowPublishPipeline.Invalidate();
    m_ShadowTables.Clear();
    m_hShadowTablesPipeline.Invalidate();
    m_Cascades.Clear();
    m_hCascadePipeline.Invalidate();
    m_ShadowFeedback.Clear();
    m_hShadowFeedbackPipeline.Invalidate();
    m_SunShadow.Clear();
    m_hShadowPipeline.Invalidate();
    m_DepthPipeline.Clear();
    m_hDepthPipeline.Invalidate();
    m_MainVisibility.Clear();
    m_SensorVisibility.Clear();
    m_ShadowVisibility.Clear();
    m_hVisibilityPipeline.Invalidate();
    xiiFrameAllocator::Reset();
    m_World.Shutdown(m_uiFrameIndex);
    m_RayProbe.Clear();
    m_hRayProbePipeline.Invalidate();
    m_Directional.Clear();
    m_hDirectionalPipeline.Invalidate();
    m_pSwapChain.Clear();
    xiiStartup::ShutdownHighLevelSystems();
    if (xiiGALDevice::GetDefaultDevice() == m_pDevice)
      xiiGALDevice::SetDefaultDevice(nullptr);
    m_pDevice.Clear();
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }

  void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();

    SUPER::BeforeCoreSystemsShutdown();
  }

private:
  xiiResult SynchronizeRayProbePipeline()
  {
    if (m_RayProbe.Synchronize(m_hRayProbePipeline).Failed())
    {
      xiiLog::Error("Selected ray-probe resource '{}' is missing or invalid.", m_hRayProbePipeline.GetResourceID());
      return XII_FAILURE;
    }
    const auto* definition = m_RayProbe.GetPipeline();
    const auto  result     = definition->FindInterface("Result", false);
    if (result == xiiInvalidIndex || definition->GetInterface()[result].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
    {
      xiiLog::Error("Ray-probe resource '{}' requires buffer output 'Result'.", m_hRayProbePipeline.GetResourceID());
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  xiiResult SynchronizeDirectionalPipeline()
  {
    if (m_Directional.Synchronize(m_hDirectionalPipeline).Failed())
    {
      xiiLog::Error("Selected directional mesh resource '{}' is missing or invalid.", m_hDirectionalPipeline.GetResourceID());
      return XII_FAILURE;
    }
    const auto* pipeline = m_Directional.GetPipeline();
    for (xiiStringView name : {"SceneInstances", "SkinningMatrices", "GeometryMetadata", "MeshletMetadata", "VisibleMeshlets", "VisibleMeshletCount", "DrawCommands", "DrawCommandCount", "SurfaceMaterials", "VirtualPageTable"})
    {
      const auto index = pipeline->FindInterface(name, true);
      if (index == xiiInvalidIndex || pipeline->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
      {
        xiiLog::Error("Directional mesh resource '{}' requires buffer input '{}'.", m_hDirectionalPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    for (xiiStringView name : {"ColorTarget", "DepthTarget", "ShadowMap", "VirtualShadowAtlas"})
    {
      const auto index = pipeline->FindInterface(name, true);
      if (index == xiiInvalidIndex || pipeline->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Texture)
      {
        xiiLog::Error("Directional mesh resource '{}' requires texture input '{}'.", m_hDirectionalPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    for (xiiStringView name : {"Cascades", "Residency"})
    {
      const auto index = pipeline->FindInterface(name, true);
      if (index == xiiInvalidIndex || pipeline->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::FrameData)
      {
        xiiLog::Error("Directional mesh resource '{}' requires frame-data input '{}'.", m_hDirectionalPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    for (xiiStringView name : {"Color", "Depth"})
    {
      const auto index = pipeline->FindInterface(name, false);
      if (index == xiiInvalidIndex || pipeline->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Texture)
      {
        xiiLog::Error("Directional mesh resource '{}' requires texture output '{}'.", m_hDirectionalPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult SynchronizeVisibilityPipelines()
  {
    bool changed = false;
    for (auto* instance : {&m_MainVisibility, &m_SensorVisibility, &m_ShadowVisibility})
    {
      bool occurrenceChanged = false;
      if (instance->Synchronize(m_hVisibilityPipeline, &occurrenceChanged).Failed())
      {
        xiiLog::Error("Selected visibility resource '{}' is missing or invalid.", m_hVisibilityPipeline.GetResourceID());
        return XII_FAILURE;
      }
      changed |= occurrenceChanged;
    }
    if (!changed)
      return XII_SUCCESS;
    const auto* pipeline = m_MainVisibility.GetPipeline();
    for (const xiiStringView name : {"SceneInstances", "SkinningMatrices", "VisibleInstances", "VisibleInstanceCount", "VisibleMeshlets", "VisibleMeshletCount", "DrawCommands", "DrawCommandCount", "VisibleMeshletOverflow"})
    {
      const auto index = pipeline->FindInterface(name, false);
      if (index == xiiInvalidIndex || pipeline->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
      {
        xiiLog::Error("Visibility resource '{}' requires buffer output '{}'.", m_hVisibilityPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    xiiUInt32 commands = 0, culling = 0;
    for (xiiUInt32 i = 0; i < pipeline->GetDescriptor().m_Passes.GetCount(); ++i)
    {
      const auto* pass = m_MainVisibility.GetPass(i);
      if (pass->GetDynamicRTTI()->IsDerivedFrom<xiiMeshDrawCommandsPass>())
      {
        const auto* native = static_cast<const xiiMeshDrawCommandsPass*>(pass);
        m_uiMeshGroupX     = native->m_uiMeshGroupCountX;
        m_uiMeshGroupY     = native->m_uiMeshGroupCountY;
        ++commands;
      }
      if (pass->GetDynamicRTTI()->IsDerivedFrom<xiiMeshletCullPass>())
      {
        m_uiVisibilityCapacity = static_cast<const xiiMeshletCullPass*>(pass)->m_uiMaxVisibleMeshlets;
        ++culling;
      }
    }
    if (commands != 1 || culling != 1)
    {
      xiiLog::Error("This sample requires one native meshlet culler and mesh command producer in the selected visibility resource.");
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  xiiResult BuildVisibility(xiiRenderPipelineInstance& instance, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiMat4& projection, xiiUInt32 width, xiiUInt32 height, xiiUInt32 flags, xiiUInt32 priority, xiiGpuVisibilityOutputs& outputs)
  {
    xiiVariantDictionary parameters;
    parameters.Insert("CullingViewProjection", projection);
    parameters.Insert("CullingCameraPosition", m_Camera.GetPosition());
    parameters.Insert("CullingWidth", width);
    parameters.Insert("CullingHeight", height);
    parameters.Insert("CullingRequiredFlags", flags);
    parameters.Insert("StreamingPriorityTier", priority);
    auto* view = m_World.GetRenderView();
    if (!view)
      return XII_FAILURE;
    XII_LOCK(view->GetWorld()->GetReadMarker());
    view->SetViewport({0, 0, float(width), float(height)});
    if (instance.Build(view, graph, blackboard, parameters).Failed())
    {
      xiiLog::Error("Authored visibility failed: {}", instance.GetLastBuildError());
      return XII_FAILURE;
    }
    const auto buffer = [&](xiiStringView name) {
      return instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface(name, false))->m_hBuffer;
    };
    outputs.m_hSceneInstances       = buffer("SceneInstances");
    outputs.m_hSkinningMatrices     = buffer("SkinningMatrices");
    outputs.m_hVisibleInstances     = buffer("VisibleInstances");
    outputs.m_hVisibleInstanceCount = buffer("VisibleInstanceCount");
    outputs.m_hVisibleMeshlets      = buffer("VisibleMeshlets");
    outputs.m_hVisibleMeshletCount  = buffer("VisibleMeshletCount");
    outputs.m_hIndirectCommands     = buffer("DrawCommands");
    outputs.m_hIndirectCommandCount = buffer("DrawCommandCount");
    outputs.m_uiMaxVisibleMeshlets  = m_uiVisibilityCapacity;
    return XII_SUCCESS;
  }

  xiiResult SynchronizeShadowPipeline()
  {
    bool changed = false;
    if (m_SunShadow.Synchronize(m_hShadowPipeline, &changed).Failed())
    {
      xiiLog::Error("Selected shadow resource '{}' is missing or invalid.", m_hShadowPipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (changed)
    {
      const auto* pipeline = m_SunShadow.GetPipeline();
      for (const xiiStringView name : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount", "GeometryMetadata", "MeshletMetadata", "SurfaceMaterials", "DrawCommands", "DrawCommandCount"})
      {
        const auto input = pipeline->FindInterface(name, true);
        if (input == xiiInvalidIndex || pipeline->GetInterface()[input].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
        {
          xiiLog::Error("Selected shadow resource '{}' requires buffer input '{}'.", m_hShadowPipeline.GetResourceID(), name);
          return XII_FAILURE;
        }
      }
      m_uiShadowOutput = pipeline->FindInterface("ShadowDepth", false);
      if (m_uiShadowOutput == xiiInvalidIndex || pipeline->GetInterface()[m_uiShadowOutput].m_Pin.m_Type != xiiRenderPipelinePin::Texture || pipeline->GetInterface()[m_uiShadowOutput].m_Pin.m_Format != xiiGALResourceFormat::D32Float)
      {
        xiiLog::Error("Selected shadow resource '{}' requires a D32 ShadowDepth output.", m_hShadowPipeline.GetResourceID());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult BuildShadowResource(xiiRenderPipelineInstance& instance, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard,
                                const xiiMat4& projection, xiiSizeU32 size, const xiiVec4U32& viewport,
                                const xiiGpuVisibilityOutputs& visibility, const xiiGeometryResidencyManager::UploadHandles& geometry,
                                const xiiMaterialGpuStorage::UploadHandles& materials, xiiRenderGraphTextureHandle& output, xiiRenderGraphTextureHandle target = {})
  {
    xiiVariantDictionary parameters;
    parameters.Insert("RasterViewProjection", projection);
    parameters.Insert("RasterPreviousViewProjection", projection);
    parameters.Insert("RasterWidth", size.width);
    parameters.Insert("RasterHeight", size.height);
    parameters.Insert("RasterViewport", viewport);
    parameters.Insert("RasterMeshGroupCountX", m_uiMeshGroupX);
    parameters.Insert("RasterMeshGroupCountY", m_uiMeshGroupY);
    xiiHybridArray<xiiRenderPipelineInputBinding, 10> inputs;
    const auto                                        bind = [&](xiiStringView name, xiiRenderGraphBufferHandle buffer) {
      inputs.PushBack(xiiRenderPipelineInputBinding(graph, instance.GetPipeline()->FindInterface(name, true), buffer));
    };
    bind("SceneInstances", visibility.m_hSceneInstances);
    bind("SkinningMatrices", visibility.m_hSkinningMatrices);
    bind("VisibleMeshlets", visibility.m_hVisibleMeshlets);
    bind("VisibleMeshletCount", visibility.m_hVisibleMeshletCount);
    bind("GeometryMetadata", geometry.m_hGeometryMetadata);
    bind("MeshletMetadata", geometry.m_hMeshletMetadata);
    bind("SurfaceMaterials", materials.m_hSurfaceData);
    bind("DrawCommands", visibility.m_hIndirectCommands);
    bind("DrawCommandCount", visibility.m_hIndirectCommandCount);
    if (target.IsValid())
      inputs.PushBack(xiiRenderPipelineInputBinding(graph, instance.GetPipeline()->FindInterface("DepthTarget", true), target));
    if (instance.Build(nullptr, graph, blackboard, parameters, inputs).Failed())
    {
      xiiLog::Error("Authored shadow build failed: {}", instance.GetLastBuildError());
      return XII_FAILURE;
    }
    output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("ShadowDepth", false))->m_hTexture;
    return output.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }

  xiiResult SynchronizeShadowPagePipeline()
  {
    xiiResourceLock<xiiRenderPipelineResource> loaded(m_hShadowPagePipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    const auto                                 compiled = loaded.IsValid() ? loaded->GetPipeline() : xiiSharedPtr<const xiiRenderPipeline>();
    if (!compiled)
    {
      m_ShadowPageDefinition = nullptr;
      xiiLog::Error("Selected virtual-page resource '{}' is missing or invalid.", m_hShadowPagePipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (compiled != m_ShadowPageDefinition)
    {
      for (const xiiStringView name : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount", "GeometryMetadata", "MeshletMetadata", "SurfaceMaterials", "DrawCommands", "DrawCommandCount"})
      {
        const auto index = compiled->FindInterface(name, true);
        if (index == xiiInvalidIndex || compiled->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
        {
          xiiLog::Error("Virtual-page resource '{}' requires buffer input '{}'.", m_hShadowPagePipeline.GetResourceID(), name);
          return XII_FAILURE;
        }
      }
      for (const bool input : {true, false})
      {
        const auto index = compiled->FindInterface(input ? "DepthTarget" : "ShadowDepth", input);
        if (index == xiiInvalidIndex || compiled->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Texture || compiled->GetInterface()[index].m_Pin.m_Format != xiiGALResourceFormat::D32Float)
        {
          xiiLog::Error("Virtual-page resource '{}' requires a D32 target input and depth output.", m_hShadowPagePipeline.GetResourceID());
          return XII_FAILURE;
        }
      }
      m_ShadowPageDefinition = compiled;
    }
    return XII_SUCCESS;
  }

  struct VirtualPageRuntime
  {
    xiiRenderPipelineInstance m_Pipeline;
    xiiUInt64                 m_uiContentRevision = 0;
  };

  xiiResult BuildVirtualShadowPages(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiVirtualShadowTableBindings& upload, const xiiSharedPtr<const xiiShadowCascadeSnapshot>& cascades, const xiiSharedPtr<const xiiVirtualShadowResidencySnapshot>& residency, const xiiGpuVisibilityOutputs& visibility, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiMaterialGpuStorage::UploadHandles& materials)
  {
    XII_SUCCEED_OR_RETURN(SynchronizeShadowPagePipeline());
    const auto count = xiiVirtualShadowMapManager::GetConfiguration().m_uiPhysicalPageCount;
    m_VirtualPageRuntimes.SetCount(count);
    XII_SUCCEED_OR_RETURN(SynchronizeShadowDataPipelines());
    xiiRenderPipelineBinding cascadeInput, residencyInput;
    cascadeInput.m_pFrameData                          = cascades;
    residencyInput.m_pFrameData                        = residency;
    const auto*                         workDefinition = m_ShadowWork.GetPipeline();
    const xiiRenderPipelineInputBinding workInputs[]   = {
      {graph, workDefinition->FindInterface("Cascades", true), cascadeInput},
      {graph, workDefinition->FindInterface("Residency", true), residencyInput}};
    xiiVariantDictionary parameters;
    parameters.Insert("ShadowRasterLightId", g_uiDirectionalLightId);
    if (m_ShadowWork.Build(nullptr, graph, blackboard, parameters, xiiMakeArrayPtr(workInputs)).Failed())
    {
      xiiLog::Error("Shadow work resource '{}': {}", m_hShadowWorkPipeline.GetResourceID(), m_ShadowWork.GetLastBuildError());
      return XII_FAILURE;
    }
    const auto batch          = m_ShadowWork.GetInterfaceBinding(workDefinition->FindInterface("RasterPages", false))->m_pFrameData.Downcast<const xiiVirtualShadowRasterBatch>();
    m_uiVirtualPagesThisFrame = batch->m_Pages.GetCount();
    for (const auto& page : batch->m_Pages)
    {
      auto& runtime = m_VirtualPageRuntimes[page.m_uiPhysicalPage];
      if (!runtime)
        runtime = XII_DEFAULT_NEW(VirtualPageRuntime);
      XII_SUCCEED_OR_RETURN(runtime->m_Pipeline.Initialize(m_ShadowPageDefinition));
      if (runtime->m_uiContentRevision != page.m_uiContentRevision)
      {
        runtime->m_Pipeline.InvalidateTemporalHistory();
        runtime->m_uiContentRevision = page.m_uiContentRevision;
      }
      const auto& region = page.m_Viewport;
      if (BuildShadowResource(runtime->m_Pipeline, graph, blackboard, page.m_ViewProjection, {region.width, region.height}, xiiVec4U32(region.x, region.y, region.width, region.height), visibility, geometry, materials, upload.m_hPhysicalAtlas, upload.m_hPhysicalAtlas).Failed())
      {
        xiiLog::Error("Virtual shadow page {} in physical slot {} failed native resource execution.", page.m_Page.GetPackedValue(), page.m_uiPhysicalPage);
        return XII_FAILURE;
      }
    }
    const auto*              publish = m_ShadowPublish.GetPipeline();
    xiiRenderPipelineBinding batchInput;
    batchInput.m_pFrameData                             = batch;
    const xiiRenderPipelineInputBinding publishInputs[] = {
      {graph, publish->FindInterface("RasterPages", true), batchInput},
      {graph, publish->FindInterface("RasterAtlas", true), upload.m_hPhysicalAtlas},
      {graph, publish->FindInterface("PhysicalPageTable", true), upload.m_hPhysicalPageTable},
      {graph, publish->FindInterface("VirtualPageTable", true), upload.m_hVirtualPageTable}};
    if (m_ShadowPublish.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(publishInputs)).Failed())
    {
      xiiLog::Error("Shadow publication resource '{}': {}", m_hShadowPublishPipeline.GetResourceID(), m_ShadowPublish.GetLastBuildError());
      return XII_FAILURE;
    }
    upload.m_hPhysicalAtlas     = m_ShadowPublish.GetInterfaceBinding(publish->FindInterface("ReadyAtlas", false))->m_hTexture;
    upload.m_hPhysicalPageTable = m_ShadowPublish.GetInterfaceBinding(publish->FindInterface("ReadyPhysicalPageTable", false))->m_hBuffer;
    upload.m_hVirtualPageTable  = m_ShadowPublish.GetInterfaceBinding(publish->FindInterface("ReadyVirtualPageTable", false))->m_hBuffer;
    return XII_SUCCESS;
  }

  xiiResult SynchronizeShadowDataPipelines()
  {
    if (m_ShadowWork.Synchronize(m_hShadowWorkPipeline).Failed() || m_ShadowPublish.Synchronize(m_hShadowPublishPipeline).Failed())
    {
      xiiLog::Error("Selected shadow work/publication resources '{}' and '{}' must both be valid.", m_hShadowWorkPipeline.GetResourceID(), m_hShadowPublishPipeline.GetResourceID());
      return XII_FAILURE;
    }
    const auto data = [&](const xiiRenderPipeline* pipeline, xiiStringView name, bool input, const xiiRTTI* type) {
      const auto index = pipeline->FindInterface(name, input);
      return index != xiiInvalidIndex && pipeline->GetInterface()[index].m_Pin.m_Type == xiiRenderPipelinePin::FrameData && pipeline->GetInterface()[index].m_Pin.m_pDataType == type;
    };
    const auto* work    = m_ShadowWork.GetPipeline();
    const auto* publish = m_ShadowPublish.GetPipeline();
    if (!data(work, "Cascades", true, xiiGetStaticRTTI<xiiShadowCascadeSnapshot>()) || !data(work, "Residency", true, xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>()) || !data(work, "RasterPages", false, xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()) || !data(publish, "RasterPages", true, xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()))
    {
      xiiLog::Error("Selected shadow work/publication resources require typed Cascades, Residency and RasterPages interfaces.");
      return XII_FAILURE;
    }
    for (const xiiStringView name : {"PhysicalPageTable", "VirtualPageTable", "ReadyPhysicalPageTable", "ReadyVirtualPageTable", "RasterAtlas", "ReadyAtlas"})
    {
      const bool input = !name.StartsWith("Ready");
      const auto index = publish->FindInterface(name, input);
      const auto kind  = name.EndsWith("Atlas") ? xiiRenderPipelinePin::Texture : xiiRenderPipelinePin::Buffer;
      if (index == xiiInvalidIndex || publish->GetInterface()[index].m_Pin.m_Type != kind || (kind == xiiRenderPipelinePin::Texture && publish->GetInterface()[index].m_Pin.m_Format != xiiGALResourceFormat::D32Float))
      {
        xiiLog::Error("Shadow publication resource '{}' has an incompatible '{}' interface.", m_hShadowPublishPipeline.GetResourceID(), name);
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult SynchronizeShadowTablesPipeline()
  {
    bool changed = false;
    if (m_ShadowTables.Synchronize(m_hShadowTablesPipeline, &changed).Failed())
    {
      xiiLog::Error("Selected shadow-table resource '{}' is missing or invalid.", m_hShadowTablesPipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (changed)
    {
      const auto* pipeline = m_ShadowTables.GetPipeline();
      for (const xiiStringView name : {"PhysicalPageTable", "VirtualPageTable"})
      {
        const auto output = pipeline->FindInterface(name, false);
        if (output == xiiInvalidIndex || pipeline->GetInterface()[output].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
        {
          xiiLog::Error("Shadow-table resource '{}' requires buffer output '{}'.", m_hShadowTablesPipeline.GetResourceID(), name);
          return XII_FAILURE;
        }
      }
      const auto residency = pipeline->FindInterface("Residency", false);
      if (residency == xiiInvalidIndex || pipeline->GetInterface()[residency].m_Pin.m_Type != xiiRenderPipelinePin::FrameData || pipeline->GetInterface()[residency].m_Pin.m_pDataType != xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>())
      {
        xiiLog::Error("Shadow-table resource '{}' requires a typed Residency output.", m_hShadowTablesPipeline.GetResourceID());
        return XII_FAILURE;
      }
      const auto atlas = pipeline->FindInterface("PhysicalAtlas", false);
      if (atlas == xiiInvalidIndex || pipeline->GetInterface()[atlas].m_Pin.m_Type != xiiRenderPipelinePin::Texture || pipeline->GetInterface()[atlas].m_Pin.m_Format != xiiGALResourceFormat::D32Float)
      {
        xiiLog::Error("Shadow-table resource '{}' requires a D32 PhysicalAtlas output.", m_hShadowTablesPipeline.GetResourceID());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult BuildShadowTables(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiVirtualShadowTableBindings& output, xiiSharedPtr<const xiiVirtualShadowResidencySnapshot>& residency)
  {
    XII_SUCCEED_OR_RETURN(SynchronizeShadowTablesPipeline());
    if (m_ShadowTables.Build(nullptr, graph, blackboard).Failed())
    {
      xiiLog::Error("Authored shadow-table upload failed: {}", m_ShadowTables.GetLastBuildError());
      return XII_FAILURE;
    }
    const auto* compiled = m_ShadowTables.GetPipeline();
    const auto  atlas    = m_ShadowTables.GetInterfaceBinding(compiled->FindInterface("PhysicalAtlas", false))->m_hTexture;
    const auto  physical = m_ShadowTables.GetInterfaceBinding(compiled->FindInterface("PhysicalPageTable", false))->m_hBuffer;
    const auto  table    = m_ShadowTables.GetInterfaceBinding(compiled->FindInterface("VirtualPageTable", false))->m_hBuffer;
    const auto* metadata = m_ShadowTables.GetInterfaceBinding(compiled->FindInterface("Residency", false));
    residency            = metadata ? metadata->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>() : xiiSharedPtr<const xiiVirtualShadowResidencySnapshot>();
    if (!residency || residency->GetSetupSerial() != graph.GetSetupSerial() || residency->m_Bindings.m_hPhysicalAtlas.GetResourceId() != atlas.GetResourceId() || residency->m_Bindings.m_hPhysicalPageTable.GetResourceId() != physical.GetResourceId() || residency->m_Bindings.m_hVirtualPageTable.GetResourceId() != table.GetResourceId())
    {
      xiiLog::Error("Shadow-table resource '{}' must expose matching typed Residency and explicit GPU outputs.", m_hShadowTablesPipeline.GetResourceID());
      return XII_FAILURE;
    }
    output                      = residency->m_Bindings;
    output.m_hPhysicalAtlas     = atlas;
    output.m_hPhysicalPageTable = physical;
    output.m_hVirtualPageTable  = table;
    return XII_SUCCESS;
  }

  xiiResult SynchronizeCascadePipeline()
  {
    bool changed = false;
    if (m_Cascades.Synchronize(m_hCascadePipeline, &changed).Failed())
    {
      xiiLog::Error("Selected cascade resource '{}' is missing or invalid.", m_hCascadePipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (changed)
    {
      const auto* pipeline = m_Cascades.GetPipeline();
      const auto  snapshot = pipeline->FindInterface("CascadeSnapshot", false);
      if (snapshot == xiiInvalidIndex || pipeline->GetInterface()[snapshot].m_Pin.m_Type != xiiRenderPipelinePin::FrameData || pipeline->GetInterface()[snapshot].m_Pin.m_pDataType != xiiGetStaticRTTI<xiiShadowCascadeSnapshot>())
      {
        xiiLog::Error("Cascade resource '{}' requires a typed CascadeSnapshot output.", m_hCascadePipeline.GetResourceID());
        return XII_FAILURE;
      }
      m_uiCascadeOutput = pipeline->FindInterface("CascadeConstants", false);
      if (m_uiCascadeOutput == xiiInvalidIndex || pipeline->GetInterface()[m_uiCascadeOutput].m_Pin.m_Type != xiiRenderPipelinePin::Buffer || !pipeline->GetInterface()[m_uiCascadeOutput].m_Pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
      {
        xiiLog::Error("Cascade resource '{}' requires a native directional snapshot and uniform CascadeConstants output.", m_hCascadePipeline.GetResourceID());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult BuildCascades(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiSharedPtr<const xiiShadowCascadeSnapshot>& snapshot, xiiRenderGraphBufferHandle& constants, xiiUInt32& resolution)
  {
    XII_SUCCEED_OR_RETURN(SynchronizeCascadePipeline());
    auto* view = m_World.GetRenderView();
    if (!view)
      return XII_FAILURE;
    xiiVariantDictionary parameters;
    parameters.Insert("ShadowLightDirection", m_World.GetSunLight().m_vDirection);
    XII_LOCK(view->GetWorld()->GetReadMarker());
    view->SetCamera(&m_Camera);
    view->SetViewport({0, 0, float(m_TargetSize.width), float(m_TargetSize.height)});
    if (m_Cascades.Build(view, graph, blackboard, parameters).Failed())
    {
      xiiLog::Error("Authored cascades failed: {}", m_Cascades.GetLastBuildError());
      return XII_FAILURE;
    }
    constants            = m_Cascades.GetInterfaceBinding(m_uiCascadeOutput)->m_hBuffer;
    const auto* metadata = m_Cascades.GetInterfaceBinding(m_Cascades.GetPipeline()->FindInterface("CascadeSnapshot", false));
    snapshot             = metadata ? metadata->m_pFrameData.Downcast<const xiiShadowCascadeSnapshot>() : xiiSharedPtr<const xiiShadowCascadeSnapshot>();
    if (!snapshot || snapshot->GetSetupSerial() != graph.GetSetupSerial() || snapshot->m_hConstants != constants || snapshot->m_Cascades.GetCount() != 1)
    {
      xiiLog::Error("Cascade resource '{}' must expose a matching CascadeSnapshot and one projection for this sample's remaining main shading.", m_hCascadePipeline.GetResourceID());
      return XII_FAILURE;
    }
    resolution = snapshot->m_uiResolution;
    return XII_SUCCESS;
  }

  xiiResult SynchronizeShadowFeedbackPipeline()
  {
    bool changed = false;
    if (m_ShadowFeedback.Synchronize(m_hShadowFeedbackPipeline, &changed).Failed())
    {
      xiiLog::Error("Selected shadow-feedback resource '{}' is missing or invalid.", m_hShadowFeedbackPipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (changed)
    {
      const auto* pipeline       = m_ShadowFeedback.GetPipeline();
      m_uiShadowFeedbackDepth    = pipeline->FindInterface("SceneDepth", true);
      m_uiShadowFeedbackCascades = pipeline->FindInterface("CascadeConstants", true);
      if (m_uiShadowFeedbackDepth == xiiInvalidIndex || m_uiShadowFeedbackCascades == xiiInvalidIndex || pipeline->GetInterface()[m_uiShadowFeedbackDepth].m_Pin.m_Type != xiiRenderPipelinePin::Texture || pipeline->GetInterface()[m_uiShadowFeedbackDepth].m_Pin.m_Format != xiiGALResourceFormat::D32Float || pipeline->GetInterface()[m_uiShadowFeedbackCascades].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
      {
        xiiLog::Error("Shadow-feedback resource '{}' requires D32 SceneDepth and buffer CascadeConstants inputs.", m_hShadowFeedbackPipeline.GetResourceID());
        m_ShadowFeedback.Clear();
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult BuildShadowFeedback(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiRenderGraphTextureHandle depth, xiiRenderGraphBufferHandle cascades)
  {
    XII_SUCCEED_OR_RETURN(SynchronizeShadowFeedbackPipeline());
    const auto&          settings = xiiVirtualShadowMapManager::GetConfiguration();
    xiiVariantDictionary parameters;
    parameters.Insert("ShadowFeedbackInverseViewProjection", m_ViewProjection.GetInverse());
    parameters.Insert("ShadowFeedbackNearPlane", m_Camera.GetNearPlane());
    parameters.Insert("ShadowFeedbackVirtualResolution", settings.m_uiVirtualResolution);
    parameters.Insert("ShadowFeedbackPageSize", settings.m_uiPageSize);
    parameters.Insert("ShadowFeedbackLightId", g_uiDirectionalLightId);
    const xiiRenderPipelineInputBinding inputs[] = {
      xiiRenderPipelineInputBinding(graph, m_uiShadowFeedbackDepth, depth),
      xiiRenderPipelineInputBinding(graph, m_uiShadowFeedbackCascades, cascades)};
    if (m_ShadowFeedback.Build(nullptr, graph, blackboard, parameters, xiiMakeArrayPtr(inputs)).Failed())
    {
      xiiLog::Error("Authored shadow-feedback build failed: {}", m_ShadowFeedback.GetLastBuildError());
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  xiiResult SynchronizeDepthPipeline()
  {
    bool changed = false;
    if (m_DepthPipeline.Synchronize(m_hDepthPipeline, &changed).Failed())
    {
      xiiLog::Error("Selected depth/history resource '{}' is missing or invalid.", m_hDepthPipeline.GetResourceID());
      return XII_FAILURE;
    }
    if (changed)
    {
      const auto* pipeline = m_DepthPipeline.GetPipeline();
      if (!pipeline)
        return XII_FAILURE;
      m_uiDepthInterface         = pipeline->FindInterface("SceneDepth", true);
      m_uiPreviousDepthInterface = pipeline->FindInterface("PreviousDepthPyramid", false);
      if (m_uiDepthInterface == xiiInvalidIndex || m_uiPreviousDepthInterface == xiiInvalidIndex || !pipeline->GetInterface()[m_uiPreviousDepthInterface].m_Pin.m_bTemporalHistoryOutput)
      {
        xiiLog::Error("Selected depth/history resource '{}' must expose SceneDepth and an explicit PreviousDepthPyramid output.", m_hDepthPipeline.GetResourceID());
        m_DepthPipeline.Clear();
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult SynchronizeViewBoundaryPipelines()
  {
    const auto synchronize = [&](xiiRenderPipelineInstance& instance, const xiiRenderPipelineResourceHandle& resource) -> xiiResult {
      if (instance.Synchronize(resource).Succeeded())
        return XII_SUCCESS;
      xiiLog::Error("View boundary resource '{}' failed: {}", resource.GetResourceID(), instance.GetLastBuildError());
      return XII_FAILURE;
    };
    XII_SUCCEED_OR_RETURN(synchronize(m_Targets, m_hTargetsPipeline));
    XII_SUCCEED_OR_RETURN(synchronize(m_Presentation, m_hPresentationPipeline));
    XII_SUCCEED_OR_RETURN(synchronize(m_SensorExport, m_hSensorExportPipeline));
    const auto check = [&](const xiiRenderPipelineInstance& instance, xiiStringView name, bool input, xiiRenderPipelinePin::Type kind, xiiStringView resource) -> xiiResult {
      const auto* pipeline = instance.GetPipeline();
      const auto  index    = pipeline->FindInterface(name, input);
      if (index != xiiInvalidIndex && pipeline->GetInterface()[index].m_Pin.m_Type == kind)
        return XII_SUCCESS;
      xiiLog::Error("View boundary resource '{}' requires {} interface '{}', resource kind {}.", resource, input ? "input" : "output", name, xiiUInt32(kind));
      return XII_FAILURE;
    };
    XII_SUCCEED_OR_RETURN(check(m_Targets, "Color", false, xiiRenderPipelinePin::Texture, m_hTargetsPipeline.GetResourceID()));
    XII_SUCCEED_OR_RETURN(check(m_Targets, "Depth", false, xiiRenderPipelinePin::Texture, m_hTargetsPipeline.GetResourceID()));
    XII_SUCCEED_OR_RETURN(check(m_Presentation, "Color", true, xiiRenderPipelinePin::Texture, m_hPresentationPipeline.GetResourceID()));
    XII_SUCCEED_OR_RETURN(check(m_Presentation, "ViewOutput", false, xiiRenderPipelinePin::Texture, m_hPresentationPipeline.GetResourceID()));
    return check(m_SensorExport, "Input", true, xiiRenderPipelinePin::Buffer, m_hSensorExportPipeline.GetResourceID());
  }

  xiiResult BuildViewTargets(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiRenderGraphTextureHandle& color, xiiRenderGraphTextureHandle& depth)
  {
    XII_SUCCEED_OR_RETURN(SynchronizeViewBoundaryPipelines());
    auto* view = m_World.GetRenderView();
    if (!view)
      return XII_FAILURE;
    view->SetViewport({0, 0, float(m_TargetSize.width), float(m_TargetSize.height)});
    view->SetSwapChain(m_pSwapChain.Borrow());
    if (m_Targets.Build(view, graph, blackboard).Failed())
    {
      xiiLog::Error("Target resource '{}' failed: {}", m_hTargetsPipeline.GetResourceID(), m_Targets.GetLastBuildError());
      return XII_FAILURE;
    }
    color                        = m_Targets.GetInterfaceBinding(m_Targets.GetPipeline()->FindInterface("Color", false))->m_hTexture;
    depth                        = m_Targets.GetInterfaceBinding(m_Targets.GetPipeline()->FindInterface("Depth", false))->m_hTexture;
    const auto* colorDescription = graph.GetTextureDescription(color);
    const auto* depthDescription = graph.GetTextureDescription(depth);
    if (!colorDescription || !depthDescription || colorDescription->m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB || depthDescription->m_Format != xiiGALResourceFormat::D32Float || colorDescription->m_Size != m_TargetSize || depthDescription->m_Size != m_TargetSize || colorDescription->m_uiSampleCount != 1 || depthDescription->m_uiSampleCount != 1 || !colorDescription->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || !depthDescription->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
    {
      xiiLog::Error("Target resource '{}' does not provide this sample's matching RGBA8 sRGB / D32 attachments.", m_hTargetsPipeline.GetResourceID());
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  xiiResult BuildViewOutput(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiRenderGraphTextureHandle color, xiiRenderGraphBufferHandle sensorCount)
  {
    const xiiRenderPipelineInputBinding sensorInput(graph, m_SensorExport.GetPipeline()->FindInterface("Input", true), sensorCount);
    if (m_SensorExport.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&sensorInput, 1)).Failed())
    {
      xiiLog::Error("Sensor export resource '{}' failed: {}", m_hSensorExportPipeline.GetResourceID(), m_SensorExport.GetLastBuildError());
      return XII_FAILURE;
    }
    const xiiRenderPipelineInputBinding input(graph, m_Presentation.GetPipeline()->FindInterface("Color", true), color);
    if (m_Presentation.Build(m_World.GetRenderView(), graph, blackboard, {}, xiiMakeArrayPtr(&input, 1)).Failed())
    {
      xiiLog::Error("Presentation resource '{}' failed: {}", m_hPresentationPipeline.GetResourceID(), m_Presentation.GetLastBuildError());
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  xiiResult BuildRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiView*)
  {
    m_uiVirtualPagesThisFrame                   = 0;
    const auto                       geometry   = xiiGeometryResidencyManager::AddUploadPass(graph, m_uiFrameIndex);
    const auto                       materials  = xiiMaterialManager::AddUploadPasses(graph);
    const xiiRenderGraphBufferHandle hMaterials = materials.m_hSurfaceData;
    if (m_hRayProbePipeline.IsValid())
    {
      auto* view = m_World.GetRenderView();
      XII_LOCK(view->GetWorld()->GetReadMarker());
      if (SynchronizeRayProbePipeline().Failed() || m_RayProbe.Build(view, graph, blackboard).Failed())
      {
        xiiLog::Error("Selected ray-probe resource '{}' failed: {}", m_hRayProbePipeline.GetResourceID(), m_RayProbe.GetLastBuildError());
        SetReturnCode(1);
        RequestQuit();
        return XII_FAILURE;
      }
      if (m_uiFrameIndex == 1)
      {
        for (xiiUInt32 i = 0; i < m_RayProbe.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
          if (const auto* source = xiiDynamicCast<const xiiRayTracingScenePass*>(m_RayProbe.GetPass(i)))
            xiiLog::Info("World-derived ray tracing contains {} TLAS instances.", source->GetLastBuildStatistics().m_uiInstanceCount);
      }
    }
    else if (m_uiFrameIndex == 1)
      xiiLog::Info("Hardware ray tracing is unavailable on this configured device; raster validation remains active.");
    if (SynchronizeDepthPipeline().Failed() || m_DepthPipeline.BeginFrame(nullptr, graph, blackboard).Failed())
    {
      xiiLog::Error("Could not prepare the selected depth/history resource: {}", m_DepthPipeline.GetLastBuildError());
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    xiiVirtualShadowTableBindings                         virtualShadowUpload;
    xiiSharedPtr<const xiiVirtualShadowResidencySnapshot> shadowResidency;
    if (BuildShadowTables(graph, blackboard, virtualShadowUpload, shadowResidency).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    if (SynchronizeVisibilityPipelines().Failed() || m_World.GetGpuInstances().GetMaximumMeshletCount() > m_uiVisibilityCapacity)
    {
      xiiLog::Error("The selected visibility resource is invalid or its meshlet budget cannot cover this world's conservative bound.");
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    xiiGpuVisibilityOutputs visibility, sensorVisibility;
    if (BuildVisibility(m_MainVisibility, graph, blackboard, m_ViewProjection, m_TargetSize.width, m_TargetSize.height, xiiGpuInstanceFlags::Enabled, 3, visibility).Failed() || BuildVisibility(m_SensorVisibility, graph, blackboard, m_ViewProjection, m_TargetSize.width, m_TargetSize.height, (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::SensorVisible).GetValue(), 3, sensorVisibility).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    xiiUInt32                                    uiShadowResolution = 0;
    xiiSharedPtr<const xiiShadowCascadeSnapshot> shadowSnapshot;
    xiiRenderGraphBufferHandle                   hCascadeConstants;
    if (BuildCascades(graph, blackboard, shadowSnapshot, hCascadeConstants, uiShadowResolution).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    const bool    bHasShadowCascade    = !shadowSnapshot->m_Cascades.IsEmpty();
    const xiiMat4 shadowViewProjection = shadowSnapshot->m_Cascades[0].m_mViewProjection;

    xiiGpuVisibilityOutputs shadowVisibility;
    if (BuildVisibility(m_ShadowVisibility, graph, blackboard, shadowViewProjection, uiShadowResolution, uiShadowResolution, (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::CastShadows).GetValue(), 1, shadowVisibility).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    if (bHasShadowCascade)
    {
      const xiiVirtualShadowMapSettings& settings = xiiVirtualShadowMapManager::GetConfiguration();
      xiiRectU32                         animatedPageRegion;
      if (BuildVirtualShadowPageRegion(shadowSnapshot->m_Cascades[0].m_mViewProjection, m_World.GetAnimatedShadowBounds(), settings.m_uiVirtualResolution, settings.m_uiPageSize, 0U, animatedPageRegion))
      {
        XII_IGNORE_UNUSED(xiiVirtualShadowMapManager::InvalidateRegion(g_uiDirectionalLightId, 0U, animatedPageRegion));
      }

      if (BuildVirtualShadowPages(graph, blackboard, virtualShadowUpload, shadowSnapshot, shadowResidency, shadowVisibility, geometry, materials).Failed())
      {
        SetReturnCode(1);
        RequestQuit();
        return XII_FAILURE;
      }
    }

    xiiRenderGraphTextureHandle hShadowMap;
    if (SynchronizeShadowPipeline().Failed() || BuildShadowResource(m_SunShadow, graph, blackboard, shadowViewProjection, {uiShadowResolution, uiShadowResolution}, xiiVec4U32::MakeZero(), shadowVisibility, geometry, materials, hShadowMap).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    xiiRenderGraphTextureHandle sceneColor, sceneDepth;
    if (BuildViewTargets(graph, blackboard, sceneColor, sceneDepth).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    if (SynchronizeDirectionalPipeline().Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    xiiVariantDictionary directionalParameters;
    directionalParameters.Insert("RasterViewProjection", m_ViewProjection);
    directionalParameters.Insert("RasterPreviousViewProjection", m_ViewProjection);
    directionalParameters.Insert("RasterWidth", m_TargetSize.width);
    directionalParameters.Insert("RasterHeight", m_TargetSize.height);
    const auto& sun = m_World.GetSunLight();
    directionalParameters.Insert("DirectionalSun", xiiVec4(sun.m_vDirection.x, sun.m_vDirection.y, sun.m_vDirection.z, sun.m_fIntensity));
    directionalParameters.Insert("RasterMeshGroupCountX", m_uiMeshGroupX);
    directionalParameters.Insert("RasterMeshGroupCountY", m_uiMeshGroupY);
    directionalParameters.Insert("ShadowSamplingLightId", g_uiDirectionalLightId);
    directionalParameters.Insert("ShadowSamplingEnabled", bHasShadowCascade);
    const auto*                                       directional = m_Directional.GetPipeline();
    xiiHybridArray<xiiRenderPipelineInputBinding, 16> directionalInputs;
    const auto                                        bindBuffer = [&](xiiStringView name, xiiRenderGraphBufferHandle handle) {
      directionalInputs.PushBack(xiiRenderPipelineInputBinding(graph, directional->FindInterface(name, true), handle));
    };
    const auto bindTexture = [&](xiiStringView name, xiiRenderGraphTextureHandle handle) {
      directionalInputs.PushBack(xiiRenderPipelineInputBinding(graph, directional->FindInterface(name, true), handle));
    };
    bindBuffer("SceneInstances", visibility.m_hSceneInstances);
    bindBuffer("SkinningMatrices", visibility.m_hSkinningMatrices);
    bindBuffer("GeometryMetadata", geometry.m_hGeometryMetadata);
    bindBuffer("MeshletMetadata", geometry.m_hMeshletMetadata);
    bindBuffer("VisibleMeshlets", visibility.m_hVisibleMeshlets);
    bindBuffer("VisibleMeshletCount", visibility.m_hVisibleMeshletCount);
    bindBuffer("DrawCommands", visibility.m_hIndirectCommands);
    bindBuffer("DrawCommandCount", visibility.m_hIndirectCommandCount);
    bindBuffer("SurfaceMaterials", hMaterials);
    bindBuffer("VirtualPageTable", virtualShadowUpload.m_hVirtualPageTable);
    bindTexture("ColorTarget", sceneColor);
    bindTexture("DepthTarget", sceneDepth);
    bindTexture("ShadowMap", hShadowMap);
    bindTexture("VirtualShadowAtlas", virtualShadowUpload.m_hPhysicalAtlas);
    xiiRenderPipelineBinding cascadeBinding, residencyBinding;
    cascadeBinding.m_pFrameData   = shadowSnapshot;
    residencyBinding.m_pFrameData = shadowResidency;
    directionalInputs.PushBack({graph, directional->FindInterface("Cascades", true), cascadeBinding});
    directionalInputs.PushBack({graph, directional->FindInterface("Residency", true), residencyBinding});
    if (m_Directional.Build(nullptr, graph, blackboard, directionalParameters, directionalInputs).Failed())
    {
      xiiLog::Error("Selected directional mesh resource '{}' failed: {}", m_hDirectionalPipeline.GetResourceID(), m_Directional.GetLastBuildError());
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    const auto                          shadedColor = m_Directional.GetInterfaceBinding(directional->FindInterface("Color", false))->m_hTexture;
    const auto                          shadedDepth = m_Directional.GetInterfaceBinding(directional->FindInterface("Depth", false))->m_hTexture;
    const xiiRenderPipelineInputBinding depthInput(graph, m_uiDepthInterface, shadedDepth);
    if (m_DepthPipeline.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&depthInput, 1)).Failed())
    {
      xiiLog::Error("Authored depth/history pipeline failed: {}", m_DepthPipeline.GetLastBuildError());
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    if (BuildShadowFeedback(graph, blackboard, shadedDepth, hCascadeConstants).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }

    if (BuildViewOutput(graph, blackboard, shadedColor, sensorVisibility.m_hVisibleInstanceCount).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return XII_FAILURE;
    }
    return XII_SUCCESS;
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
      m_pSwapChain                    = m_pDevice->CreateSwapChain(description);
      m_pSwapChain->SetPresentMode(xiiGALPresentMode::VSync);
    }
    else if (m_pSwapChain->GetCurrentSize() != m_pWindow->GetClientAreaSize())
    {
      // Resize destroys the old swapchain. Waiting here guarantees presentation has released all
      // acquired images and satisfies VUID-vkDestroySwapchainKHR-swapchain-01282.
      m_pDevice->WaitIdle();
      m_pSwapChain->Resize(m_pWindow->GetClientAreaSize()).AssertSuccess();
      m_DepthPipeline.InvalidateTemporalHistory();
      m_MainVisibility.InvalidateTemporalHistory();
      m_SensorVisibility.InvalidateTemporalHistory();
      m_ShadowVisibility.InvalidateTemporalHistory();
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

  xiiSharedPtr<xiiGALDevice>                        m_pDevice;
  xiiSharedPtr<xiiGALSwapChain>                     m_pSwapChain;
  xiiUniquePtr<xiiWindow>                           m_pWindow;
  xiiRenderPipelineResourceHandle                   m_hDirectionalPipeline;
  xiiRenderPipelineInstance                         m_Directional;
  xiiRenderPipelineResourceHandle                   m_hRayProbePipeline;
  xiiRenderPipelineInstance                         m_RayProbe;
  xiiGpuDrivenSceneConfiguration                    m_Configuration;
  xiiGpuDrivenSceneWorld                            m_World;
  xiiRenderPipelineResourceHandle                   m_hTargetsPipeline, m_hPresentationPipeline, m_hSensorExportPipeline;
  xiiRenderPipelineInstance                         m_Targets, m_Presentation, m_SensorExport;
  xiiRenderPipelineResourceHandle                   m_hDepthPipeline;
  xiiRenderPipelineInstance                         m_DepthPipeline;
  xiiUInt32                                         m_uiDepthInterface = xiiInvalidIndex, m_uiPreviousDepthInterface = xiiInvalidIndex;
  xiiRenderPipelineResourceHandle                   m_hVisibilityPipeline;
  xiiRenderPipelineResourceHandle                   m_hShadowPagePipeline;
  xiiRenderPipelineResourceHandle                   m_hShadowWorkPipeline, m_hShadowPublishPipeline;
  xiiRenderPipelineInstance                         m_ShadowWork, m_ShadowPublish;
  xiiRenderPipelineResourceHandle                   m_hShadowTablesPipeline;
  xiiRenderPipelineInstance                         m_ShadowTables;
  xiiRenderPipelineResourceHandle                   m_hCascadePipeline;
  xiiRenderPipelineInstance                         m_Cascades;
  xiiUInt32                                         m_uiCascadeOutput = xiiInvalidIndex;
  xiiRenderPipelineResourceHandle                   m_hShadowFeedbackPipeline;
  xiiRenderPipelineInstance                         m_ShadowFeedback;
  xiiUInt32                                         m_uiShadowFeedbackDepth = xiiInvalidIndex, m_uiShadowFeedbackCascades = xiiInvalidIndex;
  xiiSharedPtr<const xiiRenderPipeline>             m_ShadowPageDefinition;
  xiiDynamicArray<xiiUniquePtr<VirtualPageRuntime>> m_VirtualPageRuntimes;
  xiiRenderPipelineInstance                         m_SunShadow;
  xiiRenderPipelineResourceHandle                   m_hShadowPipeline;
  xiiUInt32                                         m_uiShadowOutput = xiiInvalidIndex;
  xiiRenderPipelineInstance                         m_MainVisibility, m_SensorVisibility, m_ShadowVisibility;
  xiiUInt32                                         m_uiMeshGroupX = 0, m_uiMeshGroupY = 0, m_uiVisibilityCapacity = 0;
  xiiUInt32                                         m_uiVirtualPagesThisFrame = 0;
  bool                                              m_bReportedVirtualBatch   = false;
  xiiCamera                                         m_Camera;
  xiiRenderGraphHandle                              m_hRenderGraph;
  xiiSizeU32                                        m_TargetSize;
  xiiMat4                                           m_ViewProjection = xiiMat4::MakeIdentity();
  xiiUInt64                                         m_uiFrameIndex   = 0U;

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;
#endif
};

XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
