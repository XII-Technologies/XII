/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/DepthTargetPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

namespace
{
  xiiRenderPipelineResourceHandle GridPipeline(bool occluded)
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    descriptor.AddPass("Background", xiiGetStaticRTTI<xiiClearTexturePass>());
    xiiVariantDictionary settings;
    settings.Insert("Depth", occluded ? 1.0f : 0.0f);
    descriptor.AddPass("Depth", xiiGetStaticRTTI<xiiDepthTargetPass>(), settings);
    descriptor.AddPass("Grid", xiiGetStaticRTTI<xiiInfiniteGridPass>());
    descriptor.AddPass("Present", xiiGetStaticRTTI<xiiPresentPass>());
    descriptor.Connect(0, "Texture", 2, "Color").AssertSuccess();
    descriptor.Connect(1, "DepthTarget", 2, "SceneDepth").AssertSuccess();
    descriptor.Connect(2, "Color", 3, "GradedColor").AssertSuccess();
    return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(occluded ? "GridTest/Occluded" : "GridTest/Visible", std::move(descriptor));
  }

  xiiUInt32 VisiblePixels(const xiiImage& image)
  {
    xiiUInt32 count = 0;
    for (xiiUInt64 i = 0; i < image.GetByteBlobPtr().GetCount(); i += 4)
      if (image.GetByteBlobPtr()[i] || image.GetByteBlobPtr()[i + 1] || image.GetByteBlobPtr()[i + 2])
        ++count;
    return count;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportVisualization);
XII_CREATE_SIMPLE_TEST(EditorViewportVisualization, InfiniteGrid)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "ViewportCaptures", "ViewportCaptures", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("ViewportCaptures"));
  xiiWorldDescription description("Procedural Editor grid");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Infinite grid", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 10000);
  camera.LookAt(xiiVec3(4, -6, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 512, 384});
  view->SetRenderPipelineResource(GridPipeline(false));
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {512, 384};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto output        = environment.GetDevice()->CreateTexture(target);
  view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  auto draw = [&](float spacing, const char* capture, bool global = true, const xiiTransform& plane = xiiTransform::MakeIdentity()) {
    XII_TEST_RESULT(xiiInfiniteGridPass::ConfigureView(*view, spacing, plane, global));
    world.Update();
    environment.GetDevice()->WaitIdle();
    XII_TEST_RESULT(view->GetLastRenderResult());
    xiiImage image;
    XII_TEST_RESULT(xiiCaptureThumbnail(output, target.m_Size, image));
    if (capture)
    {
      const xiiStringBuilder file(":ViewportCaptures/", capture, ".png");
      XII_TEST_RESULT(image.SaveTo(file));
    }
    return image;
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective, distant and shallow-angle procedural output")
  {
    const auto close = draw(1, "Grid-Close");
    XII_TEST_BOOL(VisiblePixels(close) > 1000);
    camera.LookAt(xiiVec3(100, -150, 100), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    XII_TEST_BOOL(VisiblePixels(draw(1, "Grid-Distant")) > 1000);
    camera.LookAt(xiiVec3(0, -10, 0.25f), xiiVec3(0, 10, 0), xiiVec3::MakeAxisZ());
    XII_TEST_BOOL(VisiblePixels(draw(0.1f, "Grid-Shallow")) > 100);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Orthographic, rotated/local planes, disabled grids and scene depth")
  {
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 12, 0.1f, 1000);
    camera.LookAt(xiiVec3(0, 0, 10), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    XII_TEST_BOOL(VisiblePixels(draw(0.25f, "Grid-Orthographic")) > 1000);
    auto plane        = xiiTransform::MakeIdentity();
    plane.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), xiiAngle::MakeFromDegree(45));
    XII_TEST_BOOL(VisiblePixels(draw(0.25f, "Grid-LocalPlane", false, plane)) > 1000);
    XII_TEST_INT(VisiblePixels(draw(0, nullptr)), 0);
    view->SetRenderPipelineResource(GridPipeline(true));
    XII_TEST_INT(VisiblePixels(draw(1, nullptr)), 0);
    view->SetRenderPipelineResource(GridPipeline(false));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Large-world phase is stable and invalid input clears stale settings")
  {
    camera.LookAt(xiiVec3(1000, 1000, 10), xiiVec3(1000, 1000, 0), xiiVec3::MakeAxisY());
    const auto reference = draw(0.25f, nullptr);
    camera.LookAt(xiiVec3(1000000, 1000000, 10), xiiVec3(1000000, 1000000, 0), xiiVec3::MakeAxisY());
    const auto large = draw(0.25f, "Grid-LargeWorld");
    XII_TEST_BOOL(reference.GetByteBlobPtr() == large.GetByteBlobPtr());
    XII_TEST_BOOL(xiiInfiniteGridPass::ConfigureView(*view, -1).Failed());
    XII_TEST_BOOL(!view->GetPipelineParameters().Contains("Editor.Grid"));
  }
}
