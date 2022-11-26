#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk);
static int __CPP_World_CreateObject(duk_context* pDuk);
static int __CPP_World_CreateComponent(duk_context* pDuk);
static int __CPP_World_DeleteComponent(duk_context* pDuk);
static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk);
static int __CPP_World_FindObjectsInSphere(duk_context* pDuk);
static int __CPP_World_FindObjectsInBox(duk_context* pDuk);

xiiHashTable<duk_context*, xiiWorld*> xiiTypeScriptBinding::s_DukToWorld;

xiiResult xiiTypeScriptBinding::Init_World()
{
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateObject", __CPP_World_CreateObject, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateComponent", __CPP_World_CreateComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteComponent", __CPP_World_DeleteComponent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_TryGetObjectWithGlobalKey", __CPP_World_TryGetObjectWithGlobalKey, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInSphere", __CPP_World_FindObjectsInSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInBox", __CPP_World_FindObjectsInBox, 4);

  return XII_SUCCESS;
}

void xiiTypeScriptBinding::StoreWorld(xiiWorld* pWorld)
{
  m_pWorld                         = pWorld;
  s_DukToWorld[m_Duk.GetContext()] = pWorld;
}

xiiWorld* xiiTypeScriptBinding::RetrieveWorld(duk_context* pDuk)
{
  xiiWorld* pWorld = nullptr;
  s_DukToWorld.TryGetValue(pDuk, pWorld);
  return pWorld;
}

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiWorld*           pWorld  = xiiTypeScriptBinding::RetrieveWorld(duk);
  xiiGameObjectHandle hObject = xiiTypeScriptBinding::RetrieveGameObjectHandle(duk, 0 /*this*/);

  pWorld->DeleteObjectDelayed(hObject);

  return duk.ReturnVoid();
}

static int __CPP_World_CreateObject(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiGameObjectDesc desc;

  desc.m_bActiveFlag         = duk.GetBoolProperty("ActiveFlag", desc.m_bActiveFlag, 0);
  desc.m_bDynamic            = duk.GetBoolProperty("Dynamic", desc.m_bDynamic, 0);
  desc.m_LocalPosition       = xiiTypeScriptBinding::GetVec3Property(duk, "LocalPosition", 0, xiiVec3(0.0f));
  desc.m_LocalScaling        = xiiTypeScriptBinding::GetVec3Property(duk, "LocalScaling", 0, xiiVec3(1.0f));
  desc.m_LocalRotation       = xiiTypeScriptBinding::GetQuatProperty(duk, "LocalRotation", 0);
  desc.m_LocalUniformScaling = duk.GetFloatProperty("LocalUniformScaling", 1.0f, 0);
  desc.m_uiTeamID            = static_cast<xiiUInt16>(duk.GetUIntProperty("TeamID", 0, 0));

  if (duk.PushLocalObject("Parent", 0).Succeeded())
  {
    desc.m_hParent = xiiTypeScriptBinding::RetrieveGameObjectHandle(duk, -1);
    duk.PopStack();
  }

  const char* szName = duk.GetStringProperty("Name", nullptr, 0);

  if (!xiiStringUtils::IsNullOrEmpty(szName))
  {
    desc.m_sName.Assign(szName);
  }

  xiiGameObjectHandle hObject = pWorld->CreateObject(desc);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(hObject);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_CreateComponent(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiWorld*      pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);
  xiiGameObject* pOwner = xiiTypeScriptBinding::ExpectGameObject(duk, 0);

  const xiiUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const xiiRTTI* pRtti = xiiRTTI::FindTypeByNameHash32(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    duk.Error(xiiFmt("Invalid component type name hash: {}", uiTypeNameHash));
    return duk.ReturnNull();
  }

  auto* pMan = pWorld->GetOrCreateManagerForComponentType(pRtti);

  xiiComponent* pComponent = nullptr;
  pMan->CreateComponent(pOwner, pComponent);

  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_DeleteComponent(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiWorld*          pWorld     = xiiTypeScriptBinding::RetrieveWorld(duk);
  xiiComponentHandle hComponent = xiiTypeScriptBinding::RetrieveComponentHandle(duk, 0 /*this*/);

  xiiComponent* pComponent = nullptr;
  if (pWorld->TryGetComponent(hComponent, pComponent))
  {
    pComponent->GetOwningManager()->DeleteComponent(pComponent);
  }

  return duk.ReturnVoid();
}

static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiWorld* pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  xiiGameObject*        pObject  = nullptr;
  bool                  _        = pWorld->TryGetObjectWithGlobalKey(xiiTempHashedString(duk.GetStringValue(0)), pObject);
  xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

struct FindObjectsCallback
{
  duk_context*          m_pDuk     = nullptr;
  xiiTypeScriptBinding* m_pBinding = nullptr;

  xiiVisitorExecution::Enum Callback(xiiGameObject* pObject)
  {
    xiiDuktapeHelper duk(m_pDuk);

    if (!duk_get_global_string(m_pDuk, "callback")) // [ func ]
      return xiiVisitorExecution::Stop;

    XII_DUK_VERIFY_STACK(duk, +1);

    m_pBinding->DukPutGameObject(pObject); // [ func go ]

    XII_DUK_VERIFY_STACK(duk, +2);

    duk_call(m_pDuk, 1); // [ res ]

    XII_DUK_VERIFY_STACK(duk, +1);

    if (duk_get_boolean_default(m_pDuk, -1, false) == false)
    {
      duk_pop(m_pDuk); // [ ]
      return xiiVisitorExecution::Stop;
    }

    duk_pop(m_pDuk); // [ ]

    XII_DUK_VERIFY_STACK(duk, 0);
    return xiiVisitorExecution::Continue;
  }
};

static int __CPP_World_FindObjectsInSphere(duk_context* pDuk)
{
  duk_require_function(pDuk, -1); // last argument

  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const char*   szType        = duk.GetStringValue(0);
  const xiiVec3 vSphereCenter = xiiTypeScriptBinding::GetVec3(pDuk, 1);
  const float   fRadius       = duk.GetFloatValue(2);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk     = pDuk;
  cb.m_pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  auto category = xiiSpatialData::FindCategory(szType);

  if (category != xiiInvalidSpatialDataCategory)
  {
    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(
      xiiBoundingSphere(vSphereCenter, fRadius), queryParams, xiiMakeDelegate(&FindObjectsCallback::Callback, &cb));
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_World_FindObjectsInBox(duk_context* pDuk)
{
  duk_require_function(pDuk, -1); // last argument

  xiiDuktapeFunction duk(pDuk);
  xiiWorld*          pWorld = xiiTypeScriptBinding::RetrieveWorld(duk);

  const char*   szType  = duk.GetStringValue(0);
  const xiiVec3 vBoxMin = xiiTypeScriptBinding::GetVec3(pDuk, 1);
  const xiiVec3 vBoxMax = xiiTypeScriptBinding::GetVec3(pDuk, 2);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk     = pDuk;
  cb.m_pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

  auto category = xiiSpatialData::FindCategory(szType);

  if (category != xiiInvalidSpatialDataCategory)
  {
    xiiSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInBox(
      xiiBoundingBox(vBoxMin, vBoxMax), queryParams, xiiMakeDelegate(&FindObjectsCallback::Callback, &cb));
  }

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
