#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void xiiTypeScriptBinding::GenerateMessagesFile(const char* szFile)
{
  xiiStringBuilder sFileContent;

  sFileContent =
    R"(// AUTO-GENERATED FILE

import __Message = require("TypeScript/xii/Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

import __Vec2 = require("TypeScript/xii/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/xii/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/xii/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/xii/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/xii/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/xii/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/xii/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/xii/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/xii/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")


)";

  GenerateAllMessagesCode(sFileContent);

  xiiDeferredFileWriter file;
  file.SetOutput(szFile, true);

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()).IgnoreResult();

  if (file.Close().Failed())
  {
    xiiLog::Error("Failed to write file '{}'", szFile);
    return;
  }
}

static void CreateMessageTypeList(xiiSet<const xiiRTTI*>& found, xiiDynamicArray<const xiiRTTI*>& sorted, const xiiRTTI* pRtti)
{
  if (found.Contains(pRtti))
    return;

  if (!pRtti->IsDerivedFrom<xiiMessage>())
    return;

  if (pRtti == xiiGetStaticRTTI<xiiMessage>() || pRtti == xiiGetStaticRTTI<xiiEventMessage>())
    return;

  found.Insert(pRtti);
  CreateMessageTypeList(found, sorted, pRtti->GetParentType());

  sorted.PushBack(pRtti);
}

void xiiTypeScriptBinding::GenerateAllMessagesCode(xiiStringBuilder& out_Code)
{
  xiiSet<const xiiRTTI*>          found;
  xiiDynamicArray<const xiiRTTI*> sorted;
  sorted.Reserve(100);

  xiiHybridArray<const xiiRTTI*, 64> alphabetical;
  for (auto pRtti : xiiRTTI::GetAllTypesDerivedFrom(xiiGetStaticRTTI<xiiMessage>(), alphabetical, true))
  {
    if (pRtti == xiiGetStaticRTTI<xiiMessage>() || pRtti == xiiGetStaticRTTI<xiiEventMessage>())
      continue;

    CreateMessageTypeList(found, sorted, pRtti);
  }

  for (auto pRtti : sorted)
  {
    GenerateMessageCode(out_Code, pRtti);
  }
}

void xiiTypeScriptBinding::GenerateMessageCode(xiiStringBuilder& out_Code, const xiiRTTI* pRtti)
{
  xiiStringBuilder sType, sParentType;
  GetTsName(pRtti, sType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  public static GetTypeNameHash(): number { return {}; }\n", xiiHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()));
  out_Code.AppendFormat("  constructor() { super(); this.TypeNameHash = {}; }\n", xiiHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()));
  GenerateMessagePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

void xiiTypeScriptBinding::GenerateMessagePropertiesCode(xiiStringBuilder& out_Code, const xiiRTTI* pRtti)
{
  xiiStringBuilder sProp;
  xiiStringBuilder sDefault;

  for (xiiAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    const xiiRTTI* pPropType = pProp->GetSpecificType();

    xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pProp);

    const char* szTypeName = TsType(pMember->GetSpecificType());
    if (szTypeName == nullptr)
      continue;

    const xiiVariant defaultValue = xiiReflectionUtils::GetDefaultValue(pMember);
    GenerateConstructorString(sDefault, defaultValue);

    if (!sDefault.IsEmpty())
    {
      sProp.Format("  {0}: {1} = {2};\n", pMember->GetPropertyName(), szTypeName, sDefault);
    }
    else
    {
      sProp.Format("  {0}: {1};\n", pMember->GetPropertyName(), szTypeName);
    }

    out_Code.Append(sProp.GetView());
  }
}

void xiiTypeScriptBinding::InjectMessageImportExport(xiiStringBuilder& content, const char* szMessageFile)
{
  xiiSet<const xiiRTTI*>          found;
  xiiDynamicArray<const xiiRTTI*> sorted;
  sorted.Reserve(100);

  xiiHybridArray<const xiiRTTI*, 64> alphabetical;
  for (auto pRtti : xiiRTTI::GetAllTypesDerivedFrom(xiiGetStaticRTTI<xiiMessage>(), alphabetical, true))
  {
    CreateMessageTypeList(found, sorted, pRtti);
  }

  xiiStringBuilder sImportExport, sTypeName;

  sImportExport.Format(R"(import __AllMessages = require("{}")
)",
                       szMessageFile);

  for (const xiiRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0} = __AllMessages.{0};\n", sTypeName);
  }

  AppendToTextFile(content, sImportExport);
}

