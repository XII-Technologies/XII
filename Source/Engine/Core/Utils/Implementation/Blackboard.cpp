#include <Core/CorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiBlackboardEntryFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiBlackboardEntryFlags::Save, xiiBlackboardEntryFlags::OnChangeEvent,
    xiiBlackboardEntryFlags::UserFlag0, xiiBlackboardEntryFlags::UserFlag1, xiiBlackboardEntryFlags::UserFlag2, xiiBlackboardEntryFlags::UserFlag3, xiiBlackboardEntryFlags::UserFlag4, xiiBlackboardEntryFlags::UserFlag5, xiiBlackboardEntryFlags::UserFlag6, xiiBlackboardEntryFlags::UserFlag7)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, Blackboard)

  ON_CORESYSTEMS_SHUTDOWN
  {
    XII_LOCK(xiiBlackboard::s_GlobalBlackboardsMutex);
    xiiBlackboard::s_GlobalBlackboards.Clear();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
xiiMutex                                                   xiiBlackboard::s_GlobalBlackboardsMutex;
xiiHashTable<xiiHashedString, xiiSharedPtr<xiiBlackboard>> xiiBlackboard::s_GlobalBlackboards;

// static
xiiSharedPtr<xiiBlackboard> xiiBlackboard::Create(xiiAllocatorBase* pAllocator /*= xiiFoundation::GetDefaultAllocator()*/)
{
  return XII_NEW(pAllocator, xiiBlackboard);
}

// static
xiiSharedPtr<xiiBlackboard> xiiBlackboard::GetOrCreateGlobal(const xiiHashedString& sBlackboardName, xiiAllocatorBase* pAllocator /*= xiiFoundation::GetDefaultAllocator()*/)
{
  XII_LOCK(s_GlobalBlackboardsMutex);

  auto it = s_GlobalBlackboards.Find(sBlackboardName);

  if (it.IsValid())
  {
    return it.Value();
  }

  xiiSharedPtr<xiiBlackboard> pShrd = XII_NEW(pAllocator, xiiBlackboard);
  pShrd->m_sName                    = sBlackboardName;
  s_GlobalBlackboards.Insert(sBlackboardName, pShrd);

  return pShrd;
}

// static
xiiSharedPtr<xiiBlackboard> xiiBlackboard::FindGlobal(const xiiTempHashedString& sBlackboardName)
{
  XII_LOCK(s_GlobalBlackboardsMutex);

  xiiSharedPtr<xiiBlackboard> pBlackboard;
  s_GlobalBlackboards.TryGetValue(sBlackboardName, pBlackboard);
  return pBlackboard;
}

xiiBlackboard::xiiBlackboard()  = default;
xiiBlackboard::~xiiBlackboard() = default;

void xiiBlackboard::SetName(xiiStringView sName)
{
  XII_LOCK(s_GlobalBlackboardsMutex);
  m_sName.Assign(sName);
}

void xiiBlackboard::RegisterEntry(const xiiHashedString& name, const xiiVariant& initialValue, xiiBitflags<xiiBlackboardEntryFlags> flags /*= xiiBlackboardEntryFlags::None*/)
{
  XII_ASSERT_ALWAYS(!flags.IsSet(xiiBlackboardEntryFlags::Invalid), "The invalid flag is reserved for internal use.");

  bool   bExisted = false;
  Entry& entry    = m_Entries.FindOrAdd(name, &bExisted);

  if (!bExisted || entry.m_Flags != flags)
  {
    ++m_uiBlackboardChangeCounter;
    entry.m_Flags |= flags;
  }

  if (!bExisted && entry.m_Value != initialValue)
  {
    // broadcasts the change event, in case we overwrite an existing entry
    SetEntryValue(name, initialValue).IgnoreResult();
  }
}

void xiiBlackboard::UnregisterEntry(const xiiHashedString& name)
{
  if (m_Entries.Remove(name))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void xiiBlackboard::UnregisterAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

xiiResult xiiBlackboard::SetEntryValue(const xiiTempHashedString& name, const xiiVariant& value, bool force /*= false*/)
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
  {
    return XII_FAILURE;
  }

  Entry& entry = itEntry.Value();

  if (!force && entry.m_Value == value)
    return XII_SUCCESS;

  ++m_uiBlackboardEntryChangeCounter;
  ++entry.m_uiChangeCounter;

  if (entry.m_Flags.IsSet(xiiBlackboardEntryFlags::OnChangeEvent))
  {
    EntryEvent e;
    e.m_sName    = itEntry.Key();
    e.m_OldValue = entry.m_Value;
    e.m_pEntry   = &entry;

    entry.m_Value = value;

    m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
  }
  else
  {
    entry.m_Value = value;
  }

  return XII_SUCCESS;
}

const xiiBlackboard::Entry* xiiBlackboard::GetEntry(const xiiTempHashedString& name) const
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
    return nullptr;

  return &itEntry.Value();
}

xiiVariant xiiBlackboard::GetEntryValue(const xiiTempHashedString& name, xiiVariant fallback) const
{
  auto value = m_Entries.GetValue(name);
  return value != nullptr ? value->m_Value : fallback;
}

xiiBitflags<xiiBlackboardEntryFlags> xiiBlackboard::GetEntryFlags(const xiiTempHashedString& name) const
{
  auto itEntry = m_Entries.Find(name);

  if (!itEntry.IsValid())
  {
    return xiiBlackboardEntryFlags::Invalid;
  }

  return itEntry.Value().m_Flags;
}

xiiResult xiiBlackboard::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  xiiUInt32 uiEntries = 0;

  for (auto it : m_Entries)
  {
    if (it.Value().m_Flags.IsSet(xiiBlackboardEntryFlags::Save))
    {
      ++uiEntries;
    }
  }

  stream << uiEntries;

  for (auto it : m_Entries)
  {
    const Entry& e = it.Value();

    if (e.m_Flags.IsSet(xiiBlackboardEntryFlags::Save))
    {
      stream << it.Key();
      stream << e.m_Flags;
      stream << e.m_Value;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiBlackboard::Deserialize(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  xiiUInt32 uiEntries = 0;
  stream >> uiEntries;

  for (xiiUInt32 e = 0; e < uiEntries; ++e)
  {
    xiiHashedString name;
    stream >> name;

    xiiBitflags<xiiBlackboardEntryFlags> flags;
    stream >> flags;

    xiiVariant value;
    stream >> value;

    RegisterEntry(name, value, flags);
  }

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlackboardCondition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBlackboardCondition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("EntryName", GetEntryName, SetEntryName),
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiComparisonOperator, m_Operator),
    XII_MEMBER_PROPERTY("ComparisonValue", m_fComparisonValue),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiBlackboardCondition::IsConditionMet(const xiiBlackboard& blackboard) const
{
  auto pEntry = blackboard.GetEntry(m_sEntryName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    double fEntryValue = pEntry->m_Value.ConvertTo<double>();
    return xiiComparisonOperator::Compare(m_Operator, fEntryValue, m_fComparisonValue);
  }

  return false;
}

constexpr xiiTypeVersion s_BlackboardConditionVersion = 1;

xiiResult xiiBlackboardCondition::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(s_BlackboardConditionVersion);

  stream << m_sEntryName;
  stream << m_Operator;
  stream << m_fComparisonValue;
  return XII_SUCCESS;
}

xiiResult xiiBlackboardCondition::Deserialize(xiiStreamReader& stream)
{
  const xiiTypeVersion uiVersion = stream.ReadVersion(s_BlackboardConditionVersion);

  stream >> m_sEntryName;
  stream >> m_Operator;
  stream >> m_fComparisonValue;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
