#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeletonBoneCollider, 1, xiiRTTIDefaultAllocator<xiiEditableSkeletonBoneCollider>)
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
    XII_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(xiiPropertyFlags::Hidden)->AddAttributes(new xiiDefaultValueAttribute(xiiTransform::IdentityTransform())),
    XII_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    XII_ENUM_MEMBER_PROPERTY("JointType", xiiSkeletonJointType, m_JointType),
    XII_MEMBER_PROPERTY("LimitSwing", m_bLimitSwing),
    XII_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(170)), new xiiDefaultValueAttribute(xiiAngle::Degree(30))),
    XII_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(170)), new xiiDefaultValueAttribute(xiiAngle::Degree(30))),
    XII_MEMBER_PROPERTY("LimitTwist", m_bLimitTwist),
    XII_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(10), xiiAngle::Degree(170)), new xiiDefaultValueAttribute(xiiAngle::Degree(30))),
    XII_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle)->AddAttributes(new xiiClampValueAttribute(-xiiAngle::Degree(170), xiiAngle::Degree(170))),

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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditableSkeleton, 1, xiiRTTIDefaultAllocator<xiiEditableSkeleton>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new xiiFileBrowserAttribute("Select Mesh", xiiFileBrowserAttribute::SkeletalMeshes)),
    XII_ENUM_MEMBER_PROPERTY("RightDir", xiiBasisAxis, m_RightDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveX)),
    XII_ENUM_MEMBER_PROPERTY("UpDir", xiiBasisAxis, m_UpDir)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    XII_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0001f, 10000.0f)),
    XII_ENUM_MEMBER_PROPERTY("BoneDirection", xiiBasisAxis, m_BoneDirection)->AddAttributes(new xiiDefaultValueAttribute((int)xiiBasisAxis::PositiveY)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("MaxImpulse", m_fMaxImpulse)->AddAttributes(new xiiDefaultValueAttribute(100.f)),

    XII_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(xiiPropertyFlags::PointerOwner | xiiPropertyFlags::Hidden),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiExposedBone, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiExposedBone>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Parent", m_sParent),
    XII_MEMBER_PROPERTY("Transform", m_Transform),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiExposedBone);
// clang-format on


void operator<<(xiiStreamWriter& inout_stream, const xiiExposedBone& bone)
{
  inout_stream << bone.m_sName;
  inout_stream << bone.m_sParent;
  inout_stream << bone.m_Transform;
}

void operator>>(xiiStreamReader& inout_stream, xiiExposedBone& ref_bone)
{
  inout_stream >> ref_bone.m_sName;
  inout_stream >> ref_bone.m_sParent;
  inout_stream >> ref_bone.m_Transform;
}

bool operator==(const xiiExposedBone& lhs, const xiiExposedBone& rhs)
{
  if (lhs.m_sName != rhs.m_sName)
    return false;
  if (lhs.m_sParent != rhs.m_sParent)
    return false;
  if (lhs.m_Transform != rhs.m_Transform)
    return false;
  return true;
}

xiiEditableSkeleton::xiiEditableSkeleton() = default;
xiiEditableSkeleton::~xiiEditableSkeleton()
{
  ClearJoints();
}

void xiiEditableSkeleton::ClearJoints()
{
  for (xiiEditableSkeletonJoint* pChild : m_Children)
  {
    XII_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void xiiEditableSkeleton::CreateJointsRecursive(xiiSkeletonBuilder& ref_sb, xiiSkeletonResourceDescriptor& ref_desc, const xiiEditableSkeletonJoint* pParentJoint, const xiiEditableSkeletonJoint* pThisJoint, xiiUInt16 uiThisJointIdx, const xiiQuat& qParentAccuRot, const xiiMat4& mRootTransform) const
{
  for (auto& shape : pThisJoint->m_BoneShapes)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();

    geo.m_Type              = shape.m_Geometry;
    geo.m_uiAttachedToJoint = static_cast<xiiUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(shape.m_fLength, shape.m_fWidth, shape.m_fThickness);
    geo.m_Transform.m_vPosition = shape.m_vOffset;
    geo.m_Transform.m_qRotation = shape.m_qRotation;
  }

  for (auto& shape : pThisJoint->m_BoneColliders)
  {
    auto& geo               = ref_desc.m_Geometry.ExpandAndGetRef();
    geo.m_Type              = xiiSkeletonJointGeometryType::ConvexMesh;
    geo.m_uiAttachedToJoint = static_cast<xiiUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_VertexPositions = shape.m_VertexPositions;
    geo.m_TriangleIndices = shape.m_TriangleIndices;
  }

  XII_ASSERT_DEBUG(pThisJoint->m_LocalTransform.m_vScale.IsEqual(xiiVec3(1), 0.1f), "fuck");

  const xiiQuat qThisAccuRot = qParentAccuRot * pThisJoint->m_LocalTransform.m_qRotation;
  xiiQuat       qParentGlobalRot;

  {
    // as always, the root transform is the bane of my existence
    // since it can contain mirroring, the final global rotation of a joint will be incorrect if we don't incorporate the root scale
    // unfortunately this can't be done once for the first node, but has to be done on the result instead

    xiiMat4 full;
    xiiMsgAnimationPoseUpdated::ComputeFullBoneTransform(mRootTransform, qParentAccuRot.GetAsMat4(), full, qParentGlobalRot);
  }

  ref_sb.SetJointLimit(uiThisJointIdx, pThisJoint->m_qLocalJointRotation, pThisJoint->m_JointType, pThisJoint->m_bLimitSwing, pThisJoint->m_SwingLimitY, pThisJoint->m_SwingLimitZ, pThisJoint->m_bLimitTwist, pThisJoint->m_TwistLimitHalfAngle, pThisJoint->m_TwistLimitCenterAngle);

  ref_sb.SetJointCollisionLayer(uiThisJointIdx, pThisJoint->m_bOverrideCollisionLayer ? pThisJoint->m_uiCollisionLayerOverride : m_uiCollisionLayer);
  ref_sb.SetJointSurface(uiThisJointIdx, pThisJoint->m_bOverrideSurface ? pThisJoint->m_sSurfaceOverride : m_sSurfaceFile);

  for (const auto* pChildJoint : pThisJoint->m_Children)
  {
    const xiiUInt16 uiChildJointIdx = ref_sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_LocalTransform, uiThisJointIdx);

    CreateJointsRecursive(ref_sb, ref_desc, pThisJoint, pChildJoint, uiChildJointIdx, qThisAccuRot, mRootTransform);
  }
}

