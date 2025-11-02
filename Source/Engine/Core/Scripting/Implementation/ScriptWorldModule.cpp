#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

XII_IMPLEMENT_WORLD_MODULE(xiiScriptWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiScriptWorldModule::xiiScriptWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiScriptWorldModule::~xiiScriptWorldModule() = default;

void xiiScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = xiiWorldUpdatePhase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }
}

void xiiScriptWorldModule::WorldClear()
{
  m_Scheduler.Clear();
}

void xiiScriptWorldModule::AddUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance, xiiTime updateInterval, bool bOnlyWhenSimulating)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtrAndFlags(pFunction, bOnlyWhenSimulating ? FunctionContext::Flags::OnlyWhenSimulating : FunctionContext::Flags::None);
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void xiiScriptWorldModule::RemoveUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtr(pFunction);
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

xiiScriptCoroutineHandle xiiScriptWorldModule::CreateCoroutine(const xiiRTTI* pCoroutineType, xiiStringView sName, xiiScriptInstance& inout_instance, xiiScriptCoroutineCreationMode::Enum creationMode, xiiScriptCoroutine*& out_pCoroutine)
{
  if (creationMode != xiiScriptCoroutineCreationMode::AllowOverlap)
  {
    xiiScriptCoroutine* pOverlappingCoroutine = nullptr;

    auto& runningCoroutines = m_InstanceToScriptCoroutines[&inout_instance];
    for (auto& hCoroutine : runningCoroutines)
    {
      xiiUniquePtr<xiiScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        pOverlappingCoroutine = pCoroutine->Borrow();
        break;
      }
    }

    if (pOverlappingCoroutine != nullptr)
    {
      if (creationMode == xiiScriptCoroutineCreationMode::StopOther)
      {
        StopAndDeleteCoroutine(pOverlappingCoroutine->GetHandle());
      }
      else if (creationMode == xiiScriptCoroutineCreationMode::DontCreateNew)
      {
        out_pCoroutine = nullptr;
        return xiiScriptCoroutineHandle();
      }
      else
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<xiiScriptCoroutine>(xiiScriptAllocator::GetAllocator());

  xiiScriptCoroutineId id = m_RunningScriptCoroutines.Insert(pCoroutine);
  pCoroutine->Initialize(id, sName, inout_instance, *this);

  m_InstanceToScriptCoroutines[&inout_instance].PushBack(xiiScriptCoroutineHandle(id));

  out_pCoroutine = pCoroutine;
  return xiiScriptCoroutineHandle(id);
}

void xiiScriptWorldModule::StartCoroutine(xiiScriptCoroutineHandle hCoroutine, xiiArrayPtr<xiiVariant> arguments)
{
  xiiUniquePtr<xiiScriptCoroutine>* pCoroutine = nullptr;
  if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine))
  {
    (*pCoroutine)->StartWithVarArgs(arguments);
    (*pCoroutine)->UpdateAndSchedule();
  }
}

void xiiScriptWorldModule::StopAndDeleteCoroutine(xiiScriptCoroutineHandle hCoroutine)
{
  xiiUniquePtr<xiiScriptCoroutine> pCoroutine;
  if (m_RunningScriptCoroutines.Remove(hCoroutine.GetInternalID(), &pCoroutine) == false)
    return;

  pCoroutine->Stop();
  pCoroutine->Deinitialize();
  m_DeadScriptCoroutines.PushBack(std::move(pCoroutine));
}

void xiiScriptWorldModule::StopAndDeleteCoroutine(xiiStringView sName, xiiScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (xiiUInt32 i = 0; i < pCoroutines->GetCount();)
    {
      auto hCoroutine = (*pCoroutines)[i];

      xiiUniquePtr<xiiScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        StopAndDeleteCoroutine(hCoroutine);
      }
      else
      {
        ++i;
      }
    }
  }
}

void xiiScriptWorldModule::StopAndDeleteAllCoroutines(xiiScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (auto hCoroutine : *pCoroutines)
    {
      StopAndDeleteCoroutine(hCoroutine);
    }
  }
}

bool xiiScriptWorldModule::IsCoroutineFinished(xiiScriptCoroutineHandle hCoroutine) const
{
  return m_RunningScriptCoroutines.Contains(hCoroutine.GetInternalID()) == false;
}

void xiiScriptWorldModule::CallUpdateFunctions(const xiiWorldModule::UpdateContext& context)
{
  XII_IGNORE_UNUSED(context);

  xiiWorld* pWorld = GetWorld();

  xiiTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = xiiClock::GetGlobalClock()->GetTimeDiff();
  }

  m_Scheduler.Update(deltaTime,
                     [this](const FunctionContext& context, xiiTime deltaTime) {
                       if (GetWorld()->GetWorldSimulationEnabled() || context.m_pFunctionAndFlags.GetFlags() == FunctionContext::Flags::None)
                       {
                         xiiVariant args[] = {deltaTime};
                         xiiVariant returnValue;
                         context.m_pFunctionAndFlags->Execute(context.m_pInstance, xiiMakeArrayPtr(args), returnValue);
                       }
                     });

  // Delete dead coroutines
  for (xiiUInt32 i = 0; i < m_DeadScriptCoroutines.GetCount(); ++i)
  {
    auto&              pCoroutine  = m_DeadScriptCoroutines[i];
    xiiScriptInstance* pInstance   = pCoroutine->GetScriptInstance();
    auto               pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance);
    XII_ASSERT_DEV(pCoroutines != nullptr, "Implementation error");

    pCoroutines->RemoveAndSwap(pCoroutine->GetHandle());
    if (pCoroutines->IsEmpty())
    {
      m_InstanceToScriptCoroutines.Remove(pInstance);
    }

    pCoroutine = nullptr;
  }
  m_DeadScriptCoroutines.Clear();
}

XII_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptWorldModule);
