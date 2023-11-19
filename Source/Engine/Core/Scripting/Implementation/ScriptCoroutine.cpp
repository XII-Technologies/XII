#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptCoroutineHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiScriptCoroutineHandle>)
XII_END_STATIC_REFLECTED_TYPE;
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiScriptCoroutineHandle);

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptCoroutine, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(UpdateAndSchedule),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiScriptCoroutine::xiiScriptCoroutine() = default;

xiiScriptCoroutine::~xiiScriptCoroutine()
{
  XII_ASSERT_DEV(m_pOwnerModule == nullptr, "Deinitialize was not called");
}

void xiiScriptCoroutine::UpdateAndSchedule(xiiTime deltaTimeSinceLastUpdate)
{
  auto result = Update(deltaTimeSinceLastUpdate);

  // Has been deleted during update
  if (m_pOwnerModule == nullptr)
    return;

  if (result.m_State == Result::State::Running)
  {
    // We can safely pass false here since we would not end up here if the coroutine is used in a simulation only function
    // but the simulation is not running because then the outer function should not have been called.
    const bool bOnlyWhenSimulating = false;
    m_pOwnerModule->AddUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this, result.m_MaxDelay, bOnlyWhenSimulating);
  }
  else
  {
    m_pOwnerModule->StopAndDeleteCoroutine(GetHandle());
  }
}

void xiiScriptCoroutine::Initialize(xiiScriptCoroutineId id, xiiStringView sName, xiiScriptInstance& inout_instance, xiiScriptWorldModule& inout_ownerModule)
{
  m_Id = id;
  m_sName.Assign(sName);
  m_pInstance    = &inout_instance;
  m_pOwnerModule = &inout_ownerModule;
}

void xiiScriptCoroutine::Deinitialize()
{
  m_pOwnerModule->RemoveUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this);
  m_pOwnerModule = nullptr;
}

// static
const xiiAbstractFunctionProperty* xiiScriptCoroutine::GetUpdateFunctionProperty()
{
  static const xiiAbstractFunctionProperty* pUpdateFunctionProperty = []() -> const xiiAbstractFunctionProperty* {
    const xiiRTTI* pType     = xiiGetStaticRTTI<xiiScriptCoroutine>();
    auto           functions = pType->GetFunctions();
    for (auto pFunc : functions)
    {
      if (pFunc->GetPropertyName() == "UpdateAndSchedule")
      {
        return pFunc;
      }
    }
    return nullptr;
  }();

  return pUpdateFunctionProperty;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiScriptCoroutineCreationMode, 1)
  XII_ENUM_CONSTANTS(xiiScriptCoroutineCreationMode::StopOther, xiiScriptCoroutineCreationMode::DontCreateNew, xiiScriptCoroutineCreationMode::AllowOverlap)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiScriptCoroutineRTTI::xiiScriptCoroutineRTTI(xiiStringView sName, xiiUniquePtr<xiiRTTIAllocator>&& pAllocator) :
  xiiRTTI(nullptr, xiiGetStaticRTTI<xiiScriptCoroutine>(), 0, 1, xiiVariantType::Invalid, xiiTypeFlags::Class, nullptr, xiiArrayPtr<const xiiAbstractProperty*>(), xiiArrayPtr<const xiiAbstractFunctionProperty*>(), xiiArrayPtr<const xiiPropertyAttribute*>(), xiiArrayPtr<xiiAbstractMessageHandler*>(), xiiArrayPtr<xiiMessageSenderInfo>(), nullptr), m_sTypeNameStorage(sName), m_pAllocatorStorage(std::move(pAllocator))
{
  m_sTypeName  = m_sTypeNameStorage;
  m_pAllocator = m_pAllocatorStorage.Borrow();

  RegisterType();

  SetupParentHierarchy();
}

xiiScriptCoroutineRTTI::~xiiScriptCoroutineRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;
}

//////////////////////////////////////////////////////////////////////////

xiiScriptCoroutineFunctionProperty::xiiScriptCoroutineFunctionProperty(xiiStringView sName, const xiiSharedPtr<xiiScriptCoroutineRTTI>& pType, xiiScriptCoroutineCreationMode::Enum creationMode) :
  xiiScriptFunctionProperty(sName), m_pType(pType), m_CreationMode(creationMode)
{
}

xiiScriptCoroutineFunctionProperty::~xiiScriptCoroutineFunctionProperty() = default;

void xiiScriptCoroutineFunctionProperty::Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& out_returnValue) const
{
  XII_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pScriptInstance = static_cast<xiiScriptInstance*>(pInstance);

  xiiWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    xiiLog::Error("Script coroutines need a script instance with a valid xiiWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<xiiScriptWorldModule>();

  xiiScriptCoroutine* pCoroutine = nullptr;
  auto                hCoroutine = pModule->CreateCoroutine(m_pType.Borrow(), m_sPropertyName, *pScriptInstance, m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    xiiHybridArray<xiiVariant, 8> finalArgs;
    finalArgs = arguments;
    finalArgs.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, finalArgs);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiScriptCoroutineMessageHandler::xiiScriptCoroutineMessageHandler(xiiStringView sName, const xiiScriptMessageDesc& desc, const xiiSharedPtr<xiiScriptCoroutineRTTI>& pType, xiiScriptCoroutineCreationMode::Enum creationMode) :
  xiiScriptMessageHandler(desc), m_pType(pType), m_CreationMode(creationMode)
{
  m_sName.Assign(sName);
  m_DispatchFunc = &Dispatch;
}

xiiScriptCoroutineMessageHandler::~xiiScriptCoroutineMessageHandler() = default;

// static
void xiiScriptCoroutineMessageHandler::Dispatch(xiiAbstractMessageHandler* pSelf, void* pInstance, xiiMessage& ref_msg)
{
  XII_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance.");
  auto pHandler        = static_cast<xiiScriptCoroutineMessageHandler*>(pSelf);
  auto pComponent      = static_cast<xiiScriptComponent*>(pInstance);
  auto pScriptInstance = pComponent->GetScriptInstance();

  xiiWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    xiiLog::Error("Script coroutines need a script instance with a valid xiiWorld.");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<xiiScriptWorldModule>();

  xiiScriptCoroutine* pCoroutine = nullptr;
  auto                hCoroutine = pModule->CreateCoroutine(pHandler->m_pType.Borrow(), pHandler->m_sName, *pScriptInstance, pHandler->m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    xiiHybridArray<xiiVariant, 8> arguments;
    pHandler->FillMessagePropertyValues(ref_msg, arguments);
    arguments.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, arguments);
  }
}
