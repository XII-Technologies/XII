/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

namespace
{
  void TestWireframeRendering(bool enableMeshShaders)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(enableMeshShaders)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    // Extracted render packets retain mesh handles until their frame storage is reset.
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    if (environment.GetDevice()->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled)
    {
      xiiTestFramework::Output(xiiTestOutput::Message, "WireframeFill is unavailable on this device; native wireframe output was not exercised.");
      return;
    }
    if (!enableMeshShaders)
      XII_TEST_BOOL(environment.GetDevice()->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Disabled);

    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor meshDefinition;
    meshDefinition.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    meshDefinition.AddSubMesh(meshDefinition.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
    meshDefinition.ComputeBounds();
    const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Wireframe test cube", std::move(meshDefinition));
    xiiWorldDescription description("Wireframe pipeline output");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiGameObject* object = nullptr;
    world.CreateObject(xiiGameObjectDescription(), object);
    xiiMeshComponent* component = nullptr;
    xiiMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);

    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt(xiiVec3(-2, -3, 2), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    xiiView* view = nullptr;
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Wireframe output", view);
    view->SetCamera(&camera);
    view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
    view->SetViewport({0, 0, 64, 64});
    view->SetViewRenderMode(xiiViewRenderMode::Wireframe);
    view->SetRenderPipelineResource(xiiRenderPipelineResource::GetDefaultPipeline());
    xiiGALTextureCreationDescription targetDescription;
    targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    targetDescription.m_Size      = {64, 64};
    targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
    if (!XII_TEST_BOOL(target != nullptr))
      return;
    view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    for (xiiUInt32 frame = 0; frame < 3; ++frame)
      world.Update();
    xiiImage image;
    if (!XII_TEST_RESULT(xiiCaptureThumbnail(target, {64, 64}, image)))
      return;
    XII_TEST_BOOL(view->GetViewRenderMode() == xiiViewRenderMode::Wireframe);
    xiiUInt32 edgePixels = 0;
    for (xiiUInt32 y = 0; y < 64; ++y)
      for (xiiUInt32 x = 0; x < 64; ++x)
      {
        const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
        if (pixel[0] > 160)
        {
          ++edgePixels;
          XII_TEST_INT(pixel[0], pixel[1]);
          XII_TEST_INT(pixel[1], pixel[2]);
        }
      }
    XII_TEST_BOOL_MSG(edgePixels > 30 && edgePixels < 64 * 64 / 2, "The default pipeline must present actual wireframe geometry with this device's selected raster path.");
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, WireframeRendering)
{
  TestWireframeRendering(true);
}

XII_CREATE_SIMPLE_TEST(EditorViews, WireframeRenderingWithoutMeshShaders)
{
  TestWireframeRendering(false);
}
