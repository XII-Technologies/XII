#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTypeScriptMsgProxy);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTypeScriptMsgProxy, 1, xiiRTTIDefaultAllocator<xiiMsgTypeScriptMsgProxy>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTypeScriptComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Script", GetTypeScriptComponentFile, SetTypeScriptComponentFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Code_TypeScript", xiiDependencyFlags::Package)),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Script")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Scripting"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgTypeScriptMsgProxy, OnMsgTypeScriptMsgProxy)
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiTypeScriptComponent::xiiTypeScriptComponent()  = default;
xiiTypeScriptComponent::~xiiTypeScriptComponent() = default;

void xiiTypeScriptComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_TypeScriptComponentGuid;

  // version 3
  xiiUInt16 uiNumParams = static_cast<xiiUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (xiiUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void xiiTypeScriptComponent::DeserializeComponent(xiiWorldReader& stream)
{
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion >= 4)
  {
    SUPER::DeserializeComponent(stream);
  }

  auto& s = stream.GetStream();

  s >> m_TypeScriptComponentGuid;

  if (uiVersion >= 3)
  {
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

  // reset all user flags
  for (xiiUInt32 i = 0; i < 8; ++i)
  {
    SetUserFlag(i, false);
  }
}

bool xiiTypeScriptComponent::HandlesMessage(const xiiMessage& msg) const
{
  xiiTypeScriptBinding& binding = static_cast<const xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.HasMessageHandler(m_ComponentTypeInfo, msg.GetDynamicRTTI());
}

bool xiiTypeScriptComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg)
{
  return HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool xiiTypeScriptComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const
{
  return const_cast<xiiTypeScriptComponent*>(this)->HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool xiiTypeScriptComponent::HandleUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  xiiTypeScriptBinding& binding = static_cast<xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.DeliverMessage(m_ComponentTypeInfo, this, msg, bWasPostedMsg == false);
}

void xiiTypeScriptComponent::BroadcastEventMsg(xiiEventMessage& ref_msg)
{
  const xiiRTTI* pType = ref_msg.GetDynamicRTTI();

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
      return;
    }
  }

  auto& sender      = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
}

bool xiiTypeScriptComponent::CallTsFunc(const char* szFuncName)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  xiiTypeScriptBinding& binding = static_cast<xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  xiiDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(szFuncName).Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
    duk.PopStack(2);                         // [ ]

    XII_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    XII_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
  }
}

void xiiTypeScriptComponent::SetExposedVariables()
{
  xiiTypeScriptBinding& binding = static_cast<xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  xiiDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  for (xiiUInt32 p = 0; p < m_Parameters.GetCount(); ++p)
  {
    const auto& pair = m_Parameters.GetPair(p);

    xiiTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptComponent::Initialize()
{
  // SUPER::Initialize() does nothing

  if (!GetUserFlag(UserFlag::SimStartedTS))
    return;

  if (!GetUserFlag(UserFlag::InitializedTS))
  {
    SetUserFlag(UserFlag::InitializedTS, true);

    CallTsFunc("Initialize");
  }
}

void xiiTypeScriptComponent::Deinitialize()
{
  // mirror what xiiComponent::Deinitialize does, but make sure to CallTsFunc at the right time

  XII_ASSERT_DEV(GetOwner() != nullptr, "Owner must still be valid");

  if (IsActive())
  {
    SetActiveFlag(false);
  }

  if (GetUserFlag(UserFlag::InitializedTS))
  {
    CallTsFunc("Deinitialize");
  }

  SetUserFlag(UserFlag::InitializedTS, false);
}

void xiiTypeScriptComponent::OnActivated()
{
  // SUPER::OnActivated() does nothing

  if (!GetUserFlag(UserFlag::SimStartedTS))
    return;

  xiiTypeScriptComponent::Initialize();

  SetUserFlag(UserFlag::OnActivatedTS, true);

  CallTsFunc("OnActivated");
}

void xiiTypeScriptComponent::OnDeactivated()
{
  if (GetUserFlag(UserFlag::OnActivatedTS))
  {
    CallTsFunc("OnDeactivated");
  }

  SetUserFlag(UserFlag::OnActivatedTS, false);

  // SUPER::OnDeactivated() does nothing
}

void xiiTypeScriptComponent::OnSimulationStarted()
{
  xiiTypeScriptBinding& binding = static_cast<xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  SetUserFlag(UserFlag::SimStartedTS, true);

  if (binding.LoadComponent(m_TypeScriptComponentGuid, m_ComponentTypeInfo).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    xiiLog::Error("Failed to load TS component type.");
    return;
  }

  xiiUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_ComponentTypeInfo.Value().m_sComponentTypeName, GetHandle(), uiStashIdx, false).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    xiiLog::Error("Failed to register TS component type '{}'. Class may not exist under that name.", m_ComponentTypeInfo.Value().m_sComponentTypeName);
    return;
  }

  // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
  EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());

  SetExposedVariables();

  xiiTypeScriptComponent::OnActivated();

  CallTsFunc("OnSimulationStarted");
}

void xiiTypeScriptComponent::Update(xiiTypeScriptBinding& binding)
{
  if (GetUserFlag(UserFlag::ScriptFailure) || GetUserFlag(UserFlag::NoTsTick))
    return;

  if (m_UpdateInterval.IsNegative())
    return;

  const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_LastUpdate + m_UpdateInterval > tNow)
    return;

  XII_PROFILE_SCOPE(GetOwner()->GetName());

  m_LastUpdate = tNow;

  xiiDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall("Tick").Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
    duk.PopStack(2);                         // [ ]
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    SetUserFlag(UserFlag::NoTsTick, true);
  }

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiTypeScriptComponent::SetTypeScriptComponentFile(const char* szFile)
{
  if (xiiConversionUtils::IsStringUuid(szFile))
  {
    SetTypeScriptComponentGuid(xiiConversionUtils::ConvertStringToUuid(szFile));
  }
  else
  {
    SetTypeScriptComponentGuid(xiiUuid());
  }
}

const char* xiiTypeScriptComponent::GetTypeScriptComponentFile() const
{
  if (m_TypeScriptComponentGuid.IsValid())
  {
    static xiiStringBuilder sGuid; // need dummy storage
    return xiiConversionUtils::ToString(m_TypeScriptComponentGuid, sGuid);
  }

  return "";
}

void xiiTypeScriptComponent::SetTypeScriptComponentGuid(const xiiUuid& resource)
{
  m_TypeScriptComponentGuid = resource;
}

const xiiUuid& xiiTypeScriptComponent::GetTypeScriptComponentGuid() const
{
  return m_TypeScriptComponentGuid;
}

void xiiTypeScriptComponent::OnMsgTypeScriptMsgProxy(xiiMsgTypeScriptMsgProxy& msg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return;

  xiiTypeScriptBinding& binding = static_cast<xiiTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  binding.DeliverTsMessage(m_ComponentTypeInfo, this, msg);
}

const xiiRangeView<const char*, xiiUInt32> xiiTypeScriptComponent::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                              [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                              [this](const xiiUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void xiiTypeScriptComponent::SetParameter(const char* szKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  // GetWorld()->GetComponentManager<xiiTypeScriptComponentManager>()->AddToUpdateList(this);
}

void xiiTypeScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(xiiTempHashedString(szKey)))
  {
    // GetWorld()->GetComponentManager<xiiTypeScriptComponentManager>()->AddToUpdateList(this);
  }
}

bool xiiTypeScriptComponent::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(szKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}
