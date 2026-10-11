/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/Animation/AnimationPreview.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

XII_CREATE_SIMPLE_TEST(EditorViews, AnimationPreview)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiSkeletonResourceDescriptor skeletonDescriptor;
  auto                          rest = xiiTransform::MakeIdentity();
  rest.m_vPosition.x                 = 1;
  skeletonDescriptor.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), rest);
  rest.m_vPosition.x = 2;
  skeletonDescriptor.AddJoint("Child", 0, rest);
  skeletonDescriptor.BuildModelSpaceRestPose();
  auto                                 hSkeleton = xiiResourceManager::CreateResource<xiiSkeletonResource>("EditorAnimationPreviewSkeleton", std::move(skeletonDescriptor));
  xiiResourceLock<xiiSkeletonResource> skeleton(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  xiiAnimationClipResourceDescriptor clipDescriptor;
  clipDescriptor.m_Duration = xiiTime::MakeFromSeconds(2);
  clipDescriptor.m_bLooping = false;
  auto& track               = clipDescriptor.AddJointTrack("Child");
  track.m_PositionKeys.PushBack({0, xiiVec3(2, 0, 0)});
  track.m_PositionKeys.PushBack({2, xiiVec3(4, 0, 0)});
  auto                                      hClip = xiiResourceManager::CreateResource<xiiAnimationClipResource>("EditorAnimationPreviewClip", std::move(clipDescriptor));
  xiiResourceLock<xiiAnimationClipResource> clip(hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  xiiMeshResourceDescriptor mesh;
  mesh.m_hDefaultSkeleton = hSkeleton;
  xiiHashedString child, root;
  child.Assign("Child");
  root.Assign("Root");
  xiiMeshBoneData childBone, rootBone;
  childBone.m_uiBoneIndex                 = 0;
  childBone.m_GlobalInverseRestPoseMatrix = xiiMat4::MakeTranslation(xiiVec3(-3, 0, 0));
  rootBone.m_uiBoneIndex                  = 2;
  rootBone.m_GlobalInverseRestPoseMatrix  = xiiMat4::MakeTranslation(xiiVec3(-1, 0, 0));
  mesh.m_Bones.Insert(child, childBone);
  mesh.m_Bones.Insert(root, rootBone);
  xiiAnimationPreview preview;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rest pose, reordered mesh bones, sparse slots, and exact scrubbing")
  {
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh));
    XII_TEST_INT(preview.GetSkinningMatrices().GetCount(), 3);
    for (const auto& matrix : preview.GetSkinningMatrices())
      XII_TEST_BOOL(matrix.IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh, clip.GetPointer(), 0.25));
    XII_TEST_VEC3(preview.GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(0.5f, 0, 0), 0.0001f);
    XII_TEST_VEC3(preview.GetSkinningMatrices()[2].GetTranslationVector(), xiiVec3::MakeZero(), 0.0001f);
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh, clip.GetPointer(), 2));
    XII_TEST_VEC3(preview.GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(2, 0, 0), 0.0001f);
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh, clip.GetPointer(), -1));
    XII_TEST_VEC3(preview.GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3::MakeZero(), 0.0001f);
    xiiMeshResourceDescriptor jointOrderedMesh;
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), jointOrderedMesh, clip.GetPointer(), 0.25));
    XII_TEST_INT(preview.GetSkinningMatrices().GetCount(), 2);
    XII_TEST_VEC3(preview.GetSkinningMatrices()[1].GetTranslationVector(), xiiVec3(0.5f, 0, 0), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Additive clips sample neutral missing joints and preview over rest pose")
  {
    xiiAnimationClipResourceDescriptor additive;
    additive.m_Duration = xiiTime::MakeFromSeconds(2);
    additive.m_bAdditive = true;
    additive.m_bLooping = false;
    auto& delta = additive.AddJointTrack("Child");
    delta.m_PositionKeys.PushBack({0, xiiVec3::MakeZero()});
    delta.m_PositionKeys.PushBack({2, xiiVec3(2, 0, 0)});
    const auto handle = xiiResourceManager::CreateResource<xiiAnimationClipResource>("EditorAdditivePreviewClip", std::move(additive));
    xiiResourceLock<xiiAnimationClipResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource.GetPointer() != nullptr))
      return;
    xiiAnimationPose sampled;
    resource->SampleLocalPose(*skeleton.GetPointer(), xiiTime::MakeFromSeconds(1), sampled);
    XII_TEST_BOOL(sampled.m_LocalTransforms[0].IsEqual(xiiTransform::MakeIdentity(), 0.0001f));
    XII_TEST_VEC3(sampled.m_LocalTransforms[1].m_vPosition, xiiVec3(1, 0, 0), 0.0001f);
    XII_TEST_VEC3(sampled.m_LocalTransforms[1].m_vScale, xiiVec3(1), 0.0001f);
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh, resource.GetPointer(), 0.5));
    XII_TEST_VEC3(preview.GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(1, 0, 0), 0.0001f);
    XII_TEST_BOOL(preview.GetSkinningMatrices()[2].IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid mappings and times clear stale output")
  {
    rootBone.m_uiBoneIndex = 0;
    mesh.m_Bones.Insert(root, rootBone);
    XII_TEST_BOOL(preview.Build(*skeleton.GetPointer(), mesh).Failed());
    XII_TEST_BOOL(preview.GetSkinningMatrices().IsEmpty());
    XII_TEST_BOOL(preview.GetPose().m_ModelTransforms.IsEmpty());
    rootBone.m_uiBoneIndex = 2;
    mesh.m_Bones.Insert(root, rootBone);
    XII_TEST_BOOL(preview.Build(*skeleton.GetPointer(), mesh, nullptr, xiiMath::Infinity<double>()).Failed());
    xiiHashedString missing;
    missing.Assign("Missing");
    mesh.m_Bones.Insert(missing, childBone);
    XII_TEST_BOOL(preview.Build(*skeleton.GetPointer(), mesh).Failed());
    mesh.m_Bones.Remove(missing);
    rootBone.m_uiBoneIndex = xiiMath::MaxValue<xiiUInt16>();
    mesh.m_Bones.Insert(root, rootBone);
    XII_TEST_BOOL(preview.Build(*skeleton.GetPointer(), mesh).Failed());
    rootBone.m_uiBoneIndex = 2;
    mesh.m_Bones.Insert(root, rootBone);
    XII_TEST_RESULT(preview.Build(*skeleton.GetPointer(), mesh));
    rootBone.m_GlobalInverseRestPoseMatrix.Element(0, 0) = xiiMath::Infinity<float>();
    mesh.m_Bones.Insert(root, rootBone);
    XII_TEST_BOOL(preview.Build(*skeleton.GetPointer(), mesh).Failed());
    XII_TEST_BOOL(preview.GetSkinningMatrices().IsEmpty());
    rootBone.m_GlobalInverseRestPoseMatrix = xiiMat4::MakeTranslation(xiiVec3(-1, 0, 0));
    mesh.m_Bones.Insert(root, rootBone);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component palette application without world simulation")
  {
    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), true);
    mesh.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    mesh.AddSubMesh(mesh.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
    mesh.ComputeBounds();
    auto                hMesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorAnimationPreviewMesh", std::move(mesh));
    xiiWorldDescription description("Editor animation preview test");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic = true;
    xiiGameObject* object        = nullptr;
    world.CreateObject(objectDescription, object);
    xiiSkinnedMeshComponent* component = nullptr;
    xiiSkinnedMeshComponent::CreateComponent(object, component);
    component->SetMesh(hMesh);
    XII_TEST_RESULT(preview.UpdateMesh(*component, hClip, 0.5));
    XII_TEST_BOOL(component->GetSkeleton() == hSkeleton);
    XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 3);
    XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(1, 0, 0), 0.0001f);
    XII_TEST_BOOL(!world.GetWorldSimulationEnabled());

    auto hReloadableClip = xiiResourceManager::LoadResource<xiiAnimationClipResource>("EditorAnimationPreviewReloadableClip");
    auto PublishClip     = [&](float endPosition) {
      xiiAnimationClipResourceDescriptor replacement;
      replacement.m_Duration = xiiTime::MakeFromSeconds(2);
      replacement.m_bLooping = false;
      auto& replacementTrack = replacement.AddJointTrack("Child");
      replacementTrack.m_PositionKeys.PushBack({0, xiiVec3(2, 0, 0)});
      replacementTrack.m_PositionKeys.PushBack({2, xiiVec3(endPosition, 0, 0)});
      xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
      xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
      XII_TEST_RESULT(replacement.Serialize(stream));
      xiiResourceManager::UpdateResourceWithCustomLoader(hReloadableClip, std::move(loader));
      xiiResourceLock<xiiAnimationClipResource> loaded(hReloadableClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
    };
    PublishClip(4);
    XII_TEST_RESULT(preview.UpdateMesh(*component, hReloadableClip, 0.5));
    XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(1, 0, 0), 0.0001f);
    PublishClip(6);
    XII_TEST_RESULT(preview.UpdateMesh(*component, hReloadableClip, 0.5));
    XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(2, 0, 0), 0.0001f);

    xiiMeshResourceDescriptor bindMesh;
    {
      xiiResourceLock<xiiMeshResource> original(hMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      bindMesh.UseExistingMeshBuffer(original->GetMeshBuffer());
      bindMesh.AddSubMesh(12, 0, 0);
      bindMesh.ComputeBounds();
    }
    auto hBindMesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorAnimationPreviewBindMesh", std::move(bindMesh));
    component->SetMesh(hBindMesh);
    XII_TEST_RESULT(preview.UpdateMesh(*component));
    XII_TEST_BOOL(!component->GetSkeleton().IsValid());
    XII_TEST_BOOL(component->GetSkinningMatrices().IsEmpty());
    XII_TEST_RESULT(preview.UpdateMesh(*component, {}, 0.0, hSkeleton));
    XII_TEST_BOOL(component->GetSkeleton() == hSkeleton);
    XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 2);
    XII_TEST_BOOL(preview.UpdateMesh(*component, hClip, 0.5).Failed());
    component->SetMesh({});
    XII_TEST_BOOL(preview.UpdateMesh(*component, hClip, 0.5).Failed());
    XII_TEST_BOOL(component->GetSkinningMatrices().IsEmpty());
    XII_TEST_BOOL(preview.GetSkinningMatrices().IsEmpty());
  }
}
