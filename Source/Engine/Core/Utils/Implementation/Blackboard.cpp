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
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlackboard, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOrCreateGlobal, In, "Name")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_FindGlobal, In, "Name")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardNamesEnum"))),

    XII_SCRIPT_FUNCTION_PROPERTY(GetName),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetEntryValue, In, "Name", In, "Value")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name", In, "Fallback")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(IncrementEntryValue, In, "Name")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(DecrementEntryValue, In, "Name")->AddAttributes(new xiiFunctionArgumentAttributes(0, new xiiDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    XII_SCRIPT_FUNCTION_PROPERTY(GetBlackboardChangeCounter),
    XII_SCRIPT_FUNCTION_PROPERTY(GetBlackboardEntryChangeCounter)
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

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
  return XII_NEW(pAllocator, xiiBlackboard, false);
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

  xiiSharedPtr<xiiBlackboard> pShrd = XII_NEW(pAllocator, xiiBlackboard, true);
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

xiiBlackboard::xiiBlackboard(bool bIsGlobal)
{
  m_bIsGlobal = bIsGlobal;
}

xiiBlackboard::~xiiBlackboard() = default;

void xiiBlackboard::SetName(xiiStringView sName)
{
  XII_LOCK(s_GlobalBlackboardsMutex);
  m_sName.Assign(sName);
}

void xiiBlackboard::RemoveEntry(const xiiHashedString& sName)
{
  if (m_Entries.Remove(sName))
  {
    ++m_uiBlackboardChangeCounter;
  }
}

void xiiBlackboard::RemoveAllEntries()
{
  if (m_Entries.IsEmpty() == false)
  {
    ++m_uiBlackboardChangeCounter;
  }

  m_Entries.Clear();
}

void xiiBlackboard::ImplSetEntryValue(const xiiHashedString& sName, Entry& entry, const xiiVariant& value)
{
  if (entry.m_Value != value)
  {
    ++m_uiBlackboardEntryChangeCounter;
    ++entry.m_uiChangeCounter;

    if (entry.m_Flags.IsSet(xiiBlackboardEntryFlags::OnChangeEvent))
    {
      EntryEvent e;
      e.m_sName    = sName;
      e.m_OldValue = entry.m_Value;
      e.m_pEntry   = &entry;

      m_EntryEvents.Broadcast(e, 1); // limited recursion is allowed
    }

    entry.m_Value = value;
  }
}

void xiiBlackboard::SetEntryValue(xiiStringView sName, const xiiVariant& value)
{
  const xiiTempHashedString sNameTH(sName);

  auto itEntry = m_Entries.Find(sNameTH);

  if (!itEntry.IsValid())
  {
    xiiHashedString sNameHS;
    sNameHS.Assign(sName);
    m_Entries[sNameHS].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void xiiBlackboard::SetEntryValue(const xiiHashedString& sName, const xiiVariant& value)
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    m_Entries[sName].m_Value = value;

    ++m_uiBlackboardChangeCounter;
  }
  else
  {
    ImplSetEntryValue(itEntry.Key(), itEntry.Value(), value);
  }
}

void xiiBlackboard::Reflection_SetEntryValue(xiiStringView sName, const xiiVariant& value)
{
  SetEntryValue(sName, value);
}

bool xiiBlackboard::HasEntry(const xiiTempHashedString& sName) const
{
  return m_Entries.Find(sName).IsValid();
}

xiiResult xiiBlackboard::SetEntryFlags(const xiiTempHashedString& sName, xiiBitflags<xiiBlackboardEntryFlags> flags)
{
  auto itEntry = m_Entries.Find(sName);
  if (!itEntry.IsValid())
    return XII_FAILURE;

  itEntry.Value().m_Flags = flags;
  return XII_SUCCESS;
}

const xiiBlackboard::Entry* xiiBlackboard::GetEntry(const xiiTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
    return nullptr;

  return &itEntry.Value();
}

xiiVariant xiiBlackboard::GetEntryValue(const xiiTempHashedString& sName, const xiiVariant& fallback /*= xiiVariant()*/) const
{
  auto pEntry = m_Entries.GetValue(sName);
  return pEntry != nullptr ? pEntry->m_Value : fallback;
}

xiiVariant xiiBlackboard::IncrementEntryValue(const xiiTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    xiiVariant one  = xiiVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value + one;
    return pEntry->m_Value;
  }

  return xiiVariant();
}

xiiVariant xiiBlackboard::DecrementEntryValue(const xiiTempHashedString& sName)
{
  auto pEntry = m_Entries.GetValue(sName);
  if (pEntry != nullptr && pEntry->m_Value.IsNumber())
  {
    xiiVariant one  = xiiVariant(1).ConvertTo(pEntry->m_Value.GetType());
    pEntry->m_Value = pEntry->m_Value - one;
    return pEntry->m_Value;
  }

  return xiiVariant();
}

xiiBitflags<xiiBlackboardEntryFlags> xiiBlackboard::GetEntryFlags(const xiiTempHashedString& sName) const
{
  auto itEntry = m_Entries.Find(sName);

  if (!itEntry.IsValid())
  {
    return xiiBlackboardEntryFlags::Invalid;
  }

  return itEntry.Value().m_Flags;
}

xiiResult xiiBlackboard::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1);

  xiiUInt32 uiEntries = 0;

  for (auto it : m_Entries)
  {
    if (it.Value().m_Flags.IsSet(xiiBlackboardEntryFlags::Save))
    {
      ++uiEntries;
    }
  }

  ref_stream << uiEntries;

  for (auto it : m_Entries)
  {
    const Entry& e = it.Value();

    if (e.m_Flags.IsSet(xiiBlackboardEntryFlags::Save))
    {
      ref_stream << it.Key();
      ref_stream << e.m_Flags;
      ref_stream << e.m_Value;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiBlackboard::Deserialize(xiiStreamReader& ref_stream)
{
  ref_stream.ReadVersion(1);

  xiiUInt32 uiEntries = 0;
  ref_stream >> uiEntries;

  for (xiiUInt32 e = 0; e < uiEntries; ++e)
  {
    xiiHashedString name;
    ref_stream >> name;

    xiiBitflags<xiiBlackboardEntryFlags> flags;
    ref_stream >> flags;

    xiiVariant value;
    ref_stream >> value;

    SetEntryValue(name, value);
    SetEntryFlags(name, flags).AssertSuccess();
  }

  return XII_SUCCESS;
}

// static
xiiBlackboard* xiiBlackboard::Reflection_GetOrCreateGlobal(const xiiHashedString& sName)
{
  return GetOrCreateGlobal(sName).Borrow();
}

// static
xiiBlackboard* xiiBlackboard::Reflection_FindGlobal(xiiTempHashedString sName)
{
  return FindGlobal(sName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiBlackboardCondition, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiBlackboardCondition>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EntryName", m_sEntryName)->AddAttributes(new xiiDynamicStringEnumAttribute("BlackboardKeysEnum")),
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

xiiResult xiiBlackboardCondition::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(s_BlackboardConditionVersion);

  ref_stream << m_sEntryName;
  ref_stream << m_Operator;
  ref_stream << m_fComparisonValue;
  return XII_SUCCESS;
}

xiiResult xiiBlackboardCondition::Deserialize(xiiStreamReader& ref_stream)
{
  const xiiTypeVersion uiVersion = ref_stream.ReadVersion(s_BlackboardConditionVersion);
  XII_IGNORE_UNUSED(uiVersion);

  ref_stream >> m_sEntryName;
  ref_stream >> m_Operator;
  ref_stream >> m_fComparisonValue;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Utils_Implementation_Blackboard);
