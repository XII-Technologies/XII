/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>

struct xiiGameObjectHandle;
struct xiiSkeletonResourceDescriptor;

/// Interface for physics world modules that provide physics simulation and queries.
///
/// Physics world modules implement physics functionality for a world, including
/// collision detection, raycasting, and shape queries. Different physics engines
/// can provide their own implementations of this interface.
class XII_CORE_DLL xiiPhysicsWorldModuleInterface : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPhysicsWorldModuleInterface, xiiWorldModule);

protected:
  xiiPhysicsWorldModuleInterface(xiiWorld* pWorld) :
    xiiWorldModule(pWorld)
  {
  }

public:
  /// Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns xiiInvalidIndex if no such collision layer exists.
  virtual xiiUInt32 GetCollisionLayerByName(xiiStringView sName) const = 0;

  /// Searches for a weight category with the given name and returns its key.
  ///
  /// Returns xiiWeightCategoryConfig::InvalidKey if no such category exists.
  virtual xiiUInt8 GetWeightCategoryByName(xiiStringView sName) const = 0;

  /// Searches for an impulse type with the given name and returns its key.
  ///
  /// Returns xiiImpulseTypeConfig::InvalidKey if no such category exists.
  virtual xiiUInt8 GetImpulseTypeByName(xiiStringView sName) const = 0;

  virtual bool Raycast(xiiPhysicsCastResult& out_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(xiiPhysicsCastResultArray& out_results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(xiiPhysicsCastResult& out_result, float fSphereRadius, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(xiiPhysicsCastResult& out_result, const xiiVec3& vBoxExtents, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(xiiPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCylinder(xiiPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestBox(const xiiVec3& vBoxExtents, const xiiVec3& vPosition, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_results, float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInBox(xiiPhysicsOverlapResultArray& out_results, const xiiVec3& vBoxExtents, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCapsule(xiiPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCylinder(xiiPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual xiiVec3 GetGravity() const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(xiiGameObject* pOwner, xiiVec3 vBoxSize)
  {
    XII_IGNORE_UNUSED(pOwner);
    XII_IGNORE_UNUSED(vBoxSize);
  }

  struct JointConfig
  {
    xiiGameObjectHandle m_hActorA;
    xiiGameObjectHandle m_hActorB;
    xiiTransform        m_LocalFrameA = xiiTransform::MakeIdentity();
    xiiTransform        m_LocalFrameB = xiiTransform::MakeIdentity();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(xiiGameObject* pOwner, const xiiPhysicsWorldModuleInterface::FixedJointConfig& cfg)
  {
    XII_IGNORE_UNUSED(pOwner);
    XII_IGNORE_UNUSED(cfg);
  }

  /// Gets world space bounds of a physics object if its shape type is included in shapeTypes and its collision layer interacts with uiCollisionLayer.
  virtual xiiBoundingBoxSphere GetWorldSpaceBounds(xiiGameObject* pOwner, xiiUInt32 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
  {
    XII_IGNORE_UNUSED(pOwner);
    XII_IGNORE_UNUSED(uiCollisionLayer);
    XII_IGNORE_UNUSED(shapeTypes);
    XII_IGNORE_UNUSED(bIncludeChildObjects);
    return xiiBoundingBoxSphere::MakeInvalid();
  }
};

/// Used to apply a physical impulse on the object
struct XII_CORE_DLL xiiMsgPhysicsAddImpulse : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicsAddImpulse, xiiMessage);

  xiiVec3   m_vGlobalPosition;
  xiiVec3   m_vImpulse;
  xiiUInt8  m_uiImpulseType    = 0;
  xiiUInt32 m_uiObjectFilterID = xiiInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct XII_CORE_DLL xiiMsgPhysicsJointBroke : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicsJointBroke, xiiEventMessage);

  xiiGameObjectHandle m_hJointObject;
};

/// Sent by components such as xiiJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct XII_CORE_DLL xiiMsgObjectGrabbed : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgObjectGrabbed, xiiMessage);

  xiiGameObjectHandle m_hGrabbedBy;
  bool                m_bGotGrabbed = true;
};

/// Send this to components such as xiiJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct XII_CORE_DLL xiiMsgReleaseObjectGrab : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgReleaseObjectGrab, xiiMessage);

  xiiGameObjectHandle m_hGrabbedObjectToRelease;
};

/// Can be sent by character controllers to inform objects when a CC pushes into them.
///
/// Whether this message is sent, depends on the character controller implementation.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct XII_CORE_DLL xiiMsgPhysicCharacterContact : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicCharacterContact, xiiMessage);

  xiiComponentHandle m_hCharacter;
  xiiVec3            m_vGlobalPosition;
  xiiVec3            m_vNormal;
  xiiVec3            m_vCharacterVelocity;
  float              m_fImpact;
};

/// Sent to physics components that have contact reporting enabled (see xiiOnJoltContact::SendContactMsg).
///
/// Only sent for certain physics object combinations, e.g. debris doesn't trigger this.
/// The reported contact position and normal is an average of the contact manifold.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct XII_CORE_DLL xiiMsgPhysicContact : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicContact, xiiMessage);

  xiiVec3 m_vGlobalPosition;
  xiiVec3 m_vNormal;
  float   m_fImpactSqr;
};


//////////////////////////////////////////////////////////////////////////

struct XII_CORE_DLL xiiSmcTriangle
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiVertexIndices[3];
};

struct XII_CORE_DLL xiiSmcSubMesh
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiFirstTriangle = 0;
  xiiUInt32 m_uiNumTriangles  = 0;
  xiiUInt16 m_uiSurfaceIndex  = 0;
};

struct XII_CORE_DLL xiiSmcDescription
{
  xiiDeque<xiiVec3>        m_Vertices;
  xiiDeque<xiiSmcTriangle> m_Triangles;
  xiiDeque<xiiSmcSubMesh>  m_SubMeshes;
  xiiDeque<xiiString>      m_Surfaces;
};

struct XII_CORE_DLL xiiMsgBuildStaticMesh : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgBuildStaticMesh, xiiMessage);

  /// Append data to this description to add meshes to the automatic static mesh generation
  xiiSmcDescription* m_pStaticMeshDescription = nullptr;
};
