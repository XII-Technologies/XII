/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <SharedPluginAssets/SkeletonAsset/SkeletonPreview.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreviewMessages.h>

XII_CREATE_SIMPLE_TEST(EditorViews, SkeletonPreview)
{
  xiiEditableSkeleton       authoring;
  xiiEditableSkeletonJoint* pRoot = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
  authoring.m_Children.PushBack(pRoot);
  pRoot->SetName("Root");
  xiiEditableSkeletonJoint* pChild = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
  pRoot->m_Children.PushBack(pChild);
  pChild->SetName("Child");
  pChild->m_LocalTransform.m_vPosition.Set(0, 1, 0);
  xiiEditableSkeletonJoint* pOther = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
  authoring.m_Children.PushBack(pOther);
  pOther->SetName("ChildExtra");
  pOther->m_LocalTransform.m_vPosition.Set(2, 0, 0);
  xiiSkeletonResourceDescriptor descriptor;
  XII_TEST_RESULT(authoring.FillResourceDescriptor(descriptor));
  xiiSkeletonPreviewSettings settings;
  settings.m_sHighlightedBones = "Child;";
  xiiSkeletonPreview preview;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native bones, exact highlighting and disconnected roots")
  {
    XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings));
    XII_TEST_INT(preview.GetGeometry().GetLines().GetCount(), 10);
    xiiUInt32 yellowVertices = 0;
    for (const auto& vertex : preview.GetGeometry().GetVertices())
      yellowVertices += vertex.m_Color == xiiColor::Yellow ? 1 : 0;
    XII_TEST_INT(yellowVertices, 8); // Child's connection and marker; ChildExtra is not selected.
    XII_TEST_BOOL(preview.GetBounds().GetBox().Contains(descriptor.m_Joints[2].m_ModelRestPose.GetTranslationVector()));
    settings.m_bBones = false;
    XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings));
    XII_TEST_BOOL(preview.GetGeometry().GetLines().IsEmpty());
    XII_TEST_BOOL(preview.GetBounds().IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Colliders, axes and limits use authored settings")
  {
    settings.m_bBones  = false;
    auto& shape        = pChild->m_BoneShapes.ExpandAndGetRef();
    shape.m_fLength    = 1;
    shape.m_fWidth     = 0.2f;
    shape.m_fThickness = 0.1f;
    for (auto type : {xiiSkeletonJointGeometryType::Box, xiiSkeletonJointGeometryType::Sphere, xiiSkeletonJointGeometryType::Capsule})
    {
      shape.m_Geometry = type;
      XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings, &authoring));
      XII_TEST_BOOL(!preview.GetGeometry().GetLines().IsEmpty());
      for (const auto& vertex : preview.GetGeometry().GetVertices())
      {
        XII_TEST_BOOL(vertex.m_vPosition.IsValid());
        XII_TEST_BOOL(preview.GetBounds().GetBox().Contains(vertex.m_vPosition));
      }
    }
    pChild->m_BoneShapes.Clear();
    auto& collider = pChild->m_BoneColliders.ExpandAndGetRef();
    collider.m_VertexPositions.PushBack({0, 0, 0});
    collider.m_VertexPositions.PushBack({1, 0, 0});
    collider.m_VertexPositions.PushBack({0, 1, 0});
    collider.m_TriangleIndices.PushBack(0);
    collider.m_TriangleIndices.PushBack(1);
    collider.m_TriangleIndices.PushBack(2);
    XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings, &authoring));
    XII_TEST_INT(preview.GetGeometry().GetLines().GetCount(), 3);
    collider.m_TriangleIndices[2] = 3;
    XII_TEST_BOOL(preview.Build(descriptor.m_Joints, settings, &authoring).Failed());
    XII_TEST_BOOL(preview.GetGeometry().GetLines().IsEmpty());
    collider.m_TriangleIndices[2]   = 2;
    settings.m_bColliders           = false;
    settings.m_bJoints              = true;
    settings.m_bSwingLimits         = true;
    settings.m_bTwistLimits         = true;
    pChild->m_JointType             = xiiSkeletonJointType::SwingTwist;
    pChild->m_SwingLimitY           = xiiAngle::MakeFromDegree(130);
    pChild->m_SwingLimitZ           = xiiAngle::MakeFromDegree(35);
    pChild->m_TwistLimitHalfAngle   = xiiAngle::MakeFromDegree(45);
    pChild->m_TwistLimitCenterAngle = xiiAngle::MakeFromDegree(10);
    XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings, &authoring));
    XII_TEST_BOOL(preview.GetGeometry().GetLines().GetCount() > 70);
    pChild->m_SwingLimitY = xiiAngle::MakeFromDegree(-1);
    XII_TEST_BOOL(preview.Build(descriptor.m_Joints, settings, &authoring).Failed());
    pChild->m_SwingLimitY = xiiAngle::MakeFromDegree(130);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Preview metadata round trip and invalid message rejection")
  {
    xiiEditorEngineSkeletonPreviewMsg message;
    XII_TEST_RESULT(message.SetSkeleton(authoring));
    xiiUniquePtr<xiiEditableSkeleton> restored;
    XII_TEST_RESULT(message.GetSkeleton(restored));
    XII_TEST_INT(restored->m_Children.GetCount(), 2);
    XII_TEST_FLOAT(restored->m_Children[0]->m_Children[0]->m_SwingLimitY.GetDegree(), 130, 0.001f);
    XII_TEST_RESULT(preview.Build(descriptor.m_Joints, settings, restored.Borrow()));
    message.m_Data.Clear();
    XII_TEST_BOOL(message.GetSkeleton(restored).Failed());
    XII_TEST_BOOL(restored == nullptr);
    pChild->SetName("Root");
    XII_TEST_BOOL(message.SetSkeleton(authoring).Failed());
    XII_TEST_BOOL(message.m_Data.IsEmpty());
    pChild->SetName("Child");
    descriptor.m_Joints[1].m_uiParentIndex = 1;
    XII_TEST_BOOL(preview.Build(descriptor.m_Joints, settings, &authoring).Failed());
    XII_TEST_BOOL(preview.GetGeometry().GetLines().IsEmpty());
    XII_TEST_BOOL(!preview.GetBounds().IsValid());
  }
}
