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

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <GraphicsCore/Lighting/GpuShadowRaster.h>
#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>

#include <Shaders/GpuDrivenSceneConstants.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/VirtualShadowMap/VirtualShadowMapConstants.h>

xiiCommandLineOptionInt opt_GpuDrivenMonitor("GpuDrivenScene", "-monitor", "Monitor used by the GPU-driven scene sample.", 0U);
xiiCommandLineOptionInt opt_GpuDrivenFrameCount("GpuDrivenScene", "-frames", "Quit normally after rendering this many frames (zero runs until closed).", 0U);
xiiCommandLineOptionInt opt_GpuDrivenGridWidth("GpuDrivenScene", "-grid-width", "Number of scene instances along the X axis.", 24, 1, 512);
xiiCommandLineOptionInt opt_GpuDrivenGridHeight("GpuDrivenScene", "-grid-height", "Number of scene instances along the Y axis.", 16, 1, 512);

namespace
{
  bool                g_bWindowResized       = false;
  constexpr xiiUInt32 g_uiDirectionalLightId = 1U;

  bool BuildVirtualShadowPageRegion(const xiiMat4& viewProjection, const xiiBoundingBox& worldBounds,
                                    xiiUInt32 uiVirtualResolution, xiiUInt32 uiPageSize, xiiUInt32 uiMipLevel,
                                    xiiRectU32& out_region)
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

  struct SceneTargetsPassData
  {
    xiiRenderGraphTextureHandle m_hColor;
    xiiRenderGraphTextureHandle m_hDepth;
  };

  struct ShadowTargetPassData
  {
    xiiRenderGraphTextureHandle m_hDepth;
  };

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

  struct VirtualShadowCascadePassData
  {
    xiiRenderGraphBufferHandle m_hConstants;
    xiiMat4                    m_CascadeViewProjection[4];
    xiiVec4                    m_vCascadeSplitDepths  = xiiVec4::MakeZero();
    xiiVec4                    m_vCascadeWorldRadii   = xiiVec4::MakeZero();
    xiiUInt32                  m_uiActiveCascadeCount = 0U;
  };

  struct PresentPassData
  {
    xiiRenderGraphTextureHandle m_hColor;
    xiiRenderGraphTextureHandle m_hBackBuffer;
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
      return Execution::Quit;

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

