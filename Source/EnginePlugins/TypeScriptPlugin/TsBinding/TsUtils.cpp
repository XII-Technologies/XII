#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Utils_StringToHash(duk_context* pDuk);
static int __CPP_Utils_FindPrefabRootNode(duk_context* pDuk);
static int __CPP_Utils_FindPrefabRootScript(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_Utils()
{
  m_Duk.RegisterGlobalFunction("__CPP_Utils_StringToHash", __CPP_Utils_StringToHash, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Utils_FindPrefabRootNode", __CPP_Utils_FindPrefabRootNode, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Utils_FindPrefabRootScript", __CPP_Utils_FindPrefabRootScript, 2);

  return XII_SUCCESS;
}

static int __CPP_Utils_StringToHash(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  return duk.ReturnUInt(xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(duk.GetStringValue(0))));
}

static int __CPP_Utils_FindPrefabRootNode(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pObject = xiiTypeScriptBinding::ExpectGameObject(duk, 0);

  if (pObject != nullptr)
  {
    xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

    if (pObject->GetChildCount() == 1)
    {
      pBinding->DukPutGameObject(pObject->GetChildren());
      goto found;
    }
    else
    {
      for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
      {
        if (it->GetName() == "root")
        {
          pBinding->DukPutGameObject(pObject->GetChildren());
          goto found;
        }
      }
    }
  }

  duk.PushNull();
found:

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Utils_FindPrefabRootScript(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiGameObject* pObject    = xiiTypeScriptBinding::ExpectGameObject(duk, 0);
  const char*    szTypeName = duk.GetStringValue(1);

  if (pObject != nullptr)
  {
    xiiTypeScriptBinding* pBinding = xiiTypeScriptBinding::RetrieveBinding(pDuk);

    xiiTypeScriptBinding::TsComponentTypeInfo info;
    if (pBinding->FindScriptComponentInfo(szTypeName, info).Succeeded())
    {
      xiiHybridArray<xiiTypeScriptComponent*, 8> components;

      for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
      {
        it->TryGetComponentsOfBaseType(components);

        for (auto* pTs : components)
        {
          if (pTs->GetTypeScriptComponentGuid() == info.Key())
          {
            pBinding->DukPutComponentObject(pTs);
            goto found;
          }
        }
      }
    }
  }

  duk.PushNull();
found:

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}
