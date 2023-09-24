#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>

struct xiiGameObjectHandle;
struct xiiSkeletonResourceDescriptor;

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;

/// \brief Classifies the facing of an individual raycast hit
enum class xiiPhysicsHitType : int8_t
{
  Undefined         = -1, ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0,  ///< The raycast hit the front face of a triangle
  TriangleBackFace  = 1,  ///< The raycast hit the back face of a triangle
};

/// \brief Used for raycast and seep tests
struct xiiPhysicsCastResult
{
  xiiVec3 m_vPosition;
  xiiVec3 m_vNormal;
  float   m_fDistance;

  xiiGameObjectHandle      m_hShapeObject;                                    ///< The game object to which the hit physics shape is attached.
  xiiGameObjectHandle      m_hActorObject;                                    ///< The game object to which the parent actor of the hit physics shape is attached.
  xiiSurfaceResourceHandle m_hSurface;                                        ///< The type of surface that was hit (if available)
  xiiUInt32                m_uiObjectFilterID = xiiInvalidIndex;              ///< An ID either per object (rigid-body / ragdoll) or per shape (implementation specific) that can be used to ignore this object during raycasts and shape queries.
  xiiPhysicsHitType        m_hitType          = xiiPhysicsHitType::Undefined; ///< Classification of the triangle face, see xiiPhysicsHitType

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct xiiPhysicsCastResultArray
{
  xiiHybridArray<xiiPhysicsCastResult, 16> m_Results;
};

/// \brief Used to report overlap query results
struct xiiPhysicsOverlapResult
{
  XII_DECLARE_POD_TYPE();

  xiiGameObjectHandle m_hShapeObject;                       ///< The game object to which the hit physics shape is attached.
  xiiGameObjectHandle m_hActorObject;                       ///< The game object to which the parent actor of the hit physics shape is attached.
  xiiUInt32           m_uiObjectFilterID = xiiInvalidIndex; ///< The shape id of the hit physics shape
  xiiVec3             m_vCenterPosition;                    ///< The center position of the reported object in world space.

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct xiiPhysicsOverlapResultArray
{
  xiiHybridArray<xiiPhysicsOverlapResult, 16> m_Results;
};

struct xiiPhysicsTriangle
{
  xiiVec3                   m_Vertices[3];
  const xiiSurfaceResource* m_pSurface = nullptr;
};

/// \brief Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
///
/// This is mainly for optimization purposes. It is up to the physics integration to support some or all of these flags.
///
/// Note: If this is modified, 'Physics.ts' also has to be updated.
XII_DECLARE_FLAGS_WITH_DEFAULT(xiiUInt32, xiiPhysicsShapeType, 0xFFFFFFFF,
                               Static,    ///< Static geometry
                               Dynamic,   ///< Dynamic and kinematic objects
                               Query,     ///< Query shapes are kinematic bodies that don't participate in the simulation and are only used for raycasts and other queries.
                               Trigger,   ///< Trigger shapes
                               Character, ///< Shapes associated with character controllers.
                               Ragdoll,   ///< All shapes belonging to ragdolls.
                               Rope       ///< All shapes belonging to ropes.
);

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiPhysicsShapeType);

struct xiiPhysicsQueryParameters
{
  xiiPhysicsQueryParameters() = default;
  explicit xiiPhysicsQueryParameters(xiiUInt32 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Default, xiiUInt32 uiIgnoreObjectFilterID = xiiInvalidIndex) :
    m_uiCollisionLayer(uiCollisionLayer), m_ShapeTypes(shapeTypes), m_uiIgnoreObjectFilterID(uiIgnoreObjectFilterID)
  {
  }

  xiiUInt32                        m_uiCollisionLayer       = 0;
  xiiBitflags<xiiPhysicsShapeType> m_ShapeTypes             = xiiPhysicsShapeType::Default;
  xiiUInt32                        m_uiIgnoreObjectFilterID = xiiInvalidIndex;
  bool                             m_bIgnoreInitialOverlap  = false;
};

enum class xiiPhysicsHitCollection
{
  Closest,
  Any
};

class XII_CORE_DLL xiiPhysicsWorldModuleInterface : public xiiWorldModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPhysicsWorldModuleInterface, xiiWorldModule);

protected:
  xiiPhysicsWorldModuleInterface(xiiWorld* pWorld) :
    xiiWorldModule(pWorld)
  {
  }

public:
  /// \brief Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns xiiInvalidIndex if no such collision layer exists.
  virtual xiiUInt32 GetCollisionLayerByName(xiiStringView sName) const = 0;

  virtual bool Raycast(xiiPhysicsCastResult& out_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(xiiPhysicsCastResultArray& out_results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(xiiPhysicsCastResult& out_result, float fSphereRadius, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(xiiPhysicsCastResult& out_result, xiiVec3 vBoxExtends, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(xiiPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_results, float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const = 0;

  virtual xiiVec3 GetGravity() const = 0;

  virtual void QueryGeometryInBox(const xiiPhysicsQueryParameters& params, xiiBoundingBox box, xiiDynamicArray<xiiPhysicsTriangle>& out_triangles) const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// \brief Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(xiiGameObject* pOwner, xiiVec3 vBoxSize) {}

  struct JointConfig
  {
    xiiGameObjectHandle m_hActorA;
    xiiGameObjectHandle m_hActorB;
    xiiTransform        m_LocalFrameA = xiiTransform::IdentityTransform();
    xiiTransform        m_LocalFrameB = xiiTransform::IdentityTransform();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// \brief Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(xiiGameObject* pOwner, const xiiPhysicsWorldModuleInterface::FixedJointConfig& cfg) {}
};

/// \brief Used to apply a physical impulse on the object
struct XII_CORE_DLL xiiMsgPhysicsAddImpulse : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicsAddImpulse, xiiMessage);

  xiiVec3   m_vGlobalPosition;
  xiiVec3   m_vImpulse;
  xiiUInt32 m_uiObjectFilterID = xiiInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

/// \brief Used to apply a physical force on the object
struct XII_CORE_DLL xiiMsgPhysicsAddForce : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicsAddForce, xiiMessage);

  xiiVec3 m_vGlobalPosition;
  xiiVec3 m_vForce;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct XII_CORE_DLL xiiMsgPhysicsJointBroke : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgPhysicsJointBroke, xiiEventMessage);

  xiiGameObjectHandle m_hJointObject;
};

/// \brief Sent by components such as xiiJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct XII_CORE_DLL xiiMsgObjectGrabbed : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgObjectGrabbed, xiiMessage);

  xiiGameObjectHandle m_hGrabbedBy;
  bool                m_bGotGrabbed = true;
};

/// \brief Send this to components such as xiiJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct XII_CORE_DLL xiiMsgReleaseObjectGrab : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgReleaseObjectGrab, xiiMessage);

  xiiGameObjectHandle m_hGrabbedObjectToRelease;
};

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Communication/Message.h>

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

  /// \brief Append data to this description to add meshes to the automatic static mesh generation
  xiiSmcDescription* m_pStaticMeshDescription = nullptr;
};
