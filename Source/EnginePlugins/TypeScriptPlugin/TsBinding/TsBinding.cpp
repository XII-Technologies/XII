#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static xiiHashTable<duk_context*, xiiTypeScriptBinding*> s_DukToBinding;

xiiSet<const xiiRTTI*> xiiTypeScriptBinding::s_RequiredEnums;
xiiSet<const xiiRTTI*> xiiTypeScriptBinding::s_RequiredFlags;

static int __CPP_Time_Get(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      return duk.ReturnFloat(xiiTime::Now().AsFloatInSeconds());

    case 1:
    {
      xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetAccumulatedTime().AsFloatInSeconds());
    }

    case 2:
    {
      xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetTimeDiff().AsFloatInSeconds());
    }
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return duk.ReturnFloat(0.0f);
}

xiiTypeScriptBinding::xiiTypeScriptBinding() :
  m_Duk("Typescript Binding")
{
  s_DukToBinding[m_Duk] = this;
}

xiiTypeScriptBinding::~xiiTypeScriptBinding()
{
  if (!m_CVars.IsEmpty())
  {
    m_CVars.Clear();

    xiiCVar::ListOfCVarsChanged("invalid");
  }

  s_DukToBinding.Remove(m_Duk);
}

xiiResult xiiTypeScriptBinding::Init_Time()
{
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetRealTime", __CPP_Time_Get, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTime", __CPP_Time_Get, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTimeDiff", __CPP_Time_Get, 0, 2);

  return XII_SUCCESS;
}

xiiTypeScriptBinding* xiiTypeScriptBinding::RetrieveBinding(duk_context* pDuk)
{
  xiiTypeScriptBinding* pBinding = nullptr;
  s_DukToBinding.TryGetValue(pDuk, pBinding);
  return pBinding;
}

