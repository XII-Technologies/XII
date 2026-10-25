/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/Animation/AnimationPreview.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <ModelImporter/AnimationImport.h>
#include <ModelImporter/MeshImport.h>
#include <ModelImporter/SkeletonImport.h>

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
  clipDescriptor.m_bLooping = true;
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

  const auto source = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-importModel");
  if (!source.IsEmpty())
  {
    xiiModelSkeletonImport    sourceSkeleton;
    xiiModelMeshImport        sourceMesh;
    xiiModelMeshImportOptions options;
    options.m_bImportSkinning = true;
    xiiStringBuilder error;
    const auto       skeletonResult = xiiModelSkeletonImport::ReadFile(source, sourceSkeleton, &error);
    if (!XII_TEST_RESULT_MSG(skeletonResult, "%s", error.GetData()))
      return;
    const auto meshResult = xiiModelMeshImport::ReadFile(source, options, sourceMesh, &error);
    if (!XII_TEST_RESULT_MSG(meshResult, "%s", error.GetData()))
      return;
    const auto                           sourceHandle = xiiResourceManager::CreateResource<xiiSkeletonResource>("ExternalAnimationPreviewSkeleton", std::move(sourceSkeleton.m_Skeleton));
    xiiResourceLock<xiiSkeletonResource> sourceResource(sourceHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiDynamicArray<xiiString>           names;
    XII_TEST_RESULT(xiiModelAnimationImport::ReadClipNames(source, names, &error));
    for (const auto& name : names)
    {
      xiiModelAnimationImportOptions clipOptions;
      clipOptions.m_sClipName = name;
      xiiModelAnimationImport sourceClip;
      const auto              result = xiiModelAnimationImport::ReadFile(source, clipOptions, sourceClip, &error);
      if (!XII_TEST_RESULT_MSG(result, "%s", error.GetData()))
        return;
      const auto                                clipHandle = xiiResourceManager::CreateResource<xiiAnimationClipResource>(xiiStringBuilder("ExternalAnimationPreview/", name), std::move(sourceClip.m_Clip));
      xiiResourceLock<xiiAnimationClipResource> clipResource(clipHandle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_RESULT(preview.Build(*sourceResource.GetPointer(), sourceMesh.m_Mesh, clipResource.GetPointer(), 0));
      xiiDynamicArray<xiiMat4> firstPose;
      firstPose.PushBackRange(preview.GetSkinningMatrices());
      XII_TEST_RESULT(preview.Build(*sourceResource.GetPointer(), sourceMesh.m_Mesh, clipResource.GetPointer(), 0.5));
      bool        moved  = false;
      const auto& buffer = sourceMesh.m_Mesh.GetMeshBufferDescriptor();
      for (xiiUInt32 i = 0; i < buffer.GetVertexCount(); ++i)
      {
        xiiMeshPackedVertex vertex;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&vertex), buffer.GetVertexData().GetPtr() + i * sizeof(vertex), sizeof(vertex));
        xiiVec3 first = xiiVec3::MakeZero(), second = xiiVec3::MakeZero();
        for (xiiUInt32 influence = 0; influence < 4; ++influence)
        {
          const auto bone   = vertex.m_vBoneIndices0.GetData()[influence];
          const auto weight = vertex.m_BoneWeights0.GetData()[influence] / 255.0f;
          first += firstPose[bone].TransformPosition(vertex.m_vPosition) * weight;
          second += preview.GetSkinningMatrices()[bone].TransformPosition(vertex.m_vPosition) * weight;
        }
        moved |= !first.IsEqual(second, 0.00001f);
      }
      XII_TEST_BOOL_MSG(moved, "Clip '%s' must deform source mesh vertices.", name.GetData());
    }
    return;
  }

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
    xiiAnimationPose playback;
    clip->SampleLocalPose(*skeleton.GetPointer(), clip->GetDuration(), playback);
    XII_TEST_VEC3(playback.m_LocalTransforms[1].m_vPosition, xiiVec3(2, 0, 0), 0.0001f); // Runtime still loops.
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
    additive.m_Duration  = xiiTime::MakeFromSeconds(2);
    additive.m_bAdditive = true;
    additive.m_bLooping  = false;
    auto& delta          = additive.AddJointTrack("Child");
    delta.m_PositionKeys.PushBack({0, xiiVec3::MakeZero()});
    delta.m_PositionKeys.PushBack({2, xiiVec3(2, 0, 0)});
    const auto                                handle = xiiResourceManager::CreateResource<xiiAnimationClipResource>("EditorAdditivePreviewClip", std::move(additive));
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

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pose mutations refresh world bounds without simulation")
    {
      world.Update();
      XII_TEST_BOOL(object->GetLocalBounds().GetBox().Contains(xiiVec3(1.5f, 0, 0)));
      const xiiMat4 matrices[] = {xiiMat4::MakeTranslation(xiiVec3(10, 0, 0)), xiiMat4::MakeTranslation(xiiVec3(30, 0, 0))};
      XII_TEST_RESULT(component->SetSkinningMatrices(matrices));
      world.Update();
      XII_TEST_BOOL(object->GetLocalBounds().GetBox().Contains(xiiVec3(30.5f, 0, 0)));
      XII_TEST_BOOL(object->GetLocalBounds().GetBox().Contains(xiiVec3(-0.5f, 0, 0)));
      auto invalid          = xiiMat4::MakeIdentity();
      invalid.Element(0, 3) = 1;
      XII_TEST_BOOL(component->SetSkinningMatrices(xiiMakeArrayPtr(&invalid, 1)).Failed());
      XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 2);
      XII_TEST_VEC3(component->GetSkinningMatrices()[1].GetTranslationVector(), xiiVec3(30, 0, 0), 0);
      XII_TEST_RESULT(component->SetSkinningMatrices(component->GetSkinningMatrices().GetSubArray(1, 1)));
      XII_TEST_INT(component->GetSkinningMatrices().GetCount(), 1);
      XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(30, 0, 0), 0);
      component->ClearPose();
      world.Update();
      XII_TEST_VEC3(object->GetLocalBounds().GetBox().m_vMax, xiiVec3(0.5f), 0.0001f);
      XII_TEST_RESULT(component->SetSkinningMatrices(matrices));
      XII_TEST_BOOL(component->SetAnimationPose({}, preview.GetPose()).Failed());
      world.Update();
      XII_TEST_BOOL(component->GetSkinningMatrices().IsEmpty());
      XII_TEST_VEC3(object->GetLocalBounds().GetBox().m_vMax, xiiVec3(0.5f), 0.0001f);
      XII_TEST_RESULT(preview.UpdateMesh(*component, hClip, 0.5));
      world.Update();
      XII_TEST_BOOL(object->GetLocalBounds().GetBox().Contains(xiiVec3(1.5f, 0, 0)));
    }

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
      // A custom payload may be queued behind the previous loader's closing
      // task. A Final acquisition can still contain that previous payload.
      const auto deadline  = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
      bool       published = false;
      do
      {
        {
          xiiResourceLock<xiiAnimationClipResource> loaded(hReloadableClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
          if (loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final)
          {
            const auto& tracks = loaded->GetDescriptor().m_JointTracks;
            published          = tracks.GetCount() == 1 && !tracks[0].m_PositionKeys.IsEmpty() && tracks[0].m_PositionKeys.PeekBack().m_Value.x == endPosition;
          }
        }
        if (!published)
          xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
      } while (!published && xiiTime::Now() < deadline);
      XII_TEST_BOOL(published);
    };
    PublishClip(4);
    XII_TEST_RESULT(preview.UpdateMesh(*component, hReloadableClip, 0.5));
    XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(1, 0, 0), 0.0001f);
    PublishClip(6);
    XII_TEST_RESULT(preview.UpdateMesh(*component, hReloadableClip, 0.5));
    XII_TEST_VEC3(component->GetSkinningMatrices()[0].GetTranslationVector(), xiiVec3(2, 0, 0), 0.0001f);
    world.Update();
    XII_TEST_BOOL(object->GetLocalBounds().GetBox().Contains(xiiVec3(2.5f, 0, 0)));

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
