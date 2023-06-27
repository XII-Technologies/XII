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
    auto updateDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiScriptWorldModule::ReloadScripts, this);
    updateDesc.m_Phase     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_fPriority = 10000.0f;

    RegisterUpdateFunction(updateDesc);
  }
}

void xiiScriptWorldModule::AddUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance, xiiTime updateInterval)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void xiiScriptWorldModule::RemoveUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

void xiiScriptWorldModule::AddScriptReloadFunction(xiiScriptClassResourceHandle hScript, ReloadFunction function)
{
  if (hScript.IsValid() == false)
    return;

  m_ReloadFunctions[hScript].PushBack(function);
}

void xiiScriptWorldModule::RemoveScriptReloadFunction(xiiScriptClassResourceHandle hScript, void* pInstance)
{
  ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_ReloadFunctions.TryGetValue(hScript, pReloadFunctions))
  {
    for (xiiUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      if ((*pReloadFunctions)[i].GetClassInstance() == pInstance)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void xiiScriptWorldModule::CallUpdateFunctions(const xiiWorldModule::UpdateContext& context)
{
  const xiiTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const FunctionContext& context, xiiTime deltaTime) {
    xiiVariant returnValue;
    context.m_pFunction->Execute(context.m_pInstance, xiiArrayPtr<xiiVariant>(), returnValue); });
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