xiiResult xiiTypeScriptBinding::Initialize(xiiWorld& ref_world)
{
  if (m_bInitialized)
    return XII_SUCCESS;

  XII_LOG_BLOCK("Initialize TypeScript Binding");
  XII_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_hScriptCompendium = xiiResourceManager::LoadResource<xiiScriptCompendiumResource>(":project/AssetCache/Common/Scripts.xiiScriptCompendium");

  m_Duk.EnableModuleSupport(&xiiTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("xiiTypeScriptBinding", this);

  m_Duk.RegisterGlobalFunction("__CPP_Binding_RegisterMessageHandler", &xiiTypeScriptBinding::__CPP_Binding_RegisterMessageHandler, 2);

  StoreWorld(&ref_world);

  SetupRttiFunctionBindings();
  SetupRttiPropertyBindings();

  XII_SUCCEED_OR_RETURN(Init_RequireModules());
  XII_SUCCEED_OR_RETURN(Init_Log());
  XII_SUCCEED_OR_RETURN(Init_Utils());
  XII_SUCCEED_OR_RETURN(Init_Time());
  XII_SUCCEED_OR_RETURN(Init_GameObject());
  XII_SUCCEED_OR_RETURN(Init_FunctionBinding());
  XII_SUCCEED_OR_RETURN(Init_PropertyBinding());
  XII_SUCCEED_OR_RETURN(Init_Component());
  XII_SUCCEED_OR_RETURN(Init_World());
  XII_SUCCEED_OR_RETURN(Init_Clock());
  XII_SUCCEED_OR_RETURN(Init_Debug());
  XII_SUCCEED_OR_RETURN(Init_Random());
  XII_SUCCEED_OR_RETURN(Init_Physics());

  m_bInitialized = true;
  return XII_SUCCESS;
}

xiiResult xiiTypeScriptBinding::LoadComponent(const xiiUuid& typeGuid, TsComponentTypeInfo& out_typeInfo)
{
  if (!m_bInitialized || !typeGuid.IsValid())
  {
    return XII_FAILURE;
  }

  // check if this component type has been loaded before
  {
    auto itLoaded = m_LoadedComponents.Find(typeGuid);

    if (itLoaded.IsValid())
    {
      out_typeInfo = m_TsComponentTypes.Find(typeGuid);
      return itLoaded.Value() ? XII_SUCCESS : XII_FAILURE;
    }
  }

  XII_PROFILE_SCOPE("Load Script Component");

  bool& bLoaded = m_LoadedComponents[typeGuid];
  bLoaded       = false;

  xiiResourceLock<xiiScriptCompendiumResource> pCompendium(m_hScriptCompendium, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    return XII_FAILURE;
  }

  auto itType = pCompendium->GetDescriptor().m_AssetGuidToInfo.Find(typeGuid);
  if (!itType.IsValid())
  {
    return XII_FAILURE;
  }

  const xiiString& sComponentPath = itType.Value().m_sComponentFilePath;
  const xiiString& sComponentName = itType.Value().m_sComponentTypeName;

  const xiiStringBuilder sCompModule("__", sComponentName);

  m_Duk.PushGlobalObject();

  xiiStringBuilder req;
  req.Format("var {} = require(\"./{}\");", sCompModule, itType.Value().m_sComponentFilePath);
  if (m_Duk.ExecuteString(req).Failed())
  {
    xiiLog::Error("Could not load component");
    return XII_FAILURE;
  }

  m_Duk.PopStack();

  m_TsComponentTypes[typeGuid].m_sComponentTypeName = sComponentName;
  RegisterMessageHandlersForComponentType(sComponentName, typeGuid);

  bLoaded = true;

  out_typeInfo = m_TsComponentTypes.FindOrAdd(typeGuid, nullptr);

  return XII_SUCCESS;
}

xiiResult xiiTypeScriptBinding::FindScriptComponentInfo(const char* szComponentType, TsComponentTypeInfo& out_typeInfo)
{
  for (auto it : m_TsComponentTypes)
  {
    if (it.Value().m_sComponentTypeName == szComponentType)
    {
      out_typeInfo = it;
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

void xiiTypeScriptBinding::Update()
{
  ExecuteConsoleFuncs();
}

void xiiTypeScriptBinding::CleanupStash(xiiUInt32 uiNumIterations)
{
  if (!m_LastCleanupObj.IsValid())
    m_LastCleanupObj = m_GameObjectToStashIdx.GetIterator();

  xiiDuktapeHelper duk(m_Duk); // [ ]
  duk.PushGlobalStash();       // [ stash ]

  for (xiiUInt32 i = 0; i < uiNumIterations && m_LastCleanupObj.IsValid(); ++i)
  {
    xiiGameObject* pGO;
    if (!m_pWorld->TryGetObject(m_LastCleanupObj.Key(), pGO))
    {
      duk.PushNull();                                        // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupObj.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupObj.Value());
      m_LastCleanupObj = m_GameObjectToStashIdx.Remove(m_LastCleanupObj);
    }
    else
    {
      ++m_LastCleanupObj;
    }
  }

  if (!m_LastCleanupComp.IsValid())
    m_LastCleanupComp = m_ComponentToStashIdx.GetIterator();

  for (xiiUInt32 i = 0; i < uiNumIterations && m_LastCleanupComp.IsValid(); ++i)
  {
    xiiComponent* pGO;
    if (!m_pWorld->TryGetComponent(m_LastCleanupComp.Key(), pGO))
    {
      duk.PushNull();                                         // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupComp.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupComp.Value());
      m_LastCleanupComp = m_ComponentToStashIdx.Remove(m_LastCleanupComp);
    }
    else
    {
      ++m_LastCleanupComp;
    }
  }

  duk.PopStack(); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

xiiUInt32 xiiTypeScriptBinding::AcquireStashObjIndex()
{
  xiiUInt32 idx;

  if (!m_FreeStashObjIdx.IsEmpty())
  {
    idx = m_FreeStashObjIdx.PeekBack();
    m_FreeStashObjIdx.PopBack();
  }
  else
  {
    idx = m_uiNextStashObjIdx;
    ++m_uiNextStashObjIdx;
  }

  return idx;
}

void xiiTypeScriptBinding::ReleaseStashObjIndex(xiiUInt32 uiIdx)
{
  m_FreeStashObjIdx.PushBack(uiIdx);
}

void xiiTypeScriptBinding::StoreReferenceInStash(duk_context* pDuk, xiiUInt32 uiStashIdx)
{
  xiiDuktapeHelper duk(pDuk);              // [ object ]
  duk.PushGlobalStash();                   // [ object stash ]
  duk_dup(duk, -2);                        // [ object stash object ]
  duk_put_prop_index(duk, -2, uiStashIdx); // [ object stash ]
  duk.PopStack();                          // [ object ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

bool xiiTypeScriptBinding::DukPushStashObject(duk_context* pDuk, xiiUInt32 uiStashIdx)
{
  xiiDuktapeHelper duk(pDuk);

  duk.PushGlobalStash();          // [ stash ]
  duk_push_uint(duk, uiStashIdx); // [ stash idx ]

  if (!duk_get_prop(duk, -2)) // [ stash obj/undef ]
  {
    duk_pop_2(duk);     // [ ]
    duk_push_null(duk); // [ null ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }
  else // [ stash obj ]
  {
    duk_replace(duk, -2); // [ obj ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, +1);
  }
}

void xiiTypeScriptBinding::SyncTsObjectXIITsObject(duk_context* pDuk, const xiiRTTI* pRtti, void* pObject, xiiInt32 iObjIdx)
{
  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (xiiAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pProp);

    const xiiVariant value = xiiTypeScriptBinding::GetVariantProperty(pDuk, pProp->GetPropertyName(), iObjIdx, pMember->GetSpecificType());

    if (value.IsValid())
    {
      xiiReflectionUtils::SetMemberPropertyValue(pMember, pObject, value);
    }
  }
}

void xiiTypeScriptBinding::SyncXIIObjectToTsObject(duk_context* pDuk, const xiiRTTI* pRtti, const void* pObject, xiiInt32 iObjIdx)
{
  xiiDuktapeHelper duk(pDuk);

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (xiiAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pProp);

    const xiiRTTI* pType = pMember->GetSpecificType();

    if (pType->GetTypeFlags().IsAnySet(xiiTypeFlags::IsEnum | xiiTypeFlags::Bitflags))
    {
      const xiiVariant val = xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
    else
    {
      if (pType->GetVariantType() == xiiVariant::Type::Invalid)
        continue;

      const xiiVariant val = xiiReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
  }

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptBinding::GenerateConstructorString(xiiStringBuilder& out_String, const xiiVariant& value)
{
  out_String.Clear();

  const xiiVariant::Type::Enum type = value.GetType();

  switch (type)
  {
    case xiiVariant::Type::Invalid:
      break;

    case xiiVariant::Type::Bool:
    case xiiVariant::Type::Int8:
    case xiiVariant::Type::UInt8:
    case xiiVariant::Type::Int16:
    case xiiVariant::Type::UInt16:
    case xiiVariant::Type::Int32:
    case xiiVariant::Type::UInt32:
    case xiiVariant::Type::Int64:
    case xiiVariant::Type::UInt64:
    case xiiVariant::Type::Float:
    case xiiVariant::Type::Double:
    case xiiVariant::Type::String:
    case xiiVariant::Type::StringView:
    {
      out_String = value.ConvertTo<xiiString>();
      break;
    }

    case xiiVariant::Type::Color:
    case xiiVariant::Type::ColorGamma:
    {
      const xiiColor c = value.ConvertTo<xiiColor>();
      out_String.Format("new Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
      break;
    }

    case xiiVariant::Type::Vector2:
    {
      const xiiVec2 v = value.Get<xiiVec2>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case xiiVariant::Type::Vector3:
    {
      const xiiVec3 v = value.Get<xiiVec3>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case xiiVariant::Type::Vector4:
    {
      const xiiVec4 v = value.Get<xiiVec4>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case xiiVariant::Type::Vector2I:
    {
      const xiiVec2I32 v = value.Get<xiiVec2I32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case xiiVariant::Type::Vector3I:
    {
      const xiiVec3I32 v = value.Get<xiiVec3I32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case xiiVariant::Type::Vector4I:
    {
      const xiiVec4I32 v = value.Get<xiiVec4I32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case xiiVariant::Type::Vector2U:
    {
      const xiiVec2U32 v = value.Get<xiiVec2U32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case xiiVariant::Type::Vector3U:
    {
      const xiiVec3U32 v = value.Get<xiiVec3U32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case xiiVariant::Type::Vector4U:
    {
      const xiiVec4U32 v = value.Get<xiiVec4U32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case xiiVariant::Type::Quaternion:
    {
      const xiiQuat q = value.Get<xiiQuat>();
      out_String.Format("new Quat({}, {}, {}, {})", q.v.x, q.v.y, q.v.z, q.w);
      break;
    }

    case xiiVariant::Type::Matrix3:
    {
      out_String = "new Mat3()";
      break;
    }

    case xiiVariant::Type::Matrix4:
    {
      out_String = "new Mat4()";
      break;
    }

    case xiiVariant::Type::Transform:
    {
      out_String = "new Transform()";
      break;
    }

    case xiiVariant::Type::Time:
      out_String.Format("{0}", value.Get<xiiTime>().GetSeconds());
      break;

    case xiiVariant::Type::Angle:
      out_String.Format("{0}", value.Get<xiiAngle>().GetRadian());
      break;

    case xiiVariant::Type::Uuid:
    case xiiVariant::Type::DataBuffer:
    case xiiVariant::Type::VariantArray:
    case xiiVariant::Type::VariantDictionary:
    case xiiVariant::Type::TypedPointer:
    case xiiVariant::Type::TypedObject:
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}
