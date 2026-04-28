/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>

void xiiWorldRttiConverterContext::Clear()
{
  xiiRttiConverterContext::Clear();

  m_pWorld = nullptr;
  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();

  m_OtherPickingMap.Clear();
  m_ComponentPickingMap.Clear();

  m_UnknownTypes.Clear();
}

void xiiWorldRttiConverterContext::DeleteExistingObjects()
{
  if (m_pWorld == nullptr)
    return;

  m_UnknownTypes.Clear();

  XII_LOCK(m_pWorld->GetWriteMarker());

  const auto& map = m_GameObjectMap.GetHandleToGuidMap();
  while (!map.IsEmpty())
  {
    auto it = map.GetIterator();

    xiiGameObject* pGameObject = nullptr;
    if (m_pWorld->TryGetObject(it.Key(), pGameObject))
    {
      DeleteObject(it.Value());
    }
    else
    {
      m_GameObjectMap.UnregisterObject(it.Value());
    }
  }

  // call base class clear, not the overridden one
  xiiRttiConverterContext::Clear();

  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();
  m_ComponentPickingMap.Clear();
  // Need to do this to make sure all deleted objects are actually deleted as singleton components are
  // still considered alive until Update actually deletes them.
  const bool bSim = m_pWorld->GetWorldSimulationEnabled();
  m_pWorld->SetWorldSimulationEnabled(false);
  m_pWorld->Update();
  m_pWorld->SetWorldSimulationEnabled(bSim);
  // m_OtherPickingMap.Clear(); // do not clear this
}

xiiInternal::NewInstance<void> xiiWorldRttiConverterContext::CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti)
{
  XII_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti == xiiGetStaticRTTI<xiiGameObject>())
  {
    xiiStringBuilder tmp;

    xiiGameObjectDesc d;
    d.m_sName.Assign(xiiConversionUtils::ToString(guid, tmp).GetData());
    d.m_uiStableRandomSeed = xiiHashingUtils::xxHash32(tmp.GetData(), tmp.GetElementCount());

    xiiGameObjectHandle hObject = m_pWorld->CreateObject(d);
    xiiGameObject*      pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      RegisterObject(guid, pRtti, pObject);

      Event e;
      e.m_Type       = Event::Type::GameObjectCreated;
      e.m_ObjectGuid = guid;
      m_Events.Broadcast(e);

      return {pObject, nullptr};
    }
    else
    {
      xiiLog::Error("Failed to create xiiGameObject!");
      return nullptr;
    }
  }
  else if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    xiiComponentManagerBase* pMan = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      xiiLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return nullptr;
    }

    // Component is added via reflection shortly so passing a nullptr as owner is fine here.
    xiiComponentHandle hComponent = pMan->CreateComponent(nullptr);
    xiiComponent*      pComponent;
    if (pMan->TryGetComponent(hComponent, pComponent))
    {
      RegisterObject(guid, pRtti, pComponent);
      return {pComponent, nullptr};
    }
    else
    {
      xiiLog::Error("Component of type '{0}' cannot be found after creation", pRtti->GetTypeName());
      return nullptr;
    }
  }
  else
  {
    return xiiRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void xiiWorldRttiConverterContext::DeleteObject(const xiiUuid& guid)
{
  xiiRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when manipulating scenes during simulation
  // and when creating two components of a type that acts like a singleton (and therefore ignores the second instance creation)
  if (object.m_pObject == nullptr)
    return;

  const xiiRTTI* pRtti = object.m_pType;
  XII_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");

  if (pRtti == xiiGetStaticRTTI<xiiGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    UnregisterObject(guid);
    m_pWorld->DeleteObjectNow(hObject, false);

    Event e;
    e.m_Type       = Event::Type::GameObjectDeleted;
    e.m_ObjectGuid = guid;
    m_Events.Broadcast(e);
  }
  else if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    xiiComponentHandle       hComponent = m_ComponentMap.GetHandle(guid);
    xiiComponentManagerBase* pMan       = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      xiiLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return;
    }

    UnregisterObject(guid);
    pMan->DeleteComponent(hComponent);
  }
  else
  {
    xiiRttiConverterContext::DeleteObject(guid);
  }
}

