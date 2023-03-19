#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_IsValid(duk_context* pDuk);
static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_SetX_Float(duk_context* pDuk);
static int __CPP_GameObject_GetX_Float(duk_context* pDuk);
static int __CPP_GameObject_SetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_GetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_SetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_GetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_FindChildByName(duk_context* pDuk);
static int __CPP_GameObject_FindChildByPath(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk);
static int __CPP_GameObject_TryGetScriptComponent(duk_context* pDuk);
static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk);
static int __CPP_GameObject_SendMessage(duk_context* pDuk);
static int __CPP_GameObject_SendEventMessage(duk_context* pDuk);
static int __CPP_GameObject_SetString(duk_context* pDuk);
static int __CPP_GameObject_GetString(duk_context* pDuk);
static int __CPP_GameObject_SetTeamID(duk_context* pDuk);
static int __CPP_GameObject_GetTeamID(duk_context* pDuk);
static int __CPP_GameObject_ChangeTags(duk_context* pDuk);
static int __CPP_GameObject_CheckTags(duk_context* pDuk);
static int __CPP_GameObject_GetParent(duk_context* pDuk);
static int __CPP_GameObject_SetX_GameObject(duk_context* pDuk);
static int __CPP_GameObject_GetChildren(duk_context* pDuk);

namespace GameObject_X
{
  enum Enum
  {
    LocalPosition,
    GlobalPosition,
    LocalScaling,
    GlobalScaling,
    LocalUniformScaling,
    LocalRotation,
    GlobalRotation,
    GlobalDirForwards,
    GlobalDirRight,
    GlobalDirUp,
    Velocity,
    ActiveFlag,
    Active,
    ChildCount,
    SetParent,
    AddChild,
    DetachChild,
  };
}

xiiResult xiiTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsValid", __CPP_GameObject_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalUniformScaling", __CPP_GameObject_SetX_Float, 2, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalUniformScaling", __CPP_GameObject_GetX_Float, 1, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetActiveFlag", __CPP_GameObject_SetX_Bool, 2, GameObject_X::ActiveFlag);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetActiveFlag", __CPP_GameObject_GetX_Bool, 1, GameObject_X::ActiveFlag);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsActive", __CPP_GameObject_GetX_Bool, 1, GameObject_X::Active);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByName", __CPP_GameObject_FindChildByName, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByPath", __CPP_GameObject_FindChildByPath, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeName", __CPP_GameObject_TryGetComponentOfBaseTypeName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeNameHash", __CPP_GameObject_TryGetComponentOfBaseTypeNameHash, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetScriptComponent", __CPP_GameObject_TryGetScriptComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SearchForChildByNameSequence", __CPP_GameObject_SearchForChildByNameSequence, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendMessage", __CPP_GameObject_SendMessage, 5, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_PostMessage", __CPP_GameObject_SendMessage, 5, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendEventMessage", __CPP_GameObject_SendEventMessage, 5, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_PostEventMessage", __CPP_GameObject_SendEventMessage, 5, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirForwards", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirForwards);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirRight", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirRight);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirUp", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirUp);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetVelocity", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetVelocity", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::Velocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetName", __CPP_GameObject_SetString, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetName", __CPP_GameObject_GetString, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalKey", __CPP_GameObject_SetString, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalKey", __CPP_GameObject_GetString, 1, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetTeamID", __CPP_GameObject_SetTeamID, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetTeamID", __CPP_GameObject_GetTeamID, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetParent", __CPP_GameObject_GetParent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetParent", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::SetParent);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_AddChild", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::AddChild);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_DetachChild", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::DetachChild);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetChildCount", __CPP_GameObject_GetX_Float, 1, GameObject_X::ChildCount);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetChildren", __CPP_GameObject_GetChildren, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_SetTags", __CPP_GameObject_ChangeTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_AddTags", __CPP_GameObject_ChangeTags, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_RemoveTags", __CPP_GameObject_ChangeTags, 2);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAnyTags", __CPP_GameObject_CheckTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAllTags", __CPP_GameObject_CheckTags, 1);

  return XII_SUCCESS;
}

