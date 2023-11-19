#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptFunctionProperty::xiiVisualScriptFunctionProperty(xiiStringView sName, const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc) :
  xiiScriptFunctionProperty(sName), m_pDesc(pDesc), m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  XII_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_LocalDataStorage.AllocateStorage();
}

xiiVisualScriptFunctionProperty::~xiiVisualScriptFunctionProperty() = default;

void xiiVisualScriptFunctionProperty::Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const
{
  XII_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<xiiVisualScriptInstance*>(pInstance);

  xiiVisualScriptExecutionContext context(m_pDesc);
  context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);

  auto result = context.Execute(xiiTime::Zero());
  XII_ASSERT_DEBUG(result.m_NextExecAndState != xiiVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");

  // TODO: return value
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptMessageHandler::xiiVisualScriptMessageHandler(const xiiScriptMessageDesc& desc, const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc) :
  xiiScriptMessageHandler(desc), m_pDesc(pDesc), m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  XII_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_DispatchFunc = &Dispatch;
  m_LocalDataStorage.AllocateStorage();
}

xiiVisualScriptMessageHandler::~xiiVisualScriptMessageHandler() = default;

// static
void xiiVisualScriptMessageHandler::Dispatch(xiiAbstractMessageHandler* pSelf, void* pInstance, xiiMessage& ref_msg)
{
  auto pHandler              = static_cast<xiiVisualScriptMessageHandler*>(pSelf);
  auto pComponent            = static_cast<xiiScriptComponent*>(pInstance);
  auto pVisualScriptInstance = static_cast<xiiVisualScriptInstance*>(pComponent->GetScriptInstance());

  xiiHybridArray<xiiVariant, 8> arguments;
  pHandler->FillMessagePropertyValues(ref_msg, arguments);

  xiiVisualScriptExecutionContext context(pHandler->m_pDesc);
  context.Initialize(*pVisualScriptInstance, pHandler->m_LocalDataStorage, arguments);

  auto result = context.Execute(xiiTime::Zero());
  XII_ASSERT_DEBUG(result.m_NextExecAndState != xiiVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");
}
