#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Physics.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Physics, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetGravity, In, "World"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetCollisionLayerByName, In, "World", In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(Raycast, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes( new xiiFunctionArgumentAttributes(7, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(8, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic)), new xiiFunctionArgumentAttributes(9, new xiiDefaultValueAttribute((xiiInt32)xiiInvalidIndex))),
    XII_SCRIPT_FUNCTION_PROPERTY(OverlapTestSphere, In, "World", In, "Radius", In, "Position", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(OverlapTestCapsule, In, "World", In, "Radius", In, "Height", In, "Transform", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(4, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(SweepTestSphere, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(8, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),new xiiFunctionArgumentAttributes(9, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(SweepTestCapsule, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Height", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(9, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),new xiiFunctionArgumentAttributes(10, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("Physics"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiVec3 xiiScriptExtensionClass_Physics::GetGravity(xiiWorld* pWorld)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    return pModule->GetGravity();
  }

  return xiiVec3::MakeZero();
}

xiiUInt8 xiiScriptExtensionClass_Physics::GetCollisionLayerByName(xiiWorld* pWorld, xiiStringView sLayerName)
{
  if (xiiPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<xiiPhysicsWorldModuleInterface>())
  {
    return static_cast<xiiUInt8>(pInterface->GetCollisionLayerByName(sLayerName));
  }

  return 0;
}

bool xiiScriptExtensionClass_Physics::Raycast(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/, xiiUInt32 uiIgnoreObjectID)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes             = shapeTypes;
    params.m_uiCollisionLayer       = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap  = true;

    if (pModule->Raycast(res, vStart, vDirection, fDistance, params))
    {
      // res.m_hSurface
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal   = res.m_vNormal;
      out_hHitObject   = res.m_hActorObject;
      return true;
    }
  }

  return false;
}

bool xiiScriptExtensionClass_Physics::OverlapTestSphere(xiiWorld* pWorld, float fRadius, const xiiVec3& vPosition, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes       = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestSphere(fRadius, vPosition, params);
  }

  return false;
}

bool xiiScriptExtensionClass_Physics::OverlapTestCapsule(xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& transform, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes       = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestCapsule(fRadius, fHeight, transform, params);
  }
  return false;
}

bool xiiScriptExtensionClass_Physics::SweepTestSphere(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, const xiiVec3& vStart, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes       = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestSphere(res, fRadius, vStart, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal   = res.m_vNormal;
      out_hHitObject   = res.m_hActorObject;
      return true;
    }
  }
  return false;
}

bool xiiScriptExtensionClass_Physics::SweepTestCapsule(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, float fRadius, float fHeight, const xiiTransform& start, const xiiVec3& vDirection, float fDistance, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes       = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestCapsule(res, fRadius, fHeight, start, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal   = res.m_vNormal;
      out_hHitObject   = res.m_hActorObject;
      return true;
    }
  }
  return false;
}
