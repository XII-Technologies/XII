/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Physics.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Physics, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetGravity, In, "World"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetCollisionLayerByName, In, "World", In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetWeightCategoryByName, In, "World", In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetImpulseTypeByName, In, "World", In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(Raycast, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Start", In, "Direction", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(new xiiFunctionArgumentAttributes(6, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(7, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic)), new xiiFunctionArgumentAttributes(8, new xiiDefaultValueAttribute((xiiInt32)xiiInvalidIndex))),
    XII_SCRIPT_FUNCTION_PROPERTY(OverlapTestLine, In, "World", In, "Start", In, "End", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic)),new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute((xiiInt32)xiiInvalidIndex))),
    XII_SCRIPT_FUNCTION_PROPERTY(OverlapTestSphere, In, "World", In, "Radius", In, "Position", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(OverlapTestCapsule, In, "World", In, "Radius", In, "Height", In, "Transform", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(4, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(SweepTestSphere, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(8, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(9, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(SweepTestCapsule, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Height", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(new xiiFunctionArgumentAttributes(9, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(10, new xiiDefaultValueAttribute((xiiInt32)xiiPhysicsShapeType::Static | (xiiInt32)xiiPhysicsShapeType::Dynamic))),
    XII_SCRIPT_FUNCTION_PROPERTY(RaycastSurfaceInteraction, In, "World", In, "RayStart", In, "RayDirection", In, "CollisionLayer", In, "ShapeTypes", In, "FallbackSurface", In, "Interaction", In, "Impulse", In, "IgnoreObjectID")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiDynamicEnumAttribute("PhysicsCollisionLayer")), new xiiFunctionArgumentAttributes(7, new xiiDefaultValueAttribute(0.0f)), new xiiFunctionArgumentAttributes(8, new xiiDefaultValueAttribute((xiiInt32)xiiInvalidIndex))),
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

xiiUInt8 xiiScriptExtensionClass_Physics::GetWeightCategoryByName(xiiWorld* pWorld, xiiStringView sCategoryName)
{
  if (xiiPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<xiiPhysicsWorldModuleInterface>())
  {
    return static_cast<xiiUInt8>(pInterface->GetWeightCategoryByName(sCategoryName));
  }

  return 255;
}

xiiUInt8 xiiScriptExtensionClass_Physics::GetImpulseTypeByName(xiiWorld* pWorld, xiiStringView sImpulseTypeName)
{
  if (xiiPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<xiiPhysicsWorldModuleInterface>())
  {
    return static_cast<xiiUInt8>(pInterface->GetImpulseTypeByName(sImpulseTypeName));
  }

  return 255;
}

bool xiiScriptExtensionClass_Physics::Raycast(xiiVec3& out_vHitPosition, xiiVec3& out_vHitNormal, xiiGameObjectHandle& out_hHitObject, xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vDirection, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/, xiiUInt32 uiIgnoreObjectID)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes             = shapeTypes;
    params.m_uiCollisionLayer       = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap  = true;

    if (pModule->Raycast(res, vStart, vDirection, 1.0f, params))
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

bool xiiScriptExtensionClass_Physics::OverlapTestLine(xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes /*= xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic*/, xiiUInt32 uiIgnoreObjectID /*= xiiInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes             = shapeTypes;
    params.m_uiCollisionLayer       = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap  = true;

    xiiVec3     vDirection = vEnd - vStart;
    const float fDistance  = vDirection.GetLengthAndNormalize();

    if (pModule->Raycast(res, vStart, vDirection, fDistance, params))
    {
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

bool xiiScriptExtensionClass_Physics::RaycastSurfaceInteraction(xiiWorld* pWorld, const xiiVec3& vRayStart, const xiiVec3& vRayDirection, xiiUInt8 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, xiiStringView sFallbackSurface, const xiiTempHashedString& sInteraction, float fInteractionImpulse, xiiUInt32 uiIgnoreObjectID /*= xiiInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>())
  {
    xiiPhysicsCastResult      res;
    xiiPhysicsQueryParameters params;
    params.m_ShapeTypes             = shapeTypes;
    params.m_uiCollisionLayer       = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap  = true;

    if (pModule->Raycast(res, vRayStart, vRayDirection, 1.0f, params))
    {
      xiiSurfaceResourceHandle hSurface = res.m_hSurface;
      if (!hSurface.IsValid() && !sFallbackSurface.IsEmpty())
      {
        hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(sFallbackSurface);
      }

      if (hSurface.IsValid())
      {
        xiiResourceLock<xiiSurfaceResource> pSurf(hSurface, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pSurf.GetAcquireResult() == xiiResourceAcquireResult::Final)
        {
          return pSurf->InteractWithSurface(pWorld, {}, res.m_vPosition, res.m_vNormal, vRayDirection, sInteraction, nullptr, fInteractionImpulse);
        }
      }
    }
  }

  return false;
}

XII_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Physics);
