/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <EditorEngineProcessFrameworkTest/GpuBufferReadback.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST(EditorViews, WorldRenderFrames)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiUniquePtr<xiiWorld> worlds[2];
  xiiUInt64              frames[2]     = {};
  xiiUInt32              executions[2] = {};
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiWorldDescription description("Concurrent rendered world");
    worlds[i] = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(worlds[i]->GetWriteMarker());
    worlds[i]->SetWorldSimulationEnabled(false);
    xiiView* view = nullptr;
    worlds[i]->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Concurrent view", view);
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {8, 8};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto texture       = environment.GetDevice()->CreateTexture(target);
    view->SetRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetViewport({0, 0, 8, 8});
    view->SetCamera(&camera);
    view->SetRenderGraphBuilder([&, i](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
      struct Data
      {
        XII_DECLARE_POD_TYPE();
      };
      graph.AddPass<Data>("Record world frame", xiiGALCommandQueueFlags::Graphics, [](Data&, xiiRenderGraphBuilder&) {}, [&, i](const Data&, xiiRenderGraphPassContext& context) {
          frames[i] = context.GetFrameIndex();
          ++executions[i]; }, true);
    });
  }
  xiiUInt64  registeredFrame = 0;
  const auto registered      = xiiRenderGraphManager::CreateGraph("Interleaved registered frame", [&](xiiRenderGraph& graph, xiiRenderGraphBlackboard&, const xiiView*) -> xiiResult {
    struct Stamp
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiUInt64                  m_uiFrame = 0;
    };
    graph.AddPass<Stamp>("GPU frame stamp", xiiGALCommandQueueFlags::Graphics, [&](Stamp& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = sizeof(xiiUInt64);
      buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
      buffer.m_Mode = xiiGALBufferMode::Structured;
      buffer.m_uiElementByteStride = sizeof(xiiUInt64);
      data.m_Buffer = builder.WriteBuffer("Frame stamp", buffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_Buffer, xiiGALResourceStateFlags::CopySource);
      data.m_uiFrame = graph.GetFrameIndex(); }, [&](const Stamp& data, xiiRenderGraphPassContext& context) {
      registeredFrame = context.GetFrameIndex();
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_Buffer), 0, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&data.m_uiFrame), sizeof(data.m_uiFrame))); });
    return XII_SUCCESS;
  });
  XII_SCOPE_EXIT(xiiRenderGraphManager::DestroyGraph(registered).AssertSuccess());
  xiiRenderGraphFrame inactive;
  XII_TEST_BOOL(inactive.Begin(0).Failed());
  XII_TEST_BOOL(xiiRenderGraphManager::ExecuteGraph(inactive, registered, nullptr).Failed());
  xiiUInt64 previousFrame = 0;
  for (xiiUInt32 iteration = 0; iteration < 8; ++iteration)
  {
    const auto group = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::EarlyThisFrame);
    for (auto& world : worlds)
      xiiTaskSystem::AddTaskToGroup(group, world->GetUpdateTask());
    xiiTaskSystem::StartTaskGroup(group);
    xiiTaskSystem::WaitForGroup(group);
    XII_TEST_BOOL(frames[0] > previousFrame && frames[1] > previousFrame);
    XII_TEST_BOOL(frames[0] != frames[1]);
    XII_TEST_INT(executions[0], iteration + 1);
    XII_TEST_INT(executions[1], iteration + 1);
    previousFrame = xiiMath::Max(frames[0], frames[1]);
    {
      xiiRenderGraphFrame frame;
      if (!XII_TEST_RESULT(frame.Begin(2)))
        return;
      XII_TEST_INT(frame.GetFrameIndex(), previousFrame + 1);
      XII_TEST_BOOL(frame.GetCompletedFrame() >= frame.GetFrameIndex() - 2);
      xiiRenderGraphFrame nested;
      XII_TEST_BOOL(nested.Begin().Failed());
      XII_TEST_BOOL(frame.Begin().Failed());
      XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(frame, registered, nullptr));
      XII_TEST_INT(registeredFrame, previousFrame + 1);
    }
    auto*     graph         = xiiRenderGraphManager::GetGraph(registered);
    xiiUInt64 uploadedFrame = 0;
    if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph->GetExportedBuffer(graph->FindBuffer("Frame stamp")).Borrow(), 0, uploadedFrame))
      XII_TEST_INT(uploadedFrame, registeredFrame);
    previousFrame = registeredFrame;
    // Empty and early-return scopes still capture the queue timeline, keeping ring waits valid.
    {
      xiiRenderGraphFrame empty;
      XII_TEST_RESULT(empty.Begin(1));
      XII_TEST_INT(empty.GetFrameIndex(), ++previousFrame);
      empty.End();
      empty.End();
      XII_TEST_BOOL(!empty.IsActive());
    }
  }
  // A later graph can fail compilation after an earlier graph submitted GPU work.
  // The automatic frame scope must still capture it and release the recording lock.
  const auto cyclic = xiiRenderGraphManager::CreateGraph("Cyclic registered frame", [](xiiRenderGraph& graph, xiiRenderGraphBlackboard&, const xiiView*) -> xiiResult {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
    };
    auto       source    = graph.AddPass<Data>("Original version", xiiGALCommandQueueFlags::Graphics, [](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription buffer;
      buffer.m_uiSize = 8;
      data.m_Buffer = builder.WriteBuffer("Cyclic buffer", buffer, xiiGALResourceStateFlags::CopyDestination); }, [](const Data&, xiiRenderGraphPassContext&) {});
    const auto original  = source.first->m_Buffer;
    auto       overwrite = graph.AddPass<Data>("Overwrite", xiiGALCommandQueueFlags::Graphics, [original](Data& data, xiiRenderGraphBuilder& builder) { data.m_Buffer = builder.WriteBuffer(original, xiiGALResourceStateFlags::CopyDestination); }, [](const Data&, xiiRenderGraphPassContext&) {});
    const auto newer     = overwrite.first->m_Buffer;
    graph.AddPass<Data>("Read both physical versions", xiiGALCommandQueueFlags::Graphics, [original, newer](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Buffer = builder.ReadBuffer(original, xiiGALResourceStateFlags::CopySource);
      data.m_Buffer = builder.ReadBuffer(newer, xiiGALResourceStateFlags::CopySource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  });
  XII_SCOPE_EXIT(if (xiiRenderGraphManager::GetGraph(cyclic) != nullptr) xiiRenderGraphManager::DestroyGraph(cyclic).AssertSuccess());
  xiiStringBuilder error;
  {
    xiiRenderGraphFrame failed;
    if (!XII_TEST_RESULT(failed.Begin(1)))
      return;
    XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(failed, registered, nullptr));
    XII_TEST_BOOL(xiiRenderGraphManager::ExecuteGraph(failed, cyclic, nullptr, {}, &error).Failed());
  }
  XII_TEST_BOOL(error.FindSubString("dependency cycle") != nullptr);
  XII_TEST_INT(registeredFrame, ++previousFrame);
  XII_TEST_RESULT(xiiRenderGraphManager::DestroyGraph(cyclic));
  {
    xiiRenderGraphFrame recovery;
    XII_TEST_RESULT(recovery.Begin(1));
    XII_TEST_INT(recovery.GetFrameIndex(), previousFrame + 1);
    XII_TEST_BOOL(recovery.GetCompletedFrame() >= previousFrame);
    XII_TEST_RESULT(xiiRenderGraphManager::ExecuteGraph(recovery, registered, nullptr));
    previousFrame = registeredFrame;
  }
  worlds[0].Clear();
  {
    XII_LOCK(worlds[1]->GetWriteMarker());
    worlds[1]->Update();
  }
  XII_TEST_BOOL(frames[1] > previousFrame);
}

