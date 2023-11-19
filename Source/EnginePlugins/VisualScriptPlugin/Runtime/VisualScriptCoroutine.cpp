#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptCoroutine::xiiVisualScriptCoroutine(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc)
  : m_LocalDataStorage(pDesc->GetLocalDataDesc())
  , m_Context(pDesc)
{
}

xiiVisualScriptCoroutine::~xiiVisualScriptCoroutine() = default;

void xiiVisualScriptCoroutine::Start(xiiArrayPtr<xiiVariant> arguments)
{
  m_LocalDataStorage.AllocateStorage();

  auto pVisualScriptInstance = static_cast<xiiVisualScriptInstance*>(GetScriptInstance());
  m_Context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);
}

void xiiVisualScriptCoroutine::Stop()
{
  m_Context.Deinitialize();
}

xiiScriptCoroutine::Result xiiVisualScriptCoroutine::Update(xiiTime deltaTimeSinceLastUpdate)
{
  auto result = m_Context.Execute(deltaTimeSinceLastUpdate);
  if (result.m_NextExecAndState == xiiVisualScriptExecutionContext::ExecResult::State::ContinueLater)
  {
    return Result::Running(result.m_MaxDelay);
  }

  return Result::Completed();
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptCoroutineAllocator::xiiVisualScriptCoroutineAllocator(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

void xiiVisualScriptCoroutineAllocator::Deallocate(void* pObject, xiiAllocatorBase* pAllocator /*= nullptr*/)
{
  XII_REPORT_FAILURE("Deallocate is not supported");
}

xiiInternal::NewInstance<void> xiiVisualScriptCoroutineAllocator::AllocateInternal(xiiAllocatorBase* pAllocator)
{
  return XII_SCRIPT_NEW(xiiVisualScriptCoroutine, m_pDesc);
}
