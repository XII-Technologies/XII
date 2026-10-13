/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>

#include <EditorFramework/Animation/SkeletonPoseAuthoring.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <GraphicsCore/AnimationSystem/SkeletonComponent.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class PoseAuthoringDocument : public xiiDocument
  {
  public:
    PoseAuthoringDocument() : xiiDocument("SkeletonPoseAuthoring.xiiScene", XII_DEFAULT_NEW(xiiDocumentObjectManager))
    {
      m_pDocumentInfo = CreateDocumentInfo();
    }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };

  // Keep the production bone edit command and event subscriptions, excluding window-owned gizmos.
  class PoseAuthoringManipulator : public xiiBoneManipulatorAdapter
  {
  public:
    xiiUInt32 m_uiUpdates = 0;
    void      SetBones(xiiArrayPtr<const xiiExposedBone> bones) { m_Bones = bones; }
    void      SetSkeletonAsset(const xiiUuid& guid) { m_SkeletonAssetGuid = guid; }
    void      AssetEvent(const xiiAssetCuratorEvent& event) { AssetCuratorEventHandler(event); }
    void      Redraw() { xiiBoneManipulatorAdapter::UpdateGizmoTransform(); }
    void      Rotate(xiiUInt32 index, const xiiQuat& rotation)
    {
      auto transform        = xiiTransform::MakeIdentity();
      transform.m_qRotation = rotation;
      SetTransform(index, transform);
    }

  protected:
    void Finalize() override {}
    void Update() override { ++m_uiUpdates; }
    void UpdateGizmoTransform() override {}
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, SkeletonPoseAuthoring)
{
  xiiExposedBone import, root, child;
  import.m_sName                             = "<root-transform>";
  import.m_Transform.m_vScale                = xiiVec3(-2, 2, 2);
  root.m_sName                               = "Root";
  root.m_Transform.m_vPosition               = xiiVec3(1, 2, 3);
  root.m_Transform.m_qRotation               = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisZ(), xiiAngle::MakeFromDegree(35));
  child.m_sName                              = "Child";
  child.m_sParent                            = "Root";
  child.m_Transform.m_vPosition              = xiiVec3(0, 3, 0);
  child.m_Transform.m_vScale                 = xiiVec3(1, 2, 3);
  xiiExposedBone                  metadata[] = {child, import, root};
  xiiMap<xiiString, xiiTransform> overrides;
  xiiDynamicArray<xiiExposedBone> bones;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Asset import coordinates are baked into roots exactly once")
  {
    XII_TEST_RESULT(xiiResolveSkeletonPoseForAuthoring(metadata, overrides, bones));
    XII_TEST_INT(bones.GetCount(), 2);
    XII_TEST_STRING(bones[0].m_sName, "Child");
    XII_TEST_BOOL(bones[1].m_Transform.GetAsMat4().IsEqual(import.m_Transform.GetAsMat4() * root.m_Transform.GetAsMat4(), 0.0001f));
    xiiDynamicArray<xiiMat4> parents;
    XII_TEST_RESULT(xiiExposedBone::ComputeParentTransforms(bones, parents));
    XII_TEST_BOOL(parents[0].IsEqual(bones[1].m_Transform.GetAsMat4(), 0.0001f));
    auto nativeRoot          = xiiTransform::MakeIdentity();
    nativeRoot.m_vPosition.x = 9;
    overrides.Insert("Root", nativeRoot);
    overrides.Insert("FutureJoint", nativeRoot);
    XII_TEST_RESULT(xiiResolveSkeletonPoseForAuthoring(metadata, overrides, bones));
    XII_TEST_BOOL(bones[1].m_Transform.IsEqual(nativeRoot, 0));
    XII_TEST_INT(bones.GetCount(), 2);
    overrides.Clear();
    child.m_sParent          = "Missing";
    xiiExposedBone invalid[] = {child, root};
    XII_TEST_BOOL(xiiResolveSkeletonPoseForAuthoring(invalid, overrides, bones).Failed());
    XII_TEST_BOOL(bones.IsEmpty());
    child.m_sParent = "Root";
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native pose edits, rotations and removals support undo and redo")
  {
    PoseAuthoringDocument document;
    auto*                 accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Create pose");
    xiiUuid objectId, id;
    auto    result = accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiGameObject>(), objectId);
    if (result.Succeeded())
      result = accessor->AddObjectByName(accessor->GetObject(objectId), "Components", -1, xiiGetStaticRTTI<xiiSkeletonPoseComponent>(), id);
    XII_TEST_STATUS(result);
    if (result.Failed())
    {
      accessor->CancelTransaction();
      return;
    }
    accessor->FinishTransaction();
    const auto*         component = document.GetObjectManager()->GetObject(id);
    const auto*         property  = component->GetType()->FindPropertyByName("BoneTransforms");
    const auto*         attribute = component->GetType()->GetAttributeByType<xiiBoneManipulatorAttribute>();
    xiiWorldDescription description("Pose authoring document mirror");
    description.m_bReportErrorWhenStaticObjectMoves = false;
    xiiWorld world(description);
    XII_LOCK(world.GetWriteMarker());
    world.SetWorldSimulationEnabled(false);
    xiiWorldRttiConverterContext context;
    context.m_pWorld = &world;
    xiiDocumentObjectMirror mirror;
    mirror.InitSender(document.GetObjectManager());
    mirror.InitReceiver(&context);
    XII_SCOPE_EXIT(mirror.Clear(); mirror.DeInit(); context.Clear());
    mirror.SendDocument();
    const auto native = context.GetObjectByGUID(id);
    if (!XII_TEST_BOOL(native.m_pObject != nullptr && native.m_pType == xiiGetStaticRTTI<xiiSkeletonPoseComponent>()))
      return;
    auto*                    nativePose = static_cast<xiiSkeletonPoseComponent*>(native.m_pObject);
    PoseAuthoringManipulator manipulator;
    manipulator.SetManipulator(attribute, component);
    XII_TEST_RESULT(xiiResolveSkeletonPoseForAuthoring(metadata, overrides, bones));
    manipulator.SetBones(bones);
    const auto rotation  = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisX(), xiiAngle::MakeFromDegree(45));
    const auto before    = manipulator.m_uiUpdates;
    const auto assetGuid = xiiUuid::MakeUuid();
    manipulator.SetSkeletonAsset(assetGuid);
    xiiAssetCuratorEvent event;
    event.m_Type      = xiiAssetCuratorEvent::Type::AssetUpdated;
    event.m_AssetGuid = xiiUuid::MakeUuid();
    manipulator.AssetEvent(event);
    manipulator.Redraw();
    XII_TEST_INT(manipulator.m_uiUpdates, before);
    event.m_AssetGuid = assetGuid;
    manipulator.AssetEvent(event);
    manipulator.Redraw();
    XII_TEST_INT(manipulator.m_uiUpdates, before + 1);
    manipulator.Redraw();
    XII_TEST_INT(manipulator.m_uiUpdates, before + 1);
    const auto beforeRotate = manipulator.m_uiUpdates;
    accessor->StartTransaction("Rotate child");
    manipulator.Rotate(0, rotation);
    accessor->FinishTransaction();
    XII_TEST_BOOL(manipulator.m_uiUpdates > beforeRotate);
    xiiVariant value;
    XII_TEST_STATUS(accessor->GetValue(component, property, value, "Child"));
    XII_TEST_BOOL(value.IsA<xiiTransform>());
    auto transform = value.Get<xiiTransform>();
    XII_TEST_VEC3(transform.m_vPosition, child.m_Transform.m_vPosition, 0);
    XII_TEST_VEC3(transform.m_vScale, child.m_Transform.m_vScale, 0);
    XII_TEST_BOOL(transform.m_qRotation.IsEqualRotation(rotation, 0.0001f));
    const auto* nativeTransform = nativePose->GetBoneTransforms().GetValue("Child");
    if (XII_TEST_BOOL(nativeTransform != nullptr))
      XII_TEST_BOOL(nativeTransform->IsEqual(transform, 0));
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_BOOL(accessor->GetValue(component, property, value, "Child").Failed());
    XII_TEST_BOOL(nativePose->GetBoneTransforms().IsEmpty());
    XII_TEST_STATUS(document.GetCommandHistory()->Redo());
    XII_TEST_STATUS(accessor->GetValue(component, property, value, "Child"));

    overrides.Insert("Child", child.m_Transform);
    overrides.Insert("Root", root.m_Transform);
    accessor->StartTransaction("Pull native pose");
    XII_TEST_STATUS(xiiApplySkeletonPoseForAuthoring(*accessor, component, overrides));
    accessor->FinishTransaction();
    XII_TEST_STATUS(accessor->GetValue(component, property, value, "Root"));
    XII_TEST_BOOL(value.Get<xiiTransform>().IsEqual(root.m_Transform, 0));
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_BOOL(accessor->GetValue(component, property, value, "Root").Failed());
    XII_TEST_STATUS(accessor->GetValue(component, property, value, "Child"));
    XII_TEST_BOOL(value.Get<xiiTransform>().m_qRotation.IsEqualRotation(rotation, 0.0001f));

    auto invalid       = xiiTransform::MakeIdentity();
    invalid.m_vScale.x = xiiMath::NaN<float>();
    overrides.Insert("Invalid", invalid);
    accessor->StartTransaction("Reject invalid pose");
    XII_TEST_BOOL(xiiApplySkeletonPoseForAuthoring(*accessor, component, overrides).Failed());
    accessor->CancelTransaction();
    XII_TEST_BOOL(accessor->GetValue(component, property, value, "Root").Failed());
    const auto beforeRemove = manipulator.m_uiUpdates;
    accessor->StartTransaction("Reset child");
    XII_TEST_STATUS(accessor->RemoveValue(component, property, "Child"));
    accessor->FinishTransaction();
    XII_TEST_BOOL(manipulator.m_uiUpdates > beforeRemove);
    XII_TEST_STATUS(document.GetCommandHistory()->Undo());
    XII_TEST_STATUS(accessor->GetValue(component, property, value, "Child"));
  }
}
