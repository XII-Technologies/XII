#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>
#include <GraphicsCore/Declarations.h>

class xiiSkeletonBuilder;
class xiiSkeleton;

namespace ozz::animation
{
  class Skeleton;

  namespace offline
  {
    struct RawSkeleton;
  }
} // namespace ozz::animation

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSkeletonJointGeometryType);

struct XII_GRAPHICSCORE_DLL xiiEditableSkeletonBoneShape : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonBoneShape, xiiReflectedClass);

  xiiEnum<xiiSkeletonJointGeometryType> m_Geometry;

  xiiVec3 m_vOffset   = xiiVec3::MakeZero();
  xiiQuat m_qRotation = xiiQuat::MakeIdentity();

  float m_fLength    = 0; // Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth     = 0; // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius
};

struct XII_GRAPHICSCORE_DLL xiiEditableSkeletonBoneCollider : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonBoneCollider, xiiReflectedClass);

  xiiString                 m_sIdentifier;
  xiiDynamicArray<xiiVec3>  m_VertexPositions;
  xiiDynamicArray<xiiUInt8> m_TriangleIndices;
};

class XII_GRAPHICSCORE_DLL xiiEditableSkeletonJoint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonJoint, xiiReflectedClass);

public:
  xiiEditableSkeletonJoint();
  ~xiiEditableSkeletonJoint();

  const char* GetName() const;
  void        SetName(const char* szSz);

  void ClearJoints();

  // copies the properties for geometry etc. from another joint
  // does NOT copy the name, the transform or the children
  void CopyPropertiesFrom(const xiiEditableSkeletonJoint* pJoint);

  xiiHashedString m_sName;
  xiiTransform    m_LocalTransform = xiiTransform::MakeIdentity();

  xiiEnum<xiiSkeletonJointType> m_JointType;

  float m_fStiffness = 0.0f;

  xiiAngle m_TwistLimitHalfAngle;
  xiiAngle m_TwistLimitCenterAngle;
  xiiAngle m_SwingLimitY;
  xiiAngle m_SwingLimitZ;

  xiiVec3 m_vGizmoOffsetPositionRO = xiiVec3::MakeZero();
  xiiQuat m_qGizmoOffsetRotationRO = xiiQuat::MakeIdentity();

  xiiQuat m_qLocalJointRotation = xiiQuat::MakeIdentity();

  xiiHybridArray<xiiEditableSkeletonJoint*, 4>     m_Children;
  xiiHybridArray<xiiEditableSkeletonBoneShape, 1>  m_BoneShapes;
  xiiDynamicArray<xiiEditableSkeletonBoneCollider> m_BoneColliders;

  bool      m_bOverrideSurface        = false;
  bool      m_bOverrideCollisionLayer = false;
  xiiString m_sSurfaceOverride;
  xiiUInt8  m_uiCollisionLayerOverride;
};

class XII_GRAPHICSCORE_DLL xiiEditableSkeleton : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeleton, xiiReflectedClass);

public:
  xiiEditableSkeleton();
  ~xiiEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(xiiSkeletonResourceDescriptor& ref_desc) const;
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const;
  void GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const;
  void CreateJointsRecursive(xiiSkeletonBuilder& ref_sb, xiiSkeletonResourceDescriptor& ref_desc, const xiiEditableSkeletonJoint* pParentJoint, const xiiEditableSkeletonJoint* pThisJoint, xiiUInt16 uiThisJointIdx, const xiiQuat& qParentAccuRot, const xiiMat4& mRootTransform) const;

  xiiString m_sSourceFile;
  xiiString m_sPreviewMesh;

  xiiString m_sSurfaceFile;
  xiiUInt8  m_uiCollisionLayer = 0;

  float m_fUniformScaling = 1.0f;
  float m_fMaxImpulse     = 100.0f;

  xiiEnum<xiiMeshImportTransform> m_ImportTransform;
  xiiEnum<xiiBasisAxis>           m_RightDir        = xiiBasisAxis::NegativeX;
  xiiEnum<xiiBasisAxis>           m_UpDir           = xiiBasisAxis::PositiveY;
  bool                            m_bFlipForwardDir = false;
  xiiEnum<xiiBasisAxis>           m_BoneDirection;

  xiiHybridArray<xiiEditableSkeletonJoint*, 4> m_Children;

  // used for motion extraction
  xiiString m_sLeftFootJoint;
  xiiString m_sRightFootJoint;
};

struct XII_GRAPHICSCORE_DLL xiiExposedBone
{
  xiiString    m_sName;
  xiiString    m_sParent;
  xiiTransform m_Transform;
  // when adding new values, the hash function below has to be adjusted
};

XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiExposedBone);

XII_GRAPHICSCORE_DLL void operator<<(xiiStreamWriter& inout_stream, const xiiExposedBone& bone);
XII_GRAPHICSCORE_DLL void operator>>(xiiStreamReader& inout_stream, xiiExposedBone& ref_bone);
XII_GRAPHICSCORE_DLL bool operator==(const xiiExposedBone& lhs, const xiiExposedBone& rhs);

template <>
struct xiiHashHelper<xiiExposedBone>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiExposedBone& value)
  {
    return xiiHashingUtils::xxHash32String(value.m_sName) + xiiHashingUtils::xxHash32String(value.m_sParent) + xiiHashingUtils::xxHash32(&value, sizeof(xiiTransform));
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiExposedBone& a, const xiiExposedBone& b) { return a == b; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiExposedBone);
