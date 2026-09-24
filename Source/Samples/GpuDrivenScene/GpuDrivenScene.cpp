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
#include <Core/ResourceManager/Implementation/ResourceLock.h>
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

#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

#include <Shaders/GpuDrivenSceneConstants.h>

xiiCommandLineOptionInt opt_GpuDrivenMonitor("GpuDrivenScene", "-monitor", "Monitor used by the GPU-driven scene sample.", 0U);
xiiCommandLineOptionInt opt_GpuDrivenFrameCount("GpuDrivenScene", "-frames", "Quit normally after rendering this many frames (zero runs until closed).", 0U);

namespace
{
  bool g_bWindowResized = false;

  struct SceneTargetsPassData
  {
    xiiRenderGraphTextureHandle m_hColor;
    xiiRenderGraphTextureHandle m_hDepth;
  };

  struct GpuDrivenDrawPassData
  {
    xiiRenderGraphTextureHandle m_hColor;
    xiiRenderGraphTextureHandle m_hDepth;
    xiiRenderGraphBufferHandle  m_hConstants;
    xiiRenderGraphBufferHandle  m_hSceneInstances;
    xiiRenderGraphBufferHandle  m_hGeometry;
    xiiRenderGraphBufferHandle  m_hMeshlets;
    xiiRenderGraphBufferHandle  m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle  m_hIndirectCommands;
    xiiRenderGraphBufferHandle  m_hIndirectCommandCount;
    xiiRenderGraphBufferHandle  m_hMaterials;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
    xiiSharedPtr<xiiGALRenderPass>      m_pRenderPass;
    xiiMat4   m_ViewProjection = xiiMat4::MakeIdentity();
    xiiUInt32 m_uiGeometryBase = 0U;
    xiiUInt32 m_uiMaterialFrameBase = 0U;
    xiiUInt32 m_uiMaterialStride = 0U;
  };

