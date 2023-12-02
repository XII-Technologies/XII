#include "Foundation/Serialization/AbstractObjectGraph.h"

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/View.h>

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage,
    InitializedFromTemplate
  };
};

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlackboardEntry, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBlackboardEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDynamicStringEnumAttribute("BlackboardKeysEnum")),
    XII_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new xiiDefaultValueAttribute(0)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiBlackboardEntryFlags, m_Flags)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiBlackboardEntry::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_InitialValue;
  inout_stream << m_Flags;

  return XII_SUCCESS;
}

xiiResult xiiBlackboardEntry::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_InitialValue;
  inout_stream >> m_Flags;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgBlackboardEntryChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgBlackboardEntryChanged, 1, xiiRTTIDefaultAllocator<xiiMsgBlackboardEntryChanged>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    XII_MEMBER_PROPERTY("OldValue", m_OldValue),
    XII_MEMBER_PROPERTY("NewValue", m_NewValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiBlackboardComponent, 3)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_FindBlackboard, In, "SearchObject", In, "BlackboardName")->AddFlags(xiiPropertyFlags::Const)->AddAttributes(new xiiFunctionArgumentAttributes(1, new xiiDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
  }
  XII_END_FUNCTIONS;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiBlackboardComponent::xiiBlackboardComponent()  = default;
xiiBlackboardComponent::~xiiBlackboardComponent() = default;

// static
xiiSharedPtr<xiiBlackboard> xiiBlackboardComponent::FindBlackboard(xiiGameObject* pObject, xiiStringView sBlackboardName /*= xiiStringView()*/)
{
  const xiiTempHashedString sBlackboardNameHashed(sBlackboardName);

  xiiBlackboardComponent* pBlackboardComponent = nullptr;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || (pBlackboardComponent->GetBoard() && pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed))
      {
        return pBlackboardComponent->GetBoard();
      }
    }

    pObject = pObject->GetParent();
  }

  if (sBlackboardName.IsEmpty() == false)
  {
    xiiHashedString sHashedBlackboardName;
    sHashedBlackboardName.Assign(sBlackboardName);
    return xiiBlackboard::GetOrCreateGlobal(sHashedBlackboardName);
  }

  return nullptr;
}

void xiiBlackboardComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplate;
}

void xiiBlackboardComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  if (uiVersion < 3)
    return;

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplate;
}

void xiiBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiBlackboardComponent::OnDeactivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }

  SUPER::OnDeactivated();
}

const xiiSharedPtr<xiiBlackboard>& xiiBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

xiiSharedPtr<const xiiBlackboard> xiiBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

void xiiBlackboardComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(BCFlags::ShowDebugInfo, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool xiiBlackboardComponent::GetShowDebugInfo() const
{
  return GetUserFlag(BCFlags::ShowDebugInfo);
}

void xiiBlackboardComponent::SetTemplateFile(const char* szName)
{
  xiiBlackboardTemplateResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szName))
  {
    hResource = xiiResourceManager::LoadResource<xiiBlackboardTemplateResource>(szName);
  }

  m_hTemplate = hResource;
}

const char* xiiBlackboardComponent::GetTemplateFile() const
{
  if (m_hTemplate.IsValid())
  {
    return m_hTemplate.GetResourceID();
  }

  return "";
}

void xiiBlackboardComponent::SetEntryValue(const char* szName, const xiiVariant& value)
{
  if (m_pBoard)
  {
    m_pBoard->SetEntryValue(szName, value);
  }
}

xiiVariant xiiBlackboardComponent::GetEntryValue(const char* szName) const
{
  if (m_pBoard)
  {
    return m_pBoard->GetEntryValue(xiiTempHashedString(szName));
  }

  return {};
}

// static
xiiBlackboard* xiiBlackboardComponent::Reflection_FindBlackboard(xiiGameObject* pSearchObject, xiiStringView sBlackboardName)
{
  return FindBlackboard(pSearchObject, sBlackboardName).Borrow();
}

void xiiBlackboardComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const
{
  if (GetShowDebugInfo())
  {
    msg.AddBounds(xiiBoundingSphere(xiiVec3::ZeroVector(), 2.0f), xiiDefaultSpatialDataCategories::RenderDynamic);
  }
}

void xiiBlackboardComponent::OnExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo() || m_pBoard == nullptr)
    return;

  if (msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  auto& entries = m_pBoard->GetAllEntries();
  if (entries.IsEmpty())
    return;

  xiiStringBuilder sb;
  sb.Append(m_pBoard->GetName(), "\n");

  for (auto it = entries.GetIterator(); it.IsValid(); ++it)
  {
    sb.AppendFormat("{}: {}\n", it.Key(), it.Value().m_Value);
  }

  xiiDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), xiiColor::Orange);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLocalBlackboardComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new xiiDynamicStringEnumAttribute("BlackboardNamesEnum")),
    XII_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    XII_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGESENDERS
  {
    XII_MESSAGE_SENDER(m_EntryChangedSender)
  }
  XII_END_MESSAGESENDERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiLocalBlackboardComponent::xiiLocalBlackboardComponent()
{
  m_pBoard = xiiBlackboard::Create();
}