void xiiWorldRttiConverterContext::RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject)
{
  if (pRtti == xiiGetStaticRTTI<xiiGameObject>())
  {
    xiiGameObject* pGameObject = static_cast<xiiGameObject*>(pObject);
    m_GameObjectMap.RegisterObject(guid, pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    xiiComponent* pComponent = static_cast<xiiComponent*>(pObject);

    XII_ASSERT_DEV(m_pWorld != nullptr && pComponent->GetWorld() == m_pWorld, "Invalid object to register");

    m_ComponentMap.RegisterObject(guid, pComponent->GetHandle());
    pComponent->SetUniqueID(m_uiNextComponentPickingID++);
    m_ComponentPickingMap.RegisterObject(guid, pComponent->GetUniqueID());
  }

  xiiRttiConverterContext::RegisterObject(guid, pRtti, pObject);
}

void xiiWorldRttiConverterContext::UnregisterObject(const xiiUuid& guid)
{
  xiiRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when running a game simulation and the object is destroyed by the game code
  // XII_ASSERT_DEBUG(object.m_pObject, "Failed to retrieve object by guid!");

  if (object.m_pType != nullptr)
  {
    const xiiRTTI* pRtti = object.m_pType;
    if (pRtti == xiiGetStaticRTTI<xiiGameObject>())
    {
      m_GameObjectMap.UnregisterObject(guid);
    }
    else if (pRtti->IsDerivedFrom<xiiComponent>())
    {
      m_ComponentMap.UnregisterObject(guid);
      m_ComponentPickingMap.UnregisterObject(guid);
    }
  }

  xiiRttiConverterContext::UnregisterObject(guid);
}

xiiRttiConverterObject xiiWorldRttiConverterContext::GetObjectByGUID(const xiiUuid& guid) const
{
  xiiRttiConverterObject object = xiiRttiConverterContext::GetObjectByGUID(guid);

  if (!guid.IsValid() || object.m_pType == nullptr)
    return object;

  // We can't look up the ptr via the base class map as it keeps changing, we we need to use the handle.
  if (object.m_pType == xiiGetStaticRTTI<xiiGameObject>())
  {
    auto           hObject     = m_GameObjectMap.GetHandle(guid);
    xiiGameObject* pGameObject = nullptr;
    if (!m_pWorld->TryGetObject(hObject, pGameObject))
    {
      object.m_pObject = nullptr;
      object.m_pType   = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // XII_REPORT_FAILURE("Can't resolve game object GUID!");
      return object;
    }

    // Update new ptr of game object
    if (object.m_pObject != pGameObject)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pGameObject;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  else if (object.m_pType->IsDerivedFrom<xiiComponent>())
  {
    auto          hComponent = m_ComponentMap.GetHandle(guid);
    xiiComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      object.m_pObject = nullptr;
      object.m_pType   = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // XII_REPORT_FAILURE("Can't resolve component GUID!");
      return object;
    }

    // Update new ptr of component
    if (object.m_pObject != pComponent)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pComponent;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  return object;
}

xiiUuid xiiWorldRttiConverterContext::GetObjectGUID(const xiiRTTI* pRtti, const void* pObject) const
{
  if (pRtti == xiiGetStaticRTTI<xiiGameObject>())
  {
    const xiiGameObject* pGameObject = static_cast<const xiiGameObject*>(pObject);
    return m_GameObjectMap.GetGuid(pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    const xiiComponent* pComponent = static_cast<const xiiComponent*>(pObject);
    return m_ComponentMap.GetGuid(pComponent->GetHandle());
  }
  return xiiRttiConverterContext::GetObjectGUID(pRtti, pObject);
}

void xiiWorldRttiConverterContext::OnUnknownTypeError(xiiStringView sTypeName)
{
  xiiRttiConverterContext::OnUnknownTypeError(sTypeName);

  m_UnknownTypes.Insert(sTypeName);
}
