#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

xiiHashTable<xiiUInt32, xiiTypeScriptBinding::PropertyBinding> xiiTypeScriptBinding::s_BoundProperties;

static int __CPP_ComponentProperty_get(duk_context* pDuk);
static int __CPP_ComponentProperty_set(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_PropertyBinding()
{
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_get", __CPP_ComponentProperty_get, 2);
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_set", __CPP_ComponentProperty_set, 3);

  return XII_SUCCESS;
}

xiiUInt32 xiiTypeScriptBinding::ComputePropertyBindingHash(const xiiRTTI* pType, xiiAbstractMemberProperty* pMember)
{
  xiiStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pMember->GetPropertyName());

  return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sFuncName.GetData()));
}

void xiiTypeScriptBinding::SetupRttiPropertyBindings()
{
  if (!s_BoundProperties.IsEmpty())
    return;

  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<xiiComponent>())
      continue;

    for (xiiAbstractProperty* pProp : pRtti->GetProperties())
    {
      if (pProp->GetCategory() != xiiPropertyCategory::Member)
        continue;

      const xiiUInt32 uiHash = ComputePropertyBindingHash(pRtti, static_cast<xiiAbstractMemberProperty*>(pProp));
      XII_ASSERT_DEV(!s_BoundProperties.Contains(uiHash), "Hash collision for bound property name!");

      s_BoundProperties[uiHash].m_pMember = static_cast<xiiAbstractMemberProperty*>(pProp);
    }
  }
}

void xiiTypeScriptBinding::GeneratePropertiesCode(xiiStringBuilder& out_Code, const xiiRTTI* pRtti)
{
  xiiStringBuilder sProp;

  for (xiiAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    xiiAbstractMemberProperty* pMember   = static_cast<xiiAbstractMemberProperty*>(pProp);
    const xiiRTTI*             pPropType = pProp->GetSpecificType();

    const xiiUInt32 uiHash = ComputePropertyBindingHash(pRtti, pMember);

    xiiStringBuilder sTypeName;

    sTypeName = TsType(pPropType);
    if (sTypeName.IsEmpty())
      continue;

    sProp.Format("  get {0}(): {1} { return __CPP_ComponentProperty_get(this, {2}); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
    out_Code.Append(sProp.GetView());

    sProp.Format("  set {0}(value: {1}) { __CPP_ComponentProperty_set(this, {2}, value); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
    out_Code.Append(sProp.GetView());
  }
}

const xiiTypeScriptBinding::PropertyBinding* xiiTypeScriptBinding::FindPropertyBinding(xiiUInt32 uiHash)
{
  const PropertyBinding* pBinding = nullptr;
  s_BoundProperties.TryGetValue(uiHash, pBinding);
  return pBinding;
}

int __CPP_ComponentProperty_get(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  const xiiUInt32 uiHash = duk.GetUIntValue(1);

  const xiiTypeScriptBinding::PropertyBinding* pBinding = xiiTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    xiiLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const xiiVariant value = xiiReflectionUtils::GetMemberPropertyValue(pBinding->m_pMember, pComponent);
  xiiTypeScriptBinding::PushVariant(duk, value);
  return duk.ReturnCustom();
}

int __CPP_ComponentProperty_set(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  const xiiUInt32 uiHash = duk.GetUIntValue(1);

  const xiiTypeScriptBinding::PropertyBinding* pBinding = xiiTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    xiiLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const xiiVariant value = xiiTypeScriptBinding::GetVariant(duk, 2, pBinding->m_pMember->GetSpecificType());

  xiiReflectionUtils::SetMemberPropertyValue(pBinding->m_pMember, pComponent, value);

  return duk.ReturnVoid();
}