XII_CREATE_SIMPLE_TEST(EditorViews, DisabledViewsDoNotRecordFrames)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Disabled query view");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*    renderer = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view     = nullptr;
  renderer->CreateView("Externally executed query", view);
  view->SetRenderEnabled(false);
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  // A disabled query view can be used during setup by another graph owner. World
  // updates must not start a second frame or initialize unrequested rendering data.
  world.Update();
  XII_TEST_BOOL(frame.IsActive());
  XII_TEST_BOOL(renderer->GetGpuInstanceData() == nullptr);
}

namespace
{
  class TestViewContext : public xiiEngineProcessViewContext
  {
  public:
    using xiiEngineProcessViewContext::xiiEngineProcessViewContext;
    using xiiEngineProcessViewContext::SetCamera;
    using xiiEngineProcessViewContext::OnSwapChainChanged;
    using xiiEngineProcessViewContext::ResolveRenderPipeline;
    using xiiEngineProcessViewContext::UpdateRenderPipeline;
    xiiView* BorrowView() const { return GetView(); }
    void     SetWindowHandle(xiiRegisteredWindowHandle window) { m_hEditorWindow = window; }

  protected:
    xiiViewHandle CreateView() override
    {
      xiiView* view   = nullptr;
      auto     handle = GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Editor Test View", view);
      view->SetCamera(&m_Camera);
      view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
      return handle;
    }
  };

