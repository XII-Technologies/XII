/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;

/// Classifies the facing of an individual raycast hit
enum class xiiPhysicsHitType : xiiInt8
{
  Undefined         = -1, ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0,  ///< The raycast hit the front face of a triangle
  TriangleBackFace  = 1,  ///< The raycast hit the back face of a triangle
};

/// Used for raycast and sweep tests
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

/// Used to report overlap query results
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

/// Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
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
                               Rope,      ///< All shapes belonging to ropes.
                               Cloth,     ///< Soft-body shapes. Mainly for decorative purposes.
                               Debris     ///< Small stuff for visuals, but shouldn't affect the game. This will only have one-way interactions, ie get pushed, but won't push others.
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
