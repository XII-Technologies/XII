#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_IsValid(duk_context* pDuk);
static int __CPP_Component_GetUniqueID(duk_context* pDuk);
static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActiveFlag(duk_context* pDuk);
static int __CPP_Component_IsActive(duk_context* pDuk);
static int __CPP_Component_SendMessage(duk_context* pDuk);
static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk);
static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsValid", __CPP_Component_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetUniqueID", __CPP_Component_GetUniqueID, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SetActiveFlag", __CPP_Component_SetActiveFlag, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetActiveFlag", __CPP_Component_IsActive, 1, -1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActive", __CPP_Component_IsActive, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndInitialized", __CPP_Component_IsActive, 1, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndSimulating", __CPP_Component_IsActive, 1, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SendMessage", __CPP_Component_SendMessage, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_PostMessage", __CPP_Component_SendMessage, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_BroadcastEvent", __CPP_TsComponent_BroadcastEvent, 4);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_SetTickInterval", __CPP_TsComponent_SetTickInterval, 2);

  return XII_SUCCESS;
}

xiiResult xiiTypeScriptBinding::RegisterComponent(const char* szTypeName, xiiComponentHandle hHandle, xiiUInt32& out_uiStashIdx, bool bIsNativeComponent)
{
  if (hHandle.IsInvalidated())
    return XII_FAILURE;

  xiiUInt32& uiStashIdx = m_ComponentToStashIdx[hHandle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return XII_SUCCESS;
  }

  uiStashIdx = AcquireStashObjIndex();

  xiiDuktapeHelper duk(m_Duk);
  XII_DUK_VERIFY_STACK(duk, 0);

  duk.PushGlobalObject(); // [ global ]
  XII_DUK_VERIFY_STACK(duk, +1);

  xiiStringBuilder sTypeName = szTypeName;

  if (bIsNativeComponent)
  {
    sTypeName.TrimWordStart("xii");

    XII_SUCCEED_OR_RETURN(duk.PushLocalObject("__AllComponents")); // [ global __AllComponents ]
    XII_DUK_VERIFY_STACK(duk, +2);
  }
  else
  {
    const xiiStringBuilder sCompModule("__", sTypeName);

    XII_SUCCEED_OR_RETURN(duk.PushLocalObject(sCompModule)); // [ global __CompModule ]
    XII_DUK_VERIFY_STACK(duk, +2);
  }

  if (!duk_get_prop_string(duk, -1, sTypeName)) // [ global __CompModule sTypeName ]
  {
    duk.PopStack(3); // [ ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_FAILURE, 0);
  }

  duk_new(duk, 0); // [ global __CompModule object ]
  XII_DUK_VERIFY_STACK(duk, +3);

  // store C++ side component handle in obj as property
  {
    xiiComponentHandle* pBuffer =
      reinterpret_cast<xiiComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(xiiComponentHandle))); // [ global __CompModule object buffer ]
    *pBuffer = hHandle;
    duk_put_prop_index(duk, -2, xiiTypeScriptBindingIndexProperty::ComponentHandle); // [ global __CompModule object ]
  }

  XII_DUK_VERIFY_STACK(duk, +3);

  StoreReferenceInStash(duk, uiStashIdx); // [ global __CompModule object ]
  XII_DUK_VERIFY_STACK(duk, +3);

  duk.PopStack(3); // [ ]
  XII_DUK_VERIFY_STACK(duk, 0);

  out_uiStashIdx = uiStashIdx;

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_SUCCESS, 0);
}

void xiiTypeScriptBinding::DukPutComponentObject(xiiComponent* pComponent)
{
  if (pComponent == nullptr)
  {
    m_Duk.PushNull(); // [ null ]
  }
  else
  {
    xiiUInt32 uiStashIdx = 0;
    if (RegisterComponent(pComponent->GetDynamicRTTI()->GetTypeName(), pComponent->GetHandle(), uiStashIdx, true).Failed())
    {
      m_Duk.PushNull(); // [ null ]
      return;
    }

    DukPushStashObject(m_Duk, uiStashIdx);
  }
}

xiiComponentHandle xiiTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, xiiInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return xiiComponentHandle();

  xiiDuktapeHelper duk(pDuk);

  if (duk_get_prop_index(pDuk, iObjIdx, xiiTypeScriptBindingIndexProperty::ComponentHandle))
  {
    xiiComponentHandle hComponent = *reinterpret_cast<xiiComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, hComponent, 0);
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, xiiComponentHandle(), 0);
}

static int __CPP_Component_IsValid(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  if (!duk_get_prop_index(pDuk, 0, xiiTypeScriptBindingIndexProperty::ComponentHandle))
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), 1);
  }

  xiiComponentHandle hComponent = *reinterpret_cast<xiiComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  xiiComponent* pComponent = nullptr;
  xiiWorld*     pWorld     = xiiTypeScriptBinding::RetrieveWorld(pDuk);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetComponent(hComponent, pComponent)), 1);
}

static int __CPP_Component_GetUniqueID(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pComponent->GetUniqueID()), +1);
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pComponent->GetOwner()->GetHandle());

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Component_SetActiveFlag(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  pComponent->SetActiveFlag(duk.GetBoolValue(1, true));

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_Component_IsActive(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case -1:
      XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->GetActiveFlag()), +1);

    case 0:
      XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActive()), +1);

    case 1:
      XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndInitialized()), +1);

    case 2:
      XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndSimulating()), +1);
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
}

static int __CPP_Component_SendMessage(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(duk, 0 /*this*/);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, xiiTime::Zero());
    pComponent->SendMessage(*pMsg);

    if (duk.GetBoolValue(3)) // expect the message to have result values
    {
      // sync msg back to TS
      xiiTypeScriptBinding::SyncXIIObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const xiiTime delay = xiiTime::Seconds(duk.GetNumberValue(3));

    xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);
    pComponent->PostMessage(*pMsg, delay);
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiTypeScriptComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiTypeScriptComponent>(duk, 0 /*this*/);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);

  xiiUniquePtr<xiiMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, xiiTime::Zero());
  pComponent->BroadcastEventMsg(xiiStaticCast<xiiEventMessage&>(*pMsg));

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiTypeScriptComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiTypeScriptComponent>(duk, 0 /*this*/);

  const xiiTime interval = xiiTime::Seconds(duk.GetFloatValue(1, 0.0f));
  pComponent->SetUpdateInterval(interval);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
