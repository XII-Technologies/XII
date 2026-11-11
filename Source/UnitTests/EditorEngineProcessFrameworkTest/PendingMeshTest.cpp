/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <SharedPluginScene/Scene/SceneMeshPlaceholder.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <Texture/Image/Image.h>

namespace
{
  struct PendingMeshSignals : xiiRefCounted
  {
    xiiThreadSignal m_Started;
    xiiThreadSignal m_Release;
    xiiAtomicBool   m_bTimedOut = false;
  };

  class PendingMeshLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<PendingMeshSignals> m_Signals;
    xiiResourceLoadData              OpenDataStream(const xiiResource* resource) override
    {
      m_Signals->m_Started.RaiseSignal();
      m_Signals->m_bTimedOut = m_Signals->m_Release.WaitForSignal(xiiTime::MakeFromSeconds(30)) == xiiThreadSignal::WaitResult::Timeout;
      return xiiResourceLoaderFromMemory::OpenDataStream(resource);
    }
  };

  class MissingMeshLoader : public xiiResourceTypeLoader
  {
  public:
    xiiResourceLoadData OpenDataStream(const xiiResource*) override { return {}; }
    void                CloseDataStream(const xiiResource*, const xiiResourceLoadData&) override {}
    bool                IsResourceOutdated(const xiiResource*) const override { return false; }
  };

  xiiMeshResourceDescriptor MakePendingTestMesh()
  {
    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor descriptor;
    descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
    descriptor.AddSection(12, 0, 0);
    descriptor.ComputeBounds();
    return descriptor;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(PendingMeshes);
XII_CREATE_SIMPLE_TEST(PendingMeshes, ExtractionAndEditorBounds)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  auto readyDescription = MakePendingTestMesh();
  XII_TEST_RESULT(readyDescription.BuildMeshlets());
  const auto readyBuffer = xiiResourceManager::LoadResource<xiiMeshBufferResource>("PendingMesh.ReloadableBuffer");
  {
    xiiUniquePtr<xiiResourceLoaderFromMemory> memory = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&memory->m_CustomData);
    XII_TEST_RESULT(readyDescription.GetMeshBufferDescriptor().Serialize(stream));
    xiiResourceManager::UpdateResourceWithCustomLoader(readyBuffer, std::move(memory));
    xiiResourceLock<xiiMeshBufferResource> buffer(readyBuffer, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(buffer.IsValid()))
      return;
  }
  readyDescription.UseExistingMeshBuffer(readyBuffer);
  const auto ready   = xiiResourceManager::CreateResource<xiiMeshResource>("PendingMesh.Ready", std::move(readyDescription));
  const auto pending = xiiResourceManager::LoadResource<xiiMeshResource>("PendingMesh.Pending");
  const auto missing = xiiResourceManager::LoadResource<xiiMeshResource>("PendingMesh.Missing");
  xiiResourceManager::UpdateResourceWithCustomLoader(missing, XII_DEFAULT_NEW(MissingMeshLoader));
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("Missing Resource of Type 'xiiMeshResource'", xiiLogMsgType::SeriousWarningMsg);
    xiiResourceLock<xiiMeshResource> absent(missing, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(!absent.IsValid());
  }
  xiiSharedPtr<PendingMeshSignals> signals = XII_DEFAULT_NEW(PendingMeshSignals);
  xiiUniquePtr<PendingMeshLoader>  loader  = XII_DEFAULT_NEW(PendingMeshLoader);
  loader->m_Signals                        = signals;
  xiiMemoryStreamWriter writer(&loader->m_CustomData);
  XII_TEST_RESULT(MakePendingTestMesh().Save(writer));
  xiiResourceManager::UpdateResourceWithCustomLoader(pending, std::move(loader));
  xiiResourceManager::PreloadResource(pending);
  XII_SCOPE_EXIT(signals->m_Release.RaiseSignal());
  if (!XII_TEST_BOOL(signals->m_Started.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled))
    return;

  xiiWorldDescription description("Pending mesh recovery");
  description.m_bReportErrorWhenStaticObjectMoves = false;
  xiiWorld world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject*              objects[3]    = {};
  xiiStaticMeshComponent*     components[3] = {};
  const xiiMeshResourceHandle meshes[]      = {ready, pending, missing};
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_LocalPosition = {0, float(i == 0 ? 2 : i == 1 ? 0 :
                                                                        -2),
                                         0};
    world.CreateObject(objectDescription, objects[i]);
    xiiStaticMeshComponent::CreateComponent(objects[i], components[i]);
    components[i]->SetMesh(meshes[i]);
  }
  world.Update(); // Must not wait for the blocked mesh loader while computing bounds.
  XII_TEST_BOOL(!signals->m_bTimedOut);
  auto*             module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiMeshRenderData packets[3];
  xiiRenderData*    data[3];
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    packets[i].m_hMesh           = meshes[i];
    packets[i].m_hOwnerObject    = objects[i]->GetHandle();
    packets[i].m_hOwnerComponent = components[i]->GetHandle();
    packets[i].m_GlobalTransform = objects[i]->GetGlobalTransform();
    packets[i].m_RangeMode       = xiiMeshRenderData::RangeMode::WholeMesh;
    data[i]                      = &packets[i];
  }
  for (xiiUInt64 frame = 0; frame < 4; ++frame)
  {
    xiiStringBuilder error;
    XII_TEST_RESULT(module->UpdateGpuInstances(xiiMakeArrayPtr(data), frame, &error));
    XII_TEST_BOOL(error.IsEmpty());
    XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 1);
  }

  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-5, 0, 0}, {0, 0, 0}, {0, 0, 1});
  xiiView* view = nullptr;
  module->CreateView("Unavailable mesh editor bounds", view);
  view->SetCamera(&camera);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  view->SetViewport({0, 0, 128, 128});
  view->SetRenderEnabled(false);
  xiiDynamicArray<xiiSceneMeshPlaceholder> bounds;
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  if (!XII_TEST_INT(bounds.GetCount(), 2))
    return;
  XII_TEST_BOOL(bounds[0].m_LocalBounds.GetHalfExtents().IsEqual(xiiVec3(0.25f), 0.0001f));
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_TEST_RESULT(xiiSceneMeshPlaceholder::Pick(*view, 0.5f, 0.5f, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == objects[1]->GetHandle());
  XII_TEST_BOOL(hit.m_Source.m_hOwnerComponent == components[1]->GetHandle());
  xiiDynamicArray<xiiGameObjectHandle> selected;
  xiiSceneMeshPlaceholder::PickObjectsInRect(*view, {0, 0, 128, 128}, selected);
  XII_TEST_INT(selected.GetCount(), 2);
  selected.Clear();
  xiiSceneMeshPlaceholder::PickObjectsInRect(*view, {48, 48, 32, 32}, selected);
  if (XII_TEST_INT(selected.GetCount(), 1))
    XII_TEST_BOOL(selected[0] == objects[1]->GetHandle());
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
  XII_TEST_RESULT(xiiSceneMeshPlaceholder::Pick(*view, 0.5f, 0.5f, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == objects[1]->GetHandle());
  selected.Clear();
  xiiSceneMeshPlaceholder::PickObjectsInRect(*view, {48, 48, 32, 32}, selected);
  XII_TEST_INT(selected.GetCount(), 1);
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  objects[2]->SetLocalPosition(xiiVec3(-4.9f, 0, 0));
  world.Update();
  selected.Clear();
  xiiSceneMeshPlaceholder::PickObjectsInRect(*view, {48, 48, 32, 32}, selected);
  XII_TEST_INT(selected.GetCount(), 2); // Includes bounds surrounding the near plane.
  XII_TEST_RESULT(xiiSceneMeshPlaceholder::Pick(*view, 0.5f, 0.5f, hit));
  XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == objects[2]->GetHandle());
  objects[2]->SetLocalPosition(xiiVec3(0, -2, 0));
  world.Update();
  const auto hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("PendingMeshHidden");
  objects[1]->SetTag(hidden);
  view->m_ExcludeTags.Set(hidden);
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_INT(bounds.GetCount(), 1);
  view->SetCameraUsageHint(xiiCameraUsageHint::MainView);
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_BOOL(bounds.IsEmpty());
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  XII_TEST_BOOL(xiiSceneMeshPlaceholder::Pick(*view, 0.5f, 0.5f, hit).Failed());
  view->m_ExcludeTags.Remove(hidden);
  components[2]->SetActiveFlag(false);
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_INT(bounds.GetCount(), 1);

  // A transform edit changes the same placeholder without replacing authoring identities.
  objects[1]->SetLocalPosition(xiiVec3(0, 0, 1));
  objects[1]->SetLocalScaling(xiiVec3(2, 1, 0.5f));
  objects[1]->SetLocalRotation(xiiQuat::MakeFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::MakeFromDegree(35)));
  world.Update();
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_BOOL(bounds[0].m_Transform.m_vPosition.IsEqual(xiiVec3(0, 0, 1), 0.0001f));
  XII_TEST_BOOL(bounds[0].m_hObject == objects[1]->GetHandle());

  signals->m_Release.RaiseSignal();
  {
    xiiResourceLock<xiiMeshResource> loaded(pending, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->IsReadyForRendering()))
      return;
  }
  world.Update();
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_BOOL(bounds.IsEmpty());
  XII_TEST_BOOL(objects[1]->GetLocalBounds().IsValid());
  XII_TEST_RESULT(module->UpdateGpuInstances(xiiMakeArrayPtr(data), 5));
  XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 2);

  xiiMeshBufferResourceHandle      bufferHandle;
  xiiSharedPtr<PendingMeshSignals> bufferSignals = XII_DEFAULT_NEW(PendingMeshSignals);
  xiiUniquePtr<PendingMeshLoader>  bufferLoader  = XII_DEFAULT_NEW(PendingMeshLoader);
  bufferLoader->m_Signals                        = bufferSignals;
  {
    xiiResourceLock<xiiMeshResource> mesh(ready, xiiResourceAcquireMode::PointerOnly);
    bufferHandle = mesh->GetMeshBuffer();
    xiiResourceLock<xiiMeshBufferResource> buffer(bufferHandle, xiiResourceAcquireMode::PointerOnly);
    xiiMemoryStreamWriter                  stream(&bufferLoader->m_CustomData);
    XII_TEST_RESULT(buffer->GetDescriptor().Serialize(stream));
  }
  xiiResourceManager::UpdateResourceWithCustomLoader(bufferHandle, std::move(bufferLoader));
  xiiResourceManager::PreloadResource(bufferHandle);
  XII_SCOPE_EXIT(bufferSignals->m_Release.RaiseSignal());
  if (!XII_TEST_BOOL(bufferSignals->m_Started.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled))
    return;
  XII_TEST_RESULT(module->UpdateGpuInstances(xiiMakeArrayPtr(data), 6));
  XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 1);
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  if (!XII_TEST_INT(bounds.GetCount(), 1))
    return;
  XII_TEST_BOOL(bounds[0].m_LocalBounds.GetHalfExtents().IsEqual(xiiVec3(0.5f), 0.0001f));
  bufferSignals->m_Release.RaiseSignal();
  {
    xiiResourceLock<xiiMeshBufferResource> buffer(bufferHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(buffer.IsValid()))
      return;
  }
  XII_TEST_RESULT(module->UpdateGpuInstances(xiiMakeArrayPtr(data), 7));
  XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 2);

  // A genuinely malformed loaded packet still fails with an actionable resource identity.
  packets[0].m_fLodBias = xiiMath::NaN<float>();
  xiiStringBuilder error;
  XII_TEST_BOOL(module->UpdateGpuInstances(xiiMakeArrayPtr(data), 8, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("PendingMesh.Ready") != nullptr);
}

