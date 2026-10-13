/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

namespace
{
  xiiEditableSkeletonJoint* AddJoint(xiiHybridArray<xiiEditableSkeletonJoint*, 4>& children, const char* szName, xiiVec3 position = xiiVec3::MakeZero())
  {
    xiiEditableSkeletonJoint* pJoint = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
    pJoint->SetName(szName);
    pJoint->m_LocalTransform.m_vPosition = position;
    children.PushBack(pJoint);
    return pJoint;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, EditableSkeleton)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native conversion preserves hierarchy, mirrored matrices and resource round trip")
  {
    xiiEditableSkeleton skeleton;
    skeleton.m_fUniformScaling          = 2.5f;
    auto* pRoot                         = AddJoint(skeleton.m_Children, "Root", xiiVec3(1, 2, 3));
    pRoot->m_LocalTransform.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(1, 2, 3).GetNormalized(), xiiAngle::MakeFromDegree(37));
    pRoot->m_LocalTransform.m_vScale    = xiiVec3(-1, 2, 3);
    auto* pChild                        = AddJoint(pRoot->m_Children, "Child", xiiVec3(2, 3, 4));
    auto* pOther                        = AddJoint(skeleton.m_Children, "Other", xiiVec3(6, 7, 8));

    for (xiiUInt32 flip = 0; flip < 2; ++flip)
    {
      skeleton.m_ImportTransform = xiiMeshImportTransform::Custom;
      skeleton.m_bFlipForwardDir = flip != 0;
      xiiTransform root;
      XII_TEST_RESULT(skeleton.GetRootTransform(root));
      xiiSkeletonResourceDescriptor descriptor;
      XII_TEST_RESULT(skeleton.FillResourceDescriptor(descriptor));
      XII_TEST_INT(descriptor.m_Joints.GetCount(), 3);
      XII_TEST_STRING(descriptor.m_Joints[0].m_sName.GetData(), "Root");
      XII_TEST_STRING(descriptor.m_Joints[1].m_sName.GetData(), "Child");
      XII_TEST_STRING(descriptor.m_Joints[2].m_sName.GetData(), "Other");
      XII_TEST_INT(descriptor.m_Joints[1].m_uiParentIndex, 0);
      XII_TEST_INT(descriptor.m_Joints[2].m_uiParentIndex, xiiMath::MaxValue<xiiUInt16>());
      const auto rootModel = root.GetAsMat4() * pRoot->m_LocalTransform.GetAsMat4();
      XII_TEST_BOOL(descriptor.m_Joints[0].m_ModelRestPose.IsEqual(rootModel, 0.0001f));
      XII_TEST_BOOL(descriptor.m_Joints[1].m_ModelRestPose.IsEqual(rootModel * pChild->m_LocalTransform.GetAsMat4(), 0.0001f));
      XII_TEST_BOOL(descriptor.m_Joints[2].m_ModelRestPose.IsEqual(root.GetAsMat4() * pOther->m_LocalTransform.GetAsMat4(), 0.0001f));
      for (const auto& joint : descriptor.m_Joints)
      {
        XII_TEST_BOOL((joint.m_ModelRestPose * joint.m_InverseBindPose).IsEqual(xiiMat4::MakeIdentity(), 0.0001f));
        XII_TEST_BOOL(descriptor.m_Bounds.GetBox().Contains(joint.m_ModelRestPose.GetTranslationVector()));
      }
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      XII_TEST_RESULT(descriptor.Serialize(writer));
      xiiMemoryStreamReader         reader(&storage);
      xiiSkeletonResourceDescriptor restored;
      XII_TEST_RESULT(restored.Deserialize(reader));
      XII_TEST_INT(restored.m_uiRuntimeHash, descriptor.m_uiRuntimeHash);
      XII_TEST_BOOL(restored.m_Joints[1].m_ModelRestPose.IsEqual(descriptor.m_Joints[1].m_ModelRestPose, 0.0001f));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Authoring schema round trip and import merge retain user settings")
  {
    xiiEditableSkeleton skeleton;
    skeleton.m_sSourceFile                           = "Meshes/Character.glb";
    skeleton.m_sPreviewMesh                          = "PreviewMesh";
    skeleton.m_sSurfaceFile                          = "Surface";
    skeleton.m_uiCollisionLayer                      = 4;
    skeleton.m_sLeftFootJoint                        = "LeftFoot";
    auto* pRoot                                      = AddJoint(skeleton.m_Children, "Root");
    pRoot->m_JointType                               = xiiSkeletonJointType::SwingTwist;
    pRoot->m_fStiffness                              = 12;
    pRoot->m_SwingLimitY                             = xiiAngle::MakeFromDegree(35);
    pRoot->m_bOverrideCollisionLayer                 = true;
    pRoot->m_uiCollisionLayerOverride                = 7;
    pRoot->m_BoneShapes.ExpandAndGetRef().m_Geometry = xiiSkeletonJointGeometryType::Capsule;
    auto& collider                                   = pRoot->m_BoneColliders.ExpandAndGetRef();
    collider.m_sIdentifier                           = "ImportedCollider";
    collider.m_VertexPositions.PushBack(xiiVec3(1, 2, 3));
    collider.m_TriangleIndices.PushBack(0);
    AddJoint(pRoot->m_Children, "Child", xiiVec3(1, 2, 3));
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, skeleton.GetDynamicRTTI(), &skeleton);
    xiiMemoryStreamReader reader(&storage);
    const xiiRTTI*        pType   = nullptr;
    void*                 pObject = xiiReflectionSerializer::ReadObjectFromBinary(reader, pType);
    if (XII_TEST_BOOL(pObject != nullptr && pType == xiiGetStaticRTTI<xiiEditableSkeleton>()))
    {
      auto* pRestored = static_cast<xiiEditableSkeleton*>(pObject);
      XII_TEST_STRING(pRestored->m_sSourceFile.GetData(), skeleton.m_sSourceFile.GetData());
      XII_TEST_STRING(pRestored->m_sLeftFootJoint.GetData(), "LeftFoot");
      XII_TEST_INT(pRestored->m_uiCollisionLayer, 4);
      XII_TEST_INT(pRestored->m_Children.GetCount(), 1);
      auto* pJoint = pRestored->m_Children[0];
      XII_TEST_INT(pJoint->m_Children.GetCount(), 1);
      XII_TEST_INT(pJoint->m_JointType.GetValue(), xiiSkeletonJointType::SwingTwist);
      XII_TEST_FLOAT(pJoint->m_SwingLimitY.GetDegree(), 35, 0.001f);
      XII_TEST_INT(pJoint->m_BoneShapes[0].m_Geometry.GetValue(), xiiSkeletonJointGeometryType::Capsule);
      XII_TEST_VEC3(pJoint->m_BoneColliders[0].m_VertexPositions[0], xiiVec3(1, 2, 3), 0.001f);
      XII_TEST_INT(pJoint->m_uiCollisionLayerOverride, 7);

      xiiEditableSkeletonJoint imported;
      imported.SetName("Reimported");
      imported.m_LocalTransform.m_vPosition.Set(5, 6, 7);
      imported.CopyPropertiesFrom(pJoint);
      XII_TEST_STRING(imported.GetName(), "Reimported");
      XII_TEST_VEC3(imported.m_LocalTransform.m_vPosition, xiiVec3(5, 6, 7), 0.001f);
      XII_TEST_BOOL(imported.m_Children.IsEmpty() && imported.m_BoneColliders.IsEmpty());
      XII_TEST_INT(imported.m_BoneShapes.GetCount(), 1);
      XII_TEST_INT(imported.m_uiCollisionLayerOverride, 7);
    }
    if (pObject != nullptr)
      pType->GetAllocator()->Deallocate(pObject);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reimport transfers hierarchy and preserves settings atomically")
  {
    xiiEditableSkeleton document;
    document.m_sSourceFile                                   = "Original.glb";
    document.m_sPreviewMesh                                  = "Preview";
    document.m_fUniformScaling                               = 2;
    document.m_ImportTransform                               = xiiMeshImportTransform::Custom;
    document.m_bFlipForwardDir                               = true;
    auto* oldRoot                                            = AddJoint(document.m_Children, "Root");
    oldRoot->m_fStiffness                                    = 42;
    oldRoot->m_BoneShapes.ExpandAndGetRef().m_Geometry       = xiiSkeletonJointGeometryType::Sphere;
    oldRoot->m_BoneColliders.ExpandAndGetRef().m_sIdentifier = "OldGenerated";
    auto* oldChild                                           = AddJoint(oldRoot->m_Children, "Child");
    oldChild->m_bOverrideSurface                             = true;
    oldChild->m_sSurfaceOverride                             = "Surface";
    AddJoint(document.m_Children, "Removed");

    xiiEditableSkeleton imported;
    auto*               newRoot                              = AddJoint(imported.m_Children, "Root", xiiVec3(1, 2, 3));
    auto*               added                                = AddJoint(newRoot->m_Children, "Added", xiiVec3(4, 5, 6));
    auto*               newChild                             = AddJoint(newRoot->m_Children, "Child", xiiVec3(7, 8, 9));
    newRoot->m_BoneColliders.ExpandAndGetRef().m_sIdentifier = "NewGenerated";
    XII_TEST_RESULT(document.MergeImportedHierarchy(imported));
    XII_TEST_BOOL(imported.m_Children.IsEmpty());
    XII_TEST_INT(document.m_Children.GetCount(), 1);
    XII_TEST_BOOL(document.m_Children[0] == newRoot);
    XII_TEST_BOOL(newRoot->m_Children[0] == added && newRoot->m_Children[1] == newChild);
    XII_TEST_STRING(document.m_sSourceFile.GetData(), "Original.glb");
    XII_TEST_STRING(document.m_sPreviewMesh.GetData(), "Preview");
    XII_TEST_FLOAT(document.m_fUniformScaling, 2, 0);
    XII_TEST_FLOAT(newRoot->m_fStiffness, 42, 0);
    XII_TEST_INT(newRoot->m_BoneShapes.GetCount(), 1);
    XII_TEST_STRING(newRoot->m_BoneColliders[0].m_sIdentifier.GetData(), "NewGenerated");
    XII_TEST_BOOL(newChild->m_bOverrideSurface);
    XII_TEST_STRING(newChild->m_sSurfaceOverride.GetData(), "Surface");
    XII_TEST_VEC3(newChild->m_LocalTransform.m_vPosition, xiiVec3(7, 8, 9), 0);
    xiiSkeletonResourceDescriptor native;
    XII_TEST_RESULT(document.FillResourceDescriptor(native));
    XII_TEST_VEC3(newChild->m_vGizmoOffsetPositionRO, native.m_Joints[2].m_ModelRestPose.GetTranslationVector(), 0.0001f);
    XII_TEST_BOOL(newRoot->m_qGizmoOffsetRotationRO.IsValid() && newChild->m_qGizmoOffsetRotationRO.IsValid());
    XII_TEST_BOOL(newChild->m_qGizmoOffsetRotationRO.GetAsMat3().IsValid());

    xiiEditableSkeleton invalid;
    auto*               invalidRoot = AddJoint(invalid.m_Children, "Duplicate");
    AddJoint(invalidRoot->m_Children, "Duplicate");
    XII_TEST_BOOL(document.MergeImportedHierarchy(invalid).Failed());
    XII_TEST_BOOL(document.m_Children[0] == newRoot && invalid.m_Children[0] == invalidRoot);
    XII_TEST_BOOL(document.MergeImportedHierarchy(document).Failed());
    document.m_fUniformScaling = 0;
    XII_TEST_BOOL(document.MergeImportedHierarchy(imported).Failed());
    XII_TEST_BOOL(document.m_Children[0] == newRoot);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Collider index schema migrates byte indices and round trips large geometry")
  {
    xiiAbstractObjectGraph legacy;
    auto*                  node = legacy.AddNode(xiiUuid::MakeUuid(), "xiiEditableSkeletonBoneCollider", 1, "root");
    xiiVariantArray        byteIndices;
    byteIndices.PushBack(xiiUInt8(0));
    byteIndices.PushBack(xiiUInt8(1));
    byteIndices.PushBack(xiiUInt8(255));
    node->AddProperty("TriangleIndices", byteIndices);
    xiiAbstractObjectGraph types;
    auto*                  typeNode = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
    typeNode->AddProperty("TypeName", "xiiEditableSkeletonBoneCollider");
    typeNode->AddProperty("ParentTypeName", "xiiReflectedClass");
    typeNode->AddProperty("TypeVersion", xiiUInt32(1));
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiAbstractGraphBinarySerializer::Write(writer, &legacy, &types);
    xiiMemoryStreamReader  reader(&storage);
    xiiAbstractObjectGraph patched;
    xiiAbstractObjectGraph patchedTypes;
    xiiAbstractGraphBinarySerializer::Read(reader, &patched, &patchedTypes, true);
    const auto* patchedNode = patched.GetNodeByName("root");
    XII_TEST_INT(patchedNode->GetTypeVersion(), 2);
    const auto& indices = patchedNode->FindProperty("TriangleIndices")->m_Value.Get<xiiVariantArray>();
    XII_TEST_BOOL(indices[2].IsA<xiiUInt32>());
    XII_TEST_INT(indices[2].ConvertTo<xiiUInt32>(), 255);
    // Binary IPC readers also accept legacy numeric property values through standard RTTI conversion.
    xiiMemoryStreamReader legacyReader(&storage);
    const xiiRTTI*        type     = nullptr;
    auto*                 collider = static_cast<xiiEditableSkeletonBoneCollider*>(xiiReflectionSerializer::ReadObjectFromBinary(legacyReader, type));
    if (XII_TEST_BOOL(collider != nullptr && type == xiiGetStaticRTTI<xiiEditableSkeletonBoneCollider>()))
    {
      XII_TEST_INT(collider->m_TriangleIndices[2], 255);
      collider->m_VertexPositions.SetCount(300, xiiVec3::MakeZero());
      collider->m_TriangleIndices[2] = 299;
      xiiDefaultMemoryStreamStorage modernStorage;
      xiiMemoryStreamWriter         modernWriter(&modernStorage);
      xiiReflectionSerializer::WriteObjectToBinary(modernWriter, type, collider);
      xiiMemoryStreamReader modernReader(&modernStorage);
      auto*                 restored = static_cast<xiiEditableSkeletonBoneCollider*>(xiiReflectionSerializer::ReadObjectFromBinary(modernReader, type));
      if (XII_TEST_BOOL(restored != nullptr))
      {
        XII_TEST_INT(restored->m_VertexPositions.GetCount(), 300);
        XII_TEST_INT(restored->m_TriangleIndices[2], 299);
        XII_DEFAULT_DELETE(restored);
      }
    }
    XII_DEFAULT_DELETE(collider);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Malformed inputs reject safely and clear stale output")
  {
    xiiEditableSkeleton           skeleton;
    auto*                         pRoot  = AddJoint(skeleton.m_Children, "Root");
    auto*                         pChild = AddJoint(pRoot->m_Children, "Child");
    xiiSkeletonResourceDescriptor descriptor;
    XII_TEST_RESULT(skeleton.FillResourceDescriptor(descriptor));
    pChild->SetName("Root");
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    XII_TEST_BOOL(descriptor.m_Joints.IsEmpty());
    pChild->SetName("");
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pChild->SetName("Child");
    pChild->m_LocalTransform.m_vScale.x = 0;
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pChild->m_LocalTransform.m_vScale.x    = 1;
    pChild->m_LocalTransform.m_vPosition.x = xiiMath::Infinity<float>();
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pChild->m_LocalTransform.SetIdentity();
    pRoot->m_Children.PushBack(pChild);
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pRoot->m_Children.PopBack();
    pChild->m_Children.PushBack(pRoot);
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pChild->m_Children.Clear();
    pRoot->m_Children.PushBack(nullptr);
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    pRoot->m_Children.PopBack();
    skeleton.m_ImportTransform = xiiMeshImportTransform::Custom;
    skeleton.m_RightDir        = skeleton.m_UpDir;
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    skeleton.m_RightDir        = xiiBasisAxis::NegativeX;
    skeleton.m_fUniformScaling = xiiMath::Infinity<float>();
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    skeleton.m_fUniformScaling = 0;
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    skeleton.m_fUniformScaling = 1;
    XII_TEST_RESULT(skeleton.FillResourceDescriptor(descriptor));
    // The owner also cleans up malformed child links without recursion or double deletion.
    pChild->m_Children.PushBack(pRoot);
    skeleton.m_Children.PushBack(pChild);
    skeleton.ClearJoints();
    XII_TEST_BOOL(skeleton.m_Children.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Deep hierarchy and joint index boundary")
  {
    xiiEditableSkeleton skeleton;
    auto*               pParent = AddJoint(skeleton.m_Children, "Joint0");
    xiiStringBuilder    name;
    for (xiiUInt32 i = 1; i < xiiMath::MaxValue<xiiUInt16>(); ++i)
    {
      name.SetFormat("Joint{}", i);
      pParent = AddJoint(pParent->m_Children, name.GetData());
    }
    xiiSkeletonResourceDescriptor descriptor;
    XII_TEST_RESULT(skeleton.FillResourceDescriptor(descriptor));
    XII_TEST_INT(descriptor.m_Joints.GetCount(), xiiMath::MaxValue<xiiUInt16>());
    XII_TEST_INT(descriptor.m_Joints.PeekBack().m_uiParentIndex, xiiMath::MaxValue<xiiUInt16>() - 2);
    AddJoint(pParent->m_Children, "Overflow");
    XII_TEST_BOOL(skeleton.FillResourceDescriptor(descriptor).Failed());
    XII_TEST_BOOL(descriptor.m_Joints.IsEmpty());
    skeleton.ClearJoints();
    XII_TEST_RESULT(skeleton.FillResourceDescriptor(descriptor));
    XII_TEST_BOOL(descriptor.m_Bounds.IsValid());
  }
}
