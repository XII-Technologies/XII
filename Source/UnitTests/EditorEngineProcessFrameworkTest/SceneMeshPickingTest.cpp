/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

namespace
{
  /// A minimal resource producer exercises the same explicit frame-table contract as PrepareFrame
  /// without involving lighting/postprocessing in a focused native query fixture.
  class MeshQueryTables : public xiiRenderPipelinePass
  {
    XII_ADD_DYNAMIC_REFLECTION(MeshQueryTables, xiiRenderPipelinePass);

  public:
    void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
    {
      for (const xiiStringView name : {"GPU Geometry Metadata", "GPU Meshlet Metadata", "Surface Material GPU Storage"})
        pins.PushBack({name, xiiRenderPipelinePin::Buffer, false, false});
    }
    xiiResult Build(xiiRenderPipelinePassContext& context) override
    {
      const auto geometry = xiiGeometryResidencyManager::AddUploadPass(context.m_Graph, context.m_Graph.GetFrameIndex());
      const auto material = xiiMaterialManager::AddUploadPasses(context.m_Graph);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuGeometryBaseIndex, geometry.m_uiGeometryBaseIndex);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuMaximumResidentMeshlets, geometry.m_uiMaximumResidentMeshletCount);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuSurfaceMaterialBaseIndex, material.m_uiSurfaceBaseIndex);
      XII_SUCCEED_OR_RETURN(context.SetOutput("GPU Geometry Metadata", geometry.m_hGeometryMetadata));
      XII_SUCCEED_OR_RETURN(context.SetOutput("GPU Meshlet Metadata", geometry.m_hMeshletMetadata));
      return context.SetOutput("Surface Material GPU Storage", material.m_hSurfaceData);
    }
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(MeshQueryTables, 1, xiiRTTIDefaultAllocator<MeshQueryTables>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneMeshPicking)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiGpuSceneRasterManager::IsSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native mesh query pass requires mesh shaders; GPU queries were not exercised on this device.");
    return;
  }
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Tables", MeshQueryTables::GetStaticRTTI()->GetTypeName());
  definition.AddPass("Query", xiiSceneMeshPickingPass::GetStaticRTTI()->GetTypeName());
  for (const xiiStringView name : {"GPU Geometry Metadata", "GPU Meshlet Metadata", "Surface Material GPU Storage"})
    XII_TEST_RESULT(definition.Connect(0, name, 1, name));
  auto                      pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("NativeMeshQueryPipeline", std::move(definition));
  xiiRenderPipelineInstance instance, other;
  if (!XII_TEST_RESULT(instance.Synchronize(pipeline)) || !XII_TEST_RESULT(other.Synchronize(pipeline)))
    return;
  const auto* pass      = xiiDynamicCast<const xiiSceneMeshPickingPass*>(instance.GetPass(1));
  const auto* otherPass = xiiDynamicCast<const xiiSceneMeshPickingPass*>(other.GetPass(1));
  if (!XII_TEST_BOOL(pass && otherPass && pass != otherPass))
    return;
  XII_TEST_BOOL(instance.GetPipeline() == other.GetPipeline());
  xiiRenderPipelineResourceDescriptor   editorDefinition;
  xiiSharedPtr<const xiiRenderPipeline> editorPipeline;
  XII_TEST_RESULT(xiiSceneShapeIconPass::GetPipelineDescriptor(editorDefinition));
  XII_TEST_RESULT(xiiRenderPipeline::Compile(editorDefinition, editorPipeline));

  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDefinition;
  meshDefinition.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList);
  meshDefinition.AddSubMesh(meshDefinition.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  meshDefinition.ComputeBounds();
  const auto                       mesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeQueryBox", std::move(meshDefinition));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;
  xiiWorldDescription worldDescription("Native mesh query world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic = true;
  xiiGameObject* frontObject   = nullptr;
  const auto     frontOwner    = world.CreateObject(objectDescription, frontObject);
  xiiGameObject* backObject    = nullptr;
  const auto     backOwner     = world.CreateObject(objectDescription, backObject);
  backObject->SetLocalPosition(xiiVec3(3, 0, 0));
  xiiMeshComponent* component      = nullptr;
  const auto        frontComponent = xiiMeshComponent::CreateComponent(frontObject, component);
  component->SetMesh(mesh);
  const auto backComponent = xiiMeshComponent::CreateComponent(backObject, component);
  component->SetMesh(mesh);
  world.Update();
  xiiMeshRenderData front, back;
  front.m_hMesh                        = mesh;
  back.m_hMesh                         = mesh;
  front.m_hOwnerObject                 = frontOwner;
  back.m_hOwnerObject                  = backOwner;
  front.m_hOwnerComponent              = frontComponent;
  back.m_hOwnerComponent               = backComponent;
  front.m_GlobalTransform              = xiiTransform::MakeIdentity();
  back.m_GlobalTransform               = xiiTransform::MakeIdentity();
  back.m_GlobalTransform.m_vPosition.x = 3;
  front.m_uiPrimitiveCount = back.m_uiPrimitiveCount = loaded->GetSections()[0].m_uiPrimitiveCount;
  front.m_uiSectionIndex                             = 1024; // Full-width packet section identity, not legacy 7-bit packing.
  xiiExtractedRenderData extracted;
  extracted.AddRenderData(&front);
  extracted.AddRenderData(&back);
  extracted.SortAndBatches();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  auto*      renderer   = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("Native query", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  view->SetViewport({8, 12, 32, 32});
  view->GetExtractedRenderData()->AddRenderData(&front);
  view->GetExtractedRenderData()->AddRenderData(&back);
  view->GetExtractedRenderData()->SortAndBatches();
  xiiGALTextureCreationDescription output;
  output.m_Type      = xiiGALResourceDimension::Texture2D;
  output.m_Size      = {48, 48};
  output.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  output.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target        = environment.GetDevice()->CreateTexture(output);
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  xiiSceneDatabaseContext mainScene;
  if (!XII_TEST_RESULT(mainScene.Initialize()))
    return;
  xiiRenderGraphContext graphContext;
  if (!XII_TEST_RESULT(graphContext.Initialize("Native query graph")))
    return;
  xiiGpuFrameCompletionTracker completion;
  completion.Initialize(environment.GetDevice());
  xiiUInt64            nextFrame = 1;
  xiiVariantDictionary parameters;
  auto                 render = [&](xiiRenderPipelineInstance* runtime = nullptr, xiiView* queryView = nullptr) -> bool {
    const auto frame     = nextFrame++;
    const auto completed = completion.PollCompletedFrames();
    xiiGALBindlessResourceTable::Collect(completed);
    xiiMaterialManager::BeginFrame(frame, completed);
    xiiGpuVisibilityManager::BeginFrame(frame, completed);
    if (!XII_TEST_RESULT(mainScene.SynchronizeExtractedMeshes(extracted, frame)))
      return false;
    xiiMaterialManager::PrepareGpuResources();
    xiiGeometryResidencyManager::ProcessStreaming(frame, completed, xiiMath::MaxValue<xiiUInt64>());
    auto& cache = graphContext.GetResourceCache();
    cache.BeginFrame(frame, completed);
    XII_SCOPE_EXIT(cache.EndFrame());
    auto& graph = graphContext.GetGraph();
    graph.BeginSetup(frame);
    graphContext.GetBlackboard().ClearFrame();
    auto& execution = runtime ? *runtime : instance;
    if (!XII_TEST_RESULT(execution.Build(queryView ? queryView : view, graph, graphContext.GetBlackboard(), parameters)))
      return false;
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphContext.GetBlackboard(), &cache)))
      return false;
    completion.CaptureSubmittedFrame(frame);
    completion.WaitForFrame(frame); // Test-only wait; production queries remain nonblocking.
    return true;
  };
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native owner, surface position, normal and nonzero viewport origin")
  {
    if (!render() || !render()) // First shader/resource loads conservatively invalidate capture.
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    XII_TEST_BOOL(hit.m_Source.m_hOwnerComponent == frontComponent);
    XII_TEST_INT(hit.m_Source.m_uiSectionIndex, 1024);
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, -0.5f, 0.001f);
    XII_TEST_VEC3(hit.m_vWorldNormal, -xiiVec3::MakeAxisX(), 0.001f);
    XII_TEST_FLOAT(hit.m_vRayStart.x, -1.9f, 0.001f);
    XII_TEST_RESULT(pass->Pick(*view, 8, 12, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
    XII_TEST_BOOL(pass->Pick(*view, 7, 28, hit).Failed());
    XII_TEST_BOOL(otherPass->Pick(*view, 24, 28, hit).Failed());
    xiiDynamicArray<xiiGameObjectHandle> objects;
    XII_TEST_RESULT(pass->PickObjectsInRect(*view, {8, 12, 32, 32}, objects));
    XII_TEST_BOOL(objects.Contains(frontOwner));
    XII_TEST_BOOL(!objects.Contains(backOwner));
    XII_TEST_INT(objects.GetCount(), 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared resource views retain independent native query captures")
  {
    xiiView*   secondView   = nullptr;
    const auto secondHandle = renderer->CreateView("Independent query", secondView);
    XII_SCOPE_EXIT(renderer->DestroyView(secondHandle));
    secondView->SetCamera(&camera);
    secondView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
    secondView->SetViewport(view->GetViewport());
    secondView->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    secondView->GetExtractedRenderData()->AddRenderData(&front);
    secondView->GetExtractedRenderData()->SortAndBatches();
    if (!render(&other, secondView) || !render(&other, secondView))
      return;
    XII_TEST_RESULT(otherPass->Pick(*secondView, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    XII_TEST_BOOL(otherPass->Pick(*view, 24, 28, hit).Failed());
    XII_TEST_BOOL(pass->Pick(*secondView, 24, 28, hit).Failed());
    other.InvalidateTemporalHistory();
    XII_TEST_BOOL(otherPass->Pick(*secondView, 24, 28, hit).Failed());
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selected object click-through preserves the main scene")
  {
    parameters.Insert("Editor.PickSelected", false);
    xiiVariantArray selected;
    selected.PushBack(static_cast<xiiUInt64>(frontOwner.GetInternalID().m_Data));
    parameters.Insert("Editor.SelectedObjects", selected);
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == backOwner);
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, 2.5f, 0.001f);
    XII_TEST_INT(mainScene.GetDatabase().GetObjectCount(), 2);
    for (const auto& entry : mainScene.GetDatabase().GetGpuInstances())
      XII_TEST_BOOL((entry.m_uiFlags & xiiSceneObjectFlags::Enabled) != 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Camera, viewport, live transform, deletion and explicit history invalidation")
  {
    view->SetViewport({9, 12, 32, 32});
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    view->SetViewport({8, 12, 32, 32});
    camera.MoveLocally(0.1f, 0, 0);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    XII_TEST_BOOL(world.TryGetObject(backOwner, backObject));
    backObject->SetGlobalPosition(xiiVec3(4, 0, 0));
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    backObject->SetGlobalPosition(xiiVec3(3, 0, 0));
    world.DeleteObjectNow(backOwner);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
    instance.InvalidateTemporalHistory();
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
  }
  view->GetExtractedRenderData()->Clear();
}
