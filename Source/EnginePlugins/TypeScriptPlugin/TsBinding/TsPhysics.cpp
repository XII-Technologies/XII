#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Physics_Raycast(duk_context* pDuk);
static int __CPP_Physics_SweepTestSphere(duk_context* pDuk);
static int __CPP_Physics_SweepTestBox(duk_context* pDuk);
static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk);
static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk);
static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk);
static int __CPP_Physics_GetGravity(duk_context* pDuk);
static int __CPP_Physics_QueryShapesInSphere(duk_context* pDuk);

#define GetPhysicsModule()                                     \
  pWorld->GetOrCreateModule<xiiPhysicsWorldModuleInterface>(); \
                                                               \
  if (pModule == nullptr)                                      \
  {                                                            \
    duk.Error("No Physics World Module available.");           \
    return 0;                                                  \
  }

xiiResult xiiTypeScriptBinding::Init_Physics()
{
  m_Duk.RegisterGlobalFunction("__CPP_Physics_Raycast", __CPP_Physics_Raycast, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestSphere", __CPP_Physics_SweepTestSphere, 7);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestBox", __CPP_Physics_SweepTestBox, 7);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_SweepTestCapsule", __CPP_Physics_SweepTestCapsule, 8);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestSphere", __CPP_Physics_OverlapTestSphere, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_OverlapTestCapsule", __CPP_Physics_OverlapTestCapsule, 6);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_GetGravity", __CPP_Physics_GetGravity, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Physics_QueryShapesInSphere", __CPP_Physics_QueryShapesInSphere, 6);

  return XII_SUCCESS;
}

static void PutHitResult(xiiDuktapeHelper& ref_duk, const xiiPhysicsCastResult& res)
{
  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(ref_duk);

  ref_duk.PushGlobalObject();                                       // [ global ]
  ref_duk.PushLocalObject("__Physics").IgnoreResult();              // [ global __Physics ]
  ref_duk.PushLocalObject("Physics").IgnoreResult();                // [ global __Physics Physics ]
  duk_get_prop_string(ref_duk, -1, "HitResult");                    // [ global __Physics Physics HitResult ]
  duk_new(ref_duk, 0);                                              // [ global __Physics Physics HitResultObj ]
  duk_remove(ref_duk, -2);                                          // [ global __Physics HitResultObj ]
  duk_remove(ref_duk, -2);                                          // [ global HitResultObj ]
  duk_remove(ref_duk, -2);                                          // [ HitResultObj ]
  ;                                                                 //
  ref_duk.SetNumberProperty("distance", res.m_fDistance, -1);       // [ HitResultObj ]
  ref_duk.SetNumberProperty("shapeId", res.m_uiObjectFilterID, -1); // [ HitResultObj ]
  ;                                                                 //
  xiiTypeScriptBinding::PushVec3(ref_duk, res.m_vPosition);         // [ HitResultObj pos ]
  ref_duk.SetCustomProperty("position", -1);                        // [ HitResultObj ]
  ;                                                                 //
  xiiTypeScriptBinding::PushVec3(ref_duk, res.m_vNormal);           // [ HitResultObj normal ]
  ref_duk.SetCustomProperty("normal", -1);                          // [ HitResultObj ]
  ;                                                                 //
  pBinding->DukPutGameObject(res.m_hShapeObject);                   // [ HitResultObj GO ]
  ref_duk.SetCustomProperty("shapeObject", -1);                     // [ HitResultObj ]
  ;                                                                 //
  pBinding->DukPutGameObject(res.m_hActorObject);                   // [ HitResultObj GO ]
  ref_duk.SetCustomProperty("actorObject", -1);                     // [ HitResultObj ]
}

