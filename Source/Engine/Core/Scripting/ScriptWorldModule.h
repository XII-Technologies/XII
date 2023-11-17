#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using xiiScriptClassResourceHandle = xiiTypedResourceHandle<class xiiScriptClassResource>;
class xiiScriptInstance;

class XII_CORE_DLL xiiScriptWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptWorldModule, xiiWorldModule);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiScriptWorldModule);

public:
  xiiScriptWorldModule(xiiWorld* pWorld);
  ~xiiScriptWorldModule();

  virtual void Initialize() override;
  virtual void WorldClear() override;

  void AddUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance, xiiTime updateInterval, bool bOnlyWhenSimulating);
  void RemoveUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// \brief Creates a new coroutine of pCoroutineType with the given name. If the creationMode prevents creating a new coroutine,
  /// this function will return an invalid handle and a nullptr in out_pCoroutine if there is already a coroutine running
  /// with the same name on the given instance.
  xiiScriptCoroutineHandle CreateCoroutine(const xiiRTTI* pCoroutineType, xiiStringView sName, xiiScriptInstance& inout_instance, xiiScriptCoroutineCreationMode::Enum creationMode, xiiScriptCoroutine*& out_pCoroutine);

  /// \brief Starts the coroutine with the given arguments. This will call the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(xiiScriptCoroutineHandle hCoroutine, xiiArrayPtr<xiiVariant> arguments);

  /// \brief Stops and deletes the coroutine. This will call the Stop() function and will delete the coroutine on next update of the script world module.
  void StopAndDeleteCoroutine(xiiScriptCoroutineHandle hCoroutine);

  /// \brief Stops and deletes all coroutines with the given name on pInstance.
  void StopAndDeleteCoroutine(xiiStringView sName, xiiScriptInstance* pInstance);

  /// \brief Stops and deletes all coroutines on pInstance.
  void StopAndDeleteAllCoroutines(xiiScriptInstance* pInstance);

  /// \brief Returns whether the coroutine has already finished or has been stopped.
  bool IsCoroutineFinished(xiiScriptCoroutineHandle hCoroutine) const;

  ///@}

  struct FunctionContext
  {
    enum Flags : xiiUInt8
    {
      None,
      OnlyWhenSimulating
    };

    xiiPointerWithFlags<const xiiAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void*                                                     m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags == other.m_pFunctionAndFlags && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const xiiWorldModule::UpdateContext& context);

  xiiIntervalScheduler<FunctionContext> m_Scheduler;

  xiiIdTable<xiiScriptCoroutineId, xiiUniquePtr<xiiScriptCoroutine>>           m_RunningScriptCoroutines;
  xiiHashTable<xiiScriptInstance*, xiiSmallArray<xiiScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  xiiDynamicArray<xiiUniquePtr<xiiScriptCoroutine>>                            m_DeadScriptCoroutines;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct xiiHashHelper<xiiScriptWorldModule::FunctionContext>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiScriptWorldModule::FunctionContext& value)
  {
    xiiUInt32 hash = xiiHashHelper<const void*>::Hash(value.m_pFunctionAndFlags);
    hash           = xiiHashingUtils::CombineHashValues32(hash, xiiHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiScriptWorldModule::FunctionContext& a, const xiiScriptWorldModule::FunctionContext& b) { return a == b; }
};
