#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>

xiiScriptRTTI::xiiScriptRTTI(xiiStringView sName, const xiiRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers) :
  xiiRTTI(nullptr, pParentType, 0, 1, xiiVariantType::Invalid, xiiTypeFlags::Class, nullptr, xiiArrayPtr<xiiAbstractProperty*>(), xiiArrayPtr<xiiAbstractFunctionProperty*>(), xiiArrayPtr<xiiPropertyAttribute*>(), xiiArrayPtr<xiiAbstractMessageHandler*>(), xiiArrayPtr<xiiMessageSenderInfo>(), nullptr), m_sTypeNameStorage(sName), m_FunctionStorage(std::move(functions)), m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_sTypeName = m_sTypeNameStorage.GetData();

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
  m_sTypeName = nullptr;
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

xiiScriptFunctionProperty::xiiScriptFunctionProperty(xiiStringView sName) :
  xiiAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage.Assign(sName);
  m_sPropertyName = m_sPropertyNameStorage.GetData();
}

xiiScriptFunctionProperty::~xiiScriptFunctionProperty() = default;

//////////////////////////////////////////////////////////////////////////

xiiScriptInstance::xiiScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld) :
  m_Owner(inout_owner), m_pWorld(pWorld)
{
}
