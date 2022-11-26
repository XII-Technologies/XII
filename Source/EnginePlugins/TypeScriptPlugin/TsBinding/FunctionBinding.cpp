#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

xiiHashTable<xiiUInt32, xiiTypeScriptBinding::FunctionBinding> xiiTypeScriptBinding::s_BoundFunctions;

static int __CPP_ComponentFunction_Call(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_FunctionBinding()
{
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_ComponentFunction_Call", __CPP_ComponentFunction_Call);

  return XII_SUCCESS;
}

xiiUInt32 xiiTypeScriptBinding::ComputeFunctionBindingHash(const xiiRTTI* pType, xiiAbstractFunctionProperty* pFunc)
{
  xiiStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pFunc->GetPropertyName());

  return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sFuncName.GetData()));
}

void xiiTypeScriptBinding::SetupRttiFunctionBindings()
{
  if (!s_BoundFunctions.IsEmpty())
    return;

  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<xiiComponent>())
      continue;

    for (xiiAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
    {
      // TODO: static members ?
      if (pFunc->GetFunctionType() != xiiFunctionType::Member)
        continue;

      const xiiUInt32 uiHash = ComputeFunctionBindingHash(pRtti, pFunc);
      XII_ASSERT_DEV(!s_BoundFunctions.Contains(uiHash), "Hash collision for bound function name!");

      s_BoundFunctions[uiHash].m_pFunc = pFunc;
    }
  }
}

const char* xiiTypeScriptBinding::TsType(const xiiRTTI* pRtti)
{
  if (pRtti == nullptr)
    return "void";

  static xiiStringBuilder res;

  if (pRtti->IsDerivedFrom<xiiEnumBase>())
  {
    s_RequiredEnums.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("xii");
    res.Prepend("Enum.");

    return res;
  }

  if (pRtti->IsDerivedFrom<xiiBitflagsBase>())
  {
    s_RequiredFlags.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("xii");
    res.Prepend("Flags.");

    return res;
  }

  switch (pRtti->GetVariantType())
  {
    case xiiVariant::Type::Invalid:
    {
      if (xiiStringUtils::IsEqual(pRtti->GetTypeName(), "xiiVariant"))
        return "any";

      return nullptr;
    }

    case xiiVariant::Type::Angle:
      return "number";

    case xiiVariant::Type::Bool:
      return "boolean";

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
      return "number";

    case xiiVariant::Type::Color:
    case xiiVariant::Type::ColorGamma:
      return "Color";

    case xiiVariant::Type::Vector2:
    case xiiVariant::Type::Vector2I:
    case xiiVariant::Type::Vector2U:
      return "Vec2";

    case xiiVariant::Type::Vector3:
    case xiiVariant::Type::Vector3I:
    case xiiVariant::Type::Vector3U:
      return "Vec3";

    case xiiVariant::Type::Quaternion:
      return "Quat";


    case xiiVariant::Type::String:
    case xiiVariant::Type::StringView:
      return "string";

    case xiiVariant::Type::Time:
      return "number";

    case xiiVariant::Type::Transform:
      return "Transform";

    case xiiVariant::Type::Matrix3:
      return "Mat3";

    case xiiVariant::Type::Matrix4:
      return "Mat4";

      // TODO: implement these types
      // case xiiVariant::Type::Vector4:
      // case xiiVariant::Type::Vector4I:
      // case xiiVariant::Type::Vector4U:

    default:
      return nullptr;
  }
}

void xiiTypeScriptBinding::GenerateExposedFunctionsCode(xiiStringBuilder& out_Code, const xiiRTTI* pRtti)
{
  xiiStringBuilder sFunc;

  for (xiiAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
  {
    // TODO: static members ?
    if (pFunc->GetFunctionType() != xiiFunctionType::Member)
      continue;

    // don't support functions with that many arguments
    if (pFunc->GetArgumentCount() > 16)
      continue;

    const xiiScriptableFunctionAttribute* pAttr = pFunc->GetAttributeByType<xiiScriptableFunctionAttribute>();

    if (pAttr == nullptr)
      goto ignore;

    sFunc.Set("  ", pFunc->GetPropertyName(), "(");

    for (xiiUInt32 i = 0; i < pFunc->GetArgumentCount(); ++i)
    {
      const char* szType = TsType(pFunc->GetArgumentType(i));

      if (szType == nullptr)
        goto ignore;

      sFunc.Append(i > 0 ? ", " : "", pAttr->GetArgumentName(i), ": ", szType);
    }

    sFunc.Append("): ");

    {
      const bool bHasReturnValue = pFunc->GetReturnType() != nullptr;

      {
        const char* szType = TsType(pFunc->GetReturnType());

        if (szType == nullptr)
          goto ignore;

        sFunc.Append(szType);
      }

      // function body
      {
        xiiUInt32 uiFuncHash = ComputeFunctionBindingHash(pRtti, pFunc);

        if (bHasReturnValue)
          sFunc.AppendFormat(" { return __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);
        else
          sFunc.AppendFormat(" { __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);

        for (xiiUInt32 arg = 0; arg < pFunc->GetArgumentCount(); ++arg)
        {
          sFunc.Append(", ", pAttr->GetArgumentName(arg));
        }

        sFunc.Append("); }\n");
      }
    }

    out_Code.Append(sFunc.GetView());

  ignore:
    continue;
  }
}

const xiiTypeScriptBinding::FunctionBinding* xiiTypeScriptBinding::FindFunctionBinding(xiiUInt32 uiFunctionHash)
{
  const FunctionBinding* pBinding = nullptr;
  s_BoundFunctions.TryGetValue(uiFunctionHash, pBinding);
  return pBinding;
}

int __CPP_ComponentFunction_Call(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);

  xiiComponent* pComponent = xiiTypeScriptBinding::ExpectComponent<xiiComponent>(pDuk);

  const xiiUInt32 uiFuncHash = duk.GetUIntValue(1);

  const xiiTypeScriptBinding::FunctionBinding* pBinding = xiiTypeScriptBinding::FindFunctionBinding(uiFuncHash);

  if (pBinding == nullptr)
  {
    xiiLog::Error("Bound function with hash {} not found.", uiFuncHash);
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), +1);
  }

  const xiiUInt32 uiNumArgs = pBinding->m_pFunc->GetArgumentCount();

  xiiVariant                     ret0;
  xiiStaticArray<xiiVariant, 16> args;
  args.SetCount(uiNumArgs);

  for (xiiUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    args[arg] = xiiTypeScriptBinding::GetVariant(duk, 2 + arg, pBinding->m_pFunc->GetArgumentType(arg));
  }

  pBinding->m_pFunc->Execute(pComponent, args, ret0);

  if (pBinding->m_pFunc->GetReturnType() != nullptr)
  {
    xiiTypeScriptBinding::PushVariant(duk, ret0);
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }
  else
  {
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
  }
}
