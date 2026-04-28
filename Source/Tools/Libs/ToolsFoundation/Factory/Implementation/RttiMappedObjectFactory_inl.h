/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename Object>
xiiRttiMappedObjectFactory<Object>::xiiRttiMappedObjectFactory() = default;

template <typename Object>
xiiRttiMappedObjectFactory<Object>::~xiiRttiMappedObjectFactory() = default;

template <typename Object>
void xiiRttiMappedObjectFactory<Object>::RegisterCreator(const xiiRTTI* pType, CreateObjectFunc creator)
{
  XII_ASSERT_DEV(!m_Creators.Contains(pType), "Type already registered.");

  m_Creators.Insert(pType, creator);
  Event e;
  e.m_Type      = Event::Type::CreatorAdded;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
}

template <typename Object>
void xiiRttiMappedObjectFactory<Object>::UnregisterCreator(const xiiRTTI* pType)
{
  XII_ASSERT_DEV(m_Creators.Contains(pType), "Type was never registered.");
  m_Creators.Remove(pType);

  Event e;
  e.m_Type      = Event::Type::CreatorRemoved;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
}

template <typename Object>
Object* xiiRttiMappedObjectFactory<Object>::CreateObject(const xiiRTTI* pType)
{
  CreateObjectFunc* creator = nullptr;
  while (pType != nullptr)
  {
    if (m_Creators.TryGetValue(pType, creator))
    {
      return (*creator)(pType);
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}
