/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/TagRegistry.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneInstanceSelectionPass.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>

namespace
{
  void TestNativeMeshCapture(bool enableMeshShaders, bool selectionOnly = false)
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize(enableMeshShaders)))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    const auto                                 surface = xiiLoadPipelineLibraryResource("EditorMeshPicking");
    xiiResourceLock<xiiRenderPipelineResource> loaded(surface, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
      return;
    xiiRenderPipelineResourceDescriptor definition;
    definition.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, surface.GetResourceID()});
    definition.AddSubgraph("Authored surface", loaded->GetAuthoring()->m_Descriptor.m_Id);
    const auto second = definition.AddPass("Second capture", xiiSceneMeshPickingPass().GetDynamicRTTI());
    XII_TEST_RESULT(definition.Connect(0, "Surface", second, "Surface"));
    if (selectionOnly)
    {
      auto& property = definition.m_Passes[0].m_Overrides.ExpandAndGetRef();
      for (const auto& node : loaded->GetAuthoring()->m_Descriptor.m_Passes)
        if (node.m_pType == xiiSceneInstanceSelectionPass().GetDynamicRTTI())
          property.m_Path.PushBack(node.m_Id);
      XII_TEST_INT(property.m_Path.GetCount(), 1);
      property.m_Properties.Insert("SelectionOnly", true);
    }
    const auto  pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(selectionOnly ? "NativeCapture.SelectedOnly" : enableMeshShaders ? "NativeCapture.EnabledMeshDevice" :
                                                                                                                                                            "NativeCapture.DisabledMeshDevice",
                                                                                         std::move(definition));
    xiiGeometry box;
    box.AddBox(xiiVec3(1), true);
    xiiMeshResourceDescriptor meshDescription;
    meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
    meshDescription.AddSection(12, 0, 0);
    meshDescription.ComputeBounds();
    const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>(selectionOnly ? "NativeCapture.SelectedMesh" : enableMeshShaders ? "NativeCapture.MeshA" :
                                                                                                                                                      "NativeCapture.MeshB",
                                                                                   std::move(meshDescription));
    xiiWorldDescription worldDescription("Native surface capture");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    const auto create = [&](xiiVec3 position, xiiGameObject*& object, xiiStaticMeshComponent*& component) {
      xiiGameObjectDescription description;
      description.m_LocalPosition = position;
      description.m_bDynamic      = true;
      const auto handle           = world.CreateObject(description, object);
      xiiStaticMeshComponent::CreateComponent(object, component);
      component->SetMesh(mesh);
      return handle;
    };
    xiiGameObject *         front = nullptr, *back = nullptr;
    xiiStaticMeshComponent *frontMesh = nullptr, *backMesh = nullptr;
    const auto              frontHandle = create(xiiVec3::MakeZero(), front, frontMesh);
    const auto              backHandle  = create({2, 0, 0}, back, backMesh);
    auto*                   renderer    = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiCamera               camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt({-5, 0, 0}, {0, 0, 0}, {0, 0, 1});
    xiiView* views[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      renderer->CreateView("Native capture view", views[i]);
      views[i]->SetCamera(&camera);
      views[i]->SetViewport({float(i * 10), float(i * 20), 64, 64});
      views[i]->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
      views[i]->SetRenderPipelineResource(pipeline);
    }
    const auto setSelected = [&](xiiGameObjectHandle owner) {
      xiiVariantArray objects;
      objects.PushBack(owner.GetInternalID().m_Data);
      for (auto* view : views)
        view->SetPipelineParameter("Editor.SelectedObjects", objects);
    };
    setSelected(frontHandle);
    const auto update = [&](xiiUInt32 frames) {
      for (xiiUInt32 i = 0; i < frames; ++i)
      {
        world.Update();
        environment.GetDevice()->WaitIdle();
      }
    };
    update(6);
    const xiiSceneMeshPickingPass* captures[2][2] = {};
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      const auto& execution = views[view]->GetRenderPipelineInstance();
      const auto* compiled  = execution.GetPipeline();
      if (!XII_TEST_BOOL(compiled != nullptr))
        return;
      xiiUInt32 count = 0;
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
        if (const auto* capture = xiiDynamicCast<const xiiSceneMeshPickingPass*>(execution.GetPass(i)))
        {
          if (!XII_TEST_BOOL(count < 2))
            return;
          captures[view][count++] = capture;
        }
      XII_TEST_INT(count, 2);
      XII_TEST_BOOL(captures[view][0] != captures[view][1]);
    }
    XII_TEST_BOOL(captures[0][0] != captures[1][0]);
    XII_TEST_BOOL(views[0]->GetRenderPipelineInstance().GetPipeline() == views[1]->GetRenderPipelineInstance().GetPipeline());
    const auto check = [&](xiiGameObjectHandle expected) {
      for (xiiUInt32 view = 0; view < 2; ++view)
        for (const auto* capture : captures[view])
        {
          xiiSceneMeshPickingPass::PickingResult hit;
          if (XII_TEST_RESULT(capture->Pick(*views[view], 32 + view * 10, 32 + view * 20, hit)))
          {
            XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == expected);
            if (!expected.IsInvalidated())
            {
              XII_TEST_BOOL(hit.m_vWorldNormal.IsValid() && hit.m_vWorldPosition.IsValid());
              XII_TEST_FLOAT(hit.m_vWorldNormal.GetLength(), 1, 0.001f);
            }
          }
        }
    };
    check(frontHandle);
    if (selectionOnly)
    {
      for (auto* view : views)
        view->SetPipelineParameter("Editor.SelectedObjects", xiiVariantArray());
      update(4);
      check({});
      setSelected(frontHandle);
      update(4);
      check(frontHandle);
    }
    if (!selectionOnly)
    {
      views[0]->SetPipelineParameter("Editor.PickSelected", false);
      update(4);
      xiiSceneMeshPickingPass::PickingResult selectedHit;
      XII_TEST_RESULT(captures[0][0]->Pick(*views[0], 32, 32, selectedHit));
      XII_TEST_BOOL(selectedHit.m_Source.m_hOwnerObject == backHandle);
      XII_TEST_RESULT(captures[1][0]->Pick(*views[1], 42, 52, selectedHit));
      XII_TEST_BOOL(selectedHit.m_Source.m_hOwnerObject == frontHandle);
      xiiMaterialResourceDescriptor translucent;
      translucent.m_AlphaMode   = xiiMaterialAlphaMode::Blend;
      translucent.m_BlendMode   = xiiMaterialBlendMode::Translucent;
      translucent.m_BaseColor.a = 0.7f;
      const auto material       = xiiResourceManager::CreateResource<xiiMaterialResource>(enableMeshShaders ? "NativeCapture.TranslucentA" : "NativeCapture.TranslucentB", std::move(translucent));
      backMesh->SetMaterialOverride(0, material);
      views[0]->SetPipelineParameter("Editor.PickTransparent", false);
      update(5);
      XII_TEST_RESULT(captures[0][0]->Pick(*views[0], 32, 32, selectedHit));
      XII_TEST_BOOL(selectedHit.m_Source.m_hOwnerObject.IsInvalidated());
      views[0]->SetPipelineParameter("Editor.PickTransparent", true);
      update(4);
      XII_TEST_RESULT(captures[0][0]->Pick(*views[0], 32, 32, selectedHit));
      XII_TEST_BOOL(selectedHit.m_Source.m_hOwnerObject == backHandle);
      views[0]->SetPipelineParameter("Editor.PickSelected", true);
      update(4);
      check(frontHandle);
    }

    const auto& hidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("NativeCapture.Hidden");
    front->SetTag(hidden);
    views[0]->m_ExcludeTags.Set(hidden);
    update(1);
    xiiSceneMeshPickingPass::PickingResult tagHit;
    if (captures[0][0]->Pick(*views[0], 32, 32, tagHit).Succeeded())
      XII_TEST_BOOL(tagHit.m_Source.m_hOwnerObject == (selectionOnly ? xiiGameObjectHandle() : backHandle));
    update(4);
    XII_TEST_RESULT(captures[0][0]->Pick(*views[0], 32, 32, tagHit));
    XII_TEST_BOOL(tagHit.m_Source.m_hOwnerObject == (selectionOnly ? xiiGameObjectHandle() : backHandle));
    XII_TEST_RESULT(captures[1][0]->Pick(*views[1], 42, 52, tagHit));
    XII_TEST_BOOL(tagHit.m_Source.m_hOwnerObject == frontHandle);
    views[0]->m_ExcludeTags.Clear();
    update(1);
    if (captures[0][0]->Pick(*views[0], 32, 32, tagHit).Succeeded())
      XII_TEST_BOOL(tagHit.m_Source.m_hOwnerObject == frontHandle);
    update(4);
    check(frontHandle);
    const auto stable = renderer->GetGpuInstanceData()->GetDataRevision();
    update(2);
    XII_TEST_INT(renderer->GetGpuInstanceData()->GetDataRevision(), stable);
    front->SetLocalPosition(xiiVec3(0, 3, 0));
    update(1);
    XII_TEST_BOOL(renderer->GetGpuInstanceData()->GetDataRevision() != stable);
    xiiSceneMeshPickingPass::PickingResult hit;
    if (captures[0][0]->Pick(*views[0], 32, 32, hit).Succeeded())
      XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == (selectionOnly ? xiiGameObjectHandle() : backHandle));
    update(4);
    check(selectionOnly ? xiiGameObjectHandle() : backHandle);
    backMesh->SetMesh({});
    update(4);
    check({});
    world.DeleteObjectNow(backHandle);
    xiiGameObject*          replacement       = nullptr;
    xiiStaticMeshComponent* replacementMesh   = nullptr;
    const auto              replacementHandle = create(xiiVec3::MakeZero(), replacement, replacementMesh);
    XII_TEST_BOOL(replacementHandle != backHandle);
    setSelected(replacementHandle);
    update(5);
    check(replacementHandle);
    views[0]->InvalidateTemporalHistory();
    XII_TEST_BOOL(captures[0][0]->Pick(*views[0], 32, 32, hit).Failed());
    XII_TEST_RESULT(captures[1][0]->Pick(*views[1], 42, 52, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == replacementHandle);
    update(4);
    views[0]->SetPipelineParameter("Editor.Picking", false);
    update(1);
    XII_TEST_BOOL(captures[0][0]->Pick(*views[0], 32, 32, hit).Failed());
    XII_TEST_RESULT(captures[1][0]->Pick(*views[1], 42, 52, hit));
    views[0]->SetPipelineParameter("Editor.Picking", true);
    update(4);
    check(replacementHandle);
    xiiDynamicArray<xiiGameObjectHandle> picked;
    XII_TEST_RESULT(captures[0][0]->PickObjectsInRect(*views[0], {0, 0, 64, 64}, picked));
    XII_TEST_BOOL(picked.Contains(replacementHandle));
    XII_TEST_BOOL(!picked.Contains(backHandle));
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeMeshCapture);
XII_CREATE_SIMPLE_TEST(NativeMeshCapture, WorldLifetimeAndIndependentViews) { TestNativeMeshCapture(true); }
XII_CREATE_SIMPLE_TEST(NativeMeshCapture, WithoutMeshShaders) { TestNativeMeshCapture(false); }

XII_CREATE_SIMPLE_TEST(NativeMeshCapture, SelectionOnlyComposition) { TestNativeMeshCapture(true, true); }
