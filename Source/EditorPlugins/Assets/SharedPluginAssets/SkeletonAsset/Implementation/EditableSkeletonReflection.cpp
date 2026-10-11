/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSkeletonJointType, 1)
  XII_ENUM_CONSTANTS(xiiSkeletonJointType::None, xiiSkeletonJointType::Fixed, xiiSkeletonJointType::SwingTwist)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSkeletonJointGeometryType, 1)
XII_ENUM_CONSTANTS(xiiSkeletonJointGeometryType::None, xiiSkeletonJointGeometryType::Capsule, xiiSkeletonJointGeometryType::Sphere, xiiSkeletonJointGeometryType::Box)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeletonBoneShape, 1, xiiRTTIDefaultAllocator<xiiEditableSkeletonBoneShape>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Geometry", xiiSkeletonJointGeometryType, m_Geometry),
    XII_MEMBER_PROPERTY("Offset", m_vOffset),
    XII_MEMBER_PROPERTY("Rotation", m_qRotation),
    XII_MEMBER_PROPERTY("Length", m_fLength)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.01f, 10.0f)),
    XII_MEMBER_PROPERTY("Width", m_fWidth)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.01f, 10.0f)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.01f, 10.0f)),

  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeletonBoneCollider, 2, xiiRTTIDefaultAllocator<xiiEditableSkeletonBoneCollider>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Identifier", m_sIdentifier)->AddAttributes(new xiiHiddenAttribute()),
    XII_ARRAY_MEMBER_PROPERTY("VertexPositions", m_VertexPositions)->AddAttributes(new xiiHiddenAttribute()),
    XII_ARRAY_MEMBER_PROPERTY("TriangleIndices", m_TriangleIndices)->AddAttributes(new xiiHiddenAttribute()),

  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeletonJoint, 2, xiiRTTIDefaultAllocator<xiiEditableSkeletonJoint>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(xiiPropertyFlags::Hidden)->AddAttributes(new xiiDefaultValueAttribute(xiiTransform::MakeIdentity())),
    XII_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    XII_ENUM_MEMBER_PROPERTY("JointType", xiiSkeletonJointType, m_JointType),
    XII_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new xiiDefaultValueAttribute(10.0f)),
    XII_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::MakeFromDegree(170)), new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(30))),
    XII_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::MakeFromDegree(170)), new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(30))),
    XII_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::MakeFromDegree(10), xiiAngle::MakeFromDegree(170)), new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(30))),
    XII_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle)->AddAttributes(new xiiClampValueAttribute(-xiiAngle::MakeFromDegree(170), xiiAngle::MakeFromDegree(170))),

    XII_MEMBER_PROPERTY("OverrideSurface", m_bOverrideSurface),
    XII_MEMBER_PROPERTY("Surface", m_sSurfaceOverride)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("OverrideCollisionLayer", m_bOverrideCollisionLayer),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayerOverride)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),

    XII_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(xiiPropertyFlags::PointerOwner | xiiPropertyFlags::Hidden),
    XII_ARRAY_MEMBER_PROPERTY("BoneShapes", m_BoneShapes),
    XII_ARRAY_MEMBER_PROPERTY("Colliders", m_BoneColliders)->AddAttributes(new xiiContainerAttribute(false, false, false)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute(nullptr, "LocalRotation", nullptr, "GizmoOffsetTranslationRO", "GizmoOffsetRotationRO"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeleton, 2, xiiRTTIDefaultAllocator<xiiEditableSkeleton>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::MeshesWithAnimations)),
    XII_ENUM_MEMBER_PROPERTY("ImportTransform", xiiMeshImportTransform, m_ImportTransform),
    XII_ENUM_MEMBER_PROPERTY("RightDir", xiiBasisAxis, m_RightDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::NegativeX)),
    XII_ENUM_MEMBER_PROPERTY("UpDir", xiiBasisAxis, m_UpDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    XII_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0001f, 10000.0f)),
    XII_ENUM_MEMBER_PROPERTY("BoneDirection", xiiBasisAxis, m_BoneDirection)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", xiiDependencyFlags::None)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("MaxImpulse", m_fMaxImpulse)->AddAttributes(new xiiDefaultValueAttribute(100.f)),
    XII_MEMBER_PROPERTY("LeftFootJoint", m_sLeftFootJoint),
    XII_MEMBER_PROPERTY("RightFootJoint", m_sRightFootJoint),

    XII_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(xiiPropertyFlags::PointerOwner | xiiPropertyFlags::Hidden),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

namespace
{
  class SkeletonColliderIndicesPatch : public xiiGraphPatch
  {
  public:
    SkeletonColliderIndicesPatch() : xiiGraphPatch("xiiEditableSkeletonBoneCollider", 2) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* pNode) const override
    {
      auto* property = pNode->FindProperty("TriangleIndices");
      if (property == nullptr || !property->m_Value.IsA<xiiVariantArray>())
        return;
      auto indices = property->m_Value.Get<xiiVariantArray>();
      for (auto& index : indices)
      {
        if (index.IsA<xiiUInt8>())
          index = static_cast<xiiUInt32>(index.Get<xiiUInt8>());
      }
      property->m_Value = std::move(indices);
    }
  };

  SkeletonColliderIndicesPatch s_ColliderIndicesPatch;
} // namespace
