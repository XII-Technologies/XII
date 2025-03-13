#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class xiiWorld;
class xiiGameObject;

class XII_CORE_DLL xiiScriptExtensionClass_Physics
{
public:
  static xiiVec3 GetGravity(xiiWorld* pWorld);

  static xiiUInt8 GetCollisionLayerByName(xiiWorld* pWorld, xiiStringView sLayerName);

  static bool Raycast(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic, xiiUInt32 uiIgnoreObjectID = xiiInvalidIndex);

  static bool OverlapTestSphere(xiiWorld* pWorld, float fRadius, const xiiVec3& vPosition, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  static bool OverlapTestCapsule(xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& transform, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  static bool SweepTestSphere(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, const xiiVec3& vStart, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);

  static bool SweepTestCapsule(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& start, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Physics);
