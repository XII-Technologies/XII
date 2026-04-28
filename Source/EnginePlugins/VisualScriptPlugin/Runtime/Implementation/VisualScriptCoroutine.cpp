/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

xiiVisualScriptCoroutine::xiiVisualScriptCoroutine(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc) :
  m_Context(pDesc, xiiScriptAllocator::GetAllocator())
{
}

xiiVisualScriptCoroutine::~xiiVisualScriptCoroutine() = default;

void xiiVisualScriptCoroutine::StartWithVarArgs(xiiArrayPtr<xiiVariant> arguments)
{
  auto pVisualScriptInstance = static_cast<xiiVisualScriptInstance*>(GetScriptInstance());
  m_Context.Initialize(*pVisualScriptInstance, arguments);
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

xiiVisualScriptCoroutineAllocator::xiiVisualScriptCoroutineAllocator(const xiiSharedPtr<const xiiVisualScriptGraphDescription>& pDesc) :
  m_pDesc(pDesc)
{
}

void xiiVisualScriptCoroutineAllocator::Deallocate(void* pObject, xiiAllocator* pAllocator /*= nullptr*/)
{
  XII_REPORT_FAILURE("Deallocate is not supported");
}

xiiInternal::NewInstance<void> xiiVisualScriptCoroutineAllocator::AllocateInternal(xiiAllocator* pAllocator)
{
  return XII_SCRIPT_NEW(xiiVisualScriptCoroutine, m_pDesc);
}
