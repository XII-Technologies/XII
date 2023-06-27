#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

xiiScriptRTTI::xiiScriptRTTI(xiiStringView sName, const xiiRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers) :
  xiiRTTI(nullptr, pParentType, 0, 1, xiiVariantType::Invalid, xiiTypeFlags::Class, nullptr, xiiArrayPtr<xiiAbstractProperty*>(), xiiArrayPtr<xiiAbstractFunctionProperty*>(), xiiArrayPtr<xiiPropertyAttribute*>(), xiiArrayPtr<xiiAbstractMessageHandler*>(), xiiArrayPtr<xiiMessageSenderInfo>(), nullptr), m_sTypeNameStorage(sName), m_FunctionStorage(std::move(functions)), m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_szTypeName = m_sTypeNameStorage.GetData();

  for (auto& pFunction : m_FunctionStorage)
  {
    if (pFunction != nullptr)
    {
      m_FunctionRawPtrs.PushBack(pFunction.Borrow());
    }
  }

  for (auto& pMessageHandler : m_MessageHandlerStorage)
  {
    if (pMessageHandler != nullptr)
    {
      m_MessageHandlerRawPtrs.PushBack(pMessageHandler.Borrow());
    }
  }

  m_Functions       = m_FunctionRawPtrs;
  m_MessageHandlers = m_MessageHandlerRawPtrs;

  RegisterType();

  SetupParentHierarchy();
  GatherDynamicMessageHandlers();
}

xiiScriptRTTI::~xiiScriptRTTI()
{
  UnregisterType();
  m_szTypeName = nullptr;
}

const xiiAbstractFunctionProperty* xiiScriptRTTI::GetFunctionByIndex(xiiUInt32 uiIndex) const
{
  if (uiIndex < m_FunctionStorage.GetCount())
  {
    return m_FunctionStorage.GetData()[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

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

void xiiScriptClassResource::CreateScriptType(xiiStringView sName, const xiiRTTI* pBaseType, xiiScriptRTTI::FunctionList&& functions, xiiScriptRTTI::MessageHandlerList&& messageHandlers)
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

    for (auto& pScriptFuncProp : functions)
    {
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        break;
      }
    }
  }

  m_pType = XII_DEFAULT_NEW(xiiScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
}

void xiiScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}
