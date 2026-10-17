/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
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
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletCullPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Visibility/GpuVisibilityResources.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

#include <Shaders/GpuDrivenSceneConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

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

  struct GpuDrivenDrawPassData
  {
    xiiRenderGraphTextureHandle        m_hColor;
    xiiRenderGraphTextureHandle        m_hDepth;
    xiiRenderGraphTextureHandle        m_hShadowMap;
    xiiRenderGraphTextureHandle        m_hVirtualShadowAtlas;
    xiiRenderGraphBufferHandle         m_hVirtualShadowPageTable;
    xiiRenderGraphBufferHandle         m_hVirtualShadowConstants;
    xiiRenderGraphBufferHandle         m_hConstants;
    xiiRenderGraphBufferHandle         m_hSceneInstances;
    xiiRenderGraphBufferHandle         m_hSkinningMatrices;
    xiiRenderGraphBufferHandle         m_hGeometry;
    xiiRenderGraphBufferHandle         m_hMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle         m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle         m_hIndirectCommands;
    xiiRenderGraphBufferHandle         m_hIndirectCommandCount;
    xiiRenderGraphBufferHandle         m_hMaterials;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
    xiiSharedPtr<xiiGALRenderPass>     m_pRenderPass;
    xiiMat4                            m_ViewProjection                     = xiiMat4::MakeIdentity();
    xiiMat4                            m_ShadowViewProjection               = xiiMat4::MakeIdentity();
    float                              m_fShadowWorldUnitsPerTexel          = 0.0f;
    xiiUInt32                          m_uiGeometryBase                     = 0U;
    xiiUInt32                          m_uiMaterialFrameBase                = 0U;
    xiiUInt32                          m_uiMaterialStride                   = 0U;
    xiiUInt32                          m_uiVirtualShadowPageTableBaseIndex  = 0U;
    xiiUInt32                          m_uiVirtualShadowPageTableCapacity   = 0U;
    xiiUInt32                          m_uiVirtualShadowResolution          = 0U;
    xiiUInt32                          m_uiVirtualShadowPageSize            = 0U;
    xiiUInt32                          m_uiVirtualShadowPhysicalAtlasWidth  = 0U;
    xiiUInt32                          m_uiVirtualShadowPhysicalAtlasHeight = 0U;
    xiiUInt32                          m_uiVirtualShadowDirectionalLightId  = 0U;
    xiiUInt32                          m_uiVirtualShadowEnabled             = 0U;
  };

  struct RayTracingValidationPassData
  {
    xiiRenderGraphBufferHandle         m_hSceneDependency;
    xiiRenderGraphBufferHandle         m_hShaderBindingTable;
    xiiRenderGraphBufferHandle         m_hValidationResult;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
    xiiSharedPtr<xiiGALTopLevelAS>     m_pTopLevelAS;
    xiiUInt32                          m_uiShaderRecordStride = 0U;
  };
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

    if (!m_hShaderPermutation.IsValid())
    {
      xiiLog::Error("The GPU-driven draw shader could not be loaded.");
      SetReturnCode(1);
      return Execution::Quit;
    }
    // Keep the validated permutation acquired while recording this frame.
    xiiResourceLock<xiiShaderPermutationResource> drawShader(m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!drawShader.IsValid() || !drawShader->IsShaderValid())
    {
      xiiLog::Error("The GPU-driven draw shader failed to compile; stopping rendering.");
      SetReturnCode(1);
      return Execution::Quit;
    }

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
      if (xiiRenderGraphManager::ExecuteFrame(frame, nullptr, settings, &error).Succeeded())
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
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();
    xiiFrameAllocator::Swap();
    return Execution::Continue;
  }

  void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder projectDirectory = ">sdk/Data/Samples/GpuDrivenScene";
    xiiStringBuilder resolvedProjectDirectory;
    xiiFileSystem::ResolveSpecialDirectory(projectDirectory, resolvedProjectDirectory).AssertSuccess();
    xiiFileSystem::SetSpecialDirectory("project", resolvedProjectDirectory);
    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    xiiInputActionConfig closeAction   = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
    closeAction.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
    xiiInputManager::SetInputActionConfig("Main", "CloseApp", closeAction, true);

    xiiWindowCreationDescription windowDescription;
    windowDescription.m_Resolution       = xiiSizeU32(1440U, 810U);
    windowDescription.m_Title            = GetApplicationName();
    windowDescription.m_bShowMouseCursor = true;
    windowDescription.m_WindowMode       = xiiWindowMode::WindowResizable;
    windowDescription.m_iMonitor         = opt_GpuDrivenMonitor.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    windowDescription.AdjustWindowSizeAndPosition().IgnoreResult();
    m_pWindow = XII_DEFAULT_NEW(xiiWindow);
    m_pWindow->Initialize(windowDescription).AssertSuccess();
    m_pWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& event) {
      if (event.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
        RequestQuit();
      else if (event.m_Type == xiiWindowEvent::Type::SizeChanged)
        g_bWindowResized = true;
    });

    xiiGALDeviceCreationDescription deviceDescription;
    deviceDescription.m_DeviceFeatures.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_MeshShaders                   = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_BindlessResources             = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray    = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_TimestampQueries              = xiiGALDeviceFeatureState::Optional;
    deviceDescription.m_DeviceFeatures.m_DurationQueries               = xiiGALDeviceFeatureState::Optional;
    deviceDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Optional;
    deviceDescription.m_DeviceFeatures.m_RayTracing                    = opt_GpuDrivenRayTracing.GetOptionValue(xiiCommandLineOption::LogMode::Always) ? xiiGALDeviceFeatureState::Optional : xiiGALDeviceFeatureState::Disabled;
    // Timeline fences are required for GPU-side synchronization between the
    // graphics, asynchronous-compute, and transfer queues.
    deviceDescription.m_DeviceFeatures.m_NativeFence = xiiGALDeviceFeatureState::Optional;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    deviceDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#endif
    constexpr const char* szGraphicsApi = "Vulkan";
    xiiStringView         shaderModel;
    xiiStringView         shaderCompiler;
    xiiGALDeviceFactory::GetShaderModelAndCompiler(szGraphicsApi, shaderModel, shaderCompiler);
    xiiGALShaderManager::Configure(shaderModel, true);
    xiiPlugin::LoadPlugin(shaderCompiler).AssertSuccess();
    m_pDevice = xiiGALDeviceFactory::CreateDevice(szGraphicsApi, xiiFoundation::GetDefaultAllocator(), deviceDescription);
    XII_ASSERT_ALWAYS(m_pDevice != nullptr && m_pDevice->Initialize().Succeeded(), "A Vulkan 1.3 mesh-shader device is required.");
    XII_ASSERT_ALWAYS(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "The GPU-driven sample requires mesh shader support.");
    xiiGALDevice::SetDefaultDevice(m_pDevice);
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

    const xiiShaderResourceHandle shader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/GpuDrivenScene.xiiShader");
    m_hShaderPermutation                 = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
    CreateRayTracingValidationResources();
    CreateSceneRenderPass();

    xiiRenderGraphRegistrationDescription graphDescription;
    graphDescription.m_sName     = "GPU Driven Scene";
    graphDescription.m_Category  = xiiRenderGraphCategory::SceneRendering;
    graphDescription.m_Frequency = xiiRenderGraphFrequency::EveryFrame;
    graphDescription.m_iPriority = 0;
    m_hRenderGraph               = xiiRenderGraphManager::RegisterGraph(graphDescription, xiiMakeDelegate(&xiiGpuDrivenSceneApp::BuildRenderGraph, this));
  }

  void BeforeHighLevelSystemsShutdown() override
  {
    if (m_pDevice)
      m_pDevice->WaitIdle();
    xiiRenderGraphManager::UnregisterGraph(m_hRenderGraph);
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
    m_hShaderPermutation.Invalidate();
    m_hRayTracingValidationPermutation.Invalidate();
    m_pRayTracingValidationSBT.Clear();
    m_pSceneRenderPass.Clear();
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

  xiiResult BuildVisibility(xiiRenderPipelineInstance& instance, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard,
                            const xiiMat4& projection, xiiUInt32 width, xiiUInt32 height, xiiUInt32 flags, xiiUInt32 priority,
                            xiiGpuVisibilityOutputs& outputs)
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
      if (m_uiShadowOutput == xiiInvalidIndex || pipeline->GetInterface()[m_uiShadowOutput].m_Pin.m_Type != xiiRenderPipelinePin::Texture ||
          pipeline->GetInterface()[m_uiShadowOutput].m_Pin.m_Format != xiiGALResourceFormat::D32Float)
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
        if (index == xiiInvalidIndex || compiled->GetInterface()[index].m_Pin.m_Type != xiiRenderPipelinePin::Texture ||
            compiled->GetInterface()[index].m_Pin.m_Format != xiiGALResourceFormat::D32Float)
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

  xiiResult BuildVirtualShadowPages(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard,
                                    xiiVirtualShadowTableBindings& upload, const xiiSharedPtr<const xiiShadowCascadeSnapshot>& cascades, const xiiSharedPtr<const xiiVirtualShadowResidencySnapshot>& residency,
                                    const xiiGpuVisibilityOutputs& visibility, const xiiGeometryResidencyManager::UploadHandles& geometry,
                                    const xiiMaterialGpuStorage::UploadHandles& materials)
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
      if (BuildShadowResource(runtime->m_Pipeline, graph, blackboard, page.m_ViewProjection, {region.width, region.height},
                              xiiVec4U32(region.x, region.y, region.width, region.height), visibility, geometry, materials,
                              upload.m_hPhysicalAtlas, upload.m_hPhysicalAtlas)
            .Failed())
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
    if (!data(work, "Cascades", true, xiiGetStaticRTTI<xiiShadowCascadeSnapshot>()) || !data(work, "Residency", true, xiiGetStaticRTTI<xiiVirtualShadowResidencySnapshot>()) ||
        !data(work, "RasterPages", false, xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()) || !data(publish, "RasterPages", true, xiiGetStaticRTTI<xiiVirtualShadowRasterBatch>()))
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
    if (!residency || residency->GetSetupSerial() != graph.GetSetupSerial() || residency->m_Bindings.m_hPhysicalAtlas.m_Id != atlas.m_Id ||
        residency->m_Bindings.m_hPhysicalPageTable.m_Id != physical.m_Id || residency->m_Bindings.m_hVirtualPageTable.m_Id != table.m_Id)
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
      if (m_uiCascadeOutput == xiiInvalidIndex ||
          pipeline->GetInterface()[m_uiCascadeOutput].m_Pin.m_Type != xiiRenderPipelinePin::Buffer ||
          !pipeline->GetInterface()[m_uiCascadeOutput].m_Pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer))
      {
        xiiLog::Error("Cascade resource '{}' requires a native directional snapshot and uniform CascadeConstants output.", m_hCascadePipeline.GetResourceID());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  xiiResult BuildCascades(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard,
                          xiiSharedPtr<const xiiShadowCascadeSnapshot>& snapshot, xiiRenderGraphBufferHandle& constants, xiiUInt32& resolution)
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
      if (m_uiShadowFeedbackDepth == xiiInvalidIndex || m_uiShadowFeedbackCascades == xiiInvalidIndex ||
          pipeline->GetInterface()[m_uiShadowFeedbackDepth].m_Pin.m_Type != xiiRenderPipelinePin::Texture ||
          pipeline->GetInterface()[m_uiShadowFeedbackDepth].m_Pin.m_Format != xiiGALResourceFormat::D32Float ||
          pipeline->GetInterface()[m_uiShadowFeedbackCascades].m_Pin.m_Type != xiiRenderPipelinePin::Buffer)
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
      if (m_uiDepthInterface == xiiInvalidIndex || m_uiPreviousDepthInterface == xiiInvalidIndex ||
          !pipeline->GetInterface()[m_uiPreviousDepthInterface].m_Pin.m_bTemporalHistoryOutput)
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
    if (!colorDescription || !depthDescription || colorDescription->m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB || depthDescription->m_Format != xiiGALResourceFormat::D32Float ||
        colorDescription->m_Size != m_TargetSize || depthDescription->m_Size != m_TargetSize || colorDescription->m_uiSampleCount != 1 || depthDescription->m_uiSampleCount != 1 ||
        !colorDescription->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget) || !depthDescription->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
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

  void BuildRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
  {
    m_uiVirtualPagesThisFrame                        = 0;
    const auto                       geometry        = xiiGeometryResidencyManager::AddUploadPass(graph, m_uiFrameIndex);
    const auto                       materials       = xiiMaterialManager::AddUploadPasses(graph);
    const xiiRenderGraphBufferHandle hMaterials      = materials.m_hSurfaceData;
    const auto                       rayTracingScene = m_World.AddRayTracingBuildPass(graph, m_uiFrameIndex);
    if (m_uiFrameIndex == 1)
    {
      if (xiiRayTracingResources::IsHardwareRayTracingSupported())
        xiiLog::Info("World-derived ray tracing contains {} TLAS instances.", rayTracingScene.m_uiInstanceCount);
      else
        xiiLog::Info("Hardware ray tracing is unavailable on this configured device; raster validation remains active.");
    }
    if (SynchronizeDepthPipeline().Failed() || m_DepthPipeline.BeginFrame(nullptr, graph, blackboard).Failed())
    {
      xiiLog::Error("Could not prepare the selected depth/history resource: {}", m_DepthPipeline.GetLastBuildError());
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    xiiVirtualShadowTableBindings                         virtualShadowUpload;
    xiiSharedPtr<const xiiVirtualShadowResidencySnapshot> shadowResidency;
    if (BuildShadowTables(graph, blackboard, virtualShadowUpload, shadowResidency).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    if (rayTracingScene.m_pTopLevelAS != nullptr && m_pRayTracingValidationSBT != nullptr && m_hRayTracingValidationPermutation.IsValid())
    {
      auto validationPass = graph.AddPass<RayTracingValidationPassData>("Ray Tracing Dispatch Validation", xiiGALCommandQueueFlags::Compute, [this, rayTracingScene](RayTracingValidationPassData& data, xiiRenderGraphBuilder& builder) {
          data.m_pTopLevelAS          = rayTracingScene.m_pTopLevelAS;
          data.m_hShaderPermutation   = m_hRayTracingValidationPermutation;
          data.m_uiShaderRecordStride = m_uiRayTracingValidationShaderRecordStride;
          if (rayTracingScene.m_hSceneDependency.IsValid())
            data.m_hSceneDependency = builder.ReadBuffer(rayTracingScene.m_hSceneDependency, xiiGALResourceStateFlags::BuildASRead);

          data.m_hShaderBindingTable = builder.ImportBuffer("Ray Tracing Validation SBT", m_pRayTracingValidationSBT, m_pRayTracingValidationSBT->GetResourceState());
          data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);

          xiiGALBufferCreationDescription resultDescription;
          resultDescription.m_uiSize              = sizeof(xiiUInt32);
          resultDescription.m_uiElementByteStride = sizeof(xiiUInt32);
          resultDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
          resultDescription.m_Usage               = xiiGALResourceUsage::Default;
          resultDescription.m_Mode                = xiiGALBufferMode::Structured;
          data.m_hValidationResult                = builder.WriteBuffer("Ray Tracing Validation Result", resultDescription, xiiGALResourceStateFlags::UnorderedAccess);
          builder.SetPassSideEffects(true);
          builder.SetPassAllowMerge(false); }, [this](const RayTracingValidationPassData& data, xiiRenderGraphPassContext& context) { ExecuteRayTracingValidation(data, context); }, true);
      XII_IGNORE_UNUSED(validationPass);
    }

    if (SynchronizeVisibilityPipelines().Failed() || m_World.GetGpuInstances().GetMaximumMeshletCount() > m_uiVisibilityCapacity)
    {
      xiiLog::Error("The selected visibility resource is invalid or its meshlet budget cannot cover this world's conservative bound.");
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    xiiGpuVisibilityOutputs visibility, sensorVisibility;
    if (BuildVisibility(m_MainVisibility, graph, blackboard, m_ViewProjection, m_TargetSize.width, m_TargetSize.height, xiiGpuInstanceFlags::Enabled, 3, visibility).Failed() ||
        BuildVisibility(m_SensorVisibility, graph, blackboard, m_ViewProjection, m_TargetSize.width, m_TargetSize.height, (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::SensorVisible).GetValue(), 3, sensorVisibility).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    xiiUInt32                                    uiShadowResolution = 0;
    xiiSharedPtr<const xiiShadowCascadeSnapshot> shadowSnapshot;
    xiiRenderGraphBufferHandle                   hCascadeConstants;
    if (BuildCascades(graph, blackboard, shadowSnapshot, hCascadeConstants, uiShadowResolution).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    const bool    bHasShadowCascade    = !shadowSnapshot->m_Cascades.IsEmpty();
    const xiiMat4 shadowViewProjection = shadowSnapshot->m_Cascades[0].m_mViewProjection;

    xiiGpuVisibilityOutputs shadowVisibility;
    if (BuildVisibility(m_ShadowVisibility, graph, blackboard, shadowViewProjection, uiShadowResolution, uiShadowResolution,
                        (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::CastShadows).GetValue(), 1, shadowVisibility)
          .Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
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
        return;
      }
    }

    xiiRenderGraphTextureHandle hShadowMap;
    if (SynchronizeShadowPipeline().Failed() || BuildShadowResource(m_SunShadow, graph, blackboard, shadowViewProjection, {uiShadowResolution, uiShadowResolution}, xiiVec4U32::MakeZero(), shadowVisibility, geometry, materials, hShadowMap).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    xiiRenderGraphTextureHandle sceneColor, sceneDepth;
    if (BuildViewTargets(graph, blackboard, sceneColor, sceneDepth).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    auto drawPass                               = graph.AddPass<GpuDrivenDrawPassData>("GPU Driven Mesh Dispatch", xiiGALCommandQueueFlags::Graphics, [sceneColor, sceneDepth, geometry, visibility, hMaterials, hShadowMap, virtualShadowUpload](GpuDrivenDrawPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hColor                  = builder.ReadWriteTexture(sceneColor, xiiGALResourceStateFlags::RenderTarget);
        data.m_hDepth                  = builder.ReadWriteTexture(sceneDepth, xiiGALResourceStateFlags::DepthWrite);
        data.m_hShadowMap              = builder.ReadTexture(hShadowMap, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVirtualShadowAtlas     = builder.ReadTexture(virtualShadowUpload.m_hPhysicalAtlas, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVirtualShadowPageTable = builder.ReadBuffer(virtualShadowUpload.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
        data.m_hSceneInstances         = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_hSkinningMatrices       = builder.ReadBuffer(visibility.m_hSkinningMatrices, xiiGALResourceStateFlags::ShaderResource);
        data.m_hGeometry               = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_hMeshlets               = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVisibleMeshlets        = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVisibleMeshletCount    = builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
        data.m_hIndirectCommands       = builder.ReadBuffer(visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
        data.m_hIndirectCommandCount   = builder.ReadBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);
        data.m_hMaterials              = builder.ReadBuffer(hMaterials, xiiGALResourceStateFlags::ShaderResource);

        xiiGALBufferCreationDescription constantsDescription;
        constantsDescription.m_uiSize         = sizeof(xiiGpuDrivenSceneConstants);
        constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
        constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
        constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
        data.m_hConstants                     = builder.WriteBuffer("GPU Driven Scene Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
        constantsDescription.m_uiSize         = sizeof(xiiVirtualShadowSamplingConstants);
        data.m_hVirtualShadowConstants        = builder.WriteBuffer("GPU Driven Virtual Shadow Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer); }, [this](const GpuDrivenDrawPassData& data, xiiRenderGraphPassContext& context) { ExecuteGpuDrivenDraw(data, context); });
    drawPass.first->m_hShaderPermutation        = m_hShaderPermutation;
    drawPass.first->m_pRenderPass               = m_pSceneRenderPass;
    drawPass.first->m_ViewProjection            = m_ViewProjection;
    drawPass.first->m_ShadowViewProjection      = shadowViewProjection;
    drawPass.first->m_fShadowWorldUnitsPerTexel = bHasShadowCascade ? (2.0f * shadowSnapshot->m_Cascades[0].m_fWorldRadius) / static_cast<float>(uiShadowResolution) : 0.0f;
    drawPass.first->m_uiGeometryBase            = geometry.m_uiGeometryBaseIndex;
    drawPass.first->m_uiMaterialFrameBase       = materials.m_uiSurfaceBaseIndex;
    drawPass.first->m_uiMaterialStride          = xiiMaterialManager::GetGpuStorage().GetMaterialStride();

    const xiiVirtualShadowMapSettings& virtualShadowSettings = xiiVirtualShadowMapManager::GetConfiguration();
    const xiiVirtualShadowMapStats     virtualShadowStats    = xiiVirtualShadowMapManager::GetStats();
    drawPass.first->m_uiVirtualShadowPageTableBaseIndex      = virtualShadowUpload.m_uiVirtualTableBaseIndex;
    drawPass.first->m_uiVirtualShadowPageTableCapacity       = virtualShadowUpload.m_uiVirtualTableCapacity;
    drawPass.first->m_uiVirtualShadowResolution              = virtualShadowSettings.m_uiVirtualResolution;
    drawPass.first->m_uiVirtualShadowPageSize                = virtualShadowSettings.m_uiPageSize;
    drawPass.first->m_uiVirtualShadowPhysicalAtlasWidth      = virtualShadowStats.m_uiPhysicalAtlasWidth;
    drawPass.first->m_uiVirtualShadowPhysicalAtlasHeight     = virtualShadowStats.m_uiPhysicalAtlasHeight;
    drawPass.first->m_uiVirtualShadowDirectionalLightId      = g_uiDirectionalLightId;
    drawPass.first->m_uiVirtualShadowEnabled                 = bHasShadowCascade ? 1U : 0U;

    const xiiRenderPipelineInputBinding depthInput(graph, m_uiDepthInterface, drawPass.first->m_hDepth);
    if (m_DepthPipeline.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&depthInput, 1)).Failed())
    {
      xiiLog::Error("Authored depth/history pipeline failed: {}", m_DepthPipeline.GetLastBuildError());
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    if (BuildShadowFeedback(graph, blackboard, drawPass.first->m_hDepth, hCascadeConstants).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }

    if (BuildViewOutput(graph, blackboard, drawPass.first->m_hColor, sensorVisibility.m_hVisibleInstanceCount).Failed())
    {
      SetReturnCode(1);
      RequestQuit();
      return;
    }
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

  void CreateSceneRenderPass()
  {
    xiiGALRenderPassCreationDescription description;
    auto&                               color = description.m_Attachments.ExpandAndGetRef();
    color.m_Format                            = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    color.m_uiSampleCount                     = 1U;
    color.m_LoadOperation                     = xiiGALAttachmentLoadOperation::Load;
    color.m_StoreOperation                    = xiiGALAttachmentStoreOperation::Store;
    color.m_InitialStateFlags                 = xiiGALResourceStateFlags::RenderTarget;
    color.m_FinalStateFlags                   = xiiGALResourceStateFlags::RenderTarget;
    auto& depth                               = description.m_Attachments.ExpandAndGetRef();
    depth.m_Format                            = xiiGALResourceFormat::D32Float;
    depth.m_uiSampleCount                     = 1U;
    depth.m_LoadOperation                     = xiiGALAttachmentLoadOperation::Load;
    depth.m_StoreOperation                    = xiiGALAttachmentStoreOperation::Store;
    depth.m_StencilLoadOperation              = xiiGALAttachmentLoadOperation::Load;
    depth.m_StencilStoreOperation             = xiiGALAttachmentStoreOperation::Store;
    depth.m_InitialStateFlags                 = xiiGALResourceStateFlags::DepthWrite;
    depth.m_FinalStateFlags                   = xiiGALResourceStateFlags::DepthWrite;
    auto& subPass                             = description.m_SubPasses.ExpandAndGetRef();
    subPass.m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});
    subPass.m_DepthStencilAttachment.PushBack({1U, xiiGALResourceStateFlags::DepthWrite});
    m_pSceneRenderPass = xiiGALRenderPassCache::GetRenderPass(description);
  }

  void CreateRayTracingValidationResources()
  {
    XII_ASSERT_ALWAYS(ValidateComputePipeline("Shaders/Pipeline/TemporalDenoise.xiiShader"), "Failed to validate the production temporal denoising pipeline.");
    XII_ASSERT_ALWAYS(ValidateComputePipeline("Shaders/Pipeline/ReSTIRGITemporal.xiiShader"), "Failed to validate the production temporal ReSTIR GI pipeline.");
    XII_ASSERT_ALWAYS(ValidateComputePipeline("Shaders/Pipeline/ReSTIRGISpatial.xiiShader"), "Failed to validate the production spatial ReSTIR GI pipeline.");

    if (m_pDevice->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
      return;

    XII_ASSERT_ALWAYS(ValidateRayTracingPipeline("Shaders/Pipeline/RTShadow.xiiShader", sizeof(xiiUInt32)), "Failed to validate the production ray-traced shadow pipeline.");
    XII_ASSERT_ALWAYS(ValidateRayTracingPipeline("Shaders/Pipeline/RayTracedAmbientOcclusion.xiiShader", sizeof(xiiUInt32)), "Failed to validate the production ray-traced ambient-occlusion pipeline.");
    XII_ASSERT_ALWAYS(ValidateRayTracingPipeline("Shaders/Pipeline/RTGIFinalGather.xiiShader", 16U), "Failed to validate the production ray-traced GI pipeline.");
    XII_ASSERT_ALWAYS(ValidateRayTracingPipeline("Shaders/Pipeline/RTReflection.xiiShader", 16U), "Failed to validate the production ray-traced reflection pipeline.");

    const xiiShaderResourceHandle shader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/RayTracingValidation.xiiShader");
    m_hRayTracingValidationPermutation   = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, true);

    const xiiGALRayTracingProperties& properties      = m_pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
    const xiiUInt64                   uiBaseAlignment = xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment);
    const xiiUInt64                   uiStride        = xiiMemoryUtils::AlignSize(static_cast<xiiUInt64>(properties.m_uiShaderGroupHandleSize), uiBaseAlignment);
    XII_ASSERT_DEV(uiStride != 0U, "Ray tracing shader record stride must be non-zero.");

    xiiGALBufferCreationDescription description;
    description.m_uiSize       = uiStride * 3U;
    description.m_BindFlags    = xiiGALBindFlags::RayTracing;
    description.m_Usage        = xiiGALResourceUsage::Mutable;
    description.m_Mode         = xiiGALBufferMode::Raw;
    m_pRayTracingValidationSBT = m_pDevice->CreateBuffer(description);
    XII_ASSERT_DEV(m_pRayTracingValidationSBT != nullptr, "Failed to create ray tracing validation SBT.");
    m_pRayTracingValidationSBT->SetDebugName("Ray Tracing Validation SBT");
    m_uiRayTracingValidationShaderRecordStride = static_cast<xiiUInt32>(uiStride);
  }

  bool ValidateComputePipeline(xiiStringView sShaderPath)
  {
    const xiiShaderResourceHandle                 shader            = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
    const xiiShaderPermutationResourceHandle      permutationHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, true);
    xiiResourceLock<xiiShaderPermutationResource> permutation(permutationHandle, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid() || !permutation->IsShaderValid())
      return false;

    const xiiSharedPtr<xiiGALShader> computeShader = permutation->GetGALShader(xiiGALShaderType::Compute);
    if (computeShader == nullptr)
      return false;

    xiiGALComputePipelineStateCreationDescription description;
    description.m_pComputeShader             = computeShader;
    description.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    return xiiGALPipelineCache::GetPipeline(description) != nullptr;
  }

  bool ValidateRayTracingPipeline(xiiStringView sShaderPath, xiiUInt32 uiPayloadSize)
  {
    const xiiShaderResourceHandle                 shader            = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
    const xiiShaderPermutationResourceHandle      permutationHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, true);
    xiiResourceLock<xiiShaderPermutationResource> permutation(permutationHandle, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid() || !permutation->IsShaderValid())
      return false;

    const xiiSharedPtr<xiiGALShader> rayGeneration = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
    const xiiSharedPtr<xiiGALShader> miss          = permutation->GetGALShader(xiiGALShaderType::RayMiss);
    const xiiSharedPtr<xiiGALShader> closestHit    = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
    if (rayGeneration == nullptr || miss == nullptr || closestHit == nullptr)
      return false;

    xiiGALRayTracingPipelineStateCreationDescription description;
    description.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
    description.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
    description.m_uiMaximumPayloadSize                     = uiPayloadSize;
    description.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;
    auto& rayGenerationGroup                               = description.m_GeneralShaders.ExpandAndGetRef();
    rayGenerationGroup.m_sName.Assign("ProductionValidationRayGeneration");
    rayGenerationGroup.m_pShader = rayGeneration;
    auto& missGroup              = description.m_GeneralShaders.ExpandAndGetRef();
    missGroup.m_sName.Assign("ProductionValidationMiss");
    missGroup.m_pShader = miss;
    auto& hitGroup      = description.m_TriangleHitShaders.ExpandAndGetRef();
    hitGroup.m_sName.Assign("ProductionValidationTriangleHit");
    hitGroup.m_pClosestHitShader = closestHit;

    return xiiGALPipelineCache::GetPipeline(description) != nullptr;
  }

  void ExecuteRayTracingValidation(const RayTracingValidationPassData& data, xiiRenderGraphPassContext& context)
  {
    xiiResourceLock<xiiShaderPermutationResource> permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid() || !permutation->IsShaderValid())
    {
      xiiLog::Error("Ray tracing validation shader failed to load; skipping the validation dispatch.");
      return;
    }

    xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
    pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1U;
    pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiUInt32);
    pipelineDescription.m_uiMaximumAttributeSize                   = sizeof(float) * 2U;

    auto& rayGeneration = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
    rayGeneration.m_sName.Assign("ValidationRayGeneration");
    rayGeneration.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
    auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
    miss.m_sName.Assign("ValidationMiss");
    miss.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayMiss);
    auto& hit      = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
    hit.m_sName.Assign("ValidationTriangleHit");
    hit.m_pClosestHitShader = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);

    const xiiSharedPtr<xiiGALRayTracingPipelineState> pipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
    XII_ASSERT_DEV(pipeline != nullptr, "Failed to create ray tracing validation pipeline.");

    xiiGALCommandList& commandList = context.GetCommandList();
    commandList.SetPipelineState(pipeline.Borrow());
    commandList.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
    commandList.ResolveAndSetUnorderedAccessBufferView("g_ValidationResult", context.GetBuffer(data.m_hValidationResult)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
    commandList.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();

    const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
    xiiGALUpdateSBTDescription sbtUpdate;
    sbtUpdate.m_pPipelineState      = pipeline.Borrow();
    sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
    sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
    sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
    sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
    commandList.UpdateSBT(sbtUpdate);

    xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, 1U, 1U, 1U);
    trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
    trace.m_MissTable          = sbtUpdate.m_MissTable;
    trace.m_HitTable           = sbtUpdate.m_HitTable;
    commandList.TraceRays(trace);
  }

  void ExecuteGpuDrivenDraw(const GpuDrivenDrawPassData& data, xiiRenderGraphPassContext& context)
  {
    xiiResourceLock<xiiShaderPermutationResource>  permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    xiiGALGraphicsPipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_PipelineType                          = xiiGALPipelineType::Mesh;
    pipelineDescription.m_pPipelineResourceSignature            = permutation->GetPipelineResourceSignature();
    pipelineDescription.m_pMeshShader                           = permutation->GetGALShader(xiiGALShaderType::Mesh);
    pipelineDescription.m_pPixelShader                          = permutation->GetGALShader(xiiGALShaderType::Pixel);
    pipelineDescription.m_GraphicsPipeline.m_pBlendState        = permutation->GetBlendState();
    pipelineDescription.m_GraphicsPipeline.m_pRasterizerState   = permutation->GetRasterizerState();
    pipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = permutation->GetDepthStencilState();
    pipelineDescription.m_GraphicsPipeline.m_pRenderPass        = data.m_pRenderPass;
    pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;
    const xiiSharedPtr<xiiGALGraphicsPipelineState> pipeline    = xiiGALPipelineCache::GetPipeline(pipelineDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_pRenderPass       = data.m_pRenderPass;
    framebufferDescription.m_FramebufferSize   = m_pWindow->GetClientAreaSize();
    framebufferDescription.m_uiArraySliceCount = 1U;
    framebufferDescription.m_Attachments.PushBack(context.GetTexture(data.m_hColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    framebufferDescription.m_Attachments.PushBack(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
    const xiiSharedPtr<xiiGALFramebuffer> framebuffer = m_pDevice->CreateFramebuffer(framebufferDescription);

    xiiGALCommandList& commandList = context.GetCommandList();
    {
      xiiGALMapHelper<xiiGpuDrivenSceneConstants> constants(commandList, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->ViewProjectionMatrix       = data.m_ViewProjection;
      constants->ShadowViewProjectionMatrix = data.m_ShadowViewProjection;
      constants->GeometryBaseIndex          = data.m_uiGeometryBase;
      constants->MaterialFrameBase          = data.m_uiMaterialFrameBase;
      constants->MaterialStride             = data.m_uiMaterialStride;
      constants->VertexStride               = sizeof(xiiMeshPackedVertex);
      constants->MeshDispatchGroupCountX    = m_uiMeshGroupX;
      constants->MeshDispatchGroupCountY    = m_uiMeshGroupY;
      constants->Padding                    = xiiVec2U32::MakeZero();
      const xiiGpuDrivenSceneLight& sun     = m_World.GetSunLight();
      constants->SunDirectionIntensity      = xiiVec4(sun.m_vDirection.x, sun.m_vDirection.y, sun.m_vDirection.z, sun.m_fIntensity);
      constants->AmbientColor               = xiiVec4(0.12f, 0.15f, 0.22f, 1.0f);
      const auto& shadowSize                = context.GetTexture(data.m_hShadowMap)->GetDescription().m_Size;
      constants->ShadowTexelSize            = xiiVec4(1.0f / shadowSize.width, 1.0f / shadowSize.height, data.m_fShadowWorldUnitsPerTexel, 0.0f);
    }
    {
      xiiGALMapHelper<xiiVirtualShadowSamplingConstants> constants(commandList, context.GetBuffer(data.m_hVirtualShadowConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->VirtualShadowPageTableBaseIndex  = data.m_uiVirtualShadowPageTableBaseIndex;
      constants->VirtualShadowPageTableCapacity   = data.m_uiVirtualShadowPageTableCapacity;
      constants->VirtualShadowResolution          = data.m_uiVirtualShadowResolution;
      constants->VirtualShadowPageSize            = data.m_uiVirtualShadowPageSize;
      constants->VirtualShadowPhysicalAtlasWidth  = data.m_uiVirtualShadowPhysicalAtlasWidth;
      constants->VirtualShadowPhysicalAtlasHeight = data.m_uiVirtualShadowPhysicalAtlasHeight;
      constants->VirtualShadowDirectionalLightId  = data.m_uiVirtualShadowDirectionalLightId;
      constants->VirtualShadowEnabled             = data.m_uiVirtualShadowEnabled;
    }

    commandList.BeginRenderPass({data.m_pRenderPass.Borrow(), framebuffer.Borrow()});
    commandList.SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(framebufferDescription.m_FramebufferSize.width), static_cast<float>(framebufferDescription.m_FramebufferSize.height)));
    commandList.SetPipelineState(pipeline.Borrow());
    commandList.ResolveAndSetConstantBuffer("xiiGpuDrivenSceneConstants", context.GetBuffer(data.m_hConstants));
    commandList.ResolveAndSetConstantBuffer("xiiVirtualShadowSamplingConstants", context.GetBuffer(data.m_hVirtualShadowConstants), xiiGALShaderType::Pixel);
    commandList.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices", context.GetBuffer(data.m_hSkinningMatrices)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_MaterialData", context.GetBuffer(data.m_hMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
    commandList.ResolveAndSetShaderResourceTextureView("g_ShadowMap", context.GetTexture(data.m_hShadowMap)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    commandList.ResolveAndSetShaderResourceTextureView("g_VirtualShadowAtlas", context.GetTexture(data.m_hVirtualShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    commandList.ResolveAndSetShaderResourceBufferView("g_VirtualShadowPageTable", context.GetBuffer(data.m_hVirtualShadowPageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
    xiiGALBindlessResourceTable::BindBufferSRVs(commandList, "g_Buffers", xiiGALShaderType::Mesh);
    commandList.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
    commandList.DrawMeshIndirect({context.GetBuffer(data.m_hIndirectCommands), 1U, 0U, xiiGALStateTransitionMode::None, context.GetBuffer(data.m_hIndirectCommandCount)});
    commandList.EndRenderPass();
  }

  xiiSharedPtr<xiiGALDevice>                        m_pDevice;
  xiiSharedPtr<xiiGALSwapChain>                     m_pSwapChain;
  xiiUniquePtr<xiiWindow>                           m_pWindow;
  xiiSharedPtr<xiiGALRenderPass>                    m_pSceneRenderPass;
  xiiShaderPermutationResourceHandle                m_hShaderPermutation;
  xiiShaderPermutationResourceHandle                m_hRayTracingValidationPermutation;
  xiiSharedPtr<xiiGALBuffer>                        m_pRayTracingValidationSBT;
  xiiUInt32                                         m_uiRayTracingValidationShaderRecordStride = 0U;
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
  xiiRenderGraphGraphId                             m_hRenderGraph;
  xiiSizeU32                                        m_TargetSize;
  xiiMat4                                           m_ViewProjection = xiiMat4::MakeIdentity();
  xiiUInt64                                         m_uiFrameIndex   = 0U;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