xiiLocalBlackboardComponent::xiiLocalBlackboardComponent(xiiLocalBlackboardComponent&& other) = default;
xiiLocalBlackboardComponent::~xiiLocalBlackboardComponent()                                   = default;
xiiLocalBlackboardComponent& xiiLocalBlackboardComponent::operator=(xiiLocalBlackboardComponent&& other) = default;

void xiiLocalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void xiiLocalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void xiiLocalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void xiiLocalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void xiiLocalBlackboardComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void xiiLocalBlackboardComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  const xiiUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(xiiBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  xiiStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);
  m_pBoard->RemoveAllEntries();

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  xiiDynamicArray<xiiBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (auto& entry : initialEntries)
    {
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}

void xiiLocalBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(xiiMakeDelegate(&xiiLocalBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(xiiMakeDelegate(&xiiLocalBlackboardComponent::OnEntryChanged, this));
  }
}

bool xiiLocalBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void xiiLocalBlackboardComponent::SetBlackboardName(xiiStringView sName)
{
  m_pBoard->SetName(sName);
}

xiiStringView xiiLocalBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}


xiiUInt32 xiiLocalBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const xiiBlackboardEntry& xiiLocalBlackboardComponent::Entries_GetValue(xiiUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void xiiLocalBlackboardComponent::Entries_SetValue(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);

  if (const xiiBlackboard::Entry* pEntry = m_pBoard->GetEntry(m_InitialEntries[uiIndex].m_sName))
  {
    if (m_InitialEntries[uiIndex].m_sName != entry.m_sName)
    {
      m_pBoard->RemoveEntry(m_InitialEntries[uiIndex].m_sName);
    }
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();

  m_InitialEntries[uiIndex] = entry;
}

void xiiLocalBlackboardComponent::Entries_Insert(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry)
{
  m_InitialEntries.Insert(entry, uiIndex);

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
}

void xiiLocalBlackboardComponent::Entries_Remove(xiiUInt32 uiIndex)
{
  auto& entry = m_InitialEntries[uiIndex];
  m_pBoard->RemoveEntry(entry.m_sName);

  m_InitialEntries.RemoveAtAndCopy(uiIndex);
}

void xiiLocalBlackboardComponent::OnEntryChanged(const xiiBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  xiiMsgBlackboardEntryChanged msg;
  msg.m_sName    = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}

void xiiLocalBlackboardComponent::InitializeFromTemplate()
{
  if (!m_hTemplate.IsValid())
    return;

  xiiResourceLock<xiiBlackboardTemplateResource> pTemplate(m_hTemplate, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGlobalBlackboardInitMode, 1)
  XII_ENUM_CONSTANTS(xiiGlobalBlackboardInitMode::EnsureEntriesExist, xiiGlobalBlackboardInitMode::ResetEntryValues, xiiGlobalBlackboardInitMode::ClearEntireBlackboard)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiGlobalBlackboardComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new xiiDynamicStringEnumAttribute("BlackboardNamesEnum")),
    XII_ENUM_MEMBER_PROPERTY("InitMode", xiiGlobalBlackboardInitMode, m_InitMode),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiGlobalBlackboardComponent::xiiGlobalBlackboardComponent()                                     = default;
xiiGlobalBlackboardComponent::xiiGlobalBlackboardComponent(xiiGlobalBlackboardComponent&& other) = default;
xiiGlobalBlackboardComponent::~xiiGlobalBlackboardComponent()                                    = default;
xiiGlobalBlackboardComponent& xiiGlobalBlackboardComponent::operator=(xiiGlobalBlackboardComponent&& other) = default;

void xiiGlobalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void xiiGlobalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void xiiGlobalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void xiiGlobalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void xiiGlobalBlackboardComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sName;
  s << m_InitMode;
}

void xiiGlobalBlackboardComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  const xiiUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(xiiBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sName;
  s >> m_InitMode;
}

void xiiGlobalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* xiiGlobalBlackboardComponent::GetBlackboardName() const
{
  return m_sName;
}

void xiiGlobalBlackboardComponent::InitializeFromTemplate()
{
  m_pBoard = xiiBlackboard::GetOrCreateGlobal(m_sName);

  if (m_InitMode == xiiGlobalBlackboardInitMode::ClearEntireBlackboard)
  {
    m_pBoard->RemoveAllEntries();
  }

  if (!m_hTemplate.IsValid())
    return;

  xiiResourceLock<xiiBlackboardTemplateResource> pTemplate(m_hTemplate, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (!m_pBoard->HasEntry(entry.m_sName) || m_InitMode != xiiGlobalBlackboardInitMode::EnsureEntriesExist)
    {
      // make sure the entry exists and enforce that it has this value
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      // also overwrite the flags
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}


class xiiBlackboardComponent_2_3 : public xiiGraphPatch
{
public:
  xiiBlackboardComponent_2_3() :
    xiiGraphPatch("xiiBlackboardComponent", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("xiiLocalBlackboardComponent");
  }
};

xiiBlackboardComponent_2_3 g_xiiBlackboardComponent_2_3;


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_BlackboardComponent);
