/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

namespace
{
  class TestViewContext : public xiiEngineProcessViewContext
  {
  public:
    using xiiEngineProcessViewContext::xiiEngineProcessViewContext;
    using xiiEngineProcessViewContext::SetCamera;
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
    auto*           target = texture->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow();
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
    xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, context, nullptr, "View test");
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
    xiiEngineProcessDocumentContext::AddDocumentContext(ownerGuid, {}, owner, nullptr, "Owner test");
    TestDocumentContext* borrower = XII_DEFAULT_NEW(TestDocumentContext, *owner->GetWorld());
    xiiEngineProcessDocumentContext::AddDocumentContext(borrowerGuid, {}, borrower, nullptr, "Borrower test");
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
