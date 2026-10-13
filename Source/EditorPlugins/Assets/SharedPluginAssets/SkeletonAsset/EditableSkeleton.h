/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <SharedPluginAssets/Common/MeshImportTransform.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

struct xiiSkeletonResourceDescriptor;

/// Authoring metadata retained in skeleton documents. These types do not describe runtime physics objects.
struct xiiSkeletonJointGeometryType
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,
    ConvexMesh,
    Default = None
  };
};

struct xiiSkeletonJointType
{
  using StorageType = xiiUInt8;
  enum Enum
  {
    None = 0,
    Fixed,
    SwingTwist,
    Default = None
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiSkeletonJointGeometryType);
XII_DECLARE_REFLECTABLE_TYPE(XII_SHAREDPLUGINASSETS_DLL, xiiSkeletonJointType);

struct XII_SHAREDPLUGINASSETS_DLL xiiEditableSkeletonBoneShape : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonBoneShape, xiiReflectedClass);

  xiiEnum<xiiSkeletonJointGeometryType> m_Geometry;
  xiiVec3                               m_vOffset    = xiiVec3::MakeZero();
  xiiQuat                               m_qRotation  = xiiQuat::MakeIdentity();
  float                                 m_fLength    = 0; ///< Box / capsule length. Zero requests the distance to the parent joint.
  float                                 m_fWidth     = 0;
  float                                 m_fThickness = 0; ///< Sphere / capsule radius, or box depth.
};

struct XII_SHAREDPLUGINASSETS_DLL xiiEditableSkeletonBoneCollider : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonBoneCollider, xiiReflectedClass);

  xiiString                  m_sIdentifier;
  xiiDynamicArray<xiiVec3>   m_VertexPositions;
  xiiDynamicArray<xiiUInt32> m_TriangleIndices;
};

/// A document-owned joint. Names, reflected property names and versions preserve existing skeleton assets.
/// Children have exclusive ownership; copying the owning hierarchy is deliberately disabled.
class XII_SHAREDPLUGINASSETS_DLL xiiEditableSkeletonJoint : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeletonJoint, xiiReflectedClass);

public:
  xiiEditableSkeletonJoint() = default;
  ~xiiEditableSkeletonJoint();
  xiiEditableSkeletonJoint(const xiiEditableSkeletonJoint&) = delete;
  void operator=(const xiiEditableSkeletonJoint&)           = delete;

  const char* GetName() const;
  void        SetName(const char* szName);
  void        ClearJoints();

  /// Copies user settings, leaving the imported name, transform, children and convex geometry intact.
  void CopyPropertiesFrom(const xiiEditableSkeletonJoint* pJoint);

  xiiHashedString                                  m_sName;
  xiiTransform                                     m_LocalTransform = xiiTransform::MakeIdentity();
  xiiEnum<xiiSkeletonJointType>                    m_JointType;
  float                                            m_fStiffness = 0;
  xiiAngle                                         m_TwistLimitHalfAngle;
  xiiAngle                                         m_TwistLimitCenterAngle;
  xiiAngle                                         m_SwingLimitY;
  xiiAngle                                         m_SwingLimitZ;
  xiiVec3                                          m_vGizmoOffsetPositionRO = xiiVec3::MakeZero();
  xiiQuat                                          m_qGizmoOffsetRotationRO = xiiQuat::MakeIdentity();
  xiiQuat                                          m_qLocalJointRotation    = xiiQuat::MakeIdentity();
  xiiHybridArray<xiiEditableSkeletonJoint*, 4>     m_Children;
  xiiHybridArray<xiiEditableSkeletonBoneShape, 1>  m_BoneShapes;
  xiiDynamicArray<xiiEditableSkeletonBoneCollider> m_BoneColliders;
  bool                                             m_bOverrideSurface        = false;
  bool                                             m_bOverrideCollisionLayer = false;
  xiiString                                        m_sSurfaceOverride;
  xiiUInt8                                         m_uiCollisionLayerOverride = 0;
};

/// Shared, Qt-independent skeleton document model. Runtime resources contain only native joint / rest-pose data.
/// Collider, constraint, import and preview settings remain authoring data until a runtime consumer supports them.
class XII_SHAREDPLUGINASSETS_DLL xiiEditableSkeleton : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditableSkeleton, xiiReflectedClass);

public:
  xiiEditableSkeleton() = default;
  ~xiiEditableSkeleton();
  xiiEditableSkeleton(const xiiEditableSkeleton&) = delete;
  void operator=(const xiiEditableSkeleton&)      = delete;

  void      ClearJoints();
  xiiResult GetRootTransform(xiiTransform& out_transform) const;

  /// Produces native parent-before-child joints, baking import coordinates into each root local pose.
  /// Rejects malformed hierarchies, duplicate / empty names and non-invertible or non-finite poses.
  /// Iterative conversion and destruction support deep hierarchies. Failure clears out_descriptor.
  xiiResult FillResourceDescriptor(xiiSkeletonResourceDescriptor& out_descriptor) const;

  /// Replaces the hierarchy with imported joints, preserving document settings and per-joint user properties by name.
  /// The document's import coordinates determine manipulator frames. Imported transforms and collider meshes win.
  /// Success transfers joint ownership and empties newSkeleton; failure leaves this document unchanged.
  xiiResult MergeImportedHierarchy(xiiEditableSkeleton& newSkeleton);

  xiiString                                    m_sSourceFile;
  xiiString                                    m_sPreviewMesh;
  xiiString                                    m_sSurfaceFile;
  xiiUInt8                                     m_uiCollisionLayer = 0;
  float                                        m_fUniformScaling  = 1;
  float                                        m_fMaxImpulse      = 100;
  xiiEnum<xiiMeshImportTransform>              m_ImportTransform;
  xiiEnum<xiiBasisAxis>                        m_RightDir        = xiiBasisAxis::NegativeX;
  xiiEnum<xiiBasisAxis>                        m_UpDir           = xiiBasisAxis::PositiveY;
  bool                                         m_bFlipForwardDir = false;
  xiiEnum<xiiBasisAxis>                        m_BoneDirection   = xiiBasisAxis::PositiveY;
  xiiHybridArray<xiiEditableSkeletonJoint*, 4> m_Children;
  xiiString                                    m_sLeftFootJoint;
  xiiString                                    m_sRightFootJoint;
};