static xiiUniquePtr<xiiMessage> CreateMessage(xiiUInt32 uiTypeHash, const xiiRTTI*& pRtti)
{
  static xiiHashTable<xiiUInt32, const xiiRTTI*, xiiHashHelper<xiiUInt32>, xiiStaticAllocatorWrapper> MessageTypes;

  if (!MessageTypes.TryGetValue(uiTypeHash, pRtti))
  {
    MessageTypes[uiTypeHash] = nullptr;

    for (pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (xiiHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiTypeHash)
      {
        MessageTypes[uiTypeHash] = pRtti;
        break;
      }
    }
  }

  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  return pRtti->GetAllocator()->Allocate<xiiMessage>();
}

xiiUniquePtr<xiiMessage> xiiTypeScriptBinding::MessageFromParameter(duk_context* pDuk, xiiInt32 iObjIdx, xiiTime delay)
{
  xiiDuktapeHelper duk(pDuk);

  xiiUInt32 uiTypeNameHash = duk.GetUIntValue(iObjIdx);

  const xiiRTTI*           pRtti = nullptr;
  xiiUniquePtr<xiiMessage> pMsg  = CreateMessage(uiTypeNameHash, pRtti);

  if (pMsg != nullptr)
  {
    SyncTsObjectXIITsObject(pDuk, pRtti, pMsg.Borrow(), iObjIdx + 1);
  }
  else
  {
    xiiUInt32 uiMsgStashIdx = 0xFFFFFFFF;

    m_StashedMsgDelivery.SetCount(c_uiMaxMsgStash);
    const xiiTime tNow = m_pWorld->GetClock().GetAccumulatedTime();

    for (xiiUInt32 i = 0; i < c_uiMaxMsgStash; ++i)
    {
      m_uiNextStashMsgIdx++;

      if (m_uiNextStashMsgIdx >= c_uiLastStashMsgIdx)
        m_uiNextStashMsgIdx = c_uiFirstStashMsgIdx;

      if (m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] < tNow)
        goto found;
    }

    xiiLog::Error("Too many posted messages with large delay (> {}). DukTape stash is full.", c_uiMaxMsgStash);
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, nullptr, 0);

  found:

    m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] = tNow + delay + xiiTime::Milliseconds(50);

    {
      duk_dup(duk, iObjIdx + 1);                       // [ object ]
      StoreReferenceInStash(duk, m_uiNextStashMsgIdx); // [ object ]
      duk.PopStack();                                  // [ ]
    }

    pMsg                                = xiiGetStaticRTTI<xiiMsgTypeScriptMsgProxy>()->GetAllocator()->Allocate<xiiMsgTypeScriptMsgProxy>();
    xiiMsgTypeScriptMsgProxy* pTypedMsg = static_cast<xiiMsgTypeScriptMsgProxy*>(pMsg.Borrow());
    pTypedMsg->m_uiTypeNameHash         = uiTypeNameHash;
    pTypedMsg->m_uiStashIndex           = m_uiNextStashMsgIdx;
  }

  XII_DUK_VERIFY_STACK(duk, 0);
  return pMsg;
}