    m_pDevice->BeginFrame();
    const bool bCanRender = m_pSwapChain != nullptr && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && m_pSwapChain->GetBackBufferTexture() != nullptr;
    if (bCanRender)
    {
      ++m_uiFrameIndex;
      const xiiUInt64 uiCompletedFrame = xiiRenderGraphManager::PrepareFrame(m_uiFrameIndex, m_Configuration.m_uiFramesInFlight);
      m_World.Update(m_uiFrameIndex, xiiClock::GetGlobalClock()->GetTimeDiff());

      m_TargetSize       = m_pWindow->GetClientAreaSize();
      const float aspect = static_cast<float>(m_TargetSize.width) / static_cast<float>(m_TargetSize.height);
      xiiMat4     projection;
      m_Camera.GetProjectionMatrix(aspect, projection, xiiCameraEye::Left, xiiClipSpaceDepthRange::ZeroToOne);
      m_ViewProjection = projection * m_Camera.GetViewMatrix();
      m_ViewFrustum    = xiiFrustum::MakeFromMVP(m_ViewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);

      xiiStringBuilder              error;
      xiiRenderGraphCompileSettings settings;
      settings.m_bEnablePassCulling   = true;
      settings.m_bEnableCompileCache  = true;
      settings.m_bEnableAsyncQueues   = false;
      settings.m_bEnableSplitBarriers = false;
      settings.m_bEnableGPUProfiling  = true;
      if (xiiRenderGraphManager::ExecuteFrame(m_uiFrameIndex, uiCompletedFrame, nullptr, settings, &error).Succeeded())
      {
        if (m_uiFrameIndex == 1U)
        {
          const xiiRenderGraphStatistics& statistics = xiiRenderGraphManager::GetGraph(m_hRenderGraph)->GetStatistics();
          xiiLog::Info("GPU-driven render graph: {} registered, {} compiled, {} culled passes; {} queue submissions, {} barriers ({} split).",
                       statistics.m_uiRegisteredPassCount, statistics.m_uiCompiledPassCount, statistics.m_uiCulledPassCount,
                       statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount, statistics.m_uiSplitBarrierCount);
        }
      }
      else
        xiiLog::Error("GPU-driven render graph compile failed: {0}", error);
      m_pSwapChain->Present();
    }
    else if (m_pSwapChain)
    {
      m_pSwapChain->Present();
    }
    m_pDevice->EndFrame();
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();
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
    deviceDescription.m_DeviceFeatures.m_RayTracing                    = xiiGALDeviceFeatureState::Optional;
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
    xiiGpuVisibilityDescription visibilityDescription;
    visibilityDescription.m_uiMaxInstances       = m_Configuration.m_uiGridWidth * m_Configuration.m_uiGridHeight + 1U;
    visibilityDescription.m_uiMaxVisibleMeshlets = m_Configuration.m_uiMaxVisibleMeshlets;
    visibilityDescription.m_uiMaxDrawCommands    = 1U;
    visibilityDescription.m_uiFramesInFlight     = m_Configuration.m_uiFramesInFlight;
    visibilityDescription.m_uiMaxVisibilitySets  = 3U;
    m_hVisibility                                = xiiGpuVisibilityManager::CreateContext(visibilityDescription);
    XII_ASSERT_ALWAYS(m_hVisibility.IsValid(), "Failed to create the GPU visibility context.");
    xiiGpuHiZPyramidDescription hiZDescription;
    hiZDescription.m_uiFramesInFlight = visibilityDescription.m_uiFramesInFlight;
    m_HiZPyramid.Initialize(m_pDevice.Borrow(), hiZDescription).AssertSuccess();
    const xiiSizeU32 initialSize = m_pWindow->GetClientAreaSize();
    m_HiZPyramid.Resize(initialSize.width, initialSize.height).AssertSuccess();

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
    m_HiZPyramid.Shutdown();
    xiiGpuVisibilityManager::DestroyContext(m_hVisibility);
    m_hVisibility = {};
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
  void BuildRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
  {
    const auto                                geometry            = xiiGeometryResidencyManager::AddUploadPass(graph, m_uiFrameIndex);
    const xiiRenderGraphBufferHandle          hMaterials          = xiiMaterialManager::AddUploadPass(graph);
    const auto                                rayTracingScene     = xiiRayTracingSceneManager::AddBuildPass(graph, m_uiFrameIndex);
    const xiiRenderGraphTextureHandle         hPreviousHiZ        = m_HiZPyramid.ImportPrevious(graph, m_uiFrameIndex);
    xiiVirtualShadowMapManager::UploadHandles virtualShadowUpload = xiiVirtualShadowMapManager::AddUploadPass(graph, m_uiFrameIndex);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowPhysicalBaseIndex, virtualShadowUpload.m_uiFrameBaseIndex);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowPhysicalPageCount, virtualShadowUpload.m_uiPhysicalPageCount);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowTableBaseIndex, virtualShadowUpload.m_uiVirtualTableBaseIndex);
    blackboard.Set(xiiRGBlackboardKeys::k_VirtualShadowTableCapacity, virtualShadowUpload.m_uiVirtualTableCapacity);

    if (rayTracingScene.m_pTopLevelAS != nullptr && m_pRayTracingValidationSBT != nullptr && m_hRayTracingValidationPermutation.IsValid())
    {
      auto validationPass = graph.AddPass<RayTracingValidationPassData>(
        "Ray Tracing Dispatch Validation", xiiGALCommandQueueFlags::Compute,
        [this, rayTracingScene](RayTracingValidationPassData& data, xiiRenderGraphBuilder& builder) {
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
          builder.SetPassAllowMerge(false);
        },
        [this](const RayTracingValidationPassData& data, xiiRenderGraphPassContext& context) { ExecuteRayTracingValidation(data, context); },
        true);
      XII_IGNORE_UNUSED(validationPass);
    }

    xiiGpuVisibilityView visibilityView = xiiGpuVisibilitySystem::BuildView(
      m_ViewProjection, m_ViewFrustum, m_Camera.GetPosition(), m_TargetSize.width, m_TargetSize.height,
      hPreviousHiZ.IsValid() ? m_HiZPyramid.GetMipLevelCount() : 0U,
      m_World.GetScene().GetObjectCount());
    xiiGpuVisibilityPassDescription visibilityPass;
    visibilityPass.m_sName                   = "Main View";
    visibilityPass.m_Purpose                 = xiiGpuVisibilityPurpose::MainView;
    visibilityPass.m_bAsyncCompute           = m_Configuration.m_bAsyncCompute;
    const xiiGpuVisibilityOutputs visibility = xiiGpuVisibilityManager::AddPasses(
      m_hVisibility, graph, m_uiFrameIndex, m_World.GetSceneHandle(), visibilityView, geometry, visibilityPass, hPreviousHiZ);

    // A robotics/medical sensor view owns independent frame-sliced constants. Only its instance
    // count is exported; unused meshlet and command stages are culled by the render graph.
    xiiGpuVisibilityView sensorView = visibilityView;
    sensorView.m_uiRequiredFlags    = (xiiSceneObjectFlags::Enabled | xiiSceneObjectFlags::SensorVisible).GetValue();
    xiiGpuVisibilityPassDescription sensorVisibilityPass;
    sensorVisibilityPass.m_sName                   = "Sensor View";
    sensorVisibilityPass.m_Purpose                 = xiiGpuVisibilityPurpose::Sensor;
    sensorVisibilityPass.m_bAsyncCompute           = m_Configuration.m_bAsyncCompute;
    const xiiGpuVisibilityOutputs sensorVisibility = xiiGpuVisibilityManager::AddPasses(
      m_hVisibility, graph, m_uiFrameIndex, m_World.GetSceneHandle(), sensorView, geometry, sensorVisibilityPass, hPreviousHiZ);

    constexpr xiiUInt32      uiShadowResolution = 2048U;
    xiiShadowCascadeSettings shadowSettings;
    shadowSettings.m_uiCascadeCount         = 1U;
    shadowSettings.m_fMaximumShadowDistance = 120.0f;
    shadowSettings.m_fDepthPadding          = 30.0f;
    shadowSettings.m_uiShadowMapResolution  = uiShadowResolution;
    xiiStaticArray<xiiShadowCascadeDescription, 4U> shadowCascades;
    const xiiGpuDrivenSceneLight&                   sun                  = m_World.GetSunLight();
    const float                                     shadowAspect         = static_cast<float>(m_TargetSize.width) / static_cast<float>(xiiMath::Max(m_TargetSize.height, 1U));
    const bool                                      bHasShadowCascade    = xiiShadowCascadeUtils::Build(m_Camera, shadowAspect, sun.m_vDirection, shadowSettings, shadowCascades).Succeeded() && !shadowCascades.IsEmpty();
    const xiiMat4                                   shadowViewProjection = bHasShadowCascade ? shadowCascades[0].m_mViewProjection : m_ViewProjection;
    const xiiFrustum                                shadowFrustum        = xiiFrustum::MakeFromMVP(shadowViewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);

    xiiGpuVisibilityView shadowView = xiiGpuVisibilitySystem::BuildView(
      shadowViewProjection, shadowFrustum, m_Camera.GetPosition(), uiShadowResolution, uiShadowResolution, 0U,
      m_World.GetScene().GetObjectCount());
    shadowView.m_uiRequiredFlags = (xiiSceneObjectFlags::Enabled | xiiSceneObjectFlags::CastShadows).GetValue();
    xiiGpuVisibilityPassDescription shadowVisibilityPass;
    shadowVisibilityPass.m_sName                   = "Sun Shadow";
    shadowVisibilityPass.m_Purpose                 = xiiGpuVisibilityPurpose::Shadow;
    shadowVisibilityPass.m_fLodScreenScale         = 1.0f;
    shadowVisibilityPass.m_uiMaxVisibleMeshlets    = 0U;
    shadowVisibilityPass.m_bAsyncCompute           = false;
    const xiiGpuVisibilityOutputs shadowVisibility = xiiGpuVisibilityManager::AddPasses(
      m_hVisibility, graph, m_uiFrameIndex, m_World.GetSceneHandle(), shadowView, geometry, shadowVisibilityPass);

    auto virtualShadowCascadePass = graph.AddPass<VirtualShadowCascadePassData>(
      "GPU Scene Virtual Shadow Cascades", xiiGALCommandQueueFlags::Graphics,
      [](VirtualShadowCascadePassData& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize         = sizeof(xiiShadowCascadeConstants);
        description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
        description.m_Usage          = xiiGALResourceUsage::Dynamic;
        description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
        data.m_hConstants            = builder.WriteBuffer("GPU Scene Virtual Shadow Cascade Constants", description, xiiGALResourceStateFlags::ConstantBuffer);
        builder.SetPassAllowMerge(false);
      },
      [](const VirtualShadowCascadePassData& data, xiiRenderGraphPassContext& context) {
        xiiGALMapHelper<xiiShadowCascadeConstants> constants(context.GetCommandList(), context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        for (xiiUInt32 uiCascade = 0U; uiCascade < 4U; ++uiCascade)
          constants->CascadeViewProjection[uiCascade] = data.m_CascadeViewProjection[uiCascade];
        constants->CascadeSplitDepths = data.m_vCascadeSplitDepths;
        constants->CascadeWorldRadii  = data.m_vCascadeWorldRadii;
        constants->ActiveCascadeCount = data.m_uiActiveCascadeCount;
        constants->_Pad               = xiiVec3::MakeZero();
      });
    virtualShadowCascadePass.first->m_uiActiveCascadeCount = bHasShadowCascade ? 1U : 0U;
    for (xiiUInt32 uiCascade = 0U; uiCascade < 4U; ++uiCascade)
      virtualShadowCascadePass.first->m_CascadeViewProjection[uiCascade] = xiiMat4::MakeIdentity();
    if (bHasShadowCascade)
    {
      virtualShadowCascadePass.first->m_CascadeViewProjection[0] = shadowCascades[0].m_mViewProjection;
      virtualShadowCascadePass.first->m_vCascadeSplitDepths.x    = shadowCascades[0].m_fSplitFar;
      virtualShadowCascadePass.first->m_vCascadeWorldRadii.x     = shadowCascades[0].m_fWorldRadius;
    }

    if (bHasShadowCascade)
    {
      const xiiVirtualShadowMapSettings& settings = xiiVirtualShadowMapManager::GetConfiguration();
      xiiRectU32                         animatedPageRegion;
      if (BuildVirtualShadowPageRegion(shadowCascades[0].m_mViewProjection, m_World.GetAnimatedShadowBounds(),
                                       settings.m_uiVirtualResolution, settings.m_uiPageSize, 0U, animatedPageRegion))
      {
        XII_IGNORE_UNUSED(xiiVirtualShadowMapManager::InvalidateRegion(g_uiDirectionalLightId, 0U, animatedPageRegion));
      }

      const xiiMat4 cascadeViewProjection[] = {shadowCascades[0].m_mViewProjection};
      virtualShadowUpload                   = xiiVirtualShadowMapManager::AddRasterPasses(
        graph, virtualShadowUpload, shadowVisibility, geometry, cascadeViewProjection, g_uiDirectionalLightId,
        sizeof(xiiMeshPackedVertex), xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(m_hVisibility),
        xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(m_hVisibility));
    }

    auto shadowTarget = graph.AddPass<ShadowTargetPassData>(
      "Create GPU Shadow Target", xiiGALCommandQueueFlags::Graphics,
      [](ShadowTargetPassData& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription description;
        description.m_Type        = xiiGALResourceDimension::Texture2D;
        description.m_Size.width  = uiShadowResolution;
        description.m_Size.height = uiShadowResolution;
        description.m_Format      = xiiGALResourceFormat::D32Float;
        description.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        data.m_hDepth             = builder.WriteTexture("GPU Scene Sun Shadow", description, xiiGALResourceStateFlags::CopyDestination);
        builder.SetPassAllowMerge(false);
      },
      [](const ShadowTargetPassData& data, xiiRenderGraphPassContext& context) {
        context.GetCommandList().ClearDepthStencilView(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.0f, 0U);
      });

    xiiGpuShadowRasterDescription shadowRasterDescription;
    shadowRasterDescription.m_ViewProjectionMatrix      = shadowViewProjection;
    shadowRasterDescription.m_Viewport                  = xiiVec4U32(0U, 0U, uiShadowResolution, uiShadowResolution);
    shadowRasterDescription.m_uiVertexStride            = sizeof(xiiMeshPackedVertex);
    shadowRasterDescription.m_uiMeshDispatchGroupCountX = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(m_hVisibility);
    shadowRasterDescription.m_uiMeshDispatchGroupCountY = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(m_hVisibility);
    const xiiRenderGraphTextureHandle hShadowMap        = xiiGpuShadowRasterManager::AddPass(
      graph, "GPU Scene Sun Shadow Raster", shadowTarget.first->m_hDepth, shadowVisibility, geometry, shadowRasterDescription);

    graph.AddPass<SceneTargetsPassData>(
      "Create Scene Targets", xiiGALCommandQueueFlags::Graphics,
      [targetSize = m_TargetSize](SceneTargetsPassData& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription description;
        description.m_Type      = xiiGALResourceDimension::Texture2D;
        description.m_Size      = targetSize;
        description.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
        description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
        data.m_hColor           = builder.WriteTexture("GPU Scene Color", description, xiiGALResourceStateFlags::CopyDestination);
        description.m_Format    = xiiGALResourceFormat::D24UNormalizedS8UInt;
        description.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        data.m_hDepth           = builder.WriteTexture("GPU Scene Depth", description, xiiGALResourceStateFlags::CopyDestination);
      },
      [](const SceneTargetsPassData& data, xiiRenderGraphPassContext& context) {
        xiiGALCommandList& commandList = context.GetCommandList();
        commandList.ClearRenderTargetView(context.GetTexture(data.m_hColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.008f, 0.012f, 0.025f));
        commandList.ClearDepthStencilView(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 1.0f, 0U);
      });

    auto drawPass = graph.AddPass<GpuDrivenDrawPassData>(
      "GPU Driven Mesh Dispatch", xiiGALCommandQueueFlags::Graphics,
      [geometry, visibility, hMaterials, hShadowMap, virtualShadowUpload](GpuDrivenDrawPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hColor                  = builder.WriteTexture(builder.ReadTexture("GPU Scene Color", xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
        data.m_hDepth                  = builder.WriteTexture(builder.ReadTexture("GPU Scene Depth", xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
        data.m_hShadowMap              = builder.ReadTexture(hShadowMap, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVirtualShadowAtlas     = builder.ReadTexture(virtualShadowUpload.m_hPhysicalAtlas, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVirtualShadowPageTable = builder.ReadBuffer(virtualShadowUpload.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
        data.m_hSceneInstances         = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
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
        data.m_hVirtualShadowConstants        = builder.WriteBuffer("GPU Driven Virtual Shadow Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      },
      [this](const GpuDrivenDrawPassData& data, xiiRenderGraphPassContext& context) { ExecuteGpuDrivenDraw(data, context); });
    drawPass.first->m_hShaderPermutation        = m_hShaderPermutation;
    drawPass.first->m_pRenderPass               = m_pSceneRenderPass;
    drawPass.first->m_ViewProjection            = m_ViewProjection;
    drawPass.first->m_ShadowViewProjection      = shadowViewProjection;
    drawPass.first->m_fShadowWorldUnitsPerTexel = bHasShadowCascade ? (2.0f * shadowCascades[0].m_fWorldRadius) / static_cast<float>(uiShadowResolution) : 0.0f;
    drawPass.first->m_uiGeometryBase            = geometry.m_uiGeometryBaseIndex;
    drawPass.first->m_uiMaterialFrameBase       = m_World.GetMaterialFrameBase(m_uiFrameIndex);
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

    m_HiZPyramid.AddBuildPass(graph, m_uiFrameIndex, drawPass.first->m_hDepth, m_Configuration.m_bAsyncCompute);
    xiiVirtualShadowMapManager::AddFeedbackPasses(
      graph, drawPass.first->m_hDepth, virtualShadowCascadePass.first->m_hConstants, m_TargetSize.width, m_TargetSize.height,
      m_ViewProjection.GetInverse(), m_Camera.GetNearPlane(), g_uiDirectionalLightId, m_uiFrameIndex);

    graph.AddPass<PresentPassData>(
      "Present GPU Scene", xiiGALCommandQueueFlags::Graphics,
      [this, sensorVisibility](PresentPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hColor                                 = builder.ReadTexture("GPU Scene Color", xiiGALResourceStateFlags::CopySource);
        const xiiSharedPtr<xiiGALTexture> pBackBuffer = m_pSwapChain->GetBackBufferTexture();
        data.m_hBackBuffer                            = builder.WriteTexture(
          builder.ImportTexture("BackBuffer", pBackBuffer, pBackBuffer->GetResourceState()),
          xiiGALResourceStateFlags::CopyDestination);
        builder.ExportTexture(data.m_hBackBuffer, xiiGALResourceStateFlags::Present);
        builder.ExportBuffer(sensorVisibility.m_hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
        builder.SetPassSideEffects(true);
      },
      [](const PresentPassData& data, xiiRenderGraphPassContext& context) {
        context.GetCommandList().CopyTexture(context.GetTexture(data.m_hColor), context.GetTexture(data.m_hBackBuffer));
      },
      true);
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
      const xiiSizeU32 size = m_pWindow->GetClientAreaSize();
      if (m_HiZPyramid.GetMipLevelCount() != 0U && size.HasNonZeroArea())
        m_HiZPyramid.Resize(size.width, size.height).AssertSuccess();
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
    depth.m_Format                            = xiiGALResourceFormat::D24UNormalizedS8UInt;
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
    XII_ASSERT_ALWAYS(ValidateRayTracingPipeline("Shaders/Pipeline/RTAO.xiiShader", sizeof(xiiUInt32)), "Failed to validate the production ray-traced ambient-occlusion pipeline.");
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
      constants->MeshDispatchGroupCountX    = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(m_hVisibility);
      constants->MeshDispatchGroupCountY    = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(m_hVisibility);
      constants->Padding                    = xiiVec2U32::MakeZero();
      const xiiGpuDrivenSceneLight& sun     = m_World.GetSunLight();
      constants->SunDirectionIntensity      = xiiVec4(sun.m_vDirection.x, sun.m_vDirection.y, sun.m_vDirection.z, sun.m_fIntensity);
      constants->AmbientColor               = xiiVec4(0.12f, 0.15f, 0.22f, 1.0f);
      constexpr float fShadowResolution     = 2048.0f;
      constants->ShadowTexelSize            = xiiVec4(1.0f / fShadowResolution, 1.0f / fShadowResolution, data.m_fShadowWorldUnitsPerTexel, 0.0f);
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

  xiiSharedPtr<xiiGALDevice>         m_pDevice;
  xiiSharedPtr<xiiGALSwapChain>      m_pSwapChain;
  xiiUniquePtr<xiiWindow>            m_pWindow;
  xiiSharedPtr<xiiGALRenderPass>     m_pSceneRenderPass;
  xiiShaderPermutationResourceHandle m_hShaderPermutation;
  xiiShaderPermutationResourceHandle m_hRayTracingValidationPermutation;
  xiiSharedPtr<xiiGALBuffer>         m_pRayTracingValidationSBT;
  xiiUInt32                          m_uiRayTracingValidationShaderRecordStride = 0U;
  xiiGpuDrivenSceneConfiguration     m_Configuration;
  xiiGpuDrivenSceneWorld             m_World;
  xiiGpuHiZPyramid                   m_HiZPyramid;
  xiiGpuVisibilityContextHandle      m_hVisibility;
  xiiCamera                          m_Camera;
  xiiRenderGraphGraphId              m_hRenderGraph;
  xiiSizeU32                         m_TargetSize;
  xiiMat4                            m_ViewProjection = xiiMat4::MakeIdentity();
  xiiFrustum                         m_ViewFrustum;
  xiiUInt64                          m_uiFrameIndex = 0U;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
