/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneCameraPreview.h>
#include <Texture/Image/Image.h>

namespace
{
  class CameraPreviewTestPass : public xiiRenderPipelinePass
  {
    XII_ADD_DYNAMIC_REFLECTION(CameraPreviewTestPass, xiiRenderPipelinePass);

  public:
    void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.Clear(); }
    xiiResult Build(xiiRenderPipelinePassContext& context) override
    {
      struct Data
      {
        xiiRenderGraphTextureHandle m_Target;
        xiiRenderGraphTextureHandle m_Depth;
        xiiColor                    m_Color;
      };
      xiiSharedPtr<xiiGALTexture> target    = context.m_pView->GetRenderTargetView()->GetTexture();
      const auto*                 parameter = context.m_Parameters.GetValue("TestColor");
      const auto                  color     = parameter ? parameter->Get<xiiColor>() : xiiColor::Black;
      context.m_Graph.AddPass<Data>("Camera preview test", xiiGALCommandQueueFlags::Graphics, [&target, color](Data& data, xiiRenderGraphBuilder& builder) {
          data.m_Target = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, target, target->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
          auto depth = target->GetDescription();
          depth.m_Format = xiiGALResourceFormat::D32Float;
          depth.m_BindFlags = xiiGALBindFlags::DepthStencil;
          data.m_Depth = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, depth, xiiGALResourceStateFlags::DepthWrite);
          data.m_Color = color; }, [](const Data& data, xiiRenderGraphPassContext& execution) {
          execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), data.m_Color);
          execution.GetCommandList().ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0); }, true);
      if (context.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::EditorView)
      {
        xiiDebugRenderer::AddRenderGraphPasses(context.m_Graph);
        context.m_Graph.AddPass<Data>("Camera preview test output", xiiGALCommandQueueFlags::Graphics, [](Data& data, xiiRenderGraphBuilder& builder) {
            data.m_Target = builder.ReadTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, xiiGALResourceStateFlags::CopySource);
            builder.ExportTexture(data.m_Target, xiiGALResourceStateFlags::CopySource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
      }
      return XII_SUCCESS;
    }
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(CameraPreviewTestPass, 1, xiiRTTIDefaultAllocator<CameraPreviewTestPass>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneCameraPreview)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Camera preview test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*                    module      = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCameraComponent *     firstCamera = nullptr, *secondCamera = nullptr;
  xiiGameObjectDescription objectDescription;
  objectDescription.m_LocalPosition = xiiVec3(1, 2, 3);
  xiiGameObject* firstObject        = nullptr;
  world.CreateObject(objectDescription, firstObject);
  auto* manager = world.GetOrCreateComponentManager<xiiCameraComponentManager>();
  manager->CreateComponent(firstObject, firstCamera);
  objectDescription.m_LocalPosition = xiiVec3(4, 5, 6);
  xiiGameObject* secondObject       = nullptr;
  world.CreateObject(objectDescription, secondObject);
  manager->CreateComponent(secondObject, secondCamera);
  firstCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovX);
  firstCamera->SetFieldOfView(75);
  secondCamera->SetCameraMode(xiiCameraMode::OrthoFixedHeight);
  secondCamera->SetOrthoDimension(24);
  secondCamera->SetRenderScale(0.5f);
  const auto& include = xiiTagRegistry::GetGlobalRegistry().RegisterTag("PreviewIncluded");
  const auto& exclude = xiiTagRegistry::GetGlobalRegistry().RegisterTag("PreviewExcluded");
  firstCamera->m_IncludeTags.Set(include);
  firstCamera->m_ExcludeTags.Set(exclude);
  world.Update();

  xiiRenderPipelineResourceDescriptor pipelineDescription;
  XII_TEST_INT(pipelineDescription.AddPass("Clear", CameraPreviewTestPass::GetStaticRTTI()->GetTypeName()), 0);
  auto     pipeline   = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("SceneCameraPreviewTest", std::move(pipelineDescription));
  xiiView* editorView = nullptr;
  module->CreateView("Editor owner", editorView);
  editorView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  editorView->SetRenderPipelineResource(pipeline);
  editorView->SetRenderEnabled(false);
  xiiCamera editorCamera;
  editorCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  editorView->SetCamera(&editorCamera);
  editorView->SetViewport({0, 0, 300, 180});
  xiiGALTextureCreationDescription editorTargetDescription;
  editorTargetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  editorTargetDescription.m_Size      = {300, 180};
  editorTargetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  editorTargetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto editorTarget                   = environment.GetDevice()->CreateTexture(editorTargetDescription);
  editorView->SetRenderTargetView(editorTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  xiiViewHandle destroyed;
  {
    xiiSceneCameraPreview first(world), second(world);
    XII_TEST_BOOL(first.GetView() == nullptr);
    first.SuspendRendering();
    XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
    XII_TEST_RESULT(second.Update(*editorView, *secondCamera));
    auto* firstView  = first.GetView();
    auto* secondView = second.GetView();
    if (!XII_TEST_BOOL(firstView && secondView))
      return;
    destroyed         = firstView->GetHandle();
    auto firstTarget  = firstView->GetRenderTargetView()->GetTexture();
    auto secondTarget = secondView->GetRenderTargetView()->GetTexture();

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Independent native views, cameras and targets with a shared resource")
    {
      XII_TEST_BOOL(firstView != secondView);
      XII_TEST_BOOL(firstTarget != secondTarget);
      XII_TEST_BOOL(firstView->GetCamera() != secondView->GetCamera());
      XII_TEST_BOOL(firstView->GetWorld() == &world);
      XII_TEST_BOOL(firstView->GetCameraUsageHint() == xiiCameraUsageHint::RenderTarget);
      XII_TEST_BOOL(firstView->GetRenderPipelineResource() == pipeline);
      XII_TEST_BOOL(secondView->GetRenderPipelineResource() == pipeline);
      XII_TEST_BOOL(firstView->GetRenderGraphBuilder().IsValid() == false);
      XII_TEST_BOOL(firstView->GetCamera()->GetCameraMode() == xiiCameraMode::PerspectiveFixedFovX);
      XII_TEST_FLOAT(firstView->GetCamera()->GetFovOrDim(), 75, 0);
      XII_TEST_BOOL(secondView->GetCamera()->GetCameraMode() == xiiCameraMode::OrthoFixedHeight);
      XII_TEST_FLOAT(secondView->GetCamera()->GetFovOrDim(), 24, 0);
      XII_TEST_VEC3(firstView->GetCamera()->GetCenterPosition(), xiiVec3(1, 2, 3), 0.0001f);
      XII_TEST_VEC3(secondView->GetCamera()->GetCenterPosition(), xiiVec3(4, 5, 6), 0.0001f);
      XII_TEST_FLOAT(firstView->GetCamera()->GetExposure(), firstCamera->GetExposure(), 0.0001f);
      XII_TEST_BOOL(firstView->m_IncludeTags.IsSet(include));
      XII_TEST_BOOL(firstView->m_ExcludeTags.IsSet(exclude));
      XII_TEST_BOOL(firstView->m_ExcludeTags.IsSetByName(xiiStringView("Editor")));
      XII_TEST_BOOL(firstView->m_ExcludeTags.IsSetByName(xiiStringView("EditorHidden")));
      XII_TEST_FLOAT(secondView->GetRenderScale(), 0.5f, 0.0001f);
      XII_TEST_INT(firstTarget->GetDescription().m_Size.width, 256);
      XII_TEST_INT(firstTarget->GetDescription().m_Size.height, 144);
    }

    auto checkPixel = [](xiiGALTexture* texture, xiiUInt8 red, xiiUInt8 green, xiiUInt8 blue) {
      xiiImage image;
      if (!XII_TEST_RESULT(xiiCaptureThumbnail(texture, {256, 144}, image)))
        return;
      const auto* pixel = image.GetPixelPointer<xiiUInt8>();
      XII_TEST_INT(pixel[0], red);
      XII_TEST_INT(pixel[1], green);
      XII_TEST_INT(pixel[2], blue);
      XII_TEST_INT(pixel[3], 255);
    };

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Initialized image, real pipeline execution and redraw suspension")
    {
      checkPixel(firstTarget, 0, 0, 0);
      firstView->SetPipelineParameter("TestColor", xiiColor::Red);
      secondView->SetPipelineParameter("TestColor", xiiColor::Lime);
      world.Update();
      checkPixel(firstTarget, 255, 0, 0);
      checkPixel(secondTarget, 0, 255, 0);
      first.SuspendRendering();
      firstView->SetPipelineParameter("TestColor", xiiColor::Blue);
      secondView->SetPipelineParameter("TestColor", xiiColor::Blue);
      world.Update();
      checkPixel(firstTarget, 255, 0, 0);
      checkPixel(secondTarget, 0, 0, 255);
      XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
      XII_TEST_BOOL(first.GetView() == firstView);
      XII_TEST_BOOL(first.GetView()->GetRenderTargetView()->GetTexture() == firstTarget);
      world.Update();
      checkPixel(firstTarget, 0, 0, 255);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Preview is composited by native debug passes and consumed once")
    {
      xiiDebugRenderer::ClearContext(*editorView);
      XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
      editorView->SetRenderEnabled(true);
      world.Update();
      xiiImage image;
      if (XII_TEST_RESULT(xiiCaptureThumbnail(editorTarget, {300, 180}, image)))
      {
        const auto* inside = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 44, 44);
        XII_TEST_INT(inside[0], 0);
        XII_TEST_INT(inside[1], 0);
        XII_TEST_INT(inside[2], 255);
        const auto* outside = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 4, 4);
        XII_TEST_INT(outside[2], 0);
      }
      world.Update();
      if (XII_TEST_RESULT(xiiCaptureThumbnail(editorTarget, {300, 180}, image)))
        XII_TEST_INT(image.GetPixelPointer<xiiUInt8>(0, 0, 0, 44, 44)[2], 0);
      editorView->SetRenderEnabled(false);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "World ownership is checked independently of local view handles")
    {
      xiiWorldDescription otherDescription("Foreign camera preview world");
      xiiWorld            otherWorld(otherDescription);
      XII_LOCK(otherWorld.GetWriteMarker());
      otherWorld.SetWorldSimulationEnabled(false);
      xiiView* otherView = nullptr;
      otherWorld.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Foreign editor", otherView);
      otherView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
      otherView->SetRenderEnabled(false);
      xiiGameObject* otherObject = nullptr;
      otherWorld.CreateObject(xiiGameObjectDescription(), otherObject);
      xiiCameraComponent* otherCamera = nullptr;
      otherWorld.GetOrCreateComponentManager<xiiCameraComponentManager>()->CreateComponent(otherObject, otherCamera);
      otherWorld.Update();
      XII_TEST_BOOL(first.Update(*otherView, *firstCamera).Failed());
      XII_TEST_BOOL(first.Update(*editorView, *otherCamera).Failed());
      XII_TEST_BOOL(!firstView->IsRenderEnabled());
      XII_TEST_BOOL(first.GetView() == firstView);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Changed camera, invalid input and recreated view")
    {
      firstCamera->SetCameraMode(xiiCameraMode::OrthoFixedWidth);
      firstCamera->SetOrthoDimension(18);
      firstCamera->m_IncludeTags.Clear();
      firstCamera->m_ExcludeTags.Clear();
      XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
      XII_TEST_BOOL(firstView->GetCamera()->GetCameraMode() == xiiCameraMode::OrthoFixedWidth);
      XII_TEST_FLOAT(firstView->GetCamera()->GetFovOrDim(), 18, 0);
      XII_TEST_BOOL(firstView->m_IncludeTags.IsEmpty());
      XII_TEST_BOOL(!firstView->m_ExcludeTags.IsSet(exclude));
      firstCamera->SetActiveFlag(false);
      XII_TEST_BOOL(first.Update(*editorView, *firstCamera).Failed());
      XII_TEST_BOOL(!firstView->IsRenderEnabled());
      firstCamera->SetActiveFlag(true);
      firstCamera->SetCameraMode(xiiCameraMode::PerspectiveFixedFovY);
      firstCamera->SetFieldOfView(180);
      XII_TEST_BOOL(first.Update(*editorView, *firstCamera).Failed());
      firstCamera->SetFieldOfView(60);
      firstCamera->SetNearPlane(xiiMath::NaN<float>());
      XII_TEST_BOOL(first.Update(*editorView, *firstCamera).Failed());
      firstCamera->SetNearPlane(0.1f);
      firstCamera->SetCameraMode(xiiCameraMode::None);
      XII_TEST_BOOL(first.Update(*editorView, *firstCamera).Failed());
      firstCamera->SetCameraMode(xiiCameraMode::Stereo);
      XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
      XII_TEST_BOOL(firstView->GetCamera()->GetCameraMode() == xiiCameraMode::PerspectiveFixedFovY);
      XII_TEST_FLOAT(firstView->GetCamera()->GetFovOrDim(), 45, 0);
      editorView->SetCameraUsageHint(xiiCameraUsageHint::Thumbnail);
      XII_TEST_BOOL(first.Update(*editorView, *firstCamera).Failed());
      editorView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
      module->DestroyView(destroyed);
      XII_TEST_BOOL(first.GetView() == nullptr);
      XII_TEST_RESULT(first.Update(*editorView, *firstCamera));
      XII_TEST_BOOL(first.GetView()->GetHandle() != destroyed);
      XII_TEST_BOOL(first.GetView()->GetRenderTargetView()->GetTexture() == firstTarget);
      destroyed = first.GetView()->GetHandle();
    }
  }
  xiiView* remaining = nullptr;
  XII_TEST_BOOL(!module->TryGetView(destroyed, remaining));
  XII_TEST_BOOL(editorView->GetRenderPipelineResource() == pipeline);
}
