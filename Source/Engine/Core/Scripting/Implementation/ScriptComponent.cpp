#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiScriptComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("UpdateInterval", GetUpdateInterval, SetUpdateInterval)->AddAttributes(new xiiClampValueAttribute(xiiTime::Zero(), xiiVariant())),
    XII_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("ScriptClass")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Scripting"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Alpha),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptComponent::xiiScriptComponent()  = default;
xiiScriptComponent::~xiiScriptComponent() = default;

void xiiScriptComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hScriptClass;
  s << m_UpdateInterval;

  xiiUInt16 uiNumParams = static_cast<xiiUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (xiiUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void xiiScriptComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hScriptClass;
  s >> m_UpdateInterval;

  xiiUInt16 uiNumParams = 0;
  s >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  xiiHashedString key;
  xiiVariant      value;
  for (xiiUInt32 p = 0; p < uiNumParams; ++p)
  {
    s >> key;
    s >> value;

    m_Parameters.Insert(key, value);
  }
}

void xiiScriptComponent::Initialize()
{
  SUPER::Initialize();

  if (m_hScriptClass.IsValid())
  {
    InstantiateScript(false);
  }
}

void xiiScriptComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ClearInstance(false);
}

void xiiScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::OnActivated);
}

void xiiScriptComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::OnDeactivated);
}

void xiiScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::OnSimulationStarted);
}

void xiiScriptComponent::BroadcastEventMsg(xiiEventMessage& inout_msg)
{
  const xiiRTTI* pType = inout_msg.GetDynamicRTTI();
  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(inout_msg, this, GetOwner());
      return;
    }
  }

  auto& sender      = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(inout_msg, this, GetOwner());
}

void xiiScriptComponent::SetScriptClass(const xiiScriptClassResourceHandle& hScript)
{
  if (m_hScriptClass == hScript)
    return;

  if (IsInitialized())
  {
    ClearInstance(IsActiveAndInitialized());
  }

  m_hScriptClass = hScript;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void xiiScriptComponent::SetScriptClassFile(xiiStringView sFile)
{
  xiiScriptClassResourceHandle hScript;

  if (!sFile.IsEmpty())
  {
    hScript = xiiResourceManager::LoadResource<xiiScriptClassResource>(sFile);
  }

  SetScriptClass(hScript);
}

xiiStringView xiiScriptComponent::GetScriptClassFile() const
{
  return m_hScriptClass.IsValid() ? m_hScriptClass.GetResourceID().GetData() : "";
}

void xiiScriptComponent::SetUpdateInterval(xiiTime interval)
{
  m_UpdateInterval = interval;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

xiiTime xiiScriptComponent::GetUpdateInterval() const
{
  return m_UpdateInterval;
}

const xiiRangeView<xiiStringView, xiiUInt32> xiiScriptComponent::GetParameters() const
{
  return xiiRangeView<xiiStringView, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                              [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                              [this](const xiiUInt32& uiIt) -> xiiStringView { return m_Parameters.GetKey(uiIt); });
}

void xiiScriptComponent::SetParameter(xiiStringView sKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(sKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiScriptComponent::RemoveParameter(xiiStringView sKey)
{
  if (m_Parameters.RemoveAndCopy(xiiTempHashedString(sKey)))
  {
  }
}

bool xiiScriptComponent::GetParameter(xiiStringView sKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(sKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void xiiScriptComponent::InstantiateScript(bool bActivate)
{
  ClearInstance(IsActiveAndInitialized());

  xiiResourceLock<xiiScriptClassResource> pScript(m_hScriptClass, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    xiiLog::Error("Failed to load script '{}'", GetScriptClassFile());
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || pScriptType->IsDerivedFrom(xiiGetStaticRTTI<xiiComponent>()) == false)
  {
    xiiLog::Error("Script type '{}' is not a component", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
    return;
  }

  m_pScriptType          = pScriptType;
  m_pMessageDispatchType = pScriptType;

  m_pInstance = pScript->Instantiate(*this, GetWorld());
  if (m_pInstance != nullptr)
  {
    m_pInstance->ApplyParameters(m_Parameters);
  }

  UpdateScheduling();

  CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::Initialize);
  if (bActivate)
  {
    CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::OnActivated);
  }
}

void xiiScriptComponent::ClearInstance(bool bDeactivate)
{
  if (bDeactivate)
  {
    CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::OnDeactivated);
  }
  CallScriptFunction(xiiComponent_ScriptBaseClassFunctions::Deinitialize);

  auto pModule = GetWorld()->GetOrCreateModule<xiiScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(xiiComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->RemoveUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow());
  }

  pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());
  pModule->RemoveScriptReloadFunction(m_hScriptClass, xiiMakeDelegate(&xiiScriptComponent::ReloadScript, this));

  m_pInstance   = nullptr;
  m_pScriptType = nullptr;

  m_pMessageDispatchType = GetDynamicRTTI();
}

void xiiScriptComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<xiiScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(xiiComponent_ScriptBaseClassFunctions::Update))
  {
    const bool bOnlyWhenSimulating = true;
    pModule->AddUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow(), m_UpdateInterval, bOnlyWhenSimulating);
  }

  pModule->AddScriptReloadFunction(m_hScriptClass, xiiMakeDelegate(&xiiScriptComponent::ReloadScript, this));
}

const xiiAbstractFunctionProperty* xiiScriptComponent::GetScriptFunction(xiiUInt32 uiFunctionIndex)
{
  if (m_pScriptType != nullptr && m_pInstance != nullptr)
  {
    return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
  }

  return nullptr;
}

void xiiScriptComponent::CallScriptFunction(xiiUInt32 uiFunctionIndex)
{
  if (auto pFunction = GetScriptFunction(uiFunctionIndex))
  {
    xiiVariant returnValue;
    pFunction->Execute(m_pInstance.Borrow(), xiiArrayPtr<xiiVariant>(), returnValue);
  }
}

void xiiScriptComponent::ReloadScript()
{
  InstantiateScript(IsActiveAndInitialized());
}
