/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Common/Messages.h>
#include <SharedPluginScene/Scene/SceneSkeletonPose.h>

namespace
{
  using LegacyPoseTestManager = xiiComponentManager<class LegacyPoseTestComponent, xiiBlockStorageType::Compact>;
  class LegacyPoseTestComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(LegacyPoseTestComponent, xiiComponent, LegacyPoseTestManager);

  public:
    xiiSkeletonResourceHandle m_hSkeleton;
    void                      SerializeComponent(xiiWorldWriter& writer) const override
    {
      SUPER::SerializeComponent(writer);
      auto& stream = writer.GetStream();
      stream << m_hSkeleton;
      stream << false;
      stream << xiiUInt32(3);
      auto transform = xiiTransform::MakeIdentity();
      for (xiiUInt32 i = 0; i < 3; ++i)
      {
        transform.m_vPosition.x = static_cast<float>(10 + i);
        stream << transform;
      }
    }
  };
  XII_BEGIN_COMPONENT_TYPE(LegacyPoseTestComponent, 1, xiiComponentMode::Dynamic)
  XII_END_COMPONENT_TYPE;
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SkeletonPose)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  auto hSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>("EditorNativePoseSkeleton");
  auto publish   = [&](bool reordered) {
    xiiSkeletonResourceDescriptor descriptor;
    auto                          rest = xiiTransform::MakeIdentity();
    rest.m_vPosition.x                 = 1;
    descriptor.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), rest);
    rest.m_vPosition.x = 2;
    descriptor.AddJoint(reordered ? "Other" : "Child", 0, rest);
    rest.m_vPosition.x = 3;
    descriptor.AddJoint(reordered ? "Child" : "Other", 0, rest);
    descriptor.BuildModelSpaceRestPose();
    descriptor.ComputeRuntimeHash();
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     writer(&loader->m_CustomData);
    XII_TEST_RESULT(descriptor.Serialize(writer));
    xiiResourceManager::UpdateResourceWithCustomLoader(hSkeleton, std::move(loader));
    xiiResourceLock<xiiSkeletonResource> loaded(hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(loaded && loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
  };
  publish(false);

  xiiWorldDescription worldDescription("Native pose authoring");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic      = true;
  xiiGameObject*            object  = nullptr;
  const auto                hObject = world.CreateObject(objectDescription, object);
  xiiSkeletonPoseComponent* pose    = nullptr;
  const auto                hPose   = xiiSkeletonPoseComponent::CreateComponent(object, pose);
  pose->SetSkeleton(hSkeleton);
  xiiMap<xiiString, xiiTransform> snapshot;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Named overrides evaluate without simulation and removal restores rest pose")
  {
    XII_TEST_BOOL(pose->CopyCurrentLocalPose(snapshot).Failed());
    auto transform          = xiiTransform::MakeIdentity();
    transform.m_vPosition.x = 7;
    transform.m_vScale      = xiiVec3(2, 3, 4);
    XII_TEST_RESULT(pose->SetBoneTransform("Child", transform));
    XII_TEST_RESULT(pose->SetBoneTransform("FutureJoint", transform));
    world.Update();
    XII_TEST_BOOL(world.TryGetObject(hObject, object));
    XII_TEST_BOOL(world.TryGetComponent(hPose, pose));
    XII_TEST_RESULT(pose->CopyCurrentLocalPose(snapshot));
    XII_TEST_INT(snapshot.GetCount(), 3);
    XII_TEST_BOOL(snapshot["Child"].IsEqual(transform, 0.0001f));
    XII_TEST_VEC3(pose->GetCurrentPose().m_ModelTransforms[1].GetTranslationVector(), xiiVec3(8, 0, 0), 0.0001f);
    XII_TEST_INT(pose->GetBoneTransforms().GetCount(), 2);

    pose->RemoveBoneTransform("Child");
    XII_TEST_BOOL(pose->CopyCurrentLocalPose(snapshot).Failed());
    XII_TEST_BOOL(snapshot.IsEmpty());
    world.Update();
    XII_TEST_BOOL(world.TryGetComponent(hPose, pose));
    XII_TEST_RESULT(pose->CopyCurrentLocalPose(snapshot));
    XII_TEST_VEC3(snapshot["Child"].m_vPosition, xiiVec3(2, 0, 0), 0.0001f);
    XII_TEST_VEC3(snapshot["Child"].m_vScale, xiiVec3(1), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid input is rejected atomically and full poses bind to names")
  {
    auto transform = xiiTransform::MakeIdentity();
    XII_TEST_BOOL(pose->SetBoneTransform("", transform).Failed());
    transform.m_vPosition.x = xiiMath::NaN<float>();
    XII_TEST_BOOL(pose->SetBoneTransform("Child", transform).Failed());
    xiiTransform locals[3]  = {xiiTransform::MakeIdentity(), xiiTransform::MakeIdentity(), xiiTransform::MakeIdentity()};
    locals[1].m_vPosition.x = 8;
    XII_TEST_BOOL(pose->SetLocalPose(xiiMakeArrayPtr(locals, 2)).Failed());
    XII_TEST_INT(pose->GetBoneTransforms().GetCount(), 1);
    XII_TEST_RESULT(pose->SetLocalPose(locals));
    XII_TEST_INT(pose->GetBoneTransforms().GetCount(), 3);
    pose->Update();
    XII_TEST_RESULT(pose->CopyCurrentLocalPose(snapshot));
    XII_TEST_FLOAT(snapshot["Child"].m_vPosition.x, 8, 0);

    publish(true);
    XII_TEST_BOOL(pose->CopyCurrentLocalPose(snapshot).Failed());
    XII_TEST_BOOL(snapshot.IsEmpty());
    pose->Update();
    XII_TEST_RESULT(pose->CopyCurrentLocalPose(snapshot));
    XII_TEST_FLOAT(snapshot["Child"].m_vPosition.x, 8, 0);
    XII_TEST_FLOAT(pose->GetCurrentPose().m_LocalTransforms[2].m_vPosition.x, 8, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "World serialization preserves authored overrides and mesh application settings")
  {
    pose->SetApplyToOwnerMesh(false);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         stream(&storage);
    xiiWorldWriter                writer;
    writer.WriteWorld(stream, world);
    xiiMemoryStreamReader input(&storage);
    xiiWorldReader        reader;
    XII_TEST_RESULT(reader.ReadWorldDescription(input));
    xiiWorldDescription restoredDescription("Restored pose");
    xiiWorld            restored(restoredDescription);
    XII_LOCK(restored.GetWriteMarker());
    restored.SetWorldSimulationEnabled(false);
    reader.InstantiateWorld(restored);
    auto objects = restored.GetObjects();
    if (!XII_TEST_BOOL(objects.IsValid()))
      return;
    xiiSkeletonPoseComponent* loaded = nullptr;
    if (!XII_TEST_BOOL(objects->TryGetComponentOfBaseType(loaded)))
      return;
    XII_TEST_BOOL(!loaded->GetApplyToOwnerMesh());
    XII_TEST_BOOL(loaded->GetSkeleton() == hSkeleton);
    XII_TEST_INT(loaded->GetBoneTransforms().GetCount(), 3);
    XII_TEST_BOOL(loaded->CopyCurrentLocalPose(snapshot).Failed());
    loaded->Update();
    XII_TEST_RESULT(loaded->CopyCurrentLocalPose(snapshot));
    XII_TEST_FLOAT(snapshot["Child"].m_vPosition.x, 8, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Animated pose snapshots reject stale skeleton revisions")
  {
    xiiSkeletonComponent* animated = nullptr;
    xiiSkeletonComponent::CreateComponent(object, animated);
    animated->SetApplyToOwnerMesh(false);
    animated->SetSkeleton(hSkeleton);
    XII_TEST_BOOL(animated->CopyCurrentLocalPose(snapshot).Failed());
    animated->Update();
    XII_TEST_RESULT(animated->CopyCurrentLocalPose(snapshot));
    publish(false);
    XII_TEST_BOOL(animated->CopyCurrentLocalPose(snapshot).Failed());
    animated->Update();
    XII_TEST_RESULT(animated->CopyCurrentLocalPose(snapshot));
    XII_TEST_FLOAT(snapshot["Child"].m_vPosition.x, 2, 0);
    animated->SetSkeleton({});
    XII_TEST_BOOL(animated->CopyCurrentLocalPose(snapshot).Failed());
    XII_TEST_BOOL(snapshot.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Version 1 indexed poses load and resolve into named overrides")
  {
    xiiWorldDescription legacyDescription("Version 1 pose fixture");
    xiiWorld            legacy(legacyDescription);
    XII_LOCK(legacy.GetWriteMarker());
    xiiGameObject* legacyObject = nullptr;
    legacy.CreateObject(objectDescription, legacyObject);
    LegacyPoseTestComponent* legacyPose = nullptr;
    LegacyPoseTestComponent::CreateComponent(legacyObject, legacyPose);
    legacyPose->m_hSkeleton = hSkeleton;
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         stream(&storage);
    xiiWorldWriter                writer;
    writer.WriteWorld(stream, legacy);
    const auto previousLookup = xiiWorldReader::s_FindComponentTypeCallback;
    XII_SCOPE_EXIT(xiiWorldReader::s_FindComponentTypeCallback = previousLookup);
    xiiWorldReader::s_FindComponentTypeCallback = [](xiiStringView name) -> const xiiRTTI* {
      return name == "LegacyPoseTestComponent" ? xiiGetStaticRTTI<xiiSkeletonPoseComponent>() : xiiRTTI::FindTypeByName(name);
    };
    xiiMemoryStreamReader input(&storage);
    xiiWorldReader        reader;
    XII_TEST_RESULT(reader.ReadWorldDescription(input));
    xiiWorldDescription restoredDescription("Version 1 restored pose");
    xiiWorld            restored(restoredDescription);
    XII_LOCK(restored.GetWriteMarker());
    reader.InstantiateWorld(restored);
    auto objects = restored.GetObjects();
    if (!XII_TEST_BOOL(objects.IsValid()))
      return;
    xiiSkeletonPoseComponent* loaded = nullptr;
    if (!XII_TEST_BOOL(objects->TryGetComponentOfBaseType(loaded)))
      return;
    XII_TEST_BOOL(!loaded->GetApplyToOwnerMesh());
    loaded->Update();
    XII_TEST_INT(loaded->GetBoneTransforms().GetCount(), 3);
    XII_TEST_RESULT(loaded->CopyCurrentLocalPose(snapshot));
    XII_TEST_FLOAT(snapshot["Child"].m_vPosition.x, 11, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scene pullback and IPC retain skeleton identity and native transforms")
  {
    // Remove the now-invalid animated producer so the object's authored pose is used.
    xiiSkeletonComponent* animated = nullptr;
    if (object->TryGetComponentOfBaseType(animated))
      animated->GetOwningManager()->DeleteComponent(animated->GetHandle());
    pose->Update();
    xiiPushObjectStateMsgToEditor message;
    auto&                         state = message.m_ObjectStates.ExpandAndGetRef();
    XII_TEST_RESULT(xiiCollectSceneSkeletonPose(*object, state.m_sSkeletonResource, state.m_BoneTransforms));
    XII_TEST_STRING(state.m_sSkeletonResource, hSkeleton.GetResourceID());
    XII_TEST_FLOAT(state.m_BoneTransforms["Child"].m_vPosition.x, 8, 0);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, message.GetDynamicRTTI(), &message);
    xiiMemoryStreamReader         reader(&storage);
    xiiPushObjectStateMsgToEditor decoded;
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *decoded.GetDynamicRTTI(), &decoded);
    XII_TEST_INT(decoded.m_ObjectStates.GetCount(), 1);
    XII_TEST_STRING(decoded.m_ObjectStates[0].m_sSkeletonResource, hSkeleton.GetResourceID());
    XII_TEST_BOOL(decoded.m_ObjectStates[0].m_BoneTransforms["Child"].IsEqual(state.m_BoneTransforms["Child"], 0));
    pose->SetSkeleton({});
    XII_TEST_BOOL(xiiCollectSceneSkeletonPose(*object, state.m_sSkeletonResource, state.m_BoneTransforms).Failed());
    XII_TEST_BOOL(state.m_sSkeletonResource.IsEmpty() && state.m_BoneTransforms.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native pose application resolves reordered sparse mesh palettes and preserves morph weights")
  {
    xiiMeshResourceDescriptor descriptor;
    xiiGeometry               geometry;
    geometry.AddBox(xiiVec3(1), true);
    descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    descriptor.AddSubMesh(descriptor.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
    descriptor.ComputeBounds();
    descriptor.m_hDefaultSkeleton = hSkeleton;
    xiiMeshBoneData childBone, rootBone;
    childBone.m_uiBoneIndex                 = 0;
    childBone.m_GlobalInverseRestPoseMatrix = xiiMat4::MakeTranslation(xiiVec3(-3, 0, 0));
    rootBone.m_uiBoneIndex                  = 3;
    rootBone.m_GlobalInverseRestPoseMatrix  = xiiMat4::MakeTranslation(xiiVec3(-1, 0, 0));
    xiiHashedString childName, rootName;
    childName.Assign("Child");
    rootName.Assign("Root");
    descriptor.m_Bones.Insert(childName, childBone);
    descriptor.m_Bones.Insert(rootName, rootBone);
    auto                     hMesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorNativePoseMesh", std::move(descriptor));
    xiiSkinnedMeshComponent* mesh  = nullptr;
    xiiSkinnedMeshComponent::CreateComponent(object, mesh);
    mesh->SetMesh(hMesh);
    const float weights[] = {0.25f};
    mesh->SetMorphWeights(weights);
    pose->SetSkeleton(hSkeleton);
    pose->SetApplyToOwnerMesh(true);
    pose->Update();
    XII_TEST_BOOL(mesh->GetSkeleton() == hSkeleton);
    XII_TEST_INT(mesh->GetSkinningMatrices().GetCount(), 4);
    XII_TEST_FLOAT(mesh->GetSkinningMatrices()[0].GetTranslationVector().x, 5, 0.0001f);
    XII_TEST_BOOL(mesh->GetSkinningMatrices()[1].IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
    XII_TEST_BOOL(mesh->GetSkinningMatrices()[2].IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
    XII_TEST_FLOAT(mesh->GetSkinningMatrices()[3].GetTranslationVector().x, -1, 0.0001f);
    XII_TEST_FLOAT(mesh->GetMorphWeights()[0], 0.25f, 0);
    pose->SetApplyToOwnerMesh(false);
    auto transform          = xiiTransform::MakeIdentity();
    transform.m_vPosition.x = 9;
    XII_TEST_RESULT(pose->SetBoneTransform("Child", transform));
    pose->Update();
    XII_TEST_FLOAT(mesh->GetSkinningMatrices()[0].GetTranslationVector().x, 5, 0.0001f);
    XII_TEST_RESULT(mesh->SetAnimationPose(hSkeleton, pose->GetCurrentPose()));
    XII_TEST_FLOAT(mesh->GetSkinningMatrices()[0].GetTranslationVector().x, 6, 0.0001f);
    XII_TEST_BOOL(mesh->SetAnimationPose({}, pose->GetCurrentPose()).Failed());
    XII_TEST_BOOL(mesh->GetSkinningMatrices().IsEmpty());
    XII_TEST_FLOAT(mesh->GetMorphWeights()[0], 0.25f, 0);
  }
}