  class TestDocumentContext : public xiiEngineProcessDocumentContext
  {
    XII_ADD_DYNAMIC_REFLECTION(TestDocumentContext, xiiEngineProcessDocumentContext);

  public:
    TestDocumentContext() : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) {}
    explicit TestDocumentContext(xiiBitflags<xiiEngineProcessDocumentContextFlags> flags) : xiiEngineProcessDocumentContext(flags) {}
    explicit TestDocumentContext(xiiWorld& world) : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) { m_pWorld = &world; }
    ~TestDocumentContext() { m_pWorld = nullptr; }
    TestViewContext* AddView()
    {
      auto* view = static_cast<TestViewContext*>(CreateViewContext());
      m_ViewContexts.PushBack(view);
      return view;
    }
    xiiDelegate<void()> m_OnDeinitialize;

  protected:
    void OnDeinitialize() override
    {
      if (m_OnDeinitialize.IsValid())
        m_OnDeinitialize();
    }
    xiiEngineProcessViewContext* CreateViewContext() override { return XII_DEFAULT_NEW(TestViewContext, this); }
    void                         DestroyViewContext(xiiEngineProcessViewContext* context) override { XII_DEFAULT_DELETE(context); }
  };

  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestDocumentContext, 1, xiiRTTIDefaultAllocator<TestDocumentContext>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  xiiRenderPipelineResourceDescriptor ClearPresentation(xiiColor color)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    xiiVariantDictionary                properties;
    properties.Insert("Color", color);
    const auto clear   = descriptor.AddPass("Clear", xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
    const auto present = descriptor.AddPass("Present", xiiGetStaticRTTI<xiiPresentPass>());
    descriptor.Connect(clear, "Texture", present, "GradedColor").AssertSuccess();
    return descriptor;
  }

  class GatedPipelineLoader : public xiiResourceLoaderFromMemory
  {
  public:
    GatedPipelineLoader(xiiThreadSignal& entered, xiiThreadSignal& release) : m_Entered(entered), m_Release(release) {}

    xiiResourceLoadData OpenDataStream(const xiiResource* resource) override
    {
      m_Entered.RaiseSignal();
      m_Release.WaitForSignal(xiiTime::MakeFromSeconds(10));
      return xiiResourceLoaderFromMemory::OpenDataStream(resource);
    }

  private:
    xiiThreadSignal& m_Entered;
    xiiThreadSignal& m_Release;
  };

  // A test output owns a reference exactly as a GAL swapchain does.
  class ReferencedWindowOutput : public xiiWindowOutputTargetBase
  {
  public:
    ReferencedWindowOutput(xiiWindowBase& window, bool& destroyed) : m_Window(window), m_Destroyed(destroyed) { m_Window.AddReference(); }
    ~ReferencedWindowOutput()
    {
      m_Window.RemoveReference();
      m_Destroyed = true;
    }
    bool      GetVSyncEnabled() const override { return m_VSync; }
    void      SetVSyncEnabled(bool value) override { m_VSync = value; }
    void      PresentImage() override { ++m_Presents; }
    void      Resize(const xiiSizeU32& size) override { m_Size = size; }
    xiiResult CaptureImage(xiiImage&) override { return XII_FAILURE; }

  private:
    xiiWindowBase& m_Window;
    bool&          m_Destroyed;
    bool           m_VSync    = false;
    xiiUInt32      m_Presents = 0;
    xiiSizeU32     m_Size     = xiiSizeU32::MakeZero();
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, OutputOwnership)
{
  bool destroyed = false;
  {
    xiiEditorProcessViewWindow              window;
    xiiUniquePtr<xiiWindowOutputTargetBase> output = XII_DEFAULT_NEW(ReferencedWindowOutput, window, destroyed);
    window.SetOutputTarget(std::move(output));
    XII_TEST_BOOL(window.GetOutputTarget() != nullptr);
    XII_TEST_BOOL(!destroyed);
  }
  XII_TEST_BOOL(destroyed);
}