  struct PresentPassData
  {
    xiiRenderGraphTextureHandle m_hColor;
    xiiRenderGraphTextureHandle m_hBackBuffer;
  };
}

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
      const xiiUInt64 uiCompletedFrame = m_uiFrameIndex > 3U ? m_uiFrameIndex - 3U : 0U;
      m_World.Update(m_uiFrameIndex, uiCompletedFrame, xiiClock::GetGlobalClock()->GetTimeDiff());

      const xiiSizeU32 targetSize = m_pWindow->GetClientAreaSize();
      const float aspect = static_cast<float>(targetSize.width) / static_cast<float>(targetSize.height);
      xiiMat4 projection;
      m_Camera.GetProjectionMatrix(aspect, projection, xiiCameraEye::Left, xiiClipSpaceDepthRange::ZeroToOne);
      const xiiMat4 viewProjection = projection * m_Camera.GetViewMatrix();
      const xiiFrustum frustum = xiiFrustum::MakeFromMVP(viewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);

      m_pRenderGraph->BeginSetup(m_uiFrameIndex);
      const auto geometry = m_World.GetGeometryResidency().AddUploadPass(*m_pRenderGraph, m_uiFrameIndex);
      const xiiRenderGraphBufferHandle hMaterials = m_World.GetMaterialSystem().AddUploadPass(*m_pRenderGraph);
      const xiiRenderGraphTextureHandle hPreviousHiZ = m_HiZPyramid.ImportPrevious(*m_pRenderGraph, m_uiFrameIndex);

      xiiGpuVisibilityView visibilityView = xiiGpuVisibilitySystem::BuildView(
        viewProjection, frustum, m_Camera.GetPosition(), targetSize.width, targetSize.height,
        hPreviousHiZ.IsValid() ? m_HiZPyramid.GetMipLevelCount() : 0U,
        m_World.GetScene().GetObjectCount());
      xiiGpuVisibilityPassDescription visibilityPass;
      visibilityPass.m_sName = "Main View";
      visibilityPass.m_Purpose = xiiGpuVisibilityPurpose::MainView;
      visibilityPass.m_bAsyncCompute = m_Configuration.m_bAsyncCompute;
      const xiiGpuVisibilityOutputs visibility = m_Visibility.AddPasses(
        *m_pRenderGraph, m_uiFrameIndex, m_World.GetScene(), visibilityView, geometry, visibilityPass, hPreviousHiZ);

      m_pRenderGraph->AddPass<SceneTargetsPassData>(
        "Create Scene Targets", xiiGALCommandQueueFlags::Graphics,
        [targetSize](SceneTargetsPassData& data, xiiRenderGraphBuilder& builder) {
          xiiGALTextureCreationDescription description;
          description.m_Type = xiiGALResourceDimension::Texture2D;
          description.m_Size = targetSize;
          description.m_Format = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
          description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
          data.m_hColor = builder.WriteTexture("GPU Scene Color", description, xiiGALResourceStateFlags::RenderTarget);
          description.m_Format = xiiGALResourceFormat::D24UNormalizedS8UInt;
          description.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
          data.m_hDepth = builder.WriteTexture("GPU Scene Depth", description, xiiGALResourceStateFlags::DepthWrite);
        },
        [](const SceneTargetsPassData& data, xiiRenderGraphPassContext& context) {
          xiiGALCommandList& commandList = context.GetCommandList();
          commandList.ClearRenderTargetView(context.GetTexture(data.m_hColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.008f, 0.012f, 0.025f));
          commandList.ClearDepthStencilView(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 1.0f, 0U);
        });

      auto drawPass = m_pRenderGraph->AddPass<GpuDrivenDrawPassData>(
        "GPU Driven Mesh Dispatch", xiiGALCommandQueueFlags::Graphics,
        [this, geometry, visibility, hMaterials](GpuDrivenDrawPassData& data, xiiRenderGraphBuilder& builder) {
          data.m_hColor = builder.WriteTexture(builder.ReadTexture("GPU Scene Color", xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
          data.m_hDepth = builder.WriteTexture(builder.ReadTexture("GPU Scene Depth", xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
          data.m_hSceneInstances = builder.ReadBuffer(visibility.m_hSceneInstances, xiiGALResourceStateFlags::ShaderResource);
          data.m_hGeometry = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
          data.m_hMeshlets = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
          data.m_hVisibleMeshlets = builder.ReadBuffer(visibility.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
          data.m_hIndirectCommands = builder.ReadBuffer(visibility.m_hIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);
          data.m_hIndirectCommandCount = builder.ReadBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::IndirectArgument);
          data.m_hMaterials = builder.ReadBuffer(hMaterials, xiiGALResourceStateFlags::ShaderResource);

          xiiGALBufferCreationDescription constantsDescription;
          constantsDescription.m_uiSize = sizeof(xiiGpuDrivenSceneConstants);
          constantsDescription.m_BindFlags = xiiGALBindFlags::UniformBuffer;
          constantsDescription.m_Usage = xiiGALResourceUsage::Dynamic;
          constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
          data.m_hConstants = builder.WriteBuffer("GPU Driven Scene Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
        },
        [this](const GpuDrivenDrawPassData& data, xiiRenderGraphPassContext& context) { ExecuteGpuDrivenDraw(data, context); });
      drawPass.first->m_hShaderPermutation = m_hShaderPermutation;
      drawPass.first->m_pRenderPass = m_pSceneRenderPass;
      drawPass.first->m_ViewProjection = viewProjection;
      drawPass.first->m_uiGeometryBase = geometry.m_uiGeometryBaseIndex;
      drawPass.first->m_uiMaterialFrameBase = m_World.GetMaterialFrameBase(m_uiFrameIndex);
      drawPass.first->m_uiMaterialStride = m_World.GetMaterialSystem().GetGpuStorage().GetMaterialStride();

      // The depth rendered this frame becomes conservative occlusion history for the next one.
      m_HiZPyramid.AddBuildPass(*m_pRenderGraph, m_uiFrameIndex, drawPass.first->m_hDepth, m_Configuration.m_bAsyncCompute);

      m_pRenderGraph->AddPass<PresentPassData>(
        "Present GPU Scene", xiiGALCommandQueueFlags::Graphics,
        [this](PresentPassData& data, xiiRenderGraphBuilder& builder) {
          data.m_hColor = builder.ReadTexture("GPU Scene Color", xiiGALResourceStateFlags::CopySource);
          xiiSharedPtr<xiiGALTexture> pBackBuffer = m_pSwapChain->GetBackBufferTexture();
          data.m_hBackBuffer = builder.WriteTexture(
            builder.ImportTexture("BackBuffer", pBackBuffer, pBackBuffer->GetResourceState()),
            xiiGALResourceStateFlags::CopyDestination);
          builder.ExportTexture(data.m_hBackBuffer, xiiGALResourceStateFlags::Present);
          builder.SetPassSideEffects(true);
        },
        [](const PresentPassData& data, xiiRenderGraphPassContext& context) {
          context.GetCommandList().CopyTexture(context.GetTexture(data.m_hColor), context.GetTexture(data.m_hBackBuffer));
        }, true);

      m_pRenderGraph->EndSetup();
      m_pRenderGraphResourceCache->BeginFrame(m_uiFrameIndex, uiCompletedFrame);
      xiiStringBuilder error;
      xiiRenderGraphCompileSettings settings;
      settings.m_bEnablePassCulling = true;
      settings.m_bEnableCompileCache = true;
      settings.m_bEnableAsyncQueues = true;
      settings.m_bEnableSplitBarriers = true;
      settings.m_bEnableGPUProfiling = true;
      if (m_pRenderGraph->Compile(settings, &error).Succeeded())
      {
        m_pRenderGraph->Execute(m_pDevice.Borrow(), nullptr, m_pRenderGraphBlackboard.Borrow(), m_pRenderGraphResourceCache.Borrow(), m_pRenderGraphProfiler.Borrow()).AssertSuccess();
      }
      else
        xiiLog::Error("GPU-driven render graph compile failed: {0}", error);
      m_pRenderGraphResourceCache->EndFrame();
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

    xiiInputActionConfig closeAction = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
    closeAction.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
    xiiInputManager::SetInputActionConfig("Main", "CloseApp", closeAction, true);

    xiiWindowCreationDescription windowDescription;
    windowDescription.m_Resolution = xiiSizeU32(1440U, 810U);
    windowDescription.m_Title = GetApplicationName();
    windowDescription.m_bShowMouseCursor = true;
    windowDescription.m_WindowMode = xiiWindowMode::WindowResizable;
    windowDescription.m_iMonitor = opt_GpuDrivenMonitor.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
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
    deviceDescription.m_DeviceFeatures.m_ComputeShaders = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_MeshShaders = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_BindlessResources = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray = xiiGALDeviceFeatureState::Enabled;
    deviceDescription.m_DeviceFeatures.m_TimestampQueries = xiiGALDeviceFeatureState::Optional;
    // Timeline fences are required for GPU-side synchronization between the
    // graphics, asynchronous-compute, and transfer queues.
    deviceDescription.m_DeviceFeatures.m_NativeFence = xiiGALDeviceFeatureState::Optional;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    deviceDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#endif
    constexpr const char* szGraphicsApi = "Vulkan";
    xiiStringView shaderModel;
    xiiStringView shaderCompiler;
    xiiGALDeviceFactory::GetShaderModelAndCompiler(szGraphicsApi, shaderModel, shaderCompiler);
    xiiGALShaderManager::Configure(shaderModel, true);
    xiiPlugin::LoadPlugin(shaderCompiler).AssertSuccess();
    m_pDevice = xiiGALDeviceFactory::CreateDevice(szGraphicsApi, xiiFoundation::GetDefaultAllocator(), deviceDescription);
    XII_ASSERT_ALWAYS(m_pDevice != nullptr && m_pDevice->Initialize().Succeeded(), "A Vulkan 1.3 mesh-shader device is required.");
    XII_ASSERT_ALWAYS(m_pDevice->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled, "The GPU-driven sample requires mesh shader support.");
    xiiGALDevice::SetDefaultDevice(m_pDevice);
    UpdateSwapChain();
    xiiStartup::StartupHighLevelSystems();

    m_pRenderGraph = XII_DEFAULT_NEW(xiiRenderGraph);
    m_pRenderGraphBlackboard = XII_DEFAULT_NEW(xiiRenderGraphBlackboard);
    m_pRenderGraphResourceCache = XII_DEFAULT_NEW(xiiRenderGraphResourceCache);
    m_pRenderGraphProfiler = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler);
    m_pRenderGraphResourceCache->Initialize(m_pDevice);
    m_pRenderGraphProfiler->Initialize(m_pDevice);

    m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 250.0f);
    m_Camera.LookAt(xiiVec3(-12.0f, -2.0f, 12.0f), xiiVec3(25.0f, 0.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f));

    m_Configuration = {};
    m_World.Initialize(m_pDevice.Borrow(), m_Configuration).AssertSuccess();
    xiiGpuVisibilityDescription visibilityDescription;
    visibilityDescription.m_uiMaxInstances = m_Configuration.m_uiGridWidth * m_Configuration.m_uiGridHeight;
    visibilityDescription.m_uiMaxVisibleMeshlets = m_Configuration.m_uiMaxVisibleMeshlets;
    visibilityDescription.m_uiMaxDrawCommands = 1U;
    m_Visibility.Initialize(m_pDevice.Borrow(), visibilityDescription).AssertSuccess();
    xiiGpuHiZPyramidDescription hiZDescription;
    hiZDescription.m_uiFramesInFlight = visibilityDescription.m_uiFramesInFlight;
    m_HiZPyramid.Initialize(m_pDevice.Borrow(), hiZDescription).AssertSuccess();
    const xiiSizeU32 initialSize = m_pWindow->GetClientAreaSize();
    m_HiZPyramid.Resize(initialSize.width, initialSize.height).AssertSuccess();

    const xiiShaderResourceHandle shader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/GpuDrivenScene.xiiShader");
    m_hShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
    CreateSceneRenderPass();
  }

  void BeforeHighLevelSystemsShutdown() override
  {
    if (m_pDevice)
      m_pDevice->WaitIdle();
    m_HiZPyramid.Shutdown();
    m_Visibility.Shutdown();
    m_World.Shutdown(m_uiFrameIndex);
    m_hShaderPermutation.Invalidate();
    m_pSceneRenderPass.Clear();
    m_pRenderGraphProfiler.Clear();
    m_pRenderGraphResourceCache.Clear();
    m_pRenderGraphBlackboard.Clear();
    m_pRenderGraph.Clear();
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
  void UpdateSwapChain()
  {
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription description;
      description.m_pWindow = m_pWindow.Borrow();
      description.m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      description.m_UsageFlags = xiiGALSwapChainUsageFlags::RenderTarget;
      description.m_uiBufferCount = 3U;
      m_pSwapChain = m_pDevice->CreateSwapChain(description);
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
    auto& color = description.m_Attachments.ExpandAndGetRef();
    color.m_Format = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    color.m_uiSampleCount = 1U;
    color.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
    color.m_StoreOperation = xiiGALAttachmentStoreOperation::Store;
    color.m_InitialStateFlags = xiiGALResourceStateFlags::RenderTarget;
    color.m_FinalStateFlags = xiiGALResourceStateFlags::RenderTarget;
    auto& depth = description.m_Attachments.ExpandAndGetRef();
    depth.m_Format = xiiGALResourceFormat::D24UNormalizedS8UInt;
    depth.m_uiSampleCount = 1U;
    depth.m_LoadOperation = xiiGALAttachmentLoadOperation::Load;
    depth.m_StoreOperation = xiiGALAttachmentStoreOperation::Store;
    depth.m_StencilLoadOperation = xiiGALAttachmentLoadOperation::Load;
    depth.m_StencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
    depth.m_InitialStateFlags = xiiGALResourceStateFlags::DepthWrite;
    depth.m_FinalStateFlags = xiiGALResourceStateFlags::DepthWrite;
    auto& subPass = description.m_SubPasses.ExpandAndGetRef();
    subPass.m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});
    subPass.m_DepthStencilAttachment.PushBack({1U, xiiGALResourceStateFlags::DepthWrite});
    m_pSceneRenderPass = xiiGALRenderPassCache::GetRenderPass(description);
  }

  void ExecuteGpuDrivenDraw(const GpuDrivenDrawPassData& data, xiiRenderGraphPassContext& context)
  {
    xiiResourceLock<xiiShaderPermutationResource> permutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    xiiGALGraphicsPipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_PipelineType = xiiGALPipelineType::Mesh;
    pipelineDescription.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    pipelineDescription.m_pMeshShader = permutation->GetGALShader(xiiGALShaderType::Mesh);
    pipelineDescription.m_pPixelShader = permutation->GetGALShader(xiiGALShaderType::Pixel);
    pipelineDescription.m_GraphicsPipeline.m_pBlendState = permutation->GetBlendState();
    pipelineDescription.m_GraphicsPipeline.m_pRasterizerState = permutation->GetRasterizerState();
    pipelineDescription.m_GraphicsPipeline.m_pDepthStencilState = permutation->GetDepthStencilState();
    pipelineDescription.m_GraphicsPipeline.m_pRenderPass = data.m_pRenderPass;
    pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
    const xiiSharedPtr<xiiGALGraphicsPipelineState> pipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);

    xiiGALFramebufferCreationDescription framebufferDescription;
    framebufferDescription.m_pRenderPass = data.m_pRenderPass;
    framebufferDescription.m_FramebufferSize = m_pWindow->GetClientAreaSize();
    framebufferDescription.m_uiArraySliceCount = 1U;
    framebufferDescription.m_Attachments.PushBack(context.GetTexture(data.m_hColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    framebufferDescription.m_Attachments.PushBack(context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
    const xiiSharedPtr<xiiGALFramebuffer> framebuffer = m_pDevice->CreateFramebuffer(framebufferDescription);

    xiiGALCommandList& commandList = context.GetCommandList();
    {
      xiiGALMapHelper<xiiGpuDrivenSceneConstants> constants(commandList, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      constants->ViewProjectionMatrix = data.m_ViewProjection;
      constants->GeometryBaseIndex = data.m_uiGeometryBase;
      constants->MaterialFrameBase = data.m_uiMaterialFrameBase;
      constants->MaterialStride = data.m_uiMaterialStride;
      constants->VertexStride = sizeof(xiiMeshPackedVertex);
      const xiiGpuDrivenSceneLight& sun = m_World.GetSunLight();
      constants->SunDirectionIntensity = xiiVec4(sun.m_vDirection.x, sun.m_vDirection.y, sun.m_vDirection.z, sun.m_fIntensity);
      constants->AmbientColor = xiiVec4(0.12f, 0.15f, 0.22f, 1.0f);
    }

    commandList.BeginRenderPass({data.m_pRenderPass.Borrow(), framebuffer.Borrow()});
    commandList.SetViewport(xiiRectFloat(0.0f, 0.0f, static_cast<float>(framebufferDescription.m_FramebufferSize.width), static_cast<float>(framebufferDescription.m_FramebufferSize.height)));
    commandList.SetPipelineState(pipeline.Borrow());
    commandList.ResolveAndSetConstantBuffer("xiiGpuDrivenSceneConstants", context.GetBuffer(data.m_hConstants));
    commandList.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hSceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Mesh);
    commandList.ResolveAndSetShaderResourceBufferView("g_MaterialData", context.GetBuffer(data.m_hMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Pixel);
    m_World.GetBindlessResources().BindBufferSRVs(commandList, "g_Buffers", xiiGALShaderType::Mesh);
    commandList.CommitShaderResources(xiiGALStateTransitionMode::Verify).AssertSuccess();
    commandList.DrawMeshIndirect({context.GetBuffer(data.m_hIndirectCommands), 1U, 0U, xiiGALStateTransitionMode::None, context.GetBuffer(data.m_hIndirectCommandCount)});
    commandList.EndRenderPass();
  }

  xiiSharedPtr<xiiGALDevice>    m_pDevice;
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;
  xiiUniquePtr<xiiWindow>       m_pWindow;
  xiiUniquePtr<xiiRenderGraph>                  m_pRenderGraph;
  xiiUniquePtr<xiiRenderGraphBlackboard>        m_pRenderGraphBlackboard;
  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pRenderGraphResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pRenderGraphProfiler;
  xiiSharedPtr<xiiGALRenderPass>                 m_pSceneRenderPass;
  xiiShaderPermutationResourceHandle            m_hShaderPermutation;
  xiiGpuDrivenSceneConfiguration                 m_Configuration;
  xiiGpuDrivenSceneWorld                         m_World;
  xiiGpuHiZPyramid                              m_HiZPyramid;
  xiiGpuVisibilitySystem                         m_Visibility;
  xiiCamera                                      m_Camera;
  xiiUInt64                                      m_uiFrameIndex = 0U;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
