#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using xiiScriptClassResourceHandle = xiiTypedResourceHandle<class xiiScriptClassResource>;

class XII_CORE_DLL xiiScriptWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptWorldModule, xiiWorldModule);

public:
  xiiScriptWorldModule(xiiWorld* pWorld);
  ~xiiScriptWorldModule();

  virtual void Initialize() override;

  void AddUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance, xiiTime updateInterval);
  void RemoveUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance);

  using ReloadFunction = xiiDelegate<void()>;
  void AddScriptReloadFunction(xiiScriptClassResourceHandle hScript, ReloadFunction function);
  void RemoveScriptReloadFunction(xiiScriptClassResourceHandle hScript, void* pInstance);

  struct FunctionContext
  {
    const xiiAbstractFunctionProperty* m_pFunction = nullptr;
    void*                              m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunction == other.m_pFunction && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const xiiWorldModule::UpdateContext& context);
  void ReloadScripts(const xiiWorldModule::UpdateContext& context);
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiIntervalScheduler<FunctionContext> m_Scheduler;

  using ReloadFunctionList = xiiHybridArray<ReloadFunction, 8>;
  xiiHashTable<xiiScriptClassResourceHandle, ReloadFunctionList> m_ReloadFunctions;
  xiiHashSet<xiiScriptClassResourceHandle>                       m_NeedReload;
  ReloadFunctionList                                             m_TempReloadFunctions;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct xiiHashHelper<xiiScriptWorldModule::FunctionContext>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiScriptWorldModule::FunctionContext& value)
  {
    xiiUInt32 hash = xiiHashHelper<const void*>::Hash(value.m_pFunction);
    hash           = xiiHashingUtils::CombineHashValues32(hash, xiiHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiScriptWorldModule::FunctionContext& a, const xiiScriptWorldModule::FunctionContext& b) { return a == b; }
};