XII_CREATE_SIMPLE_TEST(EditorViews, ViewLifecycle)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Editor view lifetime test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  TestDocumentContext              document(world);
  auto*                            module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Size      = {80, 60};
  textureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  textureDescription.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto          texture          = environment.GetDevice()->CreateTexture(textureDescription);
  xiiViewHandle removed;
  {
    TestViewContext first(&document), second(&document);
    auto            target = texture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    first.SetupRenderTarget({}, target, 80, 60);
    second.SetupRenderTarget({}, target, 80, 60);
    auto* firstView  = first.BorrowView();
    auto* secondView = second.BorrowView();
    if (!XII_TEST_BOOL(firstView && secondView))
      return;
    XII_TEST_BOOL(!firstView->IsRenderEnabled());
    first.Redraw(true);
    XII_TEST_BOOL(firstView->IsRenderEnabled());
    first.SuspendRendering();
    XII_TEST_BOOL(!firstView->IsRenderEnabled());
    XII_TEST_BOOL(firstView->GetWorld() == &world);
    XII_TEST_BOOL(firstView != secondView);
    XII_TEST_BOOL(firstView->GetCamera() != secondView->GetCamera());
    XII_TEST_BOOL(firstView->GetRenderPipelineResource() == secondView->GetRenderPipelineResource());
    XII_TEST_BOOL(firstView->GetRenderTargetView() == target);
    XII_TEST_INT(target->GetRefCount(), 3); // Two views plus this client, with no cache ownership.
    auto* borrowedTarget = target.Borrow();
    target.Clear();
    XII_TEST_BOOL(firstView->GetRenderTargetView() == borrowedTarget);
    XII_TEST_BOOL(secondView->GetRenderTargetView() == borrowedTarget);
    target = texture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    XII_TEST_BOOL(target.Borrow() == borrowedTarget);
    first.SetupRenderTarget({}, target, 40, 30);
    XII_TEST_BOOL(first.BorrowView() == firstView);
    XII_TEST_FLOAT(firstView->GetViewport().width, 40, 0);
    XII_TEST_FLOAT(secondView->GetViewport().width, 80, 0);

    xiiViewRedrawMsgToEngine camera;
    camera.m_CameraMode   = xiiCameraMode::OrthoFixedHeight;
    camera.m_RenderMode   = xiiViewRenderMode::Wireframe;
    camera.m_fFovOrDim    = 20.0f;
    camera.m_fNearPlane   = -100.0f;
    camera.m_fFarPlane    = 100.0f;
    camera.m_vPosition    = xiiVec3(4, 5, 6);
    camera.m_vDirForwards = xiiVec3(1, 0, 0);
    camera.m_vDirUp       = xiiVec3(0, 0, 1);
    first.SetCamera(&camera);
    XII_TEST_BOOL(firstView->GetCamera()->GetCameraMode() == xiiCameraMode::OrthoFixedHeight);
    XII_TEST_VEC3(firstView->GetCamera()->GetCenterPosition(), camera.m_vPosition, 0.001f);
    XII_TEST_BOOL(firstView->GetViewRenderMode() == xiiViewRenderMode::Wireframe);

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Views require a selected resource and recover without a fallback")
    {
      world.SetWorldSimulationEnabled(false);
      first.Redraw(false);
      XII_TEST_BOOL(!firstView->GetRenderPipelineResource().IsValid());
      xiiTestLogInterface   log;
      xiiTestLogSystemScope scope(&log, true);
      log.ExpectMessage("has no selected render pipeline resource", xiiLogMsgType::ErrorMsg, 2);
      world.Update();
      world.Update(); // Diagnose once until selection changes.
      XII_TEST_BOOL(firstView->GetRenderPipelineInstance().GetPipeline() == nullptr);
      XII_TEST_BOOL(firstView->GetExtractionRequirements().IsNoFlagSet());
      XII_TEST_BOOL(firstView->GetRuntimeFeatures().IsNoFlagSet());
      xiiPlatformProfile profile;
      profile.AddMissingConfigs();
      auto* policy = profile.GetTypeConfig<xiiRenderPipelineProfileConfig>();
      if (!XII_TEST_BOOL(policy != nullptr))
        return;
      TestViewContext configured(&document);
      configured.SetRenderPipelineProfile(&profile);
      xiiRenderPipelineResourceHandle selected;
      log.ExpectMessage("Cannot select the editor view pipeline", xiiLogMsgType::ErrorMsg, 2);
      profile.GetTypeConfig<xiiRenderPipelineProfileConfig>()->m_sMainRenderPipeline.Clear();
      XII_TEST_BOOL(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::Main, selected).Failed());
      policy->m_sMainRenderPipeline = xiiCreateInstanceVisibilityTestResource().GetResourceID();
      XII_TEST_RESULT(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::Main, selected));
      XII_TEST_STRING(selected.GetResourceID(), policy->m_sMainRenderPipeline);
      configured.SetRenderPipelineResourceReference(xiiCreateInstanceVisibilityTestResource().GetResourceID());
      configured.UpdateRenderPipeline(*firstView);
      world.Update();
      XII_TEST_BOOL(firstView->GetRenderPipelineInstance().GetPipeline() != nullptr);
      policy->m_sMainRenderPipeline.Clear();
      configured.UpdateRenderPipeline(*firstView); // Explicit selection survives project changes.
      world.Update();
      XII_TEST_BOOL(firstView->GetRenderPipelineInstance().GetPipeline() != nullptr);
      configured.SetRenderPipelineResourceReference({});
      configured.UpdateRenderPipeline(*firstView);
      XII_TEST_BOOL(!firstView->GetRenderPipelineResource().IsValid());
      // Editor and fixed utility pipelines do not add selectors to the profile.
      XII_TEST_RESULT(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::EditorScene, selected));
      XII_TEST_BOOL(selected == xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiRenderPipelineProfileConfig::s_szEditorPipeline));
      XII_TEST_RESULT(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::ScenePreview, selected));
      XII_TEST_BOOL(selected == xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiRenderPipelineProfileConfig::s_szEditorPipeline));
      XII_TEST_RESULT(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::TexturePreview, selected));
      XII_TEST_BOOL(selected == xiiTexturePreviewPass::GetPipelineResource(false));
      XII_TEST_RESULT(configured.ResolveRenderPipeline(xiiEditorRenderPipelineUsage::CubeTexturePreview, selected));
      XII_TEST_BOOL(selected == xiiTexturePreviewPass::GetPipelineResource(true));
      configured.SetRenderPipelineResourceReference(xiiCreateInstanceVisibilityTestResource().GetResourceID());
      for (auto usage : {xiiEditorRenderPipelineUsage::Main, xiiEditorRenderPipelineUsage::ScenePreview, xiiEditorRenderPipelineUsage::EditorScene, xiiEditorRenderPipelineUsage::TexturePreview, xiiEditorRenderPipelineUsage::CubeTexturePreview})
      {
        XII_TEST_RESULT(configured.ResolveRenderPipeline(usage, selected));
        XII_TEST_BOOL(selected == xiiCreateInstanceVisibilityTestResource());
      }
      firstView->SetRenderPipelineResource(xiiCreateInstanceVisibilityTestResource());
      world.Update();
      XII_TEST_BOOL(firstView->GetRenderPipelineInstance().GetPipeline() != nullptr);
      firstView->SetRenderPipelineResource({});
      world.Update();
      XII_TEST_BOOL(firstView->GetRenderPipelineInstance().GetPipeline() == nullptr);
      first.SuspendRendering();
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Paused worlds execute only enabled views and retain their configuration")
    {
      struct ClearPassData
      {
        xiiRenderGraphTextureHandle m_Target;
      };
      xiiUInt32 firstExecutions = 0, secondExecutions = 0;
      auto      configure = [&](xiiView& view, xiiUInt32& executions) {
        view.SetRenderGraphBuilder([&texture, &executions](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
          graph.AddPass<ClearPassData>("Editor output test", xiiGALCommandQueueFlags::Graphics, [&texture](ClearPassData& data, xiiRenderGraphBuilder& builder) {
              auto imported = builder.ImportTexture("Editor output", texture, texture->GetResourceState());
              data.m_Target = builder.WriteTexture(imported, xiiGALResourceStateFlags::RenderTarget); }, [&executions](const ClearPassData& data, xiiRenderGraphPassContext& context) {
              ++executions;
              context.GetCommandList().ClearRenderTargetView(context.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Blue); }, true);
        });
      };
      configure(*firstView, firstExecutions);
      configure(*secondView, secondExecutions);
      world.SetWorldSimulationEnabled(false);
      first.SuspendRendering();
      second.SuspendRendering();
      XII_TEST_BOOL(firstView->IsValid());
      world.Update();
      XII_TEST_INT(firstExecutions, 0);
      XII_TEST_INT(secondExecutions, 0);

      first.Redraw(false);
      world.Update();
      XII_TEST_INT(firstExecutions, 1);
      XII_TEST_INT(secondExecutions, 0);
      xiiImage rendered;
      if (XII_TEST_RESULT(xiiCaptureThumbnail(texture, {80, 60}, rendered)))
      {
        const auto* pixel = rendered.GetPixelPointer<xiiUInt8>();
        XII_TEST_INT(pixel[0], 0);
        XII_TEST_INT(pixel[1], 0);
        XII_TEST_INT(pixel[2], 255);
        XII_TEST_INT(pixel[3], 255);
      }
      first.SuspendRendering();
      second.Redraw(false);
      world.Update();
      XII_TEST_INT(firstExecutions, 1);
      XII_TEST_INT(secondExecutions, 1);
      XII_TEST_BOOL(!world.GetWorldSimulationEnabled());
      XII_TEST_BOOL(firstView->GetCamera() != nullptr);
      XII_TEST_BOOL(firstView->GetRenderTargetView() == target);
      XII_TEST_FLOAT(firstView->GetViewport().width, 40, 0);
      XII_TEST_BOOL(firstView->GetRenderPipelineResource() == secondView->GetRenderPipelineResource());
    }
    removed = first.GetViewHandle();
  }
  xiiView* stale = nullptr;
  XII_TEST_BOOL(!module->TryGetView(removed, stale));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Document redraw requests expire after presentation and release their windows")
  {
    const auto           guid    = xiiUuid::MakeUuid();
    TestDocumentContext* context = XII_DEFAULT_NEW(TestDocumentContext, world);
    XII_TEST_STATUS(xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, context, nullptr, "View test"));
    auto* viewContext = context->AddView();
    viewContext->SetupRenderTarget({}, texture->GetDefaultView(xiiGALTextureViewType::RenderTarget), 80, 60);
    auto*      windows = xiiWindowManager::GetSingleton();
    const auto window  = windows->Register("Editor Test", viewContext, XII_DEFAULT_NEW(xiiEditorProcessViewWindow));
    viewContext->SetWindowHandle(window);
    bool redraw = true;
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::TryGetWindowRedrawState(window, redraw));
    XII_TEST_BOOL(!redraw);
    viewContext->Redraw(true);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::TryGetWindowRedrawState(window, redraw));
    XII_TEST_BOOL(redraw);
    xiiEngineProcessDocumentContext::FinishFrame();
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::TryGetWindowRedrawState(window, redraw));
    XII_TEST_BOOL(!redraw);
    XII_TEST_BOOL(!xiiEngineProcessDocumentContext::TryGetWindowRedrawState({}, redraw));
    const auto handle = viewContext->GetViewHandle();
    xiiEngineProcessDocumentContext::DestroyDocumentContext(guid);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == nullptr);
    XII_TEST_BOOL(!windows->IsValid(window));
    XII_TEST_BOOL(!module->TryGetView(handle, stale));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared-world documents close before their owning document")
  {
    const auto           ownerGuid    = xiiUuid::MakeUuid();
    const auto           borrowerGuid = xiiUuid::MakeUuid();
    TestDocumentContext* owner        = XII_DEFAULT_NEW(TestDocumentContext, xiiEngineProcessDocumentContextFlags::CreateWorld);
    XII_TEST_STATUS(xiiEngineProcessDocumentContext::AddDocumentContext(ownerGuid, {}, owner, nullptr, "Owner test"));
    TestDocumentContext* borrower = XII_DEFAULT_NEW(TestDocumentContext, *owner->GetWorld());
    XII_TEST_STATUS(xiiEngineProcessDocumentContext::AddDocumentContext(borrowerGuid, {}, borrower, nullptr, "Borrower test"));
    bool borrowerClosed = false, ownerClosed = false;
    borrower->m_OnDeinitialize = [&]() {
      XII_TEST_BOOL(owner->GetWorld() == borrower->GetWorld());
      XII_TEST_BOOL(!ownerClosed);
      borrowerClosed = true;
    };
    owner->m_OnDeinitialize = [&]() {
      XII_TEST_BOOL(borrowerClosed);
      ownerClosed = true;
    };
    xiiEngineProcessDocumentContext::DestroyAllDocumentContexts();
    XII_TEST_BOOL(borrowerClosed && ownerClosed);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(ownerGuid) == nullptr);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(borrowerGuid) == nullptr);
  }
}

