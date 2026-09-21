/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Scripting/ScriptRTTI.h>

class xiiScriptWorldModule;

using xiiScriptCoroutineId = xiiGenericId<20, 12>;

/// A handle to a script coroutine which can be used to determine whether a coroutine is still running
/// even after the underlying coroutine object has already been deleted.
///
/// \sa xiiScriptWorldModule::CreateCoroutine, xiiScriptWorldModule::IsCoroutineFinished
struct xiiScriptCoroutineHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiScriptCoroutineHandle, xiiScriptCoroutineId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutineHandle);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiScriptCoroutineHandle);

/// Base class of script coroutines.
///
/// A coroutine is a function that can be distributed over multiple frames and behaves similar to a mini state machine.
/// That is why coroutines are actually individual objects that keep track of their state rather than simple functions.
/// At first Start() is called with the arguments of the coroutine followed by one or multiple calls to Update().
/// The return value of the Update() function determines whether the Update() function should be called again next frame
/// or at latest after the specified delay. If the Update() function returns completed the Stop() function is called and the
/// coroutine object is destroyed.
/// The xiiScriptWorldModule is used to create and manage coroutine objects. The coroutine can then either be started and
/// scheduled automatically by calling xiiScriptWorldModule::StartCoroutine or the
/// Start/Stop/Update function is called manually if the coroutine is embedded as a subroutine in another coroutine.
class XII_CORE_DLL xiiScriptCoroutine
{
public:
  xiiScriptCoroutine();
  virtual ~xiiScriptCoroutine();

  xiiScriptCoroutineHandle GetHandle() { return xiiScriptCoroutineHandle(m_Id); }

  xiiStringView GetName() const { return m_sName; }

  xiiScriptInstance*       GetScriptInstance() { return m_pInstance; }
  const xiiScriptInstance* GetScriptInstance() const { return m_pInstance; }

  xiiScriptWorldModule*       GetScriptWorldModule() { return m_pOwnerModule; }
  const xiiScriptWorldModule* GetScriptWorldModule() const { return m_pOwnerModule; }

  struct Result
  {
    struct State
    {
      using StorageType = xiiUInt8;

      enum Enum
      {
        Invalid,
        Running,
        Completed,
        Failed,

        Default = Invalid,
      };
    };

    static XII_ALWAYS_INLINE Result Running(xiiTime maxDelay = xiiTime::MakeZero()) { return {State::Running, maxDelay}; }
    static XII_ALWAYS_INLINE Result Completed() { return {State::Completed}; }
    static XII_ALWAYS_INLINE Result Failed() { return {State::Failed}; }

    xiiEnum<State> m_State;
    xiiTime        m_MaxDelay = xiiTime::MakeZero();
  };

  virtual void   StartWithVarArgs(xiiArrayPtr<xiiVariant> arguments) = 0;
  virtual void   Stop() {}
  virtual Result Update(xiiTime deltaTimeSinceLastUpdate) = 0;

  void UpdateAndSchedule(xiiTime deltaTimeSinceLastUpdate = xiiTime::MakeZero());

private:
  friend class xiiScriptWorldModule;
  void Initialize(xiiScriptCoroutineId id, xiiStringView sName, xiiScriptInstance& inout_instance, xiiScriptWorldModule& inout_ownerModule);
  void Deinitialize();

  static const xiiAbstractFunctionProperty* GetUpdateFunctionProperty();

  xiiScriptCoroutineId  m_Id;
  xiiHashedString       m_sName;
  xiiScriptInstance*    m_pInstance    = nullptr;
  xiiScriptWorldModule* m_pOwnerModule = nullptr;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutine);

