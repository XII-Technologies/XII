#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptClassResource, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiScriptClassResource);
// clang-format on

xiiScriptClassResource::xiiScriptClassResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiScriptClassResource::~xiiScriptClassResource() = default;

xiiSharedPtr<xiiScriptRTTI> xiiScriptClassResource::CreateScriptType(xiiStringView sName, const xiiRTTI* pBaseType, xiiScriptRTTI::FunctionList&& functions, xiiScriptRTTI::MessageHandlerList&& messageHandlers)
{
  xiiScriptRTTI::FunctionList sortedFunctions;
  for (auto pFuncProp : pBaseType->GetFunctions())
  {
    auto pBaseClassFuncAttr = pFuncProp->GetAttributeByType<xiiScriptBaseClassFunctionAttribute>();
    if (pBaseClassFuncAttr == nullptr)
      continue;

    xiiStringView sBaseClassFuncName = pFuncProp->GetPropertyName();
    sBaseClassFuncName.TrimWordStart("Reflection_");

    xiiUInt16 uiIndex = pBaseClassFuncAttr->GetIndex();
    sortedFunctions.EnsureCount(uiIndex + 1);

    for (xiiUInt32 i = 0; i < functions.GetCount(); ++i)
    {
      auto& pScriptFuncProp = functions[i];
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        functions.RemoveAtAndSwap(i);
        break;
      }
    }
  }

  m_pType = XII_DEFAULT_NEW(xiiScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
  return m_pType;
}

void xiiScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}

xiiSharedPtr<xiiScriptCoroutineRTTI> xiiScriptClassResource::CreateScriptCoroutineType(xiiStringView sScriptClassName, xiiStringView sFunctionName, xiiUniquePtr<xiiRTTIAllocator>&& pAllocator)
{
  xiiStringBuilder sCoroutineTypeName;
  sCoroutineTypeName.Set(sScriptClassName, "::", sFunctionName, "<Coroutine>");

  xiiSharedPtr<xiiScriptCoroutineRTTI> pCoroutineType = XII_DEFAULT_NEW(xiiScriptCoroutineRTTI, sCoroutineTypeName, std::move(pAllocator));
  m_CoroutineTypes.PushBack(pCoroutineType);

  return pCoroutineType;
}

void xiiScriptClassResource::DeleteAllScriptCoroutineTypes()
{
  m_CoroutineTypes.Clear();
}
