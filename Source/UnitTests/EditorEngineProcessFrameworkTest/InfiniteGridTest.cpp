/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoGeometry.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorEngineProcessFramework/Rendering/EditorGizmoPass.h>
#include <EditorEngineProcessFramework/Rendering/InfiniteGridPass.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
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
    descriptor.AddPass("Gizmos", xiiGetStaticRTTI<xiiEditorGizmoPass>());
    descriptor.AddPass("Present", xiiGetStaticRTTI<xiiPresentPass>());
    descriptor.Connect(0, "Texture", 2, "Color").AssertSuccess();
    descriptor.Connect(1, "DepthTarget", 2, "SceneDepth").AssertSuccess();
    descriptor.Connect(2, "Color", 3, "Color").AssertSuccess();
    descriptor.Connect(1, "DepthTarget", 3, "SceneDepth").AssertSuccess();
    descriptor.Connect(3, "Color", 4, "GradedColor").AssertSuccess();
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
    const auto horizon = draw(0.1f, "Grid-HorizonFiltered");
    for (xiiUInt32 y = 0; y < 384; ++y)
    {
      xiiUInt32 green = 0;
      for (xiiUInt32 x = 0; x < 512; ++x)
      {
        const auto* pixel = horizon.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
        green += pixel[1] > pixel[0] * 1.5f && pixel[1] > pixel[2] * 1.5f;
      }
      XII_TEST_BOOL(green < 256);
    }
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
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 10000);
    camera.LookAt(xiiVec3(4, -6, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    const auto stationary = draw(0.25f, nullptr);
    auto       moved      = xiiTransform::MakeIdentity();
    moved.m_vPosition     = xiiVec3(4, 2, 7);
    XII_TEST_BOOL(stationary.GetByteBlobPtr() == draw(0.25f, nullptr, true, moved).GetByteBlobPtr());
    XII_TEST_BOOL(stationary.GetByteBlobPtr() != draw(0.25f, nullptr, false, moved).GetByteBlobPtr());
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 12, 0.1f, 1000);
    camera.LookAt(xiiVec3(1000, 1000, 10), xiiVec3(1000, 1000, 0), xiiVec3::MakeAxisY());
    const auto reference = draw(0.25f, nullptr);
    camera.LookAt(xiiVec3(1000000, 1000000, 10), xiiVec3(1000000, 1000000, 0), xiiVec3::MakeAxisY());
    const auto large = draw(0.25f, "Grid-LargeWorld");
    XII_TEST_BOOL(reference.GetByteBlobPtr() == large.GetByteBlobPtr());
    XII_TEST_BOOL(xiiInfiniteGridPass::ConfigureView(*view, -1).Failed());
    XII_TEST_BOOL(!view->GetPipelineParameters().Contains("Editor.Grid"));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dedicated gizmo primitives, states, rings and scene-depth policies")
  {
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    xiiEditorGizmoInstance instance = {};
    instance.Transform              = xiiMat4::MakeTranslation(xiiVec3(0, 0, -5));
    instance.Color                  = xiiVec4(0.4f, 0.1f, 0.05f, 1);
    instance.Parameters             = xiiVec4(2, 0, 1.2f, 1);
    instance.GuideStart             = xiiVec4::MakeZero();
    instance.Identity               = xiiVec4U32(xiiUInt32(xiiEngineGizmoHandleType::Box), 0, 42, 0);
    xiiVariantArray files;
    files.PushBack(xiiString());
    auto submit = [&]() {
      XII_TEST_RESULT(xiiEditorGizmoPass::Submit(*view, xiiArrayPtr<const xiiEditorGizmoInstance>(&instance, 1), files));
    };
    submit();
    const auto normal = draw(0, "Gizmo-Box");
    XII_TEST_BOOL(VisiblePixels(normal) > 1000);
    instance.Identity.y = XII_EDITOR_GIZMO_HOVER;
    submit();
    const auto hover = draw(0, "Gizmo-Hover");
    XII_TEST_BOOL(hover.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192)[0] > normal.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192)[0]);
    instance.Identity.y = XII_EDITOR_GIZMO_ACTIVE;
    submit();
    const auto active = draw(0, "Gizmo-Active");
    XII_TEST_BOOL(active.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192)[0] > hover.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192)[0]);
    view->SetRenderPipelineResource(GridPipeline(true));
    XII_TEST_INT(VisiblePixels(draw(0, nullptr)), 0);
    instance.Identity.y = XII_EDITOR_GIZMO_ON_TOP;
    submit();
    XII_TEST_BOOL(VisiblePixels(draw(0, "Gizmo-Occluded")) > 1000);
    view->SetRenderPipelineResource(GridPipeline(false));
    for (xiiUInt32 shape = 0; shape <= xiiUInt32(xiiEngineGizmoHandleType::ScaleUniform); ++shape)
    {
      if (shape == xiiUInt32(xiiEngineGizmoHandleType::FromFile))
        continue;
      instance.Identity.x = shape;
      instance.Identity.y = 0;
      submit();
      const auto image = draw(0, shape == XII_EDITOR_GIZMO_RING ? "Gizmo-Ring" : nullptr);
      XII_TEST_BOOL_MSG(VisiblePixels(image) > 0, "Missing gizmo shape {}", shape);
      if (shape == XII_EDITOR_GIZMO_RING)
      {
        XII_TEST_INT(image.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192)[0], 0);
        instance.Identity.y = XII_EDITOR_GIZMO_ACTIVE;
        submit();
        XII_TEST_BOOL(VisiblePixels(draw(0, "Gizmo-RotationDelta")) > 0);
      }
    }
    XII_TEST_RESULT(xiiEditorGizmoPass::Submit(*view, {}, {}));
    XII_TEST_INT(VisiblePixels(draw(0, nullptr)), 0);
    XII_TEST_INT(world.GetObjectCount(), 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Synchronized transform handles, picking, custom meshes and view isolation")
  {
    const xiiColor colors[] = {xiiColor(0.6f, 0.12f, 0.1f), xiiColor(0.1f, 0.5f, 0.18f), xiiColor(0.12f, 0.25f, 0.7f)};
    for (bool ortho : {false, true})
    {
      camera.SetCameraMode(ortho ? xiiCameraMode::OrthoFixedHeight : xiiCameraMode::PerspectiveFixedFovY, ortho ? 5.0f : 60.0f, 0.1f, 100);
      camera.LookAt(ortho ? xiiVec3(0, 0, 5) : xiiVec3(4, -6, 4), xiiVec3::MakeZero(), ortho ? xiiVec3::MakeAxisY() : xiiVec3::MakeAxisZ());
      for (auto shape : {xiiEngineGizmoHandleType::TranslationAxis, xiiEngineGizmoHandleType::RotationRing, xiiEngineGizmoHandleType::ScaleAxis})
      {
        xiiDynamicArray<xiiEditorGizmoInstance> instances;
        xiiVariantArray                         files;
        for (xiiUInt32 axis = 0; axis < 3; ++axis)
        {
          xiiEngineGizmoHandle handle;
          handle.ConfigureHandle(nullptr, shape, colors[axis], xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
          handle.SetVisible(true);
          auto          transform = xiiTransform::MakeIdentity();
          const xiiVec3 direction = axis == 0 ? xiiVec3::MakeAxisX() : axis == 1 ? xiiVec3::MakeAxisY() :
                                                                                   xiiVec3::MakeAxisZ();
          transform.m_qRotation   = xiiQuat::MakeShortestRotation(shape == xiiEngineGizmoHandleType::RotationRing ? xiiVec3::MakeAxisZ() : xiiVec3::MakeAxisX(), direction);
          handle.SetTransformation(transform);
          XII_TEST_RESULT(handle.AppendRenderData(*view, 0, instances, files));
          xiiMat4 matrix;
          float   pixel, time;
          XII_TEST_RESULT(handle.ComputeViewTransform(*view, matrix, pixel));
          const auto point            = matrix.TransformPosition(shape == xiiEngineGizmoHandleType::RotationRing ? xiiVec3::MakeAxisX() : xiiVec3(0.5f, 0, 0));
          const auto directionToPoint = (point - camera.GetCenterPosition()).GetNormalized();
          XII_TEST_RESULT(handle.PickRay(*view, camera.GetCenterPosition(), directionToPoint, time));
        }
        if (shape == xiiEngineGizmoHandleType::ScaleAxis)
        {
          xiiEngineGizmoHandle uniform;
          uniform.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::ScaleUniform, xiiColor::LightGray, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
          uniform.SetVisible(true);
          float time;
          XII_TEST_RESULT(uniform.PickRay(*view, camera.GetCenterPosition(), (xiiVec3::MakeZero() - camera.GetCenterPosition()).GetNormalized(), time));
          XII_TEST_RESULT(uniform.AppendRenderData(*view, 0, instances, files));
        }
        XII_TEST_RESULT(xiiEditorGizmoPass::Submit(*view, instances, files));
        xiiStringBuilder name;
        name.SetFormat("Gizmo-{}-{}", shape == xiiEngineGizmoHandleType::TranslationAxis ? "Translation" : shape == xiiEngineGizmoHandleType::RotationRing ? "Rotation" :
                                                                                                                                                             "Scale",
                       ortho ? "Orthographic" : "Perspective");
        const auto image = draw(0.25f, name);
        XII_TEST_BOOL(VisiblePixels(image) > 1000);
        if (shape == xiiEngineGizmoHandleType::ScaleAxis)
        {
          const auto* center = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 256, 192);
          XII_TEST_BOOL(center[0] > 100 && xiiMath::Abs(int(center[0]) - int(center[1])) < 5 && xiiMath::Abs(int(center[0]) - int(center[2])) < 5);
        }
        XII_TEST_BOOL(image.GetByteBlobPtr() == draw(0.25f, nullptr).GetByteBlobPtr());
      }
    }
    xiiView* independent = nullptr;
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Independent Editor viewport", independent);
    independent->SetCamera(&camera);
    independent->SetViewport({0, 0, 512, 384});
    independent->SetRenderPipelineResource(GridPipeline(false));
    auto otherOutput = environment.GetDevice()->CreateTexture(target);
    independent->SetRenderTargetView(otherOutput->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    const auto reference = draw(0, nullptr);
    xiiImage   other;
    XII_TEST_RESULT(xiiCaptureThumbnail(otherOutput, target.m_Size, other));
    XII_TEST_INT(VisiblePixels(other), 0);
    XII_TEST_BOOL(VisiblePixels(reference) > 0);
    world.GetOrCreateModule<xiiRenderWorldModule>()->DestroyView(independent->GetHandle());
    {
      xiiFileWriter file;
      XII_TEST_RESULT(file.Open(":ViewportCaptures/CustomGizmo.obj"));
      const xiiStringView mesh = "v -0.5 -0.5 0\nv 0.5 -0.5 0\nv 0 0.5 0\nf 1 2 3\n";
      XII_TEST_RESULT(file.WriteBytes(mesh.GetStartPointer(), mesh.GetElementCount()));
    }
    xiiEngineGizmoHandle custom;
    custom.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::FromFile, xiiColor::Orange, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::ShowInOrtho, ":ViewportCaptures/CustomGizmo.obj");
    custom.SetVisible(true);
    xiiDynamicArray<xiiEditorGizmoInstance> instances;
    xiiVariantArray                         files;
    XII_TEST_RESULT(custom.AppendRenderData(*view, 0, instances, files));
    XII_TEST_RESULT(xiiEditorGizmoPass::Submit(*view, instances, files));
    XII_TEST_BOOL(VisiblePixels(draw(0, "Gizmo-Custom")) > 100);
    target.m_Size = {320, 240};
    output        = environment.GetDevice()->CreateTexture(target);
    view->SetRenderTargetView(output->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetViewport({0, 0, 320, 240});
    XII_TEST_BOOL(VisiblePixels(draw(0.25f, "Grid-Gizmo-Resized")) > 100);
    XII_TEST_RESULT(xiiEditorGizmoPass::Submit(*view, {}, {}));
  }
}