void xiiTypeScriptBinding::DukPutMessage(duk_context* pDuk, const xiiMessage& msg)
{
  xiiDuktapeHelper duk(pDuk);

  const xiiRTTI*   pRtti    = msg.GetDynamicRTTI();
  xiiStringBuilder sMsgName = pRtti->GetTypeName();
  sMsgName.TrimWordStart("xii");

  duk.PushGlobalObject();                              // [ global ]
  duk.PushLocalObject("__AllMessages").IgnoreResult(); // [ global __AllMessages ]
  duk_get_prop_string(duk, -1, sMsgName.GetData());    // [ global __AllMessages msgname ]
  duk_new(duk, 0);                                     // [ global __AllMessages msg ]
  duk_remove(duk, -2);                                 // [ global msg ]
  duk_remove(duk, -2);                                 // [ msg ]

  SyncXIIObjectToTsObject(pDuk, pRtti, &msg, -1);

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void xiiTypeScriptBinding::RegisterMessageHandlersForComponentType(const char* szComponent, const xiiUuid& componentType)
{
  xiiDuktapeHelper duk(m_Duk);

  m_CurrentTsMsgHandlerRegistrator = componentType;

  const xiiStringBuilder sCompModule("__", szComponent);

  duk.PushGlobalObject();                           // [ global ]
  if (duk.PushLocalObject(sCompModule).Succeeded()) // [ global __CompModule ]
  {
    if (duk.PushLocalObject(szComponent).Succeeded()) // [ global __CompModule obj ]
    {
      if (duk.PrepareObjectFunctionCall("RegisterMessageHandlers").Succeeded()) // [ global __CompModule obj func ]
      {
        duk.CallPreparedFunction().IgnoreResult(); // [ global __CompModule obj result ]
        duk.PopStack();                            // [ global __CompModule obj ]
      }

      duk.PopStack(); // [ global __CompModule ]
    }

    duk.PopStack(); // [ global ]
  }

  duk.PopStack(); // [ ]

  m_CurrentTsMsgHandlerRegistrator.SetInvalid();

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

int xiiTypeScriptBinding::__CPP_Binding_RegisterMessageHandler(duk_context* pDuk)
{
  xiiTypeScriptBinding* tsb = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  XII_ASSERT_DEV(tsb->m_CurrentTsMsgHandlerRegistrator.IsValid(), "'xii.TypescriptComponent.RegisterMessageHandler' may only be called from 'static RegisterMessageHandlers()'");

  xiiDuktapeFunction duk(pDuk);

  xiiUInt32   uiMsgTypeHash = duk.GetUIntValue(0);
  const char* szMsgHandler  = duk.GetStringValue(1);

  const xiiRTTI* pMsgType = xiiRTTI::FindTypeByNameHash32(uiMsgTypeHash);

  // this happens for pure TypeScript messages
  // if (pMsgType == nullptr)
  //{
  //  xiiLog::Error("Message with type name hash '{}' does not exist.", uiMsgTypeHash);
  //  return duk.ReturnVoid();
  //}

  auto& tsc = tsb->m_TsComponentTypes[tsb->m_CurrentTsMsgHandlerRegistrator];
  auto& mh  = tsc.m_MessageHandlers.ExpandAndGetRef();

  mh.m_sHandlerFunc          = szMsgHandler;
  mh.m_pMessageType          = pMsgType;
  mh.m_uiMessageTypeNameHash = uiMsgTypeHash;

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

bool xiiTypeScriptBinding::HasMessageHandler(const TsComponentTypeInfo& typeInfo, const xiiRTTI* pMsgRtti) const
{
  if (!typeInfo.IsValid())
    return false;

  for (auto& mh : typeInfo.Value().m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      return true;
    }
  }

  return false;
}

bool xiiTypeScriptBinding::DeliverMessage(const TsComponentTypeInfo& typeInfo, xiiTypeScriptComponent* pComponent, xiiMessage& msg, bool bSynchronizeAfterwards)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  if (tsc.m_MessageHandlers.IsEmpty())
    return false;

  const xiiRTTI* pMsgRtti = msg.GetDynamicRTTI();

  ++m_iMsgDeliveryRecursion;
  XII_SCOPE_EXIT(--m_iMsgDeliveryRecursion);

  xiiStringBuilder sStashMsgName;

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      xiiDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        xiiTypeScriptBinding::DukPutMessage(duk, msg); // [ comp func comp msg ]

        if (bSynchronizeAfterwards)
        {
          sStashMsgName.Format("xiiMsg-{}", m_iMsgDeliveryRecursion);

          duk.PushGlobalStash();                       // [ ... stash ]
          duk_dup(duk, -2);                            // [ ... stash msg ]
          duk_put_prop_string(duk, -2, sStashMsgName); // [ ... stash ]
          duk_pop(duk);                                // [ ... ]
        }

        duk.PushCustom();                        // [ comp func comp msg ]
        duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
        duk.PopStack(2);                         // [ ]

        if (bSynchronizeAfterwards)
        {
          duk.PushGlobalStash();                                                  // [ ... stash ]
          duk_get_prop_string(duk, -1, sStashMsgName);                            // [ ... stash msg ]
          xiiTypeScriptBinding::SyncTsObjectXIITsObject(duk, pMsgRtti, &msg, -1); // [ ... stash msg ]
          duk_pop_2(duk);                                                         // [ ... ]
        }

        XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        xiiLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, pMsgRtti->GetTypeName());

        // mh.m_pMessageType = nullptr;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        XII_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}

bool xiiTypeScriptBinding::DeliverTsMessage(const TsComponentTypeInfo& typeInfo, xiiTypeScriptComponent* pComponent, const xiiMsgTypeScriptMsgProxy& msg)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_uiMessageTypeNameHash == msg.m_uiTypeNameHash)
    {
      xiiDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        DukPushStashObject(duk, msg.m_uiStashIndex); // [ comp func comp msg ]
        duk.PushCustom();                            // [ comp func comp msg ]
        duk.CallPreparedMethod().IgnoreResult();     // [ comp result ]
        duk.PopStack(2);                             // [ ]

        XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        xiiLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, msg.m_uiTypeNameHash);

        // mh.m_uiMessageTypeNameHash = 0;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        XII_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}
