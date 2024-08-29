#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/ReflectionUtils.h>

xiiScriptRTTI::xiiScriptRTTI(xiiStringView sName, const xiiRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers) :
  xiiRTTI(nullptr, pParentType, 0, 1, xiiVariantType::Invalid, xiiTypeFlags::Class, nullptr, xiiArrayPtr<const xiiAbstractProperty*>(), xiiArrayPtr<const xiiAbstractFunctionProperty*>(), xiiArrayPtr<const xiiPropertyAttribute*>(), xiiArrayPtr<xiiAbstractMessageHandler*>(), xiiArrayPtr<xiiMessageSenderInfo>(), nullptr), m_sTypeNameStorage(sName), m_FunctionStorage(std::move(functions)), m_MessageHandlerStorage(std::move(messageHandlers))
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
  m_sPropertyName = m_sPropertyNameStorage;
}

xiiScriptFunctionProperty::~xiiScriptFunctionProperty() = default;

//////////////////////////////////////////////////////////////////////////

xiiScriptMessageHandler::xiiScriptMessageHandler(const xiiScriptMessageDesc& desc) :
  m_Properties(desc.m_Properties)
{
  xiiUniquePtr<xiiMessage> pMessage = desc.m_pType->GetAllocator()->Allocate<xiiMessage>();

  m_Id       = pMessage->GetId();
  m_bIsConst = true;
}

xiiScriptMessageHandler::~xiiScriptMessageHandler() = default;

void xiiScriptMessageHandler::FillMessagePropertyValues(const xiiMessage& msg, xiiDynamicArray<xiiVariant>& out_propertyValues)
{
  out_propertyValues.Clear();

  for (auto pProp : m_Properties)
  {
    if (pProp->GetCategory() == xiiPropertyCategory::Member)
    {
      out_propertyValues.PushBack(xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), &msg));
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

xiiScriptInstance::xiiScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld) :
  m_Owner(inout_owner), m_pWorld(pWorld)
{
}

void xiiScriptInstance::SetInstanceVariables(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters)
{
  for (auto it : parameters)
  {
    SetInstanceVariable(it.key, it.value);
  }
}

//////////////////////////////////////////////////////////////////////////

// static
xiiAllocatorBase* xiiScriptAllocator::GetAllocator()
{
  static xiiProxyAllocator s_ScriptAllocator("Script", xiiFoundation::GetDefaultAllocator());
  return &s_ScriptAllocator;
}
