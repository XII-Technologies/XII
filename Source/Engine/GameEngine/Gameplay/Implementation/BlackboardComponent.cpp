#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlackboardEntry, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBlackboardEntry>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    XII_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new xiiDefaultValueAttribute(0)),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiBlackboardEntryFlags, m_Flags)
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiBlackboardEntry::Serialize(xiiStreamWriter& stream) const
{
  stream << m_sName;
  stream << m_InitialValue;
  stream << m_Flags;

  return XII_SUCCESS;
}

xiiResult xiiBlackboardEntry::Deserialize(xiiStreamReader& stream)
{
  stream >> m_sName;
  stream >> m_InitialValue;
  stream >> m_Flags;

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
  XII_BEGIN_ATTRIBUTES
  {
      new xiiAutoGenVisScriptMsgHandler()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBlackboardComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    XII_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;

  XII_BEGIN_MESSAGESENDERS
  {
    XII_MESSAGE_SENDER(m_EntryChangedSender)
  }
  XII_END_MESSAGESENDERS;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name"),
  }
  XII_END_FUNCTIONS;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiBlackboardComponent::xiiBlackboardComponent() :
  m_pBoard(xiiBlackboard::Create())
{
}

xiiBlackboardComponent::xiiBlackboardComponent(xiiBlackboardComponent&& other) = default;
xiiBlackboardComponent::~xiiBlackboardComponent()                              = default;
xiiBlackboardComponent& xiiBlackboardComponent::operator=(xiiBlackboardComponent&& other) = default;

// static
xiiSharedPtr<xiiBlackboard> xiiBlackboardComponent::FindBlackboard(xiiGameObject* pObject, xiiStringView sBlackboardName /*= xiiStringView()*/)
{
  xiiTempHashedString sBlackboardNameHashed(sBlackboardName);

  xiiBlackboardComponent* pBlackboardComponent = nullptr;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed)
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

void xiiBlackboardComponent::OnActivated()
{
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
}

void xiiBlackboardComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void xiiBlackboardComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  xiiStreamReader& s = stream.GetStream();

  xiiStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  xiiDynamicArray<xiiBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (auto& entry : initialEntries)
    {
      m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
    }
  }
}

const xiiSharedPtr<xiiBlackboard>& xiiBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

xiiSharedPtr<const xiiBlackboard> xiiBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage
  };
};

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

void xiiBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(xiiMakeDelegate(&xiiBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(xiiMakeDelegate(&xiiBlackboardComponent::OnEntryChanged, this));
  }
}

bool xiiBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void xiiBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_pBoard->SetName(szName);
}

const char* xiiBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}

void xiiBlackboardComponent::SetEntryValue(const char* szName, const xiiVariant& value)
{
  if (m_pBoard->SetEntryValue(xiiTempHashedString(szName), value).Failed())
  {
    xiiLog::Error("Can't set blackboard entry '{}', because it doesn't exist.", szName);
  }
}

xiiVariant xiiBlackboardComponent::GetEntryValue(const char* szName)
{
  return m_pBoard->GetEntryValue(xiiTempHashedString(szName));
}

xiiUInt32 xiiBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const xiiBlackboardEntry& xiiBlackboardComponent::Entries_GetValue(xiiUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void xiiBlackboardComponent::Entries_SetValue(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);

  m_pBoard->UnregisterEntry(m_InitialEntries[uiIndex].m_sName);

  m_InitialEntries[uiIndex] = entry;

  m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
}

void xiiBlackboardComponent::Entries_Insert(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry)
{
  m_InitialEntries.Insert(entry, uiIndex);

  m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
}

void xiiBlackboardComponent::Entries_Remove(xiiUInt32 uiIndex)
{
  auto& entry = m_InitialEntries[uiIndex];
  m_pBoard->UnregisterEntry(entry.m_sName);

  m_InitialEntries.RemoveAtAndCopy(uiIndex);
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
  if (!GetShowDebugInfo())
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

void xiiBlackboardComponent::OnEntryChanged(const xiiBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  xiiMsgBlackboardEntryChanged msg;
  msg.m_sName    = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}
