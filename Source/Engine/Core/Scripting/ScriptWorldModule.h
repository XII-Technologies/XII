#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using xiiScriptClassResourceHandle = xiiTypedResourceHandle<class xiiScriptClassResource>;
class xiiScriptInstance;

/// World module responsible for script execution and coroutine management.
///
/// Handles the execution of script functions, manages script coroutines, and provides scheduling for script update functions.
/// This module ensures scripts are properly integrated with the world update cycle.
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

  /// Schedules a script function to be called at regular intervals.
  void AddUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance, xiiTime updateInterval, bool bOnlyWhenSimulating);

  /// Removes a previously scheduled script function from the scheduler.
  void RemoveUpdateFunctionToSchedule(const xiiAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// Creates a new coroutine of the specified type with the given name.
  ///
  /// Returns an invalid handle if the creationMode prevents creating a new coroutine
  /// and there is already a coroutine running with the same name on the given instance.
  xiiScriptCoroutineHandle CreateCoroutine(const xiiRTTI* pCoroutineType, xiiStringView sName, xiiScriptInstance& inout_instance, xiiScriptCoroutineCreationMode::Enum creationMode, xiiScriptCoroutine*& out_pCoroutine);

  /// Starts the coroutine with the given arguments.
  ///
  /// Calls the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(xiiScriptCoroutineHandle hCoroutine, xiiArrayPtr<xiiVariant> arguments);

  /// Stops and deletes the coroutine.
  ///
  /// Calls the Stop() function and deletes the coroutine on the next update cycle.
  void StopAndDeleteCoroutine(xiiScriptCoroutineHandle hCoroutine);

  /// Stops and deletes all coroutines with the given name on the specified instance.
  void StopAndDeleteCoroutine(xiiStringView sName, xiiScriptInstance* pInstance);

  /// Stops and deletes all coroutines on the specified instance.
  void StopAndDeleteAllCoroutines(xiiScriptInstance* pInstance);

  /// Returns whether the coroutine has finished or been stopped.
  bool IsCoroutineFinished(xiiScriptCoroutineHandle hCoroutine) const;

  ///@}

  /// Returns a shared expression VM for custom script implementations.
  ///
  /// The VM is NOT thread safe - only execute one expression at a time.
  xiiExpressionVM& GetSharedExpressionVM() { return m_SharedExpressionVM; }

  /// Context information for scheduled script functions.
  struct FunctionContext
  {
    /// Flags controlling when the function should be executed.
    enum Flags : xiiUInt8
    {
      None,              ///< Execute always
      OnlyWhenSimulating ///< Execute only during simulation
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

  xiiExpressionVM m_SharedExpressionVM;
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