static int __CPP_Physics_Raycast(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const xiiVec3                   vStart           = xiiTypeScriptBinding::GetVec3(duk, 0);
  xiiVec3                         vDir             = xiiTypeScriptBinding::GetVec3(duk, 1);
  const float                     fDistance        = duk.GetFloatValue(2);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(3);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(4));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(5);

  if (vDir.NormalizeIfNotZero(xiiVec3::UnitXAxis()).Failed())
  {
    xiiLog::Warning("TS: xii.Physics.Raycast() called with degenerate ray direction.");
    return duk.ReturnNull();
  }

  xiiPhysicsCastResult      res;
  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);
  queryParams.m_bIgnoreInitialOverlap = true;

  if (!pModule->Raycast(res, vStart, vDir, fDistance, queryParams))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestSphere(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float                     fRadius          = duk.GetFloatValue(0);
  const xiiVec3                   vStart           = xiiTypeScriptBinding::GetVec3(duk, 1);
  const xiiVec3                   vDir             = xiiTypeScriptBinding::GetVec3(duk, 2);
  const float                     fDistance        = duk.GetFloatValue(3);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(4);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(5));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(6);

  xiiPhysicsCastResult      res;
  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);

  if (!pModule->SweepTestSphere(res, fRadius, vStart, vDir, fDistance, queryParams))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestBox(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const xiiVec3                   vExtents         = xiiTypeScriptBinding::GetVec3(duk, 0);
  const xiiTransform              transform        = xiiTypeScriptBinding::GetTransform(duk, 1);
  const xiiVec3                   vDir             = xiiTypeScriptBinding::GetVec3(duk, 2);
  const float                     fDistance        = duk.GetFloatValue(3);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(4);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(5));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(6);

  xiiPhysicsCastResult      res;
  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);

  if (!pModule->SweepTestBox(res, vExtents, transform, vDir, fDistance, queryParams))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_SweepTestCapsule(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float                     fCapsuleRadius   = duk.GetFloatValue(0);
  const float                     fCapsuleHeight   = duk.GetFloatValue(1);
  const xiiTransform              transform        = xiiTypeScriptBinding::GetTransform(duk, 2);
  const xiiVec3                   vDir             = xiiTypeScriptBinding::GetVec3(duk, 3);
  const float                     fDistance        = duk.GetFloatValue(4);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(5);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(6));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(7);

  xiiPhysicsCastResult      res;
  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);

  if (!pModule->SweepTestCapsule(res, fCapsuleRadius, fCapsuleHeight, transform, vDir, fDistance, queryParams))
  {
    return duk.ReturnNull();
  }

  PutHitResult(duk, res);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Physics_OverlapTestSphere(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float                     fRadius          = duk.GetFloatValue(0);
  const xiiVec3                   vPos             = xiiTypeScriptBinding::GetVec3(duk, 1);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(2);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(3));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(4);

  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);
  return duk.ReturnBool(pModule->OverlapTestSphere(fRadius, vPos, queryParams));
}

static int __CPP_Physics_OverlapTestCapsule(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float                     fCapsuleRadius   = duk.GetFloatValue(0);
  const float                     fCapsuleHeight   = duk.GetFloatValue(1);
  const xiiTransform              transform        = xiiTypeScriptBinding::GetTransform(duk, 2);
  const xiiUInt32                 uiCollisionLayer = duk.GetUIntValue(3);
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(4));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(5);

  xiiPhysicsQueryParameters queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);
  return duk.ReturnBool(pModule->OverlapTestCapsule(fCapsuleRadius, fCapsuleHeight, transform, queryParams));
}

static int __CPP_Physics_GetGravity(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  xiiTypeScriptBinding::PushVec3(duk, pModule->GetGravity());
  return duk.ReturnCustom();
}

int __CPP_Physics_QueryShapesInSphere(duk_context* pDuk)
{
  duk_require_function(pDuk, -1);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiPhysicsWorldModuleInterface* pModule = GetPhysicsModule();

  const float                     fRadius          = duk.GetFloatValue(0);
  const xiiVec3                   vPos             = xiiTypeScriptBinding::GetVec3(duk, 1);
  const xiiUInt8                  uiCollisionLayer = static_cast<xiiUInt8>(duk.GetUIntValue(2));
  const xiiPhysicsShapeType::Enum shapeTypes       = static_cast<xiiPhysicsShapeType::Enum>(duk.GetUIntValue(3));
  const xiiUInt32                 uiIgnoreShapeID  = duk.GetIntValue(4);

  xiiPhysicsOverlapResultArray result;
  xiiPhysicsQueryParameters    queryParams(uiCollisionLayer, shapeTypes, uiIgnoreShapeID);

  pModule->QueryShapesInSphere(result, fRadius, vPos, queryParams);

  // forward the results via a callback
  for (const auto& res : result.m_Results)
  {
    duk_dup(pDuk, -1); // [ func ]

    pBinding->DukPutGameObject(res.m_hActorObject); // [ func go1 ]
    pBinding->DukPutGameObject(res.m_hShapeObject); // [ func go1 go2 ]
    duk.PushUInt(res.m_uiObjectFilterID);           // [ func go1 go2 sid ]

    duk_call(pDuk, 3); // [ result ]

    if (duk_get_boolean_default(pDuk, -1, false) == false)
    {
      duk_pop(pDuk); // [ ]
      break;
    }

    duk_pop(pDuk); // [ ]
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
