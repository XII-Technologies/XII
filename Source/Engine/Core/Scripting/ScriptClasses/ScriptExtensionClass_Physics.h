#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class xiiWorld;
class xiiGameObject;

/// Script extension class providing physics world queries and utilities for scripts.
///
/// Exposes physics system functionality to scripts including collision detection, raycasting, and shape overlap testing. All functions require a valid world and may return no results if no physics world module is active.
class XII_CORE_DLL xiiScriptExtensionClass_Physics
{
public:
  /// Gets the current gravity vector for the physics world.
  static xiiVec3 GetGravity(xiiWorld* pWorld);

  /// Finds collision layer index by name, returns invalid index if not found.
  static xiiUInt8 GetCollisionLayerByName(xiiWorld* pWorld, xiiStringView sLayerName);

  /// Finds weight category index by name, returns invalid key if not found.
  static xiiUInt8 GetWeightCategoryByName(xiiWorld* pWorld, xiiStringView sCategoryName);

  /// Finds impulse type index by name, returns invalid key if not found.
  static xiiUInt8 GetImpulseTypeByName(xiiWorld* pWorld, xiiStringView sImpulseTypeName);

  /// Performs raycast and returns hit information if collision is found.
  static bool Raycast(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vDirection, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic, xiiUInt32 uiIgnoreObjectID = xiiInvalidIndex);

  /// Tests if a line segment intersects with any physics shapes.
  static bool OverlapTestLine(xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic, xiiUInt32 uiIgnoreObjectID = xiiInvalidIndex);

  /// Tests if a sphere at the given position overlaps with any physics shapes.
  static bool OverlapTestSphere(xiiWorld* pWorld, float fRadius, const xiiVec3& vPosition, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  /// Tests if a capsule with the given transform overlaps with any physics shapes.
  static bool OverlapTestCapsule(xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& transform, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  /// Sweeps a sphere along a direction and returns hit information if collision is found.
  static bool SweepTestSphere(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, const xiiVec3& vStart, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  /// Sweeps a capsule along a direction and returns hit information if collision is found.
  static bool SweepTestCapsule(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& start, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  /// Performs raycast and triggers surface interaction at hit point if collision is found.
  static bool RaycastSurfaceInteraction(xiiWorld* pWorld, const xiiVec3& vRayStart, const xiiVec3& vRayDirection, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, xiiStringView sFallbackSurface, const xiiTempHashedString& sInteraction, float fInteractionImpulse, xiiUInt32 uiIgnoreObjectID = xiiInvalidIndex);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Physics);
