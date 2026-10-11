/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  class TestViewContext : public xiiEngineProcessViewContext
  {
  public:
    using xiiEngineProcessViewContext::xiiEngineProcessViewContext;
    using xiiEngineProcessViewContext::SetCamera;
    xiiView* BorrowView() const { return GetView(); }

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
  public:
    explicit TestDocumentContext(xiiWorld& world) : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) { m_pWorld = &world; }
    ~TestDocumentContext() { m_pWorld = nullptr; }

  protected:
    xiiEngineProcessViewContext* CreateViewContext() override { return XII_DEFAULT_NEW(TestViewContext, this); }
    void                         DestroyViewContext(xiiEngineProcessViewContext* context) override { XII_DEFAULT_DELETE(context); }
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
    auto*           target = texture->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow();
    first.SetupRenderTarget({}, target, 80, 60);
    second.SetupRenderTarget({}, target, 80, 60);
    auto* firstView  = first.BorrowView();
    auto* secondView = second.BorrowView();
    if (!XII_TEST_BOOL(firstView && secondView))
      return;
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
    removed = first.GetViewHandle();
  }
  xiiView* stale = nullptr;
  XII_TEST_BOOL(!module->TryGetView(removed, stale));
}