xiiGameObjectHandle xiiTypeScriptBinding::RetrieveGameObjectHandle(duk_context* pDuk, xiiInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return xiiGameObjectHandle();

  if (duk_get_prop_index(pDuk, iObjIdx, xiiTypeScriptBindingIndexProperty::GameObjectHandle))
  {
    xiiGameObjectHandle hObject = *reinterpret_cast<xiiGameObjectHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    return hObject;
  }

  return xiiGameObjectHandle();
}

xiiGameObject* xiiTypeScriptBinding::ExpectGameObject(duk_context* pDuk, xiiInt32 iObjIdx /*= 0*/)
{
  xiiGameObjectHandle hObject = xiiTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(pDuk);

  xiiGameObject* pGameObject = nullptr;
  XII_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "Invalid xiiGameObject");

  return pGameObject;
}

bool xiiTypeScriptBinding::RegisterGameObject(xiiGameObjectHandle handle, xiiUInt32& out_uiStashIdx)
{
  if (handle.IsInvalidated())
    return false;

  xiiUInt32& uiStashIdx = m_GameObjectToStashIdx[handle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return true;
  }

  uiStashIdx = AcquireStashObjIndex();

  xiiDuktapeHelper duk(m_Duk);                                     // [ ]
  duk.PushGlobalObject();                                          // [ global ]
  XII_VERIFY(duk.PushLocalObject("__GameObject").Succeeded(), ""); // [ global __GameObject ]
  duk_get_prop_string(duk, -1, "GameObject");                      // [ global __GameObject GameObject ]

  duk_new(duk, 0); // [ global __GameObject object ]

  // set the xiiGameObjectHandle property
  {
    xiiGameObjectHandle* pHandleBuffer =
      reinterpret_cast<xiiGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(xiiGameObjectHandle))); // [ global __GameObject object buffer ]
    *pHandleBuffer = handle;
    duk_put_prop_index(duk, -2, xiiTypeScriptBindingIndexProperty::GameObjectHandle); // [ global __GameObject object ]
  }

  StoreReferenceInStash(duk, uiStashIdx); // [ global __GameObject object ]
  duk.PopStack(3);                        // [ ]

  out_uiStashIdx = uiStashIdx;
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
}

bool xiiTypeScriptBinding::DukPutGameObject(const xiiGameObjectHandle& hObject)
{
  xiiDuktapeHelper duk(m_Duk);

  xiiUInt32 uiStashIdx = 0;
  if (!RegisterGameObject(hObject, uiStashIdx))
  {
    duk.PushNull(); // [ null ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, DukPushStashObject(duk, uiStashIdx), +1);
}

void xiiTypeScriptBinding::DukPutGameObject(const xiiGameObject* pObject)
{
  if (pObject == nullptr)
  {
    duk_push_null(m_Duk);
  }
  else
  {
    DukPutGameObject(pObject->GetHandle());
  }
}

static int __CPP_GameObject_IsValid(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObjectHandle hObject = xiiTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  if (hObject.IsInvalidated())
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
  }

  xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(pDuk);

  xiiGameObject* pGameObject = nullptr;
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetObject(hObject, pGameObject)), +1);
}