void xiiEditableSkeleton::FillResourceDescriptor(xiiSkeletonResourceDescriptor& ref_desc) const
{
  ref_desc.m_fMaxImpulse = m_fMaxImpulse;
  ref_desc.m_Geometry.Clear();

  xiiSkeletonBuilder sb;
  for (const auto* pJoint : m_Children)
  {
    const xiiUInt16 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_LocalTransform);

    CreateJointsRecursive(sb, ref_desc, nullptr, pJoint, idx, xiiQuat::IdentityQuaternion(), ref_desc.m_RootTransform.GetAsMat4());
  }

  sb.BuildSkeleton(ref_desc.m_Skeleton);
  ref_desc.m_Skeleton.m_BoneDirection = m_BoneDirection;
}

static void BuildOzzRawSkeleton(const xiiEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& ref_dstJoint)
{
  ref_dstJoint.name                    = srcJoint.m_sName.GetString();
  ref_dstJoint.transform.translation.x = srcJoint.m_LocalTransform.m_vPosition.x;
  ref_dstJoint.transform.translation.y = srcJoint.m_LocalTransform.m_vPosition.y;
  ref_dstJoint.transform.translation.z = srcJoint.m_LocalTransform.m_vPosition.z;
  ref_dstJoint.transform.rotation.x    = srcJoint.m_LocalTransform.m_qRotation.v.x;
  ref_dstJoint.transform.rotation.y    = srcJoint.m_LocalTransform.m_qRotation.v.y;
  ref_dstJoint.transform.rotation.z    = srcJoint.m_LocalTransform.m_qRotation.v.z;
  ref_dstJoint.transform.rotation.w    = srcJoint.m_LocalTransform.m_qRotation.w;
  ref_dstJoint.transform.scale.x       = srcJoint.m_LocalTransform.m_vScale.x;
  ref_dstJoint.transform.scale.y       = srcJoint.m_LocalTransform.m_vScale.y;
  ref_dstJoint.transform.scale.z       = srcJoint.m_LocalTransform.m_vScale.z;

  ref_dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (xiiUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], ref_dstJoint.children[b]);
  }
}

void xiiEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const
{
  out_skeleton.roots.resize((size_t)m_Children.GetCount());

  for (xiiUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_skeleton.roots[b]);
  }
}

void xiiEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto                                     pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  xiiOzzUtils::CopySkeleton(&out_skeleton, pNewOzzSkeleton.get());
}

xiiEditableSkeletonJoint::xiiEditableSkeletonJoint() = default;

xiiEditableSkeletonJoint::~xiiEditableSkeletonJoint()
{
  ClearJoints();
}

const char* xiiEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void xiiEditableSkeletonJoint::SetName(const char* szSz)
{
  m_sName.Assign(szSz);
}

void xiiEditableSkeletonJoint::ClearJoints()
{
  for (xiiEditableSkeletonJoint* pChild : m_Children)
  {
    XII_DEFAULT_DELETE(pChild);
  }
  m_Children.Clear();
}

void xiiEditableSkeletonJoint::CopyPropertiesFrom(const xiiEditableSkeletonJoint* pJoint)
{
  // copy existing (user edited) properties from pJoint into this joint
  // which has just been imported from file

  // do not copy:
  //  name
  //  transform
  //  children
  //  bone collider geometry (vertices, indices)

  // synchronize user config of bone colliders
  for (xiiUInt32 i = 0; i < m_BoneColliders.GetCount(); ++i)
  {
    auto& dst = m_BoneColliders[i];

    for (xiiUInt32 j = 0; j < pJoint->m_BoneColliders.GetCount(); ++j)
    {
      const auto& src = pJoint->m_BoneColliders[j];

      if (dst.m_sIdentifier == src.m_sIdentifier)
      {
        // dst.m_bOverrideSurface = src.m_bOverrideSurface;
        // dst.m_bOverrideCollisionLayer = src.m_bOverrideCollisionLayer;
        // dst.m_sSurfaceOverride = src.m_sSurfaceOverride;
        // dst.m_uiCollisionLayerOverride = src.m_uiCollisionLayerOverride;
        break;
      }
    }
  }

  m_BoneShapes            = pJoint->m_BoneShapes;
  m_qLocalJointRotation   = pJoint->m_qLocalJointRotation;
  m_JointType             = pJoint->m_JointType;
  m_SwingLimitY           = pJoint->m_SwingLimitY;
  m_SwingLimitZ           = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle   = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle = pJoint->m_TwistLimitCenterAngle;
  m_bLimitSwing           = pJoint->m_bLimitSwing;
  m_bLimitTwist           = pJoint->m_bLimitTwist;

  m_bOverrideSurface         = pJoint->m_bOverrideSurface;
  m_bOverrideCollisionLayer  = pJoint->m_bOverrideCollisionLayer;
  m_sSurfaceOverride         = pJoint->m_sSurfaceOverride;
  m_uiCollisionLayerOverride = pJoint->m_uiCollisionLayerOverride;
}

XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);
