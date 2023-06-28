#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptInstance::xiiVisualScriptInstance(xiiReflectedClass& ref_owner, xiiWorld* pWorld, const xiiSharedPtr<const xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pVariableDataDesc) :
  m_Owner(ref_owner), m_pWorld(pWorld), m_pConstantDataStorage(pConstantDataStorage)
{
  if (pVariableDataDesc != nullptr)
  {
    m_pVariableDataStorage = XII_DEFAULT_NEW(xiiVisualScriptDataStorage, pVariableDataDesc);
    m_pVariableDataStorage->AllocateStorage();
  }
}

void xiiVisualScriptInstance::ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters)
{
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptExecutionContext::xiiVisualScriptExecutionContext(xiiUniquePtr<xiiVisualScriptGraphDescription>&& pDesc) :
  m_pDesc(std::move(pDesc))
{
}

xiiResult xiiVisualScriptExecutionContext::Initialize(xiiVisualScriptInstance& ref_instance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& ref_returnValue)
{
  m_pInstance = &ref_instance;

  auto pNode = m_pDesc->GetNode(0);
  XII_ASSERT_DEV(pNode->m_Type == xiiVisualScriptNodeDescription::Type::EntryCall, "Invalid entry node");

  for (xiiUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    m_pInstance->SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
  return XII_SUCCESS;
}

xiiVisualScriptExecutionContext::ReturnValue::Enum xiiVisualScriptExecutionContext::Execute()
{
  XII_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_pInstance->m_uiExecutionCounter;

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
    int result = pNode->m_Function(*m_pInstance, *pNode);
    if (result < ReturnValue::Completed)
    {
      return static_cast<ReturnValue::Enum>(result);
    }

    m_uiCurrentNode = pNode->GetExecutionIndex(result);
    pNode           = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ReturnValue::Completed;
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptFunctionProperty::xiiVisualScriptFunctionProperty(const char* szPropertyName, xiiUniquePtr<xiiVisualScriptGraphDescription>&& pDesc) :
  xiiAbstractFunctionProperty(nullptr), m_ExecutionContext(std::move(pDesc))
{
  m_sPropertyNameStorage.Assign(szPropertyName);
  m_szPropertyName = m_sPropertyNameStorage.GetData();
}

xiiVisualScriptFunctionProperty::~xiiVisualScriptFunctionProperty() = default;

void xiiVisualScriptFunctionProperty::Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& ref_returnValue) const
{
  XII_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<xiiVisualScriptInstance*>(pInstance);

  if (m_ExecutionContext.Initialize(*pVisualScriptInstance, arguments, ref_returnValue).Failed())
  {
    return;
  }

  m_ExecutionContext.Execute();
}