static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    xiiLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const xiiVec3 value = xiiTypeScriptBinding::GetVec3(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      pGameObject->SetLocalPosition(value);
      break;

    case GameObject_X::GlobalPosition:
      pGameObject->SetGlobalPosition(value);
      break;

    case GameObject_X::LocalScaling:
      pGameObject->SetLocalScaling(value);
      break;

    case GameObject_X::GlobalScaling:
      pGameObject->SetGlobalScaling(value);
      break;

    case GameObject_X::Velocity:
      pGameObject->SetVelocity(value);
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiVec3 value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      value = pGameObject->GetLocalPosition();
      break;

    case GameObject_X::GlobalPosition:
      value = pGameObject->GetGlobalPosition();
      break;

    case GameObject_X::LocalScaling:
      value = pGameObject->GetLocalScaling();
      break;

    case GameObject_X::GlobalScaling:
      value = pGameObject->GetGlobalScaling();
      break;

    case GameObject_X::GlobalDirForwards:
      value = pGameObject->GetGlobalDirForwards();
      break;

    case GameObject_X::GlobalDirRight:
      value = pGameObject->GetGlobalDirRight();
      break;

    case GameObject_X::GlobalDirUp:
      value = pGameObject->GetGlobalDirUp();
      break;

    case GameObject_X::Velocity:
      value = pGameObject->GetVelocity();
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  xiiTypeScriptBinding::PushVec3(pDuk, value);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Float(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    xiiLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      pGameObject->SetLocalUniformScaling(duk.GetFloatValue(1));
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Float(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  float value = 0.0f;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      value = pGameObject->GetLocalUniformScaling();
      break;

    case GameObject_X::ChildCount:
      value = static_cast<float>(pGameObject->GetChildCount());
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnFloat(value), +1);
}

static int __CPP_GameObject_SetX_Quat(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    xiiLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const xiiQuat rot = xiiTypeScriptBinding::GetQuat(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      pGameObject->SetLocalRotation(rot);
      break;

    case GameObject_X::GlobalRotation:
      pGameObject->SetGlobalRotation(rot);
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Quat(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiQuat value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      value = pGameObject->GetLocalRotation();
      break;

    case GameObject_X::GlobalRotation:
      value = pGameObject->GetGlobalRotation();
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  xiiTypeScriptBinding::PushQuat(pDuk, value);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Bool(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const bool value = duk.GetBoolValue(1, true);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::ActiveFlag:
      pGameObject->SetActiveFlag(value);
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Bool(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  bool value = false;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::ActiveFlag:
      value = pGameObject->GetActiveFlag();
      break;

    case GameObject_X::Active:
      value = pGameObject->IsActive();
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(value), +1);
}

static int __CPP_GameObject_FindChildByName(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szName     = duk.GetStringValue(1);
  bool        bRecursive = duk.GetBoolValue(2, true);

  xiiGameObject* pChild = pGameObject->FindChildByName(xiiTempHashedString(szName), bRecursive);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_FindChildByPath(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szPath = duk.GetStringValue(1);

  xiiGameObject* pChild = pGameObject->FindChildByPath(szPath);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szTypeName = duk.GetStringValue(1);

  const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(szTypeName);
  if (pRtti == nullptr)
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  xiiComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const xiiRTTI* pRtti = xiiRTTI::FindTypeByNameHash32(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  xiiComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetScriptComponent(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);
  const char*    szTypeName  = duk.GetStringValue(1);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);

  xiiTypeScriptBinding::TsComponentTypeInfo info;
  if (pBinding->FindScriptComponentInfo(szTypeName, info).Succeeded())
  {
    xiiHybridArray<xiiTypeScriptComponent*, 8> components;
    pGameObject->TryGetComponentsOfBaseType(components);

    for (auto* pTs : components)
    {
      if (pTs->GetTypeScriptComponentGuid() == info.Key())
      {
        pBinding->DukPutComponentObject(pTs);
        goto found;
      }
    }
  }

  duk.PushNull();

found:
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const xiiUInt32 uiTypeNameHash = duk.GetUIntValue(2);

  const xiiRTTI* pRtti = nullptr;

  if (uiTypeNameHash != 0)
  {
    pRtti = xiiRTTI::FindTypeByNameHash32(uiTypeNameHash);
  }

  xiiGameObject* pObject = pGameObject->SearchForChildByNameSequence(duk.GetStringValue(1), pRtti);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SendMessage(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, xiiTime::Zero());

    if (duk.GetBoolValue(3))
      pGameObject->SendMessageRecursive(*pMsg);
    else
      pGameObject->SendMessage(*pMsg);

    if (duk.GetBoolValue(4)) // expect the message to have result values
    {
      // sync msg back to TS
      xiiTypeScriptBinding::SyncXIIObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const xiiTime delay = xiiTime::Seconds(duk.GetNumberValue(4));

    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);

    if (duk.GetBoolValue(3))
      pGameObject->PostMessageRecursive(*pMsg, delay);
    else
      pGameObject->PostMessage(*pMsg, delay);
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_SendEventMessage(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject*          pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);
  xiiTypeScriptComponent* pSender     = xiiTypeScriptBinding::ExpectComponent<xiiTypeScriptComponent>(duk, 3);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendEventMessage
  {
    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, xiiTime::Zero());

    pGameObject->SendEventMessage(xiiStaticCast<xiiEventMessage&>(*pMsg), pSender);

    if (duk.GetBoolValue(4)) // expect the message to have result values
    {
      // sync msg back to TS
      xiiTypeScriptBinding::SyncXIIObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostEventMessage
  {
    const xiiTime delay = xiiTime::Seconds(duk.GetNumberValue(4));

    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);

    pGameObject->PostEventMessage(xiiStaticCast<xiiEventMessage&>(*pMsg), pSender, delay);
  }

  return duk.ReturnVoid();
}


static int __CPP_GameObject_SetString(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      pGameObject->SetName(duk.GetStringValue(1));
      break;
    case 1:
      pGameObject->SetGlobalKey(duk.GetStringValue(1));
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetString(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiStringView res;

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      res = pGameObject->GetName();
      break;
    case 1:
      res = pGameObject->GetGlobalKey();
      break;
  }

  return duk.ReturnString(res);
}

static int __CPP_GameObject_SetTeamID(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  pGameObject->SetTeamID(static_cast<xiiUInt16>(duk.GetUIntValue(1)));

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetTeamID(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pGameObject->GetTeamID()), +1);
}

static int __CPP_GameObject_ChangeTags(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const xiiUInt32 uiMagic = duk.GetFunctionMagicValue();

  if (uiMagic == 0) // SetTags
  {
    pGameObject->SetTags(xiiTagSet());
  }

  for (xiiUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!xiiStringUtils::IsNullOrEmpty(szParam))
    {
      switch (uiMagic)
      {
        case 0: // SetTags
        case 1: // AddTags
          pGameObject->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag(szParam));
          break;

        case 2: // RemoveTags
          pGameObject->RemoveTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag(szParam));
          break;

        default:
          XII_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_GameObject_CheckTags(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const xiiUInt32 uiMagic = duk.GetFunctionMagicValue();

  bool result = (uiMagic == 1);

  for (xiiUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!xiiStringUtils::IsNullOrEmpty(szParam))
    {
      const bool bIsSet = pGameObject->GetTags().IsSetByName(szParam);

      if (uiMagic == 0) // HasAnyTag
      {
        if (bIsSet)
        {
          result = true;
          break;
        }
      }
      else if (uiMagic == 1) // HasAllTags
      {
        if (!bIsSet)
        {
          result = false;
          break;
        }
      }
      else
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(result), +1);
}

static int __CPP_GameObject_GetParent(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiGameObject* pParent = pGameObject->GetParent();

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pParent);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_GameObject(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  xiiGameObjectHandle hObject = xiiTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 1);

  const xiiGameObject::TransformPreservation preserve =
    duk.GetBoolValue(2) ? xiiGameObject::TransformPreservation::PreserveGlobal : xiiGameObject::TransformPreservation::PreserveLocal;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::SetParent:
      pGameObject->SetParent(hObject, preserve);
      break;

    case GameObject_X::AddChild:
      pGameObject->AddChild(hObject, preserve);
      break;

    case GameObject_X::DetachChild:
      pGameObject->DetachChild(hObject, preserve);
      break;
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_GameObject_GetChildren(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiTypeScriptBinding* pBinding    = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  xiiGameObject*        pGameObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const int arrayObj = duk_push_array(pDuk);

  int curIdx = 0;
  for (auto it = pGameObject->GetChildren(); it.IsValid(); ++it, ++curIdx)
  {
    pBinding->DukPutGameObject(it);
    duk_put_prop_index(pDuk, arrayObj, curIdx);
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}