XII_CREATE_SIMPLE_TEST(EditorViews, SwapChainViewportExtent)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  auto window = environment.CreateWindow(128, 96, "Editor viewport extent regression");
  if (!XII_TEST_BOOL(window != nullptr))
    return;
  xiiWorldDescription                worldDescription("Editor swapchain viewport");
  xiiWorld                           world(worldDescription);
  TestDocumentContext                document(world);
  TestViewContext                    context(&document);
  xiiSizeU32                         notifiedSize;
  xiiGALSwapChainCreationDescription description;
  description.m_pWindow = window.Borrow();
  xiiWindowOutputTargetGAL output(description, [&](xiiSharedPtr<xiiGALSwapChain> swapChain, xiiSizeU32 size) {
    notifiedSize = size;
    context.OnSwapChainChanged(std::move(swapChain), size);
  });
  if (!XII_TEST_BOOL(output.m_pSwapChain != nullptr))
    return;
  // IPC dimensions can precede or differ from the native surface allocation.
  context.SetupRenderTarget(output.m_pSwapChain, nullptr, 1024, 768);
  XII_LOCK(world.GetWriteMarker());
  auto* view = context.BorrowView();
  if (!XII_TEST_BOOL(view != nullptr))
    return;
  view->GetCamera()->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  view->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("ViewportExtent/Clear", ClearPresentation(xiiColor::Blue)));
  const auto checkAndRender = [&]() {
    const auto actual = output.m_pSwapChain->GetBackBufferTexture()->GetDescription().m_Size;
    XII_TEST_FLOAT(view->GetViewport().width, static_cast<float>(actual.width), 0);
    XII_TEST_FLOAT(view->GetViewport().height, static_cast<float>(actual.height), 0);
    context.Redraw(false);
    world.Update();
    device->WaitIdle();
    XII_TEST_RESULT(view->GetLastRenderResult());
  };
  checkAndRender();
  output.Resize({512, 384});
  XII_TEST_BOOL(notifiedSize == output.m_pSwapChain->GetCurrentSize());
  checkAndRender();
  for (const auto size : {xiiSizeU32(64, 48), xiiSizeU32(160, 120), xiiSizeU32(96, 72)})
  {
    XII_TEST_RESULT(static_cast<xiiWindow*>(window.Borrow())->Resize(size));
    window->ProcessWindowMessages();
    // Simulate surface-driven recreation, which has no output-target callback.
    XII_TEST_RESULT(output.m_pSwapChain->Resize(size));
    context.Redraw(false);
    checkAndRender();
    output.PresentImage();
  }
}