XII_CREATE_SIMPLE_TEST(PendingMeshes, EditorRendersAndRecovers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto pipeline = xiiLoadPipelineLibraryResource("Editor");
  const auto ready    = xiiResourceManager::CreateResource<xiiMeshResource>("PendingRender.Ready", MakePendingTestMesh());
  const auto missing  = xiiResourceManager::LoadResource<xiiMeshResource>("PendingRender.Untransformed");
  xiiResourceManager::UpdateResourceWithCustomLoader(missing, XII_DEFAULT_NEW(MissingMeshLoader));
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log, true);
    log.ExpectMessage("Missing Resource of Type 'xiiMeshResource'", xiiLogMsgType::SeriousWarningMsg);
    xiiResourceLock<xiiMeshResource> resource(missing, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(!resource.IsValid());
  }
  xiiMaterialResourceDescriptor materialDescription;
  materialDescription.m_BaseColor     = xiiColor(0.2f, 0.6f, 0.9f);
  materialDescription.m_EmissiveColor = xiiColor(0.2f, 0.6f, 0.9f);
  const auto          material        = xiiResourceManager::CreateResource<xiiMaterialResource>("PendingRender.Material", std::move(materialDescription));
  xiiWorldDescription worldDescription("Mixed ready and untransformed meshes");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject* objects[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGameObjectDescription description;
    description.m_LocalPosition = {0, i == 0 ? -1.0f : 1.0f, 0};
    world.CreateObject(description, objects[i]);
    xiiStaticMeshComponent* component = nullptr;
    xiiStaticMeshComponent::CreateComponent(objects[i], component);
    component->SetMesh(i == 0 ? ready : missing);
    component->SetMaterialOverride(0, material);
  }
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-5, 0, 0}, {0, 0, 0}, {0, 0, 1});
  xiiView* view = nullptr;
  module->CreateView("Mixed mesh editor view", view);
  view->SetCamera(&camera);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  view->SetViewport({0, 0, 128, 128});
  view->SetRenderPipelineResource(pipeline);
  view->SetPipelineParameter("Editor.ShapeIcons", false);
  view->SetPipelineParameter("Editor.SelectionOverlay", false);
  xiiGALTextureCreationDescription targetDescription;
  targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  targetDescription.m_Size      = {128, 128};
  targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target                   = environment.GetDevice()->CreateTexture(targetDescription);
  if (!XII_TEST_BOOL(target != nullptr))
    return;
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  const auto update = [&] {
    for (xiiUInt32 frame = 0; frame < 5; ++frame)
    {
      xiiSceneMeshPlaceholder::Draw(*view);
      world.Update();
      environment.GetDevice()->WaitIdle();
      XII_TEST_RESULT(view->GetLastRenderResult());
    }
  };
  update();
  XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 1);
  const xiiSceneMeshPickingPass* picking  = nullptr;
  const auto&                    instance = view->GetRenderPipelineInstance();
  for (xiiUInt32 i = 0; i < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
    if (const auto* pass = xiiDynamicCast<const xiiSceneMeshPickingPass*>(instance.GetPass(i)))
      picking = pass;
  if (!XII_TEST_BOOL(picking != nullptr))
    return;
  const auto pick = [&](xiiGameObject* object) {
    xiiVec3 screen;
    XII_TEST_RESULT(view->GetData().ComputeScreenSpacePos(object->GetGlobalPosition(), screen));
    xiiSceneMeshPickingPass::PickingResult hit;
    XII_TEST_RESULT(picking->Pick(*view, static_cast<xiiUInt32>(screen.x * 128), static_cast<xiiUInt32>(screen.y * 128), hit));
    return hit;
  };
  XII_TEST_BOOL(pick(objects[0]).m_Source.m_hOwnerObject == objects[0]->GetHandle());
  XII_TEST_BOOL(pick(objects[1]).m_Source.m_hOwnerObject.IsInvalidated());
  xiiDynamicArray<xiiUInt32> pixels;
  if (!XII_TEST_BOOL(xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), target, pixels)))
    return;
  xiiUInt32 orange = 0;
  for (const auto pixel : pixels)
    orange += (pixel & 255) > 100 && ((pixel >> 8) & 255) > 30 && ((pixel >> 16) & 255) < 30;
  XII_TEST_BOOL(orange > 8);
  xiiImage image;
  if (XII_TEST_RESULT(xiiCaptureThumbnail(target, targetDescription.m_Size, image)))
    XII_TEST_RESULT(image.SaveTo(":imgout/PendingMesh-Bounds.png"));

  xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
  xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
  XII_TEST_RESULT(MakePendingTestMesh().Save(stream));
  xiiResourceManager::UpdateResourceWithCustomLoader(missing, std::move(loader));
  {
    xiiResourceLock<xiiMeshResource> resource(missing, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource.IsValid()))
      return;
  }
  update();
  XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 2);
  XII_TEST_BOOL(pick(objects[1]).m_Source.m_hOwnerObject == objects[1]->GetHandle());
  xiiDynamicArray<xiiSceneMeshPlaceholder> bounds;
  xiiSceneMeshPlaceholder::Collect(*view, bounds);
  XII_TEST_BOOL(bounds.IsEmpty());
  if (XII_TEST_RESULT(xiiCaptureThumbnail(target, targetDescription.m_Size, image)))
    XII_TEST_RESULT(image.SaveTo(":imgout/PendingMesh-Ready.png"));

  xiiGameObjectDescription invalidDescription;
  invalidDescription.m_bDynamic = true;
  xiiGameObject* invalidObject  = nullptr;
  world.CreateObject(invalidDescription, invalidObject);
  xiiSkinnedMeshComponent* invalid = nullptr;
  xiiSkinnedMeshComponent::CreateComponent(invalidObject, invalid);
  invalid->SetMesh(ready);
  const float unsupportedMorph = 1.0f;
  invalid->SetMorphWeights(xiiMakeArrayPtr(&unsupportedMorph, 1));
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log, true);
    log.ExpectMessage("GPU instance extraction failed: Invalid mesh bounds, skinning or morph data", xiiLogMsgType::ErrorMsg, 2);
    for (xiiUInt32 episode = 0; episode < 2; ++episode)
    {
      invalid->SetActiveFlag(true);
      for (xiiUInt32 frame = 0; frame < 3; ++frame)
      {
        world.Update();
        XII_TEST_BOOL(view->GetLastRenderResult().Failed());
      }
      invalid->SetActiveFlag(false);
      world.Update();
      XII_TEST_RESULT(view->GetLastRenderResult());
    }
  }
}