/// Base class of coroutines which are implemented in C++ to allow automatic unpacking of the arguments from variants
template <typename Derived, class... Args>
class xiiTypedScriptCoroutine : public xiiScriptCoroutine
{
private:
  template <std::size_t... I>
  XII_ALWAYS_INLINE void StartImpl(xiiArrayPtr<xiiVariant> arguments, std::index_sequence<I...>)
  {
    static_cast<Derived*>(this)->Start(xiiVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void StartWithVarArgs(xiiArrayPtr<xiiVariant> arguments) override
  {
    StartImpl(arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};

/// Mode that decides what should happen if a new coroutine is created while there is already another coroutine running with the same name
/// on a given instance.
///
/// \sa xiiScriptWorldModule::CreateCoroutine
struct xiiScriptCoroutineCreationMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    StopOther,     ///< Stop the other coroutine before creating a new one with the same name.
    DontCreateNew, ///< Don't create a new coroutine if there is already one running with the same name.
    AllowOverlap,  ///< Allow multiple overlapping coroutines with the same name.

    Default = StopOther
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptCoroutineCreationMode);

/// A coroutine type that stores a custom allocator.
///
/// The custom allocator allows to pass more data to the created coroutine object than the default allocator.
/// E.g. this is used to pass the visual script graph to a visual script coroutine without the user needing to know
/// that the coroutine is actually implemented in visual script.
class XII_CORE_DLL xiiScriptCoroutineRTTI : public xiiRTTI, public xiiRefCountingImpl
{
public:
  xiiScriptCoroutineRTTI(xiiStringView sName, xiiUniquePtr<xiiRTTIAllocator>&& pAllocator);
  ~xiiScriptCoroutineRTTI();

private:
  xiiString                      m_sTypeNameStorage;
  xiiUniquePtr<xiiRTTIAllocator> m_pAllocatorStorage;
};

/// A function property that creates an instance of the given coroutine type and starts it immediately.
class XII_CORE_DLL xiiScriptCoroutineFunctionProperty : public xiiScriptFunctionProperty
{
public:
  xiiScriptCoroutineFunctionProperty(xiiStringView sName, const xiiSharedPtr<xiiScriptCoroutineRTTI>& pType, xiiScriptCoroutineCreationMode::Enum creationMode);
  ~xiiScriptCoroutineFunctionProperty();

  virtual xiiFunctionType::Enum         GetFunctionType() const override { return xiiFunctionType::Member; }
  virtual const xiiRTTI*                GetReturnType() const override { return nullptr; }
  virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override { return xiiPropertyFlags::Void; }
  virtual xiiUInt32                     GetArgumentCount() const override { return 0; }

  virtual const xiiRTTI* GetArgumentType(xiiUInt32 uiParamIndex) const override
  {
    XII_IGNORE_UNUSED(uiParamIndex);
    return nullptr;
  }

  virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override
  {
    XII_IGNORE_UNUSED(uiParamIndex);
    return xiiPropertyFlags::Void;
  }

  virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const override;

protected:
  xiiSharedPtr<xiiScriptCoroutineRTTI>    m_pType;
  xiiEnum<xiiScriptCoroutineCreationMode> m_CreationMode;
};

/// A message handler that creates an instance of the given coroutine type and starts it immediately.
class XII_CORE_DLL xiiScriptCoroutineMessageHandler : public xiiScriptMessageHandler
{
public:
  xiiScriptCoroutineMessageHandler(xiiStringView sName, const xiiScriptMessageDesc& desc, const xiiSharedPtr<xiiScriptCoroutineRTTI>& pType, xiiScriptCoroutineCreationMode::Enum creationMode);
  ~xiiScriptCoroutineMessageHandler();

  static void Dispatch(xiiAbstractMessageHandler* pSelf, void* pInstance, xiiMessage& ref_msg);

protected:
  xiiHashedString                         m_sName;
  xiiSharedPtr<xiiScriptCoroutineRTTI>    m_pType;
  xiiEnum<xiiScriptCoroutineCreationMode> m_CreationMode;
};

/// HashHelper implementation so coroutine handles can be used as key in a hash table. Also needed to store in a variant.
template <>
struct xiiHashHelper<xiiScriptCoroutineHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiScriptCoroutineHandle value) { return xiiHashHelper<xiiUInt32>::Hash(value.GetInternalID().m_Data); }

  XII_ALWAYS_INLINE static bool Equal(xiiScriptCoroutineHandle a, xiiScriptCoroutineHandle b) { return a == b; }
};

/// Currently not implemented as it is not needed for coroutine handles.
XII_ALWAYS_INLINE void operator<<(xiiStreamWriter& ref_stream, const xiiScriptCoroutineHandle& hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(hValue);
  XII_ASSERT_NOT_IMPLEMENTED;
}

XII_ALWAYS_INLINE void operator>>(xiiStreamReader& ref_stream, xiiScriptCoroutineHandle& ref_hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(ref_hValue);
  XII_ASSERT_NOT_IMPLEMENTED;
}
