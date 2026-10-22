/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/SurfaceRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>


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
    // Rebuild this fixture from the explicitly selected native surface graph.
    // No default full-stack pipeline or automatic device-dependent path is involved.
    xiiRenderPipelineResourceDescriptor definition;
    if (!XII_TEST_RESULT(xiiLoadPipelineLibraryDescriptor(definition, "EditorMeshPicking")))
      return;
    xiiUInt32 commands = xiiInvalidIndex;
    xiiUInt32 capture  = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < definition.m_Passes.GetCount(); ++i)
    {
      auto& pass = definition.m_Passes[i];
      if (pass.m_pType == xiiSurfaceQueryPass().GetDynamicRTTI())
      {
        pass.m_Properties.Insert("FillMode", static_cast<xiiInt64>(xiiGALFillMode::Wireframe));
        pass.m_Properties.Insert("UseMeshShaders", enableMeshShaders);
      }
      if (pass.m_pType == xiiSurfaceRasterConstantsPass().GetDynamicRTTI() && enableMeshShaders)
        pass.m_Properties.Insert("MeshGroupCountX", 65535U);
      if (pass.m_pType == xiiSceneMeshPickingPass().GetDynamicRTTI())
        capture = i;
      if (pass.m_pType == xiiVertexPullDrawCommandsPass().GetDynamicRTTI() && enableMeshShaders)
      {
        commands     = i;
        pass.m_pType = xiiMeshDrawCommandsPass().GetDynamicRTTI();
        pass.m_Properties.Clear();
      }
    }
    if (enableMeshShaders)
      for (xiiUInt32 i = definition.m_Connections.GetCount(); i-- > 0;)
      {
        const auto& edge = definition.m_Connections[i];
        if (edge.m_uiTarget == commands && edge.m_sInput != "VisibleMeshlets" && edge.m_sInput != "VisibleMeshletCount")
          definition.m_Connections.RemoveAtAndCopy(i);
      }
    if (!XII_TEST_BOOL(capture != xiiInvalidIndex))
      return;
    const auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(enableMeshShaders ? "NativeWireframeMesh" : "NativeWireframeVertexPull", std::move(definition));
    view->SetRenderPipelineResource(pipeline);
    xiiGALTextureCreationDescription targetDescription;
    targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    targetDescription.m_Size      = {64, 64};
    targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
    if (!XII_TEST_BOOL(target != nullptr))
      return;
    view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    for (xiiUInt32 frame = 0; frame < 6; ++frame)
    {
      world.Update();
      environment.GetDevice()->WaitIdle();
    }
    const auto* picking = xiiDynamicCast<const xiiSceneMeshPickingPass*>(view->GetRenderPipelineInstance().GetPass(capture));
    if (!XII_TEST_BOOL(picking != nullptr))
      return;
    XII_TEST_BOOL(view->GetViewRenderMode() == xiiViewRenderMode::Wireframe);
    xiiUInt32 edgePixels = 0;
    for (xiiUInt32 y = 0; y < 64; ++y)
      for (xiiUInt32 x = 0; x < 64; ++x)
      {
        xiiSceneMeshPickingPass::PickingResult hit;
        if (!XII_TEST_RESULT(picking->Pick(*view, x, y, hit)))
          return;
        if (!hit.m_Source.m_hOwnerObject.IsInvalidated())
        {
          ++edgePixels;
          XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == object->GetHandle());
        }
      }
    XII_TEST_BOOL_MSG(edgePixels > 30 && edgePixels < 64 * 64 / 2, "The authored raster path must produce sparse wireframe geometry with validated world identities.");
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
