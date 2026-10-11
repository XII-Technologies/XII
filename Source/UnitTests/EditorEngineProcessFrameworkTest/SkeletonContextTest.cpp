/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreviewMessages.h>

XII_CREATE_SIMPLE_TEST(EditorViews, SkeletonContext)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto       guid = xiiUuid::MakeUuid();
  xiiStringBuilder id;
  xiiConversionUtils::ToString(guid, id);
  auto                      hSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(id);
  xiiEditableSkeleton       authoring;
  xiiEditableSkeletonJoint* pRoot = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
  authoring.m_Children.PushBack(pRoot);
  pRoot->SetName("Root");
  pRoot->m_LocalTransform.m_vPosition.x = 1;
  auto publish                          = [&]() {
    xiiSkeletonResourceDescriptor descriptor;
    XII_TEST_RESULT(authoring.FillResourceDescriptor(descriptor));
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     writer(&loader->m_CustomData);
    XII_TEST_RESULT(descriptor.Serialize(writer));
    xiiResourceManager::UpdateResourceWithCustomLoader(hSkeleton, std::move(loader));
    xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
  };
  publish();
  xiiSkeletonContext first;
  xiiSkeletonContext second;
  first.Initialize(guid, {}, nullptr, "Skeleton");
  second.Initialize(guid, {}, nullptr, "Skeleton");
  XII_SCOPE_EXIT(second.Deinitialize(); first.Deinitialize());
  XII_TEST_BOOL(first.GetSkeleton() == second.GetSkeleton());
  XII_TEST_BOOL(first.GetWorld() != second.GetWorld());
  {
    XII_LOCK(first.GetWorld()->GetWriteMarker());
    XII_LOCK(second.GetWorld()->GetWriteMarker());
    first.GetWorld()->SetWorldSimulationEnabled(false);
    second.GetWorld()->SetWorldSimulationEnabled(false);
    xiiView* firstView  = nullptr;
    xiiView* secondView = nullptr;
    first.GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("First skeleton document", firstView);
    second.GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Second skeleton document", secondView);
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45, 0.1f, 100);
    firstView->SetCamera(&camera);
    secondView->SetCamera(&camera);
    xiiEditorEngineSkeletonPreviewMsg metadata;
    XII_TEST_RESULT(metadata.SetSkeleton(authoring));
    first.HandleMessage(&metadata);
    second.HandleMessage(&metadata);
    XII_TEST_RESULT(first.DrawSkeleton(*firstView));
    XII_TEST_RESULT(second.DrawSkeleton(*secondView));
    XII_TEST_BOOL(first.DrawSkeleton(*secondView).Failed());

    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), false);
    geometry.TriangulatePolygons();
    xiiMeshResourceDescriptor mesh;
    mesh.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    mesh.AddSubMesh(12, 0, 0);
    mesh.AddMaterialSlot("Editor/Materials/SkeletonPreviewMesh.xiiMaterial");
    mesh.ComputeBounds();
    auto                               hMesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorSkeletonContextMesh", std::move(mesh));
    xiiSimpleDocumentConfigMsgToEngine config;
    config.m_sWhatToDo = "PreviewMesh";
    config.m_sPayload  = hMesh.GetResourceID();
    first.HandleMessage(&config);
    XII_TEST_RESULT(first.DrawSkeleton(*firstView));
    auto findMesh = [](xiiWorld& world) -> xiiSkinnedMeshComponent* {
      for (auto object = world.GetObjects(); object.IsValid(); ++object)
      {
        xiiSkinnedMeshComponent* component = nullptr;
        if (object->TryGetComponentOfBaseType(component))
          return component;
      }
      return nullptr;
    };
    auto* component = findMesh(*first.GetWorld());
    if (!XII_TEST_BOOL(component != nullptr))
      return;
    XII_TEST_BOOL(findMesh(*second.GetWorld()) == nullptr);
    XII_TEST_BOOL(component->GetSkeleton() == hSkeleton);
    XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 1);
    const auto previousChange = [&]() {
      xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      return loaded->GetCurrentResourceChangeCounter();
    }();
    pRoot->m_LocalTransform.m_vPosition.x = 2;
    publish();
    {
      xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(loaded->GetCurrentResourceChangeCounter() != previousChange);
    }
    XII_TEST_RESULT(first.DrawSkeleton(*firstView));
    XII_TEST_RESULT(second.DrawSkeleton(*secondView));
    XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 1);
    // Topology reload requires matching new metadata, rather than using stale authored collider attachments.
    pRoot->SetName("Renamed");
    publish();
    XII_TEST_BOOL(first.DrawSkeleton(*firstView).Failed());
    XII_TEST_BOOL(component->GetSkinningMatrices().IsEmpty());
    XII_TEST_RESULT(metadata.SetSkeleton(authoring));
    first.HandleMessage(&metadata);
    XII_TEST_RESULT(first.DrawSkeleton(*firstView));
    XII_TEST_BOOL(second.DrawSkeleton(*secondView).Failed());
    second.HandleMessage(&metadata);
    XII_TEST_RESULT(second.DrawSkeleton(*secondView));
    config.m_sPayload.Clear();
    first.HandleMessage(&config);
    XII_TEST_BOOL(findMesh(*first.GetWorld()) == nullptr);
    XII_TEST_BOOL(!first.GetWorld()->GetWorldSimulationEnabled() && !second.GetWorld()->GetWorldSimulationEnabled());
    // Retain a selected mesh so reset must clear the old world's component and resource caches.
    config.m_sPayload = hMesh.GetResourceID();
    first.HandleMessage(&config);
    XII_TEST_RESULT(first.DrawSkeleton(*firstView));
  }

  first.Reset();
  XII_TEST_BOOL(first.GetSkeleton() == hSkeleton);
  XII_LOCK(first.GetWorld()->GetWriteMarker());
  xiiView* resetView = nullptr;
  first.GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Reset skeleton document", resetView);
  xiiCamera resetCamera;
  resetCamera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45, 0.1f, 100);
  resetView->SetCamera(&resetCamera);
  XII_TEST_RESULT(first.DrawSkeleton(*resetView));
  for (auto object = first.GetWorld()->GetObjects(); object.IsValid(); ++object)
  {
    xiiSkinnedMeshComponent* component = nullptr;
    XII_TEST_BOOL(!object->TryGetComponentOfBaseType(component));
  }
}