XII_CREATE_SIMPLE_TEST(EditorViews, PipelineLoadingFallback)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto previousFallback = xiiResourceManager::GetResourceTypeLoadingFallback<xiiRenderPipelineResource>();
  const auto previousForce    = xiiResourceManager::GetForceNoFallbackAcquisition();
  xiiResourceManager::ForceNoFallbackAcquisition(0);
  const auto fallback = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("ViewportFallback/Loading", ClearPresentation(xiiColor::Blue));
  xiiResourceManager::SetResourceTypeLoadingFallback(fallback);
  XII_SCOPE_EXIT(xiiResourceManager::SetResourceTypeLoadingFallback(previousFallback); xiiResourceManager::ForceNoFallbackAcquisition(previousForce));
  // Keep the fallback's shader resident before blocking the file-loading worker.
  const auto                                    blit = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/FinalBlit.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(blit, xiiResourceAcquireMode::BlockTillLoaded);
  if (!XII_TEST_BOOL(shader.IsValid() && shader->IsShaderValid()))
    return;
  xiiThreadSignal       entered, release, returned;
  auto                  loader = XII_DEFAULT_NEW(GatedPipelineLoader, entered, release);
  xiiMemoryStreamWriter writer(&loader->m_CustomData);
  writer << "ViewportFallback/Final.xiiBinRenderPipeline";
  xiiAssetFileHeader header;
  if (!XII_TEST_RESULT(header.Write(writer)))
    return;
  writer << xiiUInt8(3);
  if (!XII_TEST_RESULT(ClearPresentation(xiiColor(0, 1, 0)).Serialize(writer)))
    return;
  const auto                source = xiiResourceManager::GetExistingResourceOrCreateAsync<xiiRenderPipelineResource>("ViewportFallback/Final", std::move(loader));
  xiiRenderPipelineInstance instance;
  xiiResult                 synchronized = XII_FAILURE;
  auto                      task         = XII_DEFAULT_NEW(xiiDelegateTask<void>, "Synchronize pending pipeline", xiiTaskNesting::Maybe, [&]() {
    synchronized = instance.Synchronize(source);
    returned.RaiseSignal();
  });
  const auto                group        = xiiTaskSystem::StartSingleTask(task, xiiTaskPriority::LongRunning);
  XII_SCOPE_EXIT(release.RaiseSignal(); xiiTaskSystem::WaitForGroup(group); xiiResourceLock<xiiRenderPipelineResource> completed(source, xiiResourceAcquireMode::BlockTillLoaded));
  if (!XII_TEST_BOOL(entered.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled) ||
      !XII_TEST_BOOL(returned.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled))
    return;
  XII_TEST_RESULT(synchronized);
  XII_TEST_BOOL(instance.GetResourceAcquireResult() == xiiResourceAcquireResult::LoadingFallback);

  xiiWorldDescription              worldDescription("Pipeline loading fallback");
  xiiWorld                         world(worldDescription);
  TestDocumentContext              document(world);
  TestViewContext                  context(&document);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {16, 16};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto texture       = device->CreateTexture(target);
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  context.SetupRenderTarget({}, texture->GetDefaultView(xiiGALTextureViewType::RenderTarget), 16, 16);
  XII_LOCK(world.GetWriteMarker());
  auto* view = context.BorrowView();
  view->SetRenderPipelineResource(source);
  view->GetCamera()->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  const auto render = [&](bool final) {
    context.Redraw(false);
    world.Update();
    device->WaitIdle();
    XII_TEST_RESULT(view->GetLastRenderResult());
    XII_TEST_BOOL(view->GetRenderPipelineInstance().GetResourceAcquireResult() == (final ? xiiResourceAcquireResult::Final : xiiResourceAcquireResult::LoadingFallback));
    xiiImage image;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(texture, {16, 16}, image)))
    {
      const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 8, 8);
      XII_TEST_BOOL(pixel[final ? 1 : 2] > 200);
      XII_TEST_BOOL(pixel[final ? 2 : 1] < 5);
    }
  };
  render(false);
  release.RaiseSignal();
  {
    xiiResourceLock<xiiRenderPipelineResource> completed(source, xiiResourceAcquireMode::BlockTillLoaded);
    XII_TEST_BOOL(completed.GetAcquireResult() == xiiResourceAcquireResult::Final);
  }
  render(true);
}
