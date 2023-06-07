#pragma once

template <typename KEY, typename VALUE>
xiiObjectMetaData<KEY, VALUE>::xiiObjectMetaData()
{
  m_DefaultValue = VALUE();

  auto pStorage           = XII_DEFAULT_NEW(Storage);
  pStorage->m_AcessingKey = KEY();
  pStorage->m_AccessMode  = Storage::AccessMode::Nothing;
  SwapStorage(pStorage);
}

template <typename KEY, typename VALUE>
const VALUE* xiiObjectMetaData<KEY, VALUE>::BeginReadMetaData(const KEY objectKey) const
{
  m_pMetaStorage->m_Mutex.Lock();
  XII_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");
  m_pMetaStorage->m_AccessMode  = Storage::AccessMode::Read;
  m_pMetaStorage->m_AcessingKey = ObjectKey;

  const VALUE* pRes = nullptr;
  if (m_pMetaStorage->m_MetaData.TryGetValue(ObjectKey, pRes)) // TryGetValue is not const correct with the second parameter
    return pRes;

  return &m_DefaultValue;
}

template <typename KEY, typename VALUE>
void xiiObjectMetaData<KEY, VALUE>::ClearMetaData(const KEY objectKey)
{
  XII_LOCK(m_pMetaStorage->m_Mutex);
  XII_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");

  if (HasMetaData(ObjectKey))
  {
    m_pMetaStorage->m_MetaData.Remove(ObjectKey);

    EventData e;
    e.m_ObjectKey = ObjectKey;
    e.m_pValue    = &m_DefaultValue;

    m_pMetaStorage->m_DataModifiedEvent.Broadcast(e);
  }
}

template <typename KEY, typename VALUE>
bool xiiObjectMetaData<KEY, VALUE>::HasMetaData(const KEY objectKey) const
{
  XII_LOCK(m_pMetaStorage->m_Mutex);
  const VALUE* pValue = nullptr;
  return m_pMetaStorage->m_MetaData.TryGetValue(ObjectKey, pValue);
}

template <typename KEY, typename VALUE>
VALUE* xiiObjectMetaData<KEY, VALUE>::BeginModifyMetaData(const KEY objectKey)
{
  m_pMetaStorage->m_Mutex.Lock();
  XII_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");
  m_pMetaStorage->m_AccessMode  = Storage::AccessMode::Write;
  m_pMetaStorage->m_AcessingKey = ObjectKey;

  return &m_pMetaStorage->m_MetaData[ObjectKey];
}

template <typename KEY, typename VALUE>
void xiiObjectMetaData<KEY, VALUE>::EndReadMetaData() const
{
  XII_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Read, "Not accessing data at the moment");

  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Nothing;
  m_pMetaStorage->m_Mutex.Unlock();
}


template <typename KEY, typename VALUE>
void xiiObjectMetaData<KEY, VALUE>::EndModifyMetaData(xiiUInt32 uiModifiedFlags /*= 0xFFFFFFFF*/)
{
  XII_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Write, "Not accessing data at the moment");
  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Nothing;

  if (uiModifiedFlags != 0)
  {
    EventData e;
    e.m_ObjectKey       = m_pMetaStorage->m_AcessingKey;
    e.m_pValue          = &m_pMetaStorage->m_MetaData[m_pMetaStorage->m_AcessingKey];
    e.m_uiModifiedFlags = uiModifiedFlags;

    m_pMetaStorage->m_DataModifiedEvent.Broadcast(e);
  }

  m_pMetaStorage->m_Mutex.Unlock();
}


template <typename KEY, typename VALUE>
void xiiObjectMetaData<KEY, VALUE>::AttachMetaDataToAbstractGraph(xiiAbstractObjectGraph& ref_graph) const
{
  auto& AllNodes = graph.GetAllNodes();

  XII_LOCK(m_pMetaStorage->m_Mutex);

  xiiHashTable<const char*, xiiVariant> DefaultValues;

  // store the default values in an easily accessible hash map, to be able to compare against them
  {
    DefaultValues.Reserve(m_DefaultValue.GetDynamicRTTI()->GetProperties().GetCount());

    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != xiiPropertyCategory::Member)
        continue;

      DefaultValues[pProp->GetPropertyName()] =
        xiiReflectionUtils::GetMemberPropertyValue(static_cast<xiiAbstractMemberProperty*>(pProp), &m_DefaultValue);
    }
  }

  // now serialize all properties that differ from the default value
  {
    xiiVariant value;

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      auto*          pNode = it.Value();
      const xiiUuid& guid  = pNode->GetGuid();

      const VALUE* pMeta = nullptr;
      if (!m_pMetaStorage->m_MetaData.TryGetValue(guid, pMeta)) // TryGetValue is not const correct with the second parameter
        continue;                                               // it is the default object, so all values are default -> skip

      for (const auto& pProp : pMeta->GetDynamicRTTI()->GetProperties())
      {
        if (pProp->GetCategory() != xiiPropertyCategory::Member)
          continue;

        value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<xiiAbstractMemberProperty*>(pProp), pMeta);

        if (value.IsValid() && DefaultValues[pProp->GetPropertyName()] != value)
        {
          pNode->AddProperty(pProp->GetPropertyName(), value);
        }
      }
    }
  }
}


template <typename KEY, typename VALUE>
void xiiObjectMetaData<KEY, VALUE>::RestoreMetaDataFromAbstractGraph(const xiiAbstractObjectGraph& graph)
{
  XII_LOCK(m_pMetaStorage->m_Mutex);

  xiiHybridArray<xiiString, 16> PropertyNames;

  // find all properties (names) that we want to read
  {
    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != xiiPropertyCategory::Member)
        continue;

      PropertyNames.PushBack(pProp->GetPropertyName());
    }
  }

  auto& AllNodes = graph.GetAllNodes();

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto*          pNode = it.Value();
    const xiiUuid& guid  = pNode->GetGuid();

    for (const auto& name : PropertyNames)
    {
      if (const auto* pProp = pNode->FindProperty(name))
      {
        VALUE* pValue = &m_pMetaStorage->m_MetaData[guid];

        xiiReflectionUtils::SetMemberPropertyValue(
          static_cast<xiiAbstractMemberProperty*>(pValue->GetDynamicRTTI()->FindPropertyByName(name)), pValue, pProp->m_Value);
      }
    }
  }
}

template <typename KEY, typename VALUE>
xiiSharedPtr<typename xiiObjectMetaData<KEY, VALUE>::Storage> xiiObjectMetaData<KEY, VALUE>::SwapStorage(xiiSharedPtr<typename xiiObjectMetaData<KEY, VALUE>::Storage> pNewStorage)
{
  XII_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pMetaStorage;

  m_EventsUnsubscriber.Unsubscribe();

  m_pMetaStorage = pNewStorage;

  m_pMetaStorage->m_DataModifiedEvent.AddEventHandler([this](const EventData& e) { m_DataModifiedEvent.Broadcast(e); }, m_EventsUnsubscriber);

  return retVal;
}
