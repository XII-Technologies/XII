#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiScriptWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptWorldModule::xiiScriptWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiScriptWorldModule::ResourceEventHandler, this));
}

xiiScriptWorldModule::~xiiScriptWorldModule()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiScriptWorldModule::ResourceEventHandler, this));
}

void xiiScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiScriptWorldModule::ReloadScripts, this);
    updateDesc.m_Phase     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_fPriority = 10000.0f;

    RegisterUpdateFunction(updateDesc);
  }
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

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<xiiScriptCoroutine>(xiiFoundation::GetDefaultAllocator());

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
    (*pCoroutine)->Start(arguments);
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

void xiiScriptWorldModule::AddScriptReloadFunction(xiiScriptClassResourceHandle hScript, ReloadFunction function)
{
  if (hScript.IsValid() == false)
    return;

  XII_ASSERT_DEV(function.IsComparable(), "Function must be comparable otherwise it can't be removed");
  m_ReloadFunctions[hScript].PushBack(function);
}

void xiiScriptWorldModule::RemoveScriptReloadFunction(xiiScriptClassResourceHandle hScript, ReloadFunction function)
{
  XII_ASSERT_DEV(function.IsComparable(), "Function must be comparable otherwise it can't be removed");

  ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_ReloadFunctions.TryGetValue(hScript, pReloadFunctions))
  {
    for (xiiUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      if ((*pReloadFunctions)[i].IsEqualIfComparable(function))
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void xiiScriptWorldModule::CallUpdateFunctions(const xiiWorldModule::UpdateContext& context)
{
  xiiWorld* pWorld = GetWorld();

  xiiTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = GetWorld()->GetClock().GetTimeDiff();
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

void xiiScriptWorldModule::ReloadScripts(const xiiWorldModule::UpdateContext& context)
{
  for (auto hScript : m_NeedReload)
  {
    if (m_ReloadFunctions.TryGetValue(hScript, m_TempReloadFunctions))
    {
      for (auto& reloadFunction : m_TempReloadFunctions)
      {
        reloadFunction();
      }
    }
  }

  m_NeedReload.Clear();
}

void xiiScriptWorldModule::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type != xiiResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = xiiDynamicCast<const xiiScriptClassResource*>(e.m_pResource))
  {
    xiiScriptClassResourceHandle hScript = pResource->GetResourceHandle();
    m_NeedReload.Insert(hScript);
  }
}
